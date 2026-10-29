"""
Зависимость «сила команды, % -> реальная сила по весам» по записи tools/experiments/force_calib.py.

Вход: tools/data/fcal_*_summary.csv (по замеру) и, если рядом, fcal_*.csv (все кадры — для
графика установления). Выход рядом со скриптом:
  <имя>_calib.json — уровни, средняя сила, разброс, монотонная кривая для пересчёта
                     (читает tools/common/force_model.py)
  <имя>_calib.png  — точки замеров, среднее ± σ, кривая; справа — вес во времени на удержании
  --install        — сделать калибровку рабочей: tools/common/force_calib.json (её подхватывают
                     HandController, hand_gui, combined_gui, force_calib.py --linear)

Запись, снятая с --linear (проверка линеаризации), анализируется так же, но вместо калибровки
выводится, насколько сила пропорциональна: отклонение от прямой «N % = N % от максимума».

Единицы: weight с HX711 как есть. --scale переводит в ньютоны (например, если весы в граммах:
--scale 0.00981 --unit Н). Без --scale — единицы весов.

Запуск (из папки analysis):
  uv run python force_calib.py --file ../tools/data/fcal_XXXX_summary.csv
  uv run python force_calib.py --file ../tools/data/fcal_XXXX_summary.csv --scale 0.00981 --unit Н
  uv run python force_calib.py --file ../tools/data/fcal_XXXX_summary.csv --install
"""
import argparse
import csv
import json
import shutil
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
INSTALL_PATH = HERE.parent / 'tools' / 'common' / 'force_calib.json'
sys.path.insert(0, str(INSTALL_PATH.parent))
from force_model import ForceModel


def read_csv(path):
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.DictReader(fh))
    return rows


def robust_tail(t, w):
    """Разброс и наклон окна, устойчивые к одиночным сбоям HX711 (скачки на ±десятки тысяч):
    σ = 1.4826·MAD, наклон = разность медиан второй и первой половины / половина длительности."""
    t, w = np.asarray(t, float), np.asarray(w, float)
    sd = 1.4826 * float(np.median(np.abs(w - np.median(w))))
    h = len(w) // 2
    dt = (t[-1] - t[0]) / 2
    slope = float((np.median(w[h:]) - np.median(w[:h])) / dt) if h >= 2 and dt > 0 else 0.0
    return sd, slope


def isotonic(y, w):
    """Неубывающая аппроксимация (pool adjacent violators) — кривая для обратного пересчёта."""
    blocks = [[float(v), float(wi), 1] for v, wi in zip(y, w)]
    i = 0
    while i < len(blocks) - 1:
        if blocks[i][0] > blocks[i + 1][0]:
            v0, w0, n0 = blocks[i]
            v1, w1, n1 = blocks.pop(i + 1)
            blocks[i] = [(v0 * w0 + v1 * w1) / (w0 + w1), w0 + w1, n0 + n1]
            i = max(i - 1, 0)
        else:
            i += 1
    return np.concatenate([[v] * n for v, _, n in blocks])


