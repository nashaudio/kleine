"""Check the current env output against PD, with delivery timing kept explicit."""
import hashlib
import json
import re
import shutil
from check_pd_primitives import ROOT, render_pd, run, sf, np, metrics


def publications(rate, case):
    work = ROOT/'build/env-publication'/str(rate)/case
    work.mkdir(parents=True,exist_ok=True)
    frames = (rate+63)//64*64
    samples = np.zeros(frames,dtype='<f4')
    if case == 'constant': samples[:] = .25
    elif case == 'gated':
        start, end = rate//10//64*64, rate//2//64*64
        samples[start:end] = .25*np.cos(2*np.pi*697*np.arange(start,end)/rate)
    elif case == 'impulses': samples[[0,63,64,511,512,1023]] = 1
    samples.tofile(work/'input.f32')
    sf.write(work/'input.wav',samples,rate,subtype='FLOAT')
    shutil.copyfile(ROOT/'tests/pd/env-publication.pd',work/'meter.pd')
    lines = ['#N canvas 0 0 700 500 10;','#X obj 20 20 loadbang;',
             '#X obj 20 50 t b b b;',
             f'#X msg 250 80 read -resize {(work/"input.wav").as_posix()} input;',
             '#X obj 250 110 soundfiler;',r'#X msg 140 80 \; pd dsp 1;',
             '#X obj 20 110 tabplay~ input;',f'#X obj 20 150 meter {rate};',
             '#X obj 450 20 array define input;','#X obj 450 80 del 1010;',
             r'#X msg 450 110 \; pd quit;',
             '#X connect 0 0 1 0;','#X connect 1 2 2 0;','#X connect 2 0 3 0;',
             '#X connect 1 1 4 0;','#X connect 1 0 5 0;','#X connect 5 0 6 0;',
             '#X connect 0 0 8 0;','#X connect 8 0 9 0;']
    (work/'render.pd').write_text('\n'.join(lines)+'\n')
    pd = run(['C:/Program Files/Pd/bin/pd.exe','-nogui','-stderr','-noprefs','-noaudio',
              '-nomidi','-batch','-r',str(rate),'-open',str(work/'render.pd')],work)
    klang = run([str(ROOT/'build/env-publication/env.exe'),str(rate),str(work/'input.f32')],work)
    pe = [(int(frame),float(level)) for frame,level in re.findall(r'ENV: (\d+) (\S+)',pd['stderr'])
          if int(frame) < frames]
    ke = [(int(frame),float(level)) for frame,level in (line.split() for line in klang['stdout'].splitlines())]
    assert pe and [e[0] for e in pe] == [e[0] for e in ke], (rate,case,'Publication frames differ')
    error = max(abs(p[1]-k[1]) for p,k in zip(pe,ke))
    # PD print truncates to six significant digits; the WAV test has tighter precision.
    assert error < .0001, (rate,case,error)
    print(rate,case,len(ke),'publications; max printed-level error',error,flush=True)
    return dict(rate=rate,case=case,frames=frames,passed=True,pd_events=pe,klang_events=ke,
                max_abs_printed_error=error,input_sha256=hashlib.sha256(samples.tobytes()).hexdigest(),
                pd=pd,klang=klang)


def main():
    results = []
    events = []
    for rate in (48000,44100):
        frames = (rate+63)//64*64
        work = ROOT/'build/env-publication'/str(rate)
        output, pd = render_pd('env', '', rate, frames, '0.55', work, 'C:/Program Files/Pd/bin/pd.exe')
        klang = run([str(ROOT/'build/x64-release/kleine.exe'), '--render', 'pd-env',
                     str(work/'kleine.wav'), str(frames/rate), str(rate)], work)
        a, ar = sf.read(output)
        b, br = sf.read(work/'kleine.wav')
        assert ar == br == rate and len(a) == len(b) == frames
        assert np.isfinite(a).all() and np.isfinite(b).all()
        raw = metrics(a,b)
        assert abs(raw['level_delta_db']) < .002 and raw['max_abs_error'] < .00002
        results.append(dict(rate=rate,frames=frames,passed=True,raw_comparison=raw,
                            pd=pd,klang=klang))
        print(rate,'raw max error',raw['max_abs_error'],flush=True)
        for case in ('silence','constant','gated','impulses'):
            events.append(publications(rate,case))
    paths = ['include/klang/pd.h','tests/pd/env.pd','tests/pd/primitive.h',
             'tests/pd/env-publication.pd','tests/pd/env-publication.cpp',
             'tools/check_pd_primitives.py','tools/check_pd_env.py']
    report = dict(tests=results,publication_tests=events,
                  source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
                  executable_sha256=hashlib.sha256((ROOT/'build/x64-release/kleine.exe').read_bytes()).hexdigest(),
                  publication_executable_sha256=hashlib.sha256((ROOT/'build/env-publication/env.exe').read_bytes()).hexdigest(),
                  notes=['PD 0.55.2, block64, mono float32; 697 Hz cosine at amplitude 0.25; default 1024/512 env.',
                         'out and updated publish the pending analysis together at the following-block boundary. No public level state.',
                         'Raw samples and actual publication frames compared directly: no delivery projection, gain fitting, alignment or resampling.',
                         'Shared-input event cases include silence, repeated equal values, gating/release and impulses across block boundaries.',
                         'PD printed event values have six significant digits; event tolerance is 0.0001 dB, raw WAV tolerance 0.00002 dB.'])
    (ROOT/'tests/pd/env-output-results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__': main()
