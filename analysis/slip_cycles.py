"""
Запись slip_table.py --slide-cycles: все циклы в одном виде, выровненные по сигналу «СКОЛЬЗИТЬ» (t = 0).

Цикл: хват до касания → пауза (фаза ref, эталон G) → СКОЛЬЗИТЬ → пауза → открыть.
По каждому циклу и пальцу: r = |ΔG|/знаменатель (эталон G(F) — фаза ref этого цикла), dk — рост r
за --win-ms мс. Знаменатель (--denom):
  cur    текущая F = (|B3−B0| + |B2−B1|)/2 (минус раскрытая кисть)
  hold   F хвата — медиана F в паузе после закрытия (фаза ref), константа на цикл
  press  текущее прижатие |(Gx_x + Gy_y)/2| (минус раскрытая кисть) — меньше смешано со сдвигом
  none   без деления: r = |ΔG|, мкТл
Знаменатель не меньше --floor (по умолчанию 20 мкТл для cur/hold, 5 мкТл для press).

График (строки — r, dk, F; столбцы — пальцы): все циклы наложены (синие — вправо, красные — влево),
чёрная — медиана; серая полоса — окно скольжения.
Таблица по циклам: F хвата, пик r и пик dk в паузе ДО скольжения и в окне скольжения (+--after с),
по лучшему пальцу. Порог dk — середина между максимумом пауз и минимумом скольжений (если разделимы,
иначе лучший по точности); для каждого цикла — задержка: когда dk впервые превысил порог после сигнала.

Запуск (из папки analysis):
  uv run python slip_cycles.py --file ../tools/data/slipt_<время>.csv
  uv run python slip_cycles.py --file ../tools/data/slipt_<время>.csv --denom hold
Выход: <имя>_cycles_<denom>.png рядом со скриптом.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, RefTable, ema, feature_set, to_common
from slip_diff import auc, best_threshold
from slip_table_analysis import load

DIR_COL = {'slide_right': '#2171b5', 'slide_left': '#cb181d'}
DIR_NAME = {'slide_right': 'вправо', 'slide_left': 'влево'}


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True)
    p.add_argument('--denom', choices=['none', 'cur', 'hold', 'press'], default='cur', help='знаменатель (cur)')
    p.add_argument('--floor', type=float, help='знаменатель не меньше, мкТл (20; для press — 5)')
    p.add_argument('--f-valid', type=float, default=10.0, help='палец без контакта: F хвата меньше, мкТл (10)')
    p.add_argument('--win-ms', type=float, default=100.0, help='рост за столько мс (100)')
    p.add_argument('--after', type=float, default=1.0, help='окно скольжения: + столько с после конца шага (1)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    floor = a.floor if a.floor is not None else (5.0 if a.denom == 'press' else 20.0)
    DEN = {'cur': 'F', 'hold': 'F хвата', 'press': 'прижатие', 'none': ''}[a.denom]
    RN = '|ΔG|' + (f'/{DEN}' if DEN else ', мкТл')

    d = load(a.file)
    F_all, G_all = feature_set(ema(to_common(d['mag'])))
    cyc = []
    for L in np.unique(d['level']):
        lv = d['level'] == L
        slide = lv & np.char.startswith(d['label'].astype(str), 'slide') & (d['phase'] == 'drag')
        if not slide.any():
            continue
        noise = lv & (d['phase'] == 'noise')
        F = F_all - np.median(F_all[noise], axis=0)
        G = {k: v - np.median(v[noise], axis=0) for k, v in G_all.items()}
        ref_m = lv & (d['phase'] == 'ref')
        ref = RefTable(F[ref_m], {k: v[ref_m] for k, v in G.items()})
        use = lv & np.isin(d['phase'], ['ref', 'drag'])
        _, dsh = ref.delta(F[use], {k: v[use] for k, v in G.items()})
        t_cue = d['t'][slide][0] - d['cue'][slide][0]           # момент сигнала «СКОЛЬЗИТЬ»
        t = d['t'][use] - t_cue
        if a.denom == 'none':
            den = np.ones_like(F[use])
        elif a.denom == 'cur':
            den = F[use]
        elif a.denom == 'hold':
            den = np.broadcast_to(np.median(F[ref_m], axis=0), F[use].shape)
        else:
            den = np.abs((G['gxx'][use] + G['gyy'][use]) / 2)
        r = dsh if a.denom == 'none' else dsh / np.maximum(den, floor)
        dt = np.median(np.diff(t))
        k = max(1, int(round(a.win_ms / 1000 / dt)))
        dk = np.zeros_like(r)
        dk[k:] = r[k:] - r[:-k]
        slide_len = d['cue'][slide].max()
        cyc.append({'no': int(L), 'dir': d['label'][slide][0], 't': t, 'r': r, 'dk': dk, 'F': F[use],
                    'den': np.asarray(den, float),
                    'F_hold': np.median(F[ref_m], axis=0), 'slide_len': slide_len,
                    'valid': np.median(F[ref_m], axis=0) >= a.f_valid})

    # пики: пауза до скольжения (t < 0) и окно скольжения [0, длина + after]
    for c in cyc:
        v = c['valid']
        pre = c['t'] < 0
        win = (c['t'] >= 0) & (c['t'] <= c['slide_len'] + a.after)
        c['pre_dk'] = c['dk'][pre][:, v].max() if pre.any() else np.nan
        c['sl_dk'] = c['dk'][win][:, v].max()
        c['pre_r'] = c['r'][pre][:, v].max() if pre.any() else np.nan
        c['sl_r'] = c['r'][win][:, v].max()
        c['best'] = CELLS[np.flatnonzero(v)[int(np.argmax(c['dk'][win][:, v].max(axis=0)))]]
    pre_all = np.array([c['pre_dk'] for c in cyc])
    sl_all = np.array([c['sl_dk'] for c in cyc])
    if np.nanmax(pre_all) < sl_all.min():
        th = (np.nanmax(pre_all) + sl_all.min()) / 2
        how = 'разделимы: порог посередине'
    else:
        th, _, _ = best_threshold(sl_all, pre_all[~np.isnan(pre_all)])
        how = 'не разделимы полностью: лучший порог'
    for c in cyc:
        v = c['valid']
        after = (c['t'] >= 0) & (c['dk'][:, v].max(axis=1) >= th)
        c['lat'] = c['t'][after][0] if after.any() else np.nan

    # уровень r: пауза против скольжения
    pre_r = np.array([c['pre_r'] for c in cyc])
    sl_r = np.array([c['sl_r'] for c in cyc])
    if np.nanmax(pre_r) < sl_r.min():
        th_r = (np.nanmax(pre_r) + sl_r.min()) / 2
    else:
        th_r, _, _ = best_threshold(sl_r, pre_r[~np.isnan(pre_r)])
    lat_r = []
    for c in cyc:
        hit = (c['t'] >= 0) & (c['r'][:, c['valid']].max(axis=1) >= th_r)
        lat_r.append(c['t'][hit][0] if hit.any() else np.nan)
    print(f'{Path(a.file).name}: ' + (f'знаменатель {DEN} (не меньше {floor:.0f} мкТл)' if DEN else 'без деления')
          + f'; циклов {len(cyc)}')
    print(f'  уровень r = {RN}: AUC {auc(sl_r, pre_r[~np.isnan(pre_r)]):.2f}, порог {th_r:.3f}: найдено '
          f'{np.isfinite(lat_r).sum()}/{len(cyc)}, ложных в паузе {np.sum(pre_r >= th_r)}/{len(cyc)}, '
          f'задержка медиана {np.nanmedian(lat_r):.2f} с')
    print(f'  рост r за {a.win_ms:.0f} мс: AUC {auc(sl_all, pre_all[~np.isnan(pre_all)]):.2f}, порог {th:.3f} ({how})')
    print(f'  {"цикл":>4} {"напр.":7} {"F хвата THU/IND/MID, мкТл":>27} {"пик r: пауза→скольж.":>22} '
          f'{"пик dk: пауза→скольж.":>23} {"палец":>7} {"задержка, с":>12}')
    for c in cyc:
        print(f'  {c["no"]:4d} {DIR_NAME[c["dir"]]:7} {"/".join(f"{v:.0f}" for v in c["F_hold"]):>27} '
              f'{c["pre_r"]:9.2f} → {c["sl_r"]:5.2f}      {c["pre_dk"]:9.3f} → {c["sl_dk"]:5.3f}  {c["best"]:>7} '
              f'{c["lat"]:12.2f}')
    found = np.isfinite([c['lat'] for c in cyc])
    false_pre = np.sum(pre_all >= th)
    print(f'  найдено скольжений {found.sum()}/{len(cyc)}, ложных срабатываний в паузе до скольжения {false_pre}/{len(cyc)}; '
          f'задержка от сигнала: медиана {np.nanmedian([c["lat"] for c in cyc]):.2f} с '
          f'(включает реакцию человека на сигнал)')

    fig, axs = plt.subplots(3, 3, figsize=(20, 12), sharex=True)
    t_grid = np.arange(-3.0, max(c['slide_len'] for c in cyc) + 3.0, 0.02)
    for col, n in enumerate(CELLS):
        for row, (key, title) in enumerate([('r', RN), ('dk', f'рост {RN} за {a.win_ms:.0f} мс'),
                                            ('F', 'F, мкТл (для справки)') if not DEN else
                                            ('den', f'знаменатель: {DEN}, мкТл')]):
            ax = axs[row, col]
            stack = []
            for c in cyc:
                if not c['valid'][col]:
                    continue
                y = c[key][:, col]
                ax.plot(c['t'], y, lw=0.7, alpha=0.55, color=DIR_COL[c['dir']])
                stack.append(np.interp(t_grid, c['t'], y, left=np.nan, right=np.nan))
            if stack:
                ax.plot(t_grid, np.nanmedian(np.array(stack), axis=0), color='k', lw=1.8, label='медиана')
            ax.axvspan(0, np.median([c['slide_len'] for c in cyc]), color='#fdae6b', alpha=0.25, label='скольжение')
            ax.axvline(0, color='k', lw=0.8)
            if key == 'dk':
                ax.axhline(th, color='k', ls='--', lw=1, label=f'порог {th:.2f}')
            ax.set_title(f'{n}: {title}' + ('' if any(c['valid'][col] for c in cyc) else ' — нет контакта'),
                         fontsize=10)
            ax.grid(alpha=0.3)
            ax.tick_params(labelsize=7)
            if row == 2:
                ax.set_xlabel('время от сигнала «СКОЛЬЗИТЬ», с', fontsize=8)
    axs[0, 0].plot([], [], color=DIR_COL['slide_right'], label='вправо')
    axs[0, 0].plot([], [], color=DIR_COL['slide_left'], label='влево')
    axs[0, 0].legend(fontsize=8, loc='upper left')
    axs[1, 0].legend(fontsize=8, loc='upper left')
    fig.suptitle(f'{Path(a.file).name}: {len(cyc)} циклов, выровнены по сигналу «СКОЛЬЗИТЬ»; '
                 + (f'знаменатель — {DEN}' if DEN else 'без деления'), fontsize=11)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_cycles_{a.denom}.png'
    fig.savefig(png, dpi=95)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
