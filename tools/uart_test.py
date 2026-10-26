"""
UART-тесты платы через порт датчика ЭМГ (115200, старый протокол Motorica).

В фоне всегда: ответ на опрос версии (6) и поток 01 20 02 A B ~35 Гц.
Режимы:
  py -3 uart_test.py indy        — проверка команд привода Indy (0x10/0x0F/0x12/0x11/0x13)
  py -3 uart_test.py pulses N1,N2,...  — из открытого положения импульсы закрытия
                                   длиной N пакетов ЭМГ (A=255), пауза 2 с
Опции: UART_PORT (COM20)
"""
import os
import sys
import threading
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emg_emulator import parse_stream, describe, pack

PORT = os.environ.get('UART_PORT', 'COM20')
PERIOD = 0.028

st = {'a': 0, 'b': 0, 'run': True, 'sent': 0}
lock = threading.Lock()
ser = serial.Serial(PORT, 115200, timeout=0)
ser.reset_input_buffer()
t0 = time.time()


def log(msg):
    print(f'{time.time() - t0:7.3f}s {msg}', flush=True)


def write(raw, note=''):
    with lock:
        ser.write(raw)
    log(f'-> {raw.hex(" ").upper()}  {note}')


def worker():
    buf = bytearray()
    nxt = time.time()
    while st['run']:
        with lock:
            buf += ser.read(256)
        pk, buf = parse_stream(buf)
        for p in pk:
            if p[:2] == b'\x81\x80':
                with lock:
                    ser.write(pack(0x01, 0x80, [6]))
            else:
                log(f'<- {describe(p)}')
        if time.time() >= nxt:
            with lock:
                ser.write(pack(0x01, 0x20, [st['a'], st['b']]))
            st['sent'] += 1
            nxt += PERIOD
        time.sleep(0.001)


def emg(a, b):
    st['a'], st['b'] = a, b


def emg_pulse(channel, n_packets):
    """Ровно n пакетов со значением 255 на канале, затем 0."""
    start = st['sent']
    emg(255 if channel == 'a' else 0, 255 if channel == 'b' else 0)
    while st['sent'] - start < n_packets:
        time.sleep(0.001)
    emg(0, 0)


th = threading.Thread(target=worker, daemon=True)
th.start()
time.sleep(1.0)

mode = sys.argv[1] if len(sys.argv) > 1 else 'indy'
try:
    if mode == 'indy':
        def cmd(code, data=(), note=''):
            write(pack(0x01, code, list(data)), note)

        log('=== 1. Закрыть (0x10), 0.4 с, затем стоп (0x13) ===')
        cmd(0x10, note='DriveClose'); time.sleep(0.4); cmd(0x13, note='DriveBreak'); time.sleep(2)
        log('=== 2. Открыть (0x0F), 0.4 с, стоп ===')
        cmd(0x0F, note='DriveOpen'); time.sleep(0.4); cmd(0x13, note='DriveBreak'); time.sleep(2)
        log('=== 3. Закрыть с ШИМ 30% (0x12 1E), 0.6 с, стоп ===')
        cmd(0x12, [30], 'CloseDuty 30%'); time.sleep(0.6); cmd(0x13, note='DriveBreak'); time.sleep(2)
        log('=== 4. Открыть с ШИМ 30% (0x11 1E), 0.6 с, стоп ===')
        cmd(0x11, [30], 'OpenDuty 30%'); time.sleep(0.6); cmd(0x13, note='DriveBreak'); time.sleep(2)
        log('=== 5. Закрыть с ШИМ 100% (0x12 64), 0.6 с, стоп ===')
        cmd(0x12, [100], 'CloseDuty 100%'); time.sleep(0.6); cmd(0x13, note='DriveBreak'); time.sleep(2)
        log('=== 6. Открыть с ШИМ 100% (0x11 64), 0.6 с, стоп ===')
        cmd(0x11, [100], 'OpenDuty 100%'); time.sleep(0.6); cmd(0x13, note='DriveBreak'); time.sleep(1)

    elif mode == 'pulses':
        lens = [int(x) for x in (sys.argv[2] if len(sys.argv) > 2 else '1,2,3,5').split(',')]
        log('=== открываю полностью: B=255 на 1.5 с ===')
        emg(0, 255); time.sleep(1.5); emg(0, 0); time.sleep(2)
        for i, n in enumerate(lens, 1):
            log(f'=== импульс {i}/{len(lens)}: {n} пакетов A=255 (~{n * PERIOD * 1000:.0f} мс) ===')
            emg_pulse('a', n)
            log(f'    готово, отправлено пакетов с 255: {n}')
            time.sleep(2)
finally:
    emg(0, 0)
    time.sleep(0.2)
    st['run'] = False
    th.join(timeout=1)
    ser.close()
    log('конец, поток ЭМГ остановлен на A=B=0')
