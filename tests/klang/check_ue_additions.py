"""Compile/link/run UE integration contracts, including a second translation unit."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]

def main():
    if sys.platform == 'win32': ctypes.windll.kernel32.SetErrorMode(0x8003)
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--compiler', default='cl.exe')
    ap.add_argument('--driver', choices=['cl','gnu'], default='cl')
    ap.add_argument('--debug', action='store_true')
    ap.add_argument('--header', type=Path, default=ROOT/'include/klang.h')
    ap.add_argument('--output', type=Path, default=ROOT/'build/ue-integration/contracts')
    args = ap.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler: ap.error('Compiler unavailable; use a VS developer shell for MSVC.')
    output=args.output.resolve(); header=args.header.resolve()
    if not output.is_relative_to(ROOT/'build'): ap.error('Output must be under build/.')
    output.mkdir(parents=True,exist_ok=True)
    sources=[ROOT/'tests/klang/ue-additions.cpp',ROOT/'tests/klang/ue-link.cpp']
    hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [header,*sources]}
    exe=output/'contracts.exe'
    if args.driver=='cl':
        command=[compiler,'/nologo','/std:c++17','/EHsc','/Od' if args.debug else '/O2',
                 '/MTd' if args.debug else '/MT','/fp:fast','/D_DEBUG' if args.debug else '/DNDEBUG',
                 '/DNOMINMAX','/showIncludes','/I'+str(header.parent),*[str(p) for p in sources],
                 '/Fo'+str(output)+'/', '/Fe'+str(exe)]
    else:
        command=[compiler,'-std=c++17','-fexceptions','-O0' if args.debug else '-O2',
                 '-static','-ffast-math','-D_DEBUG' if args.debug else '-DNDEBUG','-DNOMINMAX',
                 '-H','-MD','-MF',str(output/'contracts.d'),'-I'+str(header.parent),*[str(p) for p in sources],'-o',str(exe)]
    compile=subprocess.run(command,capture_output=True,text=True,errors='replace',timeout=120)
    log=compile.stdout+compile.stderr
    (output/'compiler.log').write_text(log)
    includes=(output/'contracts.d').read_text() if args.driver=='gnu' and (output/'contracts.d').exists() else log
    selected=header.as_posix().lower() in includes.replace('\\','/').lower()
    run=None
    if compile.returncode==0:
        proc=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60)
        run=dict(returncode=proc.returncode,output=proc.stdout+proc.stderr)
    unchanged=all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in hashes.items())
    result=dict(command=command,hashes=hashes,compile_returncode=compile.returncode,
                selected_header_verified=selected,sources_unchanged=unchanged,result=run)
    (output/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(run or log[-4000:])
    return 0 if selected and unchanged and run and run['returncode']==0 else 1

if __name__=='__main__': raise SystemExit(main())
