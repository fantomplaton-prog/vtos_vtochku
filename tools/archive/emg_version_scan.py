"""
Перебор значения версии в ответе на запрос платы 81 80 01 00 (SC_Version).
На каждый запрос отвечаем 01 80 01 <v>; останавливаемся, как только плата пришлёт
что-то, отличное от запроса версии, и логируем дальнейший обмен ещё 20 с.
Сигналов ЭМГ не отправляем.

  py -3 emg_version_scan.py [COM] [start] [stop]
"""
import os
import sys
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe_board import crc8
from emg_emulator import parse_stream, describe, pack

PORT = sys.argv[1] if len(sys.argv) > 1 else 'COM20'
START = int(sys.argv[2]) if len(sys.argv) > 2 else 0
STOP = int(sys.argv[3]) if len(sys.argv) > 3 else 255

s = serial.Serial(PORT, 115200, timeout=0.005)
s.reset_input_buffer()
t0 = time.time()
buf = bytearray()
v = START
found_at = None
last_v = None
while True:
    buf += s.read(256)
    packets, buf = parse_stream(buf)
    for p in packets:
        t = time.time() - t0
        is_ver_req = p[:2] == b'\x81\x80'
        if not is_ver_req or found_at is not None:
            print(f'{t:8.3f}s <- {describe(p)}   (последний ответ: версия {last_v})', flush=True)
            if found_at is None:
                found_at = t
                print(f'*** НОВАЯ КОМАНДА после версии {last_v} (0x{last_v:02X}) ***', flush=True)
        if is_ver_req and found_at is None:
            if v > STOP:
                break
            s.write(pack(0x01, 0x80, [v]))
            last_v = v
            if v % 16 == 0:
                print(f'{t:8.3f}s пробую версии {v}..{min(v + 15, STOP)}', flush=True)
            v += 1
    if found_at is not None and time.time() - t0 - found_at > 20:
        break
    if found_at is None and v > STOP and time.time() - t0 > 5:
        # ждём ещё один запрос, чтобы увидеть реакцию на последнее значение
        pass
    if time.time() - t0 > 2.5 * (STOP - START + 1) + 30:
        break
s.close()
print('итог:', f'новая команда после версии {last_v}' if found_at else 'ни одно значение не принято')
