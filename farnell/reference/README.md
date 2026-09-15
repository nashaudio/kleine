# Updated website patches

Retrieved 14 September 2026 from the official [phone-tone practical](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical02.html). These files are preserved byte for byte alongside the older supplied collection, which remains unchanged.

| Local file | Upstream | SHA-256 | Difference from supplied patch |
| --- | --- | --- | --- |
| [p02/dialtone1.pd](p02/dialtone1.pd) | [dialtone1.pd](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/p02/dialtone1.pd) | `d945e91d814b8b0be298e61217a03f750e71a242004e3f976c8b1b5589728ab9` | 450 Hz replaces 440 Hz for the second oscillator. |
| [p02/busy-signal.pd](p02/busy-signal.pd) | [busy-signal.pd](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/p02/busy-signal.pd) | `a5191f81b84e5197a5efe1d51ee94435c8ff52ed6de49cb782eecdb0133cc889` | Adds 100 Hz lowpass smoothing to the gate; removes the unused manual 600 Hz oscillator. |

Other inspected website files need no duplicate copy for these trials: `dialtone2.pd` differs only in a subpatch window-open flag; `ringingtone.pd` is identical; `pulsedial.pd` embeds the same `telephone-line.pd` DSP as a subpatch. The [telephone-bell page](https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/practical06.html) distributes `telephonebell.pd`, byte-identical to `../pd/BELL/striker.pd` (SHA-256 `c7fb5de34ad421039cedb92d43e39a62c726dee56864631106d26ce597e9eec2`), and matching bell abstractions.

Attribution: Andy Farnell, *Designing Sound* (MIT Press, 2010). The newly found [old collection README](../pd/old/AAA-README.FIRST) allows distribution and modification of its code. The extent to which that grant covers separately updated website files remains to be established; these patches have not been relicensed by this project. See the root [licensing notes](../../README.md#licensing-and-attribution).
