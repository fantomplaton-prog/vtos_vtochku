"""
Диагностика неизвестной платы протеза (STM32WB55 + SIM800C).
Скрипт ТОЛЬКО ЧИТАЕТ: не пишет в BLE-характеристики и не шлёт команд движения.

Режимы:
  py -3 probe_board.py ports                 — список COM-портов (ищем ST VID 0483)
  py -3 probe_board.py scan [секунд]         — BLE-скан, кандидаты помечены '>>'
  py -3 probe_board.py gatt <MAC> [секунд]   — дамп всех сервисов/характеристик,
                                               чтение readable, подписка на notify
  py -3 probe_board.py serial <COM> [baud]   — пассивное прослушивание + безопасные
                                               запросы версии (UBI4, UBI3, AT для SIM800)
Результаты gatt пишутся в gatt_<MAC>.json рядом со скриптом.
"""
import asyncio
import json
import os
import sys
import time

# --- CRC8 из MotoricaInterface/crc.py (тот же полином/таблица) ---
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


def hexs(b):
    return ' '.join(f'{x:02X}' for x in b)


def ascii_s(b):
    return ''.join(chr(x) if 32 <= x < 127 else '.' for x in b)


# Признаки, по которым узнаём «свою» плату в эфире
KNOWN_UUID_PARTS = ('4368', '8e22-4541', 'fe40', '0000ffe1', '6e400001')
ST_COMPANY_ID = 0x0030  # STMicroelectronics


# ---------------------------------------------------------------- ports
def cmd_ports():
    from serial.tools import list_ports
    for p in list_ports.comports():
        mark = '>>' if (p.vid == 0x0483 or 'BTHENUM' not in (p.hwid or '')) else '  '
        print(f'{mark} {p.device:6} vid={p.vid} pid={p.pid} {p.description} | {p.hwid}')
    print("\n'>>' — не Bluetooth-порт (вероятно USB/UART плата). VID 0483 = STMicroelectronics.")


# ---------------------------------------------------------------- BLE scan
async def cmd_scan(seconds=15):
    from bleak import BleakScanner
    found = await BleakScanner.discover(timeout=seconds, return_adv=True)
    rows = sorted(found.values(), key=lambda x: -x[1].rssi)
    for dev, adv in rows:
        uuids = [u.lower() for u in adv.service_uuids]
        candidate = (
            any(part in u for u in uuids for part in KNOWN_UUID_PARTS)
            or ST_COMPANY_ID in adv.manufacturer_data
            or (adv.local_name and any(k in adv.local_name.upper()
                                       for k in ('INDY', 'FEST', 'STM', 'WB', 'MOTORICA', 'HAND', 'PROT', 'P2P')))
        )
        mark = '>>' if candidate else '  '
        mfr = {f'0x{k:04X}': v.hex() for k, v in adv.manufacturer_data.items()}
        print(f'{mark} {dev.address} rssi={adv.rssi:4d} name={adv.local_name!r} uuids={uuids} mfr={mfr}')
    print('\nПодсказка: включите/выключите плату и сравните два скана — новое устройство и есть плата.')


