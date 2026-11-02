"""
Признаки положения магнита (tools/common/mag_features.py) по записи с сырыми осями магнитометров.

Подходит любой CSV с колонками t_s и THUMB_MLX0_X … MIDDLE_MLX3_Z (ramp_*, slip_*, tm_*, fcal_*);
если есть force_pct / weight — рисуются сверху.

На графике по каждому пальцу:
  shear x, y   — (Gx_z, Gy_z): сдвиг магнита в плоскости;
  normal       — (Gx_x + Gy_y)/2: прижатие;
  |Δ общего|   — модуль изменения среднего четырёх датчиков: внешнее поле + магнит; для сравнения —
                 этот канал «чувствует» и поворот пальца, признаки выше — нет.
Все — изменения относительно начала записи, мкТл. Вертикальные линии — резкие события: скачок
сдвига за ~50 мс больше --k σ (σ — шум скачков, робастно).

Запуск (из папки analysis):
  uv run python mag_gradients.py --file ../tools/data/ramp_20261026_170237.csv
  uv run python mag_gradients.py --file ../tools/data/slip_XXXX.csv --k 6 --no-show
"""
import argparse
import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.stride_tricks import sliding_window_view

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, gradients

MAG_START = 'THUMB_MLX0_X'


def load(path):
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    h = rows[0]
    col = {n: i for i, n in enumerate(h)}
    j = col[MAG_START]
    rows = [r for r in rows[1:] if len(r) >= j + 36 and r[j] != '']
    t = np.array([float(r[col['t_s']]) for r in rows])
    mag = np.array([[float(v) for v in r[j:j + 36]] for r in rows])
    extra = {k: np.array([float(r[col[k]]) if r[col[k]] != '' else np.nan for r in rows])
             for k in ('force_pct', 'weight') if k in col}
    return t, mag, extra


def med3(x):
    """Медиана по 3 кадрам (по оси 0) — одиночные выбросы не дают «скачков»."""
    if len(x) < 3:
        return x
    return np.concatenate([x[:1], np.median(sliding_window_view(x, 3, axis=0), axis=-1), x[-1:]])


def events(t, shear, lag, k):
    """Скачки |Δshear| за lag кадров больше медиана + k·σ; соседние кадры сливаются. -> [(t, величина)]."""
    s = med3(shear)
    ds = np.hypot(*(s[lag:] - s[:-lag]).T)
    sig = 1.4826 * np.median(np.abs(ds - np.median(ds)))
    big = np.where(ds > np.median(ds) + k * max(sig, 1e-9))[0]
    out = []
    for i in big:
        if out and i - out[-1][0] <= lag:
            if ds[i] > out[-1][2]:
                out[-1] = (out[-1][0], i, ds[i])
            continue
        out.append((i, i, ds[i]))
    return [(t[i + lag], v) for _, i, v in out]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True)
    p.add_argument('--k', type=float, default=8.0, help='порог скачка, σ (8)')
    p.add_argument('--lag-ms', type=float, default=50.0, help='окно скачка, мс (50)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    t, mag, extra = load(a.file)
    g = gradients(mag)
    fs = (len(t) - 1) / (t[-1] - t[0])
    lag = max(1, int(round(a.lag_ms / 1000 * fs)))
    shear = g['shear'] - g['shear'][:20].mean(axis=0)
    normal = g['normal'] - g['normal'][:20].mean(axis=0)
    common = np.linalg.norm(g['mean'] - g['mean'][:20].mean(axis=0), axis=-1)

    print(f'{Path(a.file).name}: кадров {len(t)}, {fs:.1f}/с, окно скачка {lag} кадр(а)')
    n_top = 1 if extra else 0
    fig, axs = plt.subplots(3 + n_top, 1, figsize=(14, 9), sharex=True,
                            height_ratios=[0.5] * n_top + [1, 1, 1])
    if extra:
        ax = axs[0]
        for k, v in extra.items():
            ax.plot(t, v, lw=1, label=k)
        ax.legend(loc='upper left', fontsize=8)
        ax.grid(alpha=0.3)
    for c in range(3):
        ax = axs[c + n_top]
        ax.plot(t, shear[:, c, 0], lw=0.8, label='shear x (Gx_z)')
        ax.plot(t, shear[:, c, 1], lw=0.8, label='shear y (Gy_z)')
        ax.plot(t, normal[:, c], lw=0.8, label='normal')
        ax.plot(t, common[:, c], lw=0.8, color='gray', alpha=0.6, label='|Δ общего| (внешнее поле)')
        ev = events(t, g['shear'][:, c], lag, a.k)
        for te, _ in ev:
            ax.axvline(te, color='m', lw=1, alpha=0.6)
        ax.set_ylabel(f'{CELLS[c]}, мкТл')
        ax.grid(alpha=0.3)
        print(f'  {CELLS[c]:6}: скачков сдвига > {a.k:.0f}σ: {len(ev)}'
              + (': ' + ', '.join(f'{te:.2f} с ({v:.0f})' for te, v in ev[:10]) if ev else ''))
    axs[n_top].legend(loc='upper left', fontsize=8, ncol=4)
    axs[-1].set_xlabel('время, с')
    fig.suptitle(f'{Path(a.file).name}: признаки положения магнита (изменение от начала)', fontsize=10)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_grad.png'
    fig.savefig(png, dpi=110)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
