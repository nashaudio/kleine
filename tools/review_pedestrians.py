"""Investigate Pedestrians' gate phase, file noise floor and historical PD.

Uses the retained listening files and unmodified local website patch. Download
the original executable separately (see the retained review); no network access
or model changes are made. Scratch renders go to build/pedestrians-review.
"""
import argparse
import hashlib
import json
import platform
import shutil
import subprocess
from pathlib import Path

from compare_audio import ROOT, np, sf, signal, plt, db, describe
from scipy import optimize
from render_farnell import expose_output, wrapper, run


WORK = ROOT/'build/pedestrians-review'
AUDIO = ROOT/'farnell/audio/24-pedestrians'
SOURCE = ROOT/'farnell/zip/p01/pedestrian-beep.pd'
RATE = 48000


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def gates(y, rate):
    """Bridge carrier zero crossings; return [first-on, first-off) samples."""
    active = signal.convolve((abs(y) > .001).astype(int), np.ones(25), 'full') > 0
    # Full convolution expands each gate by 24 samples at its end.
    edges = np.diff(np.r_[False, active, False].astype(int))
    return [(int(a), min(len(y), int(b)-24))
            for a, b in zip(np.flatnonzero(edges == 1), np.flatnonzero(edges == -1))
            if b-a > .02*rate]


def sine_fit(y, spans, rate):
    first = spans[0][0]
    idx = np.concatenate([np.arange(a+64, b-64) for a, b in spans])
    time = (idx-first)/rate

    def fit(offset):
        angle = 2*np.pi*(2500+offset)*time
        basis = np.column_stack([np.cos(angle), np.sin(angle), np.ones(len(angle))])
        coef = np.linalg.lstsq(basis, y[idx], rcond=None)[0]
        return float(np.mean((y[idx]-basis@coef)**2)), coef

    opt = optimize.minimize_scalar(lambda offset: fit(offset)[0],
                                  bounds=(-.01, .01), method='bounded',
                                  options={'xatol': 1e-12})
    error, coef = fit(opt.x)
    return dict(frequency_hz=2500+float(opt.x), amplitude=float(np.hypot(*coef[:2])),
                phase_cycles=float(np.angle(coef[0]-1j*coef[1])/(2*np.pi) % 1),
                dc=float(coef[2]), residual_rms=float(np.sqrt(error)),
                interior_margin_samples=64, spans=spans)


def measure(path):
    audio, rate = sf.read(path, always_2d=True)
    y = audio[:, 0]
    spans = gates(y, rate)
    quiet = np.ones(len(y), dtype=bool)
    for a, b in spans:
        quiet[max(0, a-200):min(len(y), b+200)] = False
    off = audio[quiet]
    values, counts = np.unique(off*32768, return_counts=True)
    record = dict(path=str(path.relative_to(ROOT)), sha256=sha(path),
                  subtype=sf.info(path).subtype, channels=audio.shape[1],
                  **describe(audio, rate), gates=spans,
                  off_margin_samples=200, off_rms=float(np.sqrt(np.mean(off**2))),
                  off_nonzero_fraction=float(np.mean(off != 0)))
    if len(values) <= 10:
        record['off_values_in_pcm16_lsb'] = dict(zip(map(str, values), map(int, counts)))
    if audio.shape[1] == 2:
        record['stereo_difference_rms'] = float(np.sqrt(np.mean(np.diff(audio, axis=1)**2)))
    return y, record


