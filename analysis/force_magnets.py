"""
Сырые оси магнитометров в зависимости от силы сжатия протеза.

Данные: tools/data/ramp_*.csv из force_ramp.py — несколько циклов, сила min → 100 % с шагом.
Колонки: t_s, force_pct, THUMB_MLX0_X … MIDDLE_MLX3_Z (сырые int16, LSB).

Метод: каждый цикл делится на ступени — подряд идущие кадры с одной силой (~0,5 с, ~30 кадров).
Для каждой ступени и оси берётся медиана — она отбрасывает выбросы на переходе между ступенями.
Получается одна точка на (цикл, сила) -> дискретные графики.
Ступень контакта (минимальная сила, ~2 с, в начале пальцы ещё едут к предмету) —
медиана только последних --contact-tail секунд.

Запуск (из папки analysis):
  uv run python force_magnets.py
  uv run python force_magnets.py --file ../tools/data/ramp_XXXX.csv --no-show
Сохраняет: <имя>_THUMB/INDEX/MIDDLE.png и <имя>_medians.csv рядом со скриптом.
"""
import argparse
import csv
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
CELLS = ['THUMB', 'INDEX', 'MIDDLE']
AXES = ['X', 'Y', 'Z']
N_MLX = 4


def load_ramp(path):
    """-> t, force, mag (N,36), mag_names, cycle (0 — вне хвата)."""
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    header = rows[0]
    data = np.array([[float(v) if v != '' else np.nan for v in r] for r in rows[1:]])
    t, force, mag = data[:, 0], data[:, 1], data[:, 2:]
    grip = force > 0
    starts = np.where(grip & ~np.r_[False, grip[:-1]])[0]
    cycle = np.zeros(len(t), int)
    for k, s in enumerate(starts, 1):
        end = starts[k] if k < len(starts) else len(t)
        cycle[s:end] = k
    cycle[~grip] = 0
    return t, force, mag, header[2:], cycle


def step_medians(t, force, mag, cycle, contact_tail_s=0.5):
    """Медиана каждой оси на каждой ступени. -> levels, med [цикл, уровень, ось], n_frames [цикл, уровень]."""
    grip = force > 0
    n_cycles = cycle.max()
    levels = np.unique(force[grip]).astype(int)
    f_min = levels.min()
    med = np.full((n_cycles, len(levels), mag.shape[1]), np.nan)
    n_frames = np.zeros((n_cycles, len(levels)), int)
    for c in range(1, n_cycles + 1):
        for j, L in enumerate(levels):
            sel = np.where((cycle == c) & (force == L))[0]
            if len(sel) == 0:
                continue
            if L == f_min:                               # ступень контакта — только хвост
                sel = sel[t[sel] >= t[sel[-1]] - contact_tail_s]
            med[c - 1, j] = np.median(mag[sel], axis=0)
            n_frames[c - 1, j] = len(sel)
    return levels, med, n_frames


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', default=str(HERE.parent / 'tools' / 'data' / 'ramp_20261026_170237.csv'))
    p.add_argument('--contact-tail', type=float, default=0.5, help='с: хвост ступени контакта для медианы')
    p.add_argument('--no-show', action='store_true', help='только сохранить картинки, не показывать окна')
    a = p.parse_args()
    path = Path(a.file)

    t, force, mag, names, cycle = load_ramp(path)
    n_cycles = cycle.max()
    col = {n: i for i, n in enumerate(names)}
    print(f'{path.name}: {len(t)} кадров, {t[-1]:.1f} с, {len(t) / t[-1]:.1f} кадр/с, циклов: {n_cycles}')

    levels, med, n_frames = step_medians(t, force, mag, cycle, a.contact_tail)
    print('кадров на ступень: медиана', int(np.median(n_frames[n_frames > 0])),
          '| мин', n_frames[n_frames > 0].min(), '| макс', n_frames.max())
    med_all = np.nanmedian(med, axis=0)

    colors = plt.cm.viridis(np.linspace(0, 0.9, n_cycles))
    for cell in CELLS:
        fig, axs = plt.subplots(N_MLX, len(AXES), figsize=(15, 13), sharex=True)
        fig.suptitle(f'{cell}: медиана оси на каждой ступени силы ({n_cycles} циклов)', fontsize=14)
        for m in range(N_MLX):
            for k, axis in enumerate(AXES):
                ax = axs[m, k]
                i = col[f'{cell}_MLX{m}_{axis}']
                for c in range(n_cycles):
                    ax.plot(levels, med[c, :, i], 'o', ms=3, color=colors[c], label=f'цикл {c + 1}')
                ax.plot(levels, med_all[:, i], '-', color='black', lw=1, label='медиана по циклам')
                ax.set_title(f'{cell}_MLX{m}_{axis}', fontsize=10)
                ax.grid(alpha=0.3)
                ax.tick_params(labelsize=8)
                if m == N_MLX - 1:
                    ax.set_xlabel('сила сжатия, %')
                if k == 0:
                    ax.set_ylabel('LSB')
        axs[0, 0].legend(fontsize=7, loc='best')
        fig.tight_layout(rect=(0, 0, 1, 0.97))
        fig.savefig(HERE / f'{path.stem}_{cell}.png', dpi=110)

    out = HERE / f'{path.stem}_medians.csv'
    with open(out, 'w', newline='', encoding='utf-8-sig') as fh:
        w = csv.writer(fh)
        w.writerow(['cycle', 'force_pct', 'n_frames'] + names)
        for c in range(n_cycles):
            for j, L in enumerate(levels):
                if n_frames[c, j]:
                    w.writerow([c + 1, L, n_frames[c, j]] + [f'{v:.1f}' for v in med[c, j]])
    print('сохранено:', out)
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
