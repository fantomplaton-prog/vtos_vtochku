"""
Монитор сырых данных TacMagVTOS: каждая ось каждого магнитометра MLX90393 отдельно (+ FSR по желанию).

Сетка графиков: строка — ячейка (THUMB/INDEX/MIDDLE), столбец — ось X/Y/Z, на графике 4 линии MLX0..3.
Флажки справа включают/выключают ячейки, оси, отдельные MLX и каналы FSR.
Ячейки и FSR выключаются и в прошивке (кадр 0xAC с маской) — кадр короче, частота выше.

Запуск:
  py tools/raw_monitor.py --ble HM-10-1           # HM-10 по BLE
  py tools/raw_monitor.py --port COM5             # USB-UART на линиях USART2
  py tools/raw_monitor.py --port COM5 --fsr       # сразу с FSR
  py tools/raw_monitor.py --ble HM-10-1 --ut      # в мкТл вместо LSB
  py tools/raw_monitor.py --mock                  # без железа
  ... --csv log.csv                               # писать все кадры в CSV

Зависимости: py -m pip install numpy matplotlib pyserial bleak
"""

import argparse
import collections
import math
import sys
import threading
import time

import numpy as np

from sensor_link import (AXES, AXES_MASK_FSR, NUM_CELLS, NUM_MLX, BleLink, FrameParser, SerialLink,
                         axes_frame_len, build_axes_frame)

CELLS = ["THUMB", "INDEX", "MIDDLE"]
AXIS_NAMES = ["X", "Y", "Z"]
FSR_NAMES = ["FSR1_THUMB", "FSR2_THUMB", "FSR1_INDEX", "FSR2_INDEX", "FSR1_MIDDLE", "FSR2_MIDDLE"]
# мкТл/LSB при GAIN_1X, RES_16 (cfg в Core/Src/mlx_90393_app.c)
LSB_UT = np.array([0.150, 0.150, 0.242])


class MockLink:
    """Синтетика, понимает те же команды, что прошивка."""

    budget_bps = 8000

    def __init__(self, on_data):
        self.on_data = on_data
        self.mask, self.period = 0, 0.05
        self.alive = True
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        t0 = time.time()
        while self.alive:
            t = time.time() - t0
            if self.mask:
                mag = [[[int(300 * math.sin(0.7 * t + c + 0.3 * m + a) + 40 * m) for a in range(AXES)]
                        for m in range(NUM_MLX)] for c in range(NUM_CELLS)]
                fsr = [int(max(0, 3000 * math.sin(0.5 * t + i))) for i in range(6)]
                self.on_data(build_axes_frame(self.mask, mag, fsr))
            time.sleep(max(self.period, 0.01))

    def write(self, data: bytes):
        for c in data:
            if c & 0xF0 == 0x80:
                self.mask = c & 0x0F
            elif c & 0xF0 == 0x90:
                self.period = (c & 0x0F) * 0.01

    def close(self):
        self.alive = False


