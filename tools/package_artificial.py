"""Package the Artificial Sounds trial, preserving original recording samples.

Run render_artificial.py --retain and its 44100 Hz validation first.
Uses the local companion mirror only; never reads or publishes the book.
"""
import hashlib
import json
from pathlib import Path
import platform
import shutil
import subprocess

from compare_audio import ROOT, compare, describe, np, sf, signal
from render_artificial import PRIMARY_CASES, ALL_CASES

AUDIO = ROOT/'farnell/audio'
EVIDENCE = AUDIO/'comparisons/artificial'
FOLDERS = {'pedestrians':'24-pedestrians','dtmf':'26-dtmf-tones','police':'28-police','police-legacy':'28-police'}
EXCERPTS = [
    ('p01/pedestrian-beeps.wav',.05,5.2,'24-pedestrians/pedestrians-online.wav'),
    ('p03/dtmf.wav',.2,9.9,'26-dtmf-tones/dtmf-online.wav'),
    ('p04/alarms.wav',.4,52.9,'27-alarms/alarms-online.wav'),
    ('p04/alarms.wav',35.1,52.9,'27-alarms/alarm07-online.wav'),
    ('p05/police.wav',.02,22.8,'28-police/police-online.wav'),
]


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def reference_laws():
    """Compare spectral shape separately from fitted gain in a slow-sweep interval."""
    original=ROOT/'farnell/zip/p05/police.wav'
    online,rate=sf.read(original,always_2d=True)
    f,reference=signal.welch(online[:10*rate,0],rate,nperseg=16384)
    result={}
    for case in ['police','police-legacy']:
        samples,_=sf.read(AUDIO/'28-police'/f'{case}-pd.wav')
        _,power=signal.welch(samples[:10*rate],rate,nperseg=16384)
        mask=(f>200)&(f<6000)&(reference>reference.max()*1e-4)&(power>power.max()*1e-4)
        delta=10*np.log10(reference[mask]/power[mask]);gain=float(np.median(delta))
        result[case]=dict(fitted_psd_gain_db=gain,median_shape_difference_db=float(np.median(abs(delta-gain))))
    return dict(source=str(original.relative_to(ROOT)),sha256=sha(original),interval_seconds=[0,10],
                fft=16384,frequency_range_hz=[200,6000],active_threshold_db=-40,cases=result,
                note='Welch spectral comparison only; performance timing/phase differ. Fitted gain is analytical and is not applied to any delivered WAV. This does not establish the authoring PD version or prove post-processing.')


