"""Fit compressor dynamic structure on 44.1 kHz step + vocal data."""
import sys, os, itertools, numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs44lib import *
from numba import njit
from scipy.optimize import minimize

@njit(cache=True)
def comp_gen(x, sr, thr_db, ratio, makeup_db, env_rel_ms, att_ms, rel_ms, order, domain, env_att_ms):
    """domain: 0 = linear gain smoothing, 1 = dB smoothing. order: 1 or 2 cascaded one-poles.
    env: peak detector with attack env_att_ms (0 = instant) and release env_rel_ms."""
    envRel = np.exp(-1.0 / (env_rel_ms * 1e-3 * sr))
    envAtt = 0.0 if env_att_ms <= 0 else np.exp(-1.0 / (env_att_ms * 1e-3 * sr))
    gA = np.exp(-1.0 / (att_ms * 1e-3 * sr)); gR = np.exp(-1.0 / (rel_ms * 1e-3 * sr))
    slope = 1.0 - 1.0 / ratio
    env = 0.0; y = np.empty_like(x)
    # smoother states (in chosen domain)
    init = makeup_db if domain == 1 else 10 ** (makeup_db / 20)
    s1 = init; s2 = init
    for i in range(len(x)):
        a = abs(x[i])
        if a > env: env = envAtt * env + (1 - envAtt) * a
        else: env = envRel * env + (1 - envRel) * a
        over = 20 * np.log10(max(env, 1e-9)) - thr_db
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
        gl = 10 ** (g / 20) if domain == 1 else g
        y[i] = x[i] * gl
    return y

