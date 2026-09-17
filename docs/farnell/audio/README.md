# Designing Sound listening bundles

The **[Idiophonics trial](idiophonics.md)** completes chapters 29-33: the remaining bell studies, Bouncing, Rolling, Creaking and Boing. Start there for their models, paired audio, website recordings and 48/44.1 kHz evidence.

The completed **[Artificial Sounds extended trial](artificial-sounds.md)** covers chapters 24–28, including the earlier chapter 25 work. Start there for the new Klang files, website excerpts, paired bounces, comparison plots and reference findings. The original chapter 25/29 trial and its retained evidence are described below.

First trial 14 September 2026; refactored and revalidated 15 September. These WAVs are deliberately retained deliverables. Scratch renders and diagnostic outputs stay under `build/`; full companion audio is now available in the tracked local site mirror, `farnell/zip/`.

| Chapter | Klang code | Website excerpt(s) | PD bounce | Kleine bounce |
| --- | --- | --- | --- | --- |
| 25 — Phone tones | [phonetones.k](../../../farnell/klang/Artificial%20Sounds/Phone%20Tones/phonetones.k) | [Online montage](../../../farnell/audio/25-phone-tones/phone-tones-online.wav) | [PD montage](../../../farnell/audio/25-phone-tones/phone-tones-pd.wav) | [Kleine montage](../../../farnell/audio/25-phone-tones/phone-tones-kleine.wav) |
| 29 — Telephone bell | [telephonebell.k](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) | [Online excerpt](../../../farnell/audio/29-telephone-bell/bell-online.wav) | [PD](../../../farnell/audio/29-telephone-bell/bell-pd.wav) | [Kleine](../../../farnell/audio/29-telephone-bell/bell-kleine.wav) |

