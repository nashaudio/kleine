"""Compare pd::vline's complete float/list/cold-inlet/stop contract with PD 0.55.2.

Build tests/pd/vline.cpp as build/vline-review/vline.exe (C++17, include/).
Audio and wrappers are scratch; the retained report records raw, unaligned errors.
"""
import hashlib
import json
import platform
from render_farnell import ROOT, sf, np, run, wrapper, expose_output


def cases(rate):
    # Event time is an absolute sample position. PD parses [del] times as float.
    def e(sample, *values, inlet=0):
        return sample, inlet, values
    fraction_ms = 1000 / rate
    return {
        'silence': [],
        'jump': [e(0, .7)],
        'ramp': [e(0, 1, 13)],
        'jump-slide': [e(0, 1), e(0, 0, 13)],
        'delayed': [e(0, .8, 17, 9)],
        'queue': [e(0, 1, 11), e(0, -.5, 7, 18), e(0, .25, 0, 31), e(0, 0, 9, 37)],
        'replace-first': [e(0, 1, 5, 11), e(0, .7, 9, 22), e(0, -.5, 13, 3)],
        'replace-tail': [e(0, 1, 5, 3), e(0, .7, 9, 22), e(0, 0, 7, 41), e(0, -.5, 13, 11)],
        'same-time-ramps': [e(0, 1, 9, 7), e(0, -.5, 11, 7)],
        'same-time-jumps': [e(0, 1, 0, 7), e(0, -.5, 0, 7)],
        'same-time-jump-slide': [e(0, 1, 0, 7), e(0, 0, 9, 7)],
        'same-time-ramp-jump': [e(0, 1, 9, 7), e(0, -.5, 0, 7)],
        'interrupt': [e(0, 1, 53), e(0, -.2, 7, 71), e(17*64+.25, -.5, 17)],
        'stop': [e(0, 1, 53), e(0, -.2, 7, 71), e(17*64+.25, inlet=3)],
        'stop-restart': [e(0, 1, 53), e(17*64+.25, inlet=3), e(29*64+.25, -.3, 11)],
        'stop-clears-inlets': [e(0, 99, inlet=1), e(0, 77, inlet=2), e(0, inlet=3), e(0, .4)],
        'cold-inlets': [e(0, 11, inlet=1), e(0, 7, inlet=2), e(0, .8), e(19*64+.25, -.2)],
        'two-argument-cold-delay': [e(0, 7, inlet=2), e(0, .8, 11), e(19*64+.25, -.2, 5)],
        'negative-duration': [e(0, .5, -10, 3)],
        'negative-delay': [e(0, 1, 53), e(0, -.5, 7, 71), e(17*64+.25, .4, 31, -1)],
        'sanitise-large-small': [e(0, 1e30), e(4*64+.25, 1), e(8*64+.25, 1e-30)],
        'fractional-start-end': [e(0, 1, 2.3*fraction_ms, .35*fraction_ms), e(0, 0, 4.2*fraction_ms, 3.15*fraction_ms)],
        'multiple-in-sample': [e(0, 1, 0, .15*fraction_ms), e(0, 0, .3*fraction_ms, .15*fraction_ms), e(0, -.5, 0, .65*fraction_ms)],
        'fractional-message': [e(7*64+17.35, 1, 3), e(9*64+5.65, -.2, 7)],
        'block-boundary': [e(0, 1, 0, 64*fraction_ms), e(0, 0, 8, 128*fraction_ms)],
    }


