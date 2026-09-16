"""Compare pd::del control messages with PD 0.55.2's del, without a block adapter."""
import hashlib
import json
import re
import shutil
from render_farnell import ROOT, run


def cases():
    # Event time in ms, method, value, optional unit. All event times lie on both sample grids.
    bang = (0, 'bang', 0, 'msec')
    result = {
        'default': ((200, 0, 'msec'), [bang]),
        'idle': ((200, 0, 'msec'), []),
        'hot': ((200, 0, 'msec'), [(0, 'hot', 37.123, 'msec')]),
        'cold': ((100, 0, 'msec'), [bang, (20, 'cold', 50, 'msec'), (200, 'bang', 0, 'msec')]),
        'retrigger': ((100, 0, 'msec'), [bang, (30, 'bang', 0, 'msec')]),
        'hot-retrigger': ((100, 0, 'msec'), [bang, (30, 'hot', 50, 'msec')]),
        'stop': ((100, 0, 'msec'), [bang, (30, 'stop', 0, 'msec')]),
        'stop-restart': ((100, 0, 'msec'), [bang, (30, 'stop', 0, 'msec'), (200, 'bang', 0, 'msec')]),
        'zero': ((0, 0, 'msec'), [bang]),
        'negative': ((-20, 0, 'msec'), [bang]),
        'negative-hot': ((100, 0, 'msec'), [(0, 'hot', -10, 'msec')]),
        'fractional': ((37.123, 0, 'msec'), [bang]),
        'tempo-pending': ((100, 0, 'msec'), [bang, (30, 'tempo', 2, 'msec')]),
        'tempo-to-samples': ((100, 0, 'msec'), [bang, (30, 'tempo', 1, 'samp')]),
        'tempo-from-samples': ((8000, 1, 'samp'), [bang, (30, 'tempo', 1, 'msec')]),
        'tempo-sample-scale': ((8000, 1, 'samp'), [bang, (30, 'tempo', 2, 'samp')]),
        'tempo-invalid': ((100, 0, 'msec'), [(0, 'tempo', 2, 'bogus'), bang]),
        'tempo-nonpositive': ((100, 0, 'msec'), [(0, 'tempo', 0, 'msec'), bang]),
    }
    for name, duration, amount, unit in [
        ('milliseconds',100,2,'millisecond'), ('seconds',.2,1,'sec'),
        ('minutes',.002,1,'min'), ('samples',1234,1,'samp'),
        ('per-ms',100,2,'permsec'), ('per-sec',1,4,'persec'),
        ('per-min',1,120,'permin'), ('per-sample',1234,2,'persamp'),
    ]:
        result[name] = ((duration,amount,unit),[bang])
    return result


def main():
    root = ROOT/'build/del-review'
    results = []
    for rate in (48000,44100):
        for name, (creation, events) in cases().items():
            work = root/str(rate)/name
            work.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT/'tests/pd/del.pd',work/'clock-test.pd')
            duration, amount, unit = creation
            lines = ['#N canvas 0 0 700 600 10;', '#X obj 20 20 loadbang;',
                     f'#X obj 20 300 clock-test {duration} {amount} {unit} {rate};',
                     '#X obj 400 20 del 1000;', r'#X msg 400 50 \; pd quit;',
                     '#X connect 0 0 2 0;', '#X connect 2 0 3 0;']
            count = 4
            # Equal-time messages share a trigger, keeping explicit source order.
            for milliseconds in sorted(set(e[0] for e in events)):
                group = [e for e in events if e[0] == milliseconds]
                lines += [f'#X obj 20 60 del {milliseconds};',
                          '#X obj 20 90 t '+' '.join('b' for _ in group)+';',
                          f'#X connect 0 0 {count} 0;', f'#X connect {count} 0 {count+1} 0;']
                trigger = count+1
                count += 2
                for index, (_,action,value,event_unit) in enumerate(group):
                    message = f'tempo {value} {event_unit}' if action == 'tempo' else str(value) if action in ('hot','cold') else action
                    lines += [f'#X msg 20 130 {message};',
                              f'#X connect {trigger} {len(group)-index-1} {count} 0;',
                              f'#X connect {count} 0 1 {1 if action == "cold" else 0};']
                    count += 1
            (work/'render.pd').write_text('\n'.join(lines)+'\n')
            event_file = work/'events.tsv'
            event_file.write_text(''.join(f'{round(ms*rate/1000)} {action} {value} {units}\n'
                                         for ms,action,value,units in events))
            pd = run(['C:/Program Files/Pd/bin/pd.exe','-nogui','-stderr','-noprefs','-noaudio',
                      '-nomidi','-batch','-r',str(rate),'-open',str(work/'render.pd')],work)
            klang = run([str(root/'del.exe'),str(rate),str(event_file),str(duration),str(amount),unit],work)
            p = [int(float(v)) for v in re.findall(r'DEL: (\S+)',pd['stderr'])]
            k = [int(v) for v in klang['stdout'].split()]
            errors = [s for s in pd['stderr'].splitlines() if s.startswith('error:') and
                      'unable to create registry' not in s and not (name == 'tempo-invalid' and 'unknown time unit' in s)]
            assert not errors, errors
            # timer publishes a float: a deadline just beyond an integral sample
            # can round onto it. The polling port still rounds its double upward.
            assert len(p) == len(k) and all(0 <= b-a <= 1 for a,b in zip(p,k)), (rate,name,p,k)
            results.append(dict(rate=rate,case=name,creation=creation,events=events,
                                pd_frames=p,klang_frames=k,timing_error_samples=[b-a for a,b in zip(p,k)],
                                passed=True,pd=pd,klang=klang))
            print(rate,name,p,flush=True)
    sources = ['include/klang/pd.h','tests/pd/del.cpp','tests/pd/del.pd','tools/check_pd_del.py']
    report = dict(tests=results,source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
                  executable_sha256=hashlib.sha256((root/'del.exe').read_bytes()).hexdigest(),
                  notes=['PD 0.55.2; control messages only, no audio gain, alignment or resampling.',
                         'PD timer deadlines are explicitly projected to the first sample boundary at/after the deadline.',
                         '0.001-sample tolerance in that projection handles PD float timer rounding at integral boundaries.',
                         'Up to one later Klang sample is allowed: a PD float timer cannot resolve double deadlines just beyond an integral sample.',
                         'PD pending sample-unit tempo changes retain their old deadline; this source behaviour is tested.',
                         'No shared scheduler, synchronous feedback or block delivery is claimed.'])
    (ROOT/'tests/pd/del-results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__': main()
