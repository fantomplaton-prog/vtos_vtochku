"""
Сравнение способов оценки силы пальца по 4 магнитометрам на ramp-записи (плавный рост силы).

Способы (всё в мкТл, датчики 1 и 2 приведены к осям 3 и 0: x -> −x, y -> −y; EMA 0.6 как в прошивке):
  sides   F = (|B3 − B0| + |B2 − B1|) / 2   — как сейчас в прошивке (пары — стороны квадрата)
  diag    F = (|B0 − B1| + |B3 − B2|) / 2   — пары по диагоналям
  press   (Gx_x + Gy_y) / 2                 — прижатие из градиентов (common/mag_features.py)
Ноль: значение при раскрытой кисти перед каждым циклом (как baseline прошивки).

Опора — реальная сила: процент команды переводится в силу по весам через калибровку
force_calib.json (common/force_model.py, калибровка от 29.10 на большом пальце). Весов в самой
ramp-записи нет, поэтому это оценка: калибровка снималась другой процедурой.

Каждый цикл делится на ступени (кадры с одним процентом), берётся медиана ступени.
Метрики по способу и пальцу:
  шум       — σ кадр-к-кадру внутри ступеней, мкТл
  подъём    — от первой до последней ступени, мкТл (медиана по циклам)
  подъём/шум
  повтор.   — разброс между циклами на одной ступени / подъём, %
  R²        — линейность против силы по весам (по средним по циклам)

Запуск (из папки analysis):
  uv run python force_methods.py
  uv run python force_methods.py --file ../tools/data/ramp_XXXX.csv --no-show
Выход: <имя>_methods.png рядом со скриптом.
"""
import argparse
import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from force_model import DEFAULT_PATH, ForceModel
from mag_features import CELLS, ema, gradients, to_common

METHODS = {'sides': '(|B3−B0| + |B2−B1|)/2', 'diag': '(|B0−B1| + |B3−B2|)/2', 'press': '(Gx_x + Gy_y)/2'}


def method_values(common):
    b = [common[..., i, :] for i in range(4)]
    g = gradients(None, common=common)
    return {'sides': 0.5 * (np.linalg.norm(b[3] - b[0], axis=-1) + np.linalg.norm(b[2] - b[1], axis=-1)),
            'diag': 0.5 * (np.linalg.norm(b[0] - b[1], axis=-1) + np.linalg.norm(b[3] - b[2], axis=-1)),
            'press': g['normal']}


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', default=str(HERE.parent / 'tools' / 'data' / 'ramp_20261026_170237.csv'))
    p.add_argument('--calib', default=DEFAULT_PATH, help='калибровка «% -> сила по весам»')
    p.add_argument('--skip', type=float, default=0.3, help='отбросить начало каждой ступени, с (0.3)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    with open(a.file, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    d = np.array([[float(v) for v in r] for r in rows[1:]])
    t, pct, mag = d[:, 0], d[:, 1], d[:, 2:38]
    vals = method_values(ema(to_common(mag)))
    fm = ForceModel.load(a.calib)
    print(f'{Path(a.file).name}: кадров {len(t)}; сила по весам из {Path(a.calib).name} ({fm.source})')

    grip = pct > 0
    starts = np.where(grip & ~np.r_[False, grip[:-1]])[0]
    levels = np.unique(pct[grip]).astype(int)
    real = np.array([fm.force(L) for L in levels])
    # медианы ступеней: [метод][цикл, ступень, палец]; шум — σ первых разностей внутри ступеней
    med = {m: np.full((len(starts), len(levels), 3), np.nan) for m in METHODS}
    noise = {m: [] for m in METHODS}
    for k, s0 in enumerate(starts):
        e = starts[k + 1] if k + 1 < len(starts) else len(t)
        before = np.arange(max(0, s0 - 25), s0 - 3)              # раскрытая кисть перед циклом
        for m in METHODS:
            zero = np.median(vals[m][before], axis=0)
            for j, L in enumerate(levels):
                sel = np.arange(s0, e)[pct[s0:e] == L]
                if len(sel) == 0:
                    continue
                sel = sel[t[sel] >= t[sel[0]] + a.skip]
                if len(sel) < 3:
                    continue
                v = vals[m][sel] - zero
                med[m][k, j] = np.median(v, axis=0)
                dv = np.diff(v, axis=0)
                noise[m].append(1.4826 * np.median(np.abs(dv - np.median(dv, axis=0)), axis=0) / np.sqrt(2))

    fig, axs = plt.subplots(len(METHODS), 3, figsize=(16, 4 * len(METHODS)), squeeze=False)
    print(f'{"":8}{"":7}{"шум":>8}{"подъём":>9}{"подъём/шум":>12}{"повтор.":>9}{"R² (весы)":>11}{"R² >55%":>9}')
    for i, m in enumerate(METHODS):
        nz = np.nanmedian(np.array(noise[m]), axis=0)
        for c, n in enumerate(CELLS):
            M = med[m][:, :, c]
            rise = np.nanmedian(M[:, -1] - M[:, 0])
            spread = np.nanmedian(np.nanstd(M, axis=0)) / max(abs(rise), 1e-9) * 100
            have = ~np.all(np.isnan(M), axis=0)
            mean = np.where(have, np.nanmean(np.where(have, M, 0.0), axis=0), np.nan)

            def r2(sel):
                sel = sel & ~np.isnan(mean)
                x, y = real[sel], mean[sel]
                k_, b_ = np.polyfit(x, y, 1)
                return 1 - np.sum((y - k_ * x - b_) ** 2) / max(np.sum((y - y.mean()) ** 2), 1e-12)

            print(f'{m:8}{n:7}{nz[c]:8.2f}{rise:9.1f}{abs(rise) / nz[c]:12.0f}{spread:8.1f}%'
                  f'{r2(np.ones(len(levels), bool)):11.3f}{r2(levels > 55):9.3f}')
            ax = axs[i, c]
            for k in range(len(starts)):
                ax.plot(real, M[k], lw=0.8, alpha=0.7, label=f'цикл {k + 1}')
            ax.plot(real, mean, color='k', lw=1.8, label='среднее')
            ax.set_title(f'{n}: {m} = {METHODS[m]}', fontsize=10)
            ax.set_xlabel('сила по весам (калибровка), ед. весов', fontsize=8)
            ax.set_ylabel('мкТл', fontsize=8)
            ax.grid(alpha=0.3)
            ax.tick_params(labelsize=7)
            if c == 0 and i == 0:
                ax.legend(fontsize=7)
    fig.suptitle(f'{Path(a.file).name}: способы оценки силы по магнитометрам против силы по весам', fontsize=11)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_methods.png'
    fig.savefig(png, dpi=100)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
