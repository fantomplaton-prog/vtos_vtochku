"""
Окно поверх всех окон с крупным текстом статуса теста.
Читает текст из файла (по умолчанию tools/status.txt) каждые 150 мс.
Формат файла: "ЦВЕТ|текст" или "ЦВЕТ|текст|ASK" — с ASK показываются поле ввода
и кнопка «Далее»; по нажатию введённый текст пишется в <файл>.ack.

  py -3 status_window.py [путь_к_файлу]
"""
import os
import sys
import tkinter as tk

PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)), 'status.txt')
ACK = PATH + '.ack'

root = tk.Tk()
root.title('Indy — статус теста')
root.attributes('-topmost', True)
root.geometry('760x360+40+40')
label = tk.Label(root, text='ожидание...', font=('Segoe UI', 28, 'bold'),
                 justify='center', wraplength=740)
label.pack(expand=True, fill='both')

ask_frame = tk.Frame(root)
entry = tk.Entry(ask_frame, font=('Segoe UI', 18), width=30)
entry.pack(side='left', padx=8, pady=8)


def on_next(_event=None):
    with open(ACK, 'w', encoding='utf-8') as f:
        f.write(entry.get().strip())
    entry.delete(0, 'end')
    ask_frame.pack_forget()


tk.Button(ask_frame, text='Далее ▶', font=('Segoe UI', 18, 'bold'),
          command=on_next).pack(side='left', padx=8, pady=8)
entry.bind('<Return>', on_next)
last = None


def poll():
    global last
    try:
        with open(PATH, encoding='utf-8') as f:
            text = f.read().strip()
    except OSError:
        text = 'ожидание...'
    if text != last:
        last = text
        parts = text.split('|')
        color, body = (parts[0], parts[1]) if len(parts) >= 2 else ('white', text)
        if body == '__EXIT__':
            root.destroy()
            return
        label.config(text=body, bg=color)
        root.config(bg=color)
        if len(parts) >= 3 and parts[2] == 'ASK':
            ask_frame.pack(side='bottom')
            entry.focus_set()
        else:
            ask_frame.pack_forget()
    root.after(150, poll)


poll()
root.mainloop()
