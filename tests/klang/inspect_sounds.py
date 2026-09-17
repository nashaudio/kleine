"""Package check_sounds.py results without recompiling: float WAVs, PNGs and HTML.

Exact, unaligned, all-channel comparisons are the default. Optional tolerances
are numerical criteria only; changed audio still requires listening judgement.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import html
import json
import math
import os
from pathlib import Path
import re
import sys
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[2]
os.environ.setdefault('MPLCONFIGDIR',str(ROOT/'build/matplotlib'))
sys.path.insert(0, str(ROOT/'build/trials/python'))
import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from scipy import signal


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(a, b, atol=0., rtol=0.):
    same_shape = a.shape == b.shape
    finite = bool(np.isfinite(a).all() and np.isfinite(b).all())
    exact = same_shape and finite and bool(a.size) and bool(np.array_equal(a, b))
    delta = a.astype(np.float64)-b.astype(np.float64) if same_shape and finite and a.size else None
    return dict(shape_equal=same_shape, finite=finite, sample_identical=exact,
                equivalent=same_shape and finite and bool(a.size) and bool(np.allclose(a,b,atol=atol,rtol=rtol)),
                residual_peak=float(np.max(np.abs(delta))) if delta is not None else None,
                residual_rms=float(np.sqrt(np.mean(delta*delta))) if delta is not None else None)


def levels(x, rate):
    n=max(1, round(rate*.01))
    starts=np.arange(0, len(x), n)
    counts=np.minimum(n,len(x)-starts)
    square=x.astype(np.float64)**2
    rms=np.sqrt(np.add.reduceat(square,starts)/counts)
    peak=np.maximum.reduceat(np.abs(x),starts)
    return (starts+counts/2)/rate, rms, peak


def plots(items, rate, folder, fft, hop):
    channels=items[0][1].shape[1]
    figure, axes=plt.subplots(channels,1,figsize=(12,3*channels),squeeze=False)
    for c in range(channels):
        ax=axes[c,0]
        for label,x in items:
            t,rms,peak=levels(x[:,c],rate)
            line,=ax.plot(t,rms,label=f'{label} RMS',linewidth=1)
            ax.plot(t,peak,':',color=line.get_color(),label=f'{label} peak',linewidth=1)
        ax.set(xlabel='Seconds',ylabel='Linear amplitude',title=f'Channel {c+1} — 10 ms RMS / peak')
        ax.grid(alpha=.2); ax.legend(fontsize=8)
    figure.tight_layout(); figure.savefig(folder/'levels.png',dpi=120); plt.close(figure)
    figure,axes=plt.subplots(channels,len(items),figsize=(7*len(items),3*channels),squeeze=False,
                             layout='constrained')
    maximum=max(float(np.max(np.abs(x))) for _,x in items)
    upper=max(0.,20*math.log10(max(maximum,1e-20)))
    for col,(label,x) in enumerate(items):
        for c in range(channels):
            size=min(fft,len(x))
            freq,time,power=signal.spectrogram(x[:,c],rate,window='hann',nperseg=size,
                noverlap=max(0,size-min(hop,size)),nfft=size,detrend=False,scaling='spectrum',mode='psd')
            db=10*np.log10(np.maximum(power,1e-20))
            ax=axes[c,col]
            mesh=ax.pcolormesh(time,freq/1000,db,shading='auto',vmin=-100,vmax=upper,cmap='magma')
            ax.set(xlabel='Seconds',ylabel='kHz',title=f'{label} — channel {c+1}',ylim=(0,rate/2000))
    figure.colorbar(mesh,ax=axes.ravel().tolist(),label='dB re 1² (Hann power spectrum)')
    figure.savefig(folder/'sonogram.png',dpi=120); plt.close(figure)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('results',type=Path)
    ap.add_argument('--output',type=Path,help='Default: inspection/ beside results.json')
    ap.add_argument('--source',action='append',help='Limit packaging to repository-relative source paths')
    ap.add_argument('--all-plots',action='store_true',help='Also plot sample-identical pairs')
    ap.add_argument('--atol',type=float,default=0.)
    ap.add_argument('--rtol',type=float,default=0.)
    ap.add_argument('--fft',type=int,default=16384)
    ap.add_argument('--hop',type=int,default=1024)
    args=ap.parse_args()
    if not all(math.isfinite(x) and x>=0 for x in (args.atol,args.rtol)) or args.fft<2 or args.hop<1:
        ap.error('Tolerances must be finite/nonnegative; FFT >= 2 and hop >= 1.')
    source=args.results.resolve(strict=True)
    output=(args.output or source.parent/'inspection').resolve()
    if not output.is_relative_to(ROOT/'build'): ap.error('Output must be under build/.')
    output.mkdir(parents=True,exist_ok=True)
    data=json.loads(source.read_text())
    selected=set(p.replace('\\','/') for p in args.source or [])
    known={row['source'] for row in data['results']}
    if selected-known: ap.error('Unknown sources: '+', '.join(sorted(selected-known)))
    groups={}; failures=[]
    for row in data['results']:
        if selected and row['source'] not in selected: continue
        if not row['compiled']:
            failures.append({k:row.get(k) for k in ('header','mode','source','model','errors','log')})
        if row['mode']!='host-fixture': continue
        for render in row['renders']:
            groups.setdefault((row['source'],row['model'],render['rate']),{})[row['header']]=render
    report=dict(generated=datetime.now(timezone.utc).isoformat(),input=str(source),input_sha256=digest(source),
                compiler=data.get('compiler_version'),flags=data.get('flags'),header_sha256=data.get('header_sha256'),
                source_sha256=data.get('source_sha256'),recipe=data.get('recipe'),
                policy=dict(atol=args.atol,rtol=args.rtol,alignment_samples=0,gain=1,
                            channels='all',fft=args.fft,hop=args.hop,window='hann',level_bin_ms=10),
                compile_failures=failures,regressions=data.get('regressions',[]),cases=[])
    esc=lambda v: html.escape(str(v))
    def link(path): return quote(os.path.relpath(path,output).replace('\\','/'),safe='/')
    blocks=[]
    for index,((model_source,model,rate),renders) in enumerate(sorted(groups.items())):
        folder=output/(f'{index:03d}-'+re.sub(r'[^\w-]','_',model)+f'-{rate}')
        folder.mkdir(exist_ok=True)
        case=dict(source=model_source,model=model,rate=rate,audio={},comparisons=[],issues=[])
        arrays={}; players=[]
        for label,render in renders.items():
            path=ROOT/render['path']
            if render['returncode'] or not path.exists():
                case['issues'].append(f'{label}: failed or missing render'); continue
            channels=render.get('channels',1)
            raw=np.fromfile(path,dtype='<f4')
            if not raw.size or channels<1 or raw.size%channels:
                case['issues'].append(f'{label}: empty or malformed frames'); continue
            x=raw.reshape(-1,channels)
            actual=digest(path)
            if render.get('sha256') and actual!=render['sha256']:
                case['issues'].append(f'{label}: render changed since test'); continue
            if not np.isfinite(x).all():
                case['issues'].append(f'{label}: nonfinite audio'); continue
            if render.get('expected_samples',raw.size)!=raw.size:
                case['issues'].append(f'{label}: unexpected sample count')
            if render.get('repeat_identical') is False:
                case['issues'].append(f'{label}: repeat render differed')
            arrays[label]=x
            wav=folder/(label+'.wav')
            sf.write(wav,x,rate,subtype='FLOAT')
            case['audio'][label]=dict(wav=link(wav),raw=str(path),sha256=actual,frames=len(x),channels=channels,
                peak=float(np.max(np.abs(x))),rms=float(np.sqrt(np.mean(x.astype(np.float64)**2))),
                over_unity=int(np.count_nonzero(np.abs(x)>1)))
            players.append(f'<div>{esc(label)}: <a href="{link(wav)}">WAV</a> '
                           f'<audio controls preload="none" src="{link(wav)}"></audio></div>')
        if 'candidate' in arrays:
            for label,x in arrays.items():
                if label=='candidate': continue
                case['comparisons'].append(dict(reference=label,**compare(arrays['candidate'],x,args.atol,args.rtol)))
        comps=case['comparisons']
        status=('INVALID' if case['issues'] else 'UNPAIRED' if not comps else
                'IDENTICAL' if all(c['sample_identical'] for c in comps) else
                'WITHIN TOLERANCE' if all(c['equivalent'] for c in comps) else 'CHANGED — listen')
        case['status']=status
        details=''
        if arrays and (args.all_plots or status!='IDENTICAL'):
            # Different channel layouts are plotted independently, never silently mixed.
            layouts={x.shape[1] for x in arrays.values()}
            for channels in sorted(layouts):
                plot_dir=folder/f'{channels}ch'; plot_dir.mkdir(exist_ok=True)
                items=[(label,x) for label,x in arrays.items() if x.shape[1]==channels]
                stamp=plot_dir/'inputs.json'
                key=dict(script=digest(Path(__file__)),rate=rate,fft=args.fft,hop=args.hop,
                         renders={label:case['audio'][label]['sha256'] for label,_ in items})
                if (not stamp.exists() or json.loads(stamp.read_text())!=key or
                        not all((plot_dir/name).exists() for name in ('levels.png','sonogram.png'))):
                    plots(items,rate,plot_dir,args.fft,args.hop)
                    stamp.write_text(json.dumps(key,indent=2)+'\n')
                case.setdefault('plots',[]).extend(link(plot_dir/name) for name in ('levels.png','sonogram.png'))
                details+=''.join(f'<a href="{link(plot_dir/name)}"><img loading="lazy" src="{link(plot_dir/name)}"></a>'
                                 for name in ('levels.png','sonogram.png'))
        else:
            details='<p>Plots omitted: samples are identical.</p>' if status=='IDENTICAL' else ''
        report['cases'].append(case)
        blocks.append(f'<details><summary>{esc(model)} · {rate} Hz · {esc(status)}</summary>'
                      f'<p>{esc(model_source)}</p>'+''.join(players)+
                      f'<pre>{esc(json.dumps(dict(comparisons=comps,audio=case["audio"],issues=case["issues"]),indent=2))}</pre>'+details+'</details>')
    counts={s:sum(c['status']==s for c in report['cases']) for s in sorted({c['status'] for c in report['cases']})}
    report['counts']=counts
    (output/'report.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
    failed=''.join(f'<li>{esc(f["header"])} / {esc(f["mode"])} / {esc(f["source"])}: '
                   f'<a href="{link(ROOT/f["log"])}">compiler log</a><pre>{esc(chr(10).join(f["errors"] or []))}</pre></li>' for f in failures)
    page='''<!doctype html><html lang="en"><meta charset="utf-8"><title>Klang audio regression inspection</title>
<style>body{font:16px system-ui;margin:2em;max-width:1200px;background:#fafafa;color:#222}details{padding:1em;border:1px solid #ccc;margin:1em 0}summary{cursor:pointer;font-weight:bold}img{width:100%;height:auto}pre{white-space:pre-wrap;overflow-wrap:anywhere;font-size:12px}audio{vertical-align:middle;margin:.5em}</style>
<h1>Klang audio regression inspection</h1><p>Original levels; no normalisation or alignment. Comparisons include every channel. WAVs use 32-bit float: browser playback may clip levels above ±1; inspect the original WAVs in an audio editor.</p>'''
    page+=f'<p>{esc(counts)} · {len(failures)} compile failures (including existing baseline failures).</p>'
    page+=f'<p>Numerical tolerance: absolute {args.atol:g}, relative {args.rtol:g}. Numerical agreement does not establish perceptual acceptance. <a href="report.json">Full metadata</a></p>'
    page+=f'<details><summary>Build / recipe / regressions</summary><pre>{esc(json.dumps({k:report[k] for k in ("compiler","flags","header_sha256","recipe","policy","regressions")},indent=2))}</pre></details>'
    page+=''.join(blocks)+f'<details><summary>Compile failures</summary><ul>{failed}</ul></details></html>'
    (output/'index.html').write_text(page,encoding='utf-8')
    print(output/'index.html'); print(counts)
    return 1 if any(c['issues'] for c in report['cases']) else 0


if __name__=='__main__': raise SystemExit(main())
