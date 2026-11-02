import sys, os, numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs44lib import *
from numba import njit
W = 11
def oracle(x, r):
    r = np.roll(r, 1)
    n = len(x) // W; g = np.full(n, np.nan); lv = np.full(n, -200.0)
    for i in range(n):
        xs = x[i*W:(i+1)*W]; rs = r[i*W:(i+1)*W]; d = np.dot(xs, xs)
        if d > 1e-7: g[i] = 20*np.log10(max(np.dot(rs, xs)/d, 1e-6)); lv[i] = 10*np.log10(d/W)
    return g, lv

@njit(cache=True)
def gain_model4(x, sr, thr_db, ratio, makeup_db, sq, env_att_ms, env_rel_ms, g_att_ms, g_rel_ms, W):
    """Envelope follower (on |x| if sq==0, on x^2 if sq==1) with attack/release one-poles,
    static curve on the smoothed envelope, then optional gain smoothing (linear, att/rel; 0 = none)."""
    eA = np.exp(-1.0 / (max(env_att_ms, 1e-3) * 1e-3 * sr)); eR = np.exp(-1.0 / (max(env_rel_ms, 1e-3) * 1e-3 * sr))
    useG = g_att_ms > 0.01
    gA = np.exp(-1.0 / (max(g_att_ms, 1e-3) * 1e-3 * sr)); gR = np.exp(-1.0 / (max(g_rel_ms, 1e-3) * 1e-3 * sr))
    slope = 1.0 - 1.0 / ratio
    env = 0.0; s1 = 10 ** (makeup_db / 20)
    n = len(x); out = np.zeros(n // W)
    for i in range(n):
        v = abs(x[i]) if sq == 0 else x[i] * x[i]
        if v > env: env = eA * env + (1 - eA) * v
        else: env = eR * env + (1 - eR) * v
        lev = 20 * np.log10(max(env, 1e-10)) if sq == 0 else 10 * np.log10(max(env, 1e-20)) + 3.0103
        over = lev - thr_db
        gdb = makeup_db - (over * slope if over > 0 else 0.0)
        target = 10 ** (gdb / 20)
        if useG:
            c = gA if target < s1 else gR
            s1 = c * s1 + (1 - c) * target; g = s1
        else: g = target
        k = i // W
        if k < len(out): out[k] += 20 * np.log10(max(g, 1e-9)) / W
    return out
