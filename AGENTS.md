# klang/sfx implementation brief

## Product

This project pursues two primary objectives:

1. a Klang implementation of the examples from Andy Farnell's Designing Sound (2010) book on procedural audio, based on the Pure Data (PD) patches;
2. a faithful port of the PD-Vanilla primitives used in the above text (targeting v0.42 and above, with the precise reference version established through comparisons).

In service of three wider objectives:

- a foundation for developing an open library of improved procedural audio models;
- a set of production-ready models for use in nash.audio's This Little World game and Future Sound of Bristol demo;
- the development and testing of the Klang language.

This Little World (TLW) is a forthcoming open-world game developed in Unreal Engine 5, based on a 1:1 recreation of Britain, featuring an entirely procedural soundtrack - with music provided by the Manhattan procedural music engine (nash.audio/manhattan) and sound effects developed using Klang (nash.audio/klang). The Future Sound of Bristol (FSoB) is a playable demo of TLW, showcasing its audio and music technologies, which is also being developed as an open playground to support the development of procedural audio and music - users will ultimately be able to integrate their own procedural audio sounds as dynamically linked libraries, developed using Klang or other technologies (such as Aaron McLeran's Rhizome).

To develop and test models, the project uses the Kleine (github.com/nashaudio/kleine) framework, a C++ console project template that provides Klang language and audio hardware support, with live playback and file output.

The following files and folders are included:

- include/klang.h : the Klang language, an embedded domain specific language (EDSL), providing audio syntax and semantics through native C++ (C++17).
- include/klang/pd.h : Klang implementations of PD-Vanilla primitives - a focus of development for project (objective 2)
- include/klang/utils.h : useful higher-level / reusable Klang DSP objects, useful for this project, but which may later be considered for inclusion/bundling with klang.h
- examples : a reference collection of ~50 Klang examples encompassing common effects processors and synthesisers, plus more elaborate demonstrations (e.g. DX7, THX sound)
- farnell/pd : the original PD patches distributed with the Designing Sound book - the key reference for the project
- farnell/klang : Klang implementations of the PD patches distributed with the Designing Sound book - a focus of development for the project (objective 1)
- sounds : a curated set of Klang procedural audio models, including those developed for the TLW/FSoB games - both original and improved/modified Farnell models.
- book : a privately owned local reference copy of Designing Sound; never track, commit, or publish this directory or extracts from the book.
- README.md : build/run instructions and the current licensing position.
- farnell/README.md : reference sources, archive identity, and PD compatibility findings.

The following further resources may be useful:

- the Designing Sound book by Andy Farnell (2010) - a key reference for the project, along with the accompanying PD patches (above).
- the MIT Press resource website for Farnell (2010) book - which notably provides audio (.wav) examples of patches.
- David Creasey's Audio Processes (2016) book - a good, implementation-agnostic DSP reference for block diagrams and readability

## Key Principles

1. Klang implementations should prioritise readability, supporting learning of techniques and sharing of models.
2. The syntactic sugar it provides, notably signal flow semantics, should not affect performance (is generally compiled-out) and should be leveraged to improve readability.
3. Klang style advocates concise, self-documenting implementations using simple naming conventions that function not only as executable code, but pseudocode-like markdown for audio programming / DSP. Klang implementations should be easily reconcilable with terms, languages, and forms used in literature (maths, signal flow).
4. As C++, Klang strongly advocates an object-oriented, modular approach favouring reusable, clean, logical abstractions that closely map to audio and user mental models.
5. First get implementations working and sounding faithful; optimise performance after the model and comparison process are established. Continue to use the klang::optimised namespace and avoid unnecessary overhead, but do not sacrifice readability or delay a working model for optimisation. Record comparable PD/Klang CPU and memory measurements as useful evidence, not initial acceptance gates.
6. Klang has signal flow semantics but is immediate mode C++ - sample/buffer processing - not graph based.
7. This project will inform the development of Klang, which is pre-release. Notably opportunities include testing existing implementations, developing new reference objects, and optimised buffer-based processing for existing per-sample implementations. Where possible, develop changes to core Klang in separate files under experiments/klang before integration into include/klang.h, pending review and approval. Keep useful experiment source tracked; put generated outputs under build/.
8. The main reference for this project consists of PD patches; Klang implementations should be functionally (audibly) equivalent, but not slavishly port the PD implementation, and should take every opportunity to abstract and simplify implementation details that are forced by the visual programming idiom, which can be more clearly and concisely implemented in C++. Differences that are not especially audible are acceptable, as are deliberate improvements that sound better. Record meaningful departures and retain a reproducible reference so that fidelity and preference can be assessed separately.
9. The book was written in 2010, so care might be needed when selecting PD versions. Some PD components will contain bugs or unusual implementation choices; recreating patches might require such details to survive, but they should be flagged and evaluated for replacement with more standard/performative/better-sounding models.
10. Farnell presents theory and concepts from first principles, but acknowledges a considerable role for trial and error; PD patches may not be fully explained by theory, and there may be opportunities for more elegant approaches that deliver equivalent or better audio quality.
11. Klang implementations of PD-Vanilla primitives can directly use the original C/C++ from the appropriate Pure Data repository, retaining its notices and recording the source revision. Faithfulness to PD syntax (inlets, outlets, parameters) may help both migrating models and users, but don't pursue completeness for completeness' sake. Bit-perfect agreement is a useful diagnostic where source, arithmetic, state, and scheduling match; it is not a blanket requirement for a successful model.

## Approach

LEARN : learn, run, and analyse the PD models in Farnell (2010), Klang language, and existing Klang models.
STUDY : map the patches against book chapters, and recreate the audio examples on the MIT Press website - which may not be fully covered by the supplied patches/discussion.
BUILD : develop Klang implementations of the book's models, guided by PD patches and audio examples; port PD primitives to Klang from the appropriate Pure Data repository.
CHECK : compare Klang implementations against the PD sound, and audio examples, with respect to sound, control, and behaviour.
ADAPT : develop improved Klang models that enhance usability - fewer controls, simpler/better mappings (one-to-many), presets.

## Reference versions and comparison process

Develop the process through small trial runs. Do not block implementation on choosing one historical PD release or on completing a large test framework.

- Use the installed PD-Vanilla 0.55.2 as the initial runnable reference, not as a claim about the book's original version. Record the actual version for each run; investigate older versions when a discrepancy warrants it.
- The supplied farnell/pd files match the official older examples.tar.gz byte for byte. Its advertised README/version information was not found in the tarball or its nested archives. No explicit authoring-version metadata was found in the local patch comments. See farnell/README.md; establish compatibility per patch as evidence emerges.
- Treat the book, supplied patches, updated website patches, and website audio as distinct references. When they disagree, record the discrepancy and its evidence rather than silently choosing one as correct. Preserve original PD files and make rendering wrappers or experimental changes separately.
- For each comparison record patch/model paths, source revisions or hashes, controls and units/defaults, event timing, duration and tail, sample rate, block size, seed where available, and any alignment or gain adjustment. Start at a shared 48 kHz, record PD's DSP block size (normally 64), and investigate 44.1 kHz or other rates where relevant.
- Begin with listening, rough peak/RMS waveform envelopes (about 10 ms bins), and sonograms. Use a Hann window with FFT size 16384 and hop 1024 as initial analysis settings, with consistent frequency axes and dB colour scales. At 48 kHz this spans about 341 ms with 2.93 Hz bin spacing; also use a shorter window (e.g. FFT 2048) for impacts, droplets, and timing details. FFT size is an analysis setting, not the DSP processing block size.
- Keep unnormalised levels available. Use explicitly documented level matching for listening when useful, and distinguish gain differences from changes in timbre or dynamics. Align deterministic signals before residual comparisons; compare stochastic sounds over representative durations and seeds using envelope/spectral statistics and listening rather than expecting matching noise samples.
- Let tolerances emerge from the trials and audible results. Check for silence, clipping, NaNs/infinities, DC offset, bad tails, and control discontinuities. Record whether a difference is an error, an acceptable approximation, or a preferred adaptation.
- Report CPU time per rendered audio second, elapsed render time, and memory with the machine, compiler/build type, PD version, duration, rate, block size, and voice count. Distinguish process startup, GUI/device/file-I/O overhead and total process memory from DSP cost; do not compare a sleeping live run with an offline render as if they measured the same work. Optimisation budgets can follow later.

Kleine currently demonstrates live playback/capture. Engine::SIMULATED records MIDI event history and does not render audio offline. Use direct DSP processing or a separate renderer for deterministic comparisons. Build/run instructions and known setup issues are in README.md.

## Trial models and game priorities

1. Develop the comparison workflow with chapter 25 (phone tones), then chapter 29 (telephone bell).
2. Use Chris Nash's ToyBoatEngine and FourStrokeEngine in sounds/Motors.h as existing Klang ports of Farnell for calibration and style. Their presence is established; the new comparison process should measure their agreement with PD rather than assume numerical equivalence.
3. Investigate Rain relatively early: identify which stages of the website recording can be produced by the supplied or updated patches, test version-sensitive behavior, and look for missing controls, patches, layering, or processing. Possible selection/post-processing of examples is a hypothesis, not an established fact. The website describes its rain recording as an evolving mixture, so compare its stages as well as whole-file summaries.
4. Wind is an early implementation target. The wider ambient priorities are background ambience, rain, tree leaves in wind, British birdsong, and footsteps. Chris can supply British bird recordings when needed.
5. A modern/performance car model based on the Lotus Esprit S4 is a further game priority.

Existing game models include the Mini car, Harrier jump jet, steam locomotive (steam and whistle), helicopter, and bicycle. Inspect and reuse these before starting overlapping work. The Rain object currently in sounds/Nature.h is a placeholder, not the target rain sound.

## Repository and licensing

Track project source, documentation, scripts, tests, useful experiment source, supplied PD patches, and their supporting reference assets. Exclude the book and intermediate/generated files: use build/ for downloaded research archives, temporary renders, plots, caches, and extracted book pages. Do not blanket-ignore audio or PDF formats because legitimate reference assets may use them. Keep any deliberately retained reference fixtures documented and tracked outside build/.

The intended outcome is an open resource. The overall licence and remaining provenance are TBD, with MIT or a similar permissive licence under consideration; do not represent that intention as an already granted licence for all included material. Chris has confirmed Klang Open License 1.0 for the Klang language; its terms are in licenses/Klang-Open-License-1.0.txt. Preserve upstream terms and attribution, document confirmed information in README.md, and keep the book local regardless of other material's licence.
