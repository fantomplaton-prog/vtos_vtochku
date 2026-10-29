"""
Тест: сырые оси всех магнитометров при росте силы сжатия протеза.
Без графиков — только запись CSV.

Последовательность (повторяется --cycles раз):
  1. Контакт: закрыть с минимальной силой (--min, по умолчанию 10 %) непрерывно
     (скорость 100, сжатие до 100), держать --contact-hold с (по умолчанию 2 с).
  2. Рост силы: от --min + --step до --max с шагом --step, каждое значение --interval с
     (по умолчанию шаг 1 %, 1 с). Мотор между шагами не выключается — каждую ступень
     уходит новая команда «закрыть с силой N %».
  3. Раскрытие: стоп, открыть --open-s с, стоп.

Плата датчиков — текстовый протокол (FormatRawFrame, см. sensor_text.py): строка на кадр,
45 чисел: adc0..5 hall hall_filt weight + 36 осей магнитометров. Плата шлёт строки сама,
команд не принимает.

Файл CSV: t_s, force_pct, weight, THUMB_MLX0_X ... MIDDLE_MLX3_Z — как пришло, без пересчёта.
Одна строка на кадр датчиков. force_pct = 0, пока кисть раскрывается / вне теста.

Запуск (из папки tools/experiments):
  py -3 force_ramp.py --sensor-port COM21
  py -3 force_ramp.py --sensor-port COM21 --step 5 --cycles 3
  py -3 force_ramp.py --sensor-ble HM-10-1 --hand-port COM20
  py -3 force_ramp.py --no-hand --sensor-mock --step 10      # проверка без железа
Ctrl+C — немедленный стоп мотора, файл сохраняется.
"""
import argparse
import csv
import os
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', 'data')
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from sensor_link import BleLink, SerialLink
from sensor_text import MAG_COLS, MockTextLink, TextSensors
from hand_link import CMD_OPEN, CMD_STOP, HandUart, NoHand, cmd_close