def main():
    EVIDENCE.mkdir(parents=True,exist_ok=True)
    excerpts=[]
    for relative,start,stop,output in EXCERPTS:
        source=ROOT/'farnell/zip'/relative; destination=AUDIO/output
        samples,rate=sf.read(source,dtype='int16',always_2d=True)
        clip=samples[round(start*rate):round(stop*rate)]
        sf.write(destination,clip,rate,subtype='PCM_16')
        restored,_=sf.read(destination,dtype='int16',always_2d=True)
        assert np.array_equal(clip,restored)
        excerpts.append(dict(source=str(source.relative_to(ROOT)),source_sha256=sha(source),file=output,
                             start_seconds=start,stop_seconds=stop,sample_rate=rate,channels=clip.shape[1],gain=1))

    montages=[]
    for renderer in ['pd','kleine']:
        parts=[];timeline=[];frame=0
        for i in range(1,8):
            case=f'alarm0{i}';samples,rate=sf.read(AUDIO/'27-alarms'/f'{case}-{renderer}.wav')
            assert rate==48000
            if i>1:parts.append(np.zeros(rate//2));frame+=rate//2
            timeline.append(dict(case=case,start_seconds=frame/rate,stop_seconds=(frame+len(samples))/rate))
            parts.append(samples);frame+=len(samples)
        output=AUDIO/'27-alarms'/f'alarms-{renderer}.wav'
        sf.write(output,np.concatenate(parts),48000,subtype='FLOAT')
        montages.append(dict(file=str(output.relative_to(AUDIO)),gap_seconds=.5,timeline=timeline))

    plots={}
    for case in PRIMARY_CASES:
        folder=FOLDERS.get(case,'27-alarms')
        paths=[AUDIO/folder/f'{case}-{r}.wav' for r in ['pd','kleine']]
        reference=AUDIO/folder/f'{"police" if case=="police-legacy" else case}-online.wav'
        if reference.exists():paths.append(reference)
        paths=[p.relative_to(ROOT) for p in paths]
        plots[case]=compare(paths,EVIDENCE/f'{case}.png')
        if case in ('pedestrians','dtmf','alarm07'):compare(paths,EVIDENCE/f'{case}-2048.png',fft=2048)
        print('Packaged',case,flush=True)
    compare([Path('farnell/audio/27-alarms')/f'alarms-{r}.wav' for r in ['pd','kleine','online']],EVIDENCE/'alarms.png')

    runs={}
    for rate in [48000,44100]:
        path=ROOT/f'build/artificial/{rate}/results.json'
        results=json.loads(path.read_text())
        missing=set(ALL_CASES)-set(results)
        if missing:raise ValueError(f'Missing {rate} Hz results: {sorted(missing)}')
        runs[str(rate)]={case:results[case] for case in ALL_CASES}
    (EVIDENCE/'render-results.json').write_text(json.dumps(runs,indent=2)+'\n')
    laws=reference_laws();(EVIDENCE/'police-reference.json').write_text(json.dumps(laws,indent=2)+'\n')
    table=['# Artificial Sounds measurements','',
           'PD 0.55.2, block 64; raw residuals have no alignment or gain fitting. '
           'Component waveforms and encoded detector states are diagnostics, not listening fixtures. '
           'Both sample rates construct fresh models; live sample-rate changes are not established.','',
           '| Case | 48 kHz residual (dB) | 44.1 kHz residual (dB) | 48 kHz level delta (dB) |',
           '| --- | ---: | ---: | ---: |']
    def residual(m):return 'identical' if m['identical_samples'] else f"{m['relative_error_db']:.2f}"
    for case in ALL_CASES:
        a=runs['48000'][case]['comparison'];b=runs['44100'][case]['comparison']
        table.append(f"| {case} | {residual(a)} | {residual(b)} | {a['level_delta_db']:.6f} |")
    table+=['','Process CPU/RSS include startup and I/O and are sampled lower bounds. '
            'Kleine DSP wall time covers Processor::process, including buffer preparation. '
            'These single runs do not establish relative PD/Klang DSP efficiency.','',
            '| Case (48 kHz) | PD / Kleine CPU (ms/audio s) | PD / Kleine peak RSS (MiB) | Kleine DSP wall (ms/audio s) |',
            '| --- | ---: | ---: | ---: |']
    for case in PRIMARY_CASES:
        r=runs['48000'][case];p,k=r['pd'],r['kleine'];duration=r['duration']
        dsp=json.loads(k['stdout'])['processing_wall_seconds']*1000/duration
        table.append(f"| {case} | {p['sampled_cpu_seconds']*1000/duration:.2f} / {k['sampled_cpu_seconds']*1000/duration:.2f} | {p['sampled_peak_rss_bytes']/2**20:.2f} / {k['sampled_peak_rss_bytes']/2**20:.2f} | {dsp:.3f} |")
    (EVIDENCE/'tables.md').write_text('\n'.join(table)+'\n')
    files=[]
    for folder in ['24-pedestrians','26-dtmf-tones','27-alarms','28-police']:files+=list((AUDIO/folder).glob('*.wav'))
    files += [AUDIO/'25-phone-tones'/f'{case}-{r}.wav' for case in ['ringback-bulk','phone-effects'] for r in ['pd','kleine']]
    audio={}
    for path in sorted(files):
        samples,rate=sf.read(path,always_2d=True)
        assert np.isfinite(samples).all() and np.any(samples) and np.max(abs(samples))<1
        audio[str(path.relative_to(AUDIO))]=dict(sha256=sha(path),channels=samples.shape[1],subtype=sf.info(path).subtype,**describe(samples,rate))
    sources=list((ROOT/'farnell/klang/Artificial Sounds').rglob('*.k'))
    sources += [ROOT/p for p in ['farnell/render.h','include/klang.h','include/klang/pd.h','tests/pd/primitive.h',
                                'tests/pd/number-match.pd','tools/render_artificial.py','tools/package_artificial.py','tools/compare_audio.py']]
    version=subprocess.run(['C:/Program Files/Pd/bin/pd.exe','-version'],capture_output=True,text=True,timeout=10)
    cpu=platform.processor()
    if platform.system()=='Windows':
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:cpu=winreg.QueryValueEx(key,'ProcessorNameString')[0].strip()
    manifest=dict(pd_version=(version.stdout+version.stderr).strip(),platform=platform.platform(),cpu=cpu,
                  compiler='MSVC 19.51.36257.0, CMake x64-release',python=platform.python_version(),
                  sample_rates=[48000,44100],retained_audio_sample_rate=48000,block_size=64,voices=1,
                  fixtures_per_rate=len(ALL_CASES),source_sha256={str(p.relative_to(ROOT)):sha(p) for p in sources},
                  excerpts=excerpts,montages=montages,audio=audio,
                  reconstruction='Combined phone demos replace missing list-emath/list-dotprod number matchers with tests/pd/number-match.pd; original PD files are unchanged.',
                  notes=['Original PCM excerpt samples retained exactly; no normalisation, resampling, fades or post-processing.',
                         'Scripts disable saved GUI startup values where controls are explicitly scheduled.',
                         'Events use a shared 64-sample grid; PD delay timestamps sit 0.25 samples inside that block to avoid floating-point boundary ambiguity.',
                         'Police legacy means explicit pow~ inlet swaps (or the correctly wired bulk source), not an old PD executable.',
                         'Graph/FFT display patches are covered by their waveform outputs and the external analysis tools; no general rfft~/GUI port is claimed.'])
    (AUDIO/'artificial-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__':main()
