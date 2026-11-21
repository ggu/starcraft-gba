# Persistent Command raw validation trial

The direct Command census validates every live row on every query. Its
raw-type memo previously cleared 456 bytes on each query and recomputed
immutable raw mappings and source geometry. This trial retains that memo
across calls. Owner and coordinate checks, live pool scans, filtering and
failure behavior remain per query. The cache is static scratch, not saved
state, and adds no storage over the existing cache.

The prepared P10 cartridge at source `f11e089` was compared with the
one-header change over the 9,489-call ordinary-input saved campaign tail.
All 68 sampled world, both framebuffer pages, palette and fog observations,
per-call frame, screen and route counters, victory and SRAM matched. Mean
`demo_last_cycles` fell from 1,565,224 to 1,554,111 (0.71%); mean
`demo_update_cycles` fell from 891,254 to 880,116 (1.25%). Peak
`demo_last_cycles` fell from 33,275,388 to 33,207,413. These are emulator
guest cycle observations, not physical frame pacing.

The prepared P10 ASan/UBSan Command census differential passed, including
raw IDs, both kinds, owner boundaries, coordinate boundaries, and the
original actor provider comparison. P10 ends BSS at `0x0203b130` and IWRAM
at `0x03004ea0`, within the existing static guards. Additional cartridge,
replay and physical timing qualification remains open.

Local receipts are under `/Volumes/Storage/sc64-route-next-20261122/build/route-next`.
The full control and candidate `result.json` SHA-256 values are
`b6380d144cf18424a39e11e7754e98d39597125310fb06ddec4a63663bcfc943` and
`75420d37f257ae528a9c715b42d4b63fd2e7b7ffd7ae0004ac0278daf2f31eb4`.
