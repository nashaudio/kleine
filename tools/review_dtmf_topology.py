"""Retain PD/model evidence and compare this topology revision with a saved executable."""
import argparse
import hashlib
import json
import platform
from pathlib import Path
from render_farnell import ROOT, run, sf, np


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--renders', type=Path, default=ROOT/'build/del-review/models')
    parser.add_argument('--output', type=Path, default=ROOT/'tests/pd/dtmf-model-results.json')
    args = parser.parse_args()
    results = []
    for rate in (48000,44100):
        for case in ('dtmf','dtmf-study','dtmf-retrigger','dtmf-detector','dtmf-decoder',
                     'phone-effects','phone-match','call-recogniser','call-match'):
            work = args.renders.resolve()/str(rate)/case
            result = json.loads((work/'result.json').read_text())
            command = result['kleine']['command'].copy()
            command[0] = str(args.baseline.resolve())
            command[3] = str(work/'before.wav')
            before_run = run(command,work)
            before, br = sf.read(work/'before.wav')
            after, ar = sf.read(work/'kleine.wav')
            assert ar == br == rate and before.shape == after.shape
            identical = bool(np.array_equal(before,after))
            assert identical, (rate,case,'Existing performance changed; inspect the audio')
            results.append(dict(rate=rate,case=case,baseline_audio_identical=identical,
                                baseline_run=before_run,comparison=result))
            print(rate,case,'unchanged samples',flush=True)
    paths = ['include/klang/pd.h','farnell/render.h','farnell/klang/Artificial Sounds/DTMF Tones/dtmftones.k',
             'farnell/klang/Artificial Sounds/Phone Tones/phoneeffects.k',
             'tools/render_artificial.py','tools/review_dtmf_topology.py']
    report = dict(tests=results,machine=platform.platform(),processor=platform.processor(),
        baseline_executable_sha256=hashlib.sha256(args.baseline.read_bytes()).hexdigest(),
        current_executable_sha256=hashlib.sha256((ROOT/'build/x64-release/kleine.exe').read_bytes()).hexdigest(),
        source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
        notes=['MSVC x64 Release; PD 0.55.2; one voice; 64-sample PD blocks; raw mono float32 WAVs.',
               'Identical baseline audio applies to these existing non-overlapping-key recipes; decoder overlap/repetition behaviour intentionally changed.',
               'Run records retain total elapsed time, sampled process CPU/RSS and Kleine processing wall time. Startup, file I/O and process memory are not DSP-only cost.',
               'No level matching, alignment, resampling, new block adapter or retained listening WAV replacement.'])
    args.output.write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__': main()
