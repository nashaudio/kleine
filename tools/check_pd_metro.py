"""Compare sample-timed metro events with PD's block-delivered counter output."""
import hashlib
import json
from pathlib import Path
from render_farnell import ROOT, sf, np, run, wrapper, expose_output


def main():
    pd = 'C:/Program Files/Pd/bin/pd.exe'
    kleine = ROOT/'build/x64-release/kleine.exe'
    results = []
    cases = {'metro': 100, 'metro-fractional': 37.5, 'metro-fast': .25,
             'metro-zero': 0, 'metro-negative': -5, 'metro-controls': 100,
             'metro-legacy': 1, 'metro-stopped': 100}
    for rate in [48000, 44100]:
        frames = (rate+63)//64*64
        for case, interval in cases.items():
            work = ROOT/'build/metro-review'/str(rate)/case
            work.mkdir(parents=True, exist_ok=True)
            source = (ROOT/'tests/pd/metro.pd').read_text()
            if case == 'metro-stopped':
                source = source.replace('#X connect 0 0 1 0;\n', '')
            (work/'model.pd').write_text(source)
            text = wrapper('metro', frames, rate, work/'pd.wav').replace('model;', f'model {interval};')
            lines, count = expose_output(text)
            if case == 'metro-controls':
                events = [(3, 'audit-period .25'), (100, 'audit-metro 0'),
                          (130, 'audit-metro 1'), (150, 'audit-metro bang'),
                          (170, 'audit-period 0'), (250, 'audit-metro stop')]
                for block, message in events:
                    lines += [f'#X obj 600 20 del {(block*64+.25)*1000/rate:.12f};',
                              f'#X msg 600 50 \\; {message};',
                              f'#X connect 0 0 {count} 0;', f'#X connect {count} 0 {count+1} 0;']
                    count += 2
            (work/'render.pd').write_text('\n'.join(lines)+'\n')
            pr = run([pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch',
                      '-r', str(rate), '-open', str(work/'render.pd')], work)
            kr = run([str(kleine), '--render', 'pd-'+case, str(work/'kleine.wav'),
                      str(frames/rate), str(rate)], work)
            a, ar = sf.read(work/'pd.wav')
            b, br = sf.read(work/'kleine.wav')
            assert ar == br == rate and len(a) == len(b) == frames
            # PD runs all control clocks BEFORE the block's audio. Project the
            # sample-timed count to that delivery convention explicitly.
            projected = np.repeat(b.reshape(-1, 64)[:, -1], 64)
            same = bool(np.array_equal(a, projected))
            result = dict(case=case, rate=rate, pd_arguments=interval, frames=frames,
                          block_projected_identical=same, raw_identical=bool(np.array_equal(a, b)),
                          max_projected_error=float(max(abs(a-projected))),
                          pd_final_count=float(a[-1]), klang_final_count=float(b[-1]),
                          first_sample_count=float(b[0]), pd=pr, kleine=kr)
            results.append(result)
            print(case, rate, 'block counts match:', same, 'final:', a[-1], b[-1], flush=True)
    files = ['include/klang/pd.h', 'tests/pd/metro.h', 'tests/pd/metro.pd',
             'farnell/render.h', 'tools/check_pd_metro.py']
    report = dict(tests=results, block_size=64, sample_rates=[48000, 44100],
                  source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files},
                  notes=['WAVs encode counts, not listening audio; no gain fitting or alignment.',
                         'Block projection is analysis only; the metro and WAVs remain sample-timed.',
                         'Legacy case compares its 0.25 ms input, clamped to 1 ms, with PD metro 1.',
                         'Polled starts are delivered at next evaluation; stop cancels an unobserved start.',
                         'No synchronous outlet feedback or shared PD message scheduler is claimed.'])
    (ROOT/'tests/pd/metro-results.json').write_text(json.dumps(report, indent=2)+'\n')
    assert all(t['block_projected_identical'] for t in results), 'See metro-results.json'


if __name__ == '__main__':
    main()
