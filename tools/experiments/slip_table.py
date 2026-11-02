"""
Проскальзывание по краю стола: кисть хватает край стола (~2.5 см) и её водят в 4 стороны,
на нескольких силах (по умолчанию минимальная, средняя, максимальная). Эталон «статика»
снимается в ТОМ ЖЕ хвате — G при одной силе от хвата к хвату разный.

На каждую силу:
  1. noise    кисть раскрыта, неподвижна, --noise-s с — ноль F и G (как baseline прошивки).
  2. approach медленный подход ШИМ (окно --pwm-ms, доля --speed %) с силой --approach % до касания
              по магнитометрам (F любого пальца выросла на --touch мкТл), не дольше --approach-s с.
  3. ramp     ступени силы от --ramp-from до рабочей (--ramp-steps × --step-s с) — эталон G(F).
  4. ref      рабочая сила, неподвижно --ref-s с — тоже эталон.
  5. drag     вождение по жёсткому сценарию — кнопки нажимать не нужно. Скрипт сам объявляет шаг
              (крупная надпись + звуковой сигнал; за --warn-s с до шага — «дальше: ...») и сам ставит
              метку. Три состояния (только вправо/влево — вперёд/назад рука уходит слишком мало):
                 rest        ПОКОЙ — отпустить, касательной силы нет
                 hold_right  ТЯНУТЬ ВПРАВО, НЕ СДВИГАЯ — касательная сила есть, держит трение покоя
                 slide_right СКОЛЬЗИТЬ ВПРАВО — кисть едет по столу (трение скольжения)
                 hold_left / slide_left — то же влево
              Сценарий (--scenario, по умолчанию), ~74 с на силу:
                 ПОКОЙ 5
                 2 раза: ТЯНУТЬ ВПРАВО 4, ПОКОЙ 5, СКОЛЬЗИТЬ ВПРАВО 4, ПОКОЙ 5,
                         ТЯНУТЬ ВЛЕВО 4,  ПОКОЙ 5, СКОЛЬЗИТЬ ВЛЕВО 4,  ПОКОЙ 5
              Между шагами всегда ПОКОЙ 5 с — шаги не наползают друг на друга. Метка в CSV — шаг (label)
              и время от его начала (cue_s): в анализе начало и конец шага (реакция человека) отбрасываются.
              В консоли: шаг, F и |ΔG| (сдвиг магнита сверх того, что даёт сила) по пальцам.
  6. open     раскрыть.
Силы — сырой % команды 01 11 01 <pct> (по калибровке на весах сила растёт от ~55 %).

--touch-grip — хват на минимальной силе: подход без ШИМ с силой 100 % и СТОП в момент касания по
магнитометрам (как tm в hand_console: |F − F0| > --mthr σ, 2 кадра), сразу откат «открыть» на --retract-ms мс (30).
Ступеней силы нет, мотор больше не включается —
держит то, что осталось после остановки; эталон G — неподвижно --ref-s с. Повторов --trials.
Смысл: искать проскальзывание там, где его ещё можно убрать дожатием.

--slide-cycles N — N циклов на хвате до касания, без ожидания Enter (кроме первого):
  закрыть до касания (как --touch-grip, откат --retract-ms) → пауза --pause-s (эталон G) →
  СКОЛЬЗИТЬ --slide-s (нечётные циклы вправо, чётные влево) → пауза --pause-s → открыть.

Файл tools/data/slipt_<время>.csv: t_s, level, raw_pct, phase, label, weight, 36 сырых осей.
Анализ: cd analysis && uv run python slip_table_analysis.py --file ../tools/data/slipt_<время>.csv

Запуск (из папки tools/experiments):
  py -3 slip_table.py --sensor-port COM21
  py -3 slip_table.py --sensor-port COM21 --forces 55,80,100
  py -3 slip_table.py --sensor-port COM21 --touch-grip --trials 3
  py -3 slip_table.py --sensor-port COM21 --slide-cycles 10
  py -3 slip_table.py --sensor-port COM21 --scenario "rest 5; 3*(hold_right 4, rest 5, slide_right 4, rest 5)"
  py -3 slip_table.py --no-hand --sensor-mock --scenario "rest 1; slide_right 1; rest 1"   # без железа
Перед каждой силой скрипт ждёт Enter — время поставить кисть к краю стола.
Ctrl+C — стоп мотора и раскрытие, записанное сохраняется.
"""
import argparse
import csv
import os
import sys
import threading
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, '..', 'data')
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from hand_link import CMD_OPEN, CMD_STOP, HandUart, NoHand, _sleep_until, cmd_close, pulse_close
from mag_contact import MagContact
from mag_features import EMA_ALPHA_MAG, CELLS, RefTable, feature_set, to_common
from sensor_link import BleLink, SerialLink
from sensor_text import MAG_COLS, MockTextLink, TextSensors

