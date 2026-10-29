"""
Управление кистью по UART (порт датчика ЭМГ, 115200) с регулировкой силы и
плавным сжатием/разжатием. Регулятор — HandController из common/hand_link.py.

Ползунки:
  Сила, %          — с рабочей калибровкой (tools/common/force_calib.json) — % от максимальной
                     реальной силы; без неё — сырой % в команде закрытия 01 11 01 <pct>
  Сжатие (цель), % — 0 открыто .. 100 закрыто; кисть следует за ползунком
  Скорость, %      — программный ШИМ: доля времени работы мотора в окне 120 мс

Положение кисти НЕ измеряется — оценивается по времени работы мотора
(полный ход ~0.75 c). Кнопка «Открыть полностью» сбрасывает оценку в 0.

Направления на этой плате инвертированы относительно документации:
  0x10 — открыть, 0x0F — закрыть, 0x11 <pct> — закрыть с ШИМ, 0x13 — стоп.

  py -3 hand_gui.py [COM]
Лог команд: tools/data/hand_gui_<время>.csv
"""
import csv
import os
import queue
import sys
import time
import tkinter as tk

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'common'))
from hand_link import HandController

PORT = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('UART_PORT', 'COM20')
OUT = os.path.join(HERE, '..', 'data', f'hand_gui_{time.strftime("%Y%m%d_%H%M%S")}.csv')

ui_q = queue.Queue()
log_rows = []
t0 = time.time()
hand = None


def on_cmd(t_abs, raw, note):
    log_rows.append((f'{t_abs - t0:.3f}', raw.hex(' '), note,
                     round(hand.pos, 1), round(hand.target, 1), hand.force, hand.speed))
    ui_q.put(('cmd', f'{t_abs - t0:7.2f}s {raw.hex(" ").upper():18} {note}'))


hand = HandController(PORT, on_cmd=on_cmd)
hand.on_target_reset = lambda v: ui_q.put(('target', v))

# ------------------------------------------------ GUI
root = tk.Tk()
root.title(f'Управление кистью по UART ({PORT})')
root.geometry('980x640+60+40')
F = ('Segoe UI', 13)
FB = ('Segoe UI', 15, 'bold')

top = tk.Frame(root)
top.pack(fill='x', padx=10, pady=6)


def make_slider(parent, title, color, frm, to, init, setter, res=1):
    fr = tk.Frame(parent)
    fr.pack(side='left', expand=True, fill='both', padx=8)
    tk.Label(fr, text=title, font=FB, fg=color).pack()
    sc = tk.Scale(fr, from_=to, to=frm, orient='vertical', length=330, width=40, font=F,
                  troughcolor=color, resolution=res, command=lambda v: setter(float(v)))
    sc.set(init)
    sc.pack()
    return sc


s_force = make_slider(top, 'Сила, %', '#d9534f', 10, 100, hand.force,
                      lambda v: setattr(hand, 'force', int(v)))
s_target = make_slider(top, 'Сжатие (цель), %', '#5cb85c', 0, 100, 0,
                       lambda v: setattr(hand, 'target', v))
s_speed = make_slider(top, 'Скорость, %', '#0275d8', 5, 100, hand.speed,
                      lambda v: setattr(hand, 'speed', int(v)))

side = tk.Frame(top)
side.pack(side='left', fill='y', padx=8)
link_lbl = tk.Label(side, text='Связь: жду опроса платы...', font=FB, fg='gray')
link_lbl.pack(anchor='w', pady=(0, 4))
tk.Label(side, text=(lambda fm: f'Сила: % от максимальной (калибровка {fm.source})' if fm
              else 'Сила: сырой % команды (калибровки нет)')(hand.force_model),
         font=('Segoe UI', 10), fg='#555', wraplength=260, justify='left').pack(anchor='w', pady=(0, 8))

tk.Label(side, text='Оценка положения:', font=F).pack(anchor='w')
pos_canvas = tk.Canvas(side, width=260, height=34, bg='white', highlightthickness=1)
pos_canvas.pack(anchor='w')
pos_lbl = tk.Label(side, text='', font=F)
pos_lbl.pack(anchor='w', pady=(0, 10))

tk.Button(side, text='Открыть полностью\n(калибровка)', font=F, width=20, command=hand.calibrate).pack(pady=3)
tk.Button(side, text='Цель 0 (открыть)', font=F, width=20, command=lambda: s_target.set(0)).pack(pady=3)
tk.Button(side, text='Цель 100 (сжать)', font=F, width=20, command=lambda: s_target.set(100)).pack(pady=3)
tk.Button(side, text='Дожать (сжать до упора\nс текущей силой)', font=F, width=20,
          command=hand.push).pack(pady=3)
tk.Button(side, text='СТОП', font=FB, bg='tomato', width=18, command=hand.stop_all).pack(pady=10)

tk.Label(root, text='Отправленные команды:', font=F).pack(anchor='w', padx=10)
cmd_txt = tk.Text(root, height=8, font=('Consolas', 11))
cmd_txt.pack(fill='both', expand=True, padx=10, pady=(0, 10))


def ui_tick():
    while not ui_q.empty():
        kind, payload = ui_q.get()
        if kind == 'target':
            s_target.set(payload)
        else:
            cmd_txt.insert('end', payload + '\n')
            if int(cmd_txt.index('end-1c').split('.')[0]) > 400:
                cmd_txt.delete('1.0', '100.0')
            cmd_txt.see('end')
    if hand.linked:
        link_lbl.config(text='Связь: есть', fg='green')
    p = hand.pos
    pos_canvas.delete('all')
    pos_canvas.create_rectangle(0, 0, 260 * p / 100, 34, fill='#5cb85c', width=0)
    pos_canvas.create_line(260 * hand.target / 100, 0, 260 * hand.target / 100, 34, fill='black', width=3)
    mv = {1: 'сжимаю', -1: 'разжимаю', 0: 'стоит'}[hand.moving]
    pos_lbl.config(text=f'{p:5.1f}% (цель {hand.target:.0f}%) — {mv}')
    root.after(40, ui_tick)


def on_close():
    hand.close()
    with open(OUT, 'w', newline='', encoding='utf-8-sig') as f:
        w = csv.writer(f)
        w.writerow(['t', 'raw', 'note', 'pos_est', 'target', 'force', 'speed'])
        w.writerows(log_rows)
    root.destroy()


root.protocol('WM_DELETE_WINDOW', on_close)
ui_tick()
root.mainloop()
print(f'лог: {OUT}')
