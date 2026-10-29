"""
Связь с кистью Indy по UART (порт датчика ЭМГ, 115200).

  HandUart       — связь «я датчик ЭМГ» + отправка команд (испытания: force_ramp, slip_grip)
  NoHand         — заглушка без протеза (--no-hand)
  HandController — регулятор положения по времени работы мотора (hand_gui, combined_gui);
                   с рабочей калибровкой силы (force_model.py) force — линейный %:
                   доля максимальной реальной силы, в команду уходит пересчитанный сырой %

Протокол (старый Motorica): [01][CMD][LEN][DATA][CRC8]
  плата -> 81 80 01 00 F0  опрос версии датчика; отвечаем 01 80 01 06 (без этого команды не принимаются)
  0x10 открыть, 0x0F закрыть, 0x11 <pct> закрыть с силой pct, 0x13 стоп
  (направления на этой плате инвертированы относительно документации)

Положение кисти не измеряется — оценивается по времени работы мотора;
в крайних положениях (цель 0 / 100) делается дожим до упора.
"""
import collections
import threading
import time

import serial

# CRC8 из MotoricaInterface/crc.py (тот же полином/таблица)
CRC_TABLE = [
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53,
]


def crc8(data):
    r = 0
    for b in data:
        r = CRC_TABLE[r ^ b]
    return r


FULL_STROKE_S = 0.75             # полный ход, с работы мотора (измерено)
RATE = 100.0 / FULL_STROKE_S     # % хода в секунду
WINDOW = 0.12                    # окно программного ШИМ, с (подбирается experiments/pwm_test.py)
DEADBAND = 1.5                   # %, ближе к цели — не двигаемся
START_LOSS = 0.015               # с, теряется на разгон при каждом старте мотора
PUSH_S = 0.6                     # с, дожим до упора в крайних положениях


def pack(cmd, data=()):
    p = [0x01, cmd, len(data), *data]
    return bytes(p + [crc8(p)])


CMD_OPEN = pack(0x10)
CMD_STOP = pack(0x13)


def cmd_close(force_pct):
    return pack(0x11, [max(1, min(100, int(force_pct)))])


def pulse_close(send, force_pct, duty, window_s, seconds, stop_when=None):
    """Медленное сжатие программным ШИМ: в каждом окне window_s мотор «закрыть с силой» на долю duty,
    остальное время стоп. Моменты переключения считаются от абсолютного времени — ошибки sleep не
    копятся. stop_when() -> True прерывает раньше: проверяется ~каждую 1 мс, и во время импульса
    тоже — стоп уходит сразу. Возвращает прошедшее время, с; мотор остановлен.
    Чем короче окно, тем плавнее, но импульс короче START_LOSS мотор почти не отрабатывает."""
    on, close = window_s * duty, cmd_close(force_pct)
    t0 = time.perf_counter()
    k = 0
    while True:
        start = t0 + k * window_s
        if start - t0 >= seconds or not _sleep_until(start, stop_when):
            break
        send(close)
        stopped = not _sleep_until(start + on, stop_when)
        send(CMD_STOP)
        if stopped:
            break
        k += 1
    send(CMD_STOP)
    return time.perf_counter() - t0


def _sleep_until(t, stop_when=None):
    """Сон до момента perf_counter() = t; последние ~2 мс — активное ожидание (точность ~0.1 мс).
    С stop_when — короткими шагами ~1 мс; False, если stop_when() сработал раньше."""
    while True:
        if stop_when and stop_when():
            return False
        dt = t - time.perf_counter()
        if dt <= 0:
            return True
        if dt > 0.003:
            time.sleep(min(dt - 0.002, 0.001) if stop_when else dt - 0.002)


class HandUart:
    """Связь «я датчик ЭМГ»: ответ на опрос версии и фоновый поток ЭМГ A=B=0 (~35 Гц),
    как у настоящего датчика; send() — команды привода. linked — Event «плата нас увидела».
    emg_stream = False — не слать поток ЭМГ (только ответы на опрос); rx — что присылала плата,
    кроме опросов версии: deque (время, байты)."""

    POLL = b'\x81\x80\x01\x00\xf0'

    def __init__(self, port):
        self.ser = serial.Serial(port, 115200, timeout=0)
        self.ser.reset_input_buffer()
        self.lock = threading.Lock()
        self.linked = threading.Event()
        self.run = True
        self.emg_stream = True
        self.rx = collections.deque(maxlen=200)
        threading.Thread(target=self._worker, daemon=True).start()

    def _worker(self):
        buf = bytearray()
        nxt = time.time()
        while self.run:
            with self.lock:
                got = self.ser.read(64)
                buf += got
                other = got.replace(self.POLL, b'')
                if other:
                    self.rx.append((time.time(), other))
                i = buf.find(b'\x81\x80\x01\x00')
                if i >= 0:
                    self.ser.write(pack(0x80, [6]))
                    del buf[:i + 5]
                    self.linked.set()
                elif len(buf) > 64:
                    del buf[:-8]
                if time.time() >= nxt:
                    if self.emg_stream:
                        self.ser.write(pack(0x20, [0, 0]))
                    nxt += 0.028
                    if nxt < time.time() - 0.5:
                        nxt = time.time()
            time.sleep(0.002)

    def send(self, raw):
        with self.lock:
            self.ser.write(raw)

    def close(self):
        try:
            self.send(CMD_STOP)
            time.sleep(0.05)
        finally:
            self.run = False
            time.sleep(0.05)
            self.ser.close()


