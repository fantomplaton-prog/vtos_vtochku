"""
Монитор сырых данных TacMagVTOS (текстовый протокол FormatRawFrame, см. common/sensor_text.py):
каждая ось каждого магнитометра MLX90393 отдельно + FSR (каналы АЦП), датчик Холла, вес HX711.

Сетка графиков: строка — ячейка (THUMB/INDEX/MIDDLE), столбец — ось X/Y/Z, на графике 4 линии MLX0..3;
внизу — FSR и Холл/вес. Флажки справа только скрывают графики: прошивка всегда шлёт всю строку.

Запуск (из папки tools/sensors):
  py -3 raw_monitor.py --port COM21            # USB-UART на линиях USART2
  py -3 raw_monitor.py --ble HM-10-1           # HM-10 по BLE
  py -3 raw_monitor.py --port COM21 --ut       # в мкТл вместо LSB
  py -3 raw_monitor.py --mock                  # без железа
  ... --csv log.csv                            # писать все кадры в CSV
"""

import argparse
import collections
import os
import sys
import threading
import time

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "common"))
from sensor_link import BleLink, SerialLink
from sensor_text import AXES, AXIS_NAMES, CELLS, MAG_COLS, NUM_CELLS, NUM_MLX, MockTextLink, TextFrameParser

ADC_NAMES = [f"ADC{i}" for i in range(6)]       # FSR в порядке каналов АЦП, не пальцев
OTHER_NAMES = ["Холл", "Холл (фильтр)", "Вес"]
# мкТл/LSB при GAIN_1X, RES_16 (cfg в Core/Src/mlx_90393_app.c)
LSB_UT = np.array([0.150, 0.150, 0.242])


