"""
Общее окно: управление протезом Indy (UART) + чтение платы датчиков TacMagVTOS
(второй UART, BLE HM-10 или имитация) + запись ВСЕХ значений в один CSV.

Запуск (из папки tools):
  py -3 combined_gui.py --sensor-port COM21                     # протез COM20 + датчики по UART (сырые оси)
  py -3 combined_gui.py --sensor-ble HM-10-1                    # протез COM20 + датчики по BLE
  py -3 combined_gui.py --sensor-port COM21 --fsr               # + FSR
  py -3 combined_gui.py --sensor-port COM21 --cells 0,1         # только THUMB и INDEX
  py -3 combined_gui.py --sensor-port COM21 --sensor-mode fingers   # кадры 0xAA (давление)
  py -3 combined_gui.py --hand-port COM7 --sensor-port COM21    # другой порт протеза
  py -3 combined_gui.py --no-hand --sensor-mock                 # без железа (проверка)
  ... --csv rec.csv                                             # писать сразу с запуска

Режимы датчиков:
  axes (по умолчанию) — кадры 0xAC: сырые оси всех MLX90393 по ячейкам (+ FSR с --fsr).
                        Графики как в raw_monitor.py: строка — ячейка, столбец — ось, 4 линии MLX;
                        флажки ячеек (меняют маску в прошивке), осей, MLX, мкТл/LSB.
  fingers             — кадры 0xAA: раскрытие + давление на 3 пальца (команда 'F')

CSV (одна таблица, строки трёх видов, колонка event):
  sensor — кадр датчиков + состояние протеза на этот момент
  cmd    — команда, отправленная протезу
  hand   — состояние протеза 20 раз/с (чтобы запись шла и без датчиков)
"""
import argparse
import collections
import csv
import math
import os
import struct
import sys
import threading
import time
import tkinter as tk

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from sensor_link import (AXES, AXES_MASK_FSR, FINGERS_FMT, NUM_CELLS, NUM_MLX, BleLink, FrameParser,
                         SerialLink, axes_frame_len, build_axes_frame, crc8 as s_crc8)

CELLS = ['THUMB', 'INDEX', 'MIDDLE']
AXIS_NAMES = ['X', 'Y', 'Z']
FSR_NAMES = ['FSR1_THUMB', 'FSR2_THUMB', 'FSR1_INDEX', 'FSR2_INDEX', 'FSR1_MIDDLE', 'FSR2_MIDDLE']
HISTORY_S = 10.0

HAND_COLS = ['hand_linked', 'hand_force', 'hand_target', 'hand_speed', 'hand_pos_est', 'hand_moving']
FINGER_COLS = ['aperture'] + [f'press_{c}' for c in CELLS]
MAG_COLS = [f'{c}_MLX{m}_{a}' for c in CELLS for m in range(NUM_MLX) for a in AXIS_NAMES]
RAW_COLS = [f'raw_ch{i}' for i in range(9)] + ['raw_hall', 'raw_aperture']
COLUMNS = (['t_s', 'event'] + HAND_COLS + ['cmd_raw', 'cmd_note', 'frame']
           + FINGER_COLS + MAG_COLS + FSR_NAMES + RAW_COLS)


