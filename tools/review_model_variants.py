"""Verify that extracting development alternatives preserves raw rendered samples."""
import argparse
import hashlib
import json
from pathlib import Path

from render_farnell import ROOT, run, np, sf
from render_artificial import VARIANTS, recipe as artificial_recipe
from render_idiophonics import recipe as idiophonics_recipe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', required=True, type=Path)
    parser.add_argument('--after', type=Path, default=ROOT/'build/x64-release/kleine.exe')
    args = parser.parse_args()
    cases = {}
    for case in ['dtmf', 'dtmf-bulk', 'dtmf-study', 'dtmf-retrigger', 'dtmf-detector',
                 'dtmf-decoder', 'phone-effects', 'call-recogniser', 'phone-match', 'call-match',
                 'police', 'police-legacy', 'police-horn', 'police-environment',
                 'police-logosc', 'police-logosc-legacy', 'police-graph', 'police-graph-legacy',
                 'police-triangle', 'police00', 'police01', 'police1', 'police-compare-log']:
        cases[case] = (VARIANTS.get(case, (case,))[0], *artificial_recipe(case))
    for case in ['bouncing', 'bouncing-bulk', 'bouncing-legacy', 'bouncing-height',
                 'bouncing-envelope', 'boing', 'boing-bulk', 'boing-legacy',
                 'boing-phase', 'boing-pitch', 'boing-clamped', 'boing-free', 'boing-frequency']:
        cases[case] = (case, *idiophonics_recipe(case))
    for case in ['dial', 'dial-web', 'dial-line', 'busy', 'busy-archive', 'ringback', 'ringback-bulk', 'pulse']:
        events = [] # Pulse's three dial events are built into the renderer.
        cases[case] = (case, 12 if 'ringback' in case else 4, events)

    results = []
    for rate in [48000, 44100]:
        for case, (model, duration, events) in cases.items():
            work = ROOT/'build/variant-review'/str(rate)/case
            work.mkdir(parents=True, exist_ok=True)
            recipe = work/'events.tsv'
            recipe.write_text(''.join(f'{round(t*rate)//64*64}\t{action}' +
                                     ''.join(f'\t{value}' for value in values) + '\n'
                                     for t, action, values in events))
            runs = {}
            for label, exe in [('before', args.before), ('after', args.after)]:
                selected = 'dtmf' if label == 'before' and model in ('dtmf-bulk', 'dtmf-study') else model
                folder = work/label
                folder.mkdir(exist_ok=True)
                command = [str(exe.resolve()), '--render', selected, str(folder/'audio.wav'),
                           str(duration), str(rate), '1']
                if case != 'pulse': command.append(str(recipe))
                runs[label] = run(command, folder)
            before, br = sf.read(work/'before/audio.wav')
            after, ar = sf.read(work/'after/audio.wav')
            assert br == ar == rate and before.shape == after.shape
            assert np.isfinite(after).all(), case
            error = float(np.max(np.abs(before-after)))
            identical = np.array_equal(before, after)
            recorded_events = [(t, 'dial', [n]) for t, n in [(.25, 1), (.75, 5), (1.75, 7)]] if case == 'pulse' else events
            results.append(dict(case=case, rate=rate, seconds=duration, events=recorded_events,
                                max_abs_error=error, sample_identical=identical, runs=runs))
            print(f'{case} {rate}: max error {error:g}', flush=True)

    paths = [ROOT/'farnell/render.h', ROOT/'farnell/variants.h', ROOT/'tests/pd/phone-tones-archive.h', Path(__file__)]
    paths += sorted((ROOT/'farnell/klang').rglob('*.k'))
    report = dict(comparisons=results,
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  before_sha256=hashlib.sha256(args.before.read_bytes()).hexdigest(),
                  after_sha256=hashlib.sha256(args.after.read_bytes()).hexdigest(),
                  notes=['Raw float32 audio; unity gain; no alignment or normalization.',
                         'Same event recipes, rates and seeds before/after; host control grid is 64 samples.',
                         'This verifies refactoring only, not PD fidelity or listening preference.',
                         'DTMF alternatives now have explicit render model names; legacy metadata is validated.'])
    (ROOT/'farnell/audio/comparisons/model-variants-review.json').write_text(json.dumps(report, indent=2)+'\n')
    assert all(r['sample_identical'] for r in results), 'Audio changed; see report'


if __name__ == '__main__':
    main()
