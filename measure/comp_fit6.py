import sys; sys.path.insert(0,'/home/claude/gs44')
from comp_core4 import *
from scipy.optimize import minimize
import itertools
D='/home/claude/gs44/data/GuitarSuite_tests_44.1k/'
xv,sr=load(D+'05_in_vocal_44.wav'); rv,_=load(D+'44_vocal_comp.wav'); gv,lv=oracle(xv,rv)
xs,_=load(D+'04_comp_steps_1k.wav')
steps={}
for a in (25,100):
    r,_=load(D+f'44_comp{a}.wav'); g,l=oracle(xs,r); steps[a]=(g,l,~np.isnan(g)&(l>-60))
m=~np.isnan(gv)&(lv>-60)
# statics for amount a, with free offsets: thr_off (dB), ratio scale, makeup fixed a/20
def st(a,p): return (-0.29874*a+0.1304+p[0], (1.9291+0.03032*a)*p[1], a/20.0)
def f(p, sq, useG, verbose=False):
    ea,er,ga,gr,toff,rsc=p
    if ea<0.01 or er<0.1 or rsc<0.5 or rsc>2 or (useG and (ga<0.01 or gr<0.1)): return 1e3
    if not useG: ga=0.0; gr=1.0
    thr,ratio,mk=st(100,(toff,rsc))
    gm=gain_model4(xv,sr,thr,ratio,mk,sq,ea,er,ga,gr,W); e=np.sqrt(np.mean((gm[m]-gv[m])**2))
    es=[]
    for a in (25,100):
        thr,ratio,mk=st(a,(toff,rsc)); g,l,mm=steps[a]; g2=gain_model4(xs,sr,thr,ratio,mk,sq,ea,er,ga,gr,W); es.append(np.sqrt(np.mean((g2[mm]-g[mm])**2)))
    if verbose: print('     vocal %.3f  steps25 %.3f steps100 %.3f'%(e,es[0],es[1]),flush=True)
    return 3*e+sum(es)
res=[]
for sq,useG in ((0,False),(1,False),(0,True),(1,True)):
    best=None
    grid=itertools.product((3,8,15,30),(30,80,150),(2,8) if useG else (0,),(30,90) if useG else (1,),(-4,-2,0),(1.0,))
    for p in grid:
        v=f(p,sq,useG)
        if best is None or v<best[0]: best=(v,p)
    r=minimize(lambda p: f(p,sq,useG), best[1], method='Nelder-Mead', options={'xatol':0.02,'fatol':0.001,'maxiter':900})
    print(f'det {"x^2" if sq else "|x|"} gainSmooth {useG}: env_att {r.x[0]:.2f} env_rel {r.x[1]:.2f} g_att {r.x[2]:.2f} g_rel {r.x[3]:.2f} thr_off {r.x[4]:+.2f} ratio_scale {r.x[5]:.3f} -> {r.fun:.3f}',flush=True); f(r.x,sq,useG,True)
    res.append((r.fun,sq,useG,r.x))
res.sort(key=lambda t:t[0]); print('BEST',res[0],flush=True)
np.save('/home/claude/gs44/data/results44/comp_fit6_best.npy', np.array([res[0][1],int(res[0][2]),*res[0][3]]))
