from comp_core import *
D = sys.argv[1]
xs, sr = load(os.path.join(D, '04_comp_steps_1k.wav')); xv, _ = load(os.path.join(D, '05_in_vocal_44.wav'))
refs = {}
for a in (25, 50, 75, 100):
    refs[a] = load(os.path.join(D, f'44_comp{a}.wav'))[0]
rv = load(os.path.join(D, '44_vocal_comp.wav'))[0]
def static(a):
    return (-0.29874 * a + 0.1304, 1.9291 + 0.03032 * a, a / 20.0)

def err(y, ref, maxlag=3):
    return best_lag(ref, y, maxlag)[1]

def total(params, order, domain, verbose=False):
    env_rel, att, rel, env_att = params
    if env_rel <= 0.1 or att <= 0.1 or rel <= 1 or env_att < 0: return 1e3
    es = []
    for a in (25, 75, 100):
        thr, ratio, mk = static(a)
        if a == 50: mk = 0.0
        es.append(err(comp_gen(xs, sr, thr, ratio, mk, env_rel, att, rel, order, domain, env_att), refs[a]))
    thr, ratio, mk = static(100)
    ev = err(comp_gen(xv, sr, thr, ratio, mk, env_rel, att, rel, order, domain, env_att), rv)
    if verbose: print(f'    steps25 {es[0]:.1f} steps75 {es[1]:.1f} steps100 {es[2]:.1f} vocal {ev:.1f}')
    return sum(es) + 2 * ev  # weight vocal

results = []
for order, domain in ((1, 0), (2, 0), (1, 1), (2, 1)):
    best = None
    for env_rel, att, rel, env_att in itertools.product((2, 5.4, 10), (5, 11, 20), (40, 65, 100), (0, 2)):
        v = total((env_rel, att, rel, env_att), order, domain)
        if best is None or v < best[0]: best = (v, (env_rel, att, rel, env_att))
    r = minimize(lambda p: total(p, order, domain), best[1], method='Nelder-Mead', options={'xatol': 0.05, 'fatol': 0.05, 'maxiter': 400})
    print(f'order {order} domain {"dB" if domain else "lin"}: env_rel {r.x[0]:.2f} att {r.x[1]:.2f} rel {r.x[2]:.2f} env_att {r.x[3]:.2f} ms -> score {r.fun:.1f}')
    total(r.x, order, domain, verbose=True)
    results.append((r.fun, order, domain, r.x))
results.sort(key=lambda t: t[0])
print('BEST:', results[0])
np.save(os.path.join(os.path.dirname(D.rstrip('/')), 'results44', 'comp_dyn_best.npy'), np.array([results[0][1], results[0][2], *results[0][3]]))
