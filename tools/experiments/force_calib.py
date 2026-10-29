"""
Калибровка силы сжатия протеза по весам (тензодатчик HX711 на плате датчиков, поле weight).
Весы стоят под большим пальцем; кисть сжимает с силой N % (команда 01 11 01 <N>), записывается
показание весов. Результат — зависимость «сырой % команды -> реальная сила» (analysis/force_calib.py),
по ней force_model.py делает силу пропорциональной: «N %» = N % от максимальной силы.

Два режима:
  обычный     — уровни = сырой % команды (снять калибровку);
  --linear    — уровни = линейный % (доля максимальной силы), в команду уходит сырой % по рабочей
                калибровке (tools/common/force_calib.json или --calib) — проверка, что сила стала
                пропорциональной.

Один замер:
  1. Раскрыть --open-s с; ноль весов = медиана последних --tail-s с (снимается перед КАЖДЫМ
     замером — дрейф и сдвиг предмета не накапливаются).
  2. Подход с пониженной скоростью (--speed %): программный ШИМ — в окне --pwm-ms «закрыть с силой N %»
     на долю speed, остальное время стоп; до касания весов (вес выше нуля на --contact 3 кадра подряд)
     или пока кисть при такой скорости проходит полный ход с запасом.
  3. Откат: сразу после касания «открыть» на --retract-ms мс. На подходе палец врезается в весы, и
     самотормозящийся редуктор держит силу удара — она больше, чем даёт мотор на малых %, поэтому
     без отката малые % неразличимы. --retract-ms 0 — без отката и дожима (прежняя процедура).
  4. Дожим: ШИМ с периодом --press-pwm-ms (20) и долей --press-duty — кисть медленно, без удара,
     снова упирается; сила набирается моментом мотора при N %. Длится --press-s с.
     Затем «закрыть с силой N %» непрерывно и держать --hold-s с;
     сила = медиана последних --tail-s с минус ноль. Разброс и наклон в этом окне
     показывают, успело ли показание установиться.
  5. Стоп, раскрыть.
Каждый замер — из полностью раскрытого положения: на этой кисти снижение силы её не
разжимает, поэтому замеры подряд без раскрытия давали бы гистерезис.
Уровни идут в случайном порядке (--repeats повторов каждого), чтобы дрейф и нагрев мотора
не путались с зависимостью от силы.

Файлы (tools/data):
  fcal_<время>.csv          — все кадры: t_s, trial, level, phase, weight, adc0..5, hall, 36 осей
  fcal_<время>_summary.csv  — по замеру: trial, level, w_zero, w_hold, force, tail_std, tail_slope

Запуск (из папки tools/experiments):
  py -3 force_calib.py --sensor-port COM21
  py -3 force_calib.py --sensor-port COM21 --levels 10,20,30,40,50,60,70,80,90,100 --repeats 3
  py -3 force_calib.py --sensor-port COM21 --linear         # проверка линеаризации
  py -3 force_calib.py --no-hand --sensor-mock              # проверка без железа
Ctrl+C — стоп мотора и раскрытие, записанное сохраняется.
"""
import argparse
import csv
import math
import os
import random
import sys
import threading
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', 'data')
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from force_model import DEFAULT_PATH, ForceModel
from hand_link import CMD_OPEN, CMD_STOP, FULL_STROKE_S, WINDOW, HandUart, NoHand, cmd_close, pulse_close
from sensor_link import BleLink, SerialLink
from sensor_text import MAG_COLS, NUM_CELLS, NUM_MLX, AXES, TextSensors

TRACE_COLS = (['t_s', 'trial', 'level', 'raw_pct', 'phase', 'weight'] + [f'adc{i}' for i in range(6)] + ['hall']
              + MAG_COLS)
SUMMARY_COLS = ['trial', 'mode', 'level', 'raw_pct', 'w_zero', 'w_hold', 'force', 'tail_std', 'tail_slope_per_s', 'n_tail',
                't_contact']