class Monitor:
    HISTORY_S = 10.0

    def __init__(self, link_factory, ut, csv_path):
        self.cells_on = [True] * NUM_CELLS
        self.axes_on = [True] * AXES
        self.mlx_on = [True] * NUM_MLX
        self.adc_on = [True] * 6
        self.other_on = [False, True, True]
        self.ut = ut

        self.lock = threading.Lock()
        self.parser = TextFrameParser()
        self.t0 = time.time()
        maxlen = int(self.HISTORY_S * 120)
        self.t = collections.deque(maxlen=maxlen)
        self.mag = collections.deque(maxlen=maxlen)     # (3,4,3)
        self.adc = collections.deque(maxlen=maxlen)     # (6,)
        self.other = collections.deque(maxlen=maxlen)   # (3,): hall, hall_filt, weight
        self.n_frames = 0
        self.rate_t, self.rate_n, self.rate = time.time(), 0, 0.0

        self.csv = None
        if csv_path:
            self.csv = open(csv_path, "w", encoding="utf-8")
            self.csv.write(",".join(["t_s"] + ADC_NAMES + ["hall", "hall_filt", "weight"] + MAG_COLS) + "\n")

        self.link = link_factory(self._on_data)

    # --- приём
    def _on_data(self, data):
        with self.lock:
            for f in self.parser.feed(data):
                self.n_frames += 1
                t = time.time() - self.t0
                self.t.append(t)
                self.mag.append(np.asarray(f["mag"], float).reshape(NUM_CELLS, NUM_MLX, AXES))
                self.adc.append(np.asarray(f["adc"], float))
                self.other.append(np.array([f["hall"], f["hall_filt"], f["weight"]], float))
                if self.csv:
                    row = ([f"{t:.3f}"] + [str(v) for v in f["adc"]]
                           + [str(f["hall"]), f"{f['hall_filt']:.4f}", f"{f['weight']:.4f}"]
                           + [str(v) for v in f["mag"]])
                    self.csv.write(",".join(row) + "\n")

    def close(self):
        self.link.close()
        if self.csv:
            self.csv.close()

    # --- GUI
    def run(self):
        import matplotlib.pyplot as plt
        from matplotlib.animation import FuncAnimation
        from matplotlib.widgets import CheckButtons

        fig = plt.figure(figsize=(16, 9))
        fig.canvas.manager.set_window_title("TacMagVTOS — сырые оси магнитометров")
        unit = "мкТл" if self.ut else "LSB"
        colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]

        ax_mag = [[fig.add_axes([0, 0, 0.1, 0.1]) for _ in range(AXES)] for _ in range(NUM_CELLS)]
        lines = [[[ax_mag[c][a].plot([], [], color=colors[m], lw=1, label=f"MLX{m}")[0]
                   for m in range(NUM_MLX)] for a in range(AXES)] for c in range(NUM_CELLS)]
        for c in range(NUM_CELLS):
            for a in range(AXES):
                ax = ax_mag[c][a]
                ax.set_title(f"{CELLS[c]} {AXIS_NAMES[a]}, {unit}", fontsize=9)
                ax.tick_params(labelsize=7)
                ax.grid(alpha=0.3)
        ax_mag[0][0].legend(fontsize=7, loc="upper left")

        ax_adc = fig.add_axes([0, 0, 0.1, 0.1])
        ax_adc.set_title("FSR, отсчёты АЦП", fontsize=9)
        adc_lines = [ax_adc.plot([], [], lw=1, label=n)[0] for n in ADC_NAMES]
        ax_adc.legend(fontsize=7, loc="upper left", ncol=3)
        ax_oth = fig.add_axes([0, 0, 0.1, 0.1])
        ax_oth.set_title("Холл / вес HX711", fontsize=9)
        oth_lines = [ax_oth.plot([], [], lw=1, label=n)[0] for n in OTHER_NAMES]
        ax_oth.legend(fontsize=7, loc="upper left", ncol=3)
        for ax in (ax_adc, ax_oth):
            ax.tick_params(labelsize=7)
            ax.grid(alpha=0.3)

        def layout():
            rows = [c for c in range(NUM_CELLS) if self.cells_on[c]]
            cols = [a for a in range(AXES) if self.axes_on[a]]
            show_bottom = any(self.adc_on) or any(self.other_on)
            n_rows = len(rows) + (1 if show_bottom else 0)
            x0, x1, y0, y1, gx, gy = 0.04, 0.72, 0.05, 0.96, 0.03, 0.06
            h = (y1 - y0 - gy * max(n_rows - 1, 0)) / max(n_rows, 1)
            w = (x1 - x0 - gx * max(len(cols) - 1, 0)) / max(len(cols), 1)
            for c in range(NUM_CELLS):
                for a in range(AXES):
                    vis = c in rows and a in cols
                    ax_mag[c][a].set_visible(vis)
                    if vis:
                        r, k = rows.index(c), cols.index(a)
                        ax_mag[c][a].set_position([x0 + k * (w + gx), y1 - (r + 1) * h - r * gy, w, h])
                    for m in range(NUM_MLX):
                        lines[c][a][m].set_visible(self.mlx_on[m])
            # нижняя строка: FSR и Холл/вес рядом (или один на всю ширину)
            bottom = [ax for ax, on in ((ax_adc, any(self.adc_on)), (ax_oth, any(self.other_on))) if on]
            for ax in (ax_adc, ax_oth):
                ax.set_visible(ax in bottom)
            bw = (x1 - x0 - gx * (len(bottom) - 1)) / max(len(bottom), 1)
            for k, ax in enumerate(bottom):
                ax.set_position([x0 + k * (bw + gx), y0, bw, h])
            for i, ln in enumerate(adc_lines):
                ln.set_visible(self.adc_on[i])
            for i, ln in enumerate(oth_lines):
                ln.set_visible(self.other_on[i])

        def checks(rect, title, labels, state):
            fig.text(rect[0], rect[1] + rect[3] + 0.005, title, fontsize=9, weight="bold")
            cb = CheckButtons(fig.add_axes(rect), labels, list(state))

            def cb_clicked(label):
                state[labels.index(label)] = not state[labels.index(label)]
                layout()

            cb.on_clicked(cb_clicked)
            return cb

        x = 0.75
        widgets = [
            checks([x, 0.84, 0.09, 0.11], "Ячейки", CELLS, self.cells_on),
            checks([x, 0.68, 0.09, 0.11], "Оси", AXIS_NAMES, self.axes_on),
            checks([x, 0.50, 0.09, 0.13], "Магнитометры", [f"MLX{m}" for m in range(NUM_MLX)], self.mlx_on),
            checks([x, 0.25, 0.09, 0.20], "FSR (АЦП)", ADC_NAMES, self.adc_on),
            checks([x, 0.08, 0.09, 0.12], "Прочее", OTHER_NAMES, self.other_on),
        ]
        ax_txt = fig.add_axes([0.855, 0.02, 0.14, 0.95])
        ax_txt.axis("off")
        txt = ax_txt.text(0, 1, "", va="top", family="monospace", fontsize=8)
        layout()

        def set_xy(ax, t, lines_, y):
            for i, ln in enumerate(lines_):
                ln.set_data(t, y[:, i])
            ax.set_xlim(max(0, t[-1] - self.HISTORY_S), t[-1] + 0.1)
            ax.relim(visible_only=True)
            ax.autoscale_view(scalex=False)

        def update(_):
            with self.lock:
                t = np.asarray(self.t)
                mag = np.asarray(self.mag) if self.mag else np.empty((0, NUM_CELLS, NUM_MLX, AXES))
                adc = np.asarray(self.adc) if self.adc else np.empty((0, 6))
                oth = np.asarray(self.other) if self.other else np.empty((0, 3))
                n, bad = self.n_frames, self.parser.bad_lines
            now = time.time()
            if now - self.rate_t >= 1.0:
                self.rate = (n - self.rate_n) / (now - self.rate_t)
                self.rate_t, self.rate_n = now, n

            if self.ut:
                mag = mag * LSB_UT
            if len(t):
                for c in range(NUM_CELLS):
                    for a in range(AXES):
                        if ax_mag[c][a].get_visible():
                            set_xy(ax_mag[c][a], t, lines[c][a], mag[:, c, :, a])
                if ax_adc.get_visible():
                    set_xy(ax_adc, t, adc_lines, adc)
                if ax_oth.get_visible():
                    set_xy(ax_oth, t, oth_lines, oth)

            fmt = "{:8.1f}" if self.ut else "{:7.0f}"
            out = [f"{self.rate:5.1f} кадр/с", f"битых строк {bad}", ""]
            if len(t):
                for c in range(NUM_CELLS):
                    if not self.cells_on[c]:
                        continue
                    out.append(f"{CELLS[c]:<7}" + "".join(f"{a:>{8 if self.ut else 7}}" for a in AXIS_NAMES))
                    for m in range(NUM_MLX):
                        out.append(f" MLX{m}  " + "".join(fmt.format(v) for v in mag[-1, c, m]))
                    out.append("")
                out += [f"{nme:<6}{int(v):>6}" for nme, v in zip(ADC_NAMES, adc[-1])]
                out += ["", f"Холл    {int(oth[-1, 0])}", f"Холл ф. {oth[-1, 1]:.1f}", f"Вес     {oth[-1, 2]:.2f}"]
            else:
                out.append("ожидание строк данных...")
            txt.set_text("\n".join(out))

        fig._keep = (widgets, FuncAnimation(fig, update, interval=100, cache_frame_data=False))
        try:
            plt.show()
        finally:
            self.close()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = p.add_mutually_exclusive_group(required=True)
    src.add_argument("--port", help="COM-порт USB-UART адаптера")
    src.add_argument("--ble", help="имя или MAC HM-10 (например HM-10-1)")
    src.add_argument("--mock", action="store_true", help="синтетические данные")
    p.add_argument("--baud", type=int, help="скорость UART (по умолчанию — автоопределение)")
    p.add_argument("--ut", action="store_true", help="показывать мкТл вместо сырых LSB")
    p.add_argument("--csv", help="записывать все кадры в CSV")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    if a.port:
        factory = lambda cb: SerialLink(a.port, a.baud, cb)
    elif a.ble:
        factory = lambda cb: BleLink(a.ble, cb)
    else:
        factory = MockTextLink
    Monitor(factory, a.ut, a.csv).run()


if __name__ == "__main__":
    main()