def render(name, executable, phase=0, rate=RATE, compatibility=None):
    work = WORK/name
    work.mkdir(parents=True, exist_ok=True)
    lines, count = expose_output(SOURCE.read_text())
    lines += ['#X obj 700 20 r audit-start;', f'#X connect {count} 0 6 0;',
              '#X obj 700 50 r audit-phase;', f'#X connect {count+1} 0 9 1;']
    (work/'model.pd').write_text('\n'.join(lines)+'\n')
    lines, count = expose_output(wrapper('pedestrians', 4*rate, rate, work/'pd.wav'))
    text = '\n'.join(lines).replace('array define captured', 'table captured')
    text += (f'\n#X msg 700 20 \\; audit-phase {phase:.12f} \\; audit-start 1;'
             f'\n#X connect 0 0 {count} 0;\n')
    (work/'render.pd').write_text(text)
    command = [str(executable), '-nogui', '-stderr', '-noprefs', '-noaudio',
               '-nomidi', '-batch', '-r', str(rate)]
    if compatibility:
        command += ['-compatibility', compatibility]
    execution = run(command+['-open', str(work/'render.pd')], work)
    y, measurement = measure(work/'pd.wav')
    assert len(y) == rate*4 and measurement['sample_rate'] == rate
    assert np.isfinite(y).all() and 0 < max(abs(y)) < 1
    assert measurement['off_rms'] == 0
    record = dict(**measurement, phase_control_cycles=phase, compatibility=compatibility,
                  block_size=64, duration_seconds=4, voices=1, execution=execution,
                  cpu_ms_per_audio_second=execution['sampled_cpu_seconds']*1000/4,
                  model_sha256=sha(work/'model.pd'), wrapper_sha256=sha(work/'render.pd'))
    print(name, 'peak', measurement['peak'], flush=True)
    return y, record


def residual(reference, trial):
    error = reference-trial
    return dict(relative_residual_db=float(db(np.linalg.norm(error)/np.linalg.norm(reference))),
                residual_rms=float(np.sqrt(np.mean(error**2))),
                max_abs_error=float(max(abs(error))),
                level_delta_db=float(db(np.linalg.norm(trial)/np.linalg.norm(reference))))


def long_run(name, executable, compatibility):
    """Keep DSP and metro running; capture four seconds at 0, 13 and 30 minutes."""
    work = WORK/name
    work.mkdir(exist_ok=True)
    shutil.copyfile(WORK/'modern-zero/model.pd', work/'model.pd')
    objects, connections = [], []

    def obj(kind, body):
        index = len(objects)
        objects.append(f'#X {kind} 20 {20+20*index} {body};')
        return index

    def connect(a, b, outlet=0):
        connections.append(f'#X connect {a} {outlet} {b} 0;')

    load = obj('obj', 'loadbang')
    start = obj('msg', r'\; pd dsp 1 \; audit-phase 0 \; audit-start 1')
    model = obj('obj', 'model')
    connect(load, start)
    for seconds in [0, 780, 1800]:
        table = f'captured{seconds}'
        obj('obj', f'table {table} {4*RATE}')
        capture = obj('obj', f'tabwrite~ {table}')
        connect(model, capture)
        # A quarter sample avoids a floating-point boundary starting one block early.
        delay = obj('obj', f'del {seconds*1000+.25*1000/RATE:.9f}')
        connect(load, delay)
        connect(delay, capture)
        finish = obj('obj', f'del {(seconds+4)*1000+64*1000/RATE:.9f}')
        connect(load, finish)
        path = (work/f'{seconds}.wav').as_posix().replace(' ', r'\ ')
        save = obj('msg', f'write -wave -bytes 4 {path} {table}')
        writer = obj('obj', 'soundfiler')
        connect(finish, save)
        connect(save, writer)
    finish = obj('obj', 'del 1804100')
    quit_pd = obj('msg', r'\; pd quit')
    connect(load, finish)
    connect(finish, quit_pd)
    (work/'render.pd').write_text('\n'.join(['#N canvas 0 0 900 650 10;']+objects+connections)+'\n')
    command = [str(executable), '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi',
               '-batch', '-r', str(RATE)]
    if compatibility:
        command += ['-compatibility', compatibility]
    execution = run(command+['-open', str(work/'render.pd')], work)
    snapshots = {}
    for seconds in [0, 780, 1800]:
        y, rate = sf.read(work/f'{seconds}.wav')
        spans = gates(y, rate)
        # Exclude any partial beep crossing either boundary of the capture.
        spans = [(a, b) for a, b in spans if a > 64 and b < len(y)-64]
        fit = sine_fit(y, spans[:19], rate)
        snapshots[str(seconds)] = dict(fit, wav_sha256=sha(work/f'{seconds}.wav'))
        snapshots[str(seconds)]['drift_degrees'] = 360*(
            (fit['phase_cycles']-snapshots['0']['phase_cycles']+.5) % 1-.5)
        assert [a for a, b in spans[:19]] == [4800+i*9600 for i in range(19)]
    result = dict(execution=execution, continuous_dsp_seconds=1804.1,
                  cpu_ms_per_audio_second=execution['sampled_cpu_seconds']*1000/1804.1,
                  rate=RATE, block_size=64, snapshots=snapshots)
    (work/'result.json').write_text(json.dumps(result, indent=2)+'\n')
    print(name, {s: m['phase_cycles'] for s, m in snapshots.items()}, flush=True)
    return result


