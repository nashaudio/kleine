# Designing Sound references

Reference models from Andy Farnell, *Designing Sound* (MIT Press, 2010). Original supplied patches and supporting files are in [pd/](pd/); Klang ports are developed in [klang/](klang/). Chris Nash's existing `ToyBoatEngine` and `FourStrokeEngine` ports are in [sounds/Motors.h](../sounds/Motors.h).

## Source and version investigation

Checked on 14 September 2026:

- [MIT companion index](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/index.html), linked from the [book's publisher page](https://mitpress.mit.edu/9780262288835/designing-sound/).
- [Official examples.tar.gz](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/examples.tar.gz): 57,441,106 bytes; SHA-256 `cc0ed3ea85ec5e3d54971fda2ff88af447b4e1301ba59985c15455fda2af067a`.
- All **505 regular files** in the archive's `BOOK-PUREDATA/` directory match the supplied `pd/` files byte for byte, including all 434 `.pd` patches. This identifies the supplied collection with the site's older bulk download.
- The site describes the tarball as intended for Vanilla Pd below 0.42, directs readers to version information in a README, and separately labels its individual chapter/practical downloads suitable for 0.42 and above.
- The tarball contains 557 entries including directories, but **no README, README.md, version file, or licence file**. Neither nested archive (`REVERB.tar.gz`, `phonetones.tar.gz`) contains the advertised notes. No explicit authoring-version metadata was found in the supplied patch comments. Do not infer a precise PD release from canvas geometry or filesystem timestamps.

The historical PD baseline remains open. PD-Vanilla 0.55.2 is installed locally and is the initial executable reference. Record the version used in each trial; investigate compatibility differences per patch. Do not replace the original collection with newer website files without recording the differences.

## Rain investigation

The [rain practical page](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical15.html) describes its recording as a progression through pulse textures, noise-band excitation, bubble/drop sounds, and resonating glass. It is therefore a staged mixture, not necessarily the output of one static patch/preset.

Compare those stages with the local `RAIN/` collection, the updated page's patches, the book's chapter 38, and the audio. Investigate missing dependencies, parameter automation, PD-version differences, and possible additional processing without treating any proposed explanation as established.

## Attribution and local references

Credit Andy Farnell for the supplied patches and reference material. Their precise redistribution terms remain to be established; availability on the companion site is not a declaration that this project has relicensed them under MIT. See the root [licensing notes](../README.md#licensing-and-attribution).

The privately owned book PDF is kept under `book/` locally and excluded from Git. Downloaded research archives, extracted pages, and generated comparisons belong under `build/`. Keep source patches, useful comparison scripts, and documented reference fixtures tracked.
