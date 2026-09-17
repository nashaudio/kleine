"""Run language contracts against the production and experimental headers.

Use an x64 VS developer environment. Expected failures must fail at the named
expression with a matching diagnostic; an unrelated compiler failure is an error.
"""
from __future__ import annotations
import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
HEADERS = {"baseline": ROOT / "experiments/klang/core/baselines/production-2026-09-17/klang.h",
           "candidate": ROOT / "include/klang.h"}


def main():
    if sys.platform=='win32':
        ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="cl.exe")
    parser.add_argument('--driver',choices=['cl','gnu'],default='cl')
    parser.add_argument('--fast',action='store_true')
    parser.add_argument('--debug',action='store_true')
    parser.add_argument("--output", default="build/language-regression/msvc")
    parser.add_argument("--game-header", type=Path,
                        help="Check the supplied 0.7.9 core with its distinct abs/mixing/routing contracts")
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error("Compiler not found; use an x64 VS developer environment")
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / "build"):
        parser.error("Output must be under build/")
    if args.game_header:
        HEADERS["game"] = args.game_header.resolve(strict=True)
    flags = ["/nologo", "/std:c++17", "/EHsc", "/O2", "/fp:precise", "/MT",
             "/DNDEBUG", "/DNOMINMAX", "/DWIN32", "/D_WINDOWS",
             "/D_CRT_SECURE_NO_WARNINGS", "/W3", "/wd4244", "/wd4305", "/showIncludes"]
    if args.fast:
        flags[flags.index('/fp:precise')]='/fp:fast'
    if args.debug:
        flags[flags.index('/O2')]='/Od'
        flags[flags.index('/MT')]='/MTd'
        flags[flags.index('/DNDEBUG')]='/D_DEBUG'
    if args.driver=='gnu':
        flags=['-std=c++17','-fexceptions','-O0' if args.debug else '-O2',
               '-ffast-math' if args.fast else '-fno-fast-math','-static',
               '-D_DEBUG' if args.debug else '-DNDEBUG','-DNOMINMAX','-D_CRT_SECURE_NO_WARNINGS','-H']
    clang = args.driver=='gnu' or "clang" in Path(compiler).name.lower()
    cases = [
        ("contracts", HERE / "arithmetic.cpp", [], True, True, None),
        ("channel-broadcast", HERE / "channel-broadcast.cpp", [], True, True, None),
        ("basic-oscillators", HERE / "basic-oscillators.cpp", [], True, True, None),
        ("control-routing", HERE / "control-routing.cpp", [], True, True, None),
        ("tables", HERE / "tables.cpp", [], True, True, None),
        ("follower-flow", HERE / "follower-flow.cpp", [], True, True, None),
        ("inline-route", HERE / "ambiguities.cpp", ["INLINE_ROUTE"], False, False,
         r"C2593|ambiguous|no viable overloaded '='"),
        ("envelope-parameter", HERE / "ambiguities.cpp", ["ENVELOPE_PARAMETER"], False, None,
         r"C2664|C2665|no viable conversion|no matching member function"),
        ("function-left-scalar", HERE / "ambiguities.cpp", ["FUNCTION_LEFT_SCALAR"], False, None,
         r"C2666|ambiguous"),
        ("bank-mono", HERE / "ambiguities.cpp", ["BANK_MONO"], False, None,
         r"C2679|invalid operands"),
        ("control-route", HERE / "ambiguities.cpp", ["CONTROL_ROUTE"], False, clang,
         r"C2593|ambiguous"),
        ("scalar-temporary", HERE / "ambiguities.cpp", ["SCALAR_TEMPORARY"], False, None,
         r"C2676|C2678|C2679|invalid operands|no match|C2672"),
        ("literals", ROOT / "experiments/klang/literals/probe.cpp", [], True, True, None),
    ]
    results = []
    hash_before = {str(path.relative_to(ROOT) if path.is_relative_to(ROOT) else path):
                   hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in {*HEADERS.values(), *(c[1] for c in cases)}}
    for label, header in HEADERS.items():
        for name, source, defines, run, expected, diagnostic in cases:
            directory = output / label / name
            directory.mkdir(parents=True, exist_ok=True)
            case_defines = defines + (["RESTORED_ARITHMETIC"]
                                     if label != "baseline" and name == "contracts" else [])
            if label == "game" and name == "contracts":
                case_defines += ["GAME_HEADER"]
            if label == "candidate" and name == "contracts":
                case_defines += ["CHANNEL_REDUCTIONS", "STATELESS_MATH_FUNCTIONS", "GENERATOR_PARAM"]
            if label == "candidate" and name == "basic-oscillators":
                case_defines += ["HARDENED_BASIC"]
            if label == "candidate" and name == "control-routing":
                case_defines += ["SIMPLIFIED_CONTROL_ROUTING"]
            if label == "candidate" and name == "tables":
                case_defines += ["CALLABLE_TABLES"]
            if label == "candidate" and name == "follower-flow":
                case_defines += ["COMPARE_COUPLED_FOLLOWER"]
            should_compile = label != "baseline" if expected is None else expected
            if label == "game" and name in ("function-left-scalar", "bank-mono", "control-route"):
                should_compile = True
            if name == "control-route":
                should_compile = label == "candidate" or clang
            exe = directory / "case.exe"
            command = [compiler, *flags, *("/D" + d for d in case_defines),
                       "/I" + str(header.parent), "/I" + str(ROOT / "include"),
                       str(source), "/Fo" + str(directory / "case.obj")]
            command += ["/Fe" + str(exe)] if run else ["/c"]
            if args.driver=='gnu':
                command=[compiler,*flags,*('-D'+d for d in case_defines),'-I'+str(header.parent),
                         '-I'+str(ROOT/'include'),'-MD','-MF',str(directory/'case.d'),str(source)]
                command += ['-o',str(exe)] if run else ['-c','-o',str(directory/'case.obj')]
            proc = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=180)
            log = proc.stdout + proc.stderr
            (directory / "compiler.log").write_text(log, encoding="utf-8")
            includes=(directory/'case.d').read_text() if args.driver=='gnu' and (directory/'case.d').exists() else log
            selected = header.as_posix().lower() in includes.replace("\\", "/").lower()
            compiled = proc.returncode == 0
            matched = bool(diagnostic and re.search(diagnostic, log, re.I)
                           and re.search(r'ambiguities\.cpp[:(]',log))
            okay = selected and (compiled if should_compile else not compiled and matched)
            runtime = None
            if compiled and run:
                execution = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
                runtime = dict(returncode=execution.returncode, output=execution.stdout + execution.stderr)
                if name == "channel-broadcast":
                    fixed = label == "candidate"
                    runtime["expected_fixed"] = fixed
                    expected_output = "channels=2,2,2" if fixed else "channels=2,0,0"
                    okay = okay and execution.returncode == (0 if fixed else 1) and execution.stdout.strip() == expected_output
                else:
                    okay = okay and execution.returncode == 0
            results.append(dict(header=label, case=name, passed=okay, compiled=compiled,
                                expected_compile=should_compile, diagnostic_matched=matched,
                                selected_header_verified=selected, runtime=runtime,
                                command=command, log=(directory / "compiler.log").relative_to(ROOT).as_posix()))
            print(label, name, "PASS" if okay else "FAIL", runtime or "", flush=True)
    version = subprocess.run([compiler, *(["--version"] if clang else [])],
                             capture_output=True, text=True, errors="replace")
    report = dict(date=datetime.now(timezone.utc).isoformat(), compiler=compiler,
                  compiler_version=version.stdout + version.stderr, flags=flags,
                  header_sha256={label: hashlib.sha256(path.read_bytes()).hexdigest()
                                 for label, path in HEADERS.items()}, source_sha256=hash_before,
                  sources_unchanged=all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == h
                                        for path, h in hash_before.items()), results=results)
    (output / "results.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0 if report["sources_unchanged"] and all(r["passed"] for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
