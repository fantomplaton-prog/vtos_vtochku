"""
Признаки положения магнита в ячейке по 4 магнитометрам, нечувствительные к внешнему однородному полю.

Геометрия ячейки (проверено по данным ramp_20261026_170237: в покое поле магнита в общей системе
направлено от центра, углы через ~90°, xy-градиент почти диагональный во всех трёх пальцах):

      3 ──── 0          y (север)
      │      │          ↑
      1 ──── 2          └──→ x (восток)       квадрат 5×5 мм

Датчики 1 и 2 повёрнуты на 180° вокруг z относительно 3 и 0: в общую систему (оси датчиков 3, 0)
x -> −x, y -> −y. Значит, пары 3–0 и 1–2 — параллельные стороны, но смотрят навстречу, а 0–1 и 3–2 —
диагонали. Разложение 12 осей ячейки (точное, билинейная модель B = c + Gx·x + Gy·y + T·x·y):
  c  = среднее четырёх                      — внешнее поле + часть поля магнита (не используем)
  Gx = ((B0 − B3) + (B2 − B1)) / 2         — восток минус запад, ∂B/∂x · 5 мм
  Gy = ((B3 − B1) + (B0 − B2)) / 2         — север минус юг,     ∂B/∂y · 5 мм
  T  = ((B0 − B3) − (B2 − B1)) / 2         — «кручение», ∂²B/∂x∂y
Однородное поле сокращается во всех трёх. Для магнита над центром ячейки:
  сдвиг в плоскости  -> shear  = (Gx_z, Gy_z)   (= 0 при магните точно над центром, растёт со смещением)
  прижатие (ближе)   -> normal = (Gx_x + Gy_y)/2
Единицы — мкТл (X, Y ×0.150, Z ×0.242 LSB при GAIN_1X, RES_16).
"""
import numpy as np

NUM_CELLS, NUM_MLX, AXES = 3, 4, 3
CELLS = ['THUMB', 'INDEX', 'MIDDLE']
LSB_UT = np.array([0.150, 0.150, 0.242])
ROTATED = (1, 2)                 # датчики, повёрнутые на 180° вокруг z


def to_common(mag):
    """mag: 36 чисел или (N,36), LSB -> (..., 3, 4, 3) мкТл в общей системе ячейки."""
    m = np.asarray(mag, float)
    m = m.reshape(m.shape[:-1] + (NUM_CELLS, NUM_MLX, AXES)) * LSB_UT
    m[..., ROTATED, 0:2] *= -1
    return m


def gradients(mag, common=None):
    """-> dict векторов (..., 3 ячейки, 3 оси) мкТл: gx, gy, twist, mean;
    и скалярные: shear (..., 3, 2) = (Gx_z, Gy_z), normal (..., 3) = (Gx_x + Gy_y)/2.
    common — уже готовый to_common() (например, после ema()); тогда mag не нужен."""
    m = to_common(mag) if common is None else common
    b0, b1, b2, b3 = (m[..., i, :] for i in range(4))
    gx = ((b0 - b3) + (b2 - b1)) / 2
    gy = ((b3 - b1) + (b0 - b2)) / 2
    tw = ((b0 - b3) - (b2 - b1)) / 2
    return {'gx': gx, 'gy': gy, 'twist': tw, 'mean': m.mean(axis=-2),
            'shear': np.stack([gx[..., 2], gy[..., 2]], axis=-1),
            'normal': (gx[..., 0] + gy[..., 1]) / 2}


# ---------------------------------------------------------------- как в прошивке датчиков
# INDI_sensors_Arm/TacMagVTOS/Core: mlx_90393_app.c, mlx_90393_app.h
EMA_ALPHA_MAG = 0.6
EMA_ALPHA_BASELINE = 0.01
MAG_TOUCH_THRESHOLD = 15.0       # мкТл
FSR_TOUCH_THRESHOLD = 100.0      # отсчёты АЦП
NOISE_FLOOR = 3.0                # мкТл
CLOSURE_CLOSED_THRESH = 96.0     # %
CLOSURE_OPEN_THRESH = 60.0       # %


def ema(x, alpha=EMA_ALPHA_MAG):
    """EMA по времени (ось 0), первый кадр — как есть (MLX90393_FilterXYZ)."""
    x = np.asarray(x, float)
    y = np.empty_like(x)
    y[0] = x[0]
    for i in range(1, len(x)):
        y[i] = y[i - 1] + alpha * (x[i] - y[i - 1])
    return y