def report_linear(levels, mean, sd, unit):
    """Проверка линеаризации: насколько средние силы ложатся на прямую сила = a + b·L.
    Ошибка — в % от диапазона сил по прямой (от L_min до L_max)."""
    b, a0 = np.polyfit(levels, mean, 1)
    ideal = a0 + b * levels
    span = abs(b) * (levels.max() - levels.min())
    err = (mean - ideal) / max(span, 1e-12) * 100
    r2 = 1 - np.sum((mean - ideal) ** 2) / max(np.sum((mean - mean.mean()) ** 2), 1e-12)
    print(f'Проверка линейности: сила ≈ {a0:.1f} + {b:.2f}·L ({unit})')
    print('  линейный %   сила      прямая    ошибка, % диапазона   σ')
    for L, m, i_, e, s_ in zip(levels, mean, ideal, err, sd):
        print(f'  {int(L):3d}       {m:9.2f}  {i_:9.2f}   {e:+7.1f}              {s_:.2f}')
    print(f'  макс. |ошибка| {np.abs(err).max():.1f}% диапазона, R² {r2:.4f}; '
          f'сила при 0 % по прямой — {a0 / (a0 + 100 * b) * 100:.0f}% от силы при 100 %')
    return ideal


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True, help='fcal_*_summary.csv')
    p.add_argument('--scale', type=float, default=1.0, help='множитель единиц весов -> --unit (1)')
    p.add_argument('--unit', default='ед. весов', help='название единиц после --scale')
    p.add_argument('--mode', choices=['range', 'proportional'], default='range',
                   help='range — прямая от мин. до макс. силы (по умолчанию); proportional — через ноль')
    p.add_argument('--install', action='store_true', help='сделать калибровку рабочей (tools/common)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    path = Path(a.file)
    rows = read_csv(path)
    linear = rows[0].get('mode') == 'linear'
    trace_path = path.with_name(path.name.replace('_summary', ''))
    trace = read_csv(trace_path) if trace_path.exists() else None
    if trace:
        # разброс и наклон заново, устойчиво к сбоям HX711 (в старых записях считались по σ и МНК)
        hold_by_trial = {}
        for r in trace:
            if r['phase'] == 'hold':
                hold_by_trial.setdefault(r['trial'], []).append((float(r['t_s']), float(r['weight'])))
        for r in rows:
            h = hold_by_trial.get(r['trial'], [])
            n_tail = int(r['n_tail'])
            if len(h) >= n_tail >= 4:
                tt, ww = zip(*h[-n_tail:])
                sd, sl = robust_tail(tt, ww)
                r['tail_std'], r['tail_slope_per_s'] = sd, sl
    lvl = np.array([float(r['level']) for r in rows])
    force = np.array([float(r['force']) for r in rows]) * a.scale
    tstd = np.array([float(r['tail_std']) for r in rows]) * a.scale
    slope = np.array([float(r['tail_slope_per_s']) for r in rows]) * a.scale

    levels = np.unique(lvl)
    mean = np.array([force[lvl == L].mean() for L in levels])
    sd = np.array([force[lvl == L].std(ddof=1) if (lvl == L).sum() > 1 else 0.0 for L in levels])
    n = np.array([(lvl == L).sum() for L in levels])
    mono = isotonic(mean, n)

    print(f'{path.name}: замеров {len(rows)}, уровней {len(levels)}; сила в {a.unit}')
    print('  %      среднее      σ     σ/ср    n   (кривая)')
    for L, m, s, k, c in zip(levels, mean, sd, n, mono):
        rel = f'{s / abs(m) * 100:5.1f}%' if abs(m) > 1e-9 else '   --'
        print(f'  {int(L):3d}  {m:10.2f}  {s:8.2f}  {rel}  {k:3d}   {c:10.2f}')
    if np.any(np.diff(mean) <= 0):
        bad = [int(levels[i + 1]) for i in np.where(np.diff(mean) <= 0)[0]]
        print(f'  ВНИМАНИЕ: сила не растёт на уровнях {bad} — команда % там не различает силу '
              f'(насыщение или разброс больше шага).')
    # показание не установилось: за окно усреднения (~0.8 с) ушло больше 2σ шума и больше 2 % силы
    # (медленная релаксация ~1 %/с есть всегда — её не считаем)
    unsettled = np.abs(slope) * 0.8 > np.maximum(2 * tstd, 0.02 * np.abs(force))
    if unsettled.any():
        print(f'  ВНИМАНИЕ: в {unsettled.sum()} замерах вес ещё менялся в конце удержания '
              f'(уровни {sorted(set(int(x) for x in lvl[unsettled]))}) — увеличьте --hold-s.')
    if len(levels) >= 2:
        k, b = np.polyfit(lvl, force, 1)
        r2 = 1 - np.sum((force - (k * lvl + b)) ** 2) / max(np.sum((force - force.mean()) ** 2), 1e-12)
        print(f'  линейно: сила ≈ {k:.3f}·% {b:+.2f}  (R² = {r2:.3f})')

    line = None
    if linear:
        line = report_linear(levels, mean, sd, a.unit)
        if a.install:
            print('  --install пропущен: это проверочная запись (--linear), калибровку снимают в обычном режиме')
    else:
        fm = ForceModel(levels, mono, mode=a.mode)
        print(f'Линеаризация ({a.mode}): сила от {fm.f_min:.2f} до {fm.f_max:.2f} {a.unit} '
              f'(минимум = {100 * fm.f_min / fm.f_max:.0f}% максимума, сырой {fm.pct(fm.f_max):.0f}% даёт максимум)')
        if a.mode == 'proportional':
            print(f'  ниже {fm.min_linear:.0f}% линейных кисть меньшей силы не даёт')
        print('  линейный %  -> сырой %:  ' + '  '.join(f'{L}->{fm.raw_pct(L)}' for L in range(10, 101, 10)))

    calib = {'source': path.name, 'unit': a.unit, 'scale': a.scale,
             'levels_pct': levels.tolist(), 'force_mean': mean.tolist(), 'force_std': sd.tolist(),
             'n': n.tolist(), 'force_monotone': mono.tolist(), 'mode': a.mode}
    out_json = HERE / f'{path.stem.replace("_summary", "")}_calib.json'
    out_json.write_text(json.dumps(calib, ensure_ascii=False, indent=1), encoding='utf-8')
    print(f'Калибровка: {out_json}')
    if a.install and not linear:
        if INSTALL_PATH.exists():
            print(f'  прежняя рабочая калибровка: {json.loads(INSTALL_PATH.read_text(encoding="utf-8"))["source"]}')
        shutil.copyfile(out_json, INSTALL_PATH)
        print(f'Установлена как рабочая: {INSTALL_PATH}')

    # --- графики
    fig, axs = plt.subplots(1, 2 if trace else 1, figsize=(14, 6), squeeze=False)
    ax = axs[0, 0]
    ax.scatter(lvl, force, s=18, alpha=0.6, label='замеры')
    ax.errorbar(levels, mean, yerr=sd, fmt='o', color='k', capsize=4, label='среднее ± σ')
    if linear:
        ax.plot(levels, line, color='#2ca02c', lw=1.5, ls='--', label='прямая (МНК)')
        ax.set_xlabel('линейная сила, % от максимума')
    else:
        ax.plot(levels, mono, color='#d62728', lw=1.5, label='кривая пересчёта')
        ax.set_xlabel('сырая сила команды, %')
    ax.set_ylabel(f'сила по весам, {a.unit}')
    ax.grid(alpha=0.3)
    ax.legend()
    ax.set_title(path.name, fontsize=10)

    if trace:
        tr = trace
        ax2 = axs[0, 1]
        cmap = plt.get_cmap('viridis')
        by_trial = {}
        for r in tr:
            by_trial.setdefault(int(r['trial']), []).append(r)
        for trial, rs in by_trial.items():
            hold = [r for r in rs if r['phase'] == 'hold']
            opn = [r for r in rs if r['phase'] == 'open']
            if not hold or not opn:
                continue
            t0 = float(hold[0]['t_s'])
            zero = np.median([float(r['weight']) for r in opn[-15:]])
            L = float(hold[0]['level'])
            ax2.plot([float(r['t_s']) - t0 for r in hold],
                     [(float(r['weight']) - zero) * a.scale for r in hold],
                     color=cmap((L - levels.min()) / max(np.ptp(levels), 1)), lw=0.8, alpha=0.8)
        sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(levels.min(), levels.max()))
        fig.colorbar(sm, ax=ax2, label='сила команды, %')
        ax2.set_xlabel('время от команды «закрыть», с')
        ax2.set_ylabel(f'сила по весам, {a.unit}')
        ax2.set_title('установление силы на удержании', fontsize=10)
        ax2.grid(alpha=0.3)

    fig.tight_layout()
    out_png = HERE / f'{path.stem.replace("_summary", "")}_calib.png'
    fig.savefig(out_png, dpi=110)
    print(f'График: {out_png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
