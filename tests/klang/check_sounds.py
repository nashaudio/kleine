"""Compare raw sound compilation and explicit standalone-host renders by header.

Optionally extracts two pinned upstream headers from a read-only local Git repo.
Never modifies sound sources or upstream checkouts. Run in an x64 VS environment.
Uses Python's standard library; all generated files stay under build/.
"""
from __future__ import annotations
import argparse
import ctypes
from array import array
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
UPSTREAM = {
    "main": ("88b849c38997f4c34cadc4b92d1457e8ef43deea", "klang.h"),
    "plugins": ("3dc002be66a32166458b9679c11e94e437e819a1", "include/klang.h"),
}
CASES = [
    ("Bicycle", "Bicycle", ["HOST_PHASOR", "HOST_TANH"]),
    ("Harrier", "Harrier", []),
    ("Helicopter", "Helicopter", ["HOST_TANH"]),
    ("Mini", "Mini", ["HOST_TANH"]),
    ("Motors", "Car", ["HOST_PHASOR"]), # Known-broken legacy model; not scheduled.
    ("Motors", "ToyBoatEngine", ["HOST_PHASOR"]),
    ("Motors", "FourStrokeEngine", ["HOST_PHASOR"]), # Known-broken legacy model; not scheduled.
    ("Nature", "Rain", []),
    ("Train", "Train", ["HOST_REVERB"]),
]
ACTIVE_SOUND_KINDS = {0, 1, 2, 3, 5, 7, 8}
RAW_SOUND_KINDS = {0, 1, 2, 3, 5, 7, 8}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def samples_from(path):
    values = array("f")
    values.frombytes(path.read_bytes())
    if sys.byteorder != "little":
        values.byteswap()
    return values


