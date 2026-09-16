"""Compare isolated Pd primitives with Klang, including old Pd compatibility.

PD reference patches live in tests/pd; wrappers and WAVs are generated in build.
The saved JSON records levels, unaligned residuals, parameters and source hashes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from render_farnell import ROOT, np, sf, run, wrapper

CASES = {
    'osc': ('osc', '440 0'),
    'osc-300': ('osc', '300 0'),
    'osc-phase': ('osc', '440 0.25'),
    'osc-negative': ('osc', '-440 0'),
    'osc-fm': ('osc-fm', ''),
    'hip': ('hip', '90'),
    'hip-zero': ('hip', '0'),
    'hip-high': ('hip', '2000'),
    'hip-legacy': ('hip', '90'),
    'lop': ('lop', '100'),
    'lop-slow': ('lop', '.1'),
    'lop-ground': ('lop', '20'),
    'bp-wire': ('bp', '2000 12'),
    'bp-speaker': ('bp', '400 7'),
    'bp-can': ('bp', '359 123'),
    'bp-ground': ('bp', '632 400'),
    'noise': ('noise', '404933'),
    'vline-decay': ('vline-decay', ''),
    'phasor': ('phasor', '440 0'),
    'phasor-negative': ('phasor', '-440 0.25'),
    'cos': ('cos', ''),
    'wrap': ('wrap', ''),
    'line': ('line', ''),
    'env': ('env', ''),
    'vcf': ('vcf', '80 0'),
    'vcf-im': ('vcf', '80 1'),
    'vcf-zero': ('vcf', '0 0'),
    'samphold': ('samphold', ''),
    'rzero': ('rzero', ''),
}


def metrics(a, b):
    rms = lambda x: float(np.sqrt(np.mean(x*x)))
    error = rms(a-b)
    return dict(pd_peak=float(np.max(np.abs(a))), klang_peak=float(np.max(np.abs(b))),
                pd_dc=float(np.mean(a)), klang_dc=float(np.mean(b)),
                pd_rms=rms(a), klang_rms=rms(b), level_delta_db=float(20*np.log10(rms(b)/rms(a))),
                error_rms=error, relative_error_db=float(20*np.log10(max(1e-15, error/rms(a)))),
                max_abs_error=float(np.max(np.abs(a-b))), identical_samples=bool(np.array_equal(a,b)))


def render_pd(name, arguments, rate, frames, compatibility, work, pd):
    work.mkdir(parents=True, exist_ok=True)
    source = ROOT/'tests/pd'/f'{name}.pd'
    shutil.copyfile(source, work/'model.pd')
    destination = work/'pd.wav'
    text = wrapper('primitive', frames, rate, destination)
    text = text.replace('model;', f'model {arguments};')
    if name in ['hip', 'lop', 'bp']:
        # Wrapper object indices are derived from its actual records.
        from render_farnell import expose_output
        lines, count = expose_output(text)
        lines += ['#X obj 500 20 array define impulse 1;', '#X msg 500 50 \\; impulse 0 1;',
                  '#X obj 500 80 tabplay~ impulse;', '#X obj 500 110 t b b;',
                  f'#X connect 0 0 {count+3} 0;', f'#X connect {count+3} 1 {count+1} 0;',
                  f'#X connect {count+3} 0 {count+2} 0;', f'#X connect {count+2} 0 3 0;']
        text = '\n'.join(lines)+'\n'
    (work/'render.pd').write_text(text)
    result = run([pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch',
                  '-r', str(rate), '-compatibility', compatibility, '-open', str(work/'render.pd')], work)
    return destination, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pd', default='C:/Program Files/Pd/bin/pd.exe')
    parser.add_argument('--kleine', type=Path, default=ROOT/'build/x64-release/kleine.exe')
    parser.add_argument('--rates', type=int, nargs='+', default=[48000, 44100])
    parser.add_argument('--output', type=Path, default=ROOT/'tests/pd/results.json')
    args = parser.parse_args()
    results, versions, failed, fm_offsets = [], [], [], []
    for rate in args.rates:
        frames = (rate+63)//64*64
        for case, (name, arguments) in CASES.items():
            work = ROOT/'build/primitives'/str(rate)/case
            compatibility = '0.43' if case == 'hip-legacy' else '0.55'
            output, _ = render_pd(name, arguments, rate, frames, compatibility, work, args.pd)
            klang = work/'kleine.wav'
            run([str(args.kleine.resolve()), '--render', 'pd-'+case, str(klang), str(frames/rate), str(rate)], work)
            a, ar = sf.read(output); b, br = sf.read(klang)
            assert ar == br == rate and len(a) == len(b) == frames
            assert np.isfinite(a).all() and np.isfinite(b).all()
            m = metrics(a, b)
            # A reference tolerance for these deterministic cases, not a
            # universal perceptual threshold for arbitrary synthesis models.
            passed = abs(m['level_delta_db']) < .002 and m['relative_error_db'] < -70
            if not passed: failed.append((case, rate, m))
            results.append(dict(case=case, sample_rate=rate, frames=frames, arguments=arguments,
                                pd_compatibility=compatibility, passed=passed, **m))
            print(rate, case, f"residual {m['relative_error_db']:.2f} dB; level {m['level_delta_db']:.6f} dB", flush=True)
            if case in ['osc', 'osc-300', 'osc-fm', 'hip']:
                old, _ = render_pd(name, arguments, rate, frames, '0.43', work/'legacy', args.pd)
                historical, _ = sf.read(old)
                versions.append(dict(case=case, sample_rate=rate, comparison='0.55.2 compatibility 0.55 versus 0.43',
                                     **metrics(a, historical)))
                if case == 'osc-fm':
                    mod = next(v for v in versions if v['case'] == 'osc-300' and v['sample_rate'] == rate)
                    offset = mod['klang_dc'] - mod['pd_dc']
                    corrected, _ = render_pd('osc-fm-offset', str(offset), rate, frames, '0.43', work/'dc-offset', args.pd)
                    corrected_audio, _ = sf.read(corrected)
                    fm_offsets.append(dict(sample_rate=rate, removed_modulator_dc=offset,
                                           comparison='Current FM versus legacy FM with measured modulator DC difference subtracted',
                                           **metrics(a, corrected_audio)))
    version = subprocess.run([args.pd, '-version'], capture_output=True, text=True, timeout=10)
    source_paths = [ROOT/'include/klang/pd.h', ROOT/'tests/pd/primitive.h', Path(__file__),
                    ROOT/'farnell/klang/Idiophonics/Telephone Bell/telephonebell.k', ROOT/'farnell/render.h',
                    ROOT/'tools/render_farnell.py']
    source_paths += list((ROOT/'tests/pd').glob('*.pd'))
    report = dict(pd_version=(version.stdout+version.stderr).strip(), block_size=64, seed=404933,
                  tests=results, compatibility_comparisons=versions, fm_dc_experiment=fm_offsets,
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
                  tolerance=dict(level_db=.002, relative_error_db=-70),
                  notes=['No gain fitting, time alignment or delivery projection.',
                         'Compatibility 0.43 runs older routines inside Pd 0.55.2, not an old executable.',
                         'In compatibility comparisons, klang_* denotes the legacy PD output.',
                         'vline-decay tests only the model helper ramp, not a complete vline~ implementation.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    if failed: raise SystemExit(f'Primitive parity failures: {failed}')


if __name__ == '__main__': main()
