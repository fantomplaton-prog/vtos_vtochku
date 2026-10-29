"""
Имитация датчика 2EMG на UART-шине платы + одновременное чтение BLE-телеметрии.

Датчик (по примерам Examples/EMG): сам шлёт 01 20 02 <A> <B> CRC ~35 Гц,
на опрос платы 81 80 01 00 отвечает версией 01 80 01 06.

Профиль задаётся строкой "A,B,секунд;A,B,секунд;..."
  py -3 emg_stream.py "5,7,10"                 — 10 с с A=5, B=7
  py -3 emg_stream.py "0,0,3;40,0,2;80,0,2"    — ступени по каналу A
Опции окружения: EMG_PORT (COM20), EMG_BLE=0 — не подключаться к BLE.
Лог: tools/emg_stream_<время>.csv
"""
import asyncio
import csv
import os
import sys
import threading
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emg_emulator import parse_stream, describe, pack

PORT = os.environ.get('EMG_PORT', 'COM20')
USE_BLE = os.environ.get('EMG_BLE', '1') == '1'
BLE_ADDR = os.environ.get('INDY_MAC', '80:E1:26:09:2E:0B')
TELEMETRY = '00002a37-0000-1000-8000-00805f9b34fb'
PERIOD = 0.028  # ~35 Гц, как у настоящего датчика

PROFILE = []
for step in (sys.argv[1] if len(sys.argv) > 1 else '5,7,10').split(';'):
    a, b, sec = step.split(',')
    PROFILE.append((int(a), int(b), float(sec)))

t0 = None
log = []
state = {'a': 0, 'b': 0, 'step': 0, 'done': False}


def stamp():
    return time.time() - t0


def serial_worker():
    s = serial.Serial(PORT, 115200, timeout=0)
    s.reset_input_buffer()
    buf = bytearray()
    next_tx = time.time()
    sent = 0
    while not state['done']:
        buf += s.read(256)
        packets, buf = parse_stream(buf)
        for p in packets:
            if p[:2] == b'\x81\x80':
                s.write(pack(0x01, 0x80, [6]))
                log.append((f'{stamp():.3f}', 'uart_rx', 'version_req -> ответ 6'))
            else:
                print(f'{stamp():7.2f}s UART <- {describe(p)}', flush=True)
                log.append((f'{stamp():.3f}', 'uart_rx', p.hex(' ')))
        if time.time() >= next_tx:
            s.write(pack(0x01, 0x20, [state['a'], state['b']]))
            sent += 1
            next_tx += PERIOD
        time.sleep(0.002)
    s.close()
    print(f'{stamp():7.2f}s UART: отправлено {sent} пакетов ЭМГ', flush=True)


async def ble_worker():
    from bleak import BleakClient, BleakScanner
    last = [None]

    def on_notify(_, data):
        if data != last[0]:
            print(f'{stamp():7.2f}s BLE  телеметрия: {data.hex(" ").upper()}   '
                  f'(шлём A={state["a"]} B={state["b"]})', flush=True)
            last[0] = bytes(data)
        log.append((f'{stamp():.3f}', 'ble', data.hex(' ')))

    for attempt in range(3):
        try:
            dev = await BleakScanner.find_device_by_address(BLE_ADDR, timeout=10)
            if dev is None:
                raise RuntimeError('нет в эфире')
            async with BleakClient(dev, timeout=15) as c:
                await c.start_notify(TELEMETRY, on_notify)
                print(f'{stamp():7.2f}s BLE подключён, слушаю телеметрию', flush=True)
                while not state['done']:
                    await asyncio.sleep(0.1)
                return
        except Exception as e:
            print(f'{stamp():7.2f}s BLE попытка {attempt + 1}: {type(e).__name__} {e}', flush=True)
    print('BLE: не удалось — продолжаю без телеметрии', flush=True)


async def main():
    global t0
    t0 = time.time()
    ble_task = asyncio.create_task(ble_worker()) if USE_BLE else None
    if ble_task:
        await asyncio.sleep(6)  # даём BLE подключиться до начала профиля
    th = threading.Thread(target=serial_worker, daemon=True)
    th.start()
    for i, (a, b, sec) in enumerate(PROFILE, 1):
        state.update(a=a, b=b, step=i)
        print(f'{stamp():7.2f}s >>> ступень {i}/{len(PROFILE)}: A={a} B={b} на {sec:g} c', flush=True)
        log.append((f'{stamp():.3f}', 'step', f'A={a} B={b}'))
        await asyncio.sleep(sec)
    state.update(a=0, b=0)
    print(f'{stamp():7.2f}s >>> конец профиля, A=B=0 ещё 2 с', flush=True)
    await asyncio.sleep(2)
    state['done'] = True
    th.join(timeout=2)
    if ble_task:
        await ble_task
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       f'emg_stream_{time.strftime("%Y%m%d_%H%M%S")}.csv')
    with open(out, 'w', newline='', encoding='utf-8-sig') as f:
        w = csv.writer(f)
        w.writerow(['t', 'kind', 'data'])
        w.writerows(log)
    print(f'лог: {out}', flush=True)


asyncio.run(main())
