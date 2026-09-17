# Workspace cleanup — 17 September 2026

## Completed OSM work

[osm-improved.zip](../experiments/klang/osm-improved.zip) preserves 89 files at their original
repository-relative paths. Every archived file was verified against its SHA-256
hash before any live copy was removed. The archive includes superseded OSM
analyses, measured results, prototypes and benchmark variants, plus the source
snapshots needed to interpret them. Its manifest distinguishes retired files,
moved reports and dependency snapshots whose live copies remain.

65 superseded files were retired from the working tree. The accepted
[OSM report](klang/OSM-IMPROVED.md), listening decision and production manifest
remain live. The working header and previous-production comparison baseline
were not edited by cleanup. Concurrent editor changes were left alone; the
archive and production manifest identify the accepted promotion snapshots.
The production OSM regression runner remains
live and now defaults to the promoted header and full combined contract.

## Documentation and disposable files

Moved five Farnell reports into `docs/farnell/reviews`, three core compatibility reports
into `docs/experiments/klang/core/reviews`, and the OSM report/decision/manifest into
`docs/klang`. Updated links, catalogue evidence paths and the documentation
generator. Folder READMEs and audio-specific notes remain beside their content.

Deleted 24 disposable Python bytecode files. No source or analysis was discarded
without an archive.

## Klang 0.7.10 integration cleanup

The completed UE 0.7.9 review and subsequent Klang 0.7.10 work are preserved in
[`experiments/klang/klang-UE-0.7.9-integration.zip`](../experiments/klang/klang-UE-0.7.9-integration.zip).
Its 122 repository-relative files were independently extracted and verified
against `ARCHIVE-MANIFEST.json`; archive SHA-256:
`3559f14be256d73a15a76499710edf9f211d4fb7a921ce5016bc8d97da0f1dbe`.

The archive retains source snapshots and the compact JSON reports referenced by
the production manifest. Bulk WAV/F32 renders, plots, HTML reports, executables,
objects, dependency files and repetitive compiler logs are reproducible scratch
outputs and are excluded.

After listening acceptance and archive verification, 52 completed integration,
OSM and diagnostic build targets plus stray root compiler objects were removed,
reclaiming approximately 11.81 GiB. Active core/numeric, literal, time,
scheduling, PD, Farnell and toolchain work remains under `build/` as applicable.
An additional 222 regenerated Python cache directories were removed during the
final repository-wide hygiene pass.

Project Markdown is centralised under `docs/` by context (`klang`, `farnell`,
`experiments`, `tests`, and `licenses`); only the repository README and agent
instructions remain at the root.

## Work deliberately kept active

- Generic numeric types and core/API compatibility investigations.
- Unit literals and Time design.
- Audio-thread scheduling and PD block-processing experiments.
- Signal-flow ambiguity reproductions.
- PD primitive/model work, regression tests and source reference collections.

These remain live; inclusion of a shared header snapshot in the OSM archive
does not retire the active experiment using that header.