# ======================================================== имитация платы датчиков
class MockSensorLink:
    """Понимает те же команды, что прошивка: 'F', 'R', 0x80|mask, 0x90|k."""
    budget_bps = 8000

    def __init__(self, on_data):
        self.on_data = on_data
        self.mode, self.mask, self.period = 'F', 0, 0.02
        self.alive = True
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        t0 = time.time()
        while self.alive:
            t = time.time() - t0
            if self.mode == 'F':
                press = [int(max(0, 2000 * math.sin(0.6 * t + i))) for i in range(3)]
                body = struct.pack('<HHHH', int(500 + 400 * math.sin(0.3 * t)), *press)
                self.on_data(bytes([0xAA]) + body + bytes([s_crc8(body)]))
                time.sleep(0.02)
            else:
                if self.mask:
                    mag = [[[int(300 * math.sin(0.7 * t + c + 0.3 * m + a) + 40 * m) for a in range(AXES)]
                            for m in range(NUM_MLX)] for c in range(NUM_CELLS)]
                    fsr = [int(max(0, 3000 * math.sin(0.5 * t + i))) for i in range(6)]
                    self.on_data(build_axes_frame(self.mask, mag, fsr))
                time.sleep(max(self.period, 0.01))

    def write(self, data):
        for c in data:
            if c in (ord('F'), ord('R')):
                self.mode = chr(c)
            elif c & 0xF0 == 0x80:
                self.mode, self.mask = 'A', c & 0x0F
            elif c & 0xF0 == 0x90:
                self.period = (c & 0x0F) * 0.01

    def close(self):
        self.alive = False


# ======================================================== плата датчиков
class SensorSide:
    def __init__(self, link_factory, mode, mask, period_ms, on_frame):
        self.mode, self.mask, self.period_ms = mode, mask, period_ms
        self.on_frame = on_frame
        self.parser = FrameParser()
        self.lock = threading.Lock()
        self.n_frames = 0
        self.last = None              # (kind, dict)
        self.last_good_t = 0.0
        self.last_cfg_t = 0.0
        self.rate, self._rate_t, self._rate_n = 0.0, time.time(), 0
        self.link = link_factory(self._on_data)
        self.send_config()

    def want_kind(self):
        return 'fingers' if self.mode == 'fingers' else 'axes'

    def send_config(self):
        if self.mode == 'fingers':
            self.link.write(b'F')
        else:
            if self.period_ms:
                ms = self.period_ms
            else:
                ms = axes_frame_len(self.mask) * 1000.0 / self.link.budget_bps
            k = int(min(15, max(1, math.ceil(ms / 10.0))))
            self.link.write(bytes([0x90 | k]))
            time.sleep(0.05)          # прошивка читает по байту за цикл
            self.link.write(bytes([0x80 | self.mask]))
        self.last_cfg_t = time.time()

    def _on_data(self, data):
        with self.lock:
            frames = self.parser.feed(data)
        now = time.time()
        for kind, f in frames:
            self.n_frames += 1
            self.last = (kind, f)
            if kind == self.want_kind():
                self.last_good_t = now
            self.on_frame(now, kind, f)

    def tick(self):
        """Вызывается из GUI: частота кадров и повтор настройки, если плата не в нужном режиме."""
        now = time.time()
        if now - self._rate_t >= 1.0:
            self.rate = (self.n_frames - self._rate_n) / (now - self._rate_t)
            self._rate_t, self._rate_n = now, self.n_frames
        if now - self.last_good_t > 1.5 and now - self.last_cfg_t > 1.5:
            self.send_config()
        # команда смены маски могла потеряться (прошивка читает байт за цикл) — повторяем
        elif (self.mode == 'axes' and self.last and self.last[0] == 'axes'
              and self.last[1]['mask'] != self.mask and now - self.last_cfg_t > 1.0):
            self.send_config()

    def close(self):
        try:
            if self.mode != 'fingers':
                self.link.write(b'F')     # вернуть плату в обычный режим
        finally:
            self.link.close()


# ======================================================== запись
class Recorder:
    def __init__(self):
        self.f = None
        self.w = None
        self.path = None
        self.rows = 0
        self.lock = threading.Lock()
        self.t0 = time.time()
        self._last_flush = 0.0

    def start(self, path=None):
        with self.lock:
            if self.f:
                return
            self.path = path or os.path.join(HERE, f'rec_{time.strftime("%Y%m%d_%H%M%S")}.csv')
            self.f = open(self.path, 'w', newline='', encoding='utf-8-sig')
            self.w = csv.DictWriter(self.f, fieldnames=COLUMNS, extrasaction='ignore')
            self.w.writeheader()
            self.rows = 0
            self.t0 = time.time()

    def stop(self):
        with self.lock:
            if self.f:
                self.f.close()
            self.f = self.w = None

    @property
    def active(self):
        return self.f is not None

    def write(self, t_abs, row):
        with self.lock:
            if not self.w:
                return
            row['t_s'] = f'{t_abs - self.t0:.4f}'
            self.w.writerow(row)
            self.rows += 1
            if t_abs - self._last_flush > 0.5:
                self.f.flush()
                self._last_flush = t_abs


