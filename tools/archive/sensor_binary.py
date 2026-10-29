"""
АРХИВ. Бинарный протокол платы датчиков TacMagVTOS (Core/Inc/protocol.h, protocol.c).
Прошивка его СЕЙЧАС НЕ ОТПРАВЛЯЕТ: в main.c вызовы Protocol_Send() / Protocol_PollCommand()
закомментированы, по UART идёт только текстовая строка FormatRawFrame (см. common/sensor_text.py).
Чтобы вернуть: в main.c заменить FormatRawFrame + HAL_UART_Transmit на
Protocol_PollCommand(); Protocol_Send(&calib, raw, closure);

Кадры (little-endian, CRC-8 poly 0x07 по байтам 1..N-2):
  0xAA FingersFrame_t  10 байт   обычный режим, команда 'F'
  0xAB RawFrame_t      29 байт   калибровка, команда 'R'
  0xAC кадр осей       2+24*ячеек+12*fsr+1 байт, команда 0x80|mask, период 0x90|k (k*10 мс)
"""

import asyncio
import struct
import threading

RAW_FMT = "<B3f6HHBB"
RAW_LEN = struct.calcsize(RAW_FMT)          # 29
FINGERS_FMT = "<BHHHHB"
FINGERS_LEN = struct.calcsize(FINGERS_FMT)  # 10
assert RAW_LEN == 29 and FINGERS_LEN == 10

AXES_HEADER = 0xAC
AXES_MASK_FSR = 0x08
NUM_CELLS, NUM_MLX, AXES = 3, 4, 3
HEADERS = (0xAA, 0xAB, AXES_HEADER)

HM10_CHAR = "0000ffe1-0000-1000-8000-00805f9b34fb"


def crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def axes_frame_len(mask: int) -> int:
    cells = bin(mask & 0x07).count("1")
    fsr = 1 if mask & AXES_MASK_FSR else 0
    return 2 + cells * NUM_MLX * AXES * 2 + fsr * 6 * 2 + 1


def build_axes_frame(mask, mag, fsr):
    """Для тестов/мока: mag[cell][mlx][axis], fsr[6] — как формирует прошивка."""
    body = bytes([mask])
    for cell in range(NUM_CELLS):
        if mask & (1 << cell):
            body += struct.pack("<12h", *[v for m in mag[cell] for v in m])
    if mask & AXES_MASK_FSR:
        body += struct.pack("<6H", *fsr)
    return bytes([AXES_HEADER]) + body + bytes([crc8(body)])


class FrameParser:
    """Потоковый разбор: ищем заголовок, проверяем CRC и что за кадром идёт заголовок,
    при ошибке сдвигаемся на байт. BLE режет поток на пакеты по 20 байт — это не мешает."""

    def __init__(self):
        self.buf = bytearray()
        self.crc_errors = 0

    def feed(self, data: bytes):
        self.buf += data
        frames = []
        while self.buf:
            h = self.buf[0]
            if h == 0xAB:
                n = RAW_LEN
            elif h == 0xAA:
                n = FINGERS_LEN
            elif h == AXES_HEADER:
                if len(self.buf) < 2:
                    break
                if self.buf[1] > 0x0F:
                    del self.buf[0]
                    continue
                n = axes_frame_len(self.buf[1])
            else:
                del self.buf[0]
                continue
            if len(self.buf) < n:
                break
            chunk = bytes(self.buf[:n])
            # нулевое тело даёт CRC 0 — без проверки следующего заголовка ловим ложные кадры
            if crc8(chunk[1:-1]) != chunk[-1] or (len(self.buf) > n and self.buf[n] not in HEADERS):
                self.crc_errors += 1
                del self.buf[0]
                continue
            del self.buf[:n]
            frames.append(self._decode(h, chunk))
        return frames

    @staticmethod
    def _decode(h, chunk):
        if h == 0xAB:
            v = struct.unpack(RAW_FMT, chunk)
            return ("raw", {"ch": list(v[1:4]) + list(v[4:10]), "hall": v[10], "aperture": v[11]})
        if h == 0xAA:
            v = struct.unpack(FINGERS_FMT, chunk)
            return ("fingers", {"aperture": v[1], "press": list(v[2:5])})
        mask, pos = chunk[1], 2
        mag = [None] * NUM_CELLS             # None — ячейка не передавалась
        for cell in range(NUM_CELLS):
            if mask & (1 << cell):
                v = struct.unpack_from("<12h", chunk, pos)
                pos += 24
                mag[cell] = [list(v[m * 3:m * 3 + 3]) for m in range(NUM_MLX)]
        fsr = list(struct.unpack_from("<6H", chunk, pos)) if mask & AXES_MASK_FSR else None
        return ("axes", {"mask": mask, "mag": mag, "fsr": fsr})