KEY_NAMES = {'rest': 'ПОКОЙ (отпустить)',
             'hold_right': 'ТЯНУТЬ ВПРАВО, НЕ СДВИГАЯ', 'slide_right': 'СКОЛЬЗИТЬ ВПРАВО',
             'hold_left': 'ТЯНУТЬ ВЛЕВО, НЕ СДВИГАЯ', 'slide_left': 'СКОЛЬЗИТЬ ВЛЕВО'}
BEEP_HZ = {'rest': 400, 'hold_right': 900, 'slide_right': 1300, 'hold_left': 700, 'slide_left': 1100}
DEFAULT_SCENARIO = ('rest 5; 2*(hold_right 4, rest 5, slide_right 4, rest 5, '
                    'hold_left 4, rest 5, slide_left 4, rest 5)')


def parse_scenario(text):
    """'rest 3; 3*(fwd 1.5, rest 2)' -> [('rest', 3.0), ('fwd', 1.5), ('rest', 2.0), ...]."""
    out = []
    for part in text.split(';'):
        part = part.strip()
        if not part:
            continue
        reps = 1
        if '*' in part:
            n, part = part.split('*', 1)
            reps = int(n)
        steps = []
        for item in part.strip().strip('()').split(','):
            name, dur = item.split()
            if name not in KEY_NAMES:
                raise ValueError(f'шаг {name!r}: допустимы {", ".join(KEY_NAMES)}')
            steps.append((name, float(dur)))
        out += steps * reps
    return out


