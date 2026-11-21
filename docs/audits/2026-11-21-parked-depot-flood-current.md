# Faster parked depot floods on 64×64 maps

The [busy Blood Bath profile](2026-11-21-blood-bath-current-profile.md)
found that depot reachability accounted for about 1.4 million guest
cycles in its slowest update. A private read-only register observer
classified the actual calls on the normal Z10 cartridge at `70fc138b`.
It used the same three-CPU Blood Bath pause save and ordinary HOME
resume. The observer matched all 31 world checks and the guest clocks
from the prior timing capture.

Across 56 selected updates, the observer found 393 reachability
checks. Four use the adjacent-depot shortcut. Of the remaining 389,
284 hit a partial field and 105 build or extend a partial field. All
393 checks find a route. The sampled workload gives a complete
negative-field memo no opportunity.

The slowest update contains six builds and 26 hits. The most
expensive call starts a fresh field. It pops 2,570 cells and parks
25 in FIFO order. Its field matches the independent scan, including
the remaining neighbors of the pop that discovers the origin.

## Shared engine change

The fixed-size loop uses the existing 64×64 parked facility
frontier. It tests and clears each discovered neighbor's walkability
bit from one byte load and writes the same packed direction. It
checks neighbors left, right, up and down and tests the origin
before the next whole pop. Cache lookup, victim selection, seed
geometry, parked data and publication stay in the same path. The
change uses the existing queue and packed field, with the specialized
loop in ROM. Other shapes and builds without a usable parked
frontier use the existing loop.

The paired observer matched all 393 operations, including the logical
cache and packed field of each routed call. Independent BFS
reconstruction verified all 105 fields published by a build. The
observer checked the exact pending queue, discovered count and
remaining walkability bits for all 105 parks. Build cycles across
those calls fell from 17,484,309 to 13,596,503, a 22.24% reduction.
The biggest cold search fell from 893,957 to 660,024 guest cycles.

The facility host suite now runs its existing cases with the 64×64
parked frontier active. That covers seed hits, a blocked origin,
perimeter requests, no seeds, disconnection and randomized grids.
The same fixture checks the unparked path, other dimensions,
interleaved resumes, topology invalidation and the older FIFO's
bounded overflow case.

## Ordinary ARM comparisons

| Sample | Normal mean / p95 / peak cycles | Faster loop mean / p95 / peak cycles |
| --- | ---: | ---: |
| Three-CPU Blood Bath, 3,600 updates | 924,956 / 1,215,190 / 4,124,647 | 921,496 / 1,215,063 / 3,812,129 |
| Three-CPU Outpost, 800 input calls | 1,247,097 / 1,644,620 / 3,170,099 | 1,240,649 / 1,639,608 / 2,668,740 |

Blood Bath matches the route counters on all 3,600 updates, the world,
both indexed pages and palette at 31 cuts, a byte-identical pause
journal and 16 fresh continuation cuts. Its peak falls 7.58%. The
middle third's p95 rises 0.27%.

Outpost uses a retained ordinary-controller tape from HOME and reaches
frame 18,592 on both current builds. The match is active at the end of
the tape. Its 95 observation cuts match the world, both framebuffer
pages and palette. The 800-call window matches route counters and the
sampled peak falls 15.81%. Across all 18,743 calls, including UI and
presentation waits, mean guest cycles fall from 828,222 to 827,408.
The full-run peak stays at 6,211,862 cycles.

The normal builds of all four products match their tested trial ROM
bytes and keep the previous IWRAM and EWRAM endpoints. The four
products match their predecessors at two sampled cuts in each campaign
or Showcase opening and four cuts on each Custom Game map. The
comparison checks the active page, both indexed pages, the palette
and world state. The three normal race ROMs pass 54 ordinary Custom
Game save and fresh HOME continuation cases. The matrix covers both
maps, every human race and one to three independently named AI seats.

The optimized Terran ROM completes T1–T5 through ordinary inputs.
Each starts from HOME, makes a pause save, resumes in a fresh
process, wins and reloads its victory journal from another HOME
process. All 257 retained gameplay checkpoints match the previous
normal Terran ROM, and all pause, victory and re-exported
journals match byte for byte. These campaign checkpoints cover game
state and frames. The campaign opening tests above cover video.

The optimized Protoss ROM loads its ordinary P10 pause journal at
frame 36,761 and wins at frame 46,057 on the retained controller
tail. All 68 world, camera, indexed-page and palette checks and the
victory journal match the previous normal P10 build. A separate HOME
load holds that result for eight updates and re-exports the journal
byte for byte.

The 30-fps budget is 561,792 cycles per update. Both busy samples
still exceed it, and physical frame pacing remains open.

| Local evidence | SHA-256 |
| --- | --- |
| Facility classification and paired field comparison | `93b4c387fed9609a5a3d080177e4069bd42de762fbbbf663f15db3b0c8e4ba49` |
| Blood Bath 3,600-update comparison | `c780ac37e0f1d31ec61bc997fe42c7c8fb3a9d14861a677d1e166e4a1a9bfeaa` |
| Outpost paired comparison | `22f68844c27e71f38eeb9716e6e1cb16b01d8ac30d3353e99b030450b57a8018` |
| Both Custom Game maps on four builds | `b63baf7fb78e1a2c75b5e7093018c52c754439f1f131dffc5076612997f70ab0` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Custom Game save and fresh HOME matrix | `428556c9b2a351ce46766fb86d69d1811cdf7a2d58cb591b684fd3efd87e6015` |
| Terran T1–T5 campaign comparison | `5afd300a78ca5f0e43d523764822b753a3878a345c8f3a94c67935f403807069` |
| P10 resumed victory | `cd0db3cb89df1f3d31e7be4486f8937e71749396916230ff36f450f6637e12ba` |
| P10 fresh result load | `2c449d87ac4df1ab14fa6ef51db29424628b5e8d29bfb425be4c62e91463d67f` |
