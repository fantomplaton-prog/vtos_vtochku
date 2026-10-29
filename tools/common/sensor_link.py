"""
Транспорт до платы датчиков TacMagVTOS: USB-UART (USART2) или BLE (HM-10).
Отдаёт сырые байты в on_data; разбор строк — common/sensor_text.py.
(Бинарный протокол 0xAA/0xAB/0xAC прошивка сейчас не шлёт — он в archive/sensor_binary.py.)
"""

import asyncio
import threading

HM10_CHAR = "0000ffe1-0000-1000-8000-00805f9b34fb"


class SerialLink:
    """USB-UART на линиях USART2 (вместо HM-10 или параллельно ему), 115200 бод."""

    budget_bps = 8000   # полезных байт/с, с запасом от 11.5 КБ/с

    def __init__(self, port, baud, on_data):
        """baud=None — автоопределение по текстовым строкам (sensor_text.detect_baud)."""
        import serial
        if baud is None:
            from sensor_text import detect_baud
            baud = detect_baud(port)
            if baud is None:
                raise RuntimeError(f'{port}: правильных строк нет ни на одной скорости — проверьте '
                                   f'питание платы, RX/TX и общую землю')
            print(f'Датчики: {port}, скорость {baud} (автоопределение)')
        self.baud = baud
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
