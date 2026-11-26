# Z10 three-Zerg victory: pacing and depot validation trial

This measures the newly qualified physical-input Blood Bath win against three
Zerg CPUs, using its ordinary pause save at frame 9,044 and the following 512
recorded input updates. The integrated Z10 control ROM is
`2d1d00751fbc9e82a012823ec54fedda54dd3a55b0cf5d1a001be2de268fe7fe`;
the input tape is
`38a6a8e03ebbf660b31ec318c7b23d157ab20166333bc1f386dc5a04ede81144`.
The window includes 491 advancing updates and seven checked gameplay points.
These are emulated cartridge cycles, not hardware timings.

The disjoint budget uses the same-iteration `demo_last_cycles` counter and
advancing updates. Means, in cycles per advancing update:

| Stage | Control |
| --- | ---: |
| Orders | 296,616 |
| Sight | 41,012 |
| Other simulation | 116,500 |
| Remaining simulation | 24,032 |
| Background | 105,912 |
| Actors | 180,057 |
| HUD | 89,852 |
| Remaining rendering | 66,179 |
| Loop less update and draw | 386 |
| Total | 920,546 |

Orders are the largest individual stage; even removing them entirely would
leave the mean above the 559,241-cycle nominal 30 fps target. The 561,792-cycle
two-vblank interval is a separate comparison. A diagnostic call-stack profile
of the first 20 updates attributes roughly 11,472 self cycles and 80 calls per
update to `race_resource_depot_valid_for`; its profiler adds overhead, and that
short capture is not the 512-update budget or a whole-match estimate.

A scratch-only trial changed the existing validator's declaration from
`static int` to `static inline __attribute__((always_inline)) int`. Its logic
was unchanged and no default flag or product source was altered. It used the
same compiler recipe and generated inputs, with ROM SHA-256
`d43d3ffb0f1d372d21139027be4a5df1aac6ceb47a25ccf4b30b8788ca7130da`.

| Metric | Control | Trial |
| --- | ---: | ---: |
| Total debugger interval cycles, 512 updates | 477,319,634 | 475,619,682 |
| Mean orders cycles, advancing | 296,616 | 293,115 |
| Mean total cycles, advancing | 920,546 | 917,036 |
| Interval p95 | 1,265,860 | 1,266,136 |

Total interval cycles fall 0.356%, while p95 rises 276 cycles. All seven
gameplay checks and the final world fingerprint agree. At all 513 boundary
samples the raw unit and building arrays, both framebuffer pages, palette,
and camera match; the displayed page selector differs at five samples.
The Z10 stack guard passes with a 5,992-byte bound, 512-byte margin, and 448
bytes spare; text grows by 120 bytes. This is one saved combat window, not a
full match, campaign, other race, or host-semantic proof. Do not enable the
trial based on this window alone.

Local receipts: `/private/tmp/sc64-z10-913-pace/{control,candidate}/pacing.json`,
`profile/profile.log`, and `stack.json`; the scratch generator and candidate
source are alongside them. The raw private input assets are not included.

## Reject defense cells without hostile owners

The first 20 profiled updates attribute about 24,672 self cycles
per update to defense target searches and 9,090 to building their
cell lists. The Custom Game defense pass now sets an owner mask while
building each cell, then skips the cell if none of its owners is
hostile to the firing building. The mask is conservative after a shot
kills a unit. The existing live-unit check and lower-slot tie break
still apply.

The same pause save and 512 original inputs produce these results on
the integrated `82fc1a73` control and the owner-mask candidate. The
candidate adds 64 bytes to the defense wrapper's temporary storage.

| Metric | Control | Owner mask |
| --- | ---: | ---: |
| Mean total cycles, advancing | 920,546 | 901,404 |
| Mean other simulation cycles, advancing | 116,500 | 97,138 |
| Total debugger interval cycles | 477,319,634 | 468,044,477 |
| Interval p95 cycles | 1,265,860 | 1,256,575 |
| Emulated loop updates/s | 18.00 | 18.35 |
| Intervals over two VBlanks | 509 / 512 | 509 / 512 |

All seven gameplay checkpoints and the final fingerprint match. Both
bitmap pages, the palette, camera and raw unit and building arrays
match at all 513 boundary samples. The displayed-page selection differs
at two samples. The 2.08% mean saving closes 5.30% of the gap to the
nominal 30 fps budget in this replay. Other player counts, both-map
battle coverage and physical hardware pacing remain unmeasured here.

