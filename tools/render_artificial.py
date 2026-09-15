"""Render and compare the Artificial Sounds series using explicit shared event recipes.

Original patches stay untouched. Instrumented copies, scripts and scratch audio
live in build/artificial; --retain keeps requested WAVs and comparison evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

from render_farnell import ROOT, records, expose_output, wrapper, run, np, sf
from check_pd_primitives import metrics

PRESETS = [
    [380, 2, 349, 0, 0, 0, 1], [238, 1, 317, 0, 0, 476, 0],
    [317, 7, 300, 125, 0, 0, 1], [1031, 9, 360, 238, 174, 158, 1],
    [900, 4, 2000, 2010, 2000, 2010, 1], [1428, 3, 619, 571, 365, 206, 1],
    [450, 1, 365, 571, 619, 206, .5], [714, 74, 1000, 0, 1000, 0, 1],
    [200, 30, 1000, 476, 159, 0, 1], [634, 61, 1000, 476, 159, 0, 1],
]
KEYS = '123A456B789C*0#D'
ROWS, COLUMNS = [697, 770, 852, 941], [1209, 1336, 1477, 1633]
VARIANTS = {
    'dtmf-bulk': ('dtmf', 'pd/ALARMS/dtmf.pd'),
    'dtmf-study': ('dtmf', 'pd/ALARMS/dtmf00.pd'),
    'alarm15': ('alarm06', 'pd/ALARMS/2tone-15.pd'),
    'alarm-bank': ('alarm-bank', 'pd/ALARMS/2tone-20.pd'),
    'ringback-bulk': ('ringback-bulk', 'pd/ALARMS/dial-remotering.pd'),
    'police00': ('police-graph-legacy', 'pd/POLICE/police00.pd'),
    'police01': ('police-graph-legacy-inverted', 'pd/POLICE/police01.pd'),
    'police1': ('police-graph-compare-legacy', 'pd/POLICE/police1.pd'),
    'police3': ('police-legacy', 'pd/POLICE/police3.pd'),
    'police-osc-bulk': ('police-logosc-legacy', 'pd/POLICE/logosc~.pd'),
    'police-compare-log': ('police-graph-compare', 'zip/p05/triangle_logosc_compare.pd'),
    'dtmf-detector': ('dtmf-detector', 'pd/ALARMS/decode-tone.pd'),
    'dtmf-decoder': ('dtmf-decoder', 'pd/ALARMS/dtmfdec.pd'),
    'phone-effects': ('phone-effects', 'pd/ALARMS/phone-effects.pd'),
    'call-recogniser': ('call-recogniser', 'pd/ALARMS/dt-call-recogniser.pd'),
    'phone-match': ('phone-effects', 'pd/ALARMS/phone-effects.pd'),
    'call-match': ('call-recogniser', 'pd/ALARMS/dt-call-recogniser.pd'),
    'pedestrians-controls': ('pedestrians', 'zip/p01/pedestrian-beep.pd'),
    'alarm01-controls': ('alarm01', 'zip/p04/alarm01.pd'),
    'alarm05-controls': ('alarm05', 'zip/p04/alarm05.pd'),
    'dtmf-retrigger': ('dtmf', 'zip/p03/dtmf.pd'),
}
PRIMARY_CASES = ['pedestrians','dtmf'] + [f'alarm0{i}' for i in range(1,8)] + ['police','police-legacy']
COMPONENT_CASES = ['police-environment','police-horn','police-logosc','police-logosc-legacy',
                   'police-graph','police-graph-legacy','police-triangle']
ALL_CASES = PRIMARY_CASES + COMPONENT_CASES + list(VARIANTS)


def reference_errors(case, stderr):
    """Reject PD errors except the host registry warning and a known display quirk."""
    allowed = ['error: unable to create registry entry: loading']
    if case == 'dtmf-decoder':
        # Its display feeds letter/star/hash messages to a redundant float branch
        # of [t f a]. The separately captured eight detector states are unaffected.
        allowed.append("error: trigger: generic messages can only be converted to 'b' or 'a'")
    return [line for line in stderr.splitlines() if line.startswith('error:') and line not in allowed]


def extract(text, name):
    """Extract an existing embedded canvas without changing its DSP graph."""
    lines = records(text)
    start = next(i for i, row in enumerate(lines) if row.startswith('#N canvas') and f' {name} ' in row)
    depth = 0
    for stop in range(start, len(lines)):
        if lines[stop].startswith('#N canvas'): depth += 1
        elif lines[stop].startswith('#X restore'):
            depth -= 1
            if depth == 0: return '\n'.join(lines[start:stop]) + '\n'
    raise ValueError('Unclosed canvas ' + name)


def replace_canvas(text, name, transform):
    lines = records(text); result = []; i = 0
    while i < len(lines):
        row = lines[i]
        if row.startswith('#N canvas') and f' {name} ' in row:
            depth, stop = 1, i+1
            while depth:
                if lines[stop].startswith('#N canvas'): depth += 1
                elif lines[stop].startswith('#X restore'): depth -= 1
                stop += 1
            result += records(transform('\n'.join(lines[i:stop-1])+'\n'))
            result.append(lines[stop-1]); i = stop
        else: result.append(row); i += 1
    return '\n'.join(result)+'\n'


def inlet(lines, count, target, port, symbol):
    lines += [f'#X obj 700 20 r audit-{symbol};', f'#X connect {count} 0 {target} {port};']
    return count + 1


def disable_saved_controls(text):
    result = []
    for row in records(text):
        if re.match(r'#X obj \S+ \S+ (hsl|vsl|nbx) ', row):
            fields = row.split(); fields[10] = '0'; row = ' '.join(fields)
        result.append(row)
    return '\n'.join(result) + '\n'


def swap_pow(text):
    """Restore the pre-0.41 exponent/base convention on modern PD, explicitly."""
    stack, result = [], []
    for row in records(text):
        if row.startswith('#N canvas'): stack.append([0, set()])
        elif row.startswith('#X restore'):
            stack.pop(); stack[-1][0] += 1
        elif row.startswith('#X connect'):
            fields = row.rstrip(';').split()
            if int(fields[4]) in stack[-1][1]: fields[5] = str(1 - int(fields[5]))
            row = ' '.join(fields) + ';'
        elif row.startswith(('#X obj ', '#X msg ', '#X floatatom ', '#X symbolatom ', '#X text ')):
            if re.match(r'#X obj \S+ \S+ pow~', row): stack[-1][1].add(stack[-1][0])
            stack[-1][0] += 1
        result.append(row)
    return '\n'.join(result) + '\n'


def recipe(case):
    """Seconds, action, values. Converted once to a common PD block grid."""
    if case == 'pedestrians': return 4, []
    if case.endswith('-controls'): return 4, [(0,'start',[0]),(.25,'start',[1]),(.9,'start',[0]),(1.5,'start',[1]),(2.2,'start',[0]),(2.6,'start',[1])]
    if case == 'dtmf-retrigger':return 4,[(.25,'key',[0]),(.35,'key',[1]),(.4,'key',[1]),(1,'key',[15]),(1.02,'key',[12]),(2,'key',[13])]
    if case in ('phone-match','call-match'):
        numbers=['0100200300','0987654321','1234567890'] if case=='phone-match' else ['07814832212','07787123456','07890123456']
        events=[]
        for i,number in enumerate(numbers):
            events += [(.25+i*9+j*.3,'key',[KEYS.index(digit)]) for j,digit in enumerate(number)]
        # A wrong number after the inactivity reset must not retrigger a ringer.
        events += [(27.25+j*.3,'key',[KEYS.index(digit)]) for j,digit in enumerate('11111111111')]
        return 33,events
    if case in ('dtmf-detector','dtmf-decoder'): return recipe('dtmf')
    if case in ('phone-effects','call-recogniser'):
        # Exercise manual ringers and the keypad/line first; matching has its own fixture.
        events = [(.25,'ringer',[0]), (2,'ringer',[1]), (4,'ringer',[2])]
        events += [(6+i*.3,'key',[KEYS.index(key)]) for i,key in enumerate('123A456B789C*0#D')]
        if case=='phone-effects': events += [(5.8,'source',[0]),(11,'source',[2]),(13,'source',[1])]
        return 16, sorted(events,key=lambda e:e[0])
    if case in ('dtmf','dtmf-bulk'):
        events = [(0.25 + i * .3, 'key', [i]) for i in range(16)]
        if case == 'dtmf-bulk': events.insert(0, (0,'dtmf',[200,.3,1]))
        return 6, events
    if case == 'dtmf-study':
        return 4, [(0,'dtmf',[200,.125,0]),(.25,'tones',[300,400]),(.25,'gate',[1]),(.8,'gate',[0]),
                   (1,'tones',[200,250]),(1,'gate',[1]),(1.7,'tones',[500,250]),(2.5,'gate',[0])]
    if case == 'alarm15': return recipe('alarm06')
    if case == 'alarm-bank':
        return 6, [(.25,'bank',[.126984,.190476,.730159,.0793651,0,.460317,.507937,.0634921]),
                   (2,'bank',[.174603,.396825,.619048,.587302,.428571,.460317,.492063,.396825]),
                   (4,'bank',[.793651,.793651,.793651,0,.0634921,.460317,.269841,.0634921])]
    if case == 'ringback-bulk': return 12, []
    if case.startswith('alarm'):
        if case == 'alarm06':
            return 4, [(0.25, 'program', [471, 2, 555.55556, 507.9365, 619.0476, 619.0476, 3400/6300]),
                       (1.5, 'program', [800, 3, 723, 932, 1012, 440, .5])]
        if case == 'alarm07':
            events, start = [], .25
            for p in PRESETS:
                events.append((start, 'program', p)); start += p[0]/1000 + .4
            return int(np.ceil(start + 1)), events
        return 4, []
    if case in ('police', 'police-legacy','police3'):
        return 16, [(0,'rate',[.1]),(10, 'rate', [3])]
    return 2, []


def patch(case, work):
    web = ROOT/'farnell/zip'
    dependencies = []
    if case in VARIANTS: source = ROOT/'farnell'/VARIANTS[case][1]
    elif case == 'pedestrians': source = web/'p01/pedestrian-beep.pd'
    elif case == 'dtmf': source = web/'p03/dtmf.pd'
    elif case.startswith('alarm'): source = web/f'p04/{case}.pd'
    elif case in ('police', 'police-legacy'): source = web/'p05/police_siren.pd'
    elif case == 'police-environment': source = web/'p05/environment.pd'
    elif case == 'police-horn': source = web/'p05/plastichorn.pd'
    elif 'graph' in case: source = web/'p05/logosc_graph.pd'
    elif 'triangle' in case: source = web/'p05/triangle_logosc_compare.pd'
    else: source = web/'p05/logosc.pd'
    case={'phone-match':'phone-effects','call-match':'call-recogniser','dtmf-retrigger':'dtmf'}.get(case,case.removesuffix('-controls'))
    text = disable_saved_controls(source.read_text())
    if case == 'alarm07': text = extract(text, 'multi-tone-alarm')
    if case in ('phone-effects','call-recogniser'):
        if case=='phone-effects': text=extract(text,'phone-effects')
        text=replace_canvas(text,'multi-tone-alarm',lambda t:'\n'.join(expose_output(t)[0])+'\n')
        replacement=ROOT/'tests/pd/number-match.pd'; dependencies.append(replacement)
        reconstructed=replacement.read_text().replace('== \\$1;',f'== {10 if case=="phone-effects" else 11};')
        text=replace_canvas(text,'matchnumber',lambda _:reconstructed)
        for name in ['decode-tone.pd','sand.pd','telephone-line.pd']:
            original=ROOT/'farnell/pd/ALARMS'/name
            shutil.copyfile(original,work/name);dependencies.append(original)
    if case=='dtmf-detector':
        text=text.replace('bp~ \\$1 24;','bp~ 697 24;').replace('outlet;','sig~;')
    if case=='dtmf-decoder': text=text.replace('r~ call;','inlet~;')
    if case.endswith('-legacy'): text = swap_pow(text)
    lines, count = expose_output(text)
    if case == 'pedestrians' or case == 'alarm01' or case == 'alarm05':
        target = {'pedestrians': 6, 'alarm01': 8, 'alarm05': 20}[case]
        count = inlet(lines, count, target, 0, 'start')
    elif case in ('alarm06','alarm15'):
        # Unpack right-to-left, so the timebase starts after its parameters arrive.
        unpack = count; lines += ['#X obj 700 50 unpack f f f f f f f;']; count += 1
        count = inlet(lines, count, unpack, 0, 'program')
        voices = [19,18,20,21] if case == 'alarm15' else [16,15,17,18]
        routes = [(0,5,0),(1,5,1)] + [(2+i,target,0) for i,target in enumerate(voices)] + [(6,target,1) for target in voices]
        for outlet, target, port in routes:
            lines.append(f'#X connect {unpack} {outlet} {target} {port};')
    elif case == 'alarm07': count = inlet(lines, count, 9, 0, 'program')
    elif case in ('phone-effects','call-recogniser'):
        if case=='phone-effects': lines += ['#X connect 37 0 43 0;']
        else:
            lines += ['#X obj 700 20 outlet~;',f'#X connect 10 0 {count} 0;'];count+=1
        # Keypad audio for the standalone call recogniser is supplied as an inlet.
        if case=='call-recogniser':
            original=ROOT/'farnell/zip/p03/dtmf.pd'
            shutil.copyfile(original,work/'dialler-original.pd'); dependencies.append(original)
            dialler='\n'.join(expose_output(original.read_text())[0])+'\n'
            (work/'dialler.pd').write_text(dialler)
            lines += ['#X obj 700 20 dialler;','#X obj 700 50 s~ call;',f'#X connect {count} 0 {count+1} 0;'];count+=2
        ringer_targets=[39,40,41] if case=='phone-effects' else [12,13,14]
        lines += ['#X obj 700 20 r audit-ringer;','#X obj 700 50 sel 0 1 2;',f'#X connect {count} 0 {count+1} 0;']
        for i,target in enumerate(ringer_targets):lines.append(f'#X connect {count+1} {i} {target} 0;')
        count+=2
        if case=='phone-effects':
            lines+=['#X obj 700 20 r audit-source;','#X obj 700 50 sel 0 1 2;',
                    '#X obj 700 80 s silence;','#X obj 700 110 s dialtone;','#X obj 700 140 s ring;',
                    f'#X connect {count} 0 {count+1} 0;']
            for i in range(3):lines.append(f'#X connect {count+1} {i} {count+2+i} 0;')
            count+=5
            # The outer GUI buttons relay recognised-number bangs to each ringer.
            for name in ['tom','dick','harry']:
                lines += [f'#X obj 700 20 r {name}r;',f'#X obj 700 50 s {name};',f'#X connect {count} 0 {count+1} 0;'];count+=2
    elif case in ('dtmf-detector','dtmf-decoder'):
        original=ROOT/'farnell/pd/ALARMS/decode-tone.pd';shutil.copyfile(original,work/original.name);dependencies.append(original)
        if case=='dtmf-detector':
            lines += ['#X obj 700 20 outlet~;',f'#X connect 3 0 {count} 0;']
        else:
            out=count;lines += ['#X obj 700 20 outlet~;'];count+=1
            for i,node in enumerate([0,54,55,56,57,58,59,60]):
                lines += ['#X obj 700 40 sig~;',f'#X obj 700 60 *~ {1<<i};',
                          f'#X connect {node} 0 {count} 0;',f'#X connect {count} 0 {count+1} 0;',f'#X connect {count+1} 0 {out} 0;'];count+=2
    elif case == 'alarm-bank': count = inlet(lines,count,8,0,'bank')
    elif case == 'dtmf-study':
        count = inlet(lines,count,4,0,'tones')
        lines += ['#X obj 700 50 pack f 1;']; packed=count;count+=1
        count = inlet(lines,count,packed,0,'gate');lines.append(f'#X connect {packed} 0 5 0;')
    elif case == 'ringback-bulk':
        original = ROOT/'farnell/pd/ALARMS/telephone-line.pd'
        shutil.copyfile(original,work/original.name);dependencies.append(original)
    elif case in ('police', 'police-legacy','police3'):
        count = inlet(lines, count, 8 if case=='police3' else 6, 0, 'rate')
        names = ['logosc~.pd','plastichorn~.pd','environment.pd'] if case=='police3' else ['logosc.pd','plastichorn.pd','environment.pd']
        for name in names:
            original = ROOT/'farnell/pd/POLICE'/name if case=='police3' else web/'p05'/name
            content = original.read_text()
            if case.endswith('-legacy'): content = swap_pow(content)
            (work/name).write_text(content)
            dependencies.append(original)
    elif 'graph' in case or 'triangle' in case or case in ('police00','police01','police1','police-compare-log'):
        # Capture the waveform that the original sends to its display array.
        target = 19 if 'triangle' in case else 20 if case=='police01' else 25 if case in ('police1','police-compare-log') else 16
        lines += ['#X obj 700 20 outlet~;', f'#X connect {target} 0 {count} 0;']
    elif case.startswith('police-logosc') or case == 'police-osc-bulk':
        target = 17 if case == 'police-osc-bulk' else 16
        lines += ['#X obj 700 20 sig~ 700;', f'#X connect {count} 0 {target} 0;']
    elif case == 'police-horn':
        original = web/'p05/logosc.pd'; shutil.copyfile(original, work/'logosc.pd'); dependencies.append(original)
        lines += ['#X obj 700 20 sig~ 700;', '#X obj 700 50 logosc;',
                  f'#X connect {count} 0 {count+1} 0;', f'#X connect {count+1} 0 1 0;']
    (work/'model.pd').write_text('\n'.join(lines)+'\n')
    return [source] + dependencies


def pd_message(action, values):
    if action == 'key':
        key = int(values[0]); return f'dialme {COLUMNS[key % 4]} {ROWS[key // 4]}'
    return 'audit-' + action + ' ' + ' '.join(map(str, values))


def render_case(case, rate, args):
    work = ROOT/'build/artificial'/str(rate)/case
    work.mkdir(parents=True, exist_ok=True)
    sources = patch(case, work)
    duration, recipe_events = recipe(case)
    frames = round(duration * rate)
    events = [(int(round(t*rate))//64*64, action, values) for t,action,values in recipe_events]
    pd_audio, klang_audio = work/'pd.wav', work/'kleine.wav'
    lines, count = expose_output(wrapper('artificial', frames, rate, pd_audio))
    if case in ['pedestrians', 'alarm01', 'alarm05'] or case.endswith('-controls'):
        lines += ['#X msg 700 20 \\; audit-start 1;', f'#X connect 0 0 {count} 0;']; count += 1
    for frame, action, values in events:
        if action in ('dtmf','gain'): continue # Klang configuration; source already has these constants.
        if case in ('dtmf-detector','dtmf-decoder'): continue
        # A quarter-sample inside the block avoids float millisecond rounding
        # dispatching an exact-boundary message in the preceding PD block.
        lines += [f'#X obj 700 20 del {(frame + .25) * 1000 / rate:.12f};',
                  f'#X msg 700 50 \\; {pd_message(action, values)};',
                  f'#X connect 0 0 {count} 0;', f'#X connect {count} 0 {count+1} 0;']
        count += 2
    if case == 'police-environment':
        lines += ['#X obj 700 20 array define impulse 1;', '#X msg 700 50 \\; impulse 0 1;',
                  '#X obj 700 80 tabplay~ impulse;', '#X obj 700 110 t b b;',
                  f'#X connect 0 0 {count+3} 0;', f'#X connect {count+3} 1 {count+1} 0;',
                  f'#X connect {count+3} 0 {count+2} 0;', f'#X connect {count+2} 0 3 0;']
    if case in ('dtmf-detector','dtmf-decoder'):
        # Use the already verified dialler waveform to isolate detector behaviour.
        fixture=ROOT/f'build/artificial/{rate}/dtmf/pd.wav'
        if not fixture.exists():raise ValueError('Render dtmf at this rate before its detector fixtures')
        lines += ['#X obj 700 20 array define input;',f'#X msg 700 50 read -resize {fixture.as_posix()} input;',
                  '#X obj 700 80 soundfiler;','#X obj 700 110 tabplay~ input;','#X obj 700 140 t b b;',
                  f'#X connect 0 0 {count+4} 0;',f'#X connect {count+4} 1 {count+1} 0;',f'#X connect {count+1} 0 {count+2} 0;',
                  f'#X connect {count+4} 0 {count+3} 0;',f'#X connect {count+3} 0 3 0;']
    (work/'render.pd').write_text('\n'.join(lines)+'\n')
    script = work/'events.tsv'
    script.write_text(''.join(f'{frame}\t{action}\t' + '\t'.join(map(str,values)) + '\n' for frame,action,values in events))
    p = run([args.pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch', '-r', str(rate),
             '-compatibility', '0.55', '-open', str(work/'render.pd')], work)
    if errors := reference_errors(case, p['stderr']):
        raise RuntimeError(f'{case}: unexpected PD errors: {errors}')
    model = VARIANTS[case][0] if case in VARIANTS else case
    k = run([str(args.kleine.resolve()), '--render', model, str(klang_audio), str(duration), str(rate), '1', str(script)], work)
    a, ar = sf.read(pd_audio); b, br = sf.read(klang_audio)
    assert ar == br == rate and len(a) == len(b) == frames
    assert np.isfinite(a).all() and np.isfinite(b).all()
    assert np.max(np.abs(a)) > 1e-8 and np.max(np.abs(b)) > 1e-8
    m = metrics(a, b)
    result = dict(sample_rate=rate, duration=duration, frames=frames, block_size=64,
                  sources={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
                  events=[dict(frame=f, action=n, values=v) for f,n,v in events],
                  pd=p, kleine=k, comparison=m,
                  pd_clipped_samples=int(np.count_nonzero(np.abs(a) >= 1)),
                  kleine_clipped_samples=int(np.count_nonzero(np.abs(b) >= 1)))
    (work/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'{case}: residual {m["relative_error_db"]:.2f} dB; level {m["level_delta_db"]:.5f} dB; peaks {m["pd_peak"]:.3f}/{m["klang_peak"]:.3f}', flush=True)
    if abs(m['level_delta_db']) >= .002 or m['relative_error_db'] >= -70:
        raise RuntimeError(f'{case} failed the deterministic fixture limits; see {work / "result.json"}')
    if args.retain and case in PRIMARY_CASES + ['dtmf-bulk','dtmf-study','alarm15','alarm-bank','ringback-bulk','phone-effects','call-recogniser']:
        chapter = '24-pedestrians' if case.startswith('pedestrians') else '26-dtmf-tones' if case.startswith('dtmf') or case=='call-recogniser' else '27-alarms' if case.startswith('alarm') else '25-phone-tones' if case in ('ringback-bulk','phone-effects') else '28-police'
        dest = ROOT/'farnell/audio'/chapter; dest.mkdir(parents=True, exist_ok=True)
        for source, renderer in [(pd_audio,'pd'), (klang_audio,'kleine')]: shutil.copyfile(source, dest/f'{case}-{renderer}.wav')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pd', default='C:/Program Files/Pd/bin/pd.exe')
    parser.add_argument('--kleine', type=Path, default=ROOT/'build/x64-release/kleine.exe')
    parser.add_argument('--rate', type=int, default=48000)
    parser.add_argument('--cases', nargs='+', choices=ALL_CASES, default=ALL_CASES)
    parser.add_argument('--retain', action='store_true')
    args = parser.parse_args()
    results = {case: render_case(case,args.rate,args) for case in args.cases}
    dest = ROOT/'build/artificial'/str(args.rate)/'results.json'
    existing = json.loads(dest.read_text()) if dest.exists() else {}
    existing.update(results); dest.write_text(json.dumps(existing,indent=2)+'\n')


if __name__ == '__main__': main()
