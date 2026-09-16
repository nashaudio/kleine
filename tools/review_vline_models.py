"""Compare migrated vline models with PD and an optional pre-port Kleine binary.

Uses the established event recipes. WAVs stay under build; old model parity
failures remain visible and are not treated as new listening acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace
from render_farnell import ROOT, run, np, sf
from render_idiophonics import render_case
from render_artificial import render_case as render_alarm
from check_pd_primitives import metrics

CASES = ['bouncing', 'bouncing-bulk', 'bouncing-legacy', 'bouncing-envelope', 'bouncing-height',
         'rolling', 'rolling-bulk', 'tincan', 'uneven', 'creaking', 'creaking-controls', 'creaking-pulse',
         'boing', 'boing-bulk', 'boing-legacy', 'boing-phase', 'boing-pitch', 'boing-clamped',
         'boing-free', 'boing-frequency', 'bell-a1', 'bell-a2', 'bell-a3', 'bell-partial',
         'bell-group', 'bell-testgroup', 'alarm-bank']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, help='Executable saved before replacing Gesture')
    parser.add_argument('--rate', type=int, nargs='+', default=[48000, 44100])
    args = parser.parse_args()
    options = SimpleNamespace(kleine=ROOT/'build/x64-release/kleine.exe',
                              pd='C:/Program Files/Pd/bin/pd.exe', review=True, retain=False)
    comparisons = []
    for rate in args.rate:
        for case in CASES:
            result = (render_alarm if case == 'alarm-bank' else render_case)(case, rate, options)
            entry = dict(case=case, rate=rate, pd_current=result)
            if args.before:
                work = ROOT/'build/vline-review/models'/str(rate)/case
                work.mkdir(parents=True, exist_ok=True)
                command = result['kleine']['command'][:]
                command[0], command[3] = str(args.before.resolve()), str(work/'before.wav')
                entry['baseline_run'] = run(command, work)
                before, br = sf.read(work/'before.wav')
                after, ar = sf.read(result['kleine']['command'][3])
                reference, pr = sf.read(result['pd']['command'][-1].replace('render.pd', 'pd.wav'))
                assert br == ar == pr == rate and len(before) == len(after) == len(reference)
                assert np.isfinite(after).all()
                entry['before_after'] = metrics(before, after)
                entry['pd_before'] = metrics(reference, before)
                print('  before/after:', entry['before_after']['relative_error_db'], 'dB', flush=True)
                if case == 'alarm-bank':
                    assert np.array_equal(before, after), 'Alarm accumulator changed audio'
                elif case != 'bouncing-envelope':
                    assert entry['before_after']['max_abs_error'] <= 2e-6, f'{case}: unexpected migration change'
            comparisons.append(entry)
    if args.before:
        for rate in args.rate:
            envelope = next(e for e in comparisons if e['rate'] == rate and e['case'] == 'bouncing-envelope')
            height = next(e for e in comparisons if e['rate'] == rate and e['case'] == 'bouncing-height')
            before, _ = sf.read(envelope['baseline_run']['command'][3])
            after, _ = sf.read(envelope['pd_current']['kleine']['command'][3])
            gate, _ = sf.read(height['pd_current']['kleine']['command'][3])
            active_error = float(np.max(abs(before[gate != 0] - after[gate != 0])))
            assert active_error <= 2e-6, 'Bouncing envelope changed while its height gate was open'
            envelope['max_error_with_nonzero_height'] = active_error
    paths = [ROOT/'include/klang/pd.h', ROOT/'farnell/render.h', ROOT/'tools/review_vline_models.py']
    paths += sorted((ROOT/'farnell/klang').rglob('*.k'))
    paths += sorted((ROOT/'farnell/klang').rglob('*.h'))
    report = dict(comparisons=comparisons,
                  source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  executable_sha256=hashlib.sha256(options.kleine.read_bytes()).hexdigest(),
                  baseline_sha256=hashlib.sha256(args.before.read_bytes()).hexdigest() if args.before else None,
                  notes=['Raw audio, unity gain, no alignment; retained listening WAVs are unchanged.',
                         'PD block-routing and model scheduler differences predate this port and remain for sound-by-sound review.',
                         'All Gesture consumers use pd::vline; models do not call the primitive clock sync adapter.',
                         'With a baseline, migration errors must be <=2e-6; the standalone bounce envelope is compared where height is nonzero.',
                         'AlarmGenerator::Bank uses out directly; baseline equality is checked at both rates.'])
    (ROOT/'tests/pd/vline-model-results.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
