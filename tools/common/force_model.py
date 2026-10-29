"""
Линеаризация силы сжатия кисти по калибровке на весах (большой палец).

Сырой % — то, что уходит в команду 01 11 01 <pct>; реальная сила от него зависит нелинейно
(на этой кисти 1–50 % дают одну и ту же минимальную силу, рост — только от ~55 %).
Линейный % — сила растёт по прямой. Два режима (mode):
  'range' (по умолчанию) — N % = минимальная сила + N % от (максимум − минимум): прямая на всём
                           0–100 %, но 0 % = минимальная сила кисти, а не ноль;
  'proportional'         — N % = N % от максимальной силы: прямая через ноль, но ниже
                           min_linear % не обеспечить (будет минимальная сила).

Цепочка: tools/experiments/force_calib.py (замеры) -> analysis/force_calib.py --install
-> tools/common/force_calib.json (рабочая калибровка, её подхватывает HandController).

  fm = ForceModel.load_default()      # None, если калибровки нет
  fm.raw_pct(50)        # линейный % -> сырой % для команды
  fm.linear_pct(30)     # сырой % -> какая это доля максимальной силы, %
  fm.force(40)          # сырой % -> сила (в единицах калибровки, fm.unit)
  fm.pct(3.5)           # сила -> наименьший сырой %, дающий не меньше этой силы

Между уровнями калибровки — линейная интерполяция по монотонной кривой; за пределами — край.
Ниже минимальной достижимой силы (fm.min_linear) линейный % не обеспечить — отдаётся
наименьший откалиброванный сырой %.
"""
import json
import os

import numpy as np

DEFAULT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'force_calib.json')


class ForceModel:
    def __init__(self, levels_pct, force, unit='', source='', mode='range'):
        self.levels = np.asarray(levels_pct, float)
        # неубывание; отрицательные (остаток нуля весов в мёртвой зоне) — это 0
        self.f = np.maximum.accumulate(np.clip(np.asarray(force, float), 0.0, None))
        self.unit = unit
        self.source = source
        self.f_max = float(self.f[-1])
        self.f_min = float(self.f[0])
        self.mode = mode
        # наименьшая сила, которую кисть реально даёт (первый уровень выше мёртвой зоны), в линейных %
        live = self.f > 0.02 * self.f_max
        self.min_linear = 100.0 * self.f[np.argmax(live)] / self.f_max if self.f_max > 0 and live.any() else 100.0

    @classmethod
    def load(cls, path):
        with open(path, encoding='utf-8') as fh:
            c = json.load(fh)
        return cls(c['levels_pct'], c['force_monotone'], c.get('unit', ''), c.get('source', ''),
                   c.get('mode', 'range'))

    @classmethod
    def load_default(cls):
        return cls.load(DEFAULT_PATH) if os.path.exists(DEFAULT_PATH) else None

    def force(self, pct):
        return float(np.interp(pct, self.levels, self.f))

    def pct(self, force):
        """Наименьший сырой % с силой >= force (на плоских участках кривой — левый край)."""
        if force <= self.f[0]:
            return float(self.levels[0])
        if force >= self.f[-1]:
            return float(self.levels[np.argmax(self.f >= self.f[-1])])
        i = int(np.searchsorted(self.f, force, side='left'))
        f0, f1 = self.f[i - 1], self.f[i]
        x0, x1 = self.levels[i - 1], self.levels[i]
        return float(x1 if f1 == f0 else x0 + (force - f0) / (f1 - f0) * (x1 - x0))

    def target_force(self, linear_pct):
        """Линейный % -> какая сила нужна (по режиму)."""
        x = max(0.0, min(100.0, linear_pct)) / 100.0
        if self.mode == 'proportional':
            return x * self.f_max
        return self.f_min + x * (self.f_max - self.f_min)

    def raw_pct(self, linear_pct):
        """Линейный % -> сырой % для команды, целое 1..100."""
        return int(max(1, min(100, round(self.pct(self.target_force(linear_pct))))))

    def linear_pct(self, raw_pct):
        f = self.force(raw_pct)
        if self.mode == 'proportional':
            return 100.0 * f / self.f_max if self.f_max > 0 else 0.0
        span = self.f_max - self.f_min
        return 100.0 * (f - self.f_min) / span if span > 0 else 0.0