def main():
    results = []
    exe = ROOT / 'build/vline-review/vline.exe'
    for rate in (48000, 44100):
        frames = 8192
        for name, events in cases(rate).items():
            work = ROOT / 'build/vline-review' / str(rate) / name
            work.mkdir(parents=True, exist_ok=True)
            (work / 'model.pd').write_bytes((ROOT / 'tests/pd/vline.pd').read_bytes())
            lines, count = expose_output(wrapper('vline', frames, rate, work / 'pd.wav'))
            rows = []
            for sample, inlet, values in events:
                time = float(np.float32(sample * 1000 / rate))
                values = tuple(float(np.float32(v)) for v in values)
                message = 'stop' if inlet == 3 else ' '.join(format(v, '.9g') for v in values)
                receiver = {0: 'audit-vline', 1: 'audit-duration', 2: 'audit-delay', 3: 'audit-vline'}[inlet]
                if sample == 0:
                    lines += [f'#X msg 600 50 \\; {receiver} {message};', f'#X connect 0 0 {count} 0;']
                    count += 1
                else:
                    lines += [f'#X obj 600 20 del {time:.17g};', f'#X msg 600 50 \\; {receiver} {message};',
                              f'#X connect 0 0 {count} 0;', f'#X connect {count} 0 {count+1} 0;']
                    count += 2
                delivery = int(time * rate / 1000) // 64 * 64
                args = list(values) + [0] * (3 - len(values))
                rows.append(f'{delivery} {time:.17g} {inlet} {len(values)} ' + ' '.join(format(v, '.17g') for v in args))
            (work / 'render.pd').write_text('\n'.join(lines) + '\n')
            (work / 'events.tsv').write_text('\n'.join(rows) + ('\n' if rows else ''))
            reference = run(['C:/Program Files/Pd/bin/pd.exe', '-nogui', '-stderr', '-noprefs',
                             '-noaudio', '-nomidi', '-batch', '-r', str(rate), '-open', str(work / 'render.pd')], work)
            assert not [s for s in reference['stderr'].splitlines() if s.startswith('error:') and 'registry' not in s], reference
            candidate = run([str(exe), str(rate), str(frames), str(work / 'events.tsv'), str(work / 'klang.f32')], work)
            a, actual_rate = sf.read(work / 'pd.wav')
            b = np.fromfile(work / 'klang.f32', dtype=np.float32).astype(np.float64)
            assert actual_rate == rate and len(a) == len(b) == frames
            assert np.isfinite(a).all() and np.isfinite(b).all()
            residual = a - b
            peak_error = float(np.max(abs(residual)))
            tolerance = 2e-6
            result = dict(case=name, sample_rate=rate, frames=frames, block_size=64,
                          events=rows, max_error=peak_error, rms_error=float(np.sqrt(np.mean(residual**2))),
                          pd_peak=float(np.max(abs(a))), klang_peak=float(np.max(abs(b))),
                          pd_rms=float(np.sqrt(np.mean(a*a))), klang_rms=float(np.sqrt(np.mean(b*b))),
                          tolerance=tolerance, passed=peak_error <= tolerance, pd=reference, klang=candidate)
            results.append(result)
            print(name, rate, peak_error, result['passed'], flush=True)
    files = ['include/klang/pd.h', 'tests/pd/vline.cpp', 'tests/pd/vline.pd', 'tools/check_pd_vline.py']
    revisions = json.loads((ROOT/'tests/pd/sources.json').read_text())
    report = dict(pd_version='0.55.2', source_tag='0.55-2',
                  upstream='https://github.com/pure-data/pure-data/blob/0.55-2/src/d_ctl.c',
                  upstream_sha256={r['file']:r['sha256'] for r in revisions
                                   if r['tag'] == '0.55-2' and r['file'] in ('d_ctl.c', 'm_sched.c')},
                  platform=platform.platform(), compiler='MSVC C++17 /O2 /fp:precise',
                  source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files},
                  alignment_samples=0, gain=1, tests=results,
                  notes=['PD logical messages are delivered before their containing DSP block; absolute timestamps remain fractional.',
                         'The driver explicitly syncs the audio clock using PD m_sched.c/d_ctl.c block anchoring; pure models do not.',
                         'No reblocking/DSP pause-resume emulation; queue and ramp logic are independent of host block size.'])
    (ROOT/'tests/pd/vline-results.json').write_text(json.dumps(report, indent=2) + '\n')
    assert all(t['passed'] for t in results), 'See tests/pd/vline-results.json'


if __name__ == '__main__':
    main()
