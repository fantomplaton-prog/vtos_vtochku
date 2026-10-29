"""
Подбор частоты программного ШИМ для медленного сжатия (pulse_close в common/hand_link.py).

Кисть сжимается импульсами: в окне --windows мс мотор «закрыть с силой --force %» на долю --duty,
остальное время стоп. Чем короче окно, тем плавнее; но каждый старт мотора теряет ~15 мс на разгон,
и слишком короткие импульсы он почти не отрабатывает. Скорость оценивается по времени от начала
сжатия до касания весов под большим пальцем (вес выше нуля на --contact 3 кадра подряд —
одиночные сбои HX711 не срабатывают). Плавность — на глаз/слух во время теста.

Один замер: раскрыть --open-s с, ноль весов, сжатие импульсами до касания (не дольше --timeout с), стоп.
Варианты идут в случайном порядке; «непрерывно» — сжатие без ШИМ, эталон скорости.

Файл: tools/data/pwm_<время>.csv — по замеру: window_ms, duty, t_contact (пусто — не дошла).

Запуск (из папки tools/experiments):
  py -3 pwm_test.py --sensor-port COM21
  py -3 pwm_test.py --sensor-port COM21 --windows 40,30,20 --duty 40 --repeats 3
  py -3 pwm_test.py --no-hand --sensor-mock          # проверка без железа (касания не будет)
"""
import argparse
import csv
import os
import random
import sys
import threading
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', 'data')
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from hand_link import CMD_OPEN, CMD_STOP, HandUart, NoHand, cmd_close, pulse_close
from sensor_link import BleLink, SerialLink
from sensor_text import MockTextLink, TextSensors


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--no-hand', action='store_true', help='без протеза (проверка)')
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument('--sensor-port', help='COM-порт платы датчиков (с весами)')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10')
    src.add_argument('--sensor-mock', action='store_true', help='имитация датчиков')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    p.add_argument('--windows', default='120,60,40,30,20,10', help='окна ШИМ, мс')
    p.add_argument('--duty', type=int, default=50, help='доля включения, %% (50)')
    p.add_argument('--force', type=int, default=60, help='сырая сила в команде закрытия, %% (60)')
    p.add_argument('--repeats', type=int, default=2, help='повторов каждого варианта (2)')
    p.add_argument('--open-s', type=float, default=1.5, help='раскрытие перед замером, с (1.5)')
    p.add_argument('--timeout', type=float, default=4.0, help='не дошла до весов за, с (4)')
    p.add_argument('--contact', type=float, default=300.0, help='порог касания над нулём, ед. весов (300)')
    p.add_argument('--out', help='имя CSV (по умолчанию tools/data/pwm_<дата_время>.csv)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    windows = [int(x) for x in a.windows.split(',') if x.strip()]
    if not 5 <= a.duty <= 95:
        p.error('--duty 5..95')
    plan = ([None] + windows) * a.repeats          # None — непрерывно, эталон
    random.shuffle(plan)

    lock = threading.Lock()
    st = {'w': []}

    def on_frame(t_abs, f):
        with lock:
            st['w'].append(f['weight'])

    if a.sensor_port:
        factory = lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb)
    elif a.sensor_ble:
        factory = lambda cb: BleLink(a.sensor_ble, cb)
    else:
        factory = MockTextLink
    print('Датчики: подключаюсь...')
    sensors = TextSensors(factory, on_frame)
    hand = NoHand() if a.no_hand else HandUart(a.hand_port)

    def shutdown():
        hand.send(CMD_STOP)
        time.sleep(0.2)
        sensors.close()
        hand.close()

    if not hand.linked.wait(timeout=8):
        print('ОШИБКА: протез не ответил за 8 с (нет опроса версии).')
        shutdown()
        return
    if not sensors.wait_frames(3.0):
        print('ОШИБКА: нет строк данных от платы датчиков. Протез не двигаю.')
        shutdown()
        return

    out = a.out or os.path.join(DATA, f'pwm_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    f_csv = open(out, 'w', newline='', encoding='utf-8-sig')
    w = csv.writer(f_csv)
    w.writerow(['trial', 'window_ms', 'duty', 'force', 'zero', 't_contact'])
    print(f'ШИМ: окна {windows} мс, доля {a.duty}%, сила {a.force}%, по {a.repeats} раза + непрерывно. '
          f'Файл: {out}')
    print(' №   окно, мс   до касания, с')

    results = []
    try:
        for i, win in enumerate(plan, 1):
            hand.send(CMD_STOP)
            time.sleep(0.05)
            hand.send(CMD_OPEN)
            time.sleep(a.open_s)
            hand.send(CMD_STOP)
            with lock:
                zero = float(np.median(st['w'][-15:]))
                st['w'] = []

            def touched():
                with lock:
                    tail = st['w'][-3:]
                return len(tail) == 3 and min(tail) > zero + a.contact

            if win is None:
                hand.send(cmd_close(a.force))
                t0 = time.perf_counter()
                while time.perf_counter() - t0 < a.timeout and not touched():
                    time.sleep(0.002)
                el = time.perf_counter() - t0
                hand.send(CMD_STOP)
            else:
                el = pulse_close(hand.send, a.force, a.duty / 100, win / 1000, a.timeout, touched)
            hit = touched()
            results.append((win, el if hit else None))
            w.writerow([i, win or '', a.duty if win else 100, a.force, f'{zero:.2f}', f'{el:.3f}' if hit else ''])
            f_csv.flush()
            name = 'непрерывно' if win is None else f'{win:4d}'
            print(f'{i:3d}  {name:>10}   ' + (f'{el:.2f}' if hit else f'не дошла за {a.timeout:.0f} с'))
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
    except KeyboardInterrupt:
        print('\nПрервано (Ctrl+C).')
        hand.send(CMD_STOP)
        time.sleep(0.05)
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
    finally:
        f_csv.close()
        shutdown()

    ref = [t for win, t in results if win is None and t]
    t_ref = np.mean(ref) if ref else None
    print('\nокно, мс   до касания, с        скорость от непрерывной')
    for win in [None] + windows:
        ts = [t for x, t in results if x == win]
        ok = [t for t in ts if t]
        name = 'непрерывно' if win is None else f'{win:4d}'
        if ok:
            spd = f'{t_ref / np.mean(ok) * 100:5.0f}%' if t_ref else '  --'
            print(f'{name:>10}   {np.mean(ok):5.2f} ± {np.std(ok):.2f} (n={len(ok)})   {spd}'
                  + (f'   не дошла: {len(ts) - len(ok)}' if len(ok) < len(ts) else ''))
        elif ts:
            print(f'{name:>10}   не дошла ни разу')
    print(f'Готово. Файл: {out}')


if __name__ == '__main__':
    main()
