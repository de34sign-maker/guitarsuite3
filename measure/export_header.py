"""Generate GuitarSuiteData.h from 44.1 kHz measurements.
usage: python3 -I export_header.py <results44_dir> <out_header> [--fuzz-points JSON] [--comp-points JSON]
"""
import sys, os, json, numpy as np

R = sys.argv[1]; OUTP = sys.argv[2]
args = dict(zip(sys.argv[3::2], sys.argv[4::2]))

filt = np.load(os.path.join(R, 'filt34_fir256.npy'))
box = np.load(os.path.join(R, 'box44_ir.npy'))
delay = int(np.load(os.path.join(R, 'box44_delay.npy')))
box_full = np.concatenate([np.zeros(delay), box])  # embed the pre-delay as leading zeros, like the 48k header
fuzz_amt = json.loads(args.get('--fuzz-amt', '[10,30,50,70,90,100]'))
fuzz_lnk = json.loads(args['--fuzz-lnk'])
comp_amt = json.loads(args.get('--comp-amt', '[25,50,75,100]'))
comp_pts = json.loads(args['--comp-pts'])  # [[thr,ratio,makeup],...]

def arr(name, v, per=6):
    s = f'static const float {name}[{len(v)}] = {{\n'
    for i in range(0, len(v), per):
        s += '    ' + ', '.join(f'{x:.9e}f' for x in v[i:i + per]) + ',\n'
    return s + '};\n'

h = '// Auto-generated from measurements of Adobe Audition 3.0 Guitar Suite at 44.1 kHz (2 Nov 2026)\n'
h += '#pragma once\n'
h += 'static const double kDataSampleRate = 44100.0;\n'
h += arr('kFilterFIR_2042_res34', filt)
h += arr('kBoxIR_model9', box_full)
h += arr('kFuzzAmount', np.array(fuzz_amt, float), 8)
h += arr('kFuzzLogK', np.array(fuzz_lnk, float), 8)
h += arr('kCompAmount', np.array(comp_amt, float), 8)
h += arr('kCompThrDb', np.array([p[0] for p in comp_pts]), 8)
h += arr('kCompRatio', np.array([p[1] for p in comp_pts]), 8)
h += arr('kCompMakeupDb', np.array([p[2] for p in comp_pts]), 8)
open(OUTP, 'w').write(h)
print('wrote', OUTP, 'filter taps', len(filt), 'box taps', len(box_full))
