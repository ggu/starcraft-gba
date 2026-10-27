# Z7 save validation census

The experimental Z7 validator repeatedly decoded all 240 unit slots while
checking each actor and source-map row. Loading and the victory autosave could
therefore stall for a very long update. This change decodes a bounded metadata
census and reuses tuple counts and metadata for those comparisons.

Counts include retained-dead and canonical zero rows, as the original scans
did. Live-field checks, retained provenance checks, greedy source-row matching,
and the two-Zerglings-per-Egg obligation bound remain enforced. Metadata uses
stack storage; there is no persistent, BSS, SRAM format, or version change.
`MAX_UNITS <= 255` is asserted for the byte counters. The no-Egg configuration
also compiles without referring to its absent Egg unit type.

## Paired ARM continuation

Both ROMs loaded the same actual UI-generated SRAM from the full Z7 controller,
then replayed the remaining input stream in separate mGBA processes. The save
was not manufactured from a host fixture. The candidate header SHA-256 was
`dbcf4086fc10d288f25c3f93653fa3190f6ed1344d08ca69680448b66cfa421b`.

| Measurement | Original | Census |
| --- | ---: | ---: |
| HOME load update, cycles | 1,921,554,490 | 54,325,197 |
| Victory update including autosave, cycles | 1,724,237,141 | 50,771,294 |
| Observed stack use from initial SP, bytes | 1,496 | 4,264 |
| Observed stack margin above reserve boundary, bytes | 10,280 | 7,512 |

These are complete update timer deltas, not isolated validator timings. The
reductions are 97.17% and 97.06%. They measure pauses on this instrumented ARM
emulator path; they do not establish steady gameplay FPS or physical GBA speed.
The canary measures the exercised continuation, not every possible call path.

The loaded checkpoint 334 and all 31 subsequent checkpoints matched. Both
finished at frame 205764, won=1, lost=0, fault=0, with 50 surviving human actors,
no source Force 1 buildings, and zero cursor/clock masks. Final 32 KiB SRAM was
byte-identical: SHA-256
`83eaee02961eabf3724ddd5d8bd7b2eb6110f1c5b535c32f3e0944b26448239a`.
The newest slot had sequence 2 and payload length 16296, below 16368. Inactive
command-menu cursor normalization in the existing continuation digest is
unchanged; other named gameplay fields are retained.

Both instrumented builds ended BSS at `0x0203d7b4` and IWRAM code at
`0x03004c60`, below the unchanged linker guards. Both fixed ROMs were 32 MiB,
with boot header, full logo, checksum, SRAM signature and padding checked.

| Artifact | SHA-256 |
| --- | --- |
| Original fixed replay ROM | `6ae5d4ad443f2d714b943a55db9cfadf4d1084d123aa2a5530c2b717ccb4a6a9` |
| Census fixed replay ROM | `182ba72a23450a68d17142ebbf60785e9699c42a4e6d10b431c465e1164f4856` |
| Original actual UI prefix SRAM | `89272e18ad7319b5dcecbe37bdb963f03e557fe581c6e63e1758219accac54ed` |

Private evidence is under
`.local/build/z7-live-20261027/save-census/arm-resume-reviewed/`:
`paired-result.json`, per-variant commands, postlink checks and emulator logs.
These files and game-derived inputs remain outside the source repository.

## Host checks and scope

The public save fixtures assert acceptance/rejection for retained-dead source
rows, excess duplicate provenance, invalid owner/raw/type mappings, pending
human products, and Egg provenance/budget boundaries. Impossible row mutations
may be rejected by the row codec before candidate validation; those assertions
cover the combined boundary. Full, no-Egg and derived-row configurations run
under ASan/UBSan. These synthetic candidates are not mission-win evidence.

Independent source review found no acceptance-rule discrepancy. The paired ARM
replay uses its own frozen continuation harness, not the public fixture file.

This change does not promote Z7 or change the 22/30 qualified mission count.
The carried Z6 direct-targeting controller regression and steady gameplay
performance remain separate requirements. The all-30-mission, three-race ROM
and selectable 1–3 AI skirmish goal remains active.

## Final-source replay and standalone builds

The final tracked census header also passed a fresh complete 365-checkpoint
ASan/UBSan host replay against the frozen winning tape and expected digests,
ending at tick 209485 with source victory. Compiler include tracing confirmed
the exact header SHA above. Private commands, provenance and logs are in
`.local/build/z7-live-20261027/win-qualification/full365-census-finaltracked/`.
This is host evidence; the ARM evidence above is the saved-state continuation.

Standalone Z7 steering and unified ARM compiles were taken from their existing
Makefile compiler recipes, without executing asset-generation recipes. All ELF,
map and ROM outputs went to new private directories. Header, full logo, SRAM
signature, checksum, padding, size and linker guard symbols were verified.

| Build | BSS end | IWRAM end | Fixed ROM SHA-256 |
| --- | --- | --- | --- |
| Z7 steering probe | `0x0203dff0` | `0x03004c60` | `e2a9810eb3eb6576715dda6bf6715b28e1d7473f01cf468036b39f449bf3a5f0` |
| Unified | `0x0203d3c0` | `0x03004e28` | `0243911d815850182e82d64a32e936dcff9109ab4abcad51b4ab0ac754c3c76d` |

The standalone probe has only 16 bytes before its BSS guard. This change adds
no BSS; subsequent global storage changes still need a fresh memory review.
Both fixed ROMs are 32 MiB. These build checks do not qualify new campaign
missions or change canonical product flags.

Local final checks passed: `./dev doctor`, `./dev build`, `./dev test`
(563 Python tests plus the C checks), `make test-unified`, and
`git diff --check`. Full/no-Egg/derived public save fixtures passed with
ASan/UBSan. Source-boundary checking is run again on the staged source before
commit. No GitHub Actions were enabled or dispatched.
