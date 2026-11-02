"""
Проскальзывание против статики: какое G «должно быть» при такой силе F — и насколько оно другое.

Опорная запись (статика, без проскальзывания): ramp_*.csv — плавный рост силы. По ней для каждой ячейки
строится зависимость G_ref(F): векторные признаки положения магнита как функция силы F
(F — как в прошивке: (|B3−B0| + |B2−B1|)/2 после EMA 0.6, минус baseline).

Запись с проскальзыванием: в каждый момент берётся её F, по опорной таблице — G_ref(F), и считается
ΔG = G − G_ref(F). При чистом нажатии ΔG ≈ 0 (точка лежит на опорной кривой); при сдвиге магнита
вбок — ΔG растёт, и направление ΔG показывает направление сдвига.

Признаки G (common/mag_features.py, все без внешнего поля):
  shear  = (Gx_z, Gy_z)        — сдвиг магнита в плоскости (основной для проскальзывания)
  gxy    = Gx_x, Gx_y, Gy_x, Gy_y — xy-градиент (форма поля под магнитом)
  |G|    = |(Gx + Gy)/2|        — модуль, как на прошлых графиках
F и G считаются от baseline одинаково (вычитается состояние раскрытой кисти).

Опорная таблица: по оси F — бины (--bin мкТл), в каждом — медиана G; между бинами — интерполяция.
Вне диапазона F опорной записи значение крайнего бина (отмечается на графике).

Запуск (из папки analysis):
  uv run python slip_vs_static.py --slip <файл>.csv
  uv run python slip_vs_static.py --slip <файл>.csv --ref ../tools/data/ramp_20261026_170237.csv --bin 4
Выход: <имя>_vs_static.png и <имя>_vs_static.csv (по кадрам: F, G, G_ref, ΔG) рядом со скриптом.
"""
import argparse
import csv
import hashlib
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from mag_features import CELLS, baseline_correct, ema, final_mag, gradients, to_common

FEATS = ['shear_x', 'shear_y', 'gxx', 'gxy', 'gyx', 'gyy', 'gmod']
FEAT_LABEL = {'shear_x': 'сдвиг x (Gx_z)', 'shear_y': 'сдвиг y (Gy_z)', 'gxx': 'Gx_x', 'gxy': 'Gx_y',
              'gyx': 'Gy_x', 'gyy': 'Gy_y', 'gmod': '|(Gx+Gy)/2|'}


def load(path):
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    h = rows[0]
    j = h.index('THUMB_MLX0_X')
    col = {n: i for i, n in enumerate(h)}
    data = [r for r in rows[1:] if len(r) == len(h)]
    t = np.array([float(r[col['t_s']]) for r in data])
    mag = np.array([[float(v) for v in r[j:j + 36]] for r in data])
    force = np.array([float(r[col['force_pct']]) for r in data]) if 'force_pct' in col else None
    hall = None
    for name in ('hall_filt', 'hall'):
        if name in col:
            hall = np.array([float(r[col[name]]) for r in data])
            break
    return t, mag, force, hall


def closure_signal(t, force, n):
    """Раскрытие для baseline: из силы команды (0 — открыта), иначе считаем кисть всё время закрытой
    (baseline берётся по первым кадрам и дальше двигается только без касания)."""
    if force is None:
        return np.full(n, 100.0)
    closed = force > 0
    out = np.where(closed, 100.0, 100.0)
    t_open = None
    for i in range(n):
        if not closed[i]:
            t_open = t[i] if t_open is None else t_open
            out[i] = 100.0 if t[i] - t_open < 0.3 else 0.0
        else:
            t_open = None
    return out


