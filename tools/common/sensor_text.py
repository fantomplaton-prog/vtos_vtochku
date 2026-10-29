"""
Текстовый протокол платы датчиков (FormatRawFrame в прошивке):
одна строка на кадр, числа через пробел, конец строки \\r\\n:

  adc0 adc1 adc2 adc3 adc4 adc5 hall hall_filt weight  THUMB_MLX0_X Y Z ... MIDDLE_MLX3_X Y Z

  adc0..5   — сырые FSR с АЦП, в порядке каналов АЦП (не пальцев), %lu
  hall      — сырой датчик Холла, %lu
  hall_filt — Холл после EMA-фильтра (в прошивке поле названо angle), %.4f
  weight    — тензодатчик HX711, %.4f
  магнитометры — 3 ячейки × 4 MLX × 3 оси (thumb MLX0 XYZ … middle MLX3 XYZ), %d
Итого 45 чисел. Строка уходит каждую итерацию главного цикла (main.c), передача блокирующая —
~25–30 мс на строку при 115200, отсюда ~35–58 кадр/с. Команды прошивка сейчас не принимает.

Транспорт — SerialLink / BleLink из sensor_link.py (они отдают сырые байты);
TextSensors — транспорт + разбор, для испытаний (force_ramp, slip_grip).
"""
import math
import threading
import time

NUM_CELLS, NUM_MLX, AXES = 3, 4, 3
CELLS = ['THUMB', 'INDEX', 'MIDDLE']
AXIS_NAMES = ['X', 'Y', 'Z']
MAG_COLS = [f'{c}_MLX{m}_{a}' for c in CELLS for m in range(NUM_MLX) for a in AXIS_NAMES]
N_FIELDS = 6 + 1 + 2 + NUM_CELLS * NUM_MLX * AXES     # 45


BAUDS = (460800, 921600, 230400, 115200)   # порядок перебора: сначала целевая скорость прошивки


def detect_baud(port, bauds=BAUDS, listen_s=0.6):
    """Скорость, на которой с порта идут правильные строки (>= 2 разобранных подряд), или None.
    Только читает. На чужой скорости приходит мусор — такие строки не разбираются."""
    import serial
    for baud in bauds:
        with serial.Serial(port, baud, timeout=0.05) as ser:
            ser.reset_input_buffer()
            parser = TextFrameParser()
            end = time.time() + listen_s
            while time.time() < end:
                parser.feed(ser.read(512))
                if parser.good_lines >= 2:
                    return baud
    return None


class TextFrameParser:
    """Потоковый разбор строк. Строки с неверным числом полей или нечисловыми значениями
    отбрасываются и считаются в bad_lines (например, обрезанная первая строка после подключения)."""

    MAX_LINE = 1024

    def __init__(self):
        self.buf = bytearray()
        self.bad_lines = 0
        self.good_lines = 0

    def feed(self, data: bytes):
        self.buf += data
        frames = []
        while True:
            i = self.buf.find(b'\n')
            if i < 0:
                if len(self.buf) > self.MAX_LINE:      # мусор без перевода строки
                    self.buf.clear()
                    self.bad_lines += 1
                break
            line = bytes(self.buf[:i]).strip()
            del self.buf[:i + 1]
            if not line:
                continue
            f = self.parse_line(line)
            if f is None:
                self.bad_lines += 1
            else:
                self.good_lines += 1
                frames.append(f)
        return frames

    @staticmethod
    def parse_line(line: bytes):
        parts = line.split()
        if len(parts) != N_FIELDS:
            return None
        try:
            adc = [int(x) for x in parts[0:6]]
            hall = int(parts[6])
            hall_filt = float(parts[7])
            weight = float(parts[8])
            mag = [int(x) for x in parts[9:]]
        except ValueError:
            return None
        return {'adc': adc, 'hall': hall, 'hall_filt': hall_filt, 'weight': weight, 'mag': mag}


class MockTextLink:
    """Имитация платы: шлёт строки FormatRawFrame ~40 Гц (для проверки без железа)."""
    budget_bps = 8000

    def __init__(self, on_data):
        self.on_data = on_data
        self.alive = True
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        t0 = time.time()
        while self.alive:
            t = time.time() - t0
            adc = [int(2000 + 1000 * math.sin(t + i)) for i in range(6)]
            mag = [int(800 * math.sin(0.7 * t + k * 0.3)) for k in range(NUM_CELLS * NUM_MLX * AXES)]
            line = (' '.join(map(str, adc)) + f' 1234 {12.3456 + t:.4f} {max(0, 50 * math.sin(0.4 * t)):.4f} '
                    + ' '.join(map(str, mag)) + ' \r\n')
            self.on_data(line.encode())
            time.sleep(0.025)

    def write(self, data):
        pass

    def close(self):
        self.alive = False


class TextSensors:
    """Транспорт (link_factory(on_data) -> SerialLink/BleLink/Mock) + разбор строк.
    on_frame(t_abs, frame) — на каждый кадр, из потока транспорта."""

    def __init__(self, link_factory, on_frame):
        self.on_frame = on_frame
        self.parser = TextFrameParser()
        self.lock = threading.Lock()
        self.n = 0
        self.last_t = 0.0
        self.link = link_factory(self._on_data)

    def _on_data(self, data):
        with self.lock:
            frames = self.parser.feed(data)
        now = time.time()
        for f in frames:
            self.n += 1
            self.last_t = now
            self.on_frame(now, f)

    def wait_frames(self, seconds, min_frames=3):
        n0, end = self.n, time.time() + seconds
        while time.time() < end:
            if self.n - n0 >= min_frames:
                return True
            time.sleep(0.05)
        return self.n - n0 >= min_frames

    def close(self):
        self.link.close()
