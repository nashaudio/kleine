"""Exercise Studio/default/override capture settings in Release and Debug."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]

def main():
    if sys.platform=='win32': ctypes.windll.kernel32.SetErrorMode(0x8003)
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--compiler',default='cl.exe')
    ap.add_argument('--driver',choices=['cl','gnu'],default='cl')
    ap.add_argument('--header',type=Path,default=ROOT/'include/klang.h')
    ap.add_argument('--output',type=Path,default=ROOT/'build/debug-guard/msvc')
    args=ap.parse_args()
    compiler=shutil.which(args.compiler)
    if not compiler: ap.error('Compiler unavailable; use an x64 VS developer prompt.')
    header=args.header.resolve(); output=args.output.resolve()
    if not output.is_relative_to(ROOT/'build'): ap.error('Output must be under build/.')
    source=ROOT/'tests/klang/debug-guard.cpp'
    hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (header,source,Path(__file__))}
    results=[]
    for debug in (False,True):
        for name,definitions in (
            ('default-off',['EXPECT_CAPTURE=0']),
            ('studio-on',['KLANG=1','EXPECT_CAPTURE=1']),
            ('override-off',['KLANG=1','HAS_KLANG_DEBUG=0','EXPECT_CAPTURE=0']),
            ('override-on',['HAS_KLANG_DEBUG=1','EXPECT_CAPTURE=1'])):
            folder=output/('debug' if debug else 'release')/name
            folder.mkdir(parents=True,exist_ok=True); exe=folder/'test.exe'
            if args.driver=='cl':
                command=[compiler,'/nologo','/std:c++17','/EHsc','/Od' if debug else '/O2',
                    '/MTd' if debug else '/MT','/fp:fast','/D_DEBUG' if debug else '/DNDEBUG',
                    '/DNOMINMAX','/showIncludes',*('/D'+d for d in definitions),'/I'+str(header.parent),
                    str(source),'/Fo'+str(folder/'test.obj'),'/Fe'+str(exe)]
            else:
                command=[compiler,'-std=c++17','-fexceptions','-O0' if debug else '-O2','-static',
                    '-ffast-math','-D_DEBUG' if debug else '-DNDEBUG','-DNOMINMAX',
                    *('-D'+d for d in definitions),'-I'+str(header.parent),'-MD','-MF',str(folder/'test.d'),
                    str(source),'-o',str(exe)]
            build=subprocess.run(command,capture_output=True,text=True,errors='replace',timeout=120)
            log=build.stdout+build.stderr; (folder/'compiler.log').write_text(log)
            includes=(folder/'test.d').read_text() if args.driver=='gnu' and (folder/'test.d').exists() else log
            selected=header.as_posix().lower() in includes.replace('\\','/').lower()
            run=None
            if build.returncode==0 and selected:
                p=subprocess.run([str(exe)],capture_output=True,text=True,timeout=60)
                run=dict(returncode=p.returncode,output=p.stdout+p.stderr)
            row=dict(name=name,debug=debug,command=command,compile_returncode=build.returncode,
                     selected_header_verified=selected,run=run)
            results.append(row)
            print(folder.name,'Debug' if debug else 'Release',run or log[-2500:],flush=True)
    unchanged=all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in hashes.items())
    (output/'results.json').write_text(json.dumps(dict(hashes=hashes,sources_unchanged=unchanged,results=results),indent=2)+'\n')
    return 0 if unchanged and all(r['run'] and r['run']['returncode']==0 for r in results) else 1

if __name__=='__main__': raise SystemExit(main())