class Monitor:
    HISTORY_S = 10.0

    def __init__(self, link_factory, fsr, ut, period_ms, csv_path, keep_mode):
        self.cells_on = [True] * NUM_CELLS
        self.axes_on = [True] * AXES
        self.mlx_on = [True] * NUM_MLX
        self.fsr_on = [fsr] * 6
        self.ut = ut
        self.period_ms = period_ms
        self.keep_mode = keep_mode

        self.lock = threading.Lock()
        self.parser = FrameParser()
        self.t0 = time.time()
        maxlen = int(self.HISTORY_S * 120)
        self.t = collections.deque(maxlen=maxlen)
        self.mag = collections.deque(maxlen=maxlen)     # (3,4,3), NaN — ячейка не передавалась
        self.fsr = collections.deque(maxlen=maxlen)     # (6,)
        self.n_frames = 0
        self.last_mask = None
        self.rate_t, self.rate_n, self.rate = time.time(), 0, 0.0
        self.last_cmd_t = 0.0

        self.csv = None
        if csv_path:
            self.csv = open(csv_path, "w", encoding="utf-8")
            cols = [f"{c}_MLX{m}_{a}" for c in CELLS for m in range(NUM_MLX) for a in AXIS_NAMES]
            self.csv.write(",".join(["t_s"] + cols + FSR_NAMES) + "\n")

        self.link = link_factory(self._on_data)
        self.send_config()

    # --- приём
    def _on_data(self, data):
        with self.lock:
            for kind, f in self.parser.feed(data):
                if kind != "axes":
                    continue                    # хвост кадров старого режима после переключения
                self.n_frames += 1
                self.last_mask = f["mask"]
                mag = np.full((NUM_CELLS, NUM_MLX, AXES), np.nan)
                for c, v in enumerate(f["mag"]):
                    if v is not None:
                        mag[c] = v
                fsr = np.asarray(f["fsr"], float) if f["fsr"] else np.full(6, np.nan)
                t = time.time() - self.t0
                self.t.append(t)
                self.mag.append(mag)
                self.fsr.append(fsr)
                if self.csv:
                    row = [f"{t:.3f}"] + ["" if np.isnan(v) else str(int(v))
                                           for v in np.concatenate([mag.ravel(), fsr])]
                    self.csv.write(",".join(row) + "\n")

    # --- команды плате
    def fw_mask(self):
        m = sum(1 << c for c in range(NUM_CELLS) if self.cells_on[c])
        return m | (AXES_MASK_FSR if any(self.fsr_on) else 0)

    def period_k(self, mask):
        if self.period_ms is not None:
            ms = self.period_ms
        else:   # сколько успевает канал: BLE ~1 КБ/с, UART ~8 КБ/с
            ms = axes_frame_len(mask) * 1000.0 / self.link.budget_bps
        return int(min(15, max(1, math.ceil(ms / 10.0))))

    def send_config(self):
        mask = self.fw_mask()
        # прошивка читает по одному байту за цикл — байты команд шлём с паузой
        self.link.write(bytes([0x90 | self.period_k(mask)]))
        time.sleep(0.05)
        self.link.write(bytes([0x80 | mask]))
        self.last_cmd_t = time.time()

    def close(self):
        try:
            self.link.write(b"F" if not self.keep_mode else b"")
        finally:
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
        ax_fsr = fig.add_axes([0, 0, 0.1, 0.1])
        ax_fsr.set_title("FSR, отсчёты АЦП", fontsize=9)
        ax_fsr.set_ylim(0, 4200)
        ax_fsr.tick_params(labelsize=7)
        ax_fsr.grid(alpha=0.3)
        fsr_lines = [ax_fsr.plot([], [], lw=1, label=n)[0] for n in FSR_NAMES]
        ax_fsr.legend(fontsize=7, loc="upper left", ncol=3)

        def layout():
            rows = [c for c in range(NUM_CELLS) if self.cells_on[c]]
            cols = [a for a in range(AXES) if self.axes_on[a]]
            show_fsr = any(self.fsr_on)
            n_rows = len(rows) + (1 if show_fsr else 0)
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
            ax_fsr.set_visible(show_fsr)
            if show_fsr:
                ax_fsr.set_position([x0, y0, x1 - x0, h])
            for c in range(NUM_CELLS):
                for a in range(AXES):
                    for m in range(NUM_MLX):
                        lines[c][a][m].set_visible(self.mlx_on[m])
            for i, ln in enumerate(fsr_lines):
                ln.set_visible(self.fsr_on[i])

        def checks(rect, title, labels, state, on_change):
            fig.text(rect[0], rect[1] + rect[3] + 0.005, title, fontsize=9, weight="bold")
            cb = CheckButtons(fig.add_axes(rect), labels, list(state))

            def cb_clicked(label):
                state[labels.index(label)] = not state[labels.index(label)]
                on_change()

            cb.on_clicked(cb_clicked)
            return cb

        def changed_fw():
            layout()
            self.send_config()

        x = 0.75
        widgets = [
            checks([x, 0.82, 0.09, 0.12], "Ячейки", CELLS, self.cells_on, changed_fw),
            checks([x, 0.65, 0.09, 0.12], "Оси", AXIS_NAMES, self.axes_on, layout),
            checks([x, 0.46, 0.09, 0.14], "Магнитометры", [f"MLX{m}" for m in range(NUM_MLX)],
                   self.mlx_on, layout),
            checks([x, 0.20, 0.09, 0.21], "FSR", FSR_NAMES, self.fsr_on, changed_fw),
        ]
        ax_txt = fig.add_axes([0.855, 0.02, 0.14, 0.95])
        ax_txt.axis("off")
        txt = ax_txt.text(0, 1, "", va="top", family="monospace", fontsize=8)
        layout()

        def update(_):
            with self.lock:
                t = np.asarray(self.t)
                mag = np.asarray(self.mag) if self.mag else np.empty((0, NUM_CELLS, NUM_MLX, AXES))
                fsr = np.asarray(self.fsr) if self.fsr else np.empty((0, 6))
                n, crc, mask = self.n_frames, self.parser.crc_errors, self.last_mask
            now = time.time()
            if now - self.rate_t >= 1.0:
                self.rate = (n - self.rate_n) / (now - self.rate_t)
                self.rate_t, self.rate_n = now, n
            # команда могла потеряться (прошивка читает байт раз за цикл) — повторяем
            want = self.fw_mask()
            if mask != want and now - self.last_cmd_t > 1.0:
                self.send_config()

            if self.ut:
                mag = mag * LSB_UT
            if len(t):
                for c in range(NUM_CELLS):
                    for a in range(AXES):
                        ax = ax_mag[c][a]
                        if not ax.get_visible():
                            continue
                        for m in range(NUM_MLX):
                            lines[c][a][m].set_data(t, mag[:, c, m, a])
                        ax.set_xlim(max(0, t[-1] - self.HISTORY_S), t[-1] + 0.1)
                        ax.relim(visible_only=True)
                        ax.autoscale_view(scalex=False)
                if ax_fsr.get_visible():
                    for i, ln in enumerate(fsr_lines):
                        ln.set_data(t, fsr[:, i])
                    ax_fsr.set_xlim(max(0, t[-1] - self.HISTORY_S), t[-1] + 0.1)

            fmt = "{:8.1f}" if self.ut else "{:7.0f}"
            out = [f"{self.rate:5.1f} кадр/с", f"CRC err {crc}",
                   f"кадр {axes_frame_len(want)} байт, {self.period_k(want) * 10} мс", ""]
            if len(t):
                for c in range(NUM_CELLS):
                    if not self.cells_on[c]:
                        continue
                    out.append(f"{CELLS[c]:<7}" + "".join(f"{a:>{8 if self.ut else 7}}" for a in AXIS_NAMES))
                    for m in range(NUM_MLX):
                        v = mag[-1, c, m]
                        out.append(f" MLX{m}  " + "".join(fmt.format(x) if not np.isnan(x) else "      --"
                                                       for x in v))
                    out.append("")
                if any(self.fsr_on):
                    for i, nme in enumerate(FSR_NAMES):
                        if self.fsr_on[i]:
                            v = fsr[-1, i]
                            out.append(f"{nme:<12}{'--' if np.isnan(v) else int(v):>6}")
            else:
                out.append("ожидание кадров 0xAC...")
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
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--fsr", action="store_true", help="сразу включить все FSR")
    p.add_argument("--ut", action="store_true", help="показывать мкТл вместо сырых LSB")
    p.add_argument("--period-ms", type=int, help="период кадра 10..150 мс (по умолчанию — под канал)")
    p.add_argument("--csv", help="записывать все кадры в CSV")
    p.add_argument("--keep-mode", action="store_true", help="при выходе не возвращать плату в режим 'F'")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    if a.port:
        factory = lambda cb: SerialLink(a.port, a.baud, cb)
    elif a.ble:
        factory = lambda cb: BleLink(a.ble, cb)
    else:
        factory = MockLink
    Monitor(factory, a.fsr, a.ut, a.period_ms, a.csv, a.keep_mode).run()


if __name__ == "__main__":
    main()
