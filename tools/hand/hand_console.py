"""
Ручное управление кистью Indy из консоли: открыть / закрыть на заданное время, с заданной силой,
программный ШИМ. Если указан порт датчиков — после каждой команды печатается вес (весы HX711).

Команды (несколько — через «;», например:  c 800 30; o 20; pc 1000 20 50):
  c [мс] [сила%]          закрыть с силой (сырой % в 01 11 01 <pct>) на мс, затем стоп;
                          без мс — закрывать, пока не введёте s
  o [мс]                  открыть на мс, затем стоп; без мс — до s
  pc <мс> <окно мс> <доля%> [сила%]   закрывать ШИМ: в каждом окне доля — «закрыть», остальное — стоп
  po <мс> <окно мс> <доля%>           то же для открытия
  s                       стоп
  f <сила%>               сила по умолчанию для c и pc (сейчас 60)
  z                       ноль весов (текущее показание)
  w                       показать вес
  wait <мс>               пауза (для последовательностей)
  rep <мс>                во время c/o повторять команду движения каждые мс (0 — не повторять; сейчас 0)
  emg off | emg on        не слать / слать фоновый поток ЭМГ «покой» (01 20 02 00 00, ~35 Гц)
  rx                      что присылала плата кисти (кроме опросов версии), с последнего rx
  t <скорость%> [сила%] [окно мс] [таймаут мс]
                          закрываться до касания: ШИМ с долей включения = скорость (100 — без ШИМ),
                          окно по умолчанию 20 мс, таймаут 5000 мс; стоп, как только вес вырос над
                          нулём на порог 3 кадра подряд. Ноль берётся в начале команды. Нужны весы.
  thr <ед. весов>         порог касания для t (сейчас 300)

  Касание по магнитометрам (без весов, common/mag_contact.py): признак ячейки, как в прошивке, —
  F = (|MLX3 − MLX0| + |MLX2 − MLX1|) / 2 в мкТл; разность пар убирает поле Земли.
  mcal [скорость%] [мс]   проверка БЕЗ ПРЕДМЕТА: шум F на неподвижной кисти (σ) и насколько F
                          меняется от самого движения (закрыть ШИМ 20 мс, по умолчанию 50 % и 1500 мс,
                          открыть). Должно быть заметно ниже порога mthr.
  tm <скорость%> [сила%] [палец] [таймаут мс]
                          закрываться ШИМ (окно 20 мс) до касания по магнитометрам; палец: a — любой
                          (по умолчанию), t / i / m — большой / указательный / средний. После стопа
                          печатает время, реакцию и насколько сигнал вырос уже после стопа.
                          Кадры пишутся в tools/data/tm_<время>.csv.
  mthr <σ>                порог касания для tm, в единицах шума (сейчас 6)

  Проскальзывание в прямом эфире (common/slip_live.py): по каждому пальцу r — РОСТ касательной
  нагрузки |ΔG|/F (ΔG — сдвиг магнита от эталона хвата) над её минимумом с прошлого события; возврат
  подушечки назад (отпустили) срабатываний не даёт. r > порога 2 кадра подряд — «СКОЛЬЖЕНИЕ»; конец —
  нагрузка вернулась (r < 0.7·порога) или перестала меняться (~0.4 с) — тогда отсчёт от нового положения.
  Если F пальца упала ниже 30 % от F хвата, палец выпадает из детекции; если так у всех — «ПРЕДМЕТ
  ПОТЕРЯН», детектор выключается. Команды мотора (c, o, pc, po, t, tm, mcal) детектор выключают.
  Значения пишутся в tools/data/slip_live_<время>.csv.
  g [откат мс] [сила%]    сжать до касания: «закрыть» (сила 100 %) без ШИМ, при касании (как tm)
                          сразу «открыть» на откат мс (30), стоп; через 0.5 с — эталон, детектор ВКЛ.
                          Кисть перед g раскрыта и ничего не касается (снимается ноль).
  sthr <r>                порог скольжения (сейчас 1.0)
  sarm                    новый эталон в текущем положении (кисть неподвижно держит) и ВКЛ
  sz                      ноль F и G (кисть раскрыта, ничего не касается) — g делает это сам
  soff                    детектор выключить
  s?                      текущие r и F по пальцам
  Реакция на скольжение — дожим: при «СКОЛЬЖЕНИЕ» сразу «закрыть с силой 60 %» на 30 мс, стоп, если F
  пальцев с контактом ещё ниже предела (сжатие не сильное); через 0.5 с — новый эталон, детектор ВКЛ.
  sq on | sq off          дожим включить / выключить (сейчас ВКЛ)
  sq <сила%> [мс]         сила и длительность дожима (60 %, 30 мс)
  sqf <мкТл>              не дожимать, если F любого пальца с контактом ≥ этого (250 мкТл ≈ F при силе 78 %)
  q                       стоп и выход
Время отсчитывается точно (до ~0.1 мс), но каждая команда мотору теряет ~20 мс на старт.

Запуск (из папки tools/hand):
  py -3 hand_console.py                         # только кисть, COM20
  py -3 hand_console.py --sensor-port COM21     # + вес, t, mcal, tm, проскальзывание (g, sthr, ...)
Ctrl+C — стоп мотора.
"""
import argparse
import csv
import os
import sys
import threading
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from hand_link import CMD_OPEN, CMD_STOP, HandUart, _sleep_until, cmd_close, pulse_close
from mag_contact import CELLS, MagContact
from slip_live import LiveSlip

