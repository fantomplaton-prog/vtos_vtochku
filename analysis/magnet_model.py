"""
Модель «магнит-диполь над 4 магнитометрами» и решатель (numpy, без scipy).

Система координат (глобальная): начало — центр платы, x — восток, y — север, z — вверх, мм.
Магнитометры в углах квадрата 2*SENSOR_HALF мм (по умолчанию 5x5 мм -> ±2.5 мм), z = 0.
Оси датчиков: z у всех вверх; у левой пары (TL, BL) x на восток, y на север;
у правой пары (TR, BR) x и y развёрнуты на 180° (x на запад, y на юг).

Поле диполя: B(r) = (3 (m·r̂) r̂ − m) / |r|³,  m — в мкТл·мм³, r — в мм, B — в мкТл.
Измерение датчика: b_local = R · (B_dipole(s − p) + B0),  R = diag(±1, ±1, 1).
"""
import itertools

import numpy as np

SENSOR_HALF = 2.5          # мм: половина стороны квадрата расположения датчиков
REST_Z = 4.0               # мм: магнит в покое над центром платы
SPHERE_R = 4.0             # мм: магнит не дальше этого от центра платы
MIN_Z = 0.5                # мм: магнит не может опуститься ниже (на плату)
LSB_UT = np.array([0.150, 0.150, 0.242])   # мкТл/LSB при GAIN_1X, RES_16

# углы: имя -> (позиция, поворот осей датчика)
CORNERS = {
    'TL': (np.array([-SENSOR_HALF, +SENSOR_HALF, 0.0]), np.diag([1.0, 1.0, 1.0])),
    'BL': (np.array([-SENSOR_HALF, -SENSOR_HALF, 0.0]), np.diag([1.0, 1.0, 1.0])),
    'TR': (np.array([+SENSOR_HALF, +SENSOR_HALF, 0.0]), np.diag([-1.0, -1.0, 1.0])),
    'BR': (np.array([+SENSOR_HALF, -SENSOR_HALF, 0.0]), np.diag([-1.0, -1.0, 1.0])),
}
CORNER_NAMES = list(CORNERS)


def dipole_field(sensor_pos, magnet_pos, m):
    """sensor_pos (4,3), magnet_pos (3,), m (3,) -> B (4,3) в глобальной системе."""
    r = sensor_pos - magnet_pos
    d = np.linalg.norm(r, axis=1, keepdims=True)
    rh = r / d
    return (3 * (rh @ m)[:, None] * rh - m) / d ** 3


class Layout:
    """Какой MLX в каком углу: mapping[i] = имя угла для MLX i."""

    def __init__(self, mapping):
        self.mapping = list(mapping)
        self.pos = np.array([CORNERS[c][0] for c in mapping])
        self.rot = np.array([CORNERS[c][1] for c in mapping])

    def measure(self, magnet_pos, m, b0):
        """Модельное показание всех 4 датчиков в их локальных осях, (4,3) мкТл."""
        B = dipole_field(self.pos, magnet_pos, m) + b0
        return np.einsum('kij,kj->ki', self.rot, B)


def constrain(p):
    """Проекция положения магнита в допустимую область: шар радиуса SPHERE_R, z >= MIN_Z."""
    p = p.copy()
    p[2] = max(p[2], MIN_Z)
    n = np.linalg.norm(p)
    if n > SPHERE_R:
        p *= SPHERE_R / n
    return p


def levenberg_marquardt(resid, x0, project=None, iters=200, lam=1e-2, tol=1e-9):
    """Минимизация ||resid(x)||² с численным якобианом; project(x) — проекция ограничений."""
    x = project(x0) if project else x0.copy()
    r = resid(x)
    cost = r @ r
    for _ in range(iters):
        J = np.empty((len(r), len(x)))
        for j in range(len(x)):
            h = 1e-6 * max(1.0, abs(x[j]))
            xp = x.copy()
            xp[j] += h
            J[:, j] = (resid(xp) - r) / h
        A = J.T @ J
        g = J.T @ r
        improved = False
        for _ in range(12):
            step = np.linalg.solve(A + lam * np.diag(np.diag(A) + 1e-12), -g)
            xn = x + step
            if project:
                xn = project(xn)
            rn = resid(xn)
            cn = rn @ rn
            if cn < cost:
                x, r, cost = xn, rn, cn
                lam = max(lam / 3, 1e-12)
                improved = True
                break
            lam *= 4
        if not improved or np.linalg.norm(step) < tol * (1 + np.linalg.norm(x)):
            break
    return x, cost


def fit_rest(layout, meas_list, m_init):
    """Совместная подгонка по состояниям покоя нескольких циклов.
    meas_list: список (4,3) мкТл. Общие: m (3), b0 (3); у каждого цикла своё положение (3).
    Возвращает (positions (N,3), m, b0, rms)."""
    n = len(meas_list)
    meas = np.array(meas_list)

    def unpack(x):
        return x[:3], x[3:6], x[6:].reshape(n, 3)

    def resid(x):
        m, b0, P = unpack(x)
        return np.concatenate([(layout.measure(P[k], m, b0) - meas[k]).ravel() for k in range(n)])

    def project(x):
        x = x.copy()
        for k in range(n):
            x[6 + 3 * k:9 + 3 * k] = constrain(x[6 + 3 * k:9 + 3 * k])
        return x

    x0 = np.concatenate([m_init, np.zeros(3), np.tile([0.0, 0.0, REST_Z], n)])
    x, cost = levenberg_marquardt(resid, x0, project)
    m, b0, P = unpack(x)
    return P, m, b0, np.sqrt(cost / (n * 12))


def fit_position(layout, meas, m, b0, p0, free_dir=True):
    """Положение магнита (и наклон момента при free_dir) по одному измерению (4,3).
    |m| и фон b0 фиксированы. Возвращает (p, m_fit, rms)."""
    mag = np.linalg.norm(m)

    def unpack(x):
        p = x[:3]
        if free_dir:
            mm = x[3:6]
            mm = mm / np.linalg.norm(mm) * mag
        else:
            mm = m
        return p, mm

    def resid(x):
        p, mm = unpack(x)
        return (layout.measure(p, mm, b0) - meas).ravel()

    def project(x):
        x = x.copy()
        x[:3] = constrain(x[:3])
        return x

    x0 = np.concatenate([p0, m]) if free_dir else p0.copy()
    x, cost = levenberg_marquardt(resid, x0, project)
    p, mm = unpack(x)
    return p, mm, np.sqrt(cost / 12)


def best_layout(rest_meas, m_mag_guess=3.0e4):
    """Перебор 24 вариантов «MLX -> угол» и двух полярностей магнита; лучший по RMS.
    Возвращает список (rms, mapping, P, m, b0), отсортированный по rms."""
    out = []
    for perm in itertools.permutations(CORNER_NAMES):
        lay = Layout(perm)
        for sign in (+1, -1):
            P, m, b0, rms = fit_rest(lay, rest_meas, np.array([0, 0, sign * m_mag_guess]))
            out.append((rms, perm, P, m, b0))
    out.sort(key=lambda x: x[0])
    return out
