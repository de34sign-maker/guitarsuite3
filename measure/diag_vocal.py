import sys, os; sys.path.insert(0,'/home/claude/gs44')
import numpy as np
from comp_core import *
D='/home/claude/gs44/data/GuitarSuite_tests_44.1k/'
x,sr=load(D+'05_in_vocal_44.wav'); r,_=load(D+'44_vocal_comp.wav')
w=44
def gain_track(y):
    n=len(x)//w; g=np.full(n,np.nan)
    for i in range(n):
        xi=np.abs(x[i*w:(i+1)*w]).max()
        if xi>0.003: g[i]=20*np.log10(np.abs(y[i*w:(i+1)*w]).max()/xi)
    return g
gr=gain_track(np.concatenate([r[1:],[0]]))
m1=comp_gen(x,sr,-29.74,4.96,5.0,8.31,13.93,68.13,1,0,0.0)
m2=comp_gen(x,sr,-29.74,4.96,5.0,11.85,6.87,29.1,2,0,0.0)
g1=gain_track(m1); g2=gain_track(m2)
ok=~np.isnan(gr); d1=g1-gr; d2=g2-gr
print('gain error (dB) order1: mean %.2f  rms %.2f  max %.2f'%(np.nanmean(d1),np.sqrt(np.nanmean(d1**2)),np.nanmax(np.abs(d1))))
print('gain error (dB) order2: mean %.2f  rms %.2f  max %.2f'%(np.nanmean(d2),np.sqrt(np.nanmean(d2**2)),np.nanmax(np.abs(d2))))
lvl=np.array([20*np.log10(np.abs(x[i*w:(i+1)*w]).max()+1e-9) for i in range(len(gr))])
rising=np.gradient(lvl)>1; falling=np.gradient(lvl)<-1; steady=~rising&~falling
for name,msk in (('rising',rising),('falling',falling),('steady',steady)):
    mm=msk&ok
    print(f'  {name:8s} n={mm.sum():5d}  order1 rms {np.sqrt(np.nanmean(d1[mm]**2)):.2f} mean {np.nanmean(d1[mm]):+.2f} | order2 rms {np.sqrt(np.nanmean(d2[mm]**2)):.2f} mean {np.nanmean(d2[mm]):+.2f}')
i0=int(np.nanargmax(np.abs(d1))); print('worst region at %.2f s'%(i0*w/sr))
s=max(0,i0-40)
print(' t(ms)  in(dBFS)  gain_aud  gain_m1  gain_m2')
for i in range(s,s+80,2): print(f' {i*w/sr*1000:7.1f}  {lvl[i]:7.1f}   {gr[i]:+6.1f}    {g1[i]:+6.1f}   {g2[i]:+6.1f}')
np.save('/home/claude/gs44/data/results44/vocal_gain_track.npy', np.stack([lvl,gr,g1,g2]))
