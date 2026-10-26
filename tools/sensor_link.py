"""
Связь с платой TacMagVTOS: разбор кадров (Core/Inc/protocol.h) и транспорт — UART или BLE (HM-10).
Используется calib_tool.py и raw_monitor.py.

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


class SerialLink:
    """USB-UART на линиях USART2 (вместо HM-10 или параллельно ему), 115200 бод."""

    budget_bps = 8000   # полезных байт/с, с запасом от 11.5 КБ/с

    def __init__(self, port, baud, on_data):
        import serial
        self.ser = serial.Serial(port, baud, timeout=0.05)
        self.on_data = on_data
        self.alive = True
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        while self.alive:
            data = self.ser.read(256)
            if data:
                self.on_data(data)

    def write(self, data: bytes):
        self.ser.write(data)

    def close(self):
        self.alive = False
        self.ser.close()


class BleLink:
    """HM-10: сервис FFE0, характеристика FFE1 (notify + write)."""

    budget_bps = 1000   # на практике через HM-10 + Windows доходило ~700-1500 байт/с

    def __init__(self, target, on_data):
        from bleak import BleakClient, BleakScanner
        self.on_data = on_data
        self.loop = asyncio.new_event_loop()
        threading.Thread(target=self.loop.run_forever, daemon=True).start()

        async def connect():
            dev = await BleakScanner.find_device_by_filter(
                lambda d, _: d.address.upper() == target.upper() or (d.name or "") == target,
                timeout=10.0)
            if dev is None:
                raise RuntimeError(f"BLE-устройство '{target}' не найдено")
            client = BleakClient(dev)
            await client.connect()
            await client.start_notify(HM10_CHAR, lambda _, d: self.on_data(bytes(d)))
            return client

        self.client = self._call(connect())

    def _call(self, coro, timeout=20.0):
        return asyncio.run_coroutine_threadsafe(coro, self.loop).result(timeout)

    def write(self, data: bytes):
        self._call(self.client.write_gatt_char(HM10_CHAR, data, response=False))

    def close(self):
        try:
            self._call(self.client.disconnect())
        finally:
            self.loop.call_soon_threadsafe(self.loop.stop)
