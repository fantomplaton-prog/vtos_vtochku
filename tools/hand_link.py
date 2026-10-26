"""
Управление кистью Indy по UART (порт датчика ЭМГ, 115200) — класс для встраивания
в другие окна (combined_gui.py). Логика та же, что в hand_gui.py.

Протокол (старый Motorica): [01][CMD][LEN][DATA][CRC8]
  плата -> 81 80 01 00 F0  опрос версии датчика; отвечаем 01 80 01 06 (без этого команды не принимаются)
  0x10 открыть, 0x0F закрыть, 0x11 <pct> закрыть с силой pct, 0x13 стоп
  (направления на этой плате инвертированы относительно документации)

Положение кисти не измеряется — оценивается по времени работы мотора;
в крайних положениях (цель 0 / 100) делается дожим до упора.
"""
import threading
import time

import serial

from probe_board import crc8

FULL_STROKE_S = 0.75             # полный ход, с работы мотора (измерено)
RATE = 100.0 / FULL_STROKE_S     # % хода в секунду
WINDOW = 0.12                    # окно программного ШИМ, с
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


class HandController:
    def __init__(self, port, on_cmd=None):
        """on_cmd(t_abs, raw_bytes, note) — вызывается на каждую отправленную команду."""
        self.on_cmd = on_cmd
        self.force = 60
        self.target = 0.0
        self.speed = 40
        self.pos = 0.0
        self.moving = 0           # +1 сжимает, -1 разжимает, 0 стоит
        self.linked = False
        self._run = True
        self._calib = False
        self._stop_all = False
        self._push_req = False
        self._pushed = None
        self.on_target_reset = None   # колбэк для GUI, когда регулятор сам меняет цель
        self.lock = threading.Lock()
        self.ser = serial.Serial(port, 115200, timeout=0)
        self.ser.reset_input_buffer()
        threading.Thread(target=self._link_worker, daemon=True).start()
        threading.Thread(target=self._control_worker, daemon=True).start()

    # ---- внешние команды
    def calibrate(self):
        self._calib = True

    def push(self):
        self._push_req = True

    def stop_all(self):
        self._stop_all = True

    def state(self):
        return {'hand_linked': int(self.linked), 'hand_force': self.force,
                'hand_target': round(self.target, 1), 'hand_speed': self.speed,
                'hand_pos_est': round(self.pos, 1), 'hand_moving': self.moving}

    def close(self):
        self._run = False
        time.sleep(0.15)
        try:
            with self.lock:
                self.ser.write(CMD_STOP)
        except Exception:
            pass
        self.ser.close()

    # ---- внутреннее
    def _send(self, raw, note):
        with self.lock:
            self.ser.write(raw)
        if self.on_cmd:
            self.on_cmd(time.time(), raw, note)

    def _link_worker(self):
        buf = bytearray()
        nxt = time.time()
        while self._run:
            with self.lock:
                buf += self.ser.read(64)
                i = buf.find(b'\x81\x80\x01\x00')
                if i >= 0:
                    self.ser.write(pack(0x80, [6]))
                    del buf[:i + 5]
                    self.linked = True
                elif len(buf) > 64:
                    del buf[:-8]
                if time.time() >= nxt:
                    self.ser.write(pack(0x20, [0, 0]))
                    nxt += 0.028
                    if nxt < time.time() - 0.5:
                        nxt = time.time()
            time.sleep(0.002)

    def _stop_motor(self, note='стоп'):
        if self.moving != 0:
            self._send(CMD_STOP, note)
            self.moving = 0

    def _start_motor(self, direction):
        if direction > 0:
            self._send(cmd_close(self.force), f'закрыть, сила {self.force}%')
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
            on_time = min(WINDOW * duty, abs(err) / RATE + loss)
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
                time.sleep(max(0.0, WINDOW - on_time))