def main():
    if sys.platform=='win32':
        # Child tests inherit this process-local setting; keep access violations
        # as exit codes without displaying Windows application-error dialogs.
        ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="cl.exe")
    parser.add_argument("--driver", choices=['cl','gnu'], default='cl')
    parser.add_argument("--debug", action='store_true')
    parser.add_argument("--fast", action='store_true')
    parser.add_argument("--baseline-header", type=Path)
    parser.add_argument("--candidate-header", type=Path)
    parser.add_argument("--examples", action='store_true', help='Also compile and render all unchanged examples')
    parser.add_argument("--inspect", action='store_true', help='Export WAVs, plots and an HTML inspection report')
    parser.add_argument("--source", action='append', help='Limit to these repository-relative source paths')
    parser.add_argument("--allow-output-changes", action='store_true', help='Report waveform changes separately from invalid renders')
    parser.add_argument("--output", default="build/sound-headers/msvc")
    parser.add_argument("--upstream-repo", type=Path)
    parser.add_argument("--game-header", type=Path,
                        help="Snapshot the supplied UE plugin core as an additional reference")
    parser.add_argument("--ue-language-flags", action="store_true",
                        help="Use the observed UE C++20/conformance/RTTI flags; still a standalone build")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error("Compiler not found; use an x64 VS developer environment")
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / "build"):
        parser.error("Output must be under build/")
    output.mkdir(parents=True, exist_ok=True)
    headers = {"baseline": ROOT / "experiments/klang/core/baselines/production-2026-09-17/klang.h",
               "candidate": ROOT / "include/klang.h"}
    for label in ['baseline','candidate']:
        path = getattr(args,label+'_header')
        if path:
            headers[label] = path.resolve(strict=True)
    cases = [(ROOT/'sounds'/(stem+'.h'), model, bridges) for stem,model,bridges in CASES]
    if args.examples:
        cases += [(p,p.stem,[]) for p in sorted((ROOT/'examples').rglob('*.k'))]
    if args.upstream_repo:
        for label, (revision, path) in UPSTREAM.items():
            content = subprocess.run(["git", "-C", str(args.upstream_repo), "show", revision + ":" + path],
                                     capture_output=True, check=True).stdout
            header = output / "headers" / label / "klang.h"
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_bytes(content)
            headers[label] = header
    game_reference = None
    if args.game_header:
        original = args.game_header.resolve(strict=True)
        header = output / "headers/game/klang.h"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_bytes(original.read_bytes())
        headers["game"] = header
        game_reference = dict(path=str(original), sha256=digest(original))
    sources = sorted({c[0] for c in cases})
    sources += [ROOT / "include/klang/pd.h", ROOT / "include/klang/utils.h",
                ROOT / "examples/Reverb.k", ROOT / "examples/Delay/Reverb2.k",
                HERE / "sound-host.h", HERE / "sound-render.h", HERE/'example-render.h', Path(__file__)]
    hashes = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    header_hashes = {label: digest(path) for label, path in headers.items()}
    native_phasor = {label: bool(re.search(r'\bstruct\s+Phasor\b', path.read_text()))
                     for label, path in headers.items()}
    flags = ["/nologo", "/std:c++17", "/EHsc", "/O2", "/fp:precise", "/MT", "/DNDEBUG",
             "/DNOMINMAX", "/DWIN32", "/D_WINDOWS", "/D_CRT_SECURE_NO_WARNINGS",
             "/W3", "/wd4244", "/wd4305", "/showIncludes"]
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
    if args.ue_language_flags:
        flags[flags.index("/std:c++17")] = "/std:c++20"
        flags += ["/GR-", "/permissive-", "/Zc:strictStrings-", "/Zc:__cplusplus",
                  "/Zc:inline", "/Zc:preprocessor"]

    def compile_case(task):
        label, mode, kind = task
        source, model, bridges = cases[kind]
        example = source.is_relative_to(ROOT/'examples')
        directory = output / label / mode / (f'{kind:03d}-'+model)
        directory.mkdir(parents=True, exist_ok=True)
        wrapper = directory / "case.cpp"
        content = '#include <klang.h>\n'
        defines = []
        if mode == "host-fixture" and not example:
            defines = bridges + (["UPSTREAM_CORE"] if label in UPSTREAM else [])
            if native_phasor[label]:
                defines = [d for d in defines if d != "HOST_PHASOR"]
            content += f'#include "{(HERE / "sound-host.h").as_posix()}"\n'
        content += f'#include "{source.as_posix()}"\n'
        content += f'using Model = ::{model};\n'
        if mode == "host-fixture":
            content += f'#include "{(HERE / ("example-render.h" if example else "sound-render.h")).as_posix()}"\n'
        else:
            content += 'void instantiate() { auto* model = new Model; delete model; }\n'
        wrapper.write_text(content, encoding="utf-8")
        exe = directory / "case.exe"
        command = [compiler, *flags, *("/D" + d for d in defines), f"/DSOUND_KIND={kind}",
                   "/I" + str(headers[label].parent), "/I" + str(ROOT / "include"),
                   "/I" + str(ROOT / "examples"), str(wrapper), "/Fo" + str(directory / "case.obj")]
        command += ["/Fe" + str(exe)] if mode == "host-fixture" else ["/c"]
        if args.driver=='gnu':
            command=[compiler,*flags,*('-D'+d for d in defines),f'-DSOUND_KIND={kind}',
                     '-I'+str(headers[label].parent),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'examples'),
                     '-MD','-MF',str(directory/'case.d'),str(wrapper)]
            command += ['-o',str(exe)] if mode=='host-fixture' else ['-c','-o',str(directory/'case.obj')]
        proc = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=180)
        log = proc.stdout + proc.stderr
        (directory / "compiler.log").write_text(log, encoding="utf-8")
        includes=(directory/'case.d').read_text() if args.driver=='gnu' and (directory/'case.d').exists() else log
        included = headers[label].as_posix().lower() in includes.replace("\\", "/").lower()
        if args.driver=='gnu' and not included:
            # The DLL's link route may suppress diagnostics after a failed compile.
            diagnostic=[compiler,*flags,*('-D'+d for d in defines),f'-DSOUND_KIND={kind}',
                        '-I'+str(headers[label].parent),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'examples'),
                        '-fsyntax-only',str(wrapper)]
            diag=subprocess.run(diagnostic,capture_output=True,text=True,errors='replace',timeout=180)
            log += diag.stdout+diag.stderr
            (directory/'compiler.log').write_text(log,encoding='utf-8')
            included=headers[label].as_posix().lower() in log.replace('\\','/').lower()
        errors = [line.strip() for line in log.splitlines()
                  if re.search(r"(?:fatal )?error(?: [A-Z]\d+)?\s*:", line)]
        result = dict(header=label, mode=mode, source=source.relative_to(ROOT).as_posix(), model=model,
                      bridges=defines, compiled=proc.returncode == 0 and included,
                      selected_header_verified=included, errors=errors, command=command,
                      log=(directory / "compiler.log").relative_to(ROOT).as_posix(), renders=[])
        if result["compiled"] and mode == "host-fixture":
            for rate in (44100, 48000):
                audio = directory / f"{rate}.f32"
                run = subprocess.run([str(exe), str(rate), str(audio)], capture_output=True,
                                     text=True, timeout=120)
                render = dict(rate=rate, returncode=run.returncode, output=run.stdout + run.stderr,
                              path=audio.relative_to(ROOT).as_posix())
                if run.returncode == 0:
                    samples = samples_from(audio)
                    safe = [value for value in samples if math.isfinite(value)]
                    channels=int(re.search(r'channels=(\d+)',run.stdout).group(1)) if example else 2 if kind==8 else 1
                    expected = rate * 6 * channels
                    render.update(samples=len(samples), expected_samples=expected,
                                  channels=channels,
                                  finite=len(safe) == len(samples), nonfinite=len(samples) - len(safe),
                                  peak=max(map(abs, safe)) if safe else None,
                                  rms=math.sqrt(sum(v * v for v in safe) / len(safe)) if safe else None,
                                  mean=sum(safe) / len(safe) if safe else None,
                                  over_unity=sum(abs(v) > 1 for v in safe), sha256=digest(audio))
                    if label == "candidate":
                        repeat = directory / f"{rate}-repeat.f32"
                        rerun = subprocess.run([str(exe), str(rate), str(repeat)], capture_output=True,
                                               text=True, timeout=120)
                        render["repeat_identical"] = rerun.returncode == 0 and digest(repeat) == digest(audio)
                result["renders"].append(render)
        (directory/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(label, mode, model, "COMPILES" if result["compiled"] else "FAILS", flush=True)
        return result

    tasks = [(label, mode, kind) for label in headers for mode in ("raw", "host-fixture")
             for kind in range(len(cases))
             if kind >= len(CASES) or kind in ACTIVE_SOUND_KINDS and
             (mode != "raw" or kind in RAW_SOUND_KINDS)]
    if args.source:
        selected = {p.replace('\\', '/') for p in args.source}
        known = {c[0].relative_to(ROOT).as_posix() for c in cases}
        if selected - known:
            parser.error('Unknown sources: ' + ', '.join(sorted(selected - known)))
        tasks = [task for task in tasks if cases[task[2]][0].relative_to(ROOT).as_posix() in selected]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(compile_case, tasks))
    comparisons = []
    for row in results:
        if row["header"] != "candidate":
            continue
        for other in results:
            if other["header"] == "candidate" or row["mode"] != other["mode"] or row["model"] != other["model"] or row['source']!=other['source']:
                continue
            for current in row["renders"]:
                reference = next((r for r in other["renders"] if r["rate"] == current["rate"]), None)
                if not reference or current["returncode"] or reference["returncode"]:
                    continue
                a = samples_from(ROOT / current["path"])
                b = samples_from(ROOT / reference["path"])
                finite = all(map(math.isfinite, a)) and all(map(math.isfinite, b))
                same_shape = len(a) == len(b) and current.get('channels',1) == reference.get('channels',1)
                delta = [x - y for x, y in zip(a, b)] if same_shape and finite else []
                comparisons.append(dict(model=row["model"],source=row['source'],reference=other["header"], rate=current["rate"],
                                        shape_equal=same_shape, sample_identical=same_shape and finite and a == b, finite=finite,
                                        residual_peak=max(map(abs, delta)) if delta else None,
                                        residual_rms=math.sqrt(sum(d * d for d in delta) / len(delta)) if delta else None))
    unchanged = all(digest(ROOT / p) == h for p, h in hashes.items())
    unchanged &= all(digest(headers[label]) == h for label, h in header_hashes.items())
    if game_reference:
        unchanged &= digest(args.game_header) == game_reference["sha256"]
    regressions = []
    old = {(r["mode"], r["source"], r["model"]): r for r in results if r["header"] == "baseline"}
    for row in results:
        if row["header"] != "candidate":
            continue
        if old[row["mode"], row['source'], row["model"]]["compiled"] and not row["compiled"]:
            regressions.append(f"{row['mode']} {row['model']}: compile regression")
        for render in row["renders"]:
            if (render["returncode"] or not render.get("finite") or not render.get("repeat_identical")
                    or render.get("samples") != render.get("expected_samples")):
                regressions.append(f"{row['model']} {render['rate']}: invalid or nondeterministic render")
    if not args.allow_output_changes:
        regressions += [f"{c['model']} {c['rate']}: baseline samples changed" for c in comparisons
                        if c["reference"] == "baseline" and not c["sample_identical"]]
    version = subprocess.run([compiler, *(["--version"] if args.driver=='gnu' or "clang" in compiler.lower() else [])],
                             capture_output=True, text=True, errors="replace")
    report = dict(date=datetime.now(timezone.utc).isoformat(), compiler=compiler,
                  compiler_version=version.stdout + version.stderr, flags=flags, upstream=UPSTREAM,
                  examples=args.examples,allow_output_changes=args.allow_output_changes,
                  game_reference=game_reference,
                  header_sha256=header_hashes, source_sha256=hashes, sources_unchanged=unchanged,
                  header_paths={label:str(path) for label,path in headers.items()},
                  recipe=dict(rates=[44100, 48000], seconds=6, prepare_samples=64, seed=12345,
                              controls="tests/klang/sound-render.h and tests/klang/example-render.h; sound controls at 0, 2, 4 seconds"),
                  results=results, comparisons=comparisons, regressions=regressions)
    (output / "results.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Regressions:", regressions, flush=True)
    if args.inspect:
        subprocess.run([sys.executable, str(HERE/'inspect_sounds.py'), str(output/'results.json')], check=True)
    return 0 if unchanged and not regressions and all(r["selected_header_verified"] for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