class NoHand:
    linked = threading.Event()

    def __init__(self):
        self.linked.set()

    def send(self, raw):
        pass

    def close(self):
        pass


class HandController:
    def __init__(self, port, on_cmd=None, force_model='auto'):
        """on_cmd(t_abs, raw_bytes, note) — вызывается на каждую отправленную команду.
        force_model: 'auto' — рабочая калибровка, если есть; None — сырой %; или ForceModel."""
        if force_model == 'auto':
            from force_model import ForceModel
            force_model = ForceModel.load_default()
        self.force_model = force_model
        self.on_cmd = on_cmd
        self.force = 60
        self.target = 0.0
        self.speed = 40
        self.pos = 0.0
        self.moving = 0           # +1 сжимает, -1 разжимает, 0 стоит
        self._run = True
        self._calib = False
        self._stop_all = False
        self._push_req = False
        self._pushed = None
        self.on_target_reset = None   # колбэк для GUI, когда регулятор сам меняет цель
        self.window = WINDOW          # окно программного ШИМ, с
        self.link = HandUart(port)
        threading.Thread(target=self._control_worker, daemon=True).start()

    # ---- внешние команды
    def calibrate(self):
        self._calib = True

    def push(self):
        self._push_req = True

    def stop_all(self):
        self._stop_all = True

    def state(self):
        return {'hand_linked': int(self.linked), 'hand_force': self.force, 'hand_force_raw': self.raw_force(),
                'hand_target': round(self.target, 1), 'hand_speed': self.speed,
                'hand_pos_est': round(self.pos, 1), 'hand_moving': self.moving}

    @property
    def linked(self):
        return self.link.linked.is_set()

    def close(self):
        self._run = False
        time.sleep(0.15)
        self.link.close()             # шлёт стоп

    # ---- внутреннее
    def _send(self, raw, note):
        self.link.send(raw)
        if self.on_cmd:
            self.on_cmd(time.time(), raw, note)

    def _stop_motor(self, note='стоп'):
        if self.moving != 0:
            self._send(CMD_STOP, note)
            self.moving = 0

    def raw_force(self):
        """Сырой % для команды: по калибровке, если она есть, иначе force как есть."""
        return self.force_model.raw_pct(self.force) if self.force_model else int(self.force)

    def _start_motor(self, direction):
        if direction > 0:
            raw = self.raw_force()
            note = f'закрыть, сила {self.force}%' + (f' (сырой {raw}%)' if self.force_model else '')
            self._send(cmd_close(raw), note)
        else:
            self._send(CMD_OPEN, 'открыть')
        self.moving = direction

    def _set_target(self, v):
        self.target = v
        if self.on_target_reset:
            self.on_target_reset(v)

    def _control_worker(self):
        while self._run:
            if not self.linked:
                time.sleep(0.05)
                continue

            if self._stop_all:
                self._stop_all = False
                self.moving = 1
                self._stop_motor('СТОП (кнопка)')
                self._set_target(self.pos)

            if self._calib:
                self._calib = False
                self._stop_motor()
                self._start_motor(-1)
                time.sleep(FULL_STROKE_S * 1.6)
                self._stop_motor('стоп после калибровки')
                self.pos = 0.0
                self._pushed = 0
                self._set_target(0.0)
                continue

            end = 100 if self.target >= 99 else (0 if self.target <= 1 else None)
            if end is None:
                self._pushed = None
            if self._push_req or (end is not None and self._pushed != end
                                  and abs(self.pos - end) < DEADBAND):
                direction = 1 if (self._push_req or end == 100) else -1
                self._push_req = False
                self._stop_motor()
                self._start_motor(direction)
                time.sleep(PUSH_S)
                self._stop_motor('стоп после дожима до упора')
                self.pos = 100.0 if direction > 0 else 0.0
                self._pushed = 100 if direction > 0 else 0
                # цель = достигнутый упор, иначе регулятор сразу «откатит» кисть
                # назад к старому положению ползунка
                if self.target != self.pos:
                    self._set_target(self.pos)
                continue

            err = self.target - self.pos
            if abs(err) < DEADBAND:
                self._stop_motor()
                time.sleep(0.01)
                continue

            direction = 1 if err > 0 else -1
            duty = max(0.05, self.speed / 100.0)
            started = self.moving != direction
            loss = START_LOSS if started else 0.0
            on_time = min(self.window * duty, abs(err) / RATE + loss)
            if started:
                self._stop_motor()
                self._start_motor(direction)
            t_start = time.time()
            time.sleep(on_time)
            moved = max(0.0, time.time() - t_start - loss)
            self.pos = max(0.0, min(100.0, self.pos + direction * RATE * moved))

            continuous = duty >= 0.999 and abs(self.target - self.pos) > DEADBAND
            if not continuous:
                self._stop_motor()
                time.sleep(max(0.0, self.window - on_time))
