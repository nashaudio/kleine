"""Verify retained fixtures and record reproducible scheduler behaviour/performance evidence."""
from pathlib import Path
from collections import defaultdict
import csv
import hashlib
import json
import os
import platform
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
WORK = ROOT / 'build/scheduling'
sys.path.insert(0, str(ROOT / 'tools'))
from check_pd_primitives import metrics, np, sf


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    debug = subprocess.run([str(WORK / 'probe-debug.exe')], cwd=ROOT, capture_output=True, text=True, check=True)
    probe = subprocess.run([str(WORK / 'probe.exe')], cwd=ROOT, capture_output=True, text=True, check=True)
    validation = json.loads(probe.stdout)
    assert json.loads(debug.stdout) == validation
    comparisons = []
    for rate in (44100, 48000):
        directory = ROOT / f'build/alarm-review/{rate}/alarm01'
        metadata = json.loads((directory / 'result.json').read_text(encoding='utf-8'))
        for path, digest in metadata['sources'].items():
            assert sha(ROOT / path) == digest, f'Stale PD fixture: {path}'
        for variant, source in [('alarm-block', 'pd.wav'), ('alarm-pure', 'kleine.wav')]:
            reference, reference_rate = sf.read(directory / source)
            actual = np.fromfile(WORK / f'{variant}-{rate}.f32', dtype='<f4')
            assert reference_rate == rate and len(reference) == len(actual) == rate * 4
            result = metrics(reference, actual)
            assert result['identical_samples'], (variant, rate, result)
            comparisons.append(dict(rate=rate, variant=variant,
                                    reference=(directory / source).relative_to(ROOT).as_posix(),
                                    reference_sha256=sha(directory / source), result=result))

    start = time.perf_counter()
    benchmark = subprocess.run([str(WORK / 'benchmark.exe')], cwd=ROOT, capture_output=True, text=True, check=True)
    elapsed = time.perf_counter() - start
    (WORK / 'benchmark.csv').write_text(benchmark.stdout, encoding='utf-8')
    grouped = defaultdict(list)
    for row in csv.DictReader(benchmark.stdout.splitlines()):
        grouped[(row['case'], int(row['dsp']), int(row['buffer']))].append(float(row['ns_per_sample']))
    measurements = []
    for (name, dsp, buffer), values in grouped.items():
        reference = 'configure_counter' if dsp == 2 else 'original'
        median = statistics.median(values)
        baseline = statistics.median(grouped[(reference, dsp, buffer)])
        measurements.append(dict(case=name, dsp=dsp, buffer=buffer, samples=4194304, repeats=len(values),
                                 ns_per_sample=median, minimum=min(values), maximum=max(values),
                                 relative_to=reference, ratio=median / baseline,
                                 processing_seconds_per_audio_second=median * 48000 / 1e9,
                                 raw_ns_per_sample=values))
    paths = [
        'experiments/klang/scheduling/scheduler.h', 'experiments/klang/scheduling/examples.h',
        'experiments/klang/scheduling/probe.cpp', 'experiments/klang/scheduling/benchmark.cpp',
        'experiments/klang/scheduling/report.py', 'experiments/klang/pd-block/prototypes.h',
        'include/klang.h', 'include/klang/pd.h',
    ]
    compiler = subprocess.run(
        ['powershell', '-NoProfile', '-Command',
         "(Get-Item 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/*/bin/Hostx64/x64/cl.exe' | Select-Object -Last 1).VersionInfo.FileVersion"],
        capture_output=True, text=True, check=True).stdout.strip()
    cpu = subprocess.run(
        ['powershell', '-NoProfile', '-Command',
         "(Get-ItemProperty 'HKLM:/HARDWARE/DESCRIPTION/System/CentralProcessor/0').ProcessorNameString"],
        capture_output=True, text=True, check=True).stdout.strip()
    report = dict(
        platform=platform.platform(), cpu=cpu or os.environ.get('PROCESSOR_IDENTIFIER'),
        compiler=f'MSVC {compiler}', flags='/std:c++17 /O2 /fp:precise /DNOMINMAX /wd4244 /wd4305',
        sample_rate=48000, voices=1, validation=validation, debug_validation=json.loads(debug.stdout), audio_comparisons=comparisons,
        benchmark_elapsed_seconds=elapsed, benchmark_process_cpu=json.loads(benchmark.stderr), benchmark=measurements,
        source_sha256={path: sha(ROOT / path) for path in paths},
        executable_sha256={name: sha(WORK / name) for name in ('probe.exe', 'probe-debug.exe', 'benchmark.exe')},
        notes=[
            'Isolated experiment; no changes to core Klang, PD primitives, production models or engine.',
            'DSP microbenchmark wall time excludes model construction, reservation, file I/O and process startup.',
            'Aggregate process CPU uses Windows GetProcessTimes (kernel + user), covering all scenarios, warm-ups and CSV formatting after model construction; -1 means unavailable. Not a per-scenario CPU measurement.',
            'One voice, 48000 Hz, host buffers 32/64/256/1024; 7 rotating-order repeats with 262144-frame warm-up; benchmark thread pinned to logical CPU 2 if supported.',
            'dsp=0 simple recurrence; dsp=1 PD oscillator; dsp=2 coefficient work (sparse held control, not modulation fidelity).',
            'Tiny timing differences are subject to compiler layout, CPU frequency and system noise; no zero-cost guarantee.',
            'Object sizes are reported; resident process memory would mostly measure existing plugin storage and test harness.',
            'PD 0.55.2 WAVs are cached alarm-review fixtures with verified patch hashes; no new PD execution.',
            'Block adapter only proves the fixed-period alarm recipe, not the complete PD scheduler.',
            'Nested sample-only dispatch bypasses buffer scheduling; the expected limitation is tested.',
            'C++ allocation interception covers regular new/new[] during first prepare and steady processing, not OS allocation APIs.',
        ])
    destination = ROOT / 'experiments/klang/scheduling/results.json'
    destination.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    table = ['# Scheduling measurements', '',
             f"{report['cpu']}; {report['compiler']}; Release `/O2 /fp:precise`; one voice at 48 kHz.", '',
             'Median of seven repeats, 4,194,304 samples per repeat, 64-sample host buffers. '
             'PD oscillator workload. These are processing wall times, excluding startup and I/O.', '',
             '| Dispatch / timers | ns per sample | Relative to original |', '| --- | ---: | ---: |']
    for row in measurements:
        if row['buffer'] == 64 and row['dsp'] == 1:
            table.append(f"| {row['case']} | {row['ns_per_sample']:.3f} | {row['ratio']:.3f} |")
    table += ['', '## Sparse coefficient example', '',
              '| Method | ns per sample | Relative to manual counter |', '| --- | ---: | ---: |']
    for row in measurements:
        if row['buffer'] == 64 and row['dsp'] == 2:
            table.append(f"| {row['case']} | {row['ns_per_sample']:.3f} | {row['ratio']:.3f} |")
    table += ['', 'Full raw repetitions, 32/64/256/1024 buffer sizes, both DSP workloads, object sizes, '
              'aggregate CPU time and source hashes are in [results.json](results.json). '
              'Small differences include code-layout and measurement effects; these results do not establish zero overhead.', '']
    (destination.parent / 'measurements.md').write_text('\n'.join(table), encoding='utf-8')
    print(json.dumps(validation))
    print('Four audio comparisons are sample-identical; benchmark report written.')
    for row in measurements:
        if row['buffer'] == 64 and (row['dsp'] == 1 or row['dsp'] == 2):
            print(f"{row['case']}: {row['ns_per_sample']:.3f} ns/sample ({row['ratio']:.3f}x {row['relative_to']})")


if __name__ == '__main__':
    main()
