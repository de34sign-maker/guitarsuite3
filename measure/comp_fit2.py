from comp_core2 import *
D = sys.argv[1]
xs, sr = load(os.path.join(D, '04_comp_steps_1k.wav')); xv, _ = load(os.path.join(D, '05_in_vocal_44.wav'))
data = []
for a in (25, 75, 100):
    r = load(os.path.join(D, f'44_comp{a}.wav'))[0]; g, lv = oracle(xs, r)
    data.append(('steps%d' % a, xs, a, g, lv, 1.0))
r = load(os.path.join(D, '44_vocal_comp.wav'))[0]; g, lv = oracle(xv, r)
data.append(('vocal', xv, 100, g, lv, 3.0))
def static(a): return (-0.29874 * a + 0.1304, 1.9291 + 0.03032 * a, a / 20.0)

def score(p, det, order, domain, verbose=False):
    det_att, det_rel, att, rel = p
    if det_rel <= 0.05 or att <= 0.05 or rel <= 0.5 or det_att < 0: return 1e3
    tot = 0; parts = []
    for name, x, a, g, lv, wgt in data:
        thr, ratio, mk = static(a)
        gm = gain_model(x, sr, thr, ratio, mk, det, det_att, det_rel, order, domain, att, rel, W)
        m = ~np.isnan(g) & (lv > -60)
        e = np.sqrt(np.mean((gm[m] - g[m]) ** 2)); parts.append(e); tot += wgt * e
    if verbose: print('     rms dB: ' + '  '.join(f'{d[0]} {e:.3f}' for d, e in zip(data, parts)))
    return tot

res = []
for det, order, domain in itertools.product((0, 1), (1, 2), (0, 1)):
    best = None
    for p in itertools.product((0, 1, 3), (3, 8, 20), (5, 12, 25), (30, 65, 120)):
        v = score(p, det, order, domain)
        if best is None or v < best[0]: best = (v, p)
    r = minimize(lambda p: score(p, det, order, domain), best[1], method='Nelder-Mead', options={'xatol': 0.02, 'fatol': 0.002, 'maxiter': 600})
    print(f'det {"RMS " if det else "peak"} order {order} dom {"dB " if domain else "lin"}: det_att {r.x[0]:.2f} det_rel {r.x[1]:.2f} att {r.x[2]:.2f} rel {r.x[3]:.2f} ms -> score {r.fun:.3f}')
    score(r.x, det, order, domain, verbose=True)
    res.append((r.fun, det, order, domain, r.x))
res.sort(key=lambda t: t[0]); print('BEST', res[0])
np.save(os.path.join(os.path.dirname(D.rstrip('/')), 'results44', 'comp_fit2_best.npy'), np.array([res[0][1], res[0][2], res[0][3], *res[0][4]]))
