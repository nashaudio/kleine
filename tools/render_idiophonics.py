"""Paired chapter 29-33 trials; instrumented references and scratch WAVs live in build.

Original PD files are preserved. See farnell/audio/idiophonics.md for variant scope.
"""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

from render_farnell import ROOT, records, expose_output, wrapper, run, np, sf
from render_artificial import extract, inlet, swap_pow, reference_errors
from check_pd_primitives import metrics
from scipy import signal

SOURCES = {
    'bouncing': 'zip/p07/bouncing.pd', 'bouncing-bulk': 'pd/BOUNCINGBALL/bb1.pd',
    'bouncing-legacy': 'pd/BOUNCINGBALL/bb1.pd',
    'rolling': 'zip/p08/rolling1.pd', 'rolling-bulk': 'pd/ROLLING/rolling1.pd',
    'tincan': 'pd/ROLLING/tincan.pd',
    'uneven2': 'zip/p08/uneven2.pd', 'uneven2-bulk': 'pd/ROLLING/uneven2.pd',
    'creaking': 'zip/p09/doorcreaker.pd', 'creaking-controls': 'zip/p09/doorcreaker.pd',
    'boing': 'zip/p10/twang.pd', 'boing-bulk': 'pd/MRBOINGY/twang.pd',
    'boing-legacy': 'pd/MRBOINGY/twang.pd',
}
SOURCES.update({f'boing-{s}':'zip/p10/twang.pd' for s in ['phase','pitch','clamped','free','frequency']})
SOURCES.update({f'bouncing-{s}':'zip/p07/bouncing.pd' for s in ['height','envelope']})
SOURCES.update({'bell-'+name:'pd/BELL/'+file+'.pd' for name,file in {
    'a0':'A0-bell-oscillator','a1':'A1-bell-envelope','a2':'A2-bell-partial','a3':'A3-bell-ratio',
    'a4':'A4-bell-telephone','partial':'partial','group':'group','testgroup':'testgroup'}.items()})
SOURCES['uneven']='pd/ROLLING/uneven.pd'
SOURCES.update({f'creaking-{s}':'zip/p09/doorcreaker.pd' for s in ['pulse','wood','panel']})
SOURCES['creaking-delay']='zip/p09/dfbef.pd'
PRIMARY = ['bouncing','rolling','tincan','uneven2','creaking','boing','boing-bulk','boing-legacy']
LISTENING = PRIMARY + ['creaking-controls','bouncing-bulk','bouncing-legacy','uneven','bell-a2','bell-a3','bell-a4','bell-group','bell-testgroup']


def acceptance(case, a, b, rate, m):
    """Trial limits distinguish sharp stochastic contacts from smooth deterministic signals."""
    size = rate // 100
    count = len(a) // size * size
    envelope = lambda x: np.sqrt(np.mean(x[:count].reshape(-1,size)**2,axis=1))
    ea, eb = envelope(a), envelope(b)
    _, pa = signal.welch(a,rate,nperseg=16384)
    _, pb = signal.welch(b,rate,nperseg=16384)
    active = pa > pa.max() * 1e-5
    m['envelope_relative_error'] = float(np.linalg.norm(ea-eb)/np.linalg.norm(ea))
    m['spectrum_median_abs_db'] = float(np.median(abs(10*np.log10(np.maximum(pb[active],1e-30)/pa[active]))))
    limits = dict(level_db=.002, residual_db=-70, envelope_relative=1, spectrum_db=1)
    if case.startswith('rolling') or case in ('creaking','creaking-controls'):
        limits = dict(level_db=.01, residual_db=-35, envelope_relative=.02, spectrum_db=.1)
    elif case == 'creaking-pulse':
        limits = dict(level_db=.01, residual_db=-25, envelope_relative=.02, spectrum_db=.1)
    passed = (abs(m['level_delta_db']) < limits['level_db'] and m['relative_error_db'] < limits['residual_db']
              and m['envelope_relative_error'] < limits['envelope_relative'] and m['spectrum_median_abs_db'] < limits['spectrum_db'])
    return passed, limits


