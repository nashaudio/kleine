"""Summarise current PD trials and compare them with the saved pre-review executable.

Run render_artificial.py/render_idiophonics.py --review at both rates first, and
render_farnell.py with scratch --output/--results under build/topology-review/RATE.
This report records differences for listening review; it does not approve them.
"""
import hashlib
import json
from pathlib import Path
from render_farnell import ROOT, sf, np, run
from check_pd_primitives import metrics


def main():
    baseline = ROOT / 'build/topology-review/before/kleine.exe'
    results = []
    for rate in [48000, 44100]:
        paths = [(group, ROOT / 'build' / group / str(rate) / 'results.json')
                 for group in ['artificial', 'idiophonics']]
        paths.append(('phone-bell', ROOT / 'build/topology-review' / str(rate) / 'farnell.json'))
        for group, path in paths:
            for case, result in json.loads(path.read_text()).items():
                command = result['kleine']['command'][:]
                after_path = Path(command[3])
                pd_path = after_path.with_name(after_path.name.replace('kleine', 'pd'))
                before_path = ROOT / 'build/topology-review/before' / str(rate) / (case + '.wav')
                before_path.parent.mkdir(parents=True, exist_ok=True)
                command[0], command[3] = str(baseline), str(before_path)
                if command[2] in ('dtmf-bulk', 'dtmf-study'):
                    command[2] = 'dtmf' # Pre-separation binary reads the TSV source metadata.
                run(command, before_path.parent)
                before, br = sf.read(before_path)
                after, ar = sf.read(after_path)
                pd, pr = sf.read(pd_path)
                assert ar == br == pr == rate and len(before) == len(after) == len(pd)
                assert np.isfinite(after).all() and np.any(after)
                results.append(dict(case=case, group=group, rate=rate, frames=len(after),
                                    before_after=metrics(before, after), pd_current=metrics(pd, after),
                                    previous_parity_limits_passed=result.get('passed'),
                                    samples_over_unity=int(np.count_nonzero(abs(after) > 1)),
                                    tail_rms=float(np.sqrt(np.mean(after[-rate // 10:] ** 2))),
                                    current_audio=str(after_path.relative_to(ROOT)),
                                    reference_audio=str(pd_path.relative_to(ROOT)),
                                    events=result.get('events', 'render_farnell.py default recipe')))
    files = [ROOT / 'include/klang/pd.h', ROOT / 'farnell/render.h']
    files += sorted((ROOT / 'farnell/klang').rglob('*.k'))
    files += sorted((ROOT / 'farnell/klang').rglob('*.h'))
    report = dict(date='2026-09-16', comparison_count=len(results),
                  unchanged_count=sum(r['before_after']['identical_samples'] for r in results),
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
                  baseline_executable_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                  current_executable_sha256=hashlib.sha256((ROOT / 'build/x64-release/kleine.exe').read_bytes()).hexdigest(),
                  baseline_sources=json.loads((baseline.parent / 'sources.json').read_text(encoding='utf-8-sig')),
                  notes=['Raw float audio; no gain fitting, resampling or alignment.',
                         'Source variants and existing control recipes retained; explicit initial stop now owns startup.',
                         'Removal of implicit block timing intentionally changes some signals; listening acceptance is pending.',
                         'Prior retained WAVs are unchanged. Current scratch files are identified per case.',
                         'Diagnostic outputs can encode Hz/counts or unit impulses, so >1 is not always audio clipping.',
                         'Parallel test runs are validation, not comparable DSP performance benchmarks.'],
                  comparisons=results)
    destination = ROOT / 'farnell/audio/comparisons/klang-topology-review.json'
    destination.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(results)} cases; {report["unchanged_count"]} sample-identical to the pre-review build.')


if __name__ == '__main__':
    main()
