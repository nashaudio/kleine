"""Compile unchanged model sources against either Klang header, one TU per file.

Run from an x64 Visual Studio developer environment. Generated wrappers, objects,
compiler diagnostics and machine-readable results belong under build/.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
COLLECTIONS = ("examples", "sounds", "farnell/klang")
HEADERS = {
    "baseline": ROOT / "experiments/klang/core/baselines/production-2026-09-17/klang.h",
    "candidate": ROOT / "include/klang.h",
}
SOUND_TYPES = {
    "Bicycle": ["Bicycle"], "Harrier": ["Harrier"],
    "Helicopter": ["Helicopter"], "Mini": ["Mini"],
    "Motors": ["ToyBoatEngine", "FourStrokeEngine", "Car"],
    "Nature": ["Rain"], "Train": ["Train"],
}
FARNELL_TYPES = {
    "alarmgenerator": ["AlarmGenerator"], "dtmftones": ["DTMFTones"],
    "pedestrians": ["Pedestrians"], "phoneeffects": ["PhoneEffects"],
    "phonetones": ["PhoneTones"], "police": ["Police"],
    "boing": ["Boing"], "bouncing": ["Bouncing"],
    "creaking": ["Creaking"], "rolling": ["Rolling"],
    "studies": ["BellStudies"], "telephonebell": ["TelephoneBell"],
    "common": ["SampleDelay", "ControlRandom"],
    "boing-exponential": ["variants::BoingExponential"],
    "boing-power": ["variants::BoingPower"],
    "bouncing-exponential": ["variants::BouncingExponential"],
    "bouncing-power": ["variants::BouncingPower"],
    "dtmf-unfiltered": ["variants::DTMFUnfiltered"],
    "police-exponential": ["variants::PoliceExponential"],
    "oscillators": ["variants::" + name for name in (
        "ExponentialOsc", "PowerOsc", "SymmetricExponentialOsc",
        "InvertedExponentialOsc", "TriangleOsc")],
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def discover():
    cases = []
    for collection in COLLECTIONS:
        for path in sorted((ROOT / collection).rglob("*")):
            if path.suffix not in (".k", ".h", ".cpp"):
                continue
            if collection == "examples":
                types = ["::" + path.stem]
            elif collection == "sounds":
                types = ["::" + name for name in SOUND_TYPES[path.stem]]
            else:
                types = ["::farnell::" + name for name in FARNELL_TYPES[path.stem]]
            cases.append(dict(source=path.relative_to(ROOT).as_posix(),
                              collection=collection, types=types, sha256=digest(path)))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="cl.exe")
    parser.add_argument("--driver", choices=["cl", "gnu"], default="cl")
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--baseline-header", type=Path)
    parser.add_argument("--candidate-header", type=Path)
    parser.add_argument("--header", choices=(*HEADERS, "both", "reference"), default="both")
    parser.add_argument("--reference-header", type=Path,
                        help="Additional read-only header to check with --header reference")
    parser.add_argument("--output", default="build/core-compatibility/msvc")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--fast", action="store_true", help="Compile with Klang Studio floating-point settings")
    parser.add_argument("--require-all", action="store_true",
                        help="Fail for any model compilation failure, including baseline failures")
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error("Compiler not found; use an x64 Visual Studio developer environment")
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / "build"):
        parser.error("Generated output must stay under this repository's build/ directory")
    output.mkdir(parents=True, exist_ok=True)
    for label in ("baseline", "candidate"):
        path = getattr(args, label+"_header")
        if path:
            HEADERS[label] = path.resolve(strict=True)
    if args.reference_header:
        HEADERS["reference"] = args.reference_header.resolve(strict=True)
    if args.header == "reference" and "reference" not in HEADERS:
        parser.error("--header reference requires --reference-header")
    cases = discover()
    labels = ["baseline", "candidate"] if args.header == "both" else [args.header]
    flags = ["/nologo", "/std:c++17", "/EHsc", "/Od" if args.debug else "/O2", "/fp:fast" if args.fast else "/fp:precise", "/MTd" if args.debug else "/MT",
             "/D_DEBUG" if args.debug else "/DNDEBUG", "/DNOMINMAX", "/D_CRT_SECURE_NO_WARNINGS", "/W3",
             "/wd4244", "/wd4305", "/c", "/showIncludes"]
    if args.driver == "gnu":
        flags = ['-std=c++17', '-fexceptions', '-O0' if args.debug else '-O2',
                 '-ffast-math' if args.fast else '-fno-fast-math', '-static',
                 '-D_DEBUG' if args.debug else '-DNDEBUG', '-DNOMINMAX',
                 '-D_CRT_SECURE_NO_WARNINGS', '-H', '-c']
    version_args = ["--version"] if args.driver == "gnu" or "clang" in Path(compiler).name.lower() else []
    proc = subprocess.run([compiler, *version_args], capture_output=True,
                          text=True, errors="replace")
    version = proc.stdout + proc.stderr
    header_hashes = {label: digest(HEADERS[label]) for label in labels}

    def compile_case(label, index, case):
        directory = output / label / f"{index:03d}"
        directory.mkdir(parents=True, exist_ok=True)
        source = ROOT / case["source"]
        wrapper = directory / "model.cpp"
        wrapper.write_text(
            f'#include "{source.as_posix()}"\n'
            'template<class T> void instantiateModel() {\n'
            '    static_assert(sizeof(T) > 0);\n'
            '    static_assert(std::is_default_constructible_v<T>);\n'
            '    auto* model = new T;\n'
            '    delete model;\n'
            '}\n' + ''.join(f'template void instantiateModel<{name}>();\n'
                             for name in case["types"]), encoding="utf-8")
        header = HEADERS[label]
        command = [compiler, *flags, "/I" + str(header.parent),
                   "/I" + str(ROOT / "include"), "/I" + str(ROOT / "examples"),
                   str(wrapper), "/Fo" + str(directory / "model.obj")]
        if args.driver == 'gnu':
            command = [compiler, *flags, '-I'+str(header.parent), '-I'+str(ROOT/'include'),
                       '-I'+str(ROOT/'examples'), '-MD', '-MF', str(directory/'model.d'),
                       str(wrapper), '-o', str(directory/'model.obj')]
        proc = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                              errors="replace", timeout=180)
        log = proc.stdout + proc.stderr
        (directory / "compiler.log").write_text(log, encoding="utf-8")
        # Verify the compiler actually selected this header, including on failed cases.
        includes = (directory/'model.d').read_text() if args.driver == 'gnu' and (directory/'model.d').exists() else log
        included = header.as_posix().lower() in includes.replace("\\", "/").lower()
        errors = [line.strip() for line in log.splitlines()
                  if re.search(r"(?:fatal )?error(?: [A-Z]\d+)?\s*:", line)]
        return dict(header=label, **case, returncode=proc.returncode,
                    selected_header_verified=included,
                    passed=proc.returncode == 0 and included,
                    errors=errors, command=command,
                    log=(directory / "compiler.log").relative_to(ROOT).as_posix())

    results = []
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        pending = [pool.submit(compile_case, label, index, case)
                   for label in labels for index, case in enumerate(cases)]
        for future in as_completed(pending):
            result = future.result()
            results.append(result)
            if len(results) % 10 == 0 or len(results) == len(pending):
                print(f"Compiled {len(results)}/{len(pending)}", flush=True)

    results.sort(key=lambda row: (row["header"], row["source"]))
    unchanged = (all(digest(ROOT / case["source"]) == case["sha256"] for case in cases)
                 and all(digest(HEADERS[label]) == header_hashes[label] for label in labels))
    regressions = []
    if len(labels) == 2:
        old = {row["source"]: row for row in results if row["header"] == "baseline"}
        regressions = [row["source"] for row in results if row["header"] == "candidate"
                       and old[row["source"]]["passed"] and not row["passed"]]
    summary = {label: {collection: dict(
        passed=sum(row["passed"] for row in results
                   if row["header"] == label and row["collection"] == collection),
        total=sum(row["header"] == label and row["collection"] == collection
                  for row in results)) for collection in COLLECTIONS} for label in labels}
    report = dict(date=datetime.now(timezone.utc).isoformat(), compiler=compiler,
                  compiler_version=version.strip(), header_sha256=header_hashes,
                  sources_unchanged_during_run=unchanged, summary=summary,
                  regressions=regressions, results=results)
    (output / "results.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for label in labels:
        passed = sum(row["passed"] for row in results if row["header"] == label)
        print(f"{label}: {passed}/{len(cases)} compile")
    print(f"Regressions: {len(regressions)}; report: {output / 'results.json'}")
    failed = (not unchanged or regressions or
              any(not row["selected_header_verified"] for row in results) or
              (args.require_all and any(not row["passed"] for row in results)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
