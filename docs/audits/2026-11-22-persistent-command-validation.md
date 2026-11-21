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

## Combined armed-table qualification

The candidate was composed with the separate opt-in armed-type change and
compared to its armed-only cartridge. The Z8 saved HOME continuation matched
1,200 per-update frame, screen and route-counter rows, eleven world, both-page
and palette cuts, and SRAM. Mean guest cycles were 911,538 and 908,600
respectively. The Z10 three-CPU Blood Bath continuation matched 3,600 route
rows, 31 world and both-page cuts, sixteen paired fresh continuation cuts and
the pause SRAM. Mean guest cycles were 820,015 for both. The Z8 run starts
from a fresh saved state; the Blood Bath run writes a pause and verifies a
fresh process continuation.

P10 and Z10 prepared ASan/UBSan differential fixtures pass after explicitly
warming the cache, changing actor owners and coordinates, and reinitializing
the game. Both unit and building rows still reject malformed coordinates and
owners before query filtering. Startup clears BSS; in-process initialization
can retain cache entries safely because the entries contain only compiled
raw-type mapping and source geometry, never actor, owner, mission or save data.
The original validator remains the oracle for every supported raw ID and both
actor kinds, including coordinate and owner boundary cases.

All four combined links pass ROM and static RAM guards. BSS/IWRAM ends are
P10 `0x0203b130`/`0x03004ea0`, Z10 `0x02039870`/`0x03004f60`, Terran
`0x0203a6dc`/`0x03004b38`, and Showcase `0x020389a4`/`0x03004f00`.
Terran and Showcase cartridges are byte-identical to the armed-only controls.

Local Z8 control/candidate result SHA-256 values are
`41518efb065deeb08e46a3be1f223fe69da850b226a53b5bd893c593a8eddb27` and
`2a9a94313c9d811ed01333c2f704249e0b1e9bd4f899cf584d9c317eeb7b5ba1`.
The Blood Bath comparison is
`fb20e290ee589fac0913b729eb61406f1b98211c78211db4b9744f6cfeaf243d`.
