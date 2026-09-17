"""Retain chapter 25/29 listening fixtures and comparison evidence.

Run render_farnell.py first. Downloads only the two official WAVs when absent
from the build cache; checks their hashes before extracting unprocessed audio.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import shutil
import subprocess
import urllib.request

from compare_audio import ROOT, compare, describe, np, sf

BASE = 'https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/'
SOURCES = {
    '02/phonetones.wav': '61b0fc766cc1a6453ef1fa9cce070d890712659a94b25b64cbbb06071795d810',
    '06/telephonebell.wav': '9a6226e2f849cd5e5b95d96243eee230bc5572ffe88fd95f90afe230a37c540f',
}
CASES = ['dial', 'dial-web', 'dial-line', 'busy', 'busy-archive', 'ringback', 'pulse', 'bell']


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audio', type=Path, default=ROOT/'farnell/audio')
    parser.add_argument('--results', type=Path, default=ROOT/'build/trials/render-results.json')
    parser.add_argument('--pd', default='C:/Program Files/Pd/bin/pd.exe')
    args = parser.parse_args()
    audio_root = args.audio.resolve()
    phone = audio_root/'25-phone-tones'
    bell = audio_root/'29-telephone-bell'
    evidence = audio_root/'comparisons'
    for path in [phone, bell, evidence]: path.mkdir(parents=True, exist_ok=True)
    upstream = {}
    for name, expected in SOURCES.items():
        cache = ROOT/'build/trials/upstream'/name
        url = BASE+'p'+name
        local = ROOT/'farnell/zip'/('p'+name)
        if local.exists():
            if sha(local) != expected: raise ValueError(f'Local reference differs: {local}')
            cache.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(local, cache)
        elif not cache.exists():
            cache.parent.mkdir(parents=True, exist_ok=True)
            with urllib.request.urlopen(url, timeout=60) as response: cache.write_bytes(response.read())
        if sha(cache) != expected: raise ValueError(f'Upstream has changed: {url}')
        upstream[name] = dict(url=url, sha256=expected, bytes=cache.stat().st_size,
                              local_mirror=str(local.relative_to(ROOT)) if local.exists() else None)

    excerpts = [
        ('02/phonetones.wav', 1, 5, phone/'dial-web-online.wav'),
        ('02/phonetones.wav', 7.5, 14.5, phone/'pulse-online.wav'),
        ('02/phonetones.wav', 14.5, 25, phone/'ringback-online.wav'),
        ('02/phonetones.wav', 26, 33, phone/'busy-online.wav'),
        ('06/telephonebell.wav', 0, 4.8, bell/'bell-online.wav'),
    ]
    edits = []
    for source, start, stop, output in excerpts:
        samples, rate = sf.read(ROOT/'build/trials/upstream'/source, dtype='int16', always_2d=True)
        clip = samples[round(start*rate):round(stop*rate)]
        sf.write(output, clip, rate, subtype='PCM_16')
        restored, _ = sf.read(output, dtype='int16', always_2d=True)
        assert np.array_equal(clip, restored), 'Excerpt must preserve source PCM samples'
        edits.append(dict(file=str(output.relative_to(audio_root)), source=source,
                          start_seconds=start, stop_seconds=stop, gain=1, channels='original stereo'))

    # One easy-to-audition montage, plus individual unedited bounces. The same
    # cases/order appear in the website excerpts; event timings remain distinct.
    montage = []
    for renderer in ['pd', 'kleine', 'online']:
        pieces, timeline, frame = [], [], 0
        for i, case in enumerate(['dial-web', 'pulse', 'ringback', 'busy']):
            samples, rate = sf.read(phone/f'{case}-{renderer}.wav', always_2d=True)
            assert rate == 48000
            if i:
                pieces.append(np.zeros((rate//2, samples.shape[1])))
                frame += rate//2
            timeline.append(dict(case=case, start_seconds=frame/rate, stop_seconds=(frame+len(samples))/rate))
            pieces.append(samples); frame += len(samples)
        output = phone/f'phone-tones-{renderer}.wav'
        sf.write(output, np.concatenate(pieces), rate, subtype='PCM_16' if renderer == 'online' else 'FLOAT')
        montage.append(dict(file=str(output.relative_to(audio_root)), gap_seconds=.5, timeline=timeline))

    comparisons = {}
    for case in CASES:
        folder = bell if case == 'bell' else phone
        paths = [folder/f'{case}-{renderer}.wav' for renderer in ['pd', 'kleine']]
        if case in ['dial-web', 'pulse', 'ringback', 'busy', 'bell']:
            paths.append(folder/f'{case}-online.wav')
        # Relative labels make saved evidence portable between checkouts.
        import os
        previous = Path.cwd()
        try:
            os.chdir(ROOT)
            paths = [p.relative_to(ROOT) for p in paths]
            metrics = compare(paths, evidence/f'{case}.png')
            if case in ['pulse', 'bell']: compare(paths, evidence/f'{case}-2048.png', fft=2048)
        finally:
            os.chdir(previous)
        comparisons[case] = metrics
        print(case, f"residual {metrics['relative_error_db']:.2f} dB", flush=True)

    all_runs = json.loads(args.results.read_text())
    runs = {case: all_runs[case] for case in CASES}
    # Save the measured commands/logs alongside the files they produced.
    (evidence/'render-results.json').write_text(json.dumps(runs, indent=2)+'\n')
    cpu_name = platform.processor()
    if platform.system() == 'Windows':
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:
            cpu_name = winreg.QueryValueEx(key, 'ProcessorNameString')[0].strip()
    version = subprocess.run([args.pd, '-version'], capture_output=True, text=True, timeout=10)
    source_files = [ROOT/p for p in ['kleine.cpp', 'kleine.h', 'CMakeLists.txt', 'CMakePresets.json',
                                   'include/klang.h', 'include/klang/pd.h', 'include/klang/engine.h',
                                   'include/klang/audio.h', 'farnell/render.h', 'tests/pd/primitive.h']]
    for folder, pattern in [('farnell/klang', '*.k'), ('farnell/reference', '*.pd'),
                            ('farnell/pd/BELL', '*.pd'), ('farnell/pd/PHONETONES', '*.pd'), ('tests/pd', '*.pd'), ('tools', '*.py')]:
        source_files += list((ROOT/folder).rglob(pattern))
    assets = {}
    for path in sorted(audio_root.rglob('*.wav')):
        samples, rate = sf.read(path, always_2d=True)
        assets[str(path.relative_to(audio_root))] = dict(sha256=sha(path), channels=samples.shape[1],
                                                        subtype=sf.info(path).subtype, **describe(samples, rate))
    manifest = dict(
        pd_version=(version.stdout+version.stderr).strip(), platform=platform.platform(), cpu=cpu_name,
        python=platform.python_version(), sample_rate=48000, pd_block_size=64, voices=1, gain=1,
        noise_seed=307*1319, source_files={str(p.relative_to(ROOT)): sha(p) for p in sorted(source_files)},
        upstream=upstream, excerpts=edits, montages=montage, audio=assets,
        notes=['No normalisation, resampling, fades or post-processing.',
               'Online performance timings and oscillator/noise states differ; residual metrics compare PD and Kleine only.',
               'CPU and peak RSS are sampled every 5 ms for the whole process, including startup and I/O; lower bounds.',
               'Kleine processing_wall_seconds measures Processor::process only, including per-block preparation.'])
    (audio_root/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    rows = ['| Case | Residual dB | Level delta dB | Median spectrum delta dB | Lag (samples) |',
            '| --- | ---: | ---: | ---: | ---: |']
    for case, m in comparisons.items():
        residual = 'identical' if m['identical_samples'] else f"{m['relative_error_db']:.2f}"
        rows.append(f"| {case} | {residual} | {m['rms_difference_db']:.5f} | {m['active_spectrum_median_abs_db']:.5f} | {m['alignment_lag_samples_pd_minus_klang']} |")
    rows += ['', '| Case | PD / Kleine process CPU (ms/audio s) | PD / Kleine elapsed (s) | PD / Kleine peak RSS (MiB) | Kleine DSP wall (ms/audio s) |',
             '| --- | ---: | ---: | ---: | ---: |']
    for case in CASES:
        r = runs[case]; p, k, seconds = r['pd'], r['kleine'], r['duration']
        dsp = json.loads(k['stdout'])['processing_wall_seconds']*1000/seconds
        rows.append(f"| {case} | {p['sampled_cpu_seconds']*1000/seconds:.2f} / {k['sampled_cpu_seconds']*1000/seconds:.2f} | {p['elapsed_seconds']:.3f} / {k['elapsed_seconds']:.3f} | {p['sampled_peak_rss_bytes']/2**20:.2f} / {k['sampled_peak_rss_bytes']/2**20:.2f} | {dsp:.3f} |")
    docs = ROOT/'docs/farnell/audio/comparisons'
    docs.mkdir(parents=True, exist_ok=True)
    (docs/'tables.md').write_text('\n'.join(rows)+'\n')


if __name__ == '__main__': main()
