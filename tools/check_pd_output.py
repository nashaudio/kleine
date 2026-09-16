"""Check whether stock PD's 16-bit file writers add noise to digital silence."""
import hashlib
import json
from pathlib import Path
from render_farnell import ROOT, sf, np, run, wrapper, expose_output


def main():
    trials = []
    executables = {'0.55-2': Path('C:/Program Files/Pd/bin/pd.exe'),
                   '0.42-5': ROOT/'build/pedestrians-review/pd-0.42-5/pd/bin/pd.exe'}
    for version, exe in executables.items():
        work = ROOT/'build/metro-review'/f'output-{version}'
        work.mkdir(parents=True, exist_ok=True)
        (work/'model.pd').write_text('#N canvas 0 0 200 200 10;\n#X obj 20 20 sig~ 0;\n'
                                   '#X obj 20 60 outlet~;\n#X connect 0 0 1 0;\n')
        text = wrapper('silence', 48000, 48000, work/'soundfiler.wav')
        text = text.replace('array define captured', 'table captured').replace('-bytes 4', '-bytes 2')
        lines, count = expose_output(text)
        path = (work/'writesf.wav').as_posix()
        lines += [f'#X msg 600 20 open -bytes 2 {path} \\, start;',
                  '#X obj 600 60 writesf~;', '#X obj 600 100 del 950;',
                  '#X msg 600 140 stop;', f'#X connect 0 0 {count} 0;',
                  f'#X connect {count} 0 {count+1} 0;', f'#X connect 3 0 {count+1} 0;',
                  f'#X connect 0 0 {count+2} 0;', f'#X connect {count+2} 0 {count+3} 0;',
                  f'#X connect {count+3} 0 {count+1} 0;']
        (work/'render.pd').write_text('\n'.join(lines)+'\n')
        execution = run([str(exe), '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi',
                         '-r', '48000', '-open', str(work/'render.pd')], work)
        for writer in ['soundfiler', 'writesf']:
            path = work/f'{writer}.wav'
            y, rate = sf.read(path)
            assert rate == 48000 and len(y) > 40000 and sf.info(path).subtype == 'PCM_16'
            assert not np.any(y), f'{version} {writer} added nonzero samples'
            trials.append(dict(version=version, writer=writer, frames=len(y), rate=rate,
                               subtype='PCM_16', nonzero_samples=int(np.count_nonzero(y)),
                               sha256=hashlib.sha256(path.read_bytes()).hexdigest(), execution=execution))
            print(version, writer, 'exact digital silence', flush=True)
    sources = {}
    for version, directory in [('0.55-2', ROOT/'build/metro-review'),
                               ('0.42-5', ROOT/'build/pedestrians-review/pd-0.42-5/pd/src')]:
        for source in ['d_soundfile.c', 's_audio_pa.c']:
            sources[f'{version}/{source}'] = dict(
                url=f'https://github.com/pure-data/pure-data/blob/{version}/src/{source}',
                sha256=hashlib.sha256((directory/source).read_bytes()).hexdigest())
    report = dict(tests=trials, sources=sources,
                  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  findings=['Both built-in file writers preserve silence when writing PCM16; no dither option in inspected source.',
                            'Both inspected PortAudio paths open streams with paNoFlag, allowing backend conversion dither.',
                            'Hardware/backend recording was not exercised; this does not identify the website export path.'])
    (ROOT/'farnell/audio/comparisons/artificial/pedestrians-output.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
