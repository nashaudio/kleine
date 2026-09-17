"""Measure old/new oscillators against requested frequency and waveform metrics.

This produces evidence, not a sample-identity or all-waveforms-pass gate.
"""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-header", type=Path)
    parser.add_argument("--compiler", default="cl.exe")
    parser.add_argument("--output", default="build/osm-fix/metrics-msvc")
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error("Compiler not found; use an x64 VS developer environment")
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / "build"):
        parser.error("Output must be under build/")
    headers = {"baseline": ROOT / "experiments/klang/core/baselines/production-2026-09-17/klang.h", "candidate": ROOT / "include/klang.h"}
    if args.game_header:
        headers["game"] = args.game_header.resolve(strict=True)
    source = ROOT / "tests/klang/oscillators.cpp"
    flags = ["/nologo", "/std:c++17", "/EHsc", "/O2", "/fp:precise", "/MT", "/DNDEBUG",
             "/DNOMINMAX", "/DWIN32", "/D_WINDOWS", "/D_CRT_SECURE_NO_WARNINGS",
             "/W3", "/wd4244", "/wd4305", "/showIncludes"]
    results = []
    for label, header in headers.items():
        directory = output / label
        directory.mkdir(parents=True, exist_ok=True)
        exe = directory / "case.exe"
        command = [compiler, *flags, *(["/DREPAIRED_OSM"] if label == "candidate" else []),
                   "/I" + str(header.parent), str(source),
                   "/Fo" + str(directory / "case.obj"), "/Fe" + str(exe)]
        result = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=120)
        log = result.stdout + result.stderr
        (directory / "compiler.log").write_text(log, encoding="utf-8")
        if result.returncode or header.as_posix().lower() not in log.replace("\\", "/").lower():
            print(log[-6000:])
            return 1
        run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60, check=True)
        (directory / "metrics.csv").write_text(run.stdout, encoding="utf-8")
        rows = [{k: v if k == "wave" else float(v) for k, v in row.items()}
                for row in csv.DictReader(io.StringIO(run.stdout))]
        phases = subprocess.run([str(exe), "phase"], capture_output=True, text=True, timeout=30, check=True)
        (directory / "phase.csv").write_text(phases.stdout, encoding="utf-8")
        phase_rows = [{k: float(v) for k, v in row.items()}
                      for row in csv.DictReader(io.StringIO(phases.stdout))]
        results.append(dict(header=label, path=str(header),
                            sha256=hashlib.sha256(header.read_bytes()).hexdigest(),
                            command=command, metrics=rows, phase=phase_rows))
        print(label, len(rows), "measurements", flush=True)
    version = subprocess.run([compiler, *(["--version"] if "clang" in compiler.lower() else [])],
                             capture_output=True, text=True, errors="replace")
    (output / "results.json").write_text(json.dumps(dict(
        compiler_version=version.stdout + version.stderr, flags=flags,
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), results=results), indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
