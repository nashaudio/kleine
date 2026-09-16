"""Trace PD control delivery blocks and current Klang sample positions for PhoneEffects."""
import json
import shutil
import re
import hashlib
from render_farnell import ROOT, expose_output, run, sf, np
from render_artificial import replace_canvas


def tap(text, node, label):
    lines, count = expose_output(text)
    lines += [f'#X obj 700 400 timing {label};', f'#X connect {node} 0 {count} 0;']
    return '\n'.join(lines)+'\n'


def main():
    results = []
    for rate in (48000, 44100):
        for case in ('phone-effects', 'phone-match'):
            source = ROOT/f'build/artificial/{rate}/{case}'
            work = ROOT/f'build/phone-timing/{rate}/{case}'
            work.mkdir(parents=True, exist_ok=True)
            for p in source.glob('*.pd'): shutil.copyfile(p, work/p.name)
            shutil.copyfile(source/'events.tsv', work/'events.tsv')
            # Capture PD's logical message time as the DSP block it affects.
            (work/'timing.pd').write_text('\n'.join([
                '#N canvas 0 0 500 400 10;', '#X obj 20 20 inlet;',
                '#X obj 20 50 t b a;', '#X obj 150 50 list split 1;',
                '#X obj 150 80 unpack f;', '#X obj 20 110 timer;',
                '#X obj 280 20 loadbang;',
                f'#X obj 20 140 expr int($f1 * {rate} / 64000 + 0.00001);',
                '#X obj 20 170 pack f f;', r'#X obj 20 200 print TRACE-\$1;',
                '#X connect 0 0 1 0;', '#X connect 1 1 2 0;', '#X connect 2 0 3 0;',
                '#X connect 3 0 7 1;', '#X connect 1 0 4 1;', '#X connect 5 0 4 0;',
                '#X connect 4 0 6 0;', '#X connect 6 0 7 0;', '#X connect 7 0 8 0;'])+'\n')
            text = (work/'model.pd').read_text()
            text = replace_canvas(text, 'timebase', lambda t: tap(t, 1, 'reset'))
            for node, label in [(39,'programme'),(40,'programme'),(41,'programme'),(32,'digit'),(27,'release')]:
                text = tap(text, node, label)
            (work/'model.pd').write_text(text)
            wrapper = (work/'render.pd').read_text().replace(str(source/'pd.wav').replace('\\','/'), (work/'pd.wav').as_posix())
            (work/'render.pd').write_text(wrapper)
            pd = run(['C:/Program Files/Pd/bin/pd.exe','-nogui','-stderr','-noprefs','-noaudio','-nomidi','-batch',
                      '-r',str(rate),'-compatibility','0.55','-open',str(work/'render.pd')],work)
            assert np.array_equal(sf.read(work/'pd.wav')[0],sf.read(source/'pd.wav')[0]), 'Tracing changed the PD reference'
            duration = 16 if case == 'phone-effects' else 33
            klang = run([str(ROOT/'build/phone-timing/trace.exe'), str(rate), str(duration), str(work/'events.tsv')],work)
            pe = [(m[0],int(float(m[1]))*64,int(float(m[2]))) for m in re.findall(r'TRACE-(\w+): (\S+) (\S+)',pd['stderr'])]
            ke = [(a,int(b),int(c)) for a,b,c in (line.split() for line in klang['stdout'].splitlines()) if c.isdigit()]
            # The first of Tom's simultaneous follow-up lists is immediately overwritten.
            pe = [e for i,e in enumerate(pe) if not (e[0]=='programme' and i+1<len(pe) and pe[i+1][0]=='programme' and pe[i+1][1]==e[1])]
            comparisons = []
            for label in ('programme','reset','release','digit'):
                p = [e for e in pe if e[0]==label]
                k = [e for e in ke if e[0]==label]
                assert [e[2] for e in p] == [e[2] for e in k], (rate,case,label,'event sequence differs')
                # PD's digits-only decoder omits letter/star/hash keys.
                for a,b in zip(p,k):
                    comparisons.append(dict(event=label,pd_frame=a[1],klang_frame=b[1],pd_value=a[2],klang_value=b[2],
                                            offset_samples=b[1]-a[1],offset_ms=(b[1]-a[1])*1000/rate))
                print(rate,case,label,'counts',len(p),len(k),'offsets',sorted(set(e['offset_samples'] for e in comparisons if e['event']==label)))
            results.append(dict(rate=rate,case=case,pd_events=pe,klang_events=ke,comparisons=comparisons,pd_run=pd,klang_run=klang))
    paths = [ROOT/p for p in ['tests/pd/phone-timing.cpp','tools/check_phone_timing.py',
        'farnell/klang/Artificial Sounds/Phone Tones/phoneeffects.k',
        'farnell/klang/Artificial Sounds/Alarm Generator/alarmgenerator.k',
        'farnell/klang/Artificial Sounds/DTMF Tones/dtmftones.k','include/klang/pd.h']]
    report = dict(comparisons=results,
        source_sha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        notes=['PD 0.55.2, block 64; unchanged shared event recipes and mono audio; traced PD audio is sample-identical to the uninstrumented reference.',
               'PD timestamps are logical message times mapped down to the receiving DSP block, not wall-clock measurements.',
               'Klang timestamps are actual sample positions observed in public model state. Digit timestamps are when PhoneEffects consumes the pending decoded key.',
               'Tom sends two same-time follow-up programmes; comparison keeps the last, effective programme. Raw PD stderr retains both.',
               'The PD reference decoder emits digits only; letter/star/hash detections from Klang are excluded from this comparison.',
               'These offsets do not measure human audibility or prove that timing explains the entire waveform residual.',
               'The phone-match reference reconstructs missing list-library number matchers; original PD sources and models are unchanged.'])
    (ROOT/'farnell/audio/comparisons/phone-timing.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__': main()