class MockRig:
    """Имитация кисти + платы для проверки без железа: weight откликается на команды
    (мёртвая зона до 4 %, дальше насыщение, инерция 0.25 с, шум, медленный дрейф нуля)."""

    def __init__(self):
        self.target = 0.0
        self.linked = threading.Event()
        self.linked.set()
        self.on_data = None
        self.alive = True

    # как HandUart
    def send(self, raw):
        if raw[1] == 0x11:
            self.target = 900 * max(0.0, 1 - math.exp(-(raw[3] - 4) / 30)) / (1 - math.exp(-96 / 30))
        elif raw[1] == 0x10:
            self.target = 0.0

    def close(self):
        self.alive = False

    # как транспорт датчиков
    def link_factory(self, on_data):
        self.on_data = on_data
        threading.Thread(target=self._run, daemon=True).start()
        return self

    def write(self, data):
        pass

    def _run(self):
        rng = np.random.default_rng(2)
        w, t0, k = 0.0, time.time(), 0
        while self.alive:
            t = time.time() - t0
            w += (self.target - w) * (1 - math.exp(-1 / 58 / 0.25))
            weight = w + 3 * t / 60 + rng.normal(0, 1.5)
            mag = rng.normal(0, 20, NUM_CELLS * NUM_MLX * AXES).astype(int) + int(w)
            line = ('0 0 0 0 0 0 0 0.0000 ' + f'{weight:.4f} ' + ' '.join(map(str, mag)) + '\r\n')
            self.on_data(line.encode())
            k += 1
            time.sleep(max(0.0, t0 + k / 58 - time.time()))


