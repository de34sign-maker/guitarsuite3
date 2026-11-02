import sys; sys.path.insert(0,'/home/claude/gs44')
from comp_core3 import *
from scipy.optimize import minimize
import itertools
D='/home/claude/gs44/data/GuitarSuite_tests_44.1k/'
xv,sr=load(D+'05_in_vocal_44.wav'); rv,_=load(D+'44_vocal_comp.wav'); gv,lv=oracle(xv,rv)
xs,_=load(D+'04_comp_steps_1k.wav')
steps={}
for a in (25,100):
    r,_=load(D+f'44_comp{a}.wav'); g,l=oracle(xs,r); steps[a]=(g,l,~np.isnan(g)&(l>-60))
m=~np.isnan(gv)&(lv>-60)
def st(a): return (-0.29874*a+0.1304, 1.9291+0.03032*a, a/20.0)
def f(p, det, order, verbose=False):
    win,dr,at,rl=p
    if win<0.05 or dr<0.05 or at<0.05 or rl<0.5 or win>60: return 1e3
    thr,ratio,mk=st(100)
    gm=gain_model3(xv,sr,thr,ratio,mk,det,win,dr,at,rl,W,order); e=np.sqrt(np.mean((gm[m]-gv[m])**2))
    es=[]
    for a in (25,100):
        thr,ratio,mk=st(a); g,l,mm=steps[a]; g2=gain_model3(xs,sr,thr,ratio,mk,det,win,dr,at,rl,W,order); es.append(np.sqrt(np.mean((g2[mm]-g[mm])**2)))
    if verbose: print('     vocal %.3f  steps25 %.3f steps100 %.3f'%(e,es[0],es[1]))
    return 3*e+sum(es)
res=[]
for det,order in itertools.product((0,1,2,3),(1,2)):
    best=None
    for p in itertools.product((1,3,8,20),(2,8),(5,12,25),(40,90)):
        v=f(p,det,order)
        if best is None or v<best[0]: best=(v,p)
    r=minimize(lambda p: f(p,det,order), best[1], method='Nelder-Mead', options={'xatol':0.02,'fatol':0.001,'maxiter':500})
    name=['boxRMS','peakHold','1poleRMS','boxRMS+rel'][det]
    print(f'{name:10s} order {order}: win {r.x[0]:.2f} det_rel {r.x[1]:.2f} att {r.x[2]:.2f} rel {r.x[3]:.2f} -> {r.fun:.3f}'); f(r.x,det,order,True)
    res.append((r.fun,det,order,r.x))
res.sort(key=lambda t:t[0]); print('BEST',res[0])
np.save('/home/claude/gs44/data/results44/comp_fit4_best.npy', np.array([res[0][1],res[0][2],*res[0][3]]))
