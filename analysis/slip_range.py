"""
Скользящий размах |ΔG|/F: max − min в окне (по умолчанию 400, 500 и 700 мс), по пальцам.

|ΔG|/F — как в slip_time_plot.py / slip_diff.py. Окно — последние N мс до текущего кадра (причинное,
как работало бы в реальном времени). Пальцы с F хвата < --f-valid мкТл (нет контакта) не учитываются.
По эпизодам (шаг сценария, от начала до конца + --after с): максимум размаха; AUC скольжений
против «тянуть» и покоя и лучший порог.

Запуск (из папки analysis):
  uv run python slip_range.py --files ../tools/data/slipt_20261102_153704.csv ../tools/data/slipt_20261102_154601.csv
Выход: slip_range.png рядом со скриптом.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.stride_tricks import sliding_window_view

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS
from slip_diff import KCOL, KIND, auc, best_threshold, episodes, ratio_series


def rolling_range(r, n):
    """max − min за последние n кадров (первые n−1 кадров — по тому, что есть)."""
    out = np.zeros_like(r)
    w = sliding_window_view(r, n, axis=0)                  # (N−n+1, 3, n)
    out[n - 1:] = w.max(axis=-1) - w.min(axis=-1)
    for i in range(min(n - 1, len(r))):
        out[i] = r[:i + 1].max(axis=0) - r[:i + 1].min(axis=0)
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--files', nargs='+', required=True)
    p.add_argument('--windows', default='400,500,700', help='окна, мс (400,500,700)')
    p.add_argument('--f-min', type=float, default=20.0, help='|ΔG|/F: F не меньше, мкТл (20)')
    p.add_argument('--f-valid', type=float, default=10.0, help='палец без контакта: F хвата меньше, мкТл (10)')
    p.add_argument('--after', type=float, default=1.0, help='эпизод: + столько с после конца шага (1)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    wins = [int(x) for x in a.windows.split(',') if x.strip()]

    runs = [r for f in a.files for r in ratio_series(f, a.f_min)]
    fig, axs = plt.subplots(len(runs) * len(wins), 1, figsize=(20, 3.0 * len(runs) * len(wins)), squeeze=False)
    fig2, axs2 = plt.subplots(1, len(wins), figsize=(6 * len(wins), 4.5), squeeze=False)
    print('Размах max − min |ΔG|/F; по эпизодам — максимум за шаг (+{:.0f} с), лучший палец'.format(a.after))
    for wi, ms in enumerate(wins):
        pos, neg, pts = [], [], {k: [] for k in KCOL}
        lines = []
        rngs = []
        for run in runs:
            t = run['t']
            n = max(2, int(round(ms / 1000 / np.median(np.diff(t)))))
            valid = run['F_hold'] >= a.f_valid
            rg = rolling_range(run['r'], n)
            rngs.append(rg)
            v = {k: [] for k in KCOL}
            for lb, i0, i1 in episodes(t, run['lab'], run['cue'], a.after):
                kn = KIND.get(lb)
                if kn and i1 - i0 >= 3:
                    v[kn].append(float(rg[i0:i1][:, valid].max()))
            s, ng = np.array(v['скольжение']), np.r_[v['тянуть'], v['покой']]
            pos += list(s)
            neg += list(ng)
            for k in KCOL:
                pts[k] += v[k]
            lines.append(f'{run["name"]}: скольжение {np.round(np.sort(s), 2)}  тянуть ≤ {max(v["тянуть"], default=float("nan")):.2f}  '
                         f'покой ≤ {max(v["покой"], default=float("nan")):.2f}  AUC {auc(s, ng):.2f}')
        th, tp, tn = best_threshold(np.array(pos), np.array(neg))
        print(f'  окно {ms} мс: вместе AUC {auc(pos, neg):.2f}; порог {th:.3f}: скольжений найдено {tp * 100:.0f}%, '
              f'«не скольжений» верно {tn * 100:.0f}%')
        for ln in lines:
            print('      ' + ln)

        ax = axs2[0, wi]
        for x, kn in enumerate(KCOL):
            ax.scatter(np.full(len(pts[kn]), x) + np.random.uniform(-0.12, 0.12, len(pts[kn])), pts[kn],
                       color=KCOL[kn], s=40, edgecolor='k')
        ax.axhline(th, color='k', ls='--', lw=1, label=f'порог {th:.2f}')
        ax.set_xticks(range(len(KCOL)), list(KCOL))
        ax.set_title(f'окно {ms} мс: максимум размаха за эпизод', fontsize=10)
        ax.legend(fontsize=8)
        ax.grid(alpha=0.3)

        for ri, (run, rg) in enumerate(zip(runs, rngs)):
            ax = axs[ri * len(wins) + wi, 0]
            t = run['t']
            for c in np.flatnonzero(run['F_hold'] >= a.f_valid):
                ax.plot(t, rg[:, c], lw=0.9, label=CELLS[c])
            top = max(rg[:, run['F_hold'] >= a.f_valid].max() * 1.05, th * 1.3)
            for kn in ('тянуть', 'скольжение'):
                ax.fill_between(t, 0, top, where=run['kind'] if 'kind' in run else
                                np.array([KIND.get(x, '') for x in run['lab']]) == kn,
                                color=KCOL[kn], alpha=0.15, step='mid', label=kn)
            ax.axhline(th, color='k', ls='--', lw=1)
            ax.set_ylim(0, top)
            ax.set_title(f'{run["name"]}: max − min |ΔG|/F в окне {ms} мс (пунктир — порог)', fontsize=10)
            ax.legend(fontsize=8, ncol=5, loc='upper left')
            ax.grid(alpha=0.3)
            ax.tick_params(labelsize=7)
    axs[-1, 0].set_xlabel('время от начала вождения, с', fontsize=8)
    fig.tight_layout()
    fig2.tight_layout()
    png, png2 = HERE / 'slip_range.png', HERE / 'slip_range_episodes.png'
    fig.savefig(png, dpi=90)
    fig2.savefig(png2, dpi=100)
    print(f'Графики: {png}\n         {png2}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
