"""
Прямые команды привода Indy по UART (порт датчика ЭМГ, 115200).
В фоне: ответ на опрос версии (6) и поток ЭМГ A=B=0, как у настоящего датчика.

  py -3 uart_cmd.py open <сек>
  py -3 uart_cmd.py close <сек>
  py -3 uart_cmd.py close_duty <0..100> <сек>    — 01 11 01 <pct> (на этой плате закрывает)
  py -3 uart_cmd.py open_duty <0..100> <сек>     — 01 12 01 <pct> (на этой плате открывает)
Направления инвертированы относительно документации: 0x10 открывает, 0x0F закрывает.
  py -3 uart_cmd.py stop
Опции: UART_PORT (COM20)
"""
import os
import sys
import threading
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe_board import crc8

PORT = os.environ.get('UART_PORT', 'COM20')


def pack(addr, cmd, data=()):
    p = [addr, cmd, len(data), *data]
    return bytes(p + [crc8(p)])


STOP = pack(0x01, 0x13)
run = True
lock = threading.Lock()
ser = serial.Serial(PORT, 115200, timeout=0)
ser.reset_input_buffer()
t0 = time.time()


def log(msg):
    print(f'{time.time() - t0:6.3f}s {msg}', flush=True)


def send(raw, note):
    with lock:
        ser.write(raw)
    log(f'-> {raw.hex(" ").upper()}  {note}')


def background():
    buf = bytearray()
    nxt = time.time()
    while run:
        with lock:
            buf += ser.read(64)
            # ответ на опрос версии 81 80 01 00 F0
            i = buf.find(b'\x81\x80\x01\x00')
            if i >= 0:
                ser.write(pack(0x01, 0x80, [6]))
                del buf[:i + 5]
                linked.set()
            elif len(buf) > 64:
                del buf[:-8]
            if time.time() >= nxt:
                ser.write(pack(0x01, 0x20, [0, 0]))
                nxt += 0.028
        time.sleep(0.002)


linked = threading.Event()
threading.Thread(target=background, daemon=True).start()
# Плата принимает команды, похоже, только после того, как «увидела» датчик:
# ждём её опроса версии и ответа на него.
log('жду опроса версии от платы...')
if linked.wait(timeout=6):
    log('связь с платой есть (ответил на опрос версии)')
    time.sleep(0.1)
else:
    log('ВНИМАНИЕ: опроса версии за 6 с не было — шлю команду всё равно')

args = sys.argv[1:]
try:
    cmd = args[0] if args else 'stop'
    if cmd in ('open', 'close'):
        sec = float(args[1])
        # на этой плате направления инвертированы: 0x10 открывает, 0x0F закрывает
        send(pack(0x01, 0x10 if cmd == 'open' else 0x0F), cmd)
        time.sleep(sec)
    elif cmd in ('open_duty', 'close_duty'):
        pct, sec = int(args[1]), float(args[2])
        # предположительно так же инвертированы: 0x12 открывает, 0x11 закрывает
        send(pack(0x01, 0x12 if cmd == 'open_duty' else 0x11, [pct]), f'{cmd} {pct}%')
        time.sleep(sec)
    elif cmd != 'stop':
        print(__doc__)
finally:
    send(STOP, 'stop')
    time.sleep(0.1)
    run = False
    time.sleep(0.05)
    ser.close()
