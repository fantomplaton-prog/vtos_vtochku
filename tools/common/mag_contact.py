"""
Касание по магнитным тактильным ячейкам (3 ячейки × 4 MLX × 3 оси), без весов.

Признак ячейки — как в прошивке датчиков (INDI_sensors_Arm, mlx_90393_app.c:
MLX90393_ComputeDiffs / ComputeMagnitudes / ComputeMean):
  оси -> мкТл (X, Y ×0.150; Z ×0.242 при GAIN_1X, RES_16);
  разности пар: d0 = MLX3 − MLX0, d1 = MLX2 − MLX1 (векторы XYZ);
  F = (|d0| + |d1|) / 2.
В разности пары однородное поле (поле Земли, которое поворачивается вместе с пальцем) сокращается,
остаётся градиент поля магнита — он и меняется при деформации подушечки.

Касание: |F − F0| > thr·σ_F у выбранных ячеек confirm кадров подряд; F0 — медиана F в начале
сжатия, σ_F — шум F на неподвижной кисти. Прошивка F ещё и сглаживает EMA — здесь сырые кадры,
без сглаживания (быстрее реакция).
"""
import numpy as np

NUM_CELLS, NUM_MLX, AXES = 3, 4, 3
CELLS = ['THUMB', 'INDEX', 'MIDDLE']
LSB_UT = np.array([0.150, 0.150, 0.242])
PAIRS = ((3, 0), (2, 1))


def pair_feature(mag):
    """mag: 36 чисел (или (N,36)) -> F по ячейкам, мкТл: (3,) (или (N,3))."""
    m = np.asarray(mag, float)
    m = m.reshape(m.shape[:-1] + (NUM_CELLS, NUM_MLX, AXES)) * LSB_UT
    d = [np.linalg.norm(m[..., a, :] - m[..., b, :], axis=-1) for a, b in PAIRS]
    return 0.5 * (d[0] + d[1])


def noise_sigma(frames, floor=0.05):
    """Шум F по кадрам неподвижной кисти: (N,36) -> (3,) мкТл; по первым разностям, через MAD."""
    f = pair_feature(frames)
    d = np.diff(f, axis=0)
    mad = np.median(np.abs(d - np.median(d, axis=0)), axis=0)
    return np.maximum(mad / 0.6745 / np.sqrt(2), floor)


class MagContact:
    def __init__(self, thr=6.0, confirm=2):
        self.thr, self.confirm = thr, confirm
        self.sigma = np.full(NUM_CELLS, 1.0)
        self.free_max = None               # максимум |ΔF|/σ при движении без предмета (mcal)
        self.f0 = None
        self.run = 0

    def set_sigma(self, rest_frames):
        if len(rest_frames) >= 10:
            self.sigma = noise_sigma(rest_frames)

    def check_free(self, segments):
        """Отрезки свободного движения (каждый (N,36)) -> максимум |F − F(начала)|/σ по ячейкам."""
        out = np.zeros(NUM_CELLS)
        for s in segments:
            if len(s) > 3:
                f = pair_feature(s)
                out = np.maximum(out, np.abs(f - f[0]).max(axis=0) / self.sigma)
        self.free_max = out
        return out

    def start(self, x0_frames):
        """Начало сжатия: F0 = медиана F последних кадров."""
        self.f0 = np.median(pair_feature(x0_frames), axis=0)
        self.run = 0

    def score(self, frame):
        """frame (36,) -> (|ΔF|/σ (3,), ΔF мкТл (3,))."""
        df = pair_feature(frame) - self.f0
        return np.abs(df) / self.sigma, df

    def update(self, frame, cells=(0, 1, 2)):
        """-> (касание подтверждено, r (3,) в σ, ΔF (3,) мкТл)."""
        r, df = self.score(frame)
        self.run = self.run + 1 if max(r[c] for c in cells) > self.thr else 0
        return self.run >= self.confirm, r, df
