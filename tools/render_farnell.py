"""Render the chapter 25/29 trial cases through Pd and Kleine.

Requires numpy, soundfile and psutil. Originals are never edited: temporary Pd
copies expose the left DAC signal as an outlet and receive scripted GUI events.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT / 'build/trials/python'))
import numpy as np
import psutil
import soundfile as sf


def records(text):
    return [s.strip() + ';' for s in re.split(r'(?<!\\);\s*\n', text.strip() + '\n') if s.strip()]


def expose_output(text):
    """Keep all DSP, replacing root DAC objects with mono outlets."""
    result, depth, index, dacs = [], -1, 0, set()
    for record in records(text):
        if record.startswith('#N canvas'): depth += 1
        if depth == 0:
            if re.match(r'#X obj \S+ \S+ dac~', record):
                dacs.add(index)
                record = re.sub(r'dac~[^;]*;', 'outlet~;', record)
            if record.startswith('#X connect'):
                _, _, source, outlet, target, inlet = record.rstrip(';').split()
                if int(target) in dacs and int(inlet) != 0: continue
            elif record.startswith(('#X obj ', '#X msg ', '#X floatatom ', '#X symbolatom ', '#X text ', '#X restore ')):
                index += 1
        if record.startswith('#X restore'):
            depth -= 1
            if depth == 0: index += 1
        result.append(record)
    return result, index


def casing_patch():
    all_records = records((ROOT / 'farnell/pd/BELL/striker.pd').read_text())
    start = next(i for i, s in enumerate(all_records) if s.startswith('#N canvas') and ' casing ' in s)
    stop = next(i for i in range(start+1, len(all_records)) if all_records[i].startswith('#X restore'))
    text = '\n'.join(all_records[start:stop])
    text = text.replace(r'r~ \$0-casein;', 'inlet~;').replace(r's~ \$0-caseout;', 'outlet~;')
    return text + '\n'


def model_patch(case, work):
    pd = ROOT / 'farnell/pd'
    web = ROOT / 'farnell/reference'
    source = {
        'dial': pd/'PHONETONES/dialtone1.pd',
        'dial-web': web/'p02/dialtone1.pd',
        'dial-line': pd/'PHONETONES/dialtone2.pd',
        'busy': web/'p02/busy-signal.pd',
        'busy-archive': pd/'PHONETONES/busy-signal.pd',
        'ringback': pd/'PHONETONES/ringingtone.pd',
        'pulse': pd/'PHONETONES/pulsedial.pd',
        'bell': pd/'BELL/striker.pd',
        'bell-single': pd/'BELL/striker.pd',
        'bell-dry': pd/'BELL/striker.pd',
        'bell-single-dry': pd/'BELL/striker.pd',
        'bell-casing': pd/'BELL/striker.pd',
    }[case]
    text = source.read_text()
    if case == 'bell-casing':
        output = casing_patch()
    else:
        lines, count = expose_output(text)
        if case == 'dial-line':
            lines += [f'#X obj 600 20 loadbang;', f'#X connect {count} 0 4 0;']
        elif case == 'ringback':
            lines += ['#X obj 600 20 loadbang;', '#X msg 600 50 1;',
                      f'#X connect {count} 0 {count+1} 0;', f'#X connect {count+1} 0 14 1;']
        elif case == 'pulse':
            lines += ['#X obj 600 20 r audit-digit;', f'#X connect {count} 0 7 0;']
        elif case in ('bell', 'bell-dry'):
            lines += ['#X obj 600 20 r audit-ring;', f'#X connect {count} 0 19 0;']
        elif case in ('bell-single', 'bell-single-dry'):
            lines += ['#X obj 600 20 r audit-strike;', f'#X connect {count} 0 20 0;']
        output = '\n'.join(lines) + '\n'
        if case.endswith('-dry'): output = output.replace('#X connect 4 0 0 0;', '#X connect 2 0 0 0;')
    (work / 'model.pd').write_text(output)
    return source


def wrapper(case, frames, rate, destination):
    lines = ['#N canvas 0 0 900 650 10;']
    objects, connections = [], []
    def obj(kind, value):
        index = len(objects)
        objects.append(f'#X {kind} 20 {20+index*20} {value};')
        return index
    def connect(a, b, outlet=0, inlet=0): connections.append(f'#X connect {a} {outlet} {b} {inlet};')
    load = obj('obj', 'loadbang')
    trigger = obj('obj', 't b b b')
    dsp = obj('msg', r'\; pd dsp 1')
    model = obj('obj', 'model')
    capture = obj('obj', 'tabwrite~ captured')
    obj('obj', f'array define captured {frames}')
    finish = obj('obj', f'del {(frames+64)*1000/rate:.9f}')
    end = obj('obj', 't b b')
    pd_path = destination.as_posix().replace(' ', r'\ ').replace(',', r'\,').replace(';', r'\;')
    save = obj('msg', f'write -wave -bytes 4 {pd_path} captured')
    writer = obj('obj', 'soundfiler')
    quit_pd = obj('msg', r'\; pd quit')
    connect(load, trigger); connect(trigger, dsp, 2); connect(trigger, capture, 1); connect(trigger, finish)
    connect(model, capture); connect(finish, end); connect(end, save, 1); connect(save, writer); connect(end, quit_pd)
    events = []
    if case == 'pulse': events = [(250, 'audit-digit 1'), (750, 'audit-digit 5'), (1750, 'audit-digit 7')]
    if case in ('bell', 'bell-dry'): events = [(500, 'audit-ring 1'), (2500, 'audit-ring 0'), (4000, 'audit-ring 1'), (6000, 'audit-ring 0')]
    if case in ('bell-single', 'bell-single-dry'): events = [(500, 'audit-strike bang')]
    for ms, message in events:
        delay = obj('obj', f'del {ms}')
        event = obj('msg', r'\; ' + message)
        connect(load, delay); connect(delay, event)
    if case == 'bell-casing':
        obj('obj', 'array define impulse 1')
        initialise = obj('msg', r'\; impulse 0 1')
        play = obj('obj', 'tabplay~ impulse')
        delay = obj('obj', 'del 500')
        connect(load, initialise); connect(load, delay); connect(delay, play); connect(play, model)
    return '\n'.join(lines+objects+connections) + '\n'


def run(command, cwd):
    # Redirect to files so a child with verbose output cannot block on a pipe.
    start = time.perf_counter()
    stdout_path, stderr_path = cwd/'stdout.log', cwd/'stderr.log'
    peak, cpu = 0, 0
    with stdout_path.open('w') as stdout, stderr_path.open('w') as stderr:
        process = subprocess.Popen(command, cwd=cwd, stdout=stdout, stderr=stderr)
        observed = psutil.Process(process.pid)
        while process.poll() is None:
            if time.perf_counter() - start > 60:
                process.kill(); process.wait(); raise RuntimeError('render exceeded 60 seconds')
            try:
                peak = max(peak, observed.memory_info().rss)
                times = observed.cpu_times()
                cpu = max(cpu, times.user + times.system)
            except psutil.NoSuchProcess: pass
            time.sleep(0.005)
    stdout, stderr = stdout_path.read_text(), stderr_path.read_text()
    if process.returncode: raise RuntimeError(f'{command}\n{stdout}\n{stderr}')
    if any(message in stderr for message in ['couldn\'t create', 'connection failed', 'no such object', 'no such array']):
        raise RuntimeError(stderr)
    return dict(command=command, elapsed_seconds=time.perf_counter()-start,
                sampled_cpu_seconds=cpu, sampled_peak_rss_bytes=peak, stdout=stdout, stderr=stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kleine', type=Path, default=ROOT/'build/x64-release/kleine.exe')
    parser.add_argument('--pd', default='C:/Program Files/Pd/bin/pd.exe')
    parser.add_argument('--output', type=Path, default=ROOT/'farnell/audio')
    parser.add_argument('--rate', type=int, default=48000)
    parser.add_argument('--compatibility', default='0.55', help='Pd compatibility level (not a different executable)')
    parser.add_argument('--results', type=Path, default=ROOT/'build/trials/render-results.json')
    parser.add_argument('--cases', nargs='+', default=['dial', 'dial-web', 'dial-line', 'busy', 'busy-archive', 'ringback', 'pulse', 'bell'])
    args = parser.parse_args()
    results = {}
    for case in args.cases:
        duration = 12 if case == 'ringback' else 10 if case in ('bell', 'bell-dry') else 4
        frames = duration * args.rate
        destination = args.output.resolve() / ('29-telephone-bell' if case.startswith('bell') else '25-phone-tones')
        destination.mkdir(parents=True, exist_ok=True)
        work = ROOT / 'build/trials' / case
        work.mkdir(parents=True, exist_ok=True)
        source = model_patch(case, work)
        pd_output = destination / f'{case}-pd.wav'
        klang_output = destination / f'{case}-kleine.wav'
        (work/'render.pd').write_text(wrapper(case, frames, args.rate, pd_output))
        pd_command = [args.pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch', '-r', str(args.rate),
                      '-compatibility', args.compatibility,
                      '-path', str(ROOT/'farnell/pd/BELL'), '-path', str(ROOT/'farnell/pd/PHONETONES'), '-open', str(work/'render.pd')]
        pd_result = run(pd_command, work)
        klang_result = run([str(args.kleine.resolve()), '--render', case, str(klang_output), str(duration), str(args.rate), '1'], work)
        for output in [pd_output, klang_output]:
            data, rate = sf.read(output, always_2d=True)
            assert len(data) == frames and rate == args.rate and data.shape[1] == 1, output
            assert np.isfinite(data).all() and np.max(np.abs(data)) > 1e-6, output
        results[case] = dict(source=str(source.relative_to(ROOT)), source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                             duration=duration, sample_rate=args.rate, pd_compatibility=args.compatibility, pd=pd_result, kleine=klang_result)
        print(case, 'rendered', flush=True)
    args.results.parent.mkdir(parents=True, exist_ok=True)
    args.results.write_text(json.dumps(results, indent=2))


if __name__ == '__main__': main()
