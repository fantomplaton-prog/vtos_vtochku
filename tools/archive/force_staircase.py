"""
"Лестница силы": на каждом уровне fe42 (SHUTDOWN_CURRENT) кисть закрывается долго,
пока сама не остановится (полное закрытие или равновесие с предметом).
Текущий этап крупно показывается в отдельном окне поверх всех окон (status_window.py).

  py -3 force_staircase.py [уровни через запятую] [время_закрытия_с]
  по умолчанию: 40,70,100,130,70,40  и 1.5 c
"""
import os
import subprocess
import sys
import time

LEVELS = [int(x) for x in (sys.argv[1] if len(sys.argv) > 1 else '40,70,100,130,70,40').split(',')]
CLOSE_S = sys.argv[2] if len(sys.argv) > 2 else '1.5'
HERE = os.path.dirname(os.path.abspath(__file__))
STATUS = os.path.join(HERE, 'status.txt')
ENV = dict(os.environ, PYTHONIOENCODING='utf-8')


def status(text, color='white'):
    with open(STATUS, 'w', encoding='utf-8') as f:
        f.write(f'{color}|{text}')
    print(f'{time.time()-t0:6.1f}s {text}', flush=True)


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
status('подготовка...', 'light gray')
window = subprocess.Popen([sys.executable, os.path.join(HERE, 'status_window.py'), STATUS])
time.sleep(2)

try:
    for stage, level in enumerate(LEVELS, 1):
        status(f'Этап {stage}/{len(LEVELS)}\nfe42 = {level}\nустановка силы...', 'light yellow')
        indy('fe42', str(level))
        status(f'Этап {stage}/{len(LEVELS)}\nfe42 = {level}\nЗАКРЫВАЮ', 'orange')
        r = indy('close', CLOSE_S)
        status(f'Этап {stage}/{len(LEVELS)}\nfe42 = {level}\nсмотрите на кисть', 'pale green')
        print(f'         {r}')
        time.sleep(4.0)
    status('ТЕСТ ЗАВЕРШЁН', 'light blue')
except Exception as e:
    status(f'ОШИБКА: {e}', 'tomato')
    raise
finally:
    time.sleep(5)
    status('__EXIT__')
    window.wait(timeout=5)