def seed_sources(text):
    """Seed each canvas locally; retain object ordering and all existing connections."""
    stack = []
    def finish(lines):
        rows, count = expose_output('\n'.join(lines)+'\n')
        depth, index, targets = 0, 0, []
        for row in lines[1:]:
            if row.startswith('#N canvas'): depth += 1
            elif row.startswith('#X restore'):
                depth -= 1
                if depth == 0: index += 1
            elif depth == 0 and row.startswith(('#X obj ','#X msg ','#X text ','#X floatatom ','#X symbolatom ')):
                if re.match(r'#X obj \S+ \S+ (noise~|random)(?: |;)',row):
                    targets.append((index,404933 if 'noise~' in row else 1))
                index += 1
        # Do not expose nested DACs as a side effect of this transform.
        rows = lines[:]
        for target, seed in targets:
            rows += ['#X obj 800 10 loadbang;', f'#X msg 800 40 seed {seed};',
                     f'#X connect {count} 0 {count+1} 0;', f'#X connect {count+1} 0 {target} 0;']
            count += 2
        return rows
    for row in records(text):
        if row.startswith('#N canvas'): stack.append([row])
        elif row.startswith('#X restore'):
            child = finish(stack.pop()); stack[-1] += child + [row]
        else: stack[-1].append(row)
    return '\n'.join(finish(stack.pop()))+'\n'


def creaking_performance():
    """Approximate the website's two hand-operated force sweeps, not its random seed.

    Points describe the desired force after the source patch's 100 ms line slew.
    Send interpolated destinations at 100 ms intervals, one ramp ahead.
    See analyse_creaking.py for the independent pulse-interval estimate.
    """
    source=(ROOT/'tests/pd/creaking-force.inc').read_text()
    points=[tuple(map(float,pair)) for pair in re.findall(r'\{([\d.]+),([\d.]+)\}',source)]
    assert len(points)==39 and all(b[0]>a[0] for a,b in zip(points,points[1:]))
    t,force=zip(*points)
    return 7,[(round(step/10,1),'force',[round(float(np.interp((step+1)/10,t,force)),6)])
              for step in range(66)]


def recipe(case):
    if case=='bell-a0': return 2,[]
    if case=='bell-a3': return 7,[(.25,'ring',[1]),(5,'ring',[0])]
    if case.startswith('bell-'):return 4,[(.25,'trigger',[]),(1,'trigger',[]),(2,'trigger',[])]
    if case=='uneven':return 6,[(.25,'trigger',[]),(4,'stop',[])]
    if case.startswith('bouncing'): return 9,[(.25,'trigger',[]),(4.5,'trigger',[])]
    if case.startswith('rolling'): return 8,[(.25,'trigger',[]),(1,'trigger',[]),(2,'trigger',[]),(4,'trigger',[])]
    if case == 'tincan': return 4,[(.25,'trigger',[]),(1,'trigger',[]),(2,'trigger',[])]
    if case.startswith('uneven2'): return 8,[]
    if case in ('creaking-delay','creaking-wood','creaking-panel'):return 2,[]
    if case == 'creaking': return creaking_performance()
    if case in ('creaking-controls','creaking-pulse'): return 8,[(.25,'force',[.7]),(2,'force',[.95]),(3.5,'force',[.4]),(5,'force',[0]),(6,'force',[.8]),(7,'force',[0])]
    if case.startswith('boing'): return 10,[(.25,'trigger',[]),(3,'boing',[220,3]),(3,'trigger',[]),(6,'boing',[640,1]),(6,'trigger',[])]
    raise ValueError(case)


