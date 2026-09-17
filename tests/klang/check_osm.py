"""Gate the working OSM against independent waveform and lifecycle contracts."""
import ctypes
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    if sys.platform == 'win32':
        ctypes.windll.kernel32.SetErrorMode(0x8003)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='cl.exe')
    parser.add_argument('--driver', choices=['cl', 'gnu'], default='cl')
    parser.add_argument('--output', default='build/osm-regression/msvc')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--fast', action='store_true', help='Exercise /fp:fast as used by Klang Studio; precise remains the reference')
    parser.add_argument('--header', type=Path, help='Check a specific workspace klang.h prototype')
    parser.add_argument('--profile', choices=['fast', 'multicycle', 'fractional-duty', 'combined'], default='combined',
                        help='Combined production contract by default; narrower profiles support diagnostics')
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error('Use an x64 VS developer environment')
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT / 'build'):
        parser.error('Output must be under build/')
    output.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'tests/klang/osm-quality.cpp'
    header = ROOT / 'include/klang.h'
    if args.header:
        header = args.header.resolve(strict=True)
        if header.name != 'klang.h' or not header.is_relative_to(ROOT):
            parser.error('--header must name a workspace klang.h')
    flags = ['/nologo', '/std:c++17', '/EHsc', '/Od' if args.debug else '/O2',
             '/fp:fast' if args.fast else '/fp:precise', '/MTd' if args.debug else '/MT', '/D_DEBUG' if args.debug else '/DNDEBUG',
             '/DNOMINMAX', '/D_CRT_SECURE_NO_WARNINGS',
             '/W3', '/wd4244', '/wd4305', '/showIncludes']
    if args.driver == 'gnu':
        flags = ['-std=c++17', '-fexceptions', '-O0' if args.debug else '-O2',
                 '-ffast-math' if args.fast else '-fno-fast-math', '-static',
                 '-D_DEBUG' if args.debug else '-DNDEBUG', '-DNOMINMAX',
                 '-D_CRT_SECURE_NO_WARNINGS', '-H', '-MD', '-MF', str(output/'contracts.d')]
    define = '-D' if args.driver == 'gnu' else '/D'
    if args.profile in ['multicycle', 'combined']:
        flags.append(define+'OSM_MULTICYCLE')
    if args.profile in ['fractional-duty', 'combined']:
        flags.append(define+'OSM_FRACTIONAL_DUTY')
    exe = output / 'contracts.exe'
    command = [compiler, *flags, '/I' + str(header.parent), str(source),
               '/Fo' + str(output / 'contracts.obj'), '/Fe' + str(exe)]
    if args.driver == 'gnu':
        command = [compiler, *flags, '-I'+str(header.parent), str(source), '-o', str(exe)]
    inputs = [source, header, ROOT / 'tests/klang/oscillator-reference.h', Path(__file__)]
    hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    compile = subprocess.run(command, text=True, capture_output=True, errors='replace', timeout=120)
    log = compile.stdout + compile.stderr
    (output / 'compiler.log').write_text(log, encoding='utf-8')
    includes = (output/'contracts.d').read_text() if args.driver == 'gnu' and (output/'contracts.d').exists() else log
    selected = header.as_posix().lower() in includes.replace('\\', '/').lower()
    result = None
    if compile.returncode == 0 and selected:
        run = subprocess.run([str(exe)], text=True, capture_output=True, timeout=60)
        result = dict(returncode=run.returncode, output=run.stdout + run.stderr)
        print(result['output'])
    else:
        print(log[-6000:])
    unchanged = all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h for p, h in hashes.items())
    version = subprocess.run([compiler, *(['--version'] if args.driver == 'gnu' or 'clang' in compiler.lower() else [])],
                             text=True, capture_output=True, errors='replace')
    report = dict(date=datetime.now(timezone.utc).isoformat(), profile=args.profile, driver=args.driver, compiler_version=version.stdout + version.stderr,
                  command=command, hashes=hashes, sources_unchanged=unchanged,
                  compile_returncode=compile.returncode, selected_header_verified=selected, result=result)
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if unchanged and result and result['returncode'] == 0 else 1


if __name__ == '__main__':
    raise SystemExit(main())
