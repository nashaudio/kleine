"""Compare render levels, 10 ms envelopes, spectra and 16384-point sonograms."""
import argparse
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
os.environ.setdefault('MPLCONFIGDIR', str(ROOT/'build/trials/matplotlib'))
sys.path.insert(0, str(ROOT / 'build/trials/python'))
import numpy as np
import soundfile as sf
from scipy import signal
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def db(value): return 20*np.log10(np.maximum(value, 1e-12))


def describe(samples, rate):
    return dict(sample_rate=rate, frames=len(samples), seconds=len(samples)/rate,
                peak=float(np.max(np.abs(samples))), rms=float(np.sqrt(np.mean(samples**2))),
                dc=float(np.mean(samples)), nonfinite=int(np.sum(~np.isfinite(samples))),
                samples_over_full_scale=int(np.sum(np.abs(samples)>1)))


def compare(paths, output, fft=16384):
    if len(paths) < 2: raise ValueError('Supply at least two audio files')
    audio, rates = zip(*(sf.read(path, always_2d=True) for path in paths))
    assert len(set(rates)) == 1, 'Resample explicitly before comparison'
    rate = rates[0]
    # Avoid antiphase cancellation when summarising stereo levels. Trial bounces
    # are mono; sonograms use the left channel of original stereo excerpts.
    mono = [a[:, 0] for a in audio]
    if fft < 1024 or any(len(x) < fft for x in mono):
        raise ValueError('FFT must be at least 1024 and fit within every recording')
    if any(not np.isfinite(x).all() or not np.any(x) for x in mono):
        raise ValueError('Audio must be finite and non-silent')
    a, b = mono[:2]
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    corr = signal.correlate(a, b, method='fft')
    radius = min(n-1, rate//100)
    lag = int(np.argmax(corr[n-1-radius:n+radius]) - radius)
    aa, bb = (a[lag:], b[:n-lag]) if lag >= 0 else (a[:n+lag], b[-lag:])
    error = aa-bb
    window = rate//100
    envelopes = [np.sqrt(np.mean(np.mean(x[:len(x)//window*window].reshape(-1, window, x.shape[1])**2, axis=2), axis=1)) for x in audio]
    peaks = [np.max(np.abs(x[:len(x)//window*window].reshape(-1, window, x.shape[1])), axis=(1, 2)) for x in audio]
    envn = min(len(envelopes[0]), len(envelopes[1]))
    powers = [signal.welch(x, rate, nperseg=fft)[1] for x in [a, b]]
    mask = powers[0] > max(powers[0]) * 1e-5
    metrics = {
        'files': {str(p): describe(x, rate) for p, x in zip(paths, audio)},
        'fft': fft, 'hop': 1024, 'window': 'hann', 'envelope_ms': 10,
        'alignment_lag_samples_pd_minus_klang': lag,
        'aligned_correlation': float(np.dot(aa,bb)/np.sqrt(np.dot(aa,aa)*np.dot(bb,bb))),
        'aligned_error_rms': float(np.sqrt(np.mean(error**2))),
        'identical_samples': bool(np.array_equal(a, b)),
        'relative_error_db': float(db(np.linalg.norm(error)/np.linalg.norm(aa))),
        'rms_difference_db': float(db(np.linalg.norm(b)/np.linalg.norm(a))),
        'envelope_relative_error': float(np.linalg.norm(envelopes[0][:envn]-envelopes[1][:envn])/np.linalg.norm(envelopes[0][:envn])),
        'active_spectrum_median_abs_db': float(np.median(np.abs(10*np.log10(np.maximum(powers[1][mask],1e-30)/powers[0][mask])))),
    }
    rows = 2+len(audio)
    figure, axes = plt.subplots(rows, 1, figsize=(12, 2.3*rows), constrained_layout=True)
    for index, (path, x, envelope, peak) in enumerate(zip(paths, mono, envelopes, peaks)):
        label = Path(path).stem
        times = (np.arange(len(envelope))+.5)*window/rate
        style = dict(linestyle='--' if index == 1 else '-', linewidth=1.1 if index == 1 else 1.8)
        line, = axes[0].plot(times, db(envelope), label=label, alpha=.8, **style)
        axes[0].plot(times, db(peak), color=line.get_color(), linestyle=':', alpha=.5)
        f, p = signal.welch(x, rate, nperseg=fft)
        axes[1].plot(f, 10*np.log10(np.maximum(p, 1e-20)), label=label, alpha=.8, **style)
    axes[0].set(ylabel='10 ms level (dBFS)', xlabel='Time (s)', ylim=(-100, 0), title='RMS (solid), peak (dotted)'); axes[0].legend(loc='upper right')
    axes[1].set(xlabel='Frequency (Hz)', ylabel='PSD (dBFS/Hz)', xlim=(30,rate/2), ylim=(-120,0), xscale='log'); axes[1].legend()
    for ax, path, x in zip(axes[2:], paths, mono):
        f,t,z = signal.stft(x, rate, window='hann', nperseg=fft, noverlap=fft-1024, boundary=None, padded=False)
        # Single-sided amplitude calibration: a bin-centred unit sine is 0 dBFS.
        spectrum = db(2*np.abs(z))
        plot = ax.pcolormesh(t,f,spectrum,shading='auto',vmin=-100,vmax=0,cmap='magma',rasterized=True)
        ax.set(title=Path(path).name, ylabel='Frequency (Hz)', xlabel='Time (s)',
               xlim=(0, max(len(a) for a in audio)/rate), ylim=(30,rate/2), yscale='log')
    figure.colorbar(plot, ax=list(axes[2:]), label='Spectral amplitude (dBFS)')
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=140)
    plt.close(figure)
    output.with_suffix('.json').write_text(json.dumps(metrics, indent=2))
    return metrics


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('audio', nargs='+', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fft', type=int, default=16384)
    args = parser.parse_args()
    print(json.dumps(compare(args.audio, args.output, args.fft), indent=2))
