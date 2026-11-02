"""Re-measure Guitar Suite blocks from 44.1 kHz Audition passes.
usage: python3 -I analyze.py <data_dir> <header_path>
Writes results to <data_dir>/../results44/*.npy and prints a report.
"""
import sys, os, numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gs44lib import *
from scipy.optimize import curve_fit

D = sys.argv[1]; HDR = sys.argv[2]
OUT = os.path.join(os.path.dirname(D.rstrip('/')), 'results44'); os.makedirs(OUT, exist_ok=True)
def P(n): return os.path.join(D, n)
def find(prefix):
    for f in sorted(os.listdir(D)):
        if f.lower().startswith(prefix.lower()) and f.lower().endswith('.wav'): return P(f)
    raise FileNotFoundError(prefix)

print('=' * 70); print('FILES');
for f in sorted(os.listdir(D)):
    if f.lower().endswith('.wav'):
        x, sr = load(P(f)); print(f'  {f:28s} sr={sr} n={len(x)} peak={db(x).max():6.1f} dBFS')

# ---------------- BOX 9 ----------------
print('=' * 70); print('BOX MODEL 9')
xi, sr = load(find('01_impulse')); yo, _ = load(find('44_box9'))
assert sr == 44100
pos = int(np.argmax(np.abs(xi))); amp = xi[pos]
print(f'  impulse at {pos}, amp {amp:.4f}')
seg = yo[pos:pos + 400] / amp
first = int(np.argmax(np.abs(seg) > 1e-5))
tail_end = first + 1 + int(np.argmax(np.abs(seg[first:][::-1]) > 1e-6 * 0) ) if False else None
nz = np.where(np.abs(seg) > 1e-6)[0]
print(f'  IR: first nonzero at +{first} samples, last nonzero at +{nz[-1]} (len {nz[-1]-first+1})')
box44 = seg[first:nz[-1] + 1].copy()
np.save(os.path.join(OUT, 'box44_ir.npy'), box44); np.save(os.path.join(OUT, 'box44_delay.npy'), first)
box48 = parse_header_array(HDR, 'kBoxIR_model9')
# old header: 6 samples of delay were embedded? check leading zeros
lead48 = int(np.argmax(np.abs(box48) > 1e-6))
print(f'  48k header IR: {len(box48)} taps, leading zeros {lead48}')
box48r = resample_ir(box48, 48000, 44100)
f, H44 = spectrum_db(np.concatenate([np.zeros(first), box44]), sr)
_, H48 = spectrum_db(box48r, sr)
r44 = report_freqs(f, H44); r48 = report_freqs(f, H48)
print('  freq    direct44   resampled48   diff')
for fr in r44: print(f'  {fr:6d}  {r44[fr]:8.2f}  {r48[fr]:8.2f}  {r44[fr]-r48[fr]:+6.2f}')
# waveform-level comparison on the impulse response itself
lag, e = best_lag(np.concatenate([np.zeros(first), box44, np.zeros(100)]), np.concatenate([box48r, np.zeros(300)]), 12)
print(f'  IR match (resampled48 vs direct44): {e:.1f} dB at lag {lag}')

# ---------------- FUZZ ----------------
print('=' * 70); print('GARAGE FUZZ')
xr, _ = load(find('02_ramp'))
def fit_fuzz(name):
    y, _ = load(find(name))
    n = min(len(xr), len(y)); x = xr[:n]; yy = y[:n]
    m = np.abs(x) > 1e-4
    f_ = lambda x, A, k: A * np.tanh(k * x)
    (A, k), _ = curve_fit(f_, x[m], yy[m], p0=[0.316, 10], maxfev=20000)
    err = 10 * np.log10(np.sum((yy[m] - f_(x[m], A, k)) ** 2) / np.sum(yy[m] ** 2))
    # also fit with A fixed to 0.31623
    (k2,), _ = curve_fit(lambda x, k: 0.31623 * np.tanh(k * x), x[m], yy[m], p0=[10], maxfev=20000)
    err2 = 10 * np.log10(np.sum((yy[m] - 0.31623 * np.tanh(k2 * x[m])) ** 2) / np.sum(yy[m] ** 2))
    print(f'  {name}: free fit A={A:.5f} k={k:.3f} (ln k {np.log(k):.4f}) err {err:.1f} dB | A fixed: k={k2:.3f} ln k {np.log(k2):.4f} err {err2:.1f} dB | peak out {db(yy).max():.2f} dBFS')
    return k2
k10 = fit_fuzz('44_fuzz10'); k90 = fit_fuzz('44_fuzz90')
amt = parse_header_array(HDR, 'kFuzzAmount'); lk = parse_header_array(HDR, 'kFuzzLogK')
# current model prediction
def lk_model(a):
    if a <= amt[0]:
        s = (lk[1] - lk[0]) / (amt[1] - amt[0]); return lk[0] + (a - amt[0]) * s
    return np.interp(a, amt, lk)
print(f'  current model: ln k(10)={lk_model(10):.4f} (k={np.exp(lk_model(10)):.3f}), ln k(90)={lk_model(90):.4f} (k={np.exp(lk_model(90)):.3f})')
print(f'  measured:      ln k(10)={np.log(k10):.4f} (k={k10:.3f}), ln k(90)={np.log(k90):.4f} (k={k90:.3f})')
np.save(os.path.join(OUT, 'fuzz_k10_k90.npy'), np.array([k10, k90]))

