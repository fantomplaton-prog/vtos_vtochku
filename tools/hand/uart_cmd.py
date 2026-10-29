"""
Прямые команды привода Indy по UART (порт датчика ЭМГ, 115200).
В фоне: ответ на опрос версии (6) и поток ЭМГ A=B=0, как у настоящего датчика (HandUart).

  py -3 uart_cmd.py open <сек>
  py -3 uart_cmd.py close <сек>
  py -3 uart_cmd.py close_duty <0..100> <сек>    — 01 11 01 <pct> (на этой плате закрывает)
  py -3 uart_cmd.py open_duty <0..100> <сек>     — 01 12 01 <pct> (на этой плате открывает)
Направления инвертированы относительно документации: 0x10 открывает, 0x0F закрывает.
  py -3 uart_cmd.py stop
Опции: UART_PORT (COM20)
"""
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'common'))
from hand_link import CMD_STOP, HandUart, pack

PORT = os.environ.get('UART_PORT', 'COM20')
t0 = time.time()


def log(msg):
    print(f'{time.time() - t0:6.3f}s {msg}', flush=True)


hand = HandUart(PORT)


def send(raw, note):
    hand.send(raw)
    log(f'-> {raw.hex(" ").upper()}  {note}')


# Плата принимает команды только после того, как «увидела» датчик:
# ждём её опроса версии и ответа на него.
log('жду опроса версии от платы...')
if hand.linked.wait(timeout=6):
    log('связь с платой есть (ответил на опрос версии)')
    time.sleep(0.1)
else:
    log('ВНИМАНИЕ: опроса версии за 6 с не было — шлю команду всё равно')

args = sys.argv[1:]
try:
    cmd = args[0] if args else 'stop'
    if cmd in ('open', 'close'):
        sec = float(args[1])
        # на этой плате направления инвертированы: 0x10 открывает, 0x0F закрывает
        send(pack(0x10 if cmd == 'open' else 0x0F), cmd)
        time.sleep(sec)
    elif cmd in ('open_duty', 'close_duty'):
        pct, sec = int(args[1]), float(args[2])
        # так же инвертированы: 0x12 открывает, 0x11 закрывает
        send(pack(0x12 if cmd == 'open_duty' else 0x11, [pct]), f'{cmd} {pct}%')
        time.sleep(sec)
    elif cmd != 'stop':
        print(__doc__)
finally:
    send(CMD_STOP, 'stop')
    time.sleep(0.1)
    hand.close()
