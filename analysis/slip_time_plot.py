"""
Записи slip_table.py во времени: по каждой силе и пальцу — F, |ΔG| и |ΔG|/F, шаги сценария подсвечены.
Расчёт тот же, что в slip_table_analysis.py (эталон G(F) — фазы ramp и ref того же хвата).

Запуск (из папки analysis):
  uv run python slip_time_plot.py --file ../tools/data/slipt_<время>.csv --levels 1,2
Выход: <имя>_time.png рядом со скриптом.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, RefTable, ema, feature_set, to_common
from slip_table_analysis import COLORS, NAMES, ORDER, load


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True)
    p.add_argument('--levels', type=lambda v: [int(x) for x in v.split(',')], help='номера сил, например 1,2')
    p.add_argument('--f-min', type=float, default=20.0, help='|ΔG|/F: F не меньше, мкТл (20)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    d = load(a.file)
    F_all, G_all = feature_set(ema(to_common(d['mag'])))
    levels = [L for L in np.unique(d['level']) if np.any((d['level'] == L) & (d['phase'] == 'drag'))]
    if a.levels:
        levels = [L for L in levels if L in a.levels]

    fig, axs = plt.subplots(3 * len(levels), 3, figsize=(20, 3.0 * 3 * len(levels)), squeeze=False,
                            sharex='row')
    for li, L in enumerate(levels):
        lv = d['level'] == L
        noise = lv & (d['phase'] == 'noise')
        F = F_all - np.median(F_all[noise], axis=0)
        G = {k: v - np.median(v[noise], axis=0) for k, v in G_all.items()}
        ref_m = lv & np.isin(d['phase'], ['ramp', 'ref'])
        ref = RefTable(F[ref_m], {k: v[ref_m] for k, v in G.items()})
        drag = lv & (d['phase'] == 'drag')
        _, dsh = ref.delta(F[drag], {k: v[drag] for k, v in G.items()})
        t, lab = d['t'][drag], d['label'][drag]
        t = t - t[0]
        Fd = F[drag]
        ratio = dsh / np.maximum(Fd, a.f_min)
        force = int(d['raw'][drag][0])
        labels = [lb for lb in ORDER if np.any(lab == lb) and lb != 'rest']
        for c, n in enumerate(CELLS):
            for r, (y, title, color) in enumerate([(Fd[:, c], 'F, мкТл', 'k'),
                                                   (dsh[:, c], '|ΔG| (касательная нагрузка), мкТл', '#1f77b4'),
                                                   (ratio[:, c], '|ΔG| / F', '#d62728')]):
                ax = axs[3 * li + r, c]
                top = np.percentile(y, 99.5) * 1.1 if r else y.max() * 1.05
                bot = 0 if r else min(0, y.min())
                for lb in labels:
                    ax.fill_between(t, bot, top, where=lab == lb, color=COLORS[lb], alpha=0.25, step='mid',
                                    label=NAMES[lb])
                ax.plot(t, y, lw=0.9, color=color)
                ax.set_ylim(bot, top)
                ax.set_title(f'сила {force}% · {n} · {title}', fontsize=9)
                ax.grid(alpha=0.3)
                ax.tick_params(labelsize=7)
                if r == 2:
                    ax.set_xlabel('время от начала вождения, с', fontsize=8)
                if c == 0 and r == 0:
                    ax.legend(fontsize=7, loc='upper left', ncol=4)
    fig.suptitle(f'{Path(a.file).name}: F, |ΔG| и |ΔG|/F во времени (серые промежутки — покой)', fontsize=11)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_time.png'
    fig.savefig(png, dpi=95)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
