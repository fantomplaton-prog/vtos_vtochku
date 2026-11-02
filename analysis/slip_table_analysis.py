"""
Анализ записи slip_table.py (кисть держит край стола; шаги сценария: ПОКОЙ / ТЯНУТЬ, НЕ СДВИГАЯ /
СКОЛЬЗИТЬ, вправо и влево; несколько сил).

На каждую силу: эталон G(F) — из фаз ramp и ref этого же хвата (без касательной нагрузки);
в фазе drag ΔG = G − G_ref(F) (common/mag_features.py: RefTable). Обработка как в прошивке:
мкТл, EMA 0.6, ноль — кисть раскрыта перед хватом (фаза noise).

Признаки по пальцам:
  |ΔG|   — сдвиг магнита сверх того, что даёт сила: касательная НАГРУЗКА (есть и при трении покоя)
  ΔG x,y — направление нагрузки
  rate   — скорость изменения ΔG (по --rate-ms мс), мкТл/с: подушечка «едет»
  vib    — ВЧ-составляющая ΔG (выше --vib-hz Гц, СКЗ за --vib-ms мс), мкТл: дрожание при скольжении
  |ΔG|/F — касательная нагрузка относительно нормальной (F — сила пальца минус раскрытая кисть):
           безразмерный аналог «использованного трения» μ — сравним между силами
Гипотеза: ТЯНУТЬ даёт большой |ΔG| при малых rate/vib; СКОЛЬЗИТЬ — большой |ΔG| и большие rate/vib.

Метки ставит сценарий; в сводку и облака идут кадры шага без первых --lag с и последних --tail с
(реакция человека на сигнал).

График <имя>_slip.png, строка на силу:
  слева    — время: |ΔG| (сплошная) и vib (пунктир) по пальцам, шаги подсвечены;
  остальное — по пальцам: облако ΔG (x, y) и облако |ΔG|/F против vib, цвет — шаг.

Запуск (из папки analysis):
  uv run python slip_table_analysis.py --file ../tools/data/slipt_<время>.csv
"""
import argparse
import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, RefTable, ema, feature_set, to_common

ORDER = ['rest', 'hold_right', 'slide_right', 'hold_left', 'slide_left', 'fwd', 'back', 'left', 'right']
NAMES = {'rest': 'покой', 'hold_right': 'тянуть вправо', 'slide_right': 'скольз. вправо',
         'hold_left': 'тянуть влево', 'slide_left': 'скольз. влево',
         'fwd': 'вперёд', 'back': 'назад', 'left': 'влево', 'right': 'вправо'}
COLORS = {'rest': '#999999', 'hold_right': '#6baed6', 'slide_right': '#08519c',
          'hold_left': '#fd8d3c', 'slide_left': '#a63603',
          'fwd': '#1f77b4', 'back': '#d62728', 'left': '#2ca02c', 'right': '#ff7f0e'}


def load(path):
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    h = rows[0]
    col = {n: i for i, n in enumerate(h)}
    j = col['THUMB_MLX0_X']
    data = [r for r in rows[1:] if len(r) == len(h)]
    return {'t': np.array([float(r[col['t_s']]) for r in data]),
            'level': np.array([int(r[col['level']]) for r in data]),
            'raw': np.array([int(r[col['raw_pct']]) for r in data]),
            'phase': np.array([r[col['phase']] for r in data]),
            'label': np.array([r[col['label']] for r in data]),
            'cue': np.array([float(r[col['cue_s']]) if 'cue_s' in col and r[col['cue_s']] else np.inf
                             for r in data]),
            'mag': np.array([[float(v) for v in r[j:j + 36]] for r in data])}


def time_left(cue, lab):
    """Сколько секунд осталось до конца шага (шаг — подряд идущие кадры с одной меткой и растущим cue)."""
    out = np.full(len(cue), np.inf)
    start = 0
    for i in range(1, len(cue) + 1):
        if i == len(cue) or lab[i] != lab[i - 1] or cue[i] < cue[i - 1]:
            if np.isfinite(cue[i - 1]):
                out[start:i] = cue[i - 1] - cue[start:i]
            start = i
    return out


