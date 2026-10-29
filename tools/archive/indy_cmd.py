"""
Пошаговые команды для Indy (старый BLE-протокол).

  py -3 indy_cmd.py open <сек>                     — открывать N секунд, затем стоп
  py -3 indy_cmd.py close <сек>                    — закрывать N секунд, затем стоп
  py -3 indy_cmd.py pulses <мс> <кол-во> [пауза_мс] — серия коротких импульсов закрытия
  py -3 indy_cmd.py stop
  py -3 indy_cmd.py fe42 <0..255>                  — записать SHUTDOWN_CURRENT (1 байт)
      Шкала ПРЯМАЯ под нагрузкой: больше значение = сильнее хват
      (плотный предмет: 20→0 мм, 40→1–2, 60→2–3, 80→3, 100→3+ мм).
      Но при 120–150 свободный ход обрывается через доли секунды (вероятно,
      остановка по малому току < fe42/4); при 40 — полный ход одной командой.

MAC берётся из переменной окружения INDY_MAC (по умолчанию 80:E1:26:09:2E:0B).
"""
import asyncio
import os
import sys
import time

from bleak import BleakClient, BleakScanner

ADDR = os.environ.get('INDY_MAC', '80:E1:26:09:2E:0B')

UUID = lambda h: f'0000{h}-8e22-4541-9d4c-21edae82ed19'
# На этой плате направления инвертированы относительно документации:
# fe48 (OPEN_MOTOR_HDLE) фактически закрывает, fe49 (CLOSE_MOTOR_HDLE) открывает.
CLOSE, OPEN, SHUTDOWN_CURRENT = UUID('fe48'), UUID('fe49'), UUID('fe42')
GO, NOGO = bytes([0x01, 0x00]), bytes([0x00, 0x00])


# Запись с подтверждением (response=True) на слабом линке ждёт до 1–2 с,
# из-за чего импульс растягивается. Для движения пишем без подтверждения.
FAST = os.environ.get('INDY_FAST', '1') == '1'


async def stop(c):
    await c.write_gatt_char(CLOSE, NOGO, response=not FAST)
    await c.write_gatt_char(OPEN, NOGO, response=not FAST)


async def run(c, char, seconds):
    t = time.perf_counter()
    await c.write_gatt_char(char, GO, response=not FAST)
    t_go = time.perf_counter() - t
    await asyncio.sleep(seconds)
    await stop(c)
    print(f'движение {time.perf_counter() - t:.3f} c (отправка GO {t_go*1000:.0f} мс)')


async def main(argv):
    if len(argv) < 1:
        print(__doc__)
        return
    cmd = argv[0]
    for attempt in range(1, 4):
        try:
            # На Windows подключение по голому MAC часто даёт DeviceNotFound —
            # сначала находим устройство сканированием и подключаемся к BLEDevice
            dev = await BleakScanner.find_device_by_address(ADDR, timeout=10)
            if dev is None:
                raise RuntimeError('не найдено в эфире')
            c = BleakClient(dev, timeout=15)
            await c.connect()
            break
        except Exception as e:
            print(f'подключение, попытка {attempt}: {type(e).__name__}')
            await asyncio.sleep(1)
    else:
        print('не удалось подключиться — команда НЕ отправлена')
        return
    async with c:
        try:
            if cmd == 'open':
                await run(c, OPEN, float(argv[1]))
            elif cmd == 'close':
                await run(c, CLOSE, float(argv[1]))
            elif cmd == 'pulses':
                ms, n = float(argv[1]), int(argv[2])
                gap = float(argv[3]) if len(argv) > 3 else 400
                for i in range(n):
                    await run(c, CLOSE, ms / 1000)
                    await asyncio.sleep(gap / 1000)
            elif cmd == 'stop':
                pass
            elif cmd == 'fe42':
                v = int(argv[1])
                assert 0 <= v <= 255
                await c.write_gatt_char(SHUTDOWN_CURRENT, bytes([v]), response=True)
                print(f'fe42 <- {v}')
            else:
                print(__doc__)
        finally:
            # финальный стоп — с подтверждением, чтобы он гарантированно ушёл до отключения
            await c.write_gatt_char(CLOSE, NOGO, response=True)
            await c.write_gatt_char(OPEN, NOGO, response=True)
    print('ok, стоп отправлен')


asyncio.run(main(sys.argv[1:]))
