"""
Управление кистью по UART (порт датчика ЭМГ, 115200) с регулировкой силы и
плавным сжатием/разжатием.

Ползунки:
  Сила, %          — процент ШИМ в команде закрытия 01 11 01 <pct> (на этой плате 0x11 закрывает)
  Сжатие (цель), % — 0 открыто .. 100 закрыто; кисть следует за ползунком
  Скорость, %      — программный ШИМ: доля времени работы мотора в окне 120 мс

Положение кисти НЕ измеряется — оценивается по времени работы мотора
(полный ход ~0.75 c). Кнопка «Открыть полностью» сбрасывает оценку в 0.

Направления на этой плате инвертированы относительно документации:
  0x10 — открыть, 0x0F — закрыть, 0x11 <pct> — закрыть с ШИМ, 0x13 — стоп.

  py -3 hand_gui.py [COM]
Лог команд: tools/hand_gui_<время>.csv
"""
import csv
import os
import queue
import sys
import threading
import time
import tkinter as tk

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe_board import crc8

PORT = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('UART_PORT', 'COM20')

FULL_STROKE_S = 0.75             # полный ход, с работы мотора (измерено)
RATE = 100.0 / FULL_STROKE_S     # % хода в секунду
WINDOW = 0.12                    # окно программного ШИМ, с
DEADBAND = 1.5                   # %, ближе к цели — не двигаемся
START_LOSS = 0.015               # с, теряется на разгон при каждом старте мотора
PUSH_S = 0.6                     # с, дожим до упора в крайних положениях (0 и 100)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   f'hand_gui_{time.strftime("%Y%m%d_%H%M%S")}.csv')


def pack(cmd, data=()):
    p = [0x01, cmd, len(data), *data]
    return bytes(p + [crc8(p)])


CMD_OPEN = pack(0x10)
CMD_STOP = pack(0x13)


def cmd_close(force_pct):
    return pack(0x11, [max(1, min(100, int(force_pct)))])


st = {'force': 60, 'target': 0.0, 'speed': 40, 'pos': 0.0, 'run': True,
      'linked': False, 'moving': 0, 'calib': False, 'stop_all': False,
      'pushed': None, 'push_req': False}
lock = threading.Lock()
ui_q = queue.Queue()
log_rows = []
t0 = time.time()
ser = serial.Serial(PORT, 115200, timeout=0)
ser.reset_input_buffer()


def send(raw, note):
    with lock:
        ser.write(raw)
    log_rows.append((f'{time.time() - t0:.3f}', raw.hex(' '), note,
                     round(st['pos'], 1), round(st['target'], 1), st['force'], st['speed']))
    ui_q.put(('cmd', f'{time.time() - t0:7.2f}s {raw.hex(" ").upper():18} {note}'))


# ------------------------------------------------ связь «я датчик ЭМГ»
def link_worker():
    buf = bytearray()
    nxt = time.time()
    while st['run']:
        with lock:
            buf += ser.read(64)
            i = buf.find(b'\x81\x80\x01\x00')
            if i >= 0:
                ser.write(pack(0x80, [6]))
                del buf[:i + 5]
                if not st['linked']:
                    st['linked'] = True
                    ui_q.put(('link', True))
            elif len(buf) > 64:
                del buf[:-8]
            if time.time() >= nxt:
                ser.write(pack(0x20, [0, 0]))
                nxt += 0.028
                if nxt < time.time() - 0.5:
                    nxt = time.time()
        time.sleep(0.002)


# ------------------------------------------------ регулятор положения
def stop_motor(note='стоп'):
    if st['moving'] != 0:
        send(CMD_STOP, note)
        st['moving'] = 0


def start_motor(direction):
    """direction: +1 закрыть (с текущей силой), -1 открыть."""
    if direction > 0:
        send(cmd_close(st['force']), f'закрыть, сила {st["force"]}%')
    else:
        send(CMD_OPEN, 'открыть')
    st['moving'] = direction


def control_worker():
    while st['run']:
        if not st['linked']:
            time.sleep(0.05)
            continue

        if st['stop_all']:
            st['stop_all'] = False
            st['moving'] = 1  # чтобы stop_motor гарантированно отправил стоп
            stop_motor('СТОП (кнопка)')
            st['target'] = st['pos']
            ui_q.put(('target', st['pos']))

        if st['calib']:
            st['calib'] = False
            stop_motor()
            start_motor(-1)
            time.sleep(FULL_STROKE_S * 1.6)
            stop_motor('стоп после калибровки')
            st['pos'] = 0.0
            st['target'] = 0.0
            st['pushed'] = 0
            ui_q.put(('target', 0.0))
            continue

        # Крайние положения: положение не измеряется, поэтому в 0 и 100
        # дожимаем до упора — мотор останавливает сама прошивка по отсечке тока.
        end = 100 if st['target'] >= 99 else (0 if st['target'] <= 1 else None)
        if end is None:
            st['pushed'] = None
        if st['push_req'] or (end is not None and st['pushed'] != end
                              and abs(st['pos'] - end) < DEADBAND):
            direction = 1 if (st['push_req'] or end == 100) else -1
            st['push_req'] = False
            stop_motor()
            start_motor(direction)
            time.sleep(PUSH_S)
            stop_motor('стоп после дожима до упора')
            st['pos'] = 100.0 if direction > 0 else 0.0
            st['pushed'] = 100 if direction > 0 else 0
            # цель = достигнутый упор, иначе регулятор сразу «откатит» кисть назад
            if st['target'] != st['pos']:
                st['target'] = st['pos']
                ui_q.put(('target', st['pos']))
            continue

        err = st['target'] - st['pos']
        if abs(err) < DEADBAND:
            stop_motor()
            time.sleep(0.01)
            continue

        direction = 1 if err > 0 else -1
        duty = max(0.05, st['speed'] / 100.0)
        started = st['moving'] != direction
        loss = START_LOSS if started else 0.0
        on_time = min(WINDOW * duty, abs(err) / RATE + loss)

        if started:
            stop_motor()
            start_motor(direction)
        t_start = time.time()
        time.sleep(on_time)
        moved = max(0.0, time.time() - t_start - loss)
        st['pos'] = max(0.0, min(100.0, st['pos'] + direction * RATE * moved))

        # при 100% скорости и далёкой цели — не останавливаемся между окнами
        continuous = duty >= 0.999 and abs(st['target'] - st['pos']) > DEADBAND
        if not continuous:
            stop_motor()
            time.sleep(max(0.0, WINDOW - on_time))