def patch(case, work):
    source = ROOT/'farnell'/SOURCES[case]
    text = source.read_text(); dependencies=[]
    if case.endswith('-legacy'): text=swap_pow(text)
    if case in ('creaking-wood','creaking-panel'):text=extract(text,'wood' if case.endswith('wood') else 'squarepanel')
    if case.startswith('bell-'):
        for dep in (ROOT/'farnell/pd/BELL').glob('*.pd'):
            if dep!=source:shutil.copyfile(dep,work/dep.name);dependencies.append(dep)
    text=seed_sources(text)
    lines,count=expose_output(text)
    if case=='uneven':
        count=inlet(lines,count,58,0,'trigger');count=inlet(lines,count,55,0,'stop')
        lines+=['#X obj 700 10 loadbang;','#X msg 700 40 1;',f'#X connect {count} 0 {count+1} 0;',f'#X connect {count+1} 0 56 1;']
    elif case.startswith('bell-'):
        if case=='bell-a3':inlet(lines,count,2,0,'ring')
        elif case=='bell-a4':
            count=inlet(lines,count,2,0,'trigger')
            lines += ['#X obj 700 50 loadbang;','#X msg 700 70 650;',f'#X connect {count} 0 {count+1} 0;',f'#X connect {count+1} 0 12 0;']
        elif case=='bell-a0':
            lines += ['#X obj 700 50 loadbang;','#X msg 700 70 440;','#X msg 700 90 0.3;',f'#X connect {count} 0 {count+1} 0;',f'#X connect {count} 0 {count+2} 0;',f'#X connect {count+1} 0 4 0;',f'#X connect {count+2} 0 2 1;']
        elif case=='bell-testgroup':inlet(lines,count,3,0,'trigger')
        else:
            message,target={'bell-a1':('800',4),'bell-a2':('521 0.7 732 0.45 934 0.25 800',5),'bell-partial':('521 0.7 800',3),'bell-group':('521 0.7 800 732 0.45 500 934 0.25 200',5)}[case]
            lines += ['#X obj 700 50 r audit-trigger;',f'#X msg 700 70 {message};',f'#X connect {count} 0 {count+1} 0;',f'#X connect {count+1} 0 {target} 0;']
    elif case.startswith('bouncing'):
        count=inlet(lines,count,24 if case in ('bouncing-bulk','bouncing-legacy') else 22,0,'trigger')
        if case in ('bouncing-height','bouncing-envelope'):
            lines=[row for row in lines if row not in ('#X connect 1 0 0 0;',)]
            lines.append(f'#X connect {16 if case.endswith("height") else 2} 0 0 0;')
    elif case.startswith('rolling'): inlet(lines,count,9,0,'trigger')
    elif case=='tincan': inlet(lines,count,5,0,'trigger')
    elif case.startswith('creaking'):
        if case in ('creaking','creaking-controls','creaking-pulse'):inlet(lines,count,0,0,'force')
        if case=='creaking-pulse':
            lines=[r for r in lines if r!='#X connect 5 0 1 0;'];lines.append('#X connect 3 0 1 0;')
        dependency=source.parent/'dfbef.pd';shutil.copyfile(dependency,work/dependency.name);dependencies.append(dependency)
    elif case.startswith('boing'):
        count=inlet(lines,count,3,0,'trigger')
        lines += ['#X obj 700 40 r audit-boing;','#X obj 700 70 unpack f f;',
                  f'#X connect {count} 0 {count+1} 0;',
                  f'#X connect {count+1} 0 {38 if case in ("boing-bulk","boing-legacy") else 35} 0;',
                  f'#X connect {count+1} 1 {37 if case in ("boing-bulk","boing-legacy") else 34} 0;']
    if case.startswith('boing-') and case not in ('boing-bulk','boing-legacy'):
        node={'phase':6,'pitch':38,'clamped':11,'free':12,'frequency':31}[case[6:]]
        lines=[row for row in lines if row!='#X connect 22 0 1 0;']
        lines.append(f'#X connect {node} 0 1 0;')
    (work/'model.pd').write_text('\n'.join(lines)+'\n')
    return [source]+dependencies


