"""
Проскальзывание по резкому росту |ΔG|/F: распределения приращений и проверка по эпизодам.

r(t) = |ΔG|/F — как в slip_time_plot.py (эталон G(F) — фазы ramp/ref того же хвата; F не меньше --f-min).
Приращения:
  d1 = np.diff(r)              — между соседними отсчётами (~17 мс)
  dk = r(t) − r(t − k)         — за --win-ms мс (сглаживает шум кадра)
1. Распределения d1 и dk по состояниям (покой / тянуть / скольжение): медиана, p95, p99, доля > порога.
2. Эпизоды сценария (каждый шаг ТЯНУТЬ / СКОЛЬЗИТЬ и такие же по длине куски ПОКОЯ): в окне от начала
   шага до конца + --after с берётся max dk по пальцам. Скольжение должно давать большие max dk,
   тянуть и покой — малые. Численно: AUC (вероятность, что случайное скольжение > случайного «тянуть»)
   и лучший порог (максимум «верно скольжения» + «верно не-скольжения»).
Пальцы с F хвата < --f-valid мкТл (нет контакта) исключаются.

Запуск (из папки analysis):
  uv run python slip_diff.py --files ../tools/data/slipt_20261102_153704.csv ../tools/data/slipt_20261102_154601.csv
Выход: slip_diff.png рядом со скриптом.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, RefTable, ema, feature_set, to_common
from slip_table_analysis import load

KIND = {'rest': 'покой', 'hold_right': 'тянуть', 'hold_left': 'тянуть',
        'slide_right': 'скольжение', 'slide_left': 'скольжение'}
KCOL = {'покой': '#999999', 'тянуть': '#3182bd', 'скольжение': '#e6550d'}


def ratio_series(path, f_min, level=None):
    d = load(path)
    F_all, G_all = feature_set(ema(to_common(d['mag'])))
    levels = [L for L in np.unique(d['level']) if np.any((d['level'] == L) & (d['phase'] == 'drag'))]
    out = []
    for L in levels if level is None else [level]:
        lv = d['level'] == L
        noise = lv & (d['phase'] == 'noise')
        F = F_all - np.median(F_all[noise], axis=0)
        G = {k: v - np.median(v[noise], axis=0) for k, v in G_all.items()}
        ref_m = lv & np.isin(d['phase'], ['ramp', 'ref'])
        ref = RefTable(F[ref_m], {k: v[ref_m] for k, v in G.items()})
        drag = lv & (d['phase'] == 'drag')
        _, dsh = ref.delta(F[drag], {k: v[drag] for k, v in G.items()})
        out.append({'name': f'{Path(path).stem[-6:]}' + (f'#{L}' if len(levels) > 1 else ''),
                    't': d['t'][drag] - d['t'][drag][0], 'lab': d['label'][drag], 'cue': d['cue'][drag],
                    'r': dsh / np.maximum(F[drag], f_min), 'F_hold': np.median(F[ref_m], axis=0)})
    return out


def episodes(t, lab, cue, after_s):
    """Шаги сценария: (метка, i0, i1) — от начала шага до конца + after_s."""
    eps, start = [], 0
    for i in range(1, len(lab) + 1):
        if i == len(lab) or lab[i] != lab[i - 1] or cue[i] < cue[i - 1]:
            end_t = t[i - 1] + after_s
            i1 = int(np.searchsorted(t, end_t, side='right'))
            eps.append((lab[start], start, min(i1, len(t))))
            start = i
    return eps


def auc(pos, neg):
    pos, neg = np.asarray(pos), np.asarray(neg)
    if not len(pos) or not len(neg):
        return np.nan
    return float(np.mean(pos[:, None] > neg[None, :]) + 0.5 * np.mean(pos[:, None] == neg[None, :]))


def best_threshold(pos, neg):
    cand = np.unique(np.r_[pos, neg])
    best = (0, None, 0, 0)
    for th in cand:
        tp, tn = np.mean(pos >= th), np.mean(neg < th)
        if tp + tn > best[0]:
            best = (tp + tn, th, tp, tn)
    return best[1], best[2], best[3]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--files', nargs='+', required=True)
    p.add_argument('--f-min', type=float, default=20.0, help='|ΔG|/F: F не меньше, мкТл (20)')
    p.add_argument('--f-valid', type=float, default=10.0, help='палец без контакта: F хвата меньше, мкТл (10)')
    p.add_argument('--win-ms', type=float, default=100.0, help='приращение за столько мс (100)')
    p.add_argument('--after', type=float, default=1.0, help='эпизод: + столько с после конца шага (1)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    runs = [r for f in a.files for r in ratio_series(f, a.f_min)]
    frames = {k: {'d1': [], 'dk': []} for k in KCOL}
    ep_vals = {k: [] for k in KCOL}
    ep_rows = []
    for run in runs:
        t, r = run['t'], run['r']
        dt = np.median(np.diff(t))
        k = max(1, int(round(a.win_ms / 1000 / dt)))
        valid = run['F_hold'] >= a.f_valid
        d1 = np.vstack([np.zeros((1, 3)), np.diff(r, axis=0)])
        dk = np.zeros_like(r)
        dk[k:] = r[k:] - r[:-k]
        kind = np.array([KIND.get(x, '') for x in run['lab']])
        for kn in KCOL:
            s = kind == kn
            frames[kn]['d1'].append(d1[s][:, valid].ravel())
            frames[kn]['dk'].append(dk[s][:, valid].ravel())
        print(f'{run["name"]}: F хвата ' + ', '.join(f'{n} {v:.0f}' for n, v in zip(CELLS, run['F_hold']))
              + ' мкТл; в расчёт: ' + ', '.join(n for n, v in zip(CELLS, valid) if v)
              + f'; кадр {dt * 1000:.0f} мс, окно dk {k} кадров')
        for lb, i0, i1 in episodes(t, run['lab'], run['cue'], a.after):
            kn = KIND.get(lb)
            if not kn or i1 - i0 < 3:
                continue
            seg = dk[i0:i1][:, valid]
            m = float(seg.max())
            ep_vals[kn].append(m)
            ep_rows.append((run['name'], t[i0], lb, kn, m, CELLS[np.flatnonzero(valid)[int(np.argmax(seg.max(axis=0)))]]))
        run.update(dk=dk, valid=valid, kind=kind)

    print(f'\nРаспределение приращений r = |ΔG|/F по кадрам (пальцы с контактом):')
    print(f'  {"":12}{"d1 (соседние отсчёты)":^36}{f"dk (за {a.win_ms:.0f} мс)":^36}')
    print(f'  {"":12}' + f'{"медиана":>9}{"p95":>9}{"p99":>9}{"max":>9}' * 2)
    for kn in KCOL:
        if not sum(len(x) for x in frames[kn]['d1']):
            continue
        row = ''
        for key in ('d1', 'dk'):
            v = np.concatenate(frames[kn][key])
            row += f'{np.median(v):9.3f}{np.percentile(v, 95):9.3f}{np.percentile(v, 99):9.3f}{v.max():9.3f}'
        print(f'  {kn:12}{row}')

    print(f'\nЭпизоды: max dk (за {a.win_ms:.0f} мс) от начала шага до конца + {a.after} с, лучший палец')
    for name, t0, lb, kn, m, cell in ep_rows:
        if kn != 'покой':
            print(f'  {name:8} t={t0:5.1f} с  {lb:12} {m:6.3f}  ({cell})')
    rest_ep = [m for (_, _, lb, kn, m, _) in ep_rows if kn == 'покой']
    sl, ho = np.array(ep_vals['скольжение']), np.array(ep_vals['тянуть'])
    neg = np.r_[ho, rest_ep]
    print()
    for nm, v in (('скольжение', sl), ('тянуть', ho), ('покой', np.array(rest_ep))):
        if len(v):
            print(f'  {nm:11} n={len(v)}, медиана {np.median(v):.3f}, мин {v.min():.3f}, макс {v.max():.3f}')
    print(f'  AUC скольжение против тянуть: {auc(sl, ho):.2f};  против тянуть+покой: {auc(sl, neg):.2f}')
    th, tp, tn = best_threshold(sl, neg)
    print(f'  лучший порог max dk ≥ {th:.3f}: скольжений найдено {tp * 100:.0f}%, '
          f'«не скольжений» верно {tn * 100:.0f}%')

    # графики
    fig = plt.figure(figsize=(20, 4.2 * (len(runs) + 1)))
    gs = fig.add_gridspec(len(runs) + 1, 3)
    for j, key in enumerate(('d1', 'dk')):
        ax = fig.add_subplot(gs[0, j])
        allv = np.concatenate([np.concatenate(frames[kn][key]) for kn in KCOL])
        lo, hi = np.percentile(allv, [0.5, 99.9])
        bins = np.linspace(lo, hi, 80)
        for kn, c in KCOL.items():
            v = np.concatenate(frames[kn][key])
            if not len(v):
                continue
            ax.hist(v, bins=bins, density=True, histtype='step', lw=1.5, color=c, label=kn)
        ax.set_yscale('log')
        ax.set_title(('d1 = np.diff(|ΔG|/F), соседние отсчёты' if key == 'd1' else
                      f'dk = |ΔG|/F(t) − |ΔG|/F(t − {a.win_ms:.0f} мс)') + ', плотность (лог)', fontsize=10)
        ax.legend(fontsize=8)
        ax.grid(alpha=0.3)
    ax = fig.add_subplot(gs[0, 2])
    for x, kn in enumerate(KCOL):
        v = [m for (_, _, _, k2, m, _) in ep_rows if k2 == kn]
        ax.scatter(np.full(len(v), x) + np.random.uniform(-0.12, 0.12, len(v)), v, color=KCOL[kn], s=40,
                   edgecolor='k')
    ax.axhline(th, color='k', ls='--', lw=1, label=f'порог {th:.3f}')
    ax.set_xticks(range(len(KCOL)), list(KCOL))
    ax.set_title(f'эпизоды: max dk (за {a.win_ms:.0f} мс)', fontsize=10)
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3)
    for i, run in enumerate(runs):
        ax = fig.add_subplot(gs[i + 1, :])
        t = run['t']
        for c in np.flatnonzero(run['valid']):
            ax.plot(t, run['dk'][:, c], lw=0.8, label=CELLS[c])
        top = max(run['dk'][:, run['valid']].max() * 1.05, th * 1.5)
        for kn in ('тянуть', 'скольжение'):
            ax.fill_between(t, run['dk'][:, run['valid']].min(), top, where=run['kind'] == kn, color=KCOL[kn],
                            alpha=0.15, step='mid', label=kn)
        ax.axhline(th, color='k', ls='--', lw=1)
        ax.set_ylim(top=top)
        ax.set_title(f'{run["name"]}: dk во времени, пунктир — порог', fontsize=10)
        ax.set_xlabel('время от начала вождения, с', fontsize=8)
        ax.legend(fontsize=8, ncol=5, loc='upper left')
        ax.grid(alpha=0.3)
    fig.tight_layout()
    png = HERE / 'slip_diff.png'
    fig.savefig(png, dpi=95)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