| Receipt | SHA-256 |
| --- | --- |
| Control timing | `5f40f1e10d8db67398fa8335f641ae73b369441264f36395e250cd58fdfb298b` |
| Candidate timing | `7f94e375bd3f7cc29b8bb58a151d689aa4e8652a795778168bf1c2f47b45eb75` |
| Candidate ROM | `8c4320da2c22945ad4ab50de9d777df9a8199cc1e4a91d274cd7d5ac0a47e4a6` |

### Current source and sparse opening

On paired builds from `3bf44419`, the same Z10 battle repeats the
gain: 920,594 to 901,413 mean advancing cycles. Interval p95 drops
from 1,265,860 to 1,256,926, p99 from 1,560,442 to 1,560,095, and the
maximum from 1,937,777 to 1,920,982. The seven gameplay checkpoints,
final fingerprint, both bitmap pages and palette match. At samples
292 and 371, the control still displays the opposite page while
the candidate already displays the common work page. The displayed
images agree again at the next boundary in each case.

A separate Terran Outpost game against one Zerg CPU used ordinary
lobby controls and a pause save at world frame 1,501, with 14 units
and three buildings. Both builds loaded the same save in a fresh
process, settled for eight neutral updates and ran 512 more.
Gameplay, both bitmap pages, palette and displayed page match
at every measured boundary.

| Sparse interval metric | Control | Owner mask |
| --- | ---: | ---: |
| Mean cycles | 561,778.63 | 561,778.64 |
| P95 cycles | 583,673 | 583,657 |
| P99 cycles | 602,816 | 602,819 |
| Maximum cycles | 628,130 | 628,128 |
| Intervals over two VBlanks | 227 / 512 | 226 / 512 |

The current Terran and Protoss Custom defense differentials each pass
800 randomized scalar-versus-indexed full passes. Protoss also
passes the early-kill, tie, range, visibility, cooldown, invalid-index
fallback and saturated-impact cases. All four production ARM links
pass their linker guards. The hot-stack checks retain 40 bytes
of spare IWRAM in Terran, 360 in Z10 and 384 in P10 beyond the
512-byte margin. The P10 checker warns that its configured IWRAM
limit is above the computed safe maximum. Terran and Z10 each use
88 additional linked IWRAM bytes in the paired build.

### Terran IWRAM placement

Keeping the defense search and cell builder in IWRAM retains the
combat saving. The Terran link runs the depth projectile child
classifier from ROM to recover the 88 bytes added by the owner mask.
Terran's IWRAM end returns to `0x03005d28`, with 128 bytes beyond
the unchanged 7,768-byte main stack bound and 512-byte margin. The
IRQ stack bound remains 264 of 416 bytes. The Z10 link and its
measured battle performance are unaffected by this Terran placement.

Two Terran Custom Game pause saves were replayed for 512 neutral
updates per variant. The sparse Outpost save has 14 units and three
buildings. The three-opponent Blood Bath save has 81 units and
26 buildings. Mean interval cycles differ by at most 0.01 in both
cases. The p95, p99 and maximum differences are at most 16 cycles.
One more interval in each run crosses the two-VBlank boundary.
Both pages, palette, displayed page, world samples and endpoint
fingerprints agree at all 513 sampled boundaries per case.

A projectile-active Terran T5 run loaded the earned stage 34 save
and played inputs 401–1300 of stage 35 in the
[paid route fixture](../../tests/fixtures/terran-t5-paid-route.json).
At 643 of 900 sampled boundaries a live source-profile tracer had
a start or end point in the viewport, with up to 32 such traces.
The tracer state, both Mode 4 pages, palette, camera, displayed page,
raw unit and building arrays and final fingerprint all agree.

| T5 interval metric | IWRAM classifier | ROM classifier |
| --- | ---: | ---: |
| Mean cycles | 711,834 | 712,459 |
| P95 cycles | 866,503 | 867,212 |
| P99 cycles | 954,762 | 956,067 |
| Maximum cycles | 1,065,560 | 1,066,261 |
| Intervals over two VBlanks | 770 / 899 | 770 / 899 |

Packing the cell heads and owner masks into halfwords used eight
more linked IWRAM bytes on Z10. Moving the cell builder to ROM
recovered 168 bytes on Terran but added 2,321 simulation cycles
per update in the crowded Terran window. The projectile placement
preserves the Terran simulation improvement and costs 0.088% in
the sampled T5 effect-heavy rendering window.
