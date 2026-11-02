"""
Сила на палец «как в прошивке» и градиентный признак |(Gx + Gy)/2| по записи с сырыми осями.

Цепочка (INDI_sensors_Arm/TacMagVTOS, mlx_90393_app.c), повторена в tools/common/mag_features.py:
  оси -> мкТл -> EMA α = 0.6 -> F = (|B3 − B0| + |B2 − B1|)/2 -> baseline -> F − baseline (< 3 мкТл = 0).
Baseline: перезапись текущим F, когда кисть раскрылась (раскрытие было > 96 %, стало < 60 %);
медленный дрейф EMA α = 0.01, пока нет касания (F > baseline + 15 мкТл у любого пальца).
Признак G = |(Gx + Gy)/2| (общая система ячейки, датчики 1, 2 повёрнуты) — те же EMA и baseline,
с тем же флагом касания.

Раскрытия (датчик Холла) в ramp-файле нет — оно восстанавливается по командам: пока подана сила,
кисть закрыта (100 %); после команды «открыть» раскрытие падает ниже 60 % через --open-delay с
(по умолчанию 0.3 с: 40 % хода при полном ходе 0.75 с). Если в файле есть колонка hall_filt
или closure — берётся она.

Запуск (из папки analysis):
  uv run python force_pipeline.py                        # ramp_20261026_170237
  uv run python force_pipeline.py --file ../tools/data/XXX.csv --no-show
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, baseline_correct, ema, final_mag, gradients, to_common
from mag_gradients import load


def closure_from_force(t, force, open_delay):
    """Раскрытие, %, по командам: сила > 0 — 100; после снятия силы — 100 ещё open_delay с, затем 0."""
    cl = np.where(force > 0, 100.0, 0.0)
    off = np.where((force[:-1] > 0) & (force[1:] <= 0))[0] + 1
    for i in off:
        cl[(t >= t[i]) & (t < t[i] + open_delay) & (force <= 0)] = 100.0
    return cl


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', default=str(HERE.parent / 'tools' / 'data' / 'ramp_20261026_170237.csv'))
    p.add_argument('--open-delay', type=float, default=0.3, help='раскрытие < 60 %% через, с после «открыть» (0.3)')
    p.add_argument('--init-frames', type=int, default=20, help='baseline при старте = среднее первых кадров (20)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    t, mag, extra = load(a.file)
    force = extra.get('force_pct')
    if force is None:
        raise SystemExit('в файле нет force_pct — раскрытие восстановить не из чего')
    closure = closure_from_force(t, force, a.open_delay)

    raw = to_common(mag)
    filt = ema(raw)                                    # EMA по каждой оси, как MLX90393_FilterXYZ
    F_raw, F = final_mag(raw), final_mag(filt)
    g = gradients(None, common=filt)
    G = np.linalg.norm((g['gx'] + g['gy']) / 2, axis=-1)
    G_raw = np.linalg.norm((gradients(None, common=raw)['gx'] + gradients(None, common=raw)['gy']) / 2, axis=-1)
    Fc, Fb, touch, [(Gc, Gb)] = baseline_correct(F, closure, others=[G], init_frames=a.init_frames)

    grip = force > 0
    print(f'{Path(a.file).name}: кадров {len(t)}; касание по F в {touch[grip].mean() * 100:.0f}% кадров хвата, '
          f'вне хвата {touch[~grip].mean() * 100:.0f}%')
    for c in range(3):
        nF = 1.4826 * np.median(np.abs(np.diff(F_raw[grip, c]) - np.median(np.diff(F_raw[grip, c])))) / np.sqrt(2)
        nFe = 1.4826 * np.median(np.abs(np.diff(F[grip, c]) - np.median(np.diff(F[grip, c])))) / np.sqrt(2)
        nG = 1.4826 * np.median(np.abs(np.diff(G[grip, c]) - np.median(np.diff(G[grip, c])))) / np.sqrt(2)
        print(f'  {CELLS[c]:6} шум F: {nF:.2f} мкТл сырой -> {nFe:.2f} с EMA; шум G (с EMA) {nG:.2f}; '
              f'F−baseline на максимуме силы ~{np.percentile(Fc[grip, c], 99):.0f}, G−baseline ~{np.percentile(Gc[grip, c], 99):.0f} мкТл')

    fig, axs = plt.subplots(4, 2, figsize=(16, 10), sharex=True, height_ratios=[0.45, 1, 1, 1])
    for k in range(2):
        axs[0, k].plot(t, force, color='k', lw=1, label='сила команды, %')
        axs[0, k].plot(t, closure, color='#5cb85c', lw=1, alpha=0.7, label='раскрытие (восстановлено), %')
        axs[0, k].fill_between(t, 0, 105, where=touch, color='orange', alpha=0.15, step='mid', label='касание (F)')
        axs[0, k].legend(loc='upper left', fontsize=7, ncol=3)
        axs[0, k].grid(alpha=0.3)
    axs[0, 0].set_title('Сила как в прошивке: F = (|B3−B0| + |B2−B1|)/2', fontsize=10)
    axs[0, 1].set_title('Градиент: G = |(Gx + Gy)/2|', fontsize=10)
    for c in range(3):
        for k, (raw_s, s, b, corr) in enumerate(((F_raw, F, Fb, Fc), (G_raw, G, Gb, Gc))):
            ax = axs[c + 1, k]
            ax.plot(t, raw_s[:, c], color='#bbbbbb', lw=0.6, label='сырой')
            ax.plot(t, s[:, c], color='#1f77b4', lw=0.8, label='EMA 0.6')
            ax.plot(t, b[:, c], color='#d62728', lw=1, ls='--', label='baseline')
            ax.plot(t, corr[:, c], color='#2ca02c', lw=1, label='минус baseline')
            ax.set_ylabel(f'{CELLS[c]}, мкТл')
            ax.grid(alpha=0.3)
    axs[1, 0].legend(loc='upper left', fontsize=7, ncol=4)
    axs[-1, 0].set_xlabel('время, с')
    axs[-1, 1].set_xlabel('время, с')
    fig.suptitle(Path(a.file).name, fontsize=10)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_force.png'
    fig.savefig(png, dpi=110)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