def features(t, mag, force, init_frames=20):
    """-> F (N,3) минус baseline, G dict признак -> (N,3) минус baseline, touch (N,)."""
    m = ema(to_common(mag))
    F_raw = final_mag(m)
    g = gradients(None, common=m)
    gm = (g['gx'] + g['gy']) / 2
    G_raw = {'shear_x': g['shear'][..., 0], 'shear_y': g['shear'][..., 1],
             'gxx': g['gx'][..., 0], 'gxy': g['gx'][..., 1], 'gyx': g['gy'][..., 0], 'gyy': g['gy'][..., 1],
             'gmod': np.linalg.norm(gm, axis=-1)}
    closure = closure_signal(t, force, len(t))
    Fc, Fb, touch, others = baseline_correct(F_raw, closure, init_frames=init_frames,
                                             others=[G_raw[k] for k in FEATS])
    F = F_raw - Fb                                     # без NOISE_FLOOR: для сопоставления нужен знак
    G = {k: G_raw[k] - b for k, (_, b) in zip(FEATS, others)}
    return F, G, touch


def build_ref(F, G, mask, bin_w):
    """Опорные таблицы по ячейкам: центры бинов F и медианы G. -> {c: (fc, {k: g})}."""
    ref = {}
    for c in range(3):
        f = F[mask, c]
        edges = np.arange(np.floor(f.min()), f.max() + bin_w, bin_w)
        idx = np.digitize(f, edges) - 1
        fc, gk = [], {k: [] for k in FEATS}
        for b in range(len(edges) - 1):
            sel = idx == b
            if sel.sum() < 5:
                continue
            fc.append(np.median(f[sel]))
            for k in FEATS:
                gk[k].append(np.median(G[k][mask][sel, c]))
        ref[c] = (np.array(fc), {k: np.array(v) for k, v in gk.items()})
    return ref


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--slip', required=True, help='запись с проскальзыванием')
    p.add_argument('--ref', default=str(HERE.parent / 'tools' / 'data' / 'ramp_20261026_170237.csv'),
                   help='опорная запись без проскальзывания')
    p.add_argument('--bin', type=float, default=4.0, help='ширина бина F, мкТл (4)')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    if hashlib.md5(Path(a.slip).read_bytes()).digest() == hashlib.md5(Path(a.ref).read_bytes()).digest():
        print(f'ВНИМАНИЕ: {Path(a.slip).name} побайтно совпадает с опорной записью {Path(a.ref).name} — '
              f'это та же запись, ΔG будет ~0 по построению.')

    tr, mr, fr, _ = load(a.ref)
    Fr, Gr, touch_r = features(tr, mr, fr)
    ref_mask = touch_r & ((fr > 0) if fr is not None else True)
    ref = build_ref(Fr, Gr, ref_mask, a.bin)

    ts, ms, fs, _ = load(a.slip)
    Fs, Gs, touch_s = features(ts, ms, fs)
    dG = {k: np.zeros_like(Gs[k]) for k in FEATS}
    Gref = {k: np.zeros_like(Gs[k]) for k in FEATS}
    out_of_range = np.zeros_like(Fs, bool)
    for c in range(3):
        fc, gk = ref[c]
        out_of_range[:, c] = (Fs[:, c] < fc[0]) | (Fs[:, c] > fc[-1])
        for k in FEATS:
            Gref[k][:, c] = np.interp(Fs[:, c], fc, gk[k])
            dG[k][:, c] = Gs[k][:, c] - Gref[k][:, c]

    shear_d = np.hypot(dG['shear_x'], dG['shear_y'])
    gxy_d = np.sqrt(sum(dG[k] ** 2 for k in ('gxx', 'gxy', 'gyx', 'gyy')))
    # шум на опорной записи: ΔG самой опоры относительно своей же таблицы
    print(f'{Path(a.slip).name}: кадров {len(ts)}; опора {Path(a.ref).name}, бин F {a.bin} мкТл')
    for c in range(3):
        fc, gk = ref[c]
        r_sh = np.hypot(Gr['shear_x'][ref_mask, c] - np.interp(Fr[ref_mask, c], fc, gk['shear_x']),
                        Gr['shear_y'][ref_mask, c] - np.interp(Fr[ref_mask, c], fc, gk['shear_y']))
        act = touch_s
        print(f'  {CELLS[c]:6} F опоры {fc[0]:6.0f}..{fc[-1]:6.0f} мкТл; F теста {Fs[act, c].min():6.0f}..'
              f'{Fs[act, c].max():6.0f} (вне опоры {out_of_range[act, c].mean() * 100:3.0f}% кадров); '
              f'|Δсдвиг|: опора p95 {np.percentile(r_sh, 95):5.1f}, тест p95 {np.percentile(shear_d[act, c], 95):5.1f}, '
              f'макс {shear_d[act, c].max():5.1f} мкТл')

    out_csv = HERE / f'{Path(a.slip).stem}_vs_static.csv'
    with open(out_csv, 'w', newline='', encoding='utf-8-sig') as fh:
        w = csv.writer(fh)
        w.writerow(['t_s'] + [f'{n}_{q}' for n in CELLS for q in
                              ['F', 'dShear', 'dGxy'] + [f'{k}' for k in FEATS] + [f'{k}_ref' for k in FEATS]])
        for i in range(len(ts)):
            row = [f'{ts[i]:.4f}']
            for c in range(3):
                row += [f'{Fs[i, c]:.2f}', f'{shear_d[i, c]:.2f}', f'{gxy_d[i, c]:.2f}']
                row += [f'{Gs[k][i, c]:.2f}' for k in FEATS] + [f'{Gref[k][i, c]:.2f}' for k in FEATS]
            w.writerow(row)

    fig, axs = plt.subplots(4, 3, figsize=(18, 12), sharex='col')
    for c in range(3):
        ax = axs[0, c]
        ax.plot(ts, Fs[:, c], color='k', lw=1, label='F (минус baseline)')
        if fs is not None:
            ax2 = ax.twinx()
            ax2.plot(ts, fs, color='#bbb', lw=0.8)
            ax2.set_ylim(-5, 300)
            ax2.set_yticks([])
        ax.fill_between(ts, Fs[:, c].min(), Fs[:, c].max(), where=out_of_range[:, c] & touch_s,
                        color='#f0ad4e', alpha=0.25, step='mid', label='F вне опорной записи')
        ax.set_title(f'{CELLS[c]}: сила F, мкТл', fontsize=10)
        ax.legend(fontsize=7, loc='upper left')
        ax = axs[1, c]
        ax.plot(ts, Gs['shear_x'][:, c], lw=0.9, color='#1f77b4', label='сдвиг x')
        ax.plot(ts, Gref['shear_x'][:, c], lw=0.9, ls='--', color='#1f77b4', label='x статика при той же F')
        ax.plot(ts, Gs['shear_y'][:, c], lw=0.9, color='#ff7f0e', label='сдвиг y')
        ax.plot(ts, Gref['shear_y'][:, c], lw=0.9, ls='--', color='#ff7f0e', label='y статика при той же F')
        ax.set_title('сдвиг G = (Gx_z, Gy_z), мкТл: тест и статика', fontsize=10)
        ax.legend(fontsize=7, loc='upper left', ncol=2)
        ax = axs[2, c]
        ax.plot(ts, dG['shear_x'][:, c], lw=0.9, label='Δ сдвиг x')
        ax.plot(ts, dG['shear_y'][:, c], lw=0.9, label='Δ сдвиг y')
        ax.plot(ts, shear_d[:, c], lw=1.2, color='k', label='|Δ сдвиг|')
        ax.set_title('ΔG = G − G_статика(F): сдвиг, мкТл', fontsize=10)
        ax.legend(fontsize=7, loc='upper left', ncol=3)
        ax = axs[3, c]
        ax.plot(ts, gxy_d[:, c], lw=0.9, color='#2ca02c', label='|Δ xy-градиента|')
        ax.plot(ts, dG['gmod'][:, c], lw=0.9, color='#9467bd', label='Δ|(Gx+Gy)/2|')
        ax.set_title('ΔG: xy-градиент и модуль, мкТл', fontsize=10)
        ax.set_xlabel('время, с')
        ax.legend(fontsize=7, loc='upper left')
    for ax in axs.ravel():
        ax.grid(alpha=0.3)
        ax.tick_params(labelsize=7)
    fig.suptitle(f'{Path(a.slip).name} против статики {Path(a.ref).name}', fontsize=11)
    fig.tight_layout()
    png = HERE / f'{Path(a.slip).stem}_vs_static.png'
    fig.savefig(png, dpi=105)
    print(f'График: {png}\nКадры: {out_csv}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
