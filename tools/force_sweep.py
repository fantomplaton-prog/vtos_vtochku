"""
Замер силы по значениям fe42 без накопления: каждое измерение — из полностью
открытого положения, одно длинное закрытие на предмет.
Управление через окно status_window.py: поле для результата + кнопка «Далее».

ШКАЛА fe42 ОБРАТНАЯ: меньше значение = больше допустимый ток (сила).

  py -3 force_sweep.py [значения через запятую] [время_закрытия_с]
  по умолчанию: 80,40,100,60,30,20  и 1.5 c
Результаты: tools/force_sweep_<время>.csv
"""
import csv
import os
import subprocess
import sys
import time

VALUES = [int(x) for x in (sys.argv[1] if len(sys.argv) > 1 else '80,40,100,60,30,20').split(',')]
CLOSE_S = sys.argv[2] if len(sys.argv) > 2 else '1.5'
# режим 'free' — без предмета: проверяем, проходит ли полный ход за одну команду
FREE = len(sys.argv) > 3 and sys.argv[3] == 'free'
Q_READY = 'Кисть открыта, предмета НЕТ?' if FREE else 'Кисть открыта, предмет на месте?'
Q_RESULT = ('Закрылась до конца?\n(полностью / на N% / чуть-чуть)' if FREE
            else 'Оцените сжатие (мм / слабо / сильно)')
OPEN_FE42 = '40'   # значение, при котором кисть гарантированно открывается полностью
HERE = os.path.dirname(os.path.abspath(__file__))
STATUS = os.path.join(HERE, 'status.txt')
ACK = STATUS + '.ack'
ENV = dict(os.environ, PYTHONIOENCODING='utf-8')
OUT = os.path.join(HERE, f'force_sweep_{time.strftime("%Y%m%d_%H%M%S")}.csv')


def status(text, color='white', ask=False):
    with open(STATUS, 'w', encoding='utf-8') as f:
        f.write(f'{color}|{text}' + ('|ASK' if ask else ''))
    print(f'{time.time()-t0:6.1f}s {text.replace(chr(10), " / ")}', flush=True)


def wait_ack():
    while not os.path.exists(ACK):
        time.sleep(0.2)
    time.sleep(0.1)
    with open(ACK, encoding='utf-8') as f:
        text = f.read().strip()
    os.remove(ACK)
    return text


def indy(*args, tries=3):
    for _ in range(tries):
        out = subprocess.run([sys.executable, os.path.join(HERE, 'indy_cmd.py'), *args],
                             capture_output=True, text=True, env=ENV, encoding='utf-8').stdout
        lines = [l for l in out.strip().splitlines() if l]
        if lines and lines[-1].startswith('ok'):
            return lines[-2] if len(lines) > 1 else lines[-1]
        time.sleep(2)
    raise RuntimeError(f'команда {args} не прошла')


t0 = time.time()
if os.path.exists(ACK):
    os.remove(ACK)
status('подготовка...', 'light gray')
window = subprocess.Popen([sys.executable, os.path.join(HERE, 'status_window.py'), STATUS])
time.sleep(2)

rows = []
try:
    n = len(VALUES)
    for i, v in enumerate(VALUES, 1):
        status(f'Замер {i}/{n}: fe42 = {v}\n{Q_READY}\nНажмите «Далее»',
               'light yellow', ask=True)
        wait_ack()

        status(f'Замер {i}/{n}: fe42 = {v}\nустановка силы...', 'light yellow')
        indy('fe42', str(v))
        status(f'Замер {i}/{n}: fe42 = {v}\nЗАКРЫВАЮ', 'orange')
        r = indy('close', CLOSE_S)

        status(f'Замер {i}/{n}: fe42 = {v}\n{Q_RESULT}\n'
               'впишите и нажмите «Далее»', 'pale green', ask=True)
        result = wait_ack()
        rows.append({'n': i, 'fe42': v, 'close': r, 'result': result})
        print(f'         результат: {result!r}')

        status(f'Замер {i}/{n}\nОТКРЫВАЮ (fe42 = {OPEN_FE42})', 'light blue')
        indy('fe42', OPEN_FE42)
        indy('open', '1.5')
        indy('open', '1.0')   # контрольное — добить до упора

    status('ЗАМЕР ЗАВЕРШЁН', 'light blue')
except Exception as e:
    status(f'ОШИБКА: {e}', 'tomato')
    raise
finally:
    with open(OUT, 'w', newline='', encoding='utf-8-sig') as f:
        w = csv.DictWriter(f, fieldnames=['n', 'fe42', 'close', 'result'])
        w.writeheader()
        w.writerows(rows)
    print(f'результаты: {OUT}')
    time.sleep(4)
    status('__EXIT__')
    window.wait(timeout=5)
