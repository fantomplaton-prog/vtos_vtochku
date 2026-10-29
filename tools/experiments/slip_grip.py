"""
Удержание предмета с идентификацией проскальзывания: кисть Indy (UART) + плата датчиков
(текстовый протокол FormatRawFrame, см. sensor_text.py) + детектор slip_detector.py.

Последовательность:
  1. Шум: --calib-s с кисть раскрыта и неподвижна — оценка σ каждой оси.
  2. Хват: «закрыть с силой --f0 %», ждать --close-s с (кисть доезжает до предмета и
     останавливается), затем --settle-s с успокоения -> детектор взводится.
  3. Удержание --hold-s с. При проскальзывании: сила += --step (до --fmax), новая команда
     «закрыть с силой», детектор выключен --blank-s с (переходный процесс мотора меняет
     показания сам), затем новый опорный кадр.
     --detect-only: силу не менять, только отмечать события (сбор данных, подбор порогов).
  4. Раскрытие.

Проверка: держать предмет, тянуть его из пальцев / подвешивать груз — кисть должна
поджимать, а в CSV — события slip с причиной (vib / unload / shear) и ячейкой.

CSV: t_s, frame, phase, force_pct, weight, slip, cause, cell, score,
     <ячейка>_vib/_unload/_shear (признаки в единицах σ), 36 сырых осей.
Пороги подбираются офлайн: analysis/slip_replay.py.

Запуск (из папки tools/experiments):
  py -3 slip_grip.py --sensor-port COM21
  py -3 slip_grip.py --sensor-port COM21 --detect-only --hold-s 60
  py -3 slip_grip.py --sensor-port COM21 --f0 15 --step 10 --fmax 80
  py -3 slip_grip.py --no-hand --sensor-mock                  # проверка без железа
Ctrl+C — стоп мотора и раскрытие, файл сохраняется.
"""
import argparse
import csv
import math
import os
import sys
import threading
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', 'data')
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from hand_link import CMD_OPEN, CMD_STOP, HandUart, NoHand, cmd_close
from sensor_link import BleLink, SerialLink
from sensor_text import MAG_COLS, TextSensors
from slip_detector import CELLS, CH, NUM_CELLS, SlipDetector, estimate_sigma, load_normal_dirs

DEFAULT_MEDIANS = os.path.join(HERE, '..', '..', 'analysis', 'ramp_20261026_170237_medians.csv')
FEAT_COLS = [f'{c}_{k}' for c in CELLS for k in ('vib', 'unload', 'shear')]
COLUMNS = ['t_s', 'frame', 'phase', 'force_pct', 'weight', 'slip', 'cause', 'cell', 'score'] + FEAT_COLS + MAG_COLS