# ---------------------------------------------------------------- BLE GATT dump
async def cmd_gatt(address, listen_s=15):
    from bleak import BleakClient
    report = {'address': address, 'services': []}
    notifications = []

    def on_notify(char, data):
        t = time.time()
        notifications.append({'t': t, 'char': char.uuid, 'hex': data.hex()})
        print(f'  [notify] {char.uuid} ({len(data)} B): {hexs(data)}  |{ascii_s(data)}|')

    async with BleakClient(address, timeout=20) as client:
        print(f'Подключено к {address}, MTU={client.mtu_size}')
        for svc in client.services:
            print(f'\nSERVICE {svc.uuid}  {svc.description}')
            s = {'uuid': svc.uuid, 'description': svc.description, 'chars': []}
            for ch in svc.characteristics:
                c = {'uuid': ch.uuid, 'handle': ch.handle, 'props': ch.properties,
                     'description': ch.description, 'value': None, 'descriptors': []}
                line = f'  CHAR {ch.uuid} h=0x{ch.handle:04X} {ch.properties} {ch.description}'
                if 'read' in ch.properties:
                    try:
                        v = await client.read_gatt_char(ch)
                        c['value'] = v.hex()
                        line += f'\n       = ({len(v)} B) {hexs(v)}  |{ascii_s(v)}|'
                    except Exception as e:
                        line += f'\n       read error: {e}'
                print(line)
                for d in ch.descriptors:
                    try:
                        dv = await client.read_gatt_descriptor(d.handle)
                        c['descriptors'].append({'uuid': d.uuid, 'value': dv.hex()})
                        print(f'       DESC {d.uuid} = {hexs(dv)} |{ascii_s(dv)}|')
                    except Exception:
                        c['descriptors'].append({'uuid': d.uuid, 'value': None})
                s['chars'].append(c)
            report['services'].append(s)

        notify_chars = [ch for svc in client.services for ch in svc.characteristics
                        if 'notify' in ch.properties or 'indicate' in ch.properties]
        if notify_chars:
            print(f'\nПодписка на {len(notify_chars)} notify-характеристик на {listen_s} c '
                  '(в это время можно напрягать датчики / нажимать кнопки на плате)...')
            for ch in notify_chars:
                try:
                    await client.start_notify(ch, on_notify)
                except Exception as e:
                    print(f'  не удалось подписаться на {ch.uuid}: {e}')
            await asyncio.sleep(listen_s)
            for ch in notify_chars:
                try:
                    await client.stop_notify(ch)
                except Exception:
                    pass

    report['notifications'] = notifications
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       f"gatt_{address.replace(':', '')}.json")
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(report, f, ensure_ascii=False, indent=2)
    print(f'\nОтчёт сохранён: {out}')


# ---------------------------------------------------------------- Serial probe
def ubi4_request(package_code, payload, address=0, waiting_answer=0, request_type=0):
    header_byte = (0 & 0x1F) | (waiting_answer << 5) | (request_type << 6)
    size = len(payload)
    return bytes([header_byte, package_code, 0, size & 0xFF, size >> 8,
                  crc8(payload), address]) + bytes(payload)


def ubi3_request(addr, cmd, data=()):
    p = [addr, cmd, len(data), *data]
    return bytes(p + [crc8(p)])


SERIAL_PROBES = [
    # (описание, байты) — только запросы информации
    ('UBI4 DEVICE_INFORMATION/INICIALIZE_INFORMATION', ubi4_request(0x01, [0x01, 0x02])),
    ('UBI4 PROTOCOL_PING', ubi4_request(0x0B, [0x00])),
    ('UBI3 GetVersion (адрес 0x01|0x80, cmd 0x80)', ubi3_request(0x81, 0x80)),
    ('SIM800 AT', b'AT\r\n'),
    ('SIM800 ATI (модель/прошивка)', b'ATI\r\n'),
]


def cmd_serial(port, baud=115200):
    import serial
    ser = serial.Serial(port, baudrate=baud, timeout=0.05)
    print(f'{port} @ {baud}: пассивное прослушивание 3 c...')
    t0 = time.time()
    buf = bytearray()
    while time.time() - t0 < 3:
        buf += ser.read(ser.in_waiting or 1)
    print(f'  принято {len(buf)} B: {hexs(buf[:256])}{" ..." if len(buf) > 256 else ""}')
    if buf:
        print(f'  ascii: |{ascii_s(buf[:256])}|')

    for name, pkt in SERIAL_PROBES:
        ser.reset_input_buffer()
        ser.write(pkt)
        time.sleep(0.4)
        ans = ser.read(ser.in_waiting or 1)
        print(f'\n-> {name}: {hexs(pkt)}')
        print(f'<- ({len(ans)} B) {hexs(ans[:200])}')
        if ans:
            print(f'   ascii: |{ascii_s(ans[:200])}|')
    ser.close()
    print('\nЕсли тишина — попробуйте другую скорость: 9600, 57600, 921600 '
          '(SIM800C по умолчанию автоопределяет скорость по "AT").')


# ---------------------------------------------------------------- main
def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    mode = sys.argv[1]
    if mode == 'ports':
        cmd_ports()
    elif mode == 'scan':
        asyncio.run(cmd_scan(int(sys.argv[2]) if len(sys.argv) > 2 else 15))
    elif mode == 'gatt':
        asyncio.run(cmd_gatt(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 15))
    elif mode == 'serial':
        cmd_serial(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 115200)
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