def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--no-hand', action='store_true', help='без протеза (проверка)')
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument('--sensor-port', help='COM-порт платы датчиков')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10')
    src.add_argument('--sensor-mock', action='store_true', help='имитация датчиков')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    p.add_argument('--min', type=int, default=10, help='минимальная сила, %% (по умолчанию 10)')
    p.add_argument('--max', type=int, default=100, help='максимальная сила, %% (по умолчанию 100)')
    p.add_argument('--step', type=int, default=1, help='инкремент силы, %% (по умолчанию 1)')
    p.add_argument('--interval', type=float, default=1.0, help='время на каждой ступени, с (по умолчанию 1)')
    p.add_argument('--contact-hold', type=float, default=2.0, help='удержание контакта на мин. силе, с (2)')
    p.add_argument('--cycles', type=int, default=1, help='число циклов сжатие→раскрытие (по умолчанию 1)')
    p.add_argument('--open-s', type=float, default=1.5, help='время раскрытия между циклами, с (1.5)')
    p.add_argument('--out', help='имя CSV (по умолчанию tools/data/ramp_<дата_время>.csv)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    if not (1 <= a.min <= a.max <= 100) or a.step < 1:
        p.error('нужно 1 <= --min <= --max <= 100 и --step >= 1')
    out = a.out or os.path.join(DATA, f'ramp_{time.strftime("%Y%m%d_%H%M%S")}.csv')

    # --- файл: t_s, force_pct, weight, 36 осей магнитометров (как пришли, без пересчёта)
    f_csv = open(out, 'w', newline='', encoding='utf-8-sig')
    w = csv.writer(f_csv)
    w.writerow(['t_s', 'force_pct', 'weight'] + MAG_COLS)
    lock = threading.Lock()
    state = {'force': 0, 't0': None, 'rows': 0}

    def on_frame(t_abs, f):
        if state['t0'] is None:
            return                       # пишем только с начала теста
        row = [f'{t_abs - state["t0"]:.4f}', state['force'], f'{f["weight"]:.4f}'] + f['mag']
        with lock:
            w.writerow(row)
            state['rows'] += 1

    # --- подключение
    if a.sensor_port:
        factory = lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb)
    elif a.sensor_ble:
        factory = lambda cb: BleLink(a.sensor_ble, cb)
    else:
        factory = MockTextLink
    print('Датчики: подключаюсь...')
    sensors = TextSensors(factory, on_frame)

    hand = NoHand() if a.no_hand else HandUart(a.hand_port)
    print(f'Протез: жду опроса платы на {a.hand_port}...' if not a.no_hand else 'Протез: отключён (--no-hand)')
    if not hand.linked.wait(timeout=8):
        print('ОШИБКА: протез не ответил за 8 с (нет опроса версии). Проверьте провода UART.')
        sensors.close()
        hand.close()
        f_csv.close()
        return

    # ждём текстовые кадры; если не идут сами — отправляем команду запуска
    ok = sensors.wait_frames(3.0)
    if not ok:
        print(f'ОШИБКА: нет строк данных от платы датчиков (принято корректных {sensors.parser.good_lines}, '
              f'отброшено {sensors.parser.bad_lines}). Протез не двигаю.')
        sensors.close()
        hand.close()
        f_csv.close()
        return
    t_rate = time.time()
    n_rate = sensors.n
    time.sleep(1.0)
    print(f'Датчики: {(sensors.n - n_rate) / (time.time() - t_rate):.1f} кадр/с, '
          f'отброшено строк: {sensors.parser.bad_lines}')
    print('Связь есть с обеими платами.')

    forces = list(range(a.min + a.step, a.max + 1, a.step))
    if not forces or forces[-1] != a.max:
        forces.append(a.max)
    per_cycle = a.contact_hold + len(forces) * a.interval + a.open_s
    print(f'Тест: {a.cycles} цикл(ов), сила {a.min}→{a.max}% шаг {a.step}%, '
          f'{a.interval} с на ступень, ~{per_cycle * a.cycles:.0f} с всего. Ctrl+C — стоп.')
    print(f'Файл: {out}')

    def hold(seconds):
        end = time.time() + seconds
        while time.time() < end:
            time.sleep(min(0.05, max(0.0, end - time.time())))

    try:
        state['t0'] = time.time()
        for cyc in range(1, a.cycles + 1):
            # 1. контакт на минимальной силе
            state['force'] = a.min
            hand.send(cmd_close(a.min))
            print(f'[цикл {cyc}/{a.cycles}] контакт: сила {a.min}% на {a.contact_hold} с')
            hold(a.contact_hold)

            # 2. рост силы
            for i, fpct in enumerate(forces, 1):
                state['force'] = fpct
                hand.send(cmd_close(fpct))
                print(f'\r[цикл {cyc}/{a.cycles}] сила {fpct:3d}%  ({i}/{len(forces)})  '
                      f'строк {state["rows"]}', end='', flush=True)
                hold(a.interval)
            print()

            # 3. раскрытие
            hand.send(CMD_STOP)
            state['force'] = 0
            time.sleep(0.05)
            hand.send(CMD_OPEN)
            print(f'[цикл {cyc}/{a.cycles}] раскрытие {a.open_s} с')
            hold(a.open_s)
            hand.send(CMD_STOP)
            time.sleep(0.1)
    except KeyboardInterrupt:
        print('\nПрервано (Ctrl+C) — стоп мотора.')
    finally:
        hand.send(CMD_STOP)
        state['force'] = 0
        time.sleep(0.2)
        state['t0'] = None               # дальше кадры в файл не пишем
        sensors.close()
        hand.close()
        with lock:
            f_csv.close()
        print(f'Готово. Строк: {state["rows"]}, кадров датчиков всего: {sensors.n}. Файл: {out}')


if __name__ == '__main__':
    main()