def robust_tail(t, w):
    """Разброс и наклон окна, устойчивые к одиночным сбоям HX711 (скачки на ±десятки тысяч):
    σ = 1.4826·MAD, наклон = разность медиан второй и первой половины / половина длительности."""
    t, w = np.asarray(t, float), np.asarray(w, float)
    sd = 1.4826 * float(np.median(np.abs(w - np.median(w))))
    h = len(w) // 2
    dt = (t[-1] - t[0]) / 2
    slope = float((np.median(w[h:]) - np.median(w[:h])) / dt) if h >= 2 and dt > 0 else 0.0
    return sd, slope


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--no-hand', action='store_true', help='без протеза (только с --sensor-mock)')
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument('--sensor-port', help='COM-порт платы датчиков (с весами)')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10')
    src.add_argument('--sensor-mock', action='store_true', help='имитация кисти и весов')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    p.add_argument('--levels', default='1,5,10,20,30,40,50,60,70,80,90,100',
                   help='уровни силы, %% (сырые; с --linear — линейные)')
    p.add_argument('--repeats', type=int, default=3, help='повторов каждого уровня (3)')
    p.add_argument('--open-s', type=float, default=1.5, help='раскрытие перед замером, с (1.5)')
    p.add_argument('--speed', type=int, default=50, help='скорость подхода, %% (50; 100 — без замедления)')
    p.add_argument('--pwm-ms', type=float, default=WINDOW * 1000, help=f'окно ШИМ подхода, мс ({WINDOW * 1000:.0f})')
    p.add_argument('--contact', type=float, default=300.0, help='порог касания над нулём, ед. весов (300)')
    p.add_argument('--retract-ms', type=float, default=20.0, help='откат после касания, мс (20; 0 — без отката)')
    p.add_argument('--press-pwm-ms', type=float, default=20.0, help='период ШИМ дожима, мс (20)')
    p.add_argument('--press-duty', type=int, default=50, help='доля включения при дожиме, %% (50)')
    p.add_argument('--press-s', type=float, default=1.0, help='длительность дожима, с (1.0)')
    p.add_argument('--hold-s', type=float, default=2.5, help='удержание с силой, с (2.5)')
    p.add_argument('--tail-s', type=float, default=0.8, help='окно усреднения в конце фаз, с (0.8)')
    p.add_argument('--linear', action='store_true', help='уровни — линейные %%, пересчёт по калибровке')
    p.add_argument('--calib', default=DEFAULT_PATH, help='калибровка для --linear (рабочая по умолчанию)')
    p.add_argument('--seed', type=int, help='seed порядка уровней (по умолчанию случайный)')
    p.add_argument('--out', help='имя CSV (по умолчанию tools/data/fcal_<дата_время>.csv)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    levels = [int(x) for x in a.levels.split(',') if x.strip()]
    if not levels or not all(1 <= x <= 100 for x in levels) or a.repeats < 1:
        p.error('уровни 1..100, --repeats >= 1')
    if a.tail_s >= min(a.open_s, a.hold_s):
        p.error('--tail-s должен быть меньше --open-s и --hold-s')
    model = None
    if a.linear:
        if not os.path.exists(a.calib):
            p.error(f'--linear: нет калибровки {a.calib} — сначала analysis/force_calib.py --install')
        model = ForceModel.load(a.calib)
        print(f'Линейный режим, калибровка {model.source or a.calib}: '
              + ', '.join(f'{L}%->{model.raw_pct(L)}' for L in levels))
        if any(L < model.min_linear for L in levels):
            print(f'  ВНИМАНИЕ: ниже {model.min_linear:.0f}% линейных кисть меньшей силы не даёт')
    to_raw = (lambda L: model.raw_pct(L)) if model else (lambda L: L)
    plan = levels * a.repeats
    random.Random(a.seed).shuffle(plan)

    out = a.out or os.path.join(DATA, f'fcal_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    out_sum = out[:-4] + '_summary.csv'
    f_tr = open(out, 'w', newline='', encoding='utf-8-sig')
    w_tr = csv.writer(f_tr)
    w_tr.writerow(TRACE_COLS)
    lock = threading.Lock()
    st = {'t0': None, 'trial': 0, 'level': 0, 'raw': 0, 'live': [], 'zero': 0.0, 'phase': 'wait', 'buf': [], 'rows': 0}

    def on_frame(t_abs, f):
        with lock:
            st['buf'].append((t_abs, f['weight']))
            st['live'].append(f['weight'])
            del st['live'][:-3]
            if st['t0'] is None:
                return
            w_tr.writerow([f'{t_abs - st["t0"]:.4f}', st['trial'], st['level'], st['raw'], st['phase'], f'{f["weight"]:.4f}']
                          + f['adc'] + [f['hall']] + f['mag'])
            st['rows'] += 1

    def collect(seconds):
        """Подождать seconds и вернуть (t, weight) кадров последних tail_s секунд."""
        t_start = time.time()
        with lock:
            st['buf'] = []
        time.sleep(seconds)
        with lock:
            buf = st['buf']
        t_end = time.time()
        tail = [(t, w) for t, w in buf if t >= t_end - a.tail_s and t >= t_start]
        return np.array([t for t, _ in tail]), np.array([w for _, w in tail])

    # --- подключение
    rig = MockRig() if a.sensor_mock else None
    if a.sensor_port:
        factory = lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb)
    elif a.sensor_ble:
        factory = lambda cb: BleLink(a.sensor_ble, cb)
    else:
        factory = rig.link_factory
    print('Датчики: подключаюсь...')
    sensors = TextSensors(factory, on_frame)
    if rig:
        hand = rig
    else:
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
            f_tr.close()

    if not hand.linked.wait(timeout=8):
        print('ОШИБКА: протез не ответил за 8 с (нет опроса версии). Проверьте провода UART.')
        shutdown()
        return
    if not sensors.wait_frames(3.0):
        print(f'ОШИБКА: нет строк данных от платы датчиков (принято {sensors.parser.good_lines}, '
              f'отброшено {sensors.parser.bad_lines}). Протез не двигаю.')
        shutdown()
        return

    if not 5 <= a.speed <= 100:
        p.error('--speed 5..100')
    duty = a.speed / 100.0
    retract = a.retract_ms > 0
    approach_s = (1.6 if retract else 1.3) * FULL_STROKE_S / duty if a.speed < 100 else (2.0 if retract else 0.0)

    def touched():
        with lock:
            tail = st['live'][-3:]
            return len(tail) == 3 and min(tail) > st['zero'] + a.contact

    def approach(raw):
        """Подход импульсами до касания (с откатом) или на всё время подхода. -> время касания или None."""
        if a.speed >= 100 and retract:
            hand.send(cmd_close(raw))
            t0 = time.perf_counter()
            while time.perf_counter() - t0 < approach_s and not touched():
                time.sleep(0.002)
            el = time.perf_counter() - t0
            hand.send(CMD_STOP)
        else:
            el = pulse_close(hand.send, raw, duty, a.pwm_ms / 1000, approach_s, touched if retract else None)
        return el if retract and touched() else None

    def retract_and_press(raw):
        with lock:
            st['phase'] = 'retract'
        hand.send(CMD_OPEN)
        t = time.perf_counter() + a.retract_ms / 1000
        while time.perf_counter() < t:
            pass
        hand.send(CMD_STOP)
        with lock:
            st['phase'] = 'press'
        pulse_close(hand.send, raw, a.press_duty / 100, a.press_pwm_ms / 1000, a.press_s)

    trial_s = a.open_s + approach_s + (a.press_s if retract else 0) + a.hold_s + 0.1
    if retract:
        print(f'Откат {a.retract_ms:.0f} мс после касания, дожим ШИМ {a.press_pwm_ms:.0f} мс × {a.press_duty}% '
              f'{a.press_s} с, порог касания {a.contact}')
    print(f'Скорость подхода {a.speed}%: ' + (f'подход {approach_s:.1f} с импульсами' if approach_s else 'без замедления'))
    print(f'Калибровка: уровни {levels}, по {a.repeats} раза, {len(plan)} замеров, ~{len(plan) * trial_s:.0f} с.')
    print(f'Файлы: {out}\n       {out_sum}')
    print(' №   сила,%  сырой%  ноль       удержание   сила (вес)   разброс  наклон/с')

    results = []
    f_sum = open(out_sum, 'w', newline='', encoding='utf-8-sig')
    w_sum = csv.writer(f_sum)
    w_sum.writerow(SUMMARY_COLS)
    try:
        st['t0'] = time.time()
        for i, level in enumerate(plan, 1):
            with lock:
                raw = to_raw(level)
                st['trial'], st['level'], st['raw'] = i, level, raw
                st['phase'] = 'open'
            hand.send(CMD_STOP)
            time.sleep(0.05)
            hand.send(CMD_OPEN)
            _, w0 = collect(a.open_s)
            hand.send(CMD_STOP)
            if len(w0):
                with lock:
                    st['zero'] = float(np.median(w0))
                    st['live'] = []

            with lock:
                st['phase'] = 'approach'
            t_contact = approach(raw)
            if retract:
                retract_and_press(raw)
            with lock:
                st['phase'] = 'hold'
            hand.send(cmd_close(raw))
            th, wh = collect(a.hold_s)
            hand.send(CMD_STOP)
            if len(w0) < 3 or len(wh) < 3:
                print(f'{i:3d}  {level:5d}  {raw:5d}   мало кадров в окне ({len(w0)}, {len(wh)}) — пропуск')
                continue
            zero, hold = float(np.median(w0)), float(np.median(wh))
            sd, slope = robust_tail(th, wh)
            row = [i, 'linear' if model else 'raw', level, raw, zero, hold, hold - zero, sd, slope, len(wh),
                   round(t_contact, 3) if t_contact is not None else '']
            results.append(row)
            w_sum.writerow([f'{v:.4f}' if isinstance(v, float) else v for v in row])
            f_sum.flush()
            print(f'{i:3d}  {level:5d}  {raw:5d}   {zero:9.2f}  {hold:10.2f}  {hold - zero:11.2f}  {sd:8.2f}  {slope:8.2f}'
                  + ('' if not retract else f'  касание {t_contact:.2f} с' if t_contact is not None else '  касания нет'))
        with lock:
            st['phase'] = 'open'
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
    except KeyboardInterrupt:
        print('\nПрервано (Ctrl+C).')
        hand.send(CMD_STOP)
        time.sleep(0.05)
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
    finally:
        f_sum.close()
        rows = st['rows']
        shutdown()

    if results:
        r = np.array([[x[2], x[6]] for x in results])
        print('\nСредняя сила по уровням (в единицах весов):')
        for L in sorted(set(r[:, 0])):
            v = r[r[:, 0] == L, 1]
            print(f'  {int(L):3d} %  {v.mean():10.2f}  ± {v.std():.2f}  (n={len(v)})')
    print(f'Готово. Замеров: {len(results)}, строк: {rows}. Анализ: '
          f'cd analysis && uv run python force_calib.py --file {os.path.relpath(out_sum, os.path.join(HERE, "..", "..", "analysis"))}')


if __name__ == '__main__':
    main()
