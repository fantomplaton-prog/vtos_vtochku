"""
Детектор проскальзывания в реальном времени по касательной нагрузке, отнесённой к силе пальца.

  F    = (|B3 − B0| + |B2 − B1|)/2 — сила пальца, мкТл (как в прошивке), минус раскрытая кисть
  d    = |(Gx_z, Gy_z) − те же в эталоне хвата| — насколько магнит сдвинулся вбок (common/mag_features.py)
  load = d / max(F, floor)                       — касательная нагрузка относительно силы (|ΔG|/F)
  r    = (d − base) / max(F, floor)              — РОСТ нагрузки: base — минимум d с последнего события
Всё после EMA 0.6, как в прошивке. Считается только рост: когда подушечка возвращается назад
(отпустили, перестали тянуть), d падает, base идёт вниз за ним, и срабатывания нет.

Скольжение — r > thr у любого пальца с контактом confirm кадров подряд.
Конец — r < release·thr (нагрузка вернулась) ИЛИ r перестал меняться (размах за still_s с < still_tol):
подушечка осталась в новом положении (держит трение покоя) — base := d, следующее скольжение — от него.
Контакт по F: палец с F < max(f_valid, lost_frac·F хвата) сразу не участвует; если так lost_frames
кадров подряд — выпадает до нового эталона. Выпали все, что держали, — ('lost', ...), детектор выкл.

Проверено офлайн на slipt_20261102_163130 (10 циклов «хват до касания → скольжение»).

  ls = LiveSlip(thr=1.0)
  ev = ls.feed(mag36)   # на каждый кадр: None | ('start', палец, r) | ('end', длит_с, по_покою)
                        #                 | ('lost', [пальцы], во_время_скольжения)
  ls.set_zero()         # кисть раскрыта, не касается — ноль F и G
  ls.arm()              # кисть держит, неподвижна — эталон; детектор включён
  ls.disarm()
"""
import collections
import time

import numpy as np

from mag_features import EMA_ALPHA_MAG, CELLS, feature_set, to_common


class LiveSlip:
    def __init__(self, thr=1.0, confirm=2, release=0.7, floor=20.0, f_valid=10.0, lost_frac=0.3,
                 lost_frames=3, still_s=0.4, still_tol=0.15, fs=55.0):
        self.thr, self.confirm, self.release = thr, confirm, release
        self.floor, self.f_valid = floor, f_valid
        self.lost_frac, self.lost_frames = lost_frac, lost_frames
        self.still_tol = still_tol
        self.r_hist = collections.deque(maxlen=max(3, int(round(still_s * fs))))
        self.ema = None
        self.hist = collections.deque(maxlen=200)       # (F, shear) последних кадров, без нуля
        self.zero_F = self.zero_S = None
        self.ref_S = self.F_hold = None
        self.base = np.zeros(3)
        self.valid = np.ones(3, bool)
        self.out = np.zeros(3, bool)
        self.contact = np.zeros(3, bool)
        self.lost_run = np.zeros(3, int)
        self.armed = self.slipping = False
        self.run = 0
        self.t_start = None
        self.r = np.zeros(3)
        self.load = np.zeros(3)
        self.F = np.zeros(3)
        self.n_events = 0

    def feed(self, mag):
        x = to_common(mag)
        self.ema = x if self.ema is None else self.ema + EMA_ALPHA_MAG * (x - self.ema)
        F, G = feature_set(self.ema)
        S = np.stack([G['shear_x'], G['shear_y']], axis=-1)            # (3, 2)
        self.hist.append((F, S))
        if self.zero_F is None:
            return None
        self.F = F - self.zero_F
        if not self.armed:
            return None
        den = np.maximum(self.F, self.floor)
        d = np.linalg.norm((S - self.zero_S) - self.ref_S, axis=-1)
        if not self.slipping:
            self.base = np.minimum(self.base, d)                         # нагрузка снизилась — база за ней
        self.load = d / den
        self.r = (d - self.base) / den

        # контакт по F
        low = self.F < np.maximum(self.f_valid, self.lost_frac * self.F_hold)
        self.lost_run = np.where(low & self.valid, self.lost_run + 1, 0)
        self.out |= self.valid & (self.lost_run >= self.lost_frames)
        self.contact = self.valid & ~self.out & ~low
        if self.valid.any() and self.out[self.valid].all():
            was = self.slipping
            lost = [CELLS[k] for k in np.flatnonzero(self.valid)]
            self.disarm()
            return ('lost', lost, was)

        rv = np.where(self.contact, self.r, 0.0)
        self.r_hist.append(rv.max())
        if not self.slipping:
            self.run = self.run + 1 if rv.max() > self.thr else 0
            if self.run >= self.confirm:
                self.slipping = True
                self.t_start = time.time()
                self.n_events += 1
                self.r_hist.clear()
                k = int(np.argmax(rv))
                return ('start', CELLS[k], float(rv[k]))
        elif rv.max() < self.release * self.thr:
            self.slipping = False
            self.run = 0
            return ('end', time.time() - self.t_start, False)
        elif len(self.r_hist) == self.r_hist.maxlen and np.ptp(self.r_hist) < self.still_tol:
            # нагрузка больше не растёт: подушечка в новом положении — база туда
            self.base = d.copy()
            self.slipping = False
            self.run = 0
            return ('end', time.time() - self.t_start, True)
        return None

    def _median(self, n):
        h = list(self.hist)[-n:]
        if len(h) < 3:
            raise RuntimeError('мало кадров датчиков')
        return np.median([x[0] for x in h], axis=0), np.median([x[1] for x in h], axis=0)

    def set_zero(self, n=30):
        """Ноль на раскрытой, не касающейся кисти (как baseline прошивки)."""
        self.zero_F, self.zero_S = self._median(n)
        self.disarm()

    def arm(self, n=25):
        """Эталон: кисть держит предмет неподвижно. Возвращает F хвата по пальцам, мкТл."""
        if self.zero_F is None:
            raise RuntimeError('нет нуля: сначала раскрыть кисть и sz (или g — он снимает ноль сам)')
        F, S = self._median(n)
        self.F_hold = F - self.zero_F
        self.ref_S = S - self.zero_S
        self.base = np.zeros(3)
        self.valid = self.F_hold >= self.f_valid
        self.contact = self.valid.copy()
        self.out = np.zeros(3, bool)
        self.lost_run = np.zeros(3, int)
        self.r_hist.clear()
        self.armed, self.slipping, self.run = True, False, 0
        return self.F_hold

    def disarm(self):
        self.armed, self.slipping, self.run = False, False, 0