def plot(audio, onsets, output):
    fig = plt.figure(figsize=(13, 10), layout='constrained')
    grid = fig.add_gridspec(3, 2)
    axes = [[fig.add_subplot(grid[row, col]) for col in range(2)] for row in range(2)]
    spectrum = fig.add_subplot(grid[2, :])
    for label, y in audio.items():
        onset = onsets[label]
        style = dict(color='black', linestyle='--') if label == 'Online (left)' else {}
        for col, edge in enumerate([onset, onset+4800]):
            x = np.arange(-20, 40)
            axes[0][col].plot(x/RATE*1000, y[edge+x], '.-', markersize=3,
                              label=label, **{k: v for k, v in style.items() if k != 'linestyle'})
            # This is an illustration, not a claim about Audition's exact filter.
            up = signal.resample_poly(y[edge-100:edge+200], 8, 1, window=('kaiser', 10))
            axes[1][col].plot((np.arange(len(up))/8-100)/RATE*1000, up, label=label, **style)
        f, power = signal.welch(y[onset:onset+187200], RATE, nperseg=16384, noverlap=15360)
        spectrum.semilogx(f, 10*np.log10(np.maximum(power, 1e-25)), label=label, **style)
    for row, title in enumerate(['Stored samples', 'Illustrative 8x band-limited interpolation']):
        for col, edge in enumerate(['on', 'off']):
            axes[row][col].set(title=f'{title}: gate {edge}', xlim=(-.2, .7),
                               xlabel='Time relative to transition (ms)', ylabel='Amplitude')
            axes[row][col].grid(alpha=.3)
    axes[0][0].legend(loc='lower right', fontsize=8)
    spectrum.set(xlim=(20, 24000), ylim=(-130, -20), xlabel='Frequency (Hz)',
                 ylabel='PSD (dBFS/Hz)', title='Same gain: changing carrier phase changes the click spectrum')
    spectrum.grid(alpha=.3)
    spectrum.legend()
    fig.savefig(output, dpi=140)
    plt.close(fig)


