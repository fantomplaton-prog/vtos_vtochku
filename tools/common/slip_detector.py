"""
Детектор проскальзывания по магнитным тактильным ячейкам (3 ячейки × 4 MLX × 3 оси = 36 осей).

Идея: пока кисть держит предмет, пальцы неподвижны — поле Земли в осях датчиков не меняется,
и любое изменение показаний относительно опорного кадра удержания — это нагрузка на ячейку.
На каждую ячейку (12 осей, в единицах шума σ) считаются три признака:

  vib    — ВЧ-энергия (оси минус их медленное среднее, СКЗ по 12 осям): микровибрации и
           рывки stick-slip; в покое ≈ 1.
  unload — падение нормальной нагрузки: проекция изменения на направление «нажатия» n̂
           (n̂ — куда смещаются оси при росте силы, берётся из ramp-калибровки);
           предмет выезжает из пальцев — нагрузка падает.
  shear  — тангенциальное смещение магнита: изменение, перпендикулярное n̂
           (без калибровки — весь модуль изменения).

score ячейки = max(vib/vib_thr, unload/unload_thr, shear/shear_thr); проскальзывание —
score > 1 у любой ячейки confirm кадров подряд. Опорный кадр медленно (ref_tau) следует
за сигналом: ползучесть эластомера и дрейф не накапливаются, быстрое проскальзывание — видно.

Детектор работает только во «взведённом» состоянии (arm): когда кисть движется или
меняется сила, сигнал меняется сам по себе — тогда disarm(), после успокоения arm().

Время — по номеру кадра (fs), а не по часам ПК: строки приходят пачками с одной меткой.
"""
import csv

import numpy as np

NUM_CELLS, NUM_MLX, AXES = 3, 4, 3
CELLS = ['THUMB', 'INDEX', 'MIDDLE']
CH = NUM_MLX * AXES        # 12 осей на ячейку
FS = 58.0                  # кадр/с по умолчанию; реальная частота зависит от прошивки (с HX711 ~23) — set_fs()
CAUSES = ('vib', 'unload', 'shear')


def ema_alpha(f_hz, fs):
    return 1.0 - np.exp(-2 * np.pi * f_hz / fs)


def estimate_sigma(frames, floor=5.0):
    """Шум каждой оси по кадрам неподвижной кисти: (N,36) -> (3,12).
    По первым разностям (нечувствительно к медленному дрейфу), робастно через MAD."""
    x = np.asarray(frames, float).reshape(len(frames), NUM_CELLS, CH)
    d = np.diff(x, axis=0)
    mad = np.median(np.abs(d - np.median(d, axis=0)), axis=0)
    return np.maximum(mad / 0.6745 / np.sqrt(2), floor)


def load_normal_dirs(medians_csv):
    """Направление «нажатия» каждой ячейки из *_medians.csv (analysis/force_magnets.py):
    среднее по циклам изменения осей от минимальной до максимальной силы. -> (3,12), LSB."""
    with open(medians_csv, encoding='utf-8-sig') as fh:
        rows = list(csv.reader(fh))
    d = np.array([[float(v) for v in r] for r in rows[1:]])
    cyc, force, mag = d[:, 0], d[:, 1], d[:, 3:]
    diffs = []
    for c in np.unique(cyc):
        sel = cyc == c
        f = force[sel]
        m = mag[sel]
        if np.isnan(m[f.argmin()]).any() or np.isnan(m[f.argmax()]).any():
            continue
        diffs.append(m[f.argmax()] - m[f.argmin()])
    if not diffs:
        raise ValueError(f'{medians_csv}: нет циклов с полными данными')
    return np.mean(diffs, axis=0).reshape(NUM_CELLS, CH)


class SlipDetector:
    def __init__(self, fs=FS, sigma=20.0, normal_dirs=None, vib_thr=3.0, unload_thr=4.0, shear_thr=6.0,
                 confirm=3, lp_hz=2.5, hp_hz=3.0, vib_hz=4.0, ref_tau=3.0, cells=(0, 1, 2)):
        self.thr = np.array([vib_thr, unload_thr, shear_thr], float)
        self.confirm = confirm
        self.cells = list(cells)
        self.lp_hz, self.hp_hz, self.vib_hz, self.ref_tau = lp_hz, hp_hz, vib_hz, ref_tau
        self.set_fs(fs)
        self.normal_raw = None if normal_dirs is None else np.asarray(normal_dirs, float).reshape(NUM_CELLS, CH)
        self.set_sigma(sigma)
        self.lp = self.slow = self.ref = None
        self.vib_e = np.ones(NUM_CELLS)
        self.armed = False
        self.run = 0
        self.n = 0

    def set_fs(self, fs):
        """Частота кадров: все фильтры заданы в Гц / секундах и пересчитываются под неё."""
        self.fs = fs
        self.a_lp = ema_alpha(self.lp_hz, fs)
        self.a_hp = ema_alpha(self.hp_hz, fs)
        self.a_vib = ema_alpha(self.vib_hz, fs)
        self.a_ref = 1.0 - np.exp(-1.0 / (self.ref_tau * fs))

    def set_sigma(self, sigma):
        self.sigma = np.broadcast_to(np.asarray(sigma, float), (NUM_CELLS, CH)).copy()
        self.n_hat = None
        if self.normal_raw is not None:
            v = self.normal_raw / self.sigma      # направление в пространстве «единиц шума»
            self.n_hat = v / np.maximum(np.linalg.norm(v, axis=1, keepdims=True), 1e-9)

    def arm(self):
        """Опорный кадр = текущее сглаженное значение; дальше ищем отклонения от него."""
        if self.lp is not None:
            self.ref = self.lp.copy()
        self.armed = True
        self.run = 0

    def disarm(self):
        self.armed = False
        self.run = 0

    def update(self, mag):
        """mag — 36 чисел (как в кадре). -> dict: slip, score, cell, cause, feat (3,3) [vib, unload, shear]."""
        x = np.asarray(mag, float).reshape(NUM_CELLS, CH)
        self.n += 1
        if self.lp is None:
            self.lp, self.slow, self.ref = x.copy(), x.copy(), x.copy()
        self.lp += self.a_lp * (x - self.lp)
        self.slow += self.a_hp * (x - self.slow)
        h = (x - self.slow) / self.sigma
        self.vib_e += self.a_vib * ((h ** 2).mean(axis=1) - self.vib_e)
        vib = np.sqrt(self.vib_e)

        feat = np.zeros((NUM_CELLS, 3))
        feat[:, 0] = vib
        out = {'slip': False, 'score': 0.0, 'cell': -1, 'cause': '', 'feat': feat}
        if not self.armed:
            return out

        self.ref += self.a_ref * (self.lp - self.ref)
        delta = (self.lp - self.ref) / self.sigma
        if self.n_hat is not None:
            dn = (delta * self.n_hat).sum(axis=1)
            feat[:, 1] = np.maximum(0.0, -dn)
            feat[:, 2] = np.linalg.norm(delta - dn[:, None] * self.n_hat, axis=1)
        else:
            feat[:, 2] = np.linalg.norm(delta, axis=1)

        ratio = feat / self.thr
        ratio[[c for c in range(NUM_CELLS) if c not in self.cells]] = 0.0
        cell, k = np.unravel_index(np.argmax(ratio), ratio.shape)
        score = float(ratio[cell, k])
        self.run = self.run + 1 if score > 1.0 else 0
        out.update(score=score, cell=int(cell), cause=CAUSES[k])
        out['slip'] = self.run == self.confirm     # одно событие на эпизод превышения
        return out
