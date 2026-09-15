"""Run the C++ design probes and compare their reference output with cached PD fixtures."""
from pathlib import Path
import hashlib
import json
import platform
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from check_pd_primitives import metrics, np, sf


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    work = ROOT / 'build/pd-block'
    run = subprocess.run([str(work/'probe.exe')], cwd=ROOT, capture_output=True, text=True, check=True)
    comparisons = []
    for rate in [48000, 44100]:
        for controls in [False, True]:
            suffix = '-controls' if controls else ''
            directory = ROOT / f'build/artificial/{rate}/pedestrians{suffix}'
            source = directory / 'pd.wav'
            cached = json.loads((directory / 'result.json').read_text())
            for file, digest in cached['sources'].items():
                assert sha(ROOT / file) == digest, f'Stale PD fixture source: {file}'
            reference = np.fromfile(work / f'reference-{rate}{suffix}.f32', dtype='<f4').astype(float)
            pd, pd_rate = sf.read(source)
            assert pd_rate == rate and len(pd) == len(reference) == rate*4
            result = metrics(pd, reference)
            assert result['identical_samples'], (rate, controls, result)
            comparisons.append(dict(rate=rate, controls=controls, pd_fixture=source.relative_to(ROOT).as_posix(),
                                    pd_sha256=sha(source), comparison=result))
    ideals = []
    for rate in [48000, 44100]:
        ideal = np.fromfile(work / f'ideal-{rate}.f32', dtype='<f4').astype(float)
        reference = np.fromfile(work / f'reference-{rate}.f32', dtype='<f4').astype(float)
        ideals.append(dict(rate=rate, native_sample_timing_vs_reference=metrics(reference, ideal)))
    files = ['experiments/klang/pd-block/prototypes.h', 'experiments/klang/pd-block/probe.cpp',
             'experiments/klang/pd-block/report.py', 'include/klang.h', 'include/klang/pd.h',
             'farnell/klang/Artificial Sounds/Pedestrians/pedestrians.k']
    compiler = next((ROOT/'build/x64-release/CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
    version = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)', compiler.read_text())[1]
    report = dict(platform=platform.platform(), compiler=f'MSVC {version}, /std:c++17 /O2',
                  lifecycle_checks=run.stdout.strip(), prototype_audio_comparisons=72,
                  pd_fixture_comparisons=comparisons, ideal_comparisons=ideals,
                  source_sha256={file: sha(ROOT/file) for file in files},
                  notes=['PD WAVs are cached 0.55.2 fixtures from render_artificial.py, not new PD executions.',
                         '72 comparisons: 2 rates x 2 control recipes x 3 host partitions x 6 evaluation variants.',
                         'Buffer-only automatic dispatch fails when nested, as an expected negative check.',
                         'This is evidence for the design paper, not a published primitive or performance benchmark.'])
    destination = ROOT/'experiments/klang/pd-block/results.json'
    destination.write_text(json.dumps(report, indent=2)+'\n')
    print(run.stdout.strip())
    print('Four cached PD fixtures match exactly.')
    for item in ideals:
        print(item['rate'], 'ideal timing residual:', item['native_sample_timing_vs_reference']['relative_error_db'])


if __name__ == '__main__':
    main()
