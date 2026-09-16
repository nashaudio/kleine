"""Retain Idiophonics evidence, listening fixtures and unchanged local website recordings."""
import hashlib
import json
import platform
import shutil
import subprocess

from compare_audio import ROOT, compare, describe, np, sf
from render_idiophonics import SOURCES, PRIMARY, LISTENING, folder_for
from analyse_creaking import main as analyse_creaking

AUDIO = ROOT/'farnell/audio'
EVIDENCE = AUDIO/'comparisons/idiophonics'
ONLINE = {'bouncing':'p07/bounce.wav','rolling':'p08/rolling.wav',
          'creaking':'p09/creaking.wav','boing':'p10/boing.wav'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    EVIDENCE.mkdir(parents=True,exist_ok=True)
    runs = {}
    for rate in [48000,44100]:
        source = ROOT/f'build/idiophonics/{rate}/results.json'
        data = json.loads(source.read_text())
        if missing := set(SOURCES)-set(data): raise ValueError(f'Missing {rate} Hz cases: {missing}')
        runs[str(rate)] = {case:data[case] for case in SOURCES}
        if not all(r.get('passed') for r in runs[str(rate)].values()):
            raise ValueError(f'Rerun all {rate} Hz cases with the current comparison gates')
    (EVIDENCE/'render-results.json').write_text(json.dumps(runs,indent=2)+'\n')
    analyse_creaking()

    originals = []
    for case, relative in ONLINE.items():
        source = ROOT/'farnell/zip'/relative
        destination = folder_for(case)/f'{case}-online.wav'
        shutil.copyfile(source,destination)
        assert sha(source) == sha(destination)
        originals.append(dict(source=str(source.relative_to(ROOT)),sha256=sha(source),
                              file=str(destination.relative_to(AUDIO)),gain=1,edit='none; complete local recording'))

    plots = {}
    for case in PRIMARY:
        paths = [folder_for(case)/f'{case}-{r}.wav' for r in ['pd','kleine']]
        reference = folder_for(case)/f'{"boing" if case.startswith("boing") else case}-online.wav'
        if reference.exists(): paths.append(reference)
        plots[case] = compare(paths,EVIDENCE/f'{case}.png')
        compare(paths,EVIDENCE/f'{case}-2048.png',fft=2048)
        print('Packaged',case,flush=True)

    # Raw float recordings remain at unity. Supply a quieter audition copy for
    # the source variant which itself exceeds full scale; use identical gain on both.
    for renderer in ['pd','kleine']:
        source=folder_for('boing-bulk')/f'boing-bulk-{renderer}.wav'
        samples,rate=sf.read(source)
        sf.write(source.with_name(f'boing-bulk-{renderer}-listening.wav'),samples*.8,rate,subtype='FLOAT')

    rows = ['# Idiophonics measurements','',
            'PD 0.55.2, compatibility 0.55, block 64; one voice; raw unity-gain bounces. '
            'No time alignment or fitted gain in these residuals. See the [series guide](../../idiophonics.md) '
            'for control recipes, source differences and acceptance scope.','',
            '| Case | 48 kHz residual (dB) | 44.1 kHz residual (dB) | Worst level delta (dB) | Worst 10 ms envelope error |',
            '| --- | ---: | ---: | ---: | ---: |']
    for case in SOURCES:
        pair=[runs[str(rate)][case]['comparison'] for rate in [48000,44100]]
        residual=lambda m:'identical' if m['identical_samples'] else f"{m['relative_error_db']:.2f}"
        rows.append(f"| {case} | {residual(pair[0])} | {residual(pair[1])} | {max(abs(m['level_delta_db']) for m in pair):.6f} | {max(m['envelope_relative_error'] for m in pair):.3%} |")
    rows += ['', '## Process measurements','',
             'CPU and RSS are sampled process totals (lower bounds), including startup and file I/O. '
             'The DSP column measures Kleine Processor calls, including buffer preparation. '
             'Concurrent trial processes can affect wall time. These are context, not a comparative DSP benchmark.','',
             '| Case (48 kHz) | PD / Kleine CPU (ms/audio s) | PD / Kleine elapsed (s) | PD / Kleine RSS (MiB) | Kleine DSP wall (ms/audio s) |',
             '| --- | ---: | ---: | ---: | ---: |']
    for case in PRIMARY:
        r=runs['48000'][case];p,k=r['pd'],r['kleine'];seconds=r['duration']
        dsp=json.loads(k['stdout'])['processing_wall_seconds']*1000/seconds
        rows.append(f"| {case} | {p['sampled_cpu_seconds']*1000/seconds:.2f} / {k['sampled_cpu_seconds']*1000/seconds:.2f} | {p['elapsed_seconds']:.3f} / {k['elapsed_seconds']:.3f} | {p['sampled_peak_rss_bytes']/2**20:.2f} / {k['sampled_peak_rss_bytes']/2**20:.2f} | {dsp:.3f} |")
    (EVIDENCE/'tables.md').write_text('\n'.join(rows)+'\n')

    audio={}
    files=[folder_for(case)/f'{case}-{r}.wav' for case in LISTENING for r in ['pd','kleine']]
    files += [AUDIO/item['file'] for item in originals]
    files += list(folder_for('boing').glob('*-listening.wav'))
    for path in files:
        samples,rate=sf.read(path,always_2d=True)
        assert np.isfinite(samples).all() and np.any(samples)
        audio[str(path.relative_to(AUDIO))]=dict(sha256=sha(path),channels=samples.shape[1],subtype=sf.info(path).subtype,**describe(samples,rate))
    source_files=list((ROOT/'farnell/klang/Idiophonics').rglob('*.k'))
    source_files += [ROOT/p for p in ['farnell/klang/Idiophonics/common.h','include/klang/pd.h','include/klang.h',
        'farnell/render.h','farnell/variants.h','tests/pd/idiophonics.h','tests/pd/idiophonics-smoke.cpp','tests/pd/creaking-force.inc','sounds/Harrier.h',
        'tools/render_idiophonics.py','tools/package_idiophonics.py','tools/render_farnell.py',
        'tools/render_artificial.py','tools/check_pd_primitives.py','tools/compare_audio.py','tools/analyse_creaking.py']]
    version=subprocess.run(['C:/Program Files/Pd/bin/pd.exe','-version'],capture_output=True,text=True,timeout=10)
    cpu=platform.processor()
    if platform.system()=='Windows':
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:
            cpu=winreg.QueryValueEx(key,'ProcessorNameString')[0].strip()
    manifest=dict(pd_version=(version.stdout+version.stderr).strip(),cpu=cpu,platform=platform.platform(),
        compiler='MSVC 19.51.36257.0, CMake x64-release',python=platform.python_version(),
        sample_rates=[48000,44100],retained_sample_rate=48000,block_size=64,voices=1,
        fixtures_per_rate=len(SOURCES),noise_seed=404933,control_random_seed=1,
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in source_files},audio=audio,online=originals,
        audition_gain={'boing-bulk-*-listening.wav':.8},
        notes=['Source patches are unchanged; generated copies add outlets, shared control messages and explicit seeds.',
            'Legacy variants explicitly swap pow~ inlets on modern PD, not an old executable.',
            'The uneven study explicitly enables its saved spigot toggle to run the delay loop.',
            'A3 retains its disconnected middle group and unused even ringer outlet; numeric argument object boxes load successfully on PD 0.55.2.',
            'Event scripts use a 64-sample grid. PD delay milliseconds are float-rounded a quarter-sample inside the block; Klang gestures reproduce that offset.',
            'Website performance controls and phases are unknown. Online plots are context, not sample-residual references.',
            'Creaking uses a force contour fitted to the website recording; the original held-force control test is retained separately. See creaking-performance.json for inferred control and first-contact evidence.',
            'Numerical/visual trial validation and listening fixtures; no human listening verdict or universal parameter/rate/seed parity claim.'])
    (AUDIO/'idiophonics-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    smoke=ROOT/'build/idiophonics/smoke-results.txt'
    if smoke.exists():shutil.copyfile(smoke,EVIDENCE/'smoke-results.txt')


if __name__=='__main__':main()
