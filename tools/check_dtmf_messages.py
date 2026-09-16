"""Compare actual DTMF decoder messages, including sand, with shared audio inputs."""
import hashlib
import json
import re
import shutil

from render_farnell import ROOT, records, expose_output, run, np, sf
from render_artificial import extract

FREQUENCIES = [697,770,852,941,1209,1336,1477,1633]
KEYS = '123A456B789C*0#D'


def cases():
    result = {
        'hold-release':[(.2,[0,4]),(1.2,[])],
        'repeat':[(.2,[0,4]),(.55,[]),(.8,[0,4]),(1.2,[])],
        'release-row':[(.2,[0,4]),(.7,[4]),(1.2,[])],
        'release-column':[(.2,[0,4]),(.7,[0]),(1.2,[])],
        'switch-column':[(.2,[0,4]),(.7,[0,5]),(1.2,[])],
        'switch-row':[(.2,[0,4]),(.7,[1,4]),(1.2,[])],
        'overlap-columns':[(.2,[0,4]),(.7,[0,4,5]),(1,[0,5]),(1.3,[])],
        'overlap-rows':[(.2,[0,4]),(.7,[0,1,4]),(1,[1,4]),(1.3,[])],
        'overlap-both':[(.2,[0,4]),(.7,[0,1,4,5]),(1,[1,5]),(1.3,[])],
        'row-only':[(.2,[0]),(1.2,[])],
        'column-only':[(.2,[4]),(1.2,[])],
        'silence':[],
    }
    for i,key in enumerate(KEYS): result['key-'+str(i)] = [(.2,[i//4,4+i%4]),(1.2,[])]
    return result


def trace_patch(rate):
    # Keep a block index in PD print's six significant digits, avoiding frame rounding.
    return '\n'.join(['#N canvas 0 0 500 400 10;', '#X obj 20 20 inlet;',
        '#X obj 20 50 t a b;', '#X obj 180 80 timer;', '#X obj 280 20 loadbang;',
        f'#X obj 180 110 expr int($f1 * {rate} / 64000 + 0.5);',
        '#X obj 20 140 list prepend;', r'#X obj 20 180 print TRACE-\$1;',
        '#X connect 0 0 1 0;', '#X connect 1 1 2 1;', '#X connect 3 0 2 0;',
        '#X connect 2 0 4 0;', '#X connect 4 0 5 1;', '#X connect 1 0 5 0;',
        '#X connect 5 0 6 0;'])+'\n'


def instrument(text, taps):
    lines, count = expose_output(text)
    for node,outlet,label,change in taps:
        if change:
            lines += ['#X obj 700 10 change;',f'#X connect {node} {outlet} {count} 0;']
            node,outlet=count,0
            count+=1
        lines += [f'#X obj 700 40 dtmf-trace {label};', f'#X connect {node} {outlet} {count} 0;']
        count+=1
    return '\n'.join(lines)+'\n'


def parse(text):
    return [dict(event=label,frame=int(block)*64,value={'star':'*','hash':'#'}.get(value,value))
            for label,block,value in re.findall(r'TRACE-([\w-]+): (\d+) (\S+)',text)]


def main():
    root=ROOT/'build/dtmf-messages'
    source=ROOT/'farnell/pd/ALARMS'
    results=[]
    for rate in (48000,44100):
        for name,events in cases().items():
            work=root/str(rate)/name
            work.mkdir(parents=True,exist_ok=True)
            for file in ('sand.pd','decode-tone.pd'): shutil.copyfile(source/file,work/file)
            bank=extract((source/'phone-effects.pd').read_text(),'decoder')
            bank=instrument(bank,[(10,0,'bank-state',False),(11,1,'bank-key',False)]+
                            [(node,0,f'detector{i}',True) for i,node in enumerate([0,2,3,4,5,6,7])])
            (work/'bank.pd').write_text(bank)
            standalone=(source/'dtmfdec.pd').read_text().replace('r~ call;','inlet~;')
            standalone=instrument(standalone,[(51,1,'standalone',False),(60,0,'detector7',True)])
            (work/'standalone.pd').write_text(standalone)
            (work/'dtmf-trace.pd').write_text(trace_patch(rate))
            frames=rate*2
            time=np.arange(frames)/rate
            audio=np.zeros(frames,dtype=np.float64)
            recipe=[]
            for index,(seconds,tones) in enumerate(events):
                start=round(seconds*rate)//64*64
                end=round(events[index+1][0]*rate)//64*64 if index+1<len(events) else frames
                for tone in tones: audio[start:end]+=.25*np.cos(2*np.pi*FREQUENCIES[tone]*time[start:end])
                recipe.append(dict(frame=start,frequencies=[FREQUENCIES[t] for t in tones]))
            audio=audio.astype('<f4'); audio.tofile(work/'input.f32')
            sf.write(work/'input.wav',audio,rate,subtype='FLOAT')
            patch=['#N canvas 0 0 600 500 10;','#X obj 20 20 loadbang;','#X obj 20 50 t b b b;',
                   f'#X msg 200 80 read -resize {(work/"input.wav").as_posix()} input;',
                   '#X obj 200 110 soundfiler;',r'#X msg 100 80 \; pd dsp 1;',
                   '#X obj 20 110 tabplay~ input;','#X obj 20 160 bank;','#X obj 200 160 standalone;',
                   '#X obj 400 20 array define input;','#X obj 400 80 del 2010;',r'#X msg 400 110 \; pd quit;',
                   '#X connect 0 0 1 0;','#X connect 1 2 2 0;','#X connect 2 0 3 0;',
                   '#X connect 1 1 4 0;','#X connect 1 0 5 0;','#X connect 5 0 6 0;',
                   '#X connect 5 0 7 0;','#X connect 0 0 9 0;','#X connect 9 0 10 0;']
            (work/'render.pd').write_text('\n'.join(patch)+'\n')
            pd=run(['C:/Program Files/Pd/bin/pd.exe','-nogui','-stderr','-noprefs','-noaudio','-nomidi','-batch',
                    '-r',str(rate),'-compatibility','0.55','-open',str(work/'render.pd')],work)
            errors=[s for s in pd['stderr'].splitlines() if s.startswith('error:') and
                    not any(v in s for v in ('unable to create registry entry','trigger: generic messages can only be converted'))]
            assert not errors, errors
            kr=run([str(root/'decoder.exe'),str(rate),str(work/'input.f32')],work)
            pe=parse(pd['stderr'])
            ke=[dict(event=a,frame=int(b),value=c) for a,b,c in (line.split() for line in kr['stdout'].splitlines())]
            pd_keys=[e['value'] for e in pe if e['event']=='bank-key']
            klang_keys=[e['value'] for e in ke if e['event']=='bank-key']
            standalone_keys=[e['value'] for e in pe if e['event']=='standalone']
            detectors={}
            for i in range(8):
                p=[(e['frame'],e['value']) for e in pe if e['event']==f'detector{i}']
                k=[(e['frame'],e['value']) for e in ke if e['event']==f'detector{i}']
                detectors[str(i)]=dict(identical=p==k,pd=p,klang=k)
                assert p==k, (rate,name,i,'Detector state or publication time differs')
            if name.startswith('key-'):
                expected=KEYS[int(name[4:])]
                assert set(standalone_keys)=={expected}, (rate,name,standalone_keys)
            for label in ('bank-key','standalone'):
                p=[(e['frame'],e['value']) for e in pe if e['event']==label]
                k=[(e['frame'],e['value']) for e in ke if e['event']==label]
                assert p==k, (rate,name,label,p[:20],k[:20])
            if name in ('silence','row-only','column-only'):
                assert not pd_keys and not klang_keys and not standalone_keys
            entry=dict(rate=rate,case=name,recipe=recipe,pd_events=pe,klang_events=ke,
                bank_digits=pd_keys,klang_digits=klang_keys,bank_digit_sequence_equal=pd_keys==klang_keys,
                standalone_messages=standalone_keys,detectors=detectors,pd_run=pd,klang_run=kr,
                input_sha256=hashlib.sha256(audio.tobytes()).hexdigest())
            results.append(entry)
            print(rate,name,'bank',pd_keys,'Klang',klang_keys,'standalone messages',len(standalone_keys),flush=True)
    work=root/'sand'
    work.mkdir(exist_ok=True)
    sand=run(['C:/Program Files/Pd/bin/pd.exe','-nogui','-stderr','-noprefs','-noaudio','-nomidi','-batch',
              '-path',str(source),'-open',str(ROOT/'tests/pd/sand-messages.pd')],work)
    sand_values=[int(v) for v in re.findall(r'SAND: (-?\d+)',sand['stderr'])]
    assert sand_values==[-1,1,-1,1,-1,1,-1,1], sand_values
    sand_klang=run([str(root/'decoder.exe')],work)
    assert [int(v) for v in sand_klang['stdout'].split()]==sand_values
    paths=[source/p for p in ('phone-effects.pd','dtmfdec.pd','decode-tone.pd','sand.pd')]
    paths += [ROOT/p for p in ('tests/pd/dtmf-messages.cpp','tests/pd/sand-messages.pd','tools/check_dtmf_messages.py',
               'farnell/klang/Artificial Sounds/DTMF Tones/dtmftones.k','include/klang/pd.h')]
    report=dict(results=results,sand=dict(values=sand_values,passed=True,run=sand,klang_run=sand_klang),
        summary=dict(audio_cases=len(results),all_eight_detector_states_identical=True,
                     single_key_cases_passed=32,normal_transition_cases_passed=8,invalid_input_cases_passed=6,
                     direct_switch_cases_passed=4,overlap_cases_passed=6,
                     digit_bank_messages_identical=True,standalone_messages_identical=True),
        executable_sha256=hashlib.sha256((root/'decoder.exe').read_bytes()).hexdigest(),
        source_sha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        notes=['PD 0.55.2, block64; shared mono float32 input, 0.25 amplitude per cosine, continuous phase, no clipping or gain fitting.',
               'Compare the digits-only phone-effects decoder containing sand separately from standalone dtmfdec.pd, which has different topology.',
               'Klang control publication is logged at the actual frame, with no timestamp offset; env publishes out/updated together at the following-block boundary.',
               'The standalone PD display t f a emits known symbol-to-float errors; messages are tapped at its unconverted any outlet.',
               'Both actual Klang decoder graphs and Sand are tested; original PD sources are unchanged. Generated wrappers/audio are under build/dtmf-messages.'])
    (ROOT/'tests/pd/dtmf-message-results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__': main()
