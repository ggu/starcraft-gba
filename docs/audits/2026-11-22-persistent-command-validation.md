# Persistent Command raw validation

The direct Command census validates every live row on every query. Its
raw-type memo previously cleared 456 bytes on each query and recomputed
immutable raw mappings and source geometry. The shared census retains that memo
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
at `0x03004ea0`, within the existing static guards.

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

The combined Z10 ordinary HOME and fresh pause continuations both won at
frame 51,588 with 23 and 21 checkpoints, respectively, and no historical
drift. The fresh victory journal passed an independent HOME readback. The
combined Z7 replay completed call 231,031 with all 254 world checkpoints,
ten video cuts, six route-cache cuts and victory readback matching the pinned
reference. The local Z7 result SHA-256 is
`108afef5613c5d0fdb7d9c4551ccce3e8ca3a1146272966ca1764cdfb8e9b382`.

## Rebuilt release

Starting from `cec1b27e`, all four rebuilt cartridge binaries have the same
SHA-256 values as the replayed candidates. Terran and Showcase also match
the preceding release. A fresh repeat of the Protoss 10 saved campaign
tail matches all 68 world, display and fog cuts, all 9,489 frame/screen/route
rows and victory SRAM. The repeated timing is 1,554,111 mean main-loop
cycles. Both prepared census checks pass with ASan and UBSan.

All 1,134 normal-menu Custom Game launches pass using both maps, every
human race and every nonempty assignment of the three CPU seats. The
54 ordinary save-and-resume cases pass. Each of the four products also
matches 41 Custom Game camera cuts and both sampled campaign or Showcase
opening cuts. Use physical timing to qualify play on GBA hardware.

| Local evidence | SHA-256 |
| --- | --- |
| Rebuilt P10 tail | `5361ea3d71fcf208a0c5dd03d4e89ae6bd969928a13dbafb19f996165d68a987` |
| All Custom Game launches | `486dba4794a77f84f2dd206e6104af652d29e84ddb2ead5713cd09adf6ba5a14` |
| Custom Game save matrix | `a0ebb028604a601c8e6c99e98790283fd07e5699eab6b1fadfd906d8a68230fa` |
| Custom Game camera comparisons | `356b3078e1ebb4c9386f0bbd2de45b4c4bac8388fc8e967a7f77a47a1b3a1d59` |

## Distance from the timing target

The two-VBlank budget is 561,792 guest cycles. In the rebuilt P10 saved
tail, 9,294 samples advance the gameplay frame. Their median game-update
cost is 856,797 cycles before audio and rendering. Median and p95
main-loop costs are 1,484,475 and 2,059,784 cycles. None of these advancing
updates fits the two-VBlank budget. The busy three-CPU Z10 Blood Bath
window has a median of 799,488 cycles, and nine of 3,600 updates fit the
same budget. These comparisons use emulator counters. Both simulation
and display costs contribute to the remaining pacing work.