DATA = os.path.join(HERE, '..', 'data')
CELL_KEYS = {'a': (0, 1, 2), 't': (0,), 'i': (1,), 'm': (2,)}


def peak3(vals):
    """Максимум скользящей медианы по 3 кадрам — одиночный сбой HX711 пиком не считается."""
    v = np.asarray(vals, float)
    if len(v) < 3:
        return float(np.median(v))
    return float(np.max(np.median(np.lib.stride_tricks.sliding_window_view(v, 3), axis=1)))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--hand-port', default='COM20')
    p.add_argument('--sensor-port', help='COM-порт платы датчиков — показывать вес')
    p.add_argument('--sensor-baud', type=int, help='скорость UART датчиков (по умолчанию — автоопределение)')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    lock = threading.Lock()
    st = {'w': [], 'zero': 0.0, 'force': 60, 'peak': None, 'rep': 0.0, 'thr': 300.0,
          'm': [], 'rec': None, 'tm': None}
    mc = MagContact()
    hit = threading.Event()
    log = {'f': None, 'w': None, 'n': 0, 't0': time.time()}
    slip = LiveSlip(thr=1.0)
    # реакция на скольжение: дожать «закрыть с силой» на ms, если F пальцев ещё ниже fmax
    sq = {'on': True, 'force': 60, 'ms': 30.0, 'fmax': 250.0, 'settle': 0.5, 'busy': False, 'n': 0}

    def squeeze():
        try:
            with lock:
                F_now = slip.F[slip.contact] if slip.contact.any() else slip.F[slip.valid]
            if F_now.size and F_now.max() >= sq['fmax']:
                print(f'  (сжатие уже сильное: F до {F_now.max():.0f} ≥ {sq["fmax"]:.0f} мкТл — не дожимаю)',
                      flush=True)
                return
            with lock:
                slip.disarm()                     # дожим сам меняет сигнал — детектор на паузе
            t0 = time.perf_counter()
            hand.send(cmd_close(sq['force']))
            _sleep_until(t0 + sq['ms'] / 1000)
            hand.send(CMD_STOP)
            sq['n'] += 1
            time.sleep(sq['settle'])
            with lock:
                Fh = slip.arm()                   # новый хват — новый эталон
            print(f'  <<< ДОЖАЛ #{sq["n"]}: {sq["force"]}% на {sq["ms"]:.0f} мс; F хвата теперь '
                  + ', '.join(f'{n} {v:.0f}' for n, v in zip(CELLS, Fh)) + ' мкТл, детектор снова ВКЛ', flush=True)
        except Exception as e:                   # noqa: BLE001 — поток не должен падать молча
            print(f'  ошибка дожима: {e}', flush=True)
        finally:
            sq['busy'] = False
    slog = {'f': None, 'w': None}
    sensors = None
    if a.sensor_port:
        from sensor_link import SerialLink
        from sensor_text import MAG_COLS, TextSensors

        def on_frame(t_abs, f):
            with lock:
                st['w'].append(f['weight'])
                del st['w'][:-50]
                if st['peak'] is not None:
                    st['peak'].append(f['weight'])
                st['m'].append(f['mag'])
                del st['m'][:-100]
                if st['rec'] is not None:              # запись отрезка (калибровка mcal)
                    st['rec'].append(f['mag'])
                tm = st['tm']
                ev = slip.feed(f['mag'])               # детектор проскальзывания (если включён)
                if slip.armed and slog['w']:
                    slog['w'].writerow([f'{t_abs - log["t0"]:.4f}', int(slip.slipping)]
                                       + [f'{v:.3f}' for v in slip.r] + [f'{v:.3f}' for v in slip.load]
                                       + [f'{v:.1f}' for v in slip.F]
                                       + [int(v) for v in slip.contact])
            if ev:
                if ev[0] == 'start':
                    print(f'\n  >>> СКОЛЬЖЕНИЕ #{slip.n_events}: {ev[1]}, r = {ev[2]:.2f} '
                          f'(порог {slip.thr:.2f})', flush=True)
                    if sq['on'] and not sq['busy']:
                        sq['busy'] = True
                        threading.Thread(target=squeeze, daemon=True).start()
                elif ev[0] == 'lost':
                    print(f'\n  !!! ПРЕДМЕТ ПОТЕРЯН: F упала у {", ".join(ev[1])}'
                          + (' (во время скольжения)' if ev[2] else '')
                          + ' — детектор выключен; после нового хвата: g или sarm', flush=True)
                else:
                    print(f'\n  --- скольжение закончилось ({ev[1]:.2f} с' +
                          (', предмет держится в новом положении — отсчёт от него)' if ev[2] else ')'), flush=True)
            if tm is None:
                return
            # касание считается здесь, в потоке приёма, — без задержки опроса из главного потока
            ok, r, df = mc.update(f['mag'], tm['cells'])
            if ok and not hit.is_set():
                tm['t_hit'], tm['r_hit'], tm['df_hit'] = t_abs, r, df
                hit.set()
            tm['r_max'] = np.maximum(tm['r_max'], r)
            if hit.is_set():
                tm['r_after'] = np.maximum(tm['r_after'], r)
            if log['w']:
                log['w'].writerow([f'{t_abs - log["t0"]:.4f}', tm['no'], 'after' if hit.is_set() else 'approach',
                                   int(ok)] + [f'{v:.2f}' for v in r] + [f'{v:.3f}' for v in df]
                                  + [f'{f["weight"]:.2f}'] + list(f['mag']))

        sensors = TextSensors(lambda cb: SerialLink(a.sensor_port, a.sensor_baud, cb), on_frame)
        if not sensors.wait_frames(3.0):
            print('Датчики: строк нет — вес показываться не будет')
            sensors.close()
            sensors = None

    hand = HandUart(a.hand_port)
    print(f'Протез: жду опроса платы на {a.hand_port}...')
    if not hand.linked.wait(timeout=8):
        print('ОШИБКА: протез не ответил за 8 с.')
        hand.close()
        return
    print('Связь есть. Команды: c [мс] [сила%] | o [мс] | pc/po <мс> <окно мс> <доля%> [сила%] | t <скорость%> [сила%] | mcal | tm <скорость%> [сила%] [a/t/i/m] | g [откат мс] | sthr | sarm | soff | s? | sq | sqf | s | f | z | w | thr | mthr | wait | q'
          '\n  (несколько через «;», help — подсказка)')

    def weight():
        """Медиана последних 5 кадров минус ноль (медиана отбрасывает сбои HX711)."""
        with lock:
            w = st['w'][-5:]
        return float(np.median(w)) - st['zero'] if w else None

    def timed(cmd, ms):
        t0 = time.perf_counter()
        end = t0 + ms / 1000
        hand.send(cmd)
        rep = st['rep'] / 1000
        if rep > 0:
            k = 1
            while t0 + k * rep < end:
                _sleep_until(t0 + k * rep)
                hand.send(cmd)
                k += 1
        _sleep_until(end)
        hand.send(CMD_STOP)

    def pwm(cmd, ms, window_ms, duty):
        t0 = time.perf_counter()
        n = int(ms // window_ms)
        for k in range(n):
            start = t0 + k * window_ms / 1000
            _sleep_until(start)
            hand.send(cmd)
            _sleep_until(start + window_ms * duty / 100 / 1000)
            hand.send(CMD_STOP)
        _sleep_until(t0 + ms / 1000)
        hand.send(CMD_STOP)
        return n

    def run(cmd_line):
        parts = cmd_line.split()
        if not parts:
            return True
        c = parts[0].lower()
        numeric = {'emg': 0, 'tm': 2}.get(c, len(parts) - 1)   # у tm дальше буква пальца
        if c == 'sq' and len(parts) > 1 and not parts[1].replace('.', '', 1).isdigit():
            numeric = 0                                         # sq on / sq off
        args = [float(x) for x in parts[1:1 + numeric]]
        if c in ('c', 'o', 'pc', 'po', 't', 'tm', 'mcal') and slip.armed:
            # кисть сейчас поедет — сигнал магнитометров изменится сам: детектор выключаем
            slip.disarm()
            print('  (детектор скольжения выключен — команда мотора; после нового хвата: g или sarm)')
        if c in ('c', 'pc') and ((c == 'c' and len(args) >= 2) or (c == 'pc' and len(args) >= 4)):
            force = int(args[1] if c == 'c' else args[3])
        else:
            force = st['force']
        note = ''
        if c == 'q':
            return False
        if c in ('help', '?'):
            print(__doc__)
        elif c == 's':
            hand.send(CMD_STOP)
            note = 'стоп'
        elif c == 'f':
            st['force'] = max(1, min(100, int(args[0])))
            note = f'сила по умолчанию {st["force"]}%'
        elif c == 'z':
            w = weight()
            if w is None:
                note = 'весов нет'
            else:
                st['zero'] += w
                note = f'ноль весов = {st["zero"]:.2f}'
        elif c == 'w':
            w = weight()
            note = 'весов нет' if w is None else f'вес {w:.1f}'
        elif c == 'rep':
            st['rep'] = max(0.0, args[0])
            note = f'повтор команды движения каждые {st["rep"]:.0f} мс' if st['rep'] else 'без повтора'
        elif c == 'emg':
            on = parts[1].lower() == 'on'
            hand.emg_stream = on
            note = 'поток ЭМГ включён' if on else 'поток ЭМГ выключен (только ответы на опрос версии)'
        elif c == 'mthr':
            mc.thr = args[0]
            note = f'порог касания по магнитометрам {mc.thr:.1f} σ'
        elif c == 'sq':
            if len(parts) > 1 and parts[1].lower() in ('on', 'off'):
                sq['on'] = parts[1].lower() == 'on'
            elif args:
                sq['force'] = max(1, min(100, int(args[0])))
                if len(args) > 1:
                    sq['ms'] = max(1.0, args[1])
                sq['on'] = True
            note = (f'дожим при скольжении {"ВКЛ" if sq["on"] else "выкл"}: {sq["force"]}% на {sq["ms"]:.0f} мс, '
                    f'если F < {sq["fmax"]:.0f} мкТл; дожато раз: {sq["n"]}')
        elif c == 'sqf':
            sq['fmax'] = args[0]
            note = f'дожимать, только пока F пальцев < {sq["fmax"]:.0f} мкТл'
        elif c in ('g', 'sz', 'sarm', 'soff', 'sthr', 's?'):
            if not sensors:
                raise ValueError(f'{c}: нужны датчики — запустите с --sensor-port COM21')
            if c == 'sthr':
                slip.thr = args[0]
                note = f'порог скольжения r = |ΔG|/F > {slip.thr:.2f}'
            elif c == 'soff':
                slip.disarm()
                note = 'детектор скольжения выключен'
            elif c == 's?':
                note = ('детектор ' + ('ВКЛ' if slip.armed else 'выкл') + f', порог {slip.thr:.2f}; '
                        + ', '.join(f'{n}: r {r:.2f}, F {F_:.0f}' + ('' if v else ' (нет контакта)')
                                    for n, r, F_, v in zip(CELLS, slip.r, slip.F,
                                                           slip.contact if slip.armed else slip.valid))
                        + (f'; событий {slip.n_events}' if slip.n_events else ''))
            elif c == 'sz':
                slip.set_zero()
                note = 'ноль F и G снят (кисть должна быть раскрыта и ничего не касаться)'
            else:
                if c == 'g':
                    # сжать на максимальной скорости до касания (как tm), сразу приоткрыть, снять эталон
                    retract = args[0] if args else 30.0
                    force = int(args[1]) if len(args) > 1 else 100
                    timeout = 5.0
                    with lock:
                        rest = list(st['m'][-30:])
                    slip.set_zero()
                    mc.set_sigma(rest)
                    mc.start(rest[-5:])
                    hit.clear()
                    tm = {'no': 0, 'cells': (0, 1, 2), 't_hit': None, 'r_hit': None,
                          'r_max': np.zeros(3), 'r_after': np.zeros(3)}
                    with lock:
                        st['tm'] = tm
                    t0 = time.time()
                    hand.send(cmd_close(force))
                    _sleep_until(time.perf_counter() + timeout, hit.is_set)
                    if hit.is_set() and retract > 0:
                        hand.send(CMD_OPEN)
                        _sleep_until(time.perf_counter() + retract / 1000)
                    hand.send(CMD_STOP)
                    t_stop = time.time()
                    with lock:
                        st['tm'] = None
                    if tm['t_hit'] is None:
                        note = f'касания нет за {timeout:.0f} с — детектор не включён'
                        print('  ' + note)
                        return True
                    k = int(np.argmax(tm['r_hit']))
                    print(f'  касание {CELLS[k]} через {tm["t_hit"] - t0:.2f} с, стоп через '
                          f'{(t_stop - tm["t_hit"]) * 1000:.0f} мс (откат {retract:.0f} мс); жду 0.5 с...')
                    time.sleep(0.5)
                Fh = slip.arm()
                if slog['f'] is None:
                    path = os.path.join(DATA, f'slip_live_{time.strftime("%Y%m%d_%H%M%S")}.csv')
                    slog['f'] = open(path, 'w', newline='', encoding='utf-8-sig')
                    slog['w'] = csv.writer(slog['f'])
                    slog['w'].writerow(['t_s', 'slip'] + [f'r_{n}' for n in CELLS] + [f'load_{n}' for n in CELLS]
                                       + [f'F_{n}' for n in CELLS]
                                       + [f'contact_{n}' for n in CELLS])
                    print(f'  запись r и F: {path}')
                note = ('детектор ВКЛ: F хвата ' + ', '.join(f'{n} {v:.0f}' + ('' if ok else ' (нет контакта)')
                                                          for n, v, ok in zip(CELLS, Fh, slip.valid))
                        + f' мкТл; порог r > {slip.thr:.2f}')
        elif c == 'mcal':
            if not sensors:
                raise ValueError('mcal нужны датчики — запустите с --sensor-port COM21')
            speed = args[0] if args else 50.0
            ms = args[1] if len(args) > 1 else 1500.0
            timed(CMD_OPEN, 800)
            time.sleep(0.4)
            with lock:
                rest = list(st['m'][-20:])
            mc.set_sigma(rest)
            segs = []
            for name, go in (('закрываю', lambda: pulse_close(hand.send, st['force'], speed / 100, 0.02, ms / 1000)),
                             ('открываю', lambda: pwm(CMD_OPEN, ms, 20, speed))):
                print(f'  {name} без предмета...')
                with lock:
                    st['rec'] = list(st['m'][-3:])
                go()
                time.sleep(0.2)
                with lock:
                    segs.append(st['rec'])
                    st['rec'] = None
            free = mc.check_free(segs)
            note = (f'шум F: σ {", ".join(f"{n} {s:.2f}" for n, s in zip(CELLS, mc.sigma))} мкТл; '
                    f'движение без касания меняет F до {", ".join(f"{n} {v:.1f}" for n, v in zip(CELLS, free))} σ'
                    f' (кадров {sum(len(s) for s in segs)}). Порог {mc.thr:.1f} σ'
                    + (' — не выше, чем шумит движение: поднимите mthr' if free.max() >= mc.thr else ''))
        elif c == 'tm':
            if not sensors:
                raise ValueError('tm нужны датчики — запустите с --sensor-port COM21')
            if not args:
                raise ValueError('нужно: tm <скорость%> [сила%] [палец a/t/i/m] [таймаут мс]')
            speed = max(5.0, min(100.0, args[0]))
            force = int(args[1]) if len(args) > 1 else st['force']
            cells = CELL_KEYS.get(parts[3].lower() if len(parts) > 3 else 'a')
            if cells is None:
                raise ValueError('палец: a, t, i или m')
            timeout = float(parts[4]) / 1000 if len(parts) > 4 else 5.0
            if log['f'] is None:
                path = os.path.join(DATA, f'tm_{time.strftime("%Y%m%d_%H%M%S")}.csv')
                log['f'] = open(path, 'w', newline='', encoding='utf-8-sig')
                log['w'] = csv.writer(log['f'])
                log['w'].writerow(['t_s', 'cmd', 'phase', 'hit'] + [f'r_{n}' for n in CELLS]
                                  + [f'dF_{n}_uT' for n in CELLS] + ['weight'] + MAG_COLS)
                print(f'  кадры: {path}')
            log['n'] += 1
            with lock:
                rest = list(st['m'][-30:])
            if mc.free_max is None:
                mc.set_sigma(rest)                     # шум F по кадрам до команды (кисть стоит)
            mc.start(rest[-5:])
            hit.clear()
            tm = {'no': log['n'], 'cells': cells, 't_hit': None, 'r_hit': None,
                  'r_max': np.zeros(3), 'r_after': np.zeros(3)}
            with lock:
                st['tm'] = tm
            t_start = time.time()
            if speed >= 100:
                hand.send(cmd_close(force))
                _sleep_until(time.perf_counter() + timeout, hit.is_set)
                hand.send(CMD_STOP)
            else:
                pulse_close(hand.send, force, speed / 100, 0.02, timeout, hit.is_set)
            t_stop = time.time()
            time.sleep(0.4)                            # что происходит с сигналом после стопа
            with lock:
                st['tm'] = None
            log['f'].flush()
            how = 'без ШИМ' if speed >= 100 else f'ШИМ 20 мс × {speed:.0f}%'
            if tm['t_hit'] is None:
                note = (f'касания нет за {timeout:.1f} с ({how}, сила {force}%); '
                        f'максимум r: {", ".join(f"{n} {v:.1f}" for n, v in zip(CELLS, tm["r_max"]))} σ')
            else:
                rh, ra = tm['r_hit'], tm['r_after']
                k = max(cells, key=lambda i: rh[i])
                note = (f'касание {CELLS[k]} через {tm["t_hit"] - t_start:.2f} с ({how}, сила {force}%); '
                        f'стоп через {(t_stop - tm["t_hit"]) * 1000:.0f} мс после кадра касания\n'
                        f'    ΔF при касании: {", ".join(f"{n} {v:+.2f}" for n, v in zip(CELLS, tm["df_hit"]))} мкТл; '
                        f'r при касании: {", ".join(f"{n} {v:.1f}" for n, v in zip(CELLS, rh))} σ; '
                        f'после стопа до: {", ".join(f"{n} {v:.1f}" for n, v in zip(CELLS, ra))} σ '
                        f'(рост {ra[k] / max(rh[k], 1e-9):.1f}× — сколько кисть дожала, пока реагировали)')
        elif c == 'thr':
            st['thr'] = args[0]
            note = f'порог касания {st["thr"]:.0f}'
        elif c == 't':
            if not sensors:
                raise ValueError('t нужны весы — запустите с --sensor-port COM21')
            if not args:
                raise ValueError('нужно: t <скорость%> [сила%] [окно мс] [таймаут мс]')
            speed = max(5.0, min(100.0, args[0]))
            force = int(args[1]) if len(args) > 1 else st['force']
            window = args[2] if len(args) > 2 else 20.0
            timeout = (args[3] if len(args) > 3 else 5000.0) / 1000
            zero = weight() + st['zero']               # ноль — текущее показание (палец ещё не касается)

            def touched():
                with lock:
                    tail = st['w'][-3:]
                return len(tail) == 3 and min(tail) > zero + st['thr']

            with lock:
                st['w'] = st['w'][-1:]                 # кадры до команды касанием не считаются
                st['peak'] = []
            if speed >= 100:
                t0 = time.perf_counter()
                hand.send(cmd_close(force))
                while time.perf_counter() - t0 < timeout and not touched():
                    time.sleep(0.002)
                el = time.perf_counter() - t0
                hand.send(CMD_STOP)
            else:
                el = pulse_close(hand.send, force, speed / 100, window / 1000, timeout, touched)
            touched_now = touched()
            time.sleep(0.15)
            with lock:
                peak, st['peak'] = st['peak'], None
            how = 'без ШИМ' if speed >= 100 else f'ШИМ {window:.0f} мс × {speed:.0f}%'
            note = (f'касание через {el:.2f} с' if touched_now else f'касания нет за {timeout:.1f} с') + \
                f' ({how}, сила {force}%)  | вес {weight() + st["zero"] - zero:.1f}' + \
                (f', пик {peak3(peak) - zero:.1f}' if peak else '') + ' от нуля в начале команды'
        elif c == 'rx':
            items = list(hand.rx)
            hand.rx.clear()
            if items:
                lines = [f'    {t - items[0][0]:7.3f} с  {b.hex(" ").upper()}' for t, b in items[-30:]]
                note = f'от платы {len(items)} посылок:\n' + '\n'.join(lines)
            else:
                note = 'от платы ничего, кроме опросов версии'
        elif c == 'wait':
            time.sleep(args[0] / 1000)
            note = f'пауза {args[0]:.0f} мс'
        elif c == 'c' and not args:
            hand.send(cmd_close(force))
            note = f'закрываю с силой {force}% до команды s'
        elif c == 'o' and not args:
            hand.send(CMD_OPEN)
            note = 'открываю до команды s'
        elif c in ('c', 'o', 'pc', 'po'):
            with lock:
                st['peak'] = []
            cmd = cmd_close(force) if c in ('c', 'pc') else CMD_OPEN
            name = f'закрыть ({force}%)' if c in ('c', 'pc') else 'открыть'
            if c in ('c', 'o'):
                timed(cmd, args[0])
                note = f'{name} {args[0]:.0f} мс'
            else:
                if len(args) < 3:
                    raise ValueError('нужно: pc/po <мс> <окно мс> <доля%> [сила%]')
                n = pwm(cmd, args[0], args[1], args[2])
                note = f'{name} ШИМ {args[0]:.0f} мс, окно {args[1]:.0f} мс × {args[2]:.0f}% ({n} импульсов)'
            time.sleep(0.15)                  # вес успевает отреагировать
            with lock:
                peak, st['peak'] = st['peak'], None
            if sensors and peak:
                note += f'  | вес {weight():.1f}, пик {peak3(peak) - st["zero"]:.1f}'
        else:
            raise ValueError(f'неизвестная команда {c!r} (help — подсказка)')
        if note:
            print('  ' + note)
        return True

    try:
        while True:
            try:
                line = input('> ')
            except EOFError:
                break
            go = True
            for cmd_line in line.split(';'):
                try:
                    go = run(cmd_line)
                except (ValueError, IndexError) as e:
                    hand.send(CMD_STOP)
                    print(f'  ошибка: {e}')
                    break
                if not go:
                    break
            if not go:
                break
    except KeyboardInterrupt:
        print('\nCtrl+C — стоп')
    finally:
        hand.send(CMD_STOP)
        time.sleep(0.1)
        hand.close()
        if sensors:
            sensors.close()
        if log['f']:
            log['f'].close()
        if slog['f']:
            slog['f'].close()


if __name__ == '__main__':
    main()
