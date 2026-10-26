"""
Ручная имитация датчика 2EMG в реальном времени.

- Два ползунка (канал A, канал B, 0..255) -> пакеты 01 20 02 A B CRC на UART ~35 Гц.
- Ответ на опрос платы (81 80 01 00) версией 6.
- Опционально: пакет состояния 01 83 01 <state> (0 нейтрально, 1, 2) и float-пакет 01 22.
- BLE-телеметрия платы (2a37) в реальном времени, изменившиеся байты подсвечены.
- Кнопка СТОП: шлёт стоп мотору по BLE (fe48/fe49 <- 00 00).
Лог: tools/emg_gui_<время>.csv

  py -3 emg_gui.py [COM]          (по умолчанию COM20; INDY_MAC — адрес BLE)
"""
import asyncio
import csv
import os
import queue
import struct
import sys
import threading
import time
import tkinter as tk

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emg_emulator import parse_stream, describe, pack

PORT = sys.argv[1] if len(sys.argv) > 1 else 'COM20'
BLE_ADDR = os.environ.get('INDY_MAC', '80:E1:26:09:2E:0B')
TELEMETRY = '00002a37-0000-1000-8000-00805f9b34fb'
UUID = lambda h: f'0000{h}-8e22-4541-9d4c-21edae82ed19'
PERIOD = 0.028

state = {'a': 0, 'b': 0, 'send83': False, 'state83': 0, 'send22': False,
         'running': True, 'sent': 0, 'stop_req': False}
ui_q = queue.Queue()
t0 = time.time()
log = []
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   f'emg_gui_{time.strftime("%Y%m%d_%H%M%S")}.csv')


def ts():
    return f'{time.time() - t0:.3f}'


# ---------------------------------------------------------------- UART
def serial_worker():
    try:
        s = serial.Serial(PORT, 115200, timeout=0)
    except Exception as e:
        ui_q.put(('uart', f'НЕ ОТКРЫТ {PORT}: {e}'))
        return
    s.reset_input_buffer()
    buf = bytearray()
    next_tx = time.time()
    n83 = 0
    ui_q.put(('uart', f'{PORT} открыт, шлю поток ЭМГ'))
    while state['running']:
        buf += s.read(256)
        packets, buf = parse_stream(buf)
        for p in packets:
            if p[:2] == b'\x81\x80':
                s.write(pack(0x01, 0x80, [6]))
            else:
                ui_q.put(('uart', f'{ts()}s <- {describe(p)}'))
                log.append((ts(), 'uart_rx', p.hex(' '), state['a'], state['b']))
        if time.time() >= next_tx:
            a, b = state['a'], state['b']
            s.write(pack(0x01, 0x20, [a, b]))
            if state['send22']:
                s.write(pack(0x01, 0x22, list(struct.pack('<ff', float(a), float(b)))))
            n83 += 1
            if state['send83'] and n83 % 3 == 0:   # ~12 Гц
                s.write(pack(0x01, 0x83, [state['state83']]))
            state['sent'] += 1
            next_tx += PERIOD
            if next_tx < time.time() - 0.5:
                next_tx = time.time()
        time.sleep(0.002)
    s.close()


# ---------------------------------------------------------------- BLE
def ble_thread():
    asyncio.run(ble_main())


async def ble_main():
    from bleak import BleakClient, BleakScanner
    while state['running']:
        try:
            ui_q.put(('ble_status', 'BLE: поиск...'))
            dev = await BleakScanner.find_device_by_address(BLE_ADDR, timeout=10)
            if dev is None:
                ui_q.put(('ble_status', 'BLE: нет в эфире, повтор'))
                continue
            async with BleakClient(dev, timeout=15) as c:
                def on_notify(_, data):
                    ui_q.put(('tele', bytes(data)))
                    log.append((ts(), 'ble', data.hex(' '), state['a'], state['b']))
                await c.start_notify(TELEMETRY, on_notify)
                ui_q.put(('ble_status', 'BLE: подключён'))
                while state['running'] and c.is_connected:
                    if state['stop_req']:
                        state['stop_req'] = False
                        await c.write_gatt_char(UUID('fe48'), b'\x00\x00', response=True)
                        await c.write_gatt_char(UUID('fe49'), b'\x00\x00', response=True)
                        ui_q.put(('uart', f'{ts()}s СТОП мотора отправлен по BLE'))
                    await asyncio.sleep(0.05)
                ui_q.put(('ble_status', 'BLE: отключён, переподключаюсь'))
        except Exception as e:
            ui_q.put(('ble_status', f'BLE: {type(e).__name__}, повтор'))
            await asyncio.sleep(1)


# ---------------------------------------------------------------- GUI
root = tk.Tk()
root.title(f'Имитация 2EMG -> плата ({PORT})')
root.geometry('900x640+60+60')
F = ('Segoe UI', 14)
FB = ('Segoe UI', 16, 'bold')

