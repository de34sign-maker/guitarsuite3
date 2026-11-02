"""Fit compressor detector + smoothing structure to oracle gain tracks (44.1 kHz)."""
import sys, os, itertools, numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs44lib import *
from numba import njit
from scipy.optimize import minimize

W = 11  # oracle window (0.25 ms)

def oracle(x, r):
    r = np.roll(r, 1)
    n = len(x) // W; g = np.full(n, np.nan); lv = np.full(n, -200.0)
    for i in range(n):
        xs = x[i*W:(i+1)*W]; rs = r[i*W:(i+1)*W]; d = np.dot(xs, xs)
        if d > 1e-7: g[i] = 20*np.log10(max(np.dot(rs, xs)/d, 1e-6)); lv[i] = 10*np.log10(d/W)
    return g, lv

@njit(cache=True)
def gain_model(x, sr, thr_db, ratio, makeup_db, det, det_att_ms, det_rel_ms, order, domain, att_ms, rel_ms, W):
    """det: 0 = peak (abs), 1 = RMS (squared). Detector: one-pole with attack/release on the chosen quantity.
    Static curve on detector level in dB (peak-equivalent for sine: RMS level + 3.01 dB).
    Smoothing: order 1/2 one-poles in linear (0) or dB (1) domain with separate attack/release.
    Returns gain (dB) averaged per window W."""
    dA = 0.0 if det_att_ms <= 0 else np.exp(-1.0 / (det_att_ms * 1e-3 * sr))
    dR = np.exp(-1.0 / (det_rel_ms * 1e-3 * sr))
    gA = np.exp(-1.0 / (att_ms * 1e-3 * sr)); gR = np.exp(-1.0 / (rel_ms * 1e-3 * sr))
    slope = 1.0 - 1.0 / ratio
    env = 0.0
    init = makeup_db if domain == 1 else 10 ** (makeup_db / 20)
    s1 = init; s2 = init
    n = len(x) // W; out = np.zeros(n)
    for i in range(len(x)):
        a = abs(x[i]) if det == 0 else x[i] * x[i]
        if a > env: env = dA * env + (1 - dA) * a
        else: env = dR * env + (1 - dR) * a
        lev = 20 * np.log10(max(env, 1e-10)) if det == 0 else 10 * np.log10(max(env, 1e-20)) + 3.0103
        over = lev - thr_db
        gdb = makeup_db - (over * slope if over > 0 else 0.0)
        target = gdb if domain == 1 else 10 ** (gdb / 20)
        c = gA if target < s1 else gR
        s1 = c * s1 + (1 - c) * target
        if order == 2:
            c2 = gA if s1 < s2 else gR
            s2 = c2 * s2 + (1 - c2) * s1
            g = s2
        else:
            g = s1
        gdb_out = g if domain == 1 else 20 * np.log10(max(g, 1e-9))
        k = i // W
        if k < n: out[k] += gdb_out / W
    return out

