"""
Офлайн-прогон детектора проскальзывания по записи — для подбора порогов.

Файлы:
  tools/data/slip_*.csv (slip_grip.py) — детектор взведён там же, где был при записи
                                       (phase = hold), опорный кадр — с начала каждого hold.
  tools/data/ramp_*.csv (force_ramp.py) — проскальзываний нет: детектор взводится через --arm-after с
                                       после начала каждого хвата, все события = ложные срабатывания
                                       (проверка, что рост силы и ползучесть не дают тревог).

Запуск (из папки analysis):
  uv run python slip_replay.py --file ../tools/data/ramp_20261026_170237.csv
  uv run python slip_replay.py --file ../tools/data/slip_XXXX.csv --shear-thr 8 --vib-thr 2.5
  ... --no-show           # только сводка и <имя>_slip.png рядом со скриптом
"""
import argparse
import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'tools' / 'common'))
from slip_detector import CAUSES, CELLS, SlipDetector, estimate_sigma, load_normal_dirs

MAG_START = 'THUMB_MLX0_X'


def load(path):
    with open(path, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    h = rows[0]
    col = {n: i for i, n in enumerate(h)}
    j = col[MAG_START]
    t = np.array([float(r[0]) for r in rows[1:]])
    mag = np.array([[float(v) for v in r[j:j + 36]] for r in rows[1:]])
    force = np.array([float(r[col['force_pct']]) for r in rows[1:]])
    phase = [r[col['phase']] for r in rows[1:]] if 'phase' in col else None
    return t, force, mag, phase


def arm_plan(t, force, phase, arm_after):
    """-> armed[i] (bool), rearm[i] (True — в этом кадре снять новый опорный кадр)."""
    if phase is not None:
        armed = np.array([p == 'hold' for p in phase])
    else:
        grip = force > 0
        armed = np.zeros(len(t), bool)
        start = None
        for i in range(len(t)):
            if grip[i] and (i == 0 or not grip[i - 1]):
                start = t[i]
            armed[i] = grip[i] and start is not None and t[i] - start >= arm_after
    rearm = armed & ~np.r_[False, armed[:-1]]
    return armed, rearm


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--file', required=True)
    p.add_argument('--medians', default=str(HERE / 'ramp_20261026_170237_medians.csv'),
                   help="ramp-калибровка направления нажатия; '' — без неё")
    p.add_argument('--vib-thr', type=float, default=3.0)
    p.add_argument('--unload-thr', type=float, default=4.0)
    p.add_argument('--shear-thr', type=float, default=6.0)
    p.add_argument('--confirm', type=int, default=3)
    p.add_argument('--arm-after', type=float, default=1.7, help='ramp: взвод через N с после начала хвата')
    p.add_argument('--no-show', action='store_true')
    a = p.parse_args()
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

    t, force, mag, phase = load(a.file)
    armed, rearm = arm_plan(t, force, phase, a.arm_after)
    # шум — по кадрам до первого хвата (кисть раскрыта)
    first = np.argmax(armed | (force > 0)) if (armed | (force > 0)).any() else len(t)
    calib = mag[:max(first, 10)]
    det = SlipDetector(fs=(len(t) - 1) / (t[-1] - t[0]), sigma=estimate_sigma(calib),
                       normal_dirs=load_normal_dirs(a.medians) if a.medians else None,
                       vib_thr=a.vib_thr, unload_thr=a.unload_thr, shear_thr=a.shear_thr, confirm=a.confirm)

    feat = np.zeros((len(t), 3, 3))
    events = []
    for i in range(len(t)):
        if rearm[i]:
            det.arm()
        if not armed[i] and det.armed:
            det.disarm()
        r = det.update(mag[i])
        feat[i] = r['feat']
        if r['slip']:
            events.append((t[i], r['cell'], r['cause'], r['score']))
            det.arm()                       # как в slip_grip --detect-only

    armed_min = armed.sum() / det.fs / 60
    print(f'{Path(a.file).name}: кадров {len(t)} ({det.fs:.1f}/с), взведено {armed_min:.1f} мин, '
          f'шум σ ср. {", ".join(f"{c} {s:.0f}" for c, s in zip(CELLS, det.sigma.mean(1)))} LSB')
    print(f'Событий: {len(events)} ({len(events) / max(armed_min, 1e-9):.2f} в мин)')
    for c in range(3):
        for k in CAUSES:
            n = sum(1 for e in events if e[1] == c and e[2] == k)
            if n:
                print(f'  {CELLS[c]:<7} {k:<7} {n}')
    thr = det.thr
    for c in range(3):
        print(f'  {CELLS[c]:<7} максимум/порог во взведённом:  '
              + '  '.join(f'{k} {feat[armed, c, j].max() / thr[j]:.2f}' if armed.any() else ''
                          for j, k in enumerate(CAUSES)))

    fig, axs = plt.subplots(4, 1, figsize=(13, 9), sharex=True, height_ratios=[0.6, 1, 1, 1])
    axs[0].plot(t, force, color='k', lw=1)
    axs[0].fill_between(t, 0, 100, where=armed, color='#5cb85c', alpha=0.15, step='mid', label='взведён')
    axs[0].set_ylabel('сила, %')
    axs[0].legend(loc='upper left', fontsize=8)
    colors = ['#1f77b4', '#d62728', '#ff7f0e']
    for c in range(3):
        ax = axs[c + 1]
        for j, k in enumerate(CAUSES):
            ax.plot(t, feat[:, c, j] / thr[j], lw=0.9, color=colors[j], label=f'{k} / порог')
        ax.axhline(1, color='gray', ls='--', lw=0.8)
        for e in events:
            if e[1] == c:
                ax.axvline(e[0], color='m', lw=1.2, alpha=0.7)
        ax.set_ylabel(CELLS[c])
        ax.set_ylim(0, 3)
        ax.grid(alpha=0.3)
    axs[1].legend(loc='upper left', fontsize=8, ncol=3)
    axs[-1].set_xlabel('время, с')
    fig.suptitle(f'{Path(a.file).name}: признаки проскальзывания (линии — события)', fontsize=10)
    fig.tight_layout()
    png = HERE / f'{Path(a.file).stem}_slip.png'
    fig.savefig(png, dpi=110)
    print(f'График: {png}')
    if not a.no_show:
        plt.show()


if __name__ == '__main__':
    main()
