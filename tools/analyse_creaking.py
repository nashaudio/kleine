"""Estimate the website creak's contact timing using the measured wood/panel response.

This is an analysis of an existing recording, not a claim to recover its original
slider messages or random seed. Audio is never modified by this script.
"""
import hashlib
import json

from compare_audio import ROOT, np, sf
from scipy import signal, fft
import matplotlib.pyplot as plt

WORK = ROOT/'build/idiophonics'
DEST = ROOT/'farnell/audio/comparisons/idiophonics'


def estimate(samples, rate, response):
    # Regularised inversion suppresses amplification at the panel's spectral nulls.
    n = fft.next_fast_len(len(samples)+len(response))
    h = fft.rfft(response,n)
    excitation = fft.irfft(fft.rfft(samples,n)*h.conj()/(abs(h)**2+1e-5),n)[:len(samples)]
    peaks,_ = signal.find_peaks(excitation,height=.12,distance=int(rate*.0025),prominence=.09)
    times = peaks/rate
    intervals = np.diff(times)
    valid = (intervals > .002) & (intervals < .049)
    # Source metro: interval_ms = 3 + (1-force)*(60 + 6*random/1000).
    # E[random/1000] ~ .5; single-contact estimates retain its random uncertainty.
    force = 1-(intervals[valid]*1000-3)/63
    return dict(times=times, force_times=times[:-1][valid], force=force, excitation=excitation,
                intervals_ms=intervals[valid]*1000)


def first_contact(a,b,rate):
    # The initial pulse follows >100 ms silence, so both use the same pulse length.
    # Compare 35 ms of its material response before a second slip can enter.
    start = lambda x: np.flatnonzero(abs(x)>.01)[0]
    ia,ib = start(a),start(b)
    size = round(.035*rate)
    reference = a[ia:ia+size]
    # Fit a fractional-sample offset and gain, explicitly reported below.
    offsets=np.arange(-3,3.001,.02)
    best=None
    for offset in offsets:
        candidate=np.interp(ib+np.arange(size)+offset,np.arange(len(b)),b)
        gain=float(np.dot(reference,candidate)/np.dot(candidate,candidate))
        error=float(np.linalg.norm(reference-candidate*gain)/np.linalg.norm(reference))
        if best is None or error<best['relative_error']:
            best=dict(relative_error=error,residual_db=20*np.log10(error),gain=gain,
                      fractional_alignment_samples=float(offset),online_start_sample=int(ia),
                      render_start_sample=int(ib),duration=.035)
    return best


def main():
    DEST.mkdir(parents=True,exist_ok=True)
    paths = {'Online':ROOT/'farnell/zip/p09/creaking.wav',
             'Held-force test':WORK/'48000/creaking-controls/pd.wav',
             'Revised movement':WORK/'48000/creaking/kleine.wav'}
    wood_path=WORK/'48000/creaking-wood/pd.wav'
    panel_path=WORK/'48000/creaking-panel/pd.wav'
    wood,rate=sf.read(wood_path);panel,pr=sf.read(panel_path)
    assert rate==pr==48000
    response=signal.fftconvolve(wood,panel)*.2
    sounds={}; estimates={}
    for name,path in paths.items():
        samples,sr=sf.read(path,always_2d=True);assert sr==rate
        sounds[name]=samples.mean(axis=1)
        estimates[name]=estimate(sounds[name],rate,response)

    # Independent 100 ms windows describe the performance, not individual noise phases.
    grid=np.arange(.05,6.8,.1)
    curves={}; envelopes={}
    for name,e in estimates.items():
        curves[name]=np.array([np.median(part) if len(part:=e['force'][(e['force_times']>=t-.05)&
                      (e['force_times']<t+.05)]) else np.nan for t in grid])
        samples=sounds[name]
        envelopes[name]=np.array([np.sqrt(np.mean(samples[round((t-.05)*rate):round((t+.05)*rate)]**2)) for t in grid])
    results={}
    for name in ['Held-force test','Revised movement']:
        active=np.isfinite(curves['Online']) & np.isfinite(curves[name])
        union=np.isfinite(curves['Online']) | np.isfinite(curves[name])
        results[name]=dict(force_mae_where_both_active=float(np.mean(abs(curves['Online'][active]-curves[name][active]))),
            activity_mismatch_fraction=float(np.mean(np.isfinite(curves['Online'])[union]!=np.isfinite(curves[name])[union])),
            envelope_relative_error_100ms=float(np.linalg.norm(envelopes[name]-envelopes['Online'])/np.linalg.norm(envelopes['Online'])),
            first_contact=first_contact(sounds['Online'],sounds[name],rate))
    report=dict(sample_rate=rate,channel_mix='arithmetic mean; no gain applied',
        regularisation=1e-5,peak_height=.12,peak_prominence=.09,min_peak_distance_ms=2.5,
        valid_interval_ms=[2,49],force_estimate='1 - (interval_ms - 3)/63',
        bin_duration_ms=100,results=results,
        sources={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in [*paths.values(),wood_path,panel_path]},
        notes=['Control estimates assume the supplied stick/slip model and average its interval jitter.',
               'The revised movement was guided by the online recording; this is a fitted demonstration, not an independent model validation.',
               'Whole-file sample residuals against the website are not meaningful with unknown seed and messages.',
               'The first-contact check explicitly fits gain and sub-sample alignment; other metrics use raw level and original time.'])
    (DEST/'creaking-performance.json').write_text(json.dumps(report,indent=2)+'\n')
    fig,axes=plt.subplots(3,1,figsize=(14,10))
    colours={'Online':'#286ca7','Held-force test':'#a1662e','Revised movement':'#2a8b52'}
    for name,e in estimates.items():
        axes[0].plot(e['force_times'],e['force'],'.',ms=2,label=name,color=colours[name],alpha=.7)
        axes[1].plot(grid,20*np.log10(np.maximum(envelopes[name],1e-6)),label=name,color=colours[name])
    a=sounds['Online'];b=sounds['Revised movement'];match=results['Revised movement']['first_contact']
    count=round(match['duration']*rate);t=np.arange(count)/rate
    axes[2].plot(t*1000,a[match['online_start_sample']:match['online_start_sample']+count],label='Online',color=colours['Online'])
    candidate=np.interp(match['render_start_sample']+np.arange(count)+match['fractional_alignment_samples'],np.arange(len(b)),b)*match['gain']
    axes[2].plot(t*1000,candidate,label='Revised movement (aligned, fitted gain)',color=colours['Revised movement'],alpha=.7)
    for ax in axes[:2]:ax.set_xlim(0,7);ax.set_xlabel('Time (seconds)')
    axes[0].set_ylabel('Estimated force');axes[0].set_ylim(0,1.06)
    axes[1].set_ylabel('RMS, 100 ms (dBFS)');axes[1].set_ylim(-90,-15)
    axes[2].set_ylabel('First contact amplitude');axes[2].set_xlabel('Milliseconds from first contact')
    for ax in axes:ax.grid(alpha=.25);ax.legend(loc='best')
    fig.suptitle('Creaking: website movement versus held force and revised performance')
    fig.tight_layout();fig.savefig(DEST/'creaking-performance.png',dpi=140);plt.close(fig)
    print(json.dumps(results,indent=2))


if __name__=='__main__':main()
