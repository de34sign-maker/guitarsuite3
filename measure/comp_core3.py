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
def gain_model3(x, sr, thr_db, ratio, makeup_db, det, win_ms, det_rel_ms, att_ms, rel_ms, W, order):
    """det 0: box-window RMS (win_ms) ; det 1: box-window peak hold ; det 2: one-pole RMS (attack=release=win_ms)
       det 3: box RMS followed by one-pole release det_rel_ms (max of box and decaying)
       smoothing: linear gain one-pole att/rel, order 1 or 2."""
    N = max(1, int(win_ms * 1e-3 * sr + 0.5))
    dR = np.exp(-1.0 / (max(det_rel_ms, 0.01) * 1e-3 * sr))
    dW = np.exp(-1.0 / (max(win_ms, 0.01) * 1e-3 * sr))
    gA = np.exp(-1.0 / (att_ms * 1e-3 * sr)); gR = np.exp(-1.0 / (rel_ms * 1e-3 * sr))
    slope = 1.0 - 1.0 / ratio
    n = len(x); out = np.zeros(n // W)
    buf = np.zeros(N); acc = 0.0; pos = 0; env = 0.0; envp = 0.0
    s1 = 10 ** (makeup_db / 20); s2 = s1
    for i in range(n):
        xi = x[i]
        if det == 1:
            v = abs(xi)
            buf[pos] = v; pos = (pos + 1) % N
            m = 0.0
            for k in range(N):
                if buf[k] > m: m = buf[k]
            lev = 20 * np.log10(max(m, 1e-10))
        elif det == 2:
            v = xi * xi; env = dW * env + (1 - dW) * v
            lev = 10 * np.log10(max(env, 1e-20)) + 3.0103
        else:
            v = xi * xi
            acc += v - buf[pos]; buf[pos] = v; pos = (pos + 1) % N
            ms_ = max(acc / N, 0.0)
            if det == 3:
                if ms_ > envp: envp = ms_
                else: envp = dR * envp + (1 - dR) * ms_
                ms_ = envp
            lev = 10 * np.log10(max(ms_, 1e-20)) + 3.0103
        over = lev - thr_db
        gdb = makeup_db - (over * slope if over > 0 else 0.0)
        target = 10 ** (gdb / 20)
        c = gA if target < s1 else gR
        s1 = c * s1 + (1 - c) * target
        if order == 2:
            c2 = gA if s1 < s2 else gR
            s2 = c2 * s2 + (1 - c2) * s1
            g = s2
        else:
            g = s1
        k = i // W
        if k < len(out): out[k] += 20 * np.log10(max(g, 1e-9)) / W
    return out