The phone montage contains dial tone (website's 350/450 Hz), pulse dialling, ringback, then busy tone, separated by 0.5 s silence. The PD/Kleine versions are 25.5 s; the online montage is 30 s because the recorded performances have different timings. Bell bounces are 10 s, with ringing at 0.5–2.5 s and 4–6 s and a full final tail; the online excerpt is its first 4.8 s.

All files are **48 kHz and unnormalised**. PD and Kleine bounces are mono float32 WAV at unity output gain. Online excerpts preserve the original stereo PCM16 samples exactly, with no resampling, fades or level changes. Online and scripted event timings differ; these are listening references, not synchronised online null tests. The online bell's measured RMS during 0.6–2.4 s is 5.85 dB above the PD render. Account for that when comparing by ear.

## Individual phone tones

| Case | Online | PD | Kleine |
| --- | --- | --- | --- |
| Dial, book/archive 350/440 Hz | — | [WAV](../../../farnell/audio/25-phone-tones/dial-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/dial-kleine.wav) |
| Dial, website 350/450 Hz | [WAV](../../../farnell/audio/25-phone-tones/dial-web-online.wav) | [WAV](../../../farnell/audio/25-phone-tones/dial-web-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/dial-web-kleine.wav) |
| Dial through telephone line | — | [WAV](../../../farnell/audio/25-phone-tones/dial-line-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/dial-line-kleine.wav) |
| Busy, smoothed | [WAV](../../../farnell/audio/25-phone-tones/busy-online.wav) | [WAV](../../../farnell/audio/25-phone-tones/busy-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/busy-kleine.wav) |
| Busy, old archive | — | [WAV](../../../farnell/audio/25-phone-tones/busy-archive-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/busy-archive-kleine.wav) |
| Ringback | [WAV](../../../farnell/audio/25-phone-tones/ringback-online.wav) | [WAV](../../../farnell/audio/25-phone-tones/ringback-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/ringback-kleine.wav) |
| Pulse digits 1, 5, 7 | [WAV](../../../farnell/audio/25-phone-tones/pulse-online.wav) | [WAV](../../../farnell/audio/25-phone-tones/pulse-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/pulse-kleine.wav) |

## Findings

The eight deterministic PD/Kleine comparisons have zero detected sample lag. After switching to `pd::osc`, both plain dial tones and the unsmoothed busy tone have **identical decoded samples**. Remaining relative residuals are below **−86.51 dB**, with the full bell at **−104.03 dB**; level differences are below 0.0001 dB. This establishes the measured fixture agreement, not every possible setting or a listening verdict.

The **online pulse is +5.902 dB louder than both PD and Kleine**. The tested older highpass behaviour contributes only +0.103 dB, and the pulse model uses no oscillator. The [level investigation](comparisons/reference-levels.md) records the evidence and a separate finding: tiny DC bias in the old oscillator table can cause FM phase drift even though overall oscillator level is virtually unchanged. The original recording gain/capture path remains unresolved.

The [measurement tables](comparisons/tables.md) give the individual results. Residual dB is `20 log10(RMS(PD − Kleine) / RMS(PD))`, without gain fitting; more negative is closer. JSON uses −240 dB as a floor for zero residual, with `identical_samples` identifying exact matches. Median spectral difference uses Welch power spectra over bins above −50 dB relative to the PD spectral peak. Envelopes use 10 ms bins. Sonograms use a Hann window, FFT 16384, hop 1024, identical frequency/time axes and −100…0 dB amplitude scales. Short-window plots use FFT 2048, also with hop 1024. PD is solid blue and Kleine dashed orange so coincident curves remain visible; the original orange curve covered blue where they agreed.

View the [bell comparison](../../../farnell/audio/comparisons/bell.png), [phone dial](../../../farnell/audio/comparisons/dial-web.png), [busy](../../../farnell/audio/comparisons/busy.png), [ringback](../../../farnell/audio/comparisons/ringback.png), [pulse](../../../farnell/audio/comparisons/pulse.png), and transient views for [bell](../../../farnell/audio/comparisons/bell-2048.png) / [pulse](../../../farnell/audio/comparisons/pulse-2048.png). Each plot includes original website levels where available. Matching bell partial peaks near 650, 1679, 1960, 7151, 8118 and 8156 Hz support the connection to the online example; its different recording level/timing alone does not establish post-processing.

Source discrepancies are retained explicitly:

- The book/supplied first dial tone uses 350/440 Hz; the updated website patch and recorded example use 350/450 Hz. Both variants are available.
- The updated busy patch smooths its gate with `lop~ 100`; the old archive does not. Both are available.
- The ringback patch uses three seconds off/three on, despite the book's table specifying four off/two on. This port follows the patch.
- The website's bell patch is `BELL/striker.pd`, not the separate `A4-bell-telephone.pd`. The casing's PD block delays materially affect its sound; recreating them resolved the initial mismatch. Dry bell and isolated casing diagnostics agreed before the complete model was checked.

## Validation and performance

Windows x64 Debug and Release builds passed with CMake 4.4.3 and MSVC 19.51.36257.0. The retained audio uses Release, PD-Vanilla 0.55.2, 48 kHz, PD block size 64, and one model instance on an Intel Core i7-10700 @ 2.90 GHz. Every retained audio file was checked for length/rate, finite samples, non-silence and full-scale overs. Default bounces do not clip. Bell decay reaches silence before the 10 s file ends.

[CPU/memory results](comparisons/tables.md) are exploratory. Process CPU and RSS are sampled every 5 ms and are lower bounds; zero CPU samples mean below the available resolution. They include program startup, loading, allocations and file I/O. An [empty PD patch exiting on load](../../../farnell/audio/comparisons/pd-startup.json) itself consumed 0.625 s sampled CPU and 1.060 s elapsed: startup/loading dominate these short jobs, so their totals do **not** establish relative DSP speed. Kleine's separately measured `Processor::process` wall cost is about 0.5–2.0 ms per audio second for the phone tones and 7.9 ms for the bell in the latest run. This includes block preparation, but excludes construction and WAV writing. These are single-run observations, not optimisation targets.

The [raw render log/measurements](../../../farnell/audio/comparisons/render-results.json), per-case JSON beside each plot, and [manifest](../../../farnell/audio/manifest.json) retain source hashes, platform details, parameters, audio properties/hashes and excerpt/montage timings. See [model documentation](../klang/README.md) for controls, complete event recipes, seed and repeatable commands.

## Online sources and attribution

The source recordings are Andy Farnell's official MIT Press companion examples:

| Recording | Retained source intervals, seconds (end exclusive) | Original SHA-256 |
| --- | --- | --- |
| [phonetones.wav](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/p02/phonetones.wav) | Dial 1–5; pulse 7.5–14.5; ringback 14.5–25; busy 26–33 | `61b0fc766cc1a6453ef1fa9cce070d890712659a94b25b64cbbb06071795d810` |
| [telephonebell.wav](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/p06/telephonebell.wav) | 0–4.8 | `9a6226e2f849cd5e5b95d96243eee230bc5572ffe88fd95f90afe230a37c540f` |

Fetched 14 September 2026 from the [phone-tone](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical02.html) and [telephone-bell](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical06.html) practical pages. Farnell patch/audio redistribution terms remain TBD; retaining these requested comparison fixtures does not grant a new licence. See [project licensing](../../../README.md#licensing-and-attribution) and [updated patch provenance](../reference/README.md). The book and its extracts remain local and excluded from Git.