def beep(label, long=False):
    """Сигнал шага в фоне (winsound — только Windows)."""
    try:
        import winsound
    except ImportError:
        return
    threading.Thread(target=winsound.Beep, args=(BEEP_HZ[label], 350 if long else 120), daemon=True).start()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--no-hand', action='store_true', help='без протеза (проверка)')
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument('--sensor-port', help='COM-порт платы датчиков')
    src.add_argument('--sensor-ble', help='имя или MAC HM-10')
    src.add_argument('--sensor-mock', action='store_true', help='имитация датчиков')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    p.add_argument('--touch-grip', action='store_true',
                   help='хват до касания: без ШИМ, сила 100 %%, стоп при касании; без ступеней силы')
    p.add_argument('--retract-ms', type=float, default=30.0,
                   help='с --touch-grip: откат «открыть» сразу после касания, мс (30; 0 — без отката)')
    p.add_argument('--trials', type=int, default=3, help='с --touch-grip: число повторов (3)')
    p.add_argument('--slide-cycles', type=int, default=0,
                   help='циклы «закрыть до касания → пауза → скользить → пауза → открыть» (вправо/влево '
                        'по очереди); включает --touch-grip, сценарий не нужен. 0 — выкл.')
    p.add_argument('--pause-s', type=float, default=3.0, help='циклы: пауза до и после скольжения, с (3)')
    p.add_argument('--slide-s', type=float, default=4.0, help='циклы: длительность скольжения, с (4)')
    p.add_argument('--forces', default='55,78,100', help='рабочие силы, сырой %% (55,78,100)')
    p.add_argument('--approach', type=int, default=40, help='сила на подходе, %% (40)')
    p.add_argument('--speed', type=int, default=80, help='скорость подхода, доля ШИМ %% (80)')
    p.add_argument('--pwm-ms', type=float, default=30.0, help='окно ШИМ подхода, мс (30)')
    p.add_argument('--mthr', type=float, default=6.0,
                   help='касание: |F − F0| больше стольких σ шума 2 кадра подряд (6, как tm в hand_console)')
    p.add_argument('--approach-s', type=float, default=4.0, help='предел подхода, с (4)')
    p.add_argument('--ramp-from', type=int, default=50, help='эталон: сила первой ступени, %% (50)')
    p.add_argument('--ramp-steps', type=int, default=5, help='эталон: число ступеней (5)')
    p.add_argument('--step-s', type=float, default=1.0, help='эталон: длительность ступени, с (1)')
    p.add_argument('--ref-s', type=float, default=3.0, help='неподвижно на рабочей силе, с (3)')
    p.add_argument('--scenario', default=DEFAULT_SCENARIO, help='сценарий вождения (см. описание)')
    p.add_argument('--warn-s', type=float, default=2.0, help='«дальше: ...» за столько с до шага (2)')
    p.add_argument('--noise-s', type=float, default=1.5, help='кисть раскрыта перед хватом, с (1.5)')
    p.add_argument('--open-s', type=float, default=1.5, help='раскрытие, с (1.5)')
    p.add_argument('--out', help='имя CSV (по умолчанию tools/data/slipt_<дата_время>.csv)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    forces = [int(x) for x in a.forces.split(',') if x.strip()]
    if a.slide_cycles:
        a.touch_grip = True
        a.trials = a.slide_cycles
        a.ref_s = a.pause_s                 # пауза после закрытия — она же эталон G
    if a.touch_grip:
        forces = [100] * a.trials
    if not all(1 <= f <= 100 for f in forces):
        p.error('--forces: 1..100')
    try:
        scenario = parse_scenario(a.scenario)
    except ValueError as e:
        p.error(f'--scenario: {e}')

    out = a.out or os.path.join(DATA, f'slipt_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    f_csv = open(out, 'w', newline='', encoding='utf-8-sig')
    w = csv.writer(f_csv)
    w.writerow(['t_s', 'level', 'raw_pct', 'phase', 'label', 'cue_s', 'weight'] + MAG_COLS)
    lock = threading.Lock()
    st = {'t0': None, 'level': 0, 'raw': 0, 'phase': 'wait', 'label': 'rest', 'cue_t': None, 'rows': 0, 'm': [], 'detect': False, 'hit_info': None,
          'ema': None, 'zeroF': None, 'zeroG': None, 'F': None, 'G': None,
          'buf': None, 'ref': None, 'dsh': None}

    mc = MagContact(thr=a.mthr, confirm=2)
    hit = threading.Event()

    def on_frame(t_abs, f):
        with lock:
            x = to_common(f['mag'])
            st['ema'] = x if st['ema'] is None else st['ema'] + EMA_ALPHA_MAG * (x - st['ema'])
            F, G = feature_set(st['ema'])
            st['F'], st['G'] = F, G
            st['m'].append(f['mag'])
            del st['m'][:-100]
            if st['detect'] and not hit.is_set():
                # касание — как tm в hand_console: |F − F0| > thr·σ два кадра подряд, прямо в потоке приёма
                ok, r, df = mc.update(f['mag'])
                if ok:
                    st['hit_info'] = (t_abs, r, df)
                    hit.set()
            if st['zeroF'] is not None:
                F = F - st['zeroF']
                G = {k: G[k] - st['zeroG'][k] for k in G}
                st['Frel'] = F
                if st['buf'] is not None:
                    st['buf'].append((F, G))
                if st['ref'] is not None:
                    _, st['dsh'] = st['ref'].delta(F, G)
            if st['t0'] is not None:
                w.writerow([f'{t_abs - st["t0"]:.4f}', st['level'], st['raw'], st['phase'], st['label'],
                            '' if st['cue_t'] is None else f'{t_abs - st["cue_t"]:.3f}',
                            f'{f["weight"]:.2f}'] + f['mag'])
                st['rows'] += 1

    def set_phase(name, label='rest'):
        with lock:
            st['phase'], st['label'], st['cue_t'] = name, label, None

    if a.sensor_port:
        factory = lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb)
    elif a.sensor_ble:
        factory = lambda cb: BleLink(a.sensor_ble, cb)
    else:
        factory = MockTextLink
    print('Датчики: подключаюсь...')
    sensors = TextSensors(factory, on_frame)
    hand = NoHand() if a.no_hand else HandUart(a.hand_port)

    def open_hand():
        set_phase('open')
        hand.send(CMD_STOP)
        time.sleep(0.05)
        hand.send(CMD_OPEN)
        time.sleep(a.open_s)
        hand.send(CMD_STOP)

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
        print('ОШИБКА: протез не ответил за 8 с (нет опроса версии).')
        shutdown()
        return
    if not sensors.wait_frames(3.0):
        print('ОШИБКА: нет строк данных от платы датчиков. Протез не двигаю.')
        shutdown()
        return

    if a.slide_cycles:
        print(f'Циклы: {a.slide_cycles} × (закрыть до касания, откат {a.retract_ms:.0f} мс → пауза {a.pause_s:.0f} с → '
              f'СКОЛЬЗИТЬ {a.slide_s:.0f} с (вправо/влево по очереди) → пауза {a.pause_s:.0f} с → открыть). Файл: {out}')
    else:
        drag_s = sum(d for _, d in scenario)
        print(f'Силы {forces}. Сценарий: {len(scenario)} шагов, {drag_s:.0f} с на силу. Файл: {out}')
    print('Шаги по сигналам: ПОКОЙ — отпустить; ТЯНУТЬ — нагрузить вбок, не сдвигая; СКОЛЬЗИТЬ — вести по столу. '
          'Ctrl+C — стоп.')
    summary = []
    try:
        st['t0'] = time.time()
        for li, force in enumerate(forces, 1):
            with lock:
                st['level'], st['raw'] = li, force
            if a.slide_cycles:                    # цикл: скользить (вправо/влево по очереди) → пауза
                scenario = [('slide_right' if li % 2 else 'slide_left', a.slide_s), ('rest', a.pause_s)]
            # 1. ноль
            open_hand()
            if a.slide_cycles and li > 1:
                print(f'\n=== цикл {li}/{len(forces)} ===')
            else:
                input(f'\n>>> Сила {force}% ({li}/{len(forces)}): поставьте раскрытую кисть к краю стола '
                      f'(не касаясь) и нажмите Enter...')
            set_phase('noise')
            with lock:
                st['buf'], st['ref'], st['dsh'], st['zeroF'] = [], None, None, None
            time.sleep(a.noise_s)
            with lock:
                st['zeroF'] = st['F'].copy()
                st['zeroG'] = {k: v.copy() for k, v in st['G'].items()}
                st['Frel'] = np.zeros(3)
                mc.set_sigma(list(st['m'][-60:]))       # шум F на неподвижной раскрытой кисти
            print(f'\n=== сила {force}% ({li}/{len(forces)}): подход...')

            # 2. подход до касания (детекция как у tm в hand_console: common/mag_contact.py)
            set_phase('approach')
            with lock:
                mc.start(list(st['m'][-5:]))
                st['hit_info'] = None
                hit.clear()
                st['detect'] = True
            t0 = time.time()
            if a.touch_grip:
                # максимальная скорость: «закрыть 100 %» без ШИМ; при касании сразу «открыть» на retract_ms
                hand.send(cmd_close(100))
                _sleep_until(time.perf_counter() + a.approach_s, hit.is_set)
                if hit.is_set() and a.retract_ms > 0:
                    hand.send(CMD_OPEN)
                    _sleep_until(time.perf_counter() + a.retract_ms / 1000)
                hand.send(CMD_STOP)
            else:
                pulse_close(hand.send, a.approach, a.speed / 100, a.pwm_ms / 1000, a.approach_s, hit.is_set)
            t_stop = time.time()
            with lock:
                st['detect'] = False
                info = st['hit_info']
            time.sleep(0.3)
            with lock:
                F_hold = st['Frel'].copy()
            if info:
                th, r, df = info
                k = int(np.argmax(r))
                print(f'  касание {CELLS[k]} через {th - t0:.2f} с, ответ через {(t_stop - th) * 1000:.0f} мс после '
                      f'кадра касания' + (f' (откат {a.retract_ms:.0f} мс)' if a.touch_grip and a.retract_ms > 0 else '')
                      + '; r: ' + ', '.join(f'{n} {v:.1f}' for n, v in zip(CELLS, r)) + ' σ')
            else:
                print(f'  касания нет за {a.approach_s} с (порог {mc.thr} σ)')
            print('  F пальцев после остановки: ' + ', '.join(f'{n} {v:.0f}' for n, v in zip(CELLS, F_hold)) + ' мкТл')

            # 3. эталон: ступени силы + 4. неподвижно на рабочей (в режиме касания — только неподвижно,
            #    мотор не включается: сила та, что осталась после остановки)
            with lock:
                st['buf'] = []
            if not a.touch_grip:
                set_phase('ramp')
                steps = np.unique(np.linspace(min(a.ramp_from, force), force,
                                              max(1, a.ramp_steps)).round().astype(int))
                for s in steps:
                    with lock:
                        st['raw'] = int(s)
                    hand.send(cmd_close(int(s)))
                    time.sleep(a.step_s)
                with lock:
                    st['raw'] = force
            set_phase('ref')
            if not a.touch_grip:
                hand.send(cmd_close(force))
            time.sleep(a.ref_s)
            with lock:
                buf = st['buf']
                st['buf'] = None
            Fb = np.array([b[0] for b in buf])
            Gb = {k: np.array([b[1][k] for b in buf]) for k in buf[0][1]}
            ref = RefTable(Fb, Gb)
            with lock:
                st['ref'] = ref
            print('  эталон: ' + ', '.join(f'{n} F {ref.f_range(c)[0]:.0f}..{ref.f_range(c)[1]:.0f}'
                                           for c, n in enumerate(CELLS)) + ' мкТл')

            # 5. вождение по сценарию
            set_phase('drag')
            stats = {}
            print(f'  Приготовиться: первый шаг — {KEY_NAMES[scenario[0][0]]}')
            beep(scenario[0][0])
            t_step = time.time() + a.warn_s
            last = 0.0
            for i, (lab, dur) in enumerate(scenario):
                nxt = scenario[i + 1][0] if i + 1 < len(scenario) else None
                warned = False
                while time.time() < t_step:
                    time.sleep(0.005)
                with lock:
                    st['label'], st['cue_t'] = lab, t_step
                beep(lab, long=True)
                print(f'\n\n  >>>>>>  {KEY_NAMES[lab]}  <<<<<<   {dur:.0f} с   ({i + 1}/{len(scenario)})')
                t_end = t_step + dur
                while time.time() < t_end:
                    if nxt and not warned and time.time() > t_end - a.warn_s:
                        warned = True
                        print(f'\n      дальше: {KEY_NAMES[nxt]}', end='')
                    with lock:
                        dsh, F = st['dsh'], st['Frel']
                    if dsh is not None:
                        if time.time() - t_step > 0.4:               # без реакции человека
                            stats.setdefault(lab, []).append(dsh.copy())
                        if time.time() - last > 0.15:
                            last = time.time()
                            print('\r      F/|ΔG|: ' + '  '.join(
                                f'{n[0]} {F[c]:5.0f}/{dsh[c]:5.1f}' for c, n in enumerate(CELLS)) + '   ',
                                  end='', flush=True)
                    time.sleep(0.01)
                t_step = t_end
            print()
            summary.append((force, {lab: np.median(v, axis=0) for lab, v in stats.items()}))
            f_csv.flush()
        open_hand()
    except KeyboardInterrupt:
        print('\nПрервано (Ctrl+C).')
        open_hand()
    finally:
        rows = st['rows']
        shutdown()

    print('\nМедиана |ΔG| (сдвиг магнита сверх статики), мкТл:')
    for force, med in summary:
        for lab in KEY_NAMES:
            if lab in med:
                print(f'  сила {force:3d}%  {lab:11}  ' +
                      '  '.join(f'{n} {med[lab][c]:5.1f}' for c, n in enumerate(CELLS)))
    print(f'Готово. Строк: {rows}. Файл: {out}\nАнализ: cd analysis && uv run python slip_table_analysis.py '
          f'--file {os.path.relpath(out, os.path.join(HERE, "..", "..", "analysis"))}')


if __name__ == '__main__':
    main()
