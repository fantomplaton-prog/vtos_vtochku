"""
Эмулятор датчика 2EMG на UART-шине платы (старый протокол Motorica, 115200).
Плата — ведущая: опрашивает датчик по адресу 0x01 запросом версии (81 80 01 00 F0).

Режим 'handshake' (по умолчанию): отвечаем только на запрос версии и логируем всё,
что плата пришлёт дальше. Сигналов ЭМГ НЕ отправляем.

  py -3 emg_emulator.py [COM] [секунд] [версия]
"""
import sys
import time

import serial

sys.path.insert(0, __import__('os').path.dirname(__file__))
from probe_board import crc8

PORT = sys.argv[1] if len(sys.argv) > 1 else 'COM20'
DURATION = float(sys.argv[2]) if len(sys.argv) > 2 else 15
VERSION = int(sys.argv[3]) if len(sys.argv) > 3 else 1

ADDR = 0x01
CMD_NAMES = {0x01: 'SC_Gain', 0x02: 'SC_StreamMode', 0x03: 'SC_SelectData', 0x04: 'SC_GetResult',
             0x05: 'SC_Address', 0x20: 'EMG_DATA', 0x80: 'SC_Version', 0x81: 'SC_EnableDFU',
             0x82: 'SC_Protocol'}


def pack(addr, cmd, data):
    p = [addr, cmd, len(data), *data]
    return bytes(p + [crc8(p)])


def parse_stream(buf):
    """Выделяет пакеты [addr cmd len data*len crc] из буфера; возвращает (пакеты, остаток)."""
    out = []
    while len(buf) >= 4:
        n = buf[2]
        if len(buf) < 4 + n:
            break
        p = bytes(buf[:4 + n])
        if crc8(p[:-1]) == p[-1]:
            out.append(p)
            del buf[:4 + n]
        else:
            out.append(b'?' + bytes(buf[:1]))  # мусорный байт — сдвигаемся
            del buf[:1]
    return out, buf


def describe(p):
    if p[:1] == b'?':
        return f'мусор {p[1:].hex().upper()}'
    addr, cmd, n = p[0], p[1], p[2]
    rd = 'READ ' if addr & 0x80 else 'WRITE'
    return (f'{rd} addr=0x{addr & 0x7F:02X} {CMD_NAMES.get(cmd, f"0x{cmd:02X}"):14} '
            f'data=[{p[3:3 + n].hex(" ").upper()}]  raw={p.hex(" ").upper()}')


def main():
    s = serial.Serial(PORT, 115200, timeout=0.005)
    s.reset_input_buffer()
    t0 = time.time()
    buf = bytearray()
    print(f'{PORT}: эмулирую датчик 2EMG addr=0x01, версия={VERSION}, {DURATION:.0f} c')
    while time.time() - t0 < DURATION:
        buf += s.read(256)
        packets, buf = parse_stream(buf)
        for p in packets:
            t = time.time() - t0
            print(f'{t:7.3f}s <- {describe(p)}')
            if p[:1] != b'?' and p[0] == (ADDR | 0x80) and p[1] == 0x80:
                ans = pack(ADDR, 0x80, [VERSION])
                s.write(ans)
                print(f'{time.time() - t0:7.3f}s -> ответ версии: {ans.hex(" ").upper()}')
    s.close()
    print('конец')


if __name__ == '__main__':
    main()