# ------------------------------------------------ GUI
root = tk.Tk()
root.title(f'Управление кистью по UART ({PORT})')
root.geometry('980x640+60+40')
F = ('Segoe UI', 13)
FB = ('Segoe UI', 15, 'bold')

top = tk.Frame(root)
top.pack(fill='x', padx=10, pady=6)


def make_slider(parent, title, color, frm, to, init, key, res=1):
    fr = tk.Frame(parent)
    fr.pack(side='left', expand=True, fill='both', padx=8)
    tk.Label(fr, text=title, font=FB, fg=color).pack()
    sc = tk.Scale(fr, from_=to, to=frm, orient='vertical', length=330, width=40, font=F,
                  troughcolor=color, resolution=res,
                  command=lambda v: st.__setitem__(key, float(v) if key == 'target' else int(float(v))))
    sc.set(init)
    sc.pack()
    return sc


s_force = make_slider(top, 'Сила, %', '#d9534f', 10, 100, st['force'], 'force')
s_target = make_slider(top, 'Сжатие (цель), %', '#5cb85c', 0, 100, 0, 'target')
s_speed = make_slider(top, 'Скорость, %', '#0275d8', 5, 100, st['speed'], 'speed')

side = tk.Frame(top)
side.pack(side='left', fill='y', padx=8)
link_lbl = tk.Label(side, text='Связь: жду опроса платы...', font=FB, fg='gray')
link_lbl.pack(anchor='w', pady=(0, 10))

tk.Label(side, text='Оценка положения:', font=F).pack(anchor='w')
pos_canvas = tk.Canvas(side, width=260, height=34, bg='white', highlightthickness=1)
pos_canvas.pack(anchor='w')
pos_lbl = tk.Label(side, text='', font=F)
pos_lbl.pack(anchor='w', pady=(0, 10))


def calib():
    st['calib'] = True


def stop_all():
    st['stop_all'] = True


def set_target(v):
    s_target.set(v)


tk.Button(side, text='Открыть полностью\n(калибровка)', font=F, width=20, command=calib).pack(pady=3)
tk.Button(side, text='Цель 0 (открыть)', font=F, width=20, command=lambda: set_target(0)).pack(pady=3)
tk.Button(side, text='Цель 100 (сжать)', font=F, width=20, command=lambda: set_target(100)).pack(pady=3)
tk.Button(side, text='Дожать (сжать до упора\nс текущей силой)', font=F, width=20,
          command=lambda: st.__setitem__('push_req', True)).pack(pady=3)
tk.Button(side, text='СТОП', font=FB, bg='tomato', width=18, command=stop_all).pack(pady=10)

tk.Label(root, text='Отправленные команды:', font=F).pack(anchor='w', padx=10)
cmd_txt = tk.Text(root, height=8, font=('Consolas', 11))
cmd_txt.pack(fill='both', expand=True, padx=10, pady=(0, 10))


def ui_tick():
    while not ui_q.empty():
        kind, payload = ui_q.get()
        if kind == 'link':
            link_lbl.config(text='Связь: есть', fg='green')
        elif kind == 'target':
            s_target.set(payload)
        else:
            cmd_txt.insert('end', payload + '\n')
            if int(cmd_txt.index('end-1c').split('.')[0]) > 400:
                cmd_txt.delete('1.0', '100.0')
            cmd_txt.see('end')
    p = st['pos']
    pos_canvas.delete('all')
    pos_canvas.create_rectangle(0, 0, 260 * p / 100, 34, fill='#5cb85c', width=0)
    pos_canvas.create_line(260 * st['target'] / 100, 0, 260 * st['target'] / 100, 34, fill='black', width=3)
    mv = {1: 'сжимаю', -1: 'разжимаю', 0: 'стоит'}[st['moving']]
    pos_lbl.config(text=f'{p:5.1f}% (цель {st["target"]:.0f}%) — {mv}')
    root.after(40, ui_tick)


def on_close():
    st['run'] = False
    time.sleep(0.15)
    try:
        with lock:
            ser.write(CMD_STOP)
    except Exception:
        pass
    with open(OUT, 'w', newline='', encoding='utf-8-sig') as f:
        w = csv.writer(f)
        w.writerow(['t', 'raw', 'note', 'pos_est', 'target', 'force', 'speed'])
        w.writerows(log_rows)
    ser.close()
    root.destroy()


root.protocol('WM_DELETE_WINDOW', on_close)
threading.Thread(target=link_worker, daemon=True).start()
threading.Thread(target=control_worker, daemon=True).start()
ui_tick()
root.mainloop()
print(f'лог: {OUT}')
