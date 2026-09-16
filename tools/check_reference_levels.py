"""Measure the website pulse gain separately from PD-version differences."""
import json
import argparse
import hashlib
from pathlib import Path

from check_pd_primitives import ROOT, np, sf, metrics
from render_farnell import model_patch, wrapper, run, expose_output
from scipy import signal


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pd', default='C:/Program Files/Pd/bin/pd.exe')
    parser.add_argument('--kleine', type=Path, default=ROOT/'build/x64-release/kleine.exe')
    args = parser.parse_args()
    pd = args.pd
    version_renders = {}
    for version in ['0.55', '0.43']:
        work = ROOT/'build/levels'/version
        work.mkdir(parents=True, exist_ok=True)
        model_patch('pulse', work)
        output = work/'pulse.wav'
        (work/'render.pd').write_text(wrapper('pulse', 192000, 48000, output))
        run([pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch',
             '-r', '48000', '-compatibility', version,
             '-path', str(ROOT/'farnell/pd/PHONETONES'), '-open', str(work/'render.pd')], work)
        version_renders[version], _ = sf.read(output)
    website, rate = sf.read(ROOT/'farnell/audio/25-phone-tones/pulse-online.wav', always_2d=True)
    assert rate == 48000, 'This comparison does not resample the website recording'
    work = ROOT/'build/levels/current'
    work.mkdir(parents=True, exist_ok=True)
    klang_run = run([str(args.kleine.resolve()), '--render', 'pulse', str(work/'kleine.wav'), '4', '48000', '1'], work)
    klang, _ = sf.read(work/'kleine.wav')
    # The website embeds the handset instead of loading telephone-line.pd.
    source = ROOT/'farnell/zip/p02/pulsedial.pd'
    lines, count = expose_output(source.read_text())
    lines += ['#X obj 600 20 r audit-digit;', f'#X connect {count} 0 6 0;']
    (work/'model.pd').write_text('\n'.join(lines)+'\n')
    (work/'render.pd').write_text(wrapper('pulse', 192000, 48000, work/'website-patch.wav'))
    website_run = run([pd, '-nogui', '-stderr', '-noprefs', '-noaudio', '-nomidi', '-batch',
                       '-r', '48000', '-compatibility', '0.55', '-open', str(work/'render.pd')], work)
    website_patch, _ = sf.read(work/'website-patch.wav')
    # The first scripted digit is one isolated 40 ms contact in a 100 ms period.
    template = version_renders['0.55'][11968:11968+4800]
    candidates = signal.correlate(website[:,0], template, mode='valid', method='fft')
    peaks, _ = signal.find_peaks(candidates, distance=4800)
    peaks = [int(p) for p in peaks if candidates[p] > .9*np.max(candidates)]
    fits = []
    for offset in peaks:
        observed = website[offset:offset+len(template),0]
        gain = float(np.dot(template, observed)/np.dot(template, template))
        residual = float(20*np.log10(np.linalg.norm(observed-gain*template)/np.linalg.norm(observed)))
        fits.append(dict(source_start_seconds=7.5+offset/rate, frames=len(template),
                         fitted_gain=gain, fitted_gain_db=float(20*np.log10(gain)), residual_db=residual))
    result = dict(sample_rate=rate, template_start_frame=11968, template_frames=4800,
                  method='Integer-sample cross-correlation then least-squares scalar gain; original left channel, no resampling.',
                  local_pd_versus_klang=metrics(version_renders['0.55'], klang),
                  collection_versus_website_patch=metrics(version_renders['0.55'], website_patch),
                  current_runs=dict(kleine=klang_run, website_patch=website_run),
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [
                      source, ROOT/'farnell/pd/PHONETONES/pulsedial.pd', ROOT/'farnell/pd/PHONETONES/telephone-line.pd',
                      ROOT/'farnell/audio/25-phone-tones/pulse-online.wav', ROOT/'tools/check_reference_levels.py']},
                  executable_sha256=hashlib.sha256(args.kleine.read_bytes()).hexdigest(),
                  pd_current_versus_legacy=metrics(version_renders['0.55'], version_renders['0.43']),
                  website_pulse_fits=fits,
                  median_fitted_gain=float(np.median([f['fitted_gain'] for f in fits])),
                  notes=['Legacy means Pd 0.55.2 with compatibility 0.43, not an old executable.',
                         'In the legacy comparison, klang_* fields denote the legacy PD output.',
                         'A fitted gain is evidence of a level difference, not proof of post-processing.',
                         'The pulse patch has no oscillator; its DSP is clip/bp/hip filtering.'])
    destination = ROOT/'farnell/audio/comparisons/reference-levels.json'
    destination.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__': main()