top = tk.Frame(root)
top.pack(fill='x', padx=10, pady=6)

sliders = {}
for ch, color in (('a', '#d9534f'), ('b', '#0275d8')):
    fr = tk.Frame(top)
    fr.pack(side='left', expand=True, fill='both', padx=10)
    tk.Label(fr, text=f'Канал {ch.upper()}', font=FB, fg=color).pack()
    sc = tk.Scale(fr, from_=255, to=0, orient='vertical', length=320, width=40,
                  font=F, troughcolor=color,
                  command=lambda v, ch=ch: state.__setitem__(ch, int(float(v))))
    sc.pack()
    sliders[ch] = sc

right = tk.Frame(top)
right.pack(side='left', fill='y', padx=10)


def reset():
    for sc in sliders.values():
        sc.set(0)


def stop_motor():
    reset()
    state['stop_req'] = True


tk.Button(right, text='Сброс в 0', font=FB, command=reset, width=16).pack(pady=4)
tk.Button(right, text='СТОП мотора', font=FB, bg='tomato', command=stop_motor, width=16).pack(pady=4)

v83 = tk.BooleanVar(value=False)
v22 = tk.BooleanVar(value=False)
vst = tk.IntVar(value=0)
tk.Checkbutton(right, text='слать 01 83 (состояние)', font=F, variable=v83,
               command=lambda: state.__setitem__('send83', v83.get())).pack(anchor='w', pady=(14, 0))
for val, name in ((0, '0 нейтрально'), (1, '1'), (2, '2')):
    tk.Radiobutton(right, text=name, font=F, variable=vst, value=val,
                   command=lambda: state.__setitem__('state83', vst.get())).pack(anchor='w', padx=24)
tk.Checkbutton(right, text='слать 01 22 (float A,B)', font=F, variable=v22,
               command=lambda: state.__setitem__('send22', v22.get())).pack(anchor='w', pady=(10, 0))

info = tk.Label(right, text='', font=F, justify='left')
info.pack(anchor='w', pady=10)
ble_lbl = tk.Label(right, text='BLE: ...', font=F)
ble_lbl.pack(anchor='w')

tk.Label(root, text='Телеметрия платы (BLE 2a37), жёлтым — изменившиеся байты:', font=F).pack(anchor='w', padx=10)
tele_txt = tk.Text(root, height=2, font=('Consolas', 16), width=60)
tele_txt.pack(fill='x', padx=10)
tele_txt.tag_config('chg', background='yellow')
tk.Label(root, text='Индексы:  ' + ' '.join(f'{i:02d}' for i in range(18)),
         font=('Consolas', 12), fg='gray').pack(anchor='w', padx=10)

tk.Label(root, text='UART от платы (кроме опроса версии) и события:', font=F).pack(anchor='w', padx=10)
ev_txt = tk.Text(root, height=7, font=('Consolas', 11))
ev_txt.pack(fill='both', expand=True, padx=10, pady=(0, 10))

prev_tele = [None]


def ui_tick():
    while not ui_q.empty():
        kind, payload = ui_q.get()
        if kind == 'tele':
            tele_txt.delete('1.0', 'end')
            for i, byte in enumerate(payload):
                changed = prev_tele[0] is not None and i < len(prev_tele[0]) and prev_tele[0][i] != byte
                tele_txt.insert('end', f'{byte:02X} ', 'chg' if changed else '')
            tele_txt.insert('end', f'\nA={payload[1]:3d} B={payload[2]:3d}  b3={payload[3]} b4={payload[4]} '
                                   f'b0={payload[0]} b17={payload[17] if len(payload) > 17 else "-"}')
            if prev_tele[0] is not None and payload != prev_tele[0]:
                diff = [i for i in range(min(len(payload), len(prev_tele[0])))
                        if payload[i] != prev_tele[0][i] and i not in (1, 2)]
                if diff:
                    ev_txt.insert('end', f'{ts()}s телеметрия: изменились байты {diff}: '
                                         f'{payload.hex(" ").upper()}  (A={state["a"]} B={state["b"]})\n')
                    ev_txt.see('end')
            prev_tele[0] = payload
        elif kind == 'ble_status':
            ble_lbl.config(text=payload)
        else:
            ev_txt.insert('end', payload + '\n')
            ev_txt.see('end')
    info.config(text=f'A = {state["a"]}\nB = {state["b"]}\nотправлено: {state["sent"]}')
    root.after(50, ui_tick)


def on_close():
    state['running'] = False
    with open(OUT, 'w', newline='', encoding='utf-8-sig') as f:
        w = csv.writer(f)
        w.writerow(['t', 'kind', 'data', 'A', 'B'])
        w.writerows(log)
    root.after(300, root.destroy)


root.protocol('WM_DELETE_WINDOW', on_close)
threading.Thread(target=serial_worker, daemon=True).start()
threading.Thread(target=ble_thread, daemon=True).start()
ui_tick()
root.mainloop()
print(f'лог: {OUT}')