# ---------------- COMPRESSOR ----------------
print('=' * 70); print('COMPRESSOR (steady-state gain per step)')
xc, _ = load(find('04_comp'))
# detect steps: envelope of 1 kHz tone, segments of constant level
env = np.abs(xc); w = 441
e = np.array([env[i:i + w].max() for i in range(0, len(xc) - w, w)])
lvl = db(e)
# segment boundaries where level changes > 0.5 dB
segs = []; s = 0
for i in range(1, len(lvl)):
    if abs(lvl[i] - lvl[i - 1]) > 0.5 or i == len(lvl) - 1:
        if i - s >= 10 and lvl[s] > -80: segs.append((s * w, i * w, float(np.median(lvl[s:i]))))
        s = i
print(f'  {len(segs)} steps found: ' + ', '.join(f'{l:.1f}' for _, _, l in segs))
def comp_curve(name):
    y, _ = load(find(name)); rows = []
    for a, b, lin in segs:
        # steady state: last 40% of segment
        a2 = a + int(0.6 * (b - a));
        yo_ = db(np.abs(y[a2:b]).max()); xi_ = db(np.abs(xc[a2:b]).max())
        rows.append((xi_, yo_))
    rows = np.array(rows)
    print(f'  {name}:');
    for xi_, yo_ in rows: print(f'     in {xi_:7.2f}  out {yo_:7.2f}  gain {yo_-xi_:+6.2f}')
    # fit hard knee: out = in + makeup for in<thr ; out = thr + (in-thr)/ratio + makeup above
    def model(x, thr, ratio, mk): return np.where(x < thr, x, thr + (x - thr) / ratio) + mk
    (thr, ratio, mk), _ = curve_fit(model, rows[:, 0], rows[:, 1], p0=[-20, 4, 2], maxfev=20000)
    res = rows[:, 1] - model(rows[:, 0], thr, ratio, mk)
    print(f'     fit: thr {thr:.2f} dBFS, ratio {ratio:.2f}, makeup {mk:+.2f} dB, max resid {np.abs(res).max():.2f} dB')
    return thr, ratio, mk
c25 = comp_curve('44_comp25'); c75 = comp_curve('44_comp75')
for a, (thr, ratio, mk) in ((25, c25), (75, c75)):
    print(f'  current model @{a}: thr {-0.28667*a:.2f}, ratio {1+0.0504*a:.2f}, makeup {max(0,(a-50)*4.458/50):+.2f}')
np.save(os.path.join(OUT, 'comp25_75.npy'), np.array([c25, c75]))

# ---------------- FILTER 2042 / 34 ----------------
print('=' * 70); print('FILTER Retro BP 2042 / res 34 (LS FIR from vocal)')
xv, _ = load(find('05_in_vocal'))
yf, _ = load(find('44_vocal_filt34'))
n = min(len(xv), len(yf))
lag, e0 = best_lag(yf[:n], xv[:n], 8); print(f'  raw in/out lag {lag}, err {e0:.1f} dB')
for ntaps, ridge in ((256, 1e-4), (384, 1e-4), (256, 1e-3)):
    h, err = ls_fir(xv[:n], yf[:n], ntaps, ridge)
    print(f'  FIR {ntaps} ridge {ridge:g}: fit err {err:.1f} dB')
h34, err = ls_fir(xv[:n], yf[:n], 256, 1e-4)
np.save(os.path.join(OUT, 'filt34_fir256.npy'), h34)
f31 = parse_header_array(HDR, 'kFilterFIR_2042_res31'); f31r = resample_ir(f31, 48000, 44100)
f, H34 = spectrum_db(h34, sr); _, H31 = spectrum_db(f31r, sr)
r34 = report_freqs(f, H34); r31 = report_freqs(f, H31)
print('  freq     res34(44)   res31(48→44)   diff')
for fr in r34: print(f'  {fr:6d}  {r34[fr]:8.2f}  {r31[fr]:8.2f}  {r34[fr]-r31[fr]:+6.2f}')
pk = f[np.argmax(H34)]; print(f'  peak res34 at {pk:.0f} Hz = {H34.max():.2f} dB ; res31 peak at {f[np.argmax(H31)]:.0f} Hz = {H31.max():.2f} dB')
# how well does the old (res31, resampled) filter explain the new output?
y31 = np.convolve(xv[:n], f31r)[:n]
lag, e31 = best_lag(yf[:n], y31, 8); print(f'  old res31 filter vs new res34 output: {e31:.1f} dB (lag {lag})')
# optional control file res31 at 44.1
try:
    yf31, _ = load(find('44_vocal_filt31'))
    h31d, err31 = ls_fir(xv[:n], yf31[:n], 256, 1e-4)
    lag, e = best_lag(yf31[:n], y31, 8)
    print(f'  CONTROL filt31@44.1: LS fit {err31:.1f} dB; resampled-48 filter vs direct44: {e:.1f} dB (lag {lag})')
    np.save(os.path.join(OUT, 'filt31_fir256_44.npy'), h31d)
except FileNotFoundError:
    print('  (no 44_vocal_filt31 control file)')
print('=' * 70); print('done ->', OUT)
