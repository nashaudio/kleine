# Designing Sound references

Reference models from Andy Farnell, *Designing Sound* (MIT Press, 2010). Original supplied patches and supporting files are in [pd/](pd/); Klang ports are developed in [klang/](klang/). Chris Nash's existing `ToyBoatEngine` and `FourStrokeEngine` ports are in [sounds/Motors.h](../sounds/Motors.h).

The first completed trial ports are **chapter 25, phone tones**, and **chapter 29, telephone bell**. Start with their [audio bundle and findings](audio/README.md), [model controls and reproduction instructions](klang/README.md), and [updated patch provenance](reference/README.md).

The [full coverage work queue](COVERAGE.md) maps chapters, practicals and patches to book figures/pages, companion audio, dependencies and Klang status. See also [PD primitive usage/port status](PD-PRIMITIVES.md) and the [Klang review backlog](KLANG-REVIEW.md). Regenerate the first two with `python tools/catalogue_farnell.py` after editing the [curated mappings/status](catalogue.json). The parked `pd/old` archive is retained as a recovery resource and excluded from active counts.

## Source and version investigation

Checked on 14 September 2026:

- [MIT companion index](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/index.html), linked from the [book's publisher page](https://mitpress.mit.edu/9780262288835/designing-sound/).
- [Official examples.tar.gz](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/examples.tar.gz): 57,441,106 bytes; SHA-256 `cc0ed3ea85ec5e3d54971fda2ff88af447b4e1301ba59985c15455fda2af067a`.
- All **505 regular files** in the archive's `BOOK-PUREDATA/` directory match the supplied `pd/` files byte for byte, including all 434 `.pd` patches. This identifies the supplied collection with the site's older bulk download.
- The site describes the tarball as intended for Vanilla Pd below 0.42, directs readers to version information in a README, and separately labels its individual chapter/practical downloads suitable for 0.42 and above.
- The tarball contains 557 entries including directories, but **no README, README.md, version file, or licence file**. Neither nested archive (`REVERB.tar.gz`, `phonetones.tar.gz`) contains the advertised notes. No explicit authoring-version metadata was found in the supplied patch comments. Do not infer a precise PD release from canvas geometry or filesystem timestamps.

On 15 September Chris supplied a **2015-distribution copy** in [pd/old/](pd/old/), containing the missing [AAA-README.FIRST](pd/old/AAA-README.FIRST). Its version note says some examples target **PD < 0.41**, with reversed `pow~` inlets corresponding to Cyclone's version. This narrows the compatibility investigation, especially for nonlinear models, but does not identify one authoring version for all patches. Its filesystem date is 27 April 2009; that is not proof of a PD release. README SHA-256: `871d7a9b427110db1e293a8cb7ed56794081186683d20a626c3c274924669b34`.

The old collection contains 470 files (450 patches). A [content-based audit](reference/old-redundancy.md), including renamed files, finds 423 exact duplicates in `pd/`: 418 at the same path and five elsewhere. Another 16 same-path files differ (two only in saved window state), and 31 have no matching path or contents. `PHONETONES/pulsedial.pd`, its `telephone-line.pd`, and `BELL/striker.pd` are identical to the references used here. Differences include `pow~` inlet wiring in `RAIN/rainonwater.pd` and `MOTORS/motorenv.pd`; the old `RUNNINGWATER/running-water4.pd` is complete whereas its `pd` counterpart is empty. Preserve those variants, the README, and required local dependencies when pruning duplicates. The [full manifest](reference/old-redundancy.json) supersedes the earlier [same-path snapshot](reference/old-inventory.json) for cleanup decisions; no source files have been removed by the audit.

Chris also supplied the full companion-site mirror in [zip/](zip/): 537 files including 35 full WAV examples and 298 patches. Its `p02/phonetones.wav`, `p06/telephonebell.wav` and `examples.tar.gz` hashes match the files downloaded for the first trial. The packager now prefers those local WAVs and verifies their hashes before extracting audio. The redundant compressed `examples.tar.gz` stays local/ignored; extracted site resources and the old collection are tracked.

PD-Vanilla 0.55.2 remains the initial executable reference. The [isolated primitive investigation](../tests/pd/README.md) checks current and 0.43-compatibility routines, including highpass gain and oscillator/FM differences. Original-release binaries have not yet been exercised. Do not replace a collection or dismiss a recording-level difference without recording the evidence.

## Rain investigation

The [rain practical page](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical15.html) describes its recording as a progression through pulse textures, noise-band excitation, bubble/drop sounds, and resonating glass. It is therefore a staged mixture, not necessarily the output of one static patch/preset.

Compare those stages with the local `RAIN/` collection, the updated page's patches, the book's chapter 38, and the audio. Investigate missing dependencies, parameter automation, PD-version differences, and possible additional processing without treating any proposed explanation as established.

## Attribution and local references

Credit Andy Farnell for the supplied patches and reference material. The located README states that its code may be redistributed and modified, in whole or part, and encourages attribution. It covers that collection's code; retain separately credited contributors' notices and avoid assuming an identical grant for unrelated website files or audio. This is not a blanket MIT relicensing. See the root [licensing notes](../README.md#licensing-and-attribution).

The privately owned book PDF is kept under `book/` locally and excluded from Git. Scratch research downloads, extracted pages and temporary comparisons belong under `build/`. The retained `pd/old/` and `zip/` collections, source patches, useful comparison scripts and documented reference fixtures are tracked.