class MockSlipLink:
    """Имитация платы: контакт через ~1.3 с, затем проскальзывания каждые 4 с на разных ячейках
    (рост сдвига + вибрация + частичная разгрузка за 0.4 с). Строки FormatRawFrame, ~58 Гц."""
    budget_bps = 8000

    def __init__(self, on_data, normal=None, noise=20.0):
        rng = np.random.default_rng(1)
        self.rng = rng
        self.on_data = on_data
        self.noise = noise
        self.base = rng.uniform(-3000, 3000, (NUM_CELLS, CH))
        n = normal if normal is not None else rng.normal(size=(NUM_CELLS, CH))
        self.n = n / np.linalg.norm(n, axis=1, keepdims=True)
        s = rng.normal(size=(NUM_CELLS, CH))
        s -= (s * self.n).sum(axis=1, keepdims=True) * self.n
        self.s = s / np.linalg.norm(s, axis=1, keepdims=True)
        self.alive = True
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        t0 = time.time()
        k = 0
        while self.alive:
            t = time.time() - t0
            load = 600 * min(1.0, max(0.0, (t - 1.3) / 0.6))      # контакт
            x = self.base + load * self.n
            for i, ts in enumerate(np.arange(5.0, 60.0, 4.0)):    # проскальзывания
                c = i % NUM_CELLS
                p = min(1.0, max(0.0, (t - ts) / 0.4))
                x[c] += 350 * p * self.s[c] - 150 * p * self.n[c]
                if 0 < p < 1:
                    x[c] += 80 * math.sin(2 * math.pi * 17 * t) * self.rng.normal(size=CH)
            x = x + self.rng.normal(0, self.noise, x.shape)
            line = '0 0 0 0 0 0 0 0.0000 0.0000 ' + ' '.join(str(int(v)) for v in x.ravel()) + '\r\n'
            self.on_data(line.encode())
            k += 1
            time.sleep(max(0.0, t0 + k / 58.0 - time.time()))

    def write(self, data):
        pass

    def close(self):
        self.alive = False


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--no-hand', action='store_true', help='без протеза (проверка)')
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument('--sensor-port', help='COM-порт платы датчиков')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10')
    src.add_argument('--sensor-mock', action='store_true', help='имитация датчиков с проскальзываниями')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    # управление
    p.add_argument('--f0', type=int, default=15, help='начальная сила хвата, %% (15)')
    p.add_argument('--step', type=int, default=10, help='прибавка силы на проскальзывание, %% (10)')
    p.add_argument('--fmax', type=int, default=90, help='максимальная сила, %% (90)')
    p.add_argument('--detect-only', action='store_true', help='не менять силу, только отмечать события')
    p.add_argument('--calib-s', type=float, default=1.5, help='оценка шума при раскрытой кисти, с (1.5)')
    p.add_argument('--close-s', type=float, default=1.2, help='время на сжатие до предмета, с (1.2)')
    p.add_argument('--settle-s', type=float, default=0.5, help='успокоение перед взводом, с (0.5)')
    p.add_argument('--blank-s', type=float, default=0.5, help='детектор выключен после смены силы, с (0.5)')
    p.add_argument('--hold-s', type=float, default=30.0, help='длительность удержания, с (30)')
    p.add_argument('--open-s', type=float, default=1.5, help='раскрытие в конце, с (1.5)')
    # детектор
    p.add_argument('--medians', default=DEFAULT_MEDIANS,
                   help="ramp-калибровка направления нажатия (*_medians.csv); '' — без неё")
    p.add_argument('--vib-thr', type=float, default=3.0, help='порог вибрации, σ (3)')
    p.add_argument('--unload-thr', type=float, default=4.0, help='порог разгрузки, σ (4)')
    p.add_argument('--shear-thr', type=float, default=6.0, help='порог сдвига, σ (6)')
    p.add_argument('--confirm', type=int, default=3, help='кадров подряд выше порога (3)')
    p.add_argument('--cells', default='0,1,2', help='ячейки для детекции: 0=THUMB,1=INDEX,2=MIDDLE')
    p.add_argument('--out', help='имя CSV (по умолчанию tools/data/slip_<дата_время>.csv)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    if not (1 <= a.f0 <= a.fmax <= 100) or a.step < 1:
        p.error('нужно 1 <= --f0 <= --fmax <= 100 и --step >= 1')

    normal = None
    if a.medians:
        normal = load_normal_dirs(a.medians)
        print(f'Направление нажатия: {os.path.basename(a.medians)}')
    else:
        print('Без ramp-калибровки: признак unload выключен, shear = весь модуль изменения')
    det = SlipDetector(normal_dirs=normal, vib_thr=a.vib_thr, unload_thr=a.unload_thr,
                       shear_thr=a.shear_thr, confirm=a.confirm,
                       cells=[int(c) for c in a.cells.split(',') if c.strip()])

    out = a.out or os.path.join(DATA, f'slip_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    f_csv = open(out, 'w', newline='', encoding='utf-8-sig')
    w = csv.writer(f_csv)
    w.writerow(COLUMNS)
    lock = threading.Lock()
    st = {'phase': 'wait', 'force': 0, 't0': None, 'rows': 0, 'calib': [], 'events': [], 'last': None}

    def on_frame(t_abs, f):
        with lock:
            if st['phase'] == 'calib':
                st['calib'].append(f['mag'])
            r = det.update(f['mag'])
            st['last'] = r
            if r['slip'] and st['phase'] == 'hold':
                st['events'].append(r)
            if st['t0'] is None:
                return
            row = ([f'{t_abs - st["t0"]:.4f}', det.n, st['phase'], st['force'], f'{f["weight"]:.4f}',
                    int(r['slip']), r['cause'], r['cell'], f'{r["score"]:.2f}']
                   + [f'{v:.2f}' for v in r['feat'].ravel()] + f['mag'])
            w.writerow(row)
            st['rows'] += 1

    def set_phase(name):
        with lock:
            st['phase'] = name

    # --- подключение
    if a.sensor_port:
        factory = lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb)
    elif a.sensor_ble:
        factory = lambda cb: BleLink(a.sensor_ble, cb)
    else:
        factory = lambda cb: MockSlipLink(cb, normal)
    print('Датчики: подключаюсь...')
    sensors = TextSensors(factory, on_frame)
    hand = NoHand() if a.no_hand else HandUart(a.hand_port)
    if not a.no_hand:
        print(f'Протез: жду опроса платы на {a.hand_port}...')

    def shutdown():
        hand.send(CMD_STOP)
        time.sleep(0.2)
        with lock:
            st['t0'] = None
        sensors.close()
        hand.close()
        with lock:
            f_csv.close()

    if not hand.linked.wait(timeout=8):
        print('ОШИБКА: протез не ответил за 8 с (нет опроса версии). Проверьте провода UART.')
        shutdown()
        return
    ok = sensors.wait_frames(3.0)
    if not ok:
        print(f'ОШИБКА: нет строк данных от платы датчиков (принято {sensors.parser.good_lines}, '
              f'отброшено {sensors.parser.bad_lines}). Протез не двигаю.')
        shutdown()
        return

    n_slips, n_react = 0, 0
    force = a.f0
    try:
        st['t0'] = time.time()
        # 1. шум
        set_phase('calib')
        time.sleep(a.calib_s)
        with lock:
            frames = st['calib']
        if len(frames) < 10:
            raise RuntimeError(f'за {a.calib_s} с пришло {len(frames)} кадров — мало для оценки шума')
        sigma = estimate_sigma(frames)
        fs = len(frames) / a.calib_s
        with lock:
            det.set_fs(fs)
            det.set_sigma(sigma)
        print(f'Датчики: {fs:.1f} кадр/с')
        print('Шум σ, LSB (среднее по осям): '
              + ', '.join(f'{c} {s:.0f}' for c, s in zip(CELLS, sigma.mean(axis=1))))

        # 2. хват
        st['force'] = force
        hand.send(cmd_close(force))
        set_phase('close')
        print(f'Хват: сила {force}%')
        time.sleep(a.close_s)
        set_phase('settle')
        time.sleep(a.settle_s)
        with lock:
            det.arm()
            st['events'].clear()
            st['phase'] = 'hold'
        print(f'Удержание {a.hold_s:.0f} с' + (' (только детекция)' if a.detect_only else '')
              + f'. Файл: {out}')

        # 3. удержание
        t_end = time.time() + a.hold_s
        t_blank = None
        last_print = 0.0
        while time.time() < t_end:
            now = time.time()
            with lock:
                events, st['events'] = st['events'], []
                last = st['last']
            if t_blank is not None and now >= t_blank:
                t_blank = None
                with lock:
                    det.arm()
                    st['phase'] = 'hold'
            for ev in events[:1]:
                n_slips += 1
                what = f'{CELLS[ev["cell"]]} {ev["cause"]} (score {ev["score"]:.1f})'
                if a.detect_only or force >= a.fmax:
                    note = 'сила не меняется' if a.detect_only else f'сила уже {force}% — максимум!'
                    print(f'\n[{now - st["t0"]:6.2f} с] ПРОСКАЛЬЗЫВАНИЕ {what}: {note}')
                    with lock:
                        det.arm()
                else:
                    force = min(a.fmax, force + a.step)
                    n_react += 1
                    with lock:
                        st['force'] = force
                        st['phase'] = 'blank'
                        det.disarm()
                    hand.send(cmd_close(force))
                    t_blank = now + a.blank_s
                    print(f'\n[{now - st["t0"]:6.2f} с] ПРОСКАЛЬЗЫВАНИЕ {what} -> сила {force}%')
            if last and now - last_print > 0.2:
                last_print = now
                f = last['feat']
                print(f'\r{st["phase"]:6} сила {force:3d}%  score {last["score"]:4.1f}  '
                      + '  '.join(f'{c[0]}: v{f[i, 0]:4.1f} u{f[i, 1]:4.1f} s{f[i, 2]:4.1f}'
                                  for i, c in enumerate(CELLS))
                      + f'  событий {n_slips}', end='', flush=True)
            time.sleep(0.01)
        print()
    except KeyboardInterrupt:
        print('\nПрервано (Ctrl+C).')
    except RuntimeError as e:
        print(f'\nОШИБКА: {e}')
    finally:
        # 4. раскрытие
        set_phase('open')
        with lock:
            det.disarm()
            st['force'] = 0
        hand.send(CMD_STOP)
        time.sleep(0.05)
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
        rows = st['rows']
        shutdown()
        print(f'Готово. Проскальзываний: {n_slips}, поджатий: {n_react}, итоговая сила {force}%. '
              f'Строк: {rows}. Файл: {out}')


if __name__ == '__main__':
    main()
