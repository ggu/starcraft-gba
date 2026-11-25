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