def resampling_review(retain):
    """Illustrate conversion ringing using stored samples, not display interpolation."""
    import scipy
    WORK.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(1, 2, figsize=(13, 4), layout='constrained')
    measurements = {}
    for ax, (label, path) in zip(axes, [
            ('Default gate phase', AUDIO/'pedestrians-pd.wav'),
            ('Online-matching gate phase', AUDIO/'review/pedestrians-phase-pd055.wav')]):
        y, rate = sf.read(path)
        assert rate == 48000
        converted = signal.resample_poly(y, 147, 160, window=('kaiser', 10))
        returned = signal.resample_poly(converted, 160, 147, window=('kaiser', 10))
        variants = [('Original 48 kHz samples', y, rate),
                    ('Converted to 44.1 kHz', converted, 44100),
                    ('Converted back to 48 kHz', returned, rate)]
        result = dict(source=str(path.relative_to(ROOT)), sha256=sha(path), variants={})
        for name, audio, sr in variants:
            onset = sr//10
            idx = np.arange(onset-round(sr*.0003), onset+round(sr*.0007))
            ax.plot((idx/sr-.1)*1000, audio[idx], '.-', markersize=3, label=name)
            result['variants'][name] = dict(sample_rate=sr,
                pre_onset_peak=float(max(abs(audio[onset-round(sr*.001):onset]))),
                first_onset_sample=float(audio[onset]))
        measurements[label] = result
        ax.set(title=label, xlabel='Time relative to first gate onset (ms)', ylabel='Amplitude')
        ax.grid(alpha=.3)
        ax.legend(fontsize=8)
    fig.savefig(WORK/'resampling.png', dpi=140)
    plt.close(fig)
    report = dict(scipy_version=scipy.__version__, script_sha256=sha(Path(__file__)),
        method='resample_poly, ratios 147/160 then 160/147, Kaiser beta 10',
        gain_applied=1, onset_seconds=.1, measurements=measurements,
        note='Illustrative zero-phase FIR conversion, not a reproduction of Audition or a device driver. All plotted dots are actual samples of the indicated array. Original WAVs are not modified.')
    (WORK/'resampling.json').write_text(json.dumps(report, indent=2)+'\n')
    if retain:
        for extension in ['png', 'json']:
            shutil.copyfile(WORK/f'resampling.{extension}',
                ROOT/f'farnell/audio/comparisons/artificial/pedestrians-resampling.{extension}')
    print(json.dumps(measurements, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pd', type=Path, default=Path('C:/Program Files/Pd/bin/pd.exe'))
    parser.add_argument('--old-pd', type=Path, default=WORK/'pd-0.42-5/pd/bin/pd.exe')
    parser.add_argument('--retain', action='store_true', help='Retain review JSON, plot and phase-adjusted PD WAVs')
    parser.add_argument('--long-run', action='store_true', help='Also check uninterrupted DSP at 13 and 30 minutes')
    parser.add_argument('--resampling-only', action='store_true', help='Illustrate 48/44.1 kHz conversion using existing WAVs')
    args = parser.parse_args()
    if args.resampling_only:
        resampling_review(args.retain)
        return
    args.pd, args.old_pd = args.pd.resolve(), args.old_pd.resolve()
    for path in [args.pd, args.old_pd]:
        if not path.is_file():
            parser.error(f'Missing executable: {path}; see docs/farnell/audio/24-pedestrians/README.md')
    WORK.mkdir(parents=True, exist_ok=True)
    online, online_record = measure(AUDIO/'pedestrians-online.wav')
    pd, pd_record = measure(AUDIO/'pedestrians-pd.wav')
    kleine, kleine_record = measure(AUDIO/'pedestrians-kleine.wav')
    original, original_rate = sf.read(ROOT/'farnell/zip/p01/pedestrian-beeps.wav', always_2d=True)
    excerpt, _ = sf.read(AUDIO/'pedestrians-online.wav', always_2d=True)
    assert original_rate == RATE and np.array_equal(excerpt, original[2400:249600])
    assert np.array_equal(pd, kleine)
    spans = online_record['gates'][:20]
    assert len(spans) == 20 and all(b-a == 4800 for a, b in spans)
    assert np.all(np.diff([a for a, b in spans]) == 9600)
    fitted = sine_fit(online, spans, RATE)
    phase = fitted['phase_cycles']
    trials, waves = {}, {}
    for label, exe, compat in [('modern', args.pd, '0.55'), ('compat', args.pd, '0.43'),
                               ('original', args.old_pd, None)]:
        for suffix, value in [('zero', 0), ('phase', phase)]:
            name = f'{label}-{suffix}'
            waves[name], trials[name] = render(name, exe, value, compatibility=compat)
        if label != 'compat':
            name = f'{label}-44100'
            waves[name], trials[name] = render(name, exe, rate=44100, compatibility=compat)
    assert np.array_equal(pd, waves['modern-zero'])
    online_start, model_start = spans[0][0], pd_record['gates'][0][0]
    length = len(pd)-model_start
    reference = online[online_start:online_start+length]
    for name, y in waves.items():
        if trials[name]['sample_rate'] == RATE:
            trials[name]['online_comparison'] = residual(reference, y[model_start:])
            trials[name]['default_pd_comparison'] = residual(pd, y)
    bands = {}
    for label, y in [('online', reference), ('pd', pd[model_start:])]:
        f, power = signal.welch(y, RATE, nperseg=16384, noverlap=15360)
        bands[label] = {f'{lo}-{hi}': float(np.sum(power[(f >= lo) & (f < hi)])*(f[1]-f[0]))
                        for lo, hi in [(20, 1000), (5000, 20000)]}
    band_delta = {key: float(10*np.log10(bands['online'][key]/bands['pd'][key]))
                  for key in bands['online']}
    versions = {}
    for label, exe in [('modern', args.pd), ('original', args.old_pd)]:
        version = subprocess.run([str(exe), '-version'], capture_output=True, text=True, timeout=10)
        versions[label] = dict(path=str(exe), sha256=sha(exe),
                               version_output=(version.stdout+version.stderr).strip())
    archive = WORK/'pd-0.42-5.msw.zip'
    record = dict(platform=platform.platform(), processor=platform.processor(),
                  python=platform.python_version(), executables=versions,
                  source=str(SOURCE.relative_to(ROOT)), source_sha256=sha(SOURCE),
                  script_sha256=sha(Path(__file__)),
                  archive_url='https://msp.ucsd.edu/Software/pd-0.42-5.msw.zip',
                  archive_sha256=sha(archive) if archive.exists() else None,
                  files=dict(online=online_record, pd=pd_record, kleine=kleine_record),
                  pd_kleine_identical=True, online_excerpt_sample_identical=True,
                  sine_fit=fitted, alignment=dict(online_start=online_start,
                      model_start=model_start, frames=length, gain_applied=1, resampling=False),
                  trials=trials, spectral_band_power=bands, online_minus_pd_band_db=band_delta,
                  notes=['Phase is the only fitted control applied to renders; all use 2500 Hz, 100 ms, gain 0.2.',
                         'Sine-fit amplitude, frequency and DC are diagnostic only.',
                         'Wrapper replaces DAC with outlet, adds phase/start receivers, and uses table for old PD.',
                         'CPU and RSS include startup/I/O; sampled lower bounds, not isolated DSP benchmarks.',
                         'Interpolation plot is illustrative; Audition settings were not inspected.',
                         'Version output records build dates; compiler identities are unknown for these prebuilt PD executables.'])
    if args.long_run:
        record['long_run'] = {name: long_run(f'{name}-long', exe, compatibility)
                              for name, exe, compatibility in [('modern', args.pd, '0.55'),
                                                               ('original', args.old_pd, None)]}
    (WORK/'review.json').write_text(json.dumps(record, indent=2)+'\n')
    plot({'PD / Kleine default': pd, 'PD 0.42-5, phase adjusted': waves['original-phase'],
          'Online (left)': online},
         {'PD / Kleine default': model_start, 'PD 0.42-5, phase adjusted': model_start,
          'Online (left)': online_start}, WORK/'review.png')
    if args.retain:
        evidence = ROOT/'farnell/audio/comparisons/artificial'
        destination = AUDIO/'review'
        destination.mkdir(exist_ok=True)
        for extension in ['json', 'png']:
            shutil.copyfile(WORK/f'review.{extension}', evidence/f'pedestrians-review.{extension}')
        for name, output in [('modern-phase', 'pedestrians-phase-pd055.wav'),
                             ('original-phase', 'pedestrians-phase-pd042.wav')]:
            shutil.copyfile(WORK/name/'pd.wav', destination/output)
    print(json.dumps(dict(sine_fit=fitted, band_delta_db=band_delta,
                          original_phase=trials['original-phase']['online_comparison']), indent=2))


if __name__ == '__main__':
    main()
