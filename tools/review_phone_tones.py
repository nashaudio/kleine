"""Summarise fresh Phone Tones PD comparisons; run commands are in the model notes."""
import hashlib
import json
import subprocess

from render_farnell import ROOT, np, sf
from check_pd_primitives import metrics


def main():
    comparisons = []
    for rate in (48000, 44100):
        runs = json.loads((ROOT/f'build/phone-review/{rate}/results.json').read_text())
        for case, result in runs.items():
            a, ar = sf.read(ROOT/f'build/phone-review/{rate}/25-phone-tones/{case}-pd.wav')
            b, br = sf.read(result['kleine']['command'][3])
            assert ar == br == rate and a.shape == b.shape and np.isfinite(a).all() and np.isfinite(b).all()
            m = metrics(a, b)
            comparisons.append(dict(case=case, group='tones', **result, comparison=m,
                                    passed=abs(m['level_delta_db']) < .002 and m['relative_error_db'] < -70,
                                    samples_at_or_above_unity=int(np.count_nonzero(abs(b) >= 1)),
                                    block_size=64, alignment_samples=0, output_gain=1))
        for case in ('ringback-bulk', 'phone-effects', 'call-recogniser', 'phone-match', 'call-match'):
            result = json.loads((ROOT/f'build/artificial/{rate}/{case}/result.json').read_text())
            comparisons.append(dict(case=case, group='tones' if case == 'ringback-bulk' else 'supplements', **result))

    paths = list((ROOT/'farnell/klang/Artificial Sounds/Phone Tones').rglob('*.k'))
    paths += [ROOT/p for p in ('include/klang/pd.h', 'farnell/render.h', 'farnell/variants.h',
                              'tests/pd/phone-tones-archive.h',
                              'tools/render_farnell.py', 'tools/render_artificial.py', 'tools/review_phone_tones.py')]
    version = subprocess.run(['C:/Program Files/Pd/bin/pd.exe', '-version'], capture_output=True, text=True, timeout=10)
    report = dict(date='2026-09-16', pd_version=(version.stdout+version.stderr).strip(),
                  comparisons=comparisons,
                  source_sha256={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  executable_sha256=hashlib.sha256((ROOT/'build/x64-release/kleine.exe').read_bytes()).hexdigest(),
                  notes=['Raw mono float32 renders, unity gain; no resampling, alignment or level matching.',
                         'Core tone recipes: 4 seconds, ringbacks: 12 seconds; pulse counts 1,5,7 at 0.25,0.75,1.75 seconds.',
                         'Pulse starts are rounded to the PD block by the existing harness; subsequent Klang contacts are sample-timed.',
                         'Historical parity limits: absolute level delta <0.002 dB and PD-relative RMS error <-70 dB; these are diagnostic, not listening gates.',
                         'Tiny (~1e-19) filter-tail discrepancies are insignificant; -300 dB is the reporting floor.',
                         'Supplemental number matchers are reconstructed reference copies, not unchanged upstream patches.',
                         'PD/Kleine process CPU, wall time and RSS are recorded in each run; they include startup and I/O, not isolated DSP cost.',
                         'These tests do not choose a preferred variant or establish perceptual acceptance. Retained listening WAVs are unchanged.'])
    output = ROOT/'farnell/audio/comparisons/phone-tones-review.json'
    output.write_text(json.dumps(report, indent=2)+'\n')
    for item in comparisons:
        m = item['comparison']
        print(f"{item['sample_rate']} {item['case']}: {'PASS' if item['passed'] else 'REVIEW'}; error {m['relative_error_db']:.2f} dB; level {m['level_delta_db']:.6f} dB")


if __name__ == '__main__':
    main()
