"""Shared helpers for re-measuring Audition 3.0 Guitar Suite at 44.1 kHz."""
import numpy as np, soundfile as sf

def load(path):
    d, sr = sf.read(path, always_2d=True, dtype='float32')
    return d[:, 0].astype(np.float64), sr

def db(x):
    return 20 * np.log10(np.maximum(np.abs(x), 1e-12))

def best_lag(ref, y, maxlag=8):
    """Return (lag, err_db) minimizing ||ref - y shifted by lag||."""
    n = min(len(ref), len(y)) - 2 * maxlag
    best = (0, 1e9)
    for lag in range(-maxlag, maxlag + 1):
        r = ref[maxlag:maxlag + n]
        s = y[maxlag + lag:maxlag + lag + n]
        e = 10 * np.log10(np.sum((r - s) ** 2) / np.sum(r ** 2) + 1e-30)
        if e < best[1]:
            best = (lag, e)
    return best

def resample_ir(h, sr_old, sr_new, half=64):
    """Same windowed-sinc resampler as GuitarSuiteDSP.h resampleIR()."""
    h = np.asarray(h, dtype=np.float64)
    if abs(sr_old - sr_new) < 1:
        return h.copy()
    ratio = sr_old / sr_new
    n = len(h)
    n_new = int(np.ceil(n / ratio)) + half
    out = np.zeros(n_new)
    for i in range(n_new):
        t = i * ratio
        kc = int(np.floor(t))
        k = np.arange(max(0, kc - half), min(n - 1, kc + half) + 1)
        x = t - k
        w = 0.5 * (1 + np.cos(np.pi * x / half))
        sinc = np.sinc(x)
        out[i] = np.sum(h[k] * sinc * w) * ratio
    return out

def ls_fir(x, y, ntaps, ridge=1e-4, start=0, length=None):
    """Least-squares FIR fit y ≈ h * x (ridge-regularized). Returns (h, err_db)."""
    if length is None:
        length = len(x) - start
    xs = x[start:start + length]
    ys = y[start:start + length]
    # Toeplitz-like via FFT normal equations
    N = ntaps
    # autocorrelation of x and crosscorrelation
    nfft = 1 << int(np.ceil(np.log2(len(xs) + N)))
    X = np.fft.rfft(xs, nfft)
    Y = np.fft.rfft(ys, nfft)
    rxx = np.fft.irfft(X * np.conj(X), nfft)[:N]
    rxy = np.fft.irfft(Y * np.conj(X), nfft)[:N]
    from scipy.linalg import solve_toeplitz
    rxx0 = rxx.copy()
    rxx0[0] += ridge * rxx[0]
    h = solve_toeplitz(rxx0, rxy)
    yh = np.convolve(xs, h)[:len(ys)]
    err = 10 * np.log10(np.sum((ys - yh) ** 2) / np.sum(ys ** 2) + 1e-30)
    return h, err

def parse_header_array(path, name):
    """Read a `static const float NAME[N] = {...};` array from the C header."""
    txt = open(path, encoding='utf-8').read()
    i = txt.index(name)
    j = txt.index('{', i)
    k = txt.index('}', j)
    body = txt[j + 1:k].replace('f', '')
    return np.array([float(v) for v in body.replace('\n', ' ').split(',') if v.strip()])

def spectrum_db(h, sr, nfft=8192):
    H = np.fft.rfft(h, nfft)
    f = np.fft.rfftfreq(nfft, 1 / sr)
    return f, db(H)

def report_freqs(f, H, freqs=(50, 100, 200, 500, 1000, 1600, 2000, 3000, 5000, 8000, 10000, 16000)):
    return {fr: float(np.interp(fr, f, H)) for fr in freqs}