def frame_to_row(kind, f):
    row = {'frame': kind}
    if kind == 'fingers':
        row['aperture'] = f['aperture']
        for c, v in zip(CELLS, f['press']):
            row[f'press_{c}'] = v
    elif kind == 'axes':
        for ci, cell in enumerate(f['mag']):
            if cell is None:
                continue
            for m in range(NUM_MLX):
                for a in range(AXES):
                    row[f'{CELLS[ci]}_MLX{m}_{AXIS_NAMES[a]}'] = cell[m][a]
        if f['fsr']:
            for n, v in zip(FSR_NAMES, f['fsr']):
                row[n] = v
    elif kind == 'raw':
        for i, v in enumerate(f['ch']):
            row[f'raw_ch{i}'] = v
        row['raw_hall'] = f['hall']
        row['raw_aperture'] = f['aperture']
    return row


# ======================================================== окно
class App:
    def __init__(self, args):
        self.args = args
        self.rec = Recorder()
        self.t0 = time.time()
        self.hand = None
        self.sensor = None
        self.cmd_log = collections.deque(maxlen=300)
        self.sens_hist = collections.deque(maxlen=int(HISTORY_S * 150))   # (t, vector)
        self.hand_hist = collections.deque(maxlen=int(HISTORY_S * 25))    # (t, pos, target)
        self.hist_lock = threading.Lock()

        self.root = tk.Tk()
        self.root.title('Протез + датчики')
        self.root.geometry('1500x900+30+20')
        self._build_ui()

        if not args.no_hand:
            from hand_link import HandController
            try:
                self.hand = HandController(args.hand_port, on_cmd=self._on_hand_cmd)
                self.hand.on_target_reset = lambda v: self.root.after(0, lambda: self.s_target.set(v))
            except Exception as e:
                self.hand_status.config(text=f'Протез: ошибка {args.hand_port}: {e}', fg='red')
                print(f'Протез: не удалось открыть {args.hand_port}: {e}')

        factory = None
        if args.sensor_port:
            factory = lambda cb: SerialLink(args.sensor_port, args.sensor_baud, cb)
            self.sensor_name = f'UART {args.sensor_port}'
        elif args.sensor_ble:
            factory = lambda cb: BleLink(args.sensor_ble, cb)
            self.sensor_name = f'BLE {args.sensor_ble}'
        elif args.sensor_mock:
            factory = MockSensorLink
            self.sensor_name = 'имитация'
        if factory:
            mask = sum(1 << c for c in args.cells) | (AXES_MASK_FSR if args.fsr else 0)
            try:
                self.sensor = SensorSide(factory, args.sensor_mode, mask, args.period_ms, self._on_sensor_frame)
            except Exception as e:
                self.sens_status.config(text=f'Датчики: ошибка: {e}', fg='red')
                print(f'Датчики: ошибка подключения: {e}')

        if args.csv or args.selftest:
            self.toggle_rec(path=args.csv)

        threading.Thread(target=self._hand_sampler, daemon=True).start()
        self.root.protocol('WM_DELETE_WINDOW', self.on_close)
        self.root.after(100, self.ui_tick)
        if args.selftest:
            self.root.after(int(args.selftest * 1000), self.on_close)

    # ---------------- данные
    def _hand_state(self):
        return self.hand.state() if self.hand else {}

    def _on_hand_cmd(self, t_abs, raw, note):
        self.cmd_log.append(f'{t_abs - self.t0:8.2f}s  {raw.hex(" ").upper():18} {note}')
        row = {'event': 'cmd', 'cmd_raw': raw.hex(' '), 'cmd_note': note}
        row.update(self._hand_state())
        self.rec.write(t_abs, row)

    def _on_sensor_frame(self, t_abs, kind, f):
        row = {'event': 'sensor'}
        row.update(self._hand_state())
        row.update(frame_to_row(kind, f))
        self.rec.write(t_abs, row)
        # на график — только кадры выбранного режима (после переключения плата
        # ещё успевает прислать хвост кадров старого режима); в файл пишется всё
        want = 'fingers' if self.args.sensor_mode == 'fingers' else 'axes'
        if kind != want:
            return
        if kind == 'fingers':
            item = np.array([f['aperture']] + list(f['press']), float)
        else:
            # все оси всех магнитометров: (ячейка, MLX, ось); NaN — ячейка не передавалась
            item = np.full((NUM_CELLS, NUM_MLX, AXES), np.nan)
            for c, cell in enumerate(f['mag']):
                if cell is not None:
                    item[c] = cell
        with self.hist_lock:
            self.sens_hist.append((t_abs - self.t0, item))

    def _hand_sampler(self):
        while True:
            t = time.time()
            if self.hand:
                st = self.hand.state()
                with self.hist_lock:
                    self.hand_hist.append((t - self.t0, st['hand_pos_est'], st['hand_target']))
                row = {'event': 'hand'}
                row.update(st)
                self.rec.write(t, row)
            time.sleep(0.05)

    # ---------------- интерфейс
    def _build_ui(self):
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        from matplotlib.figure import Figure
        F, FB = ('Segoe UI', 12), ('Segoe UI', 13, 'bold')
        root = self.root

        left = tk.Frame(root)
        left.pack(side='left', fill='y', padx=8, pady=8)
        tk.Label(left, text='ПРОТЕЗ (UART)', font=FB).pack()
        self.hand_status = tk.Label(left, text='Протез: жду опроса платы...', font=F, fg='gray')
        self.hand_status.pack(pady=4)

        sl = tk.Frame(left)
        sl.pack()

        def slider(title, color, lo, hi, init, setter):
            fr = tk.Frame(sl)
            fr.pack(side='left', padx=4)
            tk.Label(fr, text=title, font=F, fg=color).pack()
            sc = tk.Scale(fr, from_=hi, to=lo, orient='vertical', length=300, width=32, font=F,
                          troughcolor=color, command=lambda v: setter(float(v)))
            sc.set(init)
            sc.pack()
            return sc

        def set_hand(attr, conv):
            return lambda v: self.hand and setattr(self.hand, attr, conv(v))

        self.s_force = slider('Сила, %', '#d9534f', 10, 100, 60, set_hand('force', int))
        self.s_target = slider('Сжатие, %', '#5cb85c', 0, 100, 0, set_hand('target', float))
        self.s_speed = slider('Скорость, %', '#0275d8', 5, 100, 40, set_hand('speed', int))

        for text, cmd in (('Открыть полностью (калибровка)', lambda: self.hand and self.hand.calibrate()),
                          ('Цель 0', lambda: self.s_target.set(0)),
                          ('Цель 100', lambda: self.s_target.set(100)),
                          ('Дожать до упора', lambda: self.hand and self.hand.push())):
            tk.Button(left, text=text, font=F, width=28, command=cmd).pack(pady=2)
        tk.Button(left, text='СТОП', font=FB, bg='tomato', width=24,
                  command=lambda: self.hand and self.hand.stop_all()).pack(pady=8)

        self.rec_btn = tk.Button(left, text='● Начать запись', font=FB, bg='#f0f0f0', width=24,
                                 command=self.toggle_rec)
        self.rec_btn.pack(pady=(16, 4))
        self.rec_lbl = tk.Label(left, text='запись выключена', font=('Segoe UI', 10), wraplength=330,
                                justify='left')
        self.rec_lbl.pack()

        tk.Label(left, text='Команды протезу:', font=F).pack(anchor='w', pady=(10, 0))
        self.cmd_txt = tk.Text(left, height=10, width=46, font=('Consolas', 9))
        self.cmd_txt.pack(fill='both', expand=True)

        right = tk.Frame(root)
        right.pack(side='left', fill='both', expand=True, padx=8, pady=8)
        top = tk.Frame(right)
        top.pack(fill='x')
        tk.Label(top, text='ДАТЧИКИ', font=FB).pack(anchor='w')
        self.sens_status = tk.Label(top, text='Датчики: не подключены', font=F, fg='gray', justify='left')
        self.sens_status.pack(anchor='w')

        row = tk.Frame(top)
        row.pack(fill='x')
        # флажки как в raw_monitor.py: ячейки (и в прошивке), оси, магнитометры, единицы
        self.cells_on = [tk.BooleanVar(value=c in self.args.cells) for c in range(NUM_CELLS)]
        self.axes_on = [tk.BooleanVar(value=True) for _ in range(AXES)]
        self.mlx_on = [tk.BooleanVar(value=True) for _ in range(NUM_MLX)]
        self.ut_on = tk.BooleanVar(value=False)
        FS = ('Segoe UI', 10)

        def group(title, names, vars_, cmd):
            fr = tk.LabelFrame(row, text=title, font=('Segoe UI', 10, 'bold'))
            fr.pack(side='left', fill='y', padx=3)
            for n, v in zip(names, vars_):
                tk.Checkbutton(fr, text=n, variable=v, font=FS, command=cmd).pack(anchor='w')

        group('Ячейки', CELLS, self.cells_on, self._cells_changed)
        group('Оси', AXIS_NAMES, self.axes_on, self._layout_plots)
        group('Магнитометры', [f'MLX{m}' for m in range(NUM_MLX)], self.mlx_on, self._layout_plots)
        group('Единицы', ['мкТл (иначе LSB)'], [self.ut_on], self._layout_plots)
        self.vals_txt = tk.Text(row, height=16, width=64, font=('Consolas', 10))
        self.vals_txt.pack(side='left', fill='both', expand=True, padx=3)

        self.fig = Figure(figsize=(11, 7), dpi=85)
        self.canvas = FigureCanvasTkAgg(self.fig, master=right)
        self.canvas.get_tk_widget().pack(fill='both', expand=True)
        self._last_draw = 0.0
        self._layout_plots()

    # ---------------- графики
    LSB_UT = np.array([0.150, 0.150, 0.242])   # мкТл/LSB при GAIN_1X, RES_16 (как в raw_monitor.py)

    def _layout_plots(self):
        """Сетка: строка — ячейка, столбец — ось, 4 линии MLX; внизу — протез."""
        from matplotlib.gridspec import GridSpec
        self.fig.clear()
        colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
        unit = 'мкТл' if self.ut_on.get() else 'LSB'
        self.mag_axes = {}
        self.mag_lines = {}
        self.fing_ax = None
        if self.args.sensor_mode == 'axes':
            rows = [c for c in range(NUM_CELLS) if self.cells_on[c].get()]
            cols = [a for a in range(AXES) if self.axes_on[a].get()] or [0]
        else:
            rows, cols = [0], [0]
        gs = GridSpec(len(rows) + 1, len(cols), figure=self.fig,
                      height_ratios=[1] * len(rows) + [0.7], hspace=0.45, wspace=0.25,
                      left=0.06, right=0.99, top=0.96, bottom=0.07)
        first = None
        if self.args.sensor_mode == 'axes':
            for r, c in enumerate(rows):
                for k, a in enumerate(cols):
                    ax = self.fig.add_subplot(gs[r, k], sharex=first)
                    first = first or ax
                    ax.set_title(f'{CELLS[c]} {AXIS_NAMES[a]}, {unit}', fontsize=9)
                    ax.tick_params(labelsize=7)
                    ax.grid(alpha=0.3)
                    self.mag_axes[(c, a)] = ax
                    self.mag_lines[(c, a)] = [ax.plot([], [], color=colors[m], lw=1, label=f'MLX{m}',
                                                      visible=self.mlx_on[m].get())[0]
                                              for m in range(NUM_MLX)]
            if self.mag_axes:
                next(iter(self.mag_axes.values())).legend(fontsize=7, loc='upper left', ncol=4)
        else:
            ax = self.fig.add_subplot(gs[0, :])
            first = ax
            ax.set_title('кадр 0xAA: раскрытие и давление', fontsize=9)
            ax.grid(alpha=0.3)
            names = ['раскрытие'] + [f'давление {c}' for c in CELLS]
            self.fing_lines = [ax.plot([], [], lw=1.2, label=n)[0] for n in names]
            ax.legend(fontsize=7, loc='upper left', ncol=4)
            self.fing_ax = ax
        self.ax_h = self.fig.add_subplot(gs[-1, :], sharex=first)
        self.ax_h.set_ylim(-5, 105)
        self.ax_h.set_title('протез: оценка положения и цель, %', fontsize=9)
        self.ax_h.set_xlabel('время, с', fontsize=8)
        self.ax_h.tick_params(labelsize=7)
        self.ax_h.grid(alpha=0.3)
        self.h_pos, = self.ax_h.plot([], [], color='#5cb85c', lw=2, label='оценка положения')
        self.h_tgt, = self.ax_h.plot([], [], color='black', lw=1, ls='--', label='цель')
        self.ax_h.legend(loc='upper left', fontsize=7)
        self.canvas.draw_idle()

    def _cells_changed(self):
        """Ячейки выключаются и в прошивке (маска кадра 0xAC) — кадр короче, частота выше."""
        if self.sensor and self.args.sensor_mode == 'axes':
            mask = sum(1 << c for c in range(NUM_CELLS) if self.cells_on[c].get())
            self.sensor.mask = mask | (self.sensor.mask & AXES_MASK_FSR)
            threading.Thread(target=self.sensor.send_config, daemon=True).start()
        self._layout_plots()

    def toggle_rec(self, path=None):
        if self.rec.active:
            self.rec.stop()
            self.rec_btn.config(text='● Начать запись', bg='#f0f0f0')
            self.rec_lbl.config(text=f'сохранено: {self.rec.path}\nстрок: {self.rec.rows}')
        else:
            self.rec.start(path)
            self.rec_btn.config(text='■ Остановить запись', bg='#ff9999')

    def ui_tick(self):
        now = time.time()
        if self.hand:
            if self.hand.linked:
                self.hand_status.config(text=f'Протез: связь есть ({self.args.hand_port})', fg='green')
            mv = {1: 'сжимает', -1: 'разжимает', 0: 'стоит'}[self.hand.moving]
            self.hand_status.config(text=self.hand_status.cget('text').split('  |')[0]
                                    + f'  |  {self.hand.pos:.0f}% — {mv}')
        while self.cmd_log:
            self.cmd_txt.insert('end', self.cmd_log.popleft() + '\n')
            self.cmd_txt.see('end')

        if self.sensor:
            self.sensor.tick()
            s = self.sensor
            self.sens_status.config(
                text=f'Датчики: {self.sensor_name}, режим {s.mode}, {s.rate:.1f} кадр/с, '
                     f'CRC ошибок {s.parser.crc_errors}', fg='green' if s.rate > 0 else 'orange')
            if s.last:
                self.vals_txt.delete('1.0', 'end')
                self.vals_txt.insert('end', self._format_frame(*s.last))

        if self.rec.active:
            self.rec_lbl.config(text=f'пишу: {self.rec.path}\nстрок: {self.rec.rows}')

        if now - self._last_draw > 0.2:
            self._last_draw = now
            self._redraw()
        self.root.after(50, self.ui_tick)

    @staticmethod
    def _format_frame(kind, f):
        if kind == 'fingers':
            return (f'кадр 0xAA (fingers)\nраскрытие: {f["aperture"]}\n'
                    + '\n'.join(f'давление {c:<7}: {v}' for c, v in zip(CELLS, f['press'])))
        if kind == 'raw':
            return f'кадр 0xAB (raw)\nch: {f["ch"]}\nhall: {f["hall"]}  aperture: {f["aperture"]}'
        lines = [f'кадр 0xAC (axes), маска 0x{f["mask"]:02X}        X       Y       Z']
        for ci, cell in enumerate(f['mag']):
            if cell is None:
                continue
            for m in range(NUM_MLX):
                lines.append(f'{CELLS[ci]:<7} MLX{m}            ' + ''.join(f'{v:8d}' for v in cell[m]))
        if f['fsr']:
            lines.append('FSR: ' + '  '.join(f'{n}={v}' for n, v in zip(FSR_NAMES, f['fsr'])))
        return '\n'.join(lines)

    def _redraw(self):
        with self.hist_lock:
            sh = list(self.sens_hist)
            hh = list(self.hand_hist)
        t_now = time.time() - self.t0
        want_shape = (NUM_CELLS, NUM_MLX, AXES) if self.args.sensor_mode == 'axes' else (4,)
        sh = [x for x in sh if x[1].shape == want_shape]
        if sh:
            t = np.array([x[0] for x in sh])
            v = np.stack([x[1] for x in sh])
            if self.args.sensor_mode == 'axes':
                if self.ut_on.get():
                    v = v * self.LSB_UT
                for (c, a), ax in self.mag_axes.items():
                    for m, ln in enumerate(self.mag_lines[(c, a)]):
                        ln.set_visible(self.mlx_on[m].get())
                        ln.set_data(t, v[:, c, m, a])
                    ax.relim(visible_only=True)
                    ax.autoscale_view(scalex=False)
            elif self.fing_ax is not None:
                for i, ln in enumerate(self.fing_lines):
                    ln.set_data(t, v[:, i])
                self.fing_ax.relim()
                self.fing_ax.autoscale_view(scalex=False)
        if hh:
            t = np.array([x[0] for x in hh])
            self.h_pos.set_data(t, [x[1] for x in hh])
            self.h_tgt.set_data(t, [x[2] for x in hh])
        self.ax_h.set_xlim(max(0, t_now - HISTORY_S), t_now + 0.2)
        self.canvas.draw_idle()

    def on_close(self):
        self.rec.stop()
        if self.hand:
            self.hand.close()
        if self.sensor:
            self.sensor.close()
        if self.rec.path:
            print(f'запись: {self.rec.path}, строк: {self.rec.rows}')
        self.root.destroy()

    def run(self):
        self.root.mainloop()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20', help='COM-порт протеза (по умолчанию COM20)')
    p.add_argument('--no-hand', action='store_true', help='без протеза')
    src = p.add_mutually_exclusive_group()
    src.add_argument('--sensor-port', help='COM-порт платы датчиков')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10 платы датчиков')
    src.add_argument('--sensor-mock', action='store_true', help='имитация платы датчиков')
    p.add_argument('--sensor-baud', type=int, default=115200)
    p.add_argument('--sensor-mode', choices=['fingers', 'axes'], default='axes',
                   help='axes (по умолчанию) — сырые оси всех MLX; fingers — кадры 0xAA')
    p.add_argument('--cells', default='0,1,2', help='ячейки для режима axes: 0=THUMB,1=INDEX,2=MIDDLE')
    p.add_argument('--fsr', action='store_true', help='в режиме axes передавать FSR')
    p.add_argument('--period-ms', type=int, help='период кадра axes 10..150 мс')
    p.add_argument('--csv', help='начать запись сразу в этот файл')
    p.add_argument('--selftest', type=float, help=argparse.SUPPRESS)
    a = p.parse_args()
    a.cells = [int(x) for x in a.cells.split(',') if x.strip()]
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    App(a).run()


if __name__ == '__main__':
    main()