def final_mag(common):
    """F = (|B3 − B0| + |B2 − B1|) / 2 по ячейкам (MLX90393_ComputeDiffs/Magnitudes/Mean).
    Пары 3–0 и 2–1 внутри себя одинаково ориентированы, модуль от общей системы не зависит."""
    d0 = np.linalg.norm(common[..., 3, :] - common[..., 0, :], axis=-1)
    d1 = np.linalg.norm(common[..., 2, :] - common[..., 1, :], axis=-1)
    return 0.5 * (d0 + d1)


def baseline_correct(F, closure, fsr=None, others=(), init_frames=1):
    """MLX90393_BaselineCorrection кадр за кадром.
    F (N,3) мкТл; closure (N,) % раскрытия (0 — открыта, 100 — закрыта); fsr (N,6) или None.
    others — ещё признаки (N,3) с ТЕМ ЖЕ baseline-алгоритмом и флагом касания по F (для сравнения).
    init_frames — baseline при старте = среднее первых кадров (в прошивке — 500 кадров калибровки).
    -> (corrected (N,3), baseline (N,3), touch (N,), [(corrected, baseline) для others])."""
    F = np.asarray(F, float)
    series = [F] + [np.asarray(o, float) for o in others]
    base = [s[:init_frames].mean(axis=0).copy() for s in series]
    out_b = [np.empty_like(s) for s in series]
    touch_all = np.zeros(len(F), bool)
    was_closed = False
    for i in range(len(F)):
        if closure[i] > CLOSURE_CLOSED_THRESH:
            was_closed = True
        elif was_closed and closure[i] < CLOSURE_OPEN_THRESH:
            was_closed = False
            for b, s in zip(base, series):
                b[:] = s[i]
        touch = bool(np.any(F[i] > base[0] + MAG_TOUCH_THRESHOLD))
        if not touch and fsr is not None:
            touch = bool(np.any(fsr[i] > FSR_TOUCH_THRESHOLD))
        if not touch:
            for b, s in zip(base, series):
                b[:] = EMA_ALPHA_BASELINE * s[i] + (1 - EMA_ALPHA_BASELINE) * b
        for b, ob in zip(base, out_b):
            ob[i] = b
        touch_all[i] = touch
    corr = [s - b for s, b in zip(series, out_b)]
    corr[0] = np.where(corr[0] < NOISE_FLOOR, 0.0, corr[0])   # обнуление — только у F, как в прошивке;
    return corr[0], out_b[0], touch_all, list(zip(corr[1:], out_b[1:]))   # у остальных знак сохраняется


SHEAR_FEATS = ['shear_x', 'shear_y', 'gxx', 'gxy', 'gyx', 'gyy']


def feature_set(common):
    """common (..., 3, 4, 3) мкТл (уже после ema) -> F (..., 3) и dict G (..., 3) для сопоставления
    «проскальзывание против статики»: сдвиг (Gx_z, Gy_z) и xy-градиент."""
    g = gradients(None, common=common)
    G = {'shear_x': g['shear'][..., 0], 'shear_y': g['shear'][..., 1],
         'gxx': g['gx'][..., 0], 'gxy': g['gx'][..., 1], 'gyx': g['gy'][..., 0], 'gyy': g['gy'][..., 1]}
    return final_mag(common), G


class RefTable:
    """Статика: G как функция F по ячейкам (медиана в бинах F, между бинами — интерполяция,
    за краями — крайнее значение). Строится по кадрам без проскальзывания того же хвата."""

    def __init__(self, F, G, bin_w=3.0, min_n=3):
        self.tab = []
        for c in range(F.shape[1]):
            f = F[:, c]
            edges = np.arange(np.floor(f.min()), f.max() + bin_w, bin_w)
            idx = np.digitize(f, edges) - 1
            fc, gk = [], {k: [] for k in G}
            for b in range(len(edges) - 1):
                sel = idx == b
                if sel.sum() >= min_n:
                    fc.append(np.median(f[sel]))
                    for k in G:
                        gk[k].append(np.median(G[k][sel, c]))
            if not fc:                                   # совсем мало кадров — одна точка
                fc = [np.median(f)]
                gk = {k: [np.median(G[k][:, c])] for k in G}
            self.tab.append((np.array(fc), {k: np.array(v) for k, v in gk.items()}))

    def f_range(self, c):
        return self.tab[c][0][0], self.tab[c][0][-1]

    def delta(self, F, G):
        """F (..., 3), G dict -> ΔG dict = G − G_ref(F) и |Δсдвиг| (..., 3)."""
        F = np.asarray(F, float)
        d = {k: np.empty_like(np.asarray(G[k], float)) for k in G}
        for c, (fc, gk) in enumerate(self.tab):
            for k in G:
                d[k][..., c] = np.asarray(G[k])[..., c] - np.interp(F[..., c], fc, gk[k])
        return d, np.hypot(d['shear_x'], d['shear_y'])