def dynamics(t, x, rate_ms, vib_hz, vib_ms):
    """x (N, 3, 2) — ΔG (x, y). -> rate (N,3) мкТл/с, vib (N,3) мкТл."""
    dt = np.median(np.diff(t)) if len(t) > 1 else 0.018
    dt = dt if dt > 0 else 0.018
    k = max(1, int(round(rate_ms / 1000 / dt)))
    rate = np.zeros(x.shape[:2])
    rate[k:] = np.linalg.norm(x[k:] - x[:-k], axis=-1) / (k * dt)
    alpha = 1 - np.exp(-2 * np.pi * vib_hz * dt)
    lp = ema(x, alpha)
    hp = np.linalg.norm(x - lp, axis=-1) ** 2
    n = max(1, int(round(vib_ms / 1000 / dt)))
    vib = np.sqrt(np.apply_along_axis(lambda v: np.convolve(v, np.ones(n) / n, 'same'), 0, hp))
    return rate, vib


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True)
    p.add_argument('--lag', type=float, default=0.8, help='отбросить начало шага, с (0.8)')
    p.add_argument('--tail', type=float, default=0.5, help='отбросить конец шага, с (0.5)')
    p.add_argument('--rate-ms', type=float, default=100.0, help='окно скорости, мс (100)')
    p.add_argument('--vib-hz', type=float, default=4.0, help='вибрация: выше этой частоты, Гц (4)')
    p.add_argument('--vib-ms', type=float, default=300.0, help='вибрация: окно СКЗ, мс (300)')
    p.add_argument('--f-min', type=float, default=20.0, help='|ΔG|/F: F не меньше, мкТл (20)')
    p.add_argument('--levels', type=lambda v: [int(x) for x in v.split(',')],
                   help='какие силы (номера по порядку) анализировать, например 1,2')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    d = load(a.file)
    F_all, G_all = feature_set(ema(to_common(d['mag'])))
    levels = [L for L in np.unique(d['level']) if np.any((d['level'] == L) & (d['phase'] == 'drag'))]
    if a.levels:
        levels = [L for L in levels if L in a.levels]
    if not levels:
        print('В записи нет фазы drag.')
        return

    fig, axs = plt.subplots(len(levels), 7, figsize=(30, 4.4 * len(levels)), squeeze=False,
                            gridspec_kw={'width_ratios': [2.6, 1, 1, 1, 1, 1, 1]})
    print(f'{Path(a.file).name}: кадров {len(d["t"])}, сил {len(levels)}; '
          f'в сводке шаг без первых {a.lag} с и последних {a.tail} с')
    for row, L in enumerate(levels):
        lv = d['level'] == L
        noise = lv & (d['phase'] == 'noise')
        F = F_all - np.median(F_all[noise], axis=0)
        G = {k: v - np.median(v[noise], axis=0) for k, v in G_all.items()}
        ref_m = lv & np.isin(d['phase'], ['ramp', 'ref'])
        ref = RefTable(F[ref_m], {k: v[ref_m] for k, v in G.items()})
        drag = lv & (d['phase'] == 'drag')
        dG, dsh = ref.delta(F[drag], {k: v[drag] for k, v in G.items()})
        t, lab, cue = d['t'][drag], d['label'][drag], d['cue'][drag]
        xy = np.stack([dG['shear_x'], dG['shear_y']], axis=-1)
        rate, vib = dynamics(t, xy, a.rate_ms, a.vib_hz, a.vib_ms)
        Fd = F[drag]
        ratio = dsh / np.maximum(Fd, a.f_min)
        settled = (cue >= a.lag) & (time_left(cue, lab) >= a.tail)
        lab_s = np.where(settled, lab, 'skip')
        labels = [lb for lb in ORDER if np.any(lab == lb)]
        force = int(d['raw'][drag][0])

        ax = axs[row, 0]
        for c, n in enumerate(CELLS):
            ax.plot(t, dsh[:, c], lw=1, color=f'C{c}', label=f'|ΔG| {n}')
        top = max(dsh.max() * 1.05, 1)
        for lb in labels:
            if lb != 'rest':
                ax.fill_between(t, 0, top, where=lab == lb, color=COLORS[lb], alpha=0.2, step='mid',
                                label=NAMES[lb])
        ax2 = ax.twinx()
        for c in range(3):
            ax2.plot(t, vib[:, c], lw=0.7, ls='--', color=f'C{c}')
        ax2.set_ylabel('vib, мкТл (пунктир)', fontsize=8)
        ax.set_ylim(0, top)
        ax.set_title(f'сила {force}%: |ΔG| (нагрузка) и vib (дрожание), мкТл', fontsize=10)
        ax.legend(fontsize=7, loc='upper left', ncol=4)
        ax.set_xlabel('время, с', fontsize=8)

        lim = max(np.percentile(np.abs(xy[settled]), 99), 5) if settled.any() else 5
        vmax = max(np.percentile(vib[settled], 99), 1) if settled.any() else 1
        rmax = max(np.percentile(ratio[settled], 99), 0.05) if settled.any() else 1
        for c, n in enumerate(CELLS):
            ax = axs[row, 1 + 2 * c]
            ax_v = axs[row, 2 + 2 * c]
            for lb in labels:
                s = lab_s == lb
                if not s.any():
                    continue
                ax.scatter(xy[s, c, 0], xy[s, c, 1], s=4, alpha=0.4, color=COLORS[lb], label=NAMES[lb])
                ax.scatter(*np.median(xy[s, c], axis=0), s=90, color=COLORS[lb], edgecolor='k', zorder=5)
                ax_v.scatter(ratio[s, c], vib[s, c], s=4, alpha=0.4, color=COLORS[lb])
                ax_v.scatter(np.median(ratio[s, c]), np.median(vib[s, c]), s=90, color=COLORS[lb],
                             edgecolor='k', zorder=5)
            ax.axhline(0, color='k', lw=0.5)
            ax.axvline(0, color='k', lw=0.5)
            ax.set_xlim(-lim, lim)
            ax.set_ylim(-lim, lim)
            ax.set_aspect('equal')
            ax.set_title(f'{n}: ΔG (x, y), мкТл', fontsize=10)
            ax_v.set_xlim(0, rmax * 1.1)
            ax_v.set_ylim(0, vmax)
            ax_v.set_title(f'{n}: |ΔG|/F против vib', fontsize=10)
            ax_v.set_xlabel('|ΔG|/F', fontsize=8)
            if c == 0:
                ax.legend(fontsize=7, loc='upper left', markerscale=3)

        print(f'  сила {force}% (F пальцев {", ".join(f"{v:.0f}" for v in np.median(Fd, axis=0))} мкТл):')
        print(f'  {"":17}{"|ΔG|, мкТл":^21}{"|ΔG|/F":^21}{"rate, мкТл/с":^21}{"vib, мкТл":^21}')
        print(f'  {"":17}' + ''.join(f'{n[:3]:>7}' for n in CELLS) * 4)
        for lb in labels:
            s = lab_s == lb
            if not s.any():
                continue
            cols = [np.median(dsh[s], axis=0), np.median(ratio[s], axis=0),
                    np.median(rate[s], axis=0), np.median(vib[s], axis=0)]
            print(f'    {NAMES[lb]:15}' + ''.join(''.join(f'{v:7.1f}' if k != 1 else f'{v:7.2f}' for v in x)
                                                for k, x in enumerate(cols)) + f'   n={s.sum()}')
    for ax in axs.ravel():
        ax.grid(alpha=0.3)
        ax.tick_params(labelsize=7)
    fig.suptitle(f'{Path(a.file).name}: покой / трение покоя / скольжение', fontsize=11)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_slip.png'
    fig.savefig(png, dpi=90)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