def render_case(case,rate,args):
    work=ROOT/'build/idiophonics'/str(rate)/case;work.mkdir(parents=True,exist_ok=True)
    sources=patch(case,work)
    duration,actions=recipe(case);frames=round(duration*rate)
    events=[(round(t*rate)//64*64,action,values) for t,action,values in actions]
    pd_audio,klang_audio=work/'pd.wav',work/'kleine.wav'
    lines,count=expose_output(wrapper('idiophonics',frames,rate,pd_audio))
    if case=='creaking-delay':lines=[r.replace('model;',r'model 4.52 0.05;') for r in lines]
    if case in ('creaking-delay','creaking-wood','creaking-panel'):
        lines+=['#X obj 700 20 array define impulse 1;',r'#X msg 700 40 \; impulse 0 1;','#X obj 700 60 tabplay~ impulse;','#X obj 700 80 t b b;',f'#X connect 0 0 {count+3} 0;',f'#X connect {count+3} 1 {count+1} 0;',f'#X connect {count+3} 0 {count+2} 0;',f'#X connect {count+2} 0 3 0;'];count+=4
    for frame,action,values in events:
        # Close to the exact sample, but inside the desired block (vline~ retains sub-sample timing).
        message=' '.join(map(str,values)) if values else 'stop' if action=='stop' else 'bang'
        lines += [f'#X obj 700 20 del {(frame+.25)*1000/rate:.12f};',
                  f'#X msg 700 50 \\; audit-{action} {message};',
                  f'#X connect 0 0 {count} 0;',f'#X connect {count} 0 {count+1} 0;']
        count+=2
    (work/'render.pd').write_text('\n'.join(lines)+'\n')
    script=work/'events.tsv';script.write_text(''.join(f'{f}\t{a}\t'+ '\t'.join(map(str,v))+'\n' for f,a,v in events))
    p=run([args.pd,'-nogui','-stderr','-noprefs','-noaudio','-nomidi','-batch','-r',str(rate),'-compatibility','0.55','-open',str(work/'render.pd')],work)
    if errors:=reference_errors(case,p['stderr']): raise RuntimeError(errors)
    model={'rolling-bulk':'rolling','uneven2-bulk':'uneven2','creaking-controls':'creaking'}.get(case,case)
    k=run([str(args.kleine.resolve()),'--render',model,str(klang_audio),str(duration),str(rate),'1',str(script)],work)
    a,ar=sf.read(pd_audio);b,br=sf.read(klang_audio)
    assert ar==br==rate and len(a)==len(b)==frames
    assert np.isfinite(a).all() and np.isfinite(b).all() and np.any(a) and np.any(b)
    m=metrics(a,b)
    passed, limits = acceptance(case,a,b,rate,m)
    result=dict(sample_rate=rate,duration=duration,frames=frames,block_size=64,
                sources={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
                events=[dict(frame=f,action=a,values=v) for f,a,v in events],pd=p,kleine=k,comparison=m,
                pd_clipped_samples=int(np.count_nonzero(abs(a)>=1)),kleine_clipped_samples=int(np.count_nonzero(abs(b)>=1)),
                passed=passed,limits=limits,review_only=getattr(args,'review',False),
                tail_rms=dict(pd=float(np.sqrt(np.mean(a[-rate//10:]**2))),kleine=float(np.sqrt(np.mean(b[-rate//10:]**2)))),
                noise_seed=404933,control_random_seed=1,gain=1,alignment_samples=0)
    (work/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'{case}: residual {m["relative_error_db"]:.2f} dB; level {m["level_delta_db"]:.4f} dB; peaks {m["pd_peak"]:.4f}/{m["klang_peak"]:.4f}',flush=True)
    if not passed and not result['review_only']: raise RuntimeError(f'{case}: comparison exceeds the documented trial limits; see {work / "result.json"}')
    if args.retain and case in LISTENING:
        folder=folder_for(case);folder.mkdir(parents=True,exist_ok=True)
        for source,renderer in [(pd_audio,'pd'),(klang_audio,'kleine')]:shutil.copyfile(source,folder/f'{case}-{renderer}.wav')
    return result


def folder_for(case):
    if case.startswith('bell-'):return ROOT/'farnell/audio/29-telephone-bell'
    chapter='30-bouncing' if case.startswith('bouncing') else '31-rolling' if case.startswith(('rolling','uneven','tincan')) else '32-creaking' if case.startswith('creaking') else '33-boing'
    return ROOT/'farnell/audio'/chapter


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pd',default='C:/Program Files/Pd/bin/pd.exe')
    parser.add_argument('--kleine',type=Path,default=ROOT/'build/x64-release/kleine.exe')
    parser.add_argument('--rate',type=int,default=48000)
    parser.add_argument('--cases',nargs='+',choices=list(SOURCES),default=list(SOURCES))
    parser.add_argument('--retain',action='store_true')
    parser.add_argument('--review',action='store_true',help='Collect raw differences without accepting failed parity; cannot retain audio')
    args=parser.parse_args()
    if args.review and args.retain: parser.error('--review cannot be combined with --retain')
    dest=ROOT/'build/idiophonics'/str(args.rate)/'results.json';dest.parent.mkdir(parents=True,exist_ok=True)
    results=json.loads(dest.read_text()) if dest.exists() else {}
    for case in args.cases:
        results[case]=render_case(case,args.rate,args)
        dest.write_text(json.dumps(results,indent=2)+'\n')


if __name__=='__main__':main()
