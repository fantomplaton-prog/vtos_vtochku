"""
Тест движения Indy по старому BLE-протоколу (fe48 = open, fe49 = close).
Пишет телеметрию 2a37 с отметками команд в CSV и печатает только изменившиеся пакеты.

  py -3 indy_motion_test.py [MAC] [длительность_импульса_с]
"""
import asyncio
import csv
import os
import sys
import time

from bleak import BleakClient

ADDR = sys.argv[1] if len(sys.argv) > 1 else '80:E1:26:09:2E:0B'
PULSE_S = float(sys.argv[2]) if len(sys.argv) > 2 else 1.5

OPEN = '0000fe48-8e22-4541-9d4c-21edae82ed19'
CLOSE = '0000fe49-8e22-4541-9d4c-21edae82ed19'
TELEMETRY = '00002a37-0000-1000-8000-00805f9b34fb'
GO = bytes([0x01, 0x00])
NOGO = bytes([0x00, 0x00])

t0 = time.time()
rows = []
last = None
event = 'baseline'


def on_telemetry(_, data: bytearray):
    global last
    t = time.time() - t0
    rows.append([f'{t:.3f}', event, data.hex(' ')])
    if data != last:
        diff = '' if last is None else ' changed@' + ','.join(
            str(i) for i in range(min(len(data), len(last))) if data[i] != last[i])
        print(f'{t:7.3f} [{event:>10}] {data.hex(" ").upper()}{diff}')
        last = bytes(data)


async def stop(client):
    await client.write_gatt_char(CLOSE, NOGO, response=True)
    await client.write_gatt_char(OPEN, NOGO, response=True)


async def pulse(client, char, name):
    global event
    event = name
    print(f'--- {name} ---')
    await client.write_gatt_char(char, GO, response=True)
    await asyncio.sleep(PULSE_S)
    event = name + '_stop'
    await stop(client)
    await asyncio.sleep(2.0)


async def main():
    global event
    async with BleakClient(ADDR, timeout=20) as client:
        print('connected', ADDR)
        await client.start_notify(TELEMETRY, on_telemetry)
        try:
            await asyncio.sleep(2.0)
            await pulse(client, OPEN, 'open')
            await pulse(client, CLOSE, 'close')
            await pulse(client, OPEN, 'open2')
        finally:
            await stop(client)
            event = 'end'
            await asyncio.sleep(0.5)
            await client.stop_notify(TELEMETRY)

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       f'motion_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    with open(out, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['t', 'event', 'hex'])
        w.writerows(rows)
    print(f'\n{len(rows)} пакетов, лог: {out}')


asyncio.run(main())
