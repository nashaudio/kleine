"""Compare both pd::line overloads' control contract with isolated PD [line].

The established primitive suite separately verifies the two-argument audio mode.
PD delivers control messages before a DSP block; projection here is analysis only.
"""
import hashlib
import json
from render_farnell import ROOT, sf, np, run, wrapper, expose_output


def main():
    results = []
    cases = ['ramp', 'ramp-zero-grain', 'ramp-negative-grain', 'ramp-fast',
             'ramp-retarget', 'ramp-stop', 'ramp-legacy', 'ramp-reset',
             'ramp-inlets', 'ramp-repeat']
    for rate in [48000, 44100]:
        frames = (rate // 4 + 63) // 64 * 64
        for case in cases:
            work = ROOT / 'build/line-review' / str(rate) / case
            work.mkdir(parents=True, exist_ok=True)
            model = (ROOT / 'tests/pd/line-control.pd').read_text()
            if case == 'ramp-repeat':
                # Count emissions, including repeated equal values.
                model = model.replace('#X connect 1 0 2 0;', '')
                model += ('#X obj 10 200 f 0;\n#X obj 10 230 + 1;\n'
                          '#X obj 10 180 t b;\n#X connect 1 0 8 0;\n'
                          '#X connect 8 0 6 0;\n#X connect 6 0 7 0;\n'
                          '#X connect 7 0 6 1;\n#X connect 7 0 2 0;\n')
            (work / 'model.pd').write_text(model)
            grain = 0 if case == 'ramp-zero-grain' else -3 if case == 'ramp-negative-grain' else .25 if case == 'ramp-fast' else 20
            events = [(0, f'audit-line {0 if case == "ramp-repeat" else 1} 113 {grain}')]
            if case == 'ramp-retarget': events += [(17, 'audit-line -0.4 83 13'), (54, 'audit-line 0.7 53 7')]
            if case in ('ramp-stop', 'ramp-legacy'): events += [(23, 'audit-line stop'), (51, 'audit-line -0.5 71 20')]
            if case == 'ramp-reset': events += [(23, 'audit-line set 0.4'), (51, 'audit-line -0.5 71 20')]
            if case == 'ramp-inlets':
                events += [(17, 'audit-grain 7 \\; audit-duration 83 \\; audit-line -0.4'),
                           (54, 'audit-line 0.7'), (77, 'audit-duration -1 \\; audit-line 0.2')]
            lines, count = expose_output(wrapper('control-line', frames, rate, work / 'pd.wav'))
            for block, message in events:
                if block == 0:
                    lines += [f'#X msg 600 50 \\; {message};', f'#X connect 0 0 {count} 0;']
                    count += 1
                    continue
                lines += [f'#X obj 600 20 del {(block * 64 + .25) * 1000 / rate:.12f};',
                          f'#X msg 600 50 \\; {message};',
                          f'#X connect 0 0 {count} 0;', f'#X connect {count} 0 {count + 1} 0;']
                count += 2
            (work / 'render.pd').write_text('\n'.join(lines) + '\n')
            pr = run(['C:/Program Files/Pd/bin/pd.exe', '-nogui', '-stderr', '-noprefs',
                      '-noaudio', '-nomidi', '-batch', '-r', str(rate), '-compatibility',
                      '0.47' if case == 'ramp-legacy' else '0.55', '-open', str(work / 'render.pd')], work)
            errors = [s for s in pr['stderr'].splitlines() if s.startswith('error:') and 'registry' not in s]
            assert not errors, errors
            kr = run([str(ROOT / 'build/x64-release/kleine.exe'), '--render', 'pd-' + case,
                      str(work / 'kleine.wav'), str(frames / rate), str(rate)], work)
            a, ar = sf.read(work / 'pd.wav')
            b, br = sf.read(work / 'kleine.wav')
            assert ar == br == rate and len(a) == len(b) == frames
            assert np.isfinite(a).all() and np.isfinite(b).all()
            projected = np.repeat(b.reshape(-1, 64)[:, -1], 64)
            error = float(np.max(abs(a - projected)))
            # Retargets in PD occur 1/4 sample after the C++ event. At these ramp
            # slopes this accounts for < 0.0001; no output alignment/gain fitting.
            tolerance = 0 if case == 'ramp-repeat' else .0001
            result = dict(case=case, rate=rate, frames=frames, max_projected_error=error,
                          tolerance=tolerance, passed=error <= tolerance,
                          pd=pr, kleine=kr)
            results.append(result)
            print(case, rate, error, result['passed'], flush=True)
    files = ['include/klang/pd.h', 'tests/pd/line-control.h', 'tests/pd/line-control.pd',
             'tools/check_pd_line.py', 'farnell/render.h']
    report = dict(tests=results, block_size=64,
                  source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in files},
                  notes=['Control WAVs hold emitted values; repeat case counts emissions.',
                         'Last-sample block projection is analysis only.',
                         'Legacy stop is modern PD compatibility 0.47, not the historical executable.'])
    (ROOT / 'tests/pd/line-results.json').write_text(json.dumps(report, indent=2) + '\n')
    assert all(t['passed'] for t in results), 'See tests/pd/line-results.json'


if __name__ == '__main__':
    main()
