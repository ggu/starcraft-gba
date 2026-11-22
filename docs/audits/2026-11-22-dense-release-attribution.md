# Dense campaign cycle attribution on the shared-depth release

The exact `6c9b4482` Protoss ROM resumes P10 through normal HOME at
frame 36,761. The retained physical controller tape drives the save
through 16 consecutive instruction-attributed updates at calls
39,760–39,775 and frames 38,745–38,760. A paired run without
instruction profiling reproduces the guest clocks and stage counters at
all 16 cuts.

| Stage | Mean guest cycles per advancing update |
| --- | ---: |
| Full loop | 1,572,861 |
| Game update | 839,828 |
| Orders, inside game update | 513,748 |
| Movement, inside orders | 137,009 |
| Contact steering, inside movement | 43,741 |
| Target-index rebuild | 61,023 |
| Nearest-target search | 58,709 |
| Sight | 75,192 |
| Campaign tick | 63,105 |
| Defenses | 63,026 |
| Game render | 692,897 |
| Depth-ordered actors, inside game render | 341,779 |
| Draw units with their images, inside depth-ordered actors | 246,755 |
| Interrupt handling | 39,435 |

The stage figures include their callees. Keep nested stages separate when
estimating headroom. Each update exceeds the 561,792-cycle budget for
roughly 30 fps. These are emulated instruction costs. Measure physical
handheld pacing separately. The new-route counter stays at 58 through
this window. Instrument actual queued field completions before treating
the span as route-free.

The largest measured recurring costs are orders and actor drawing.
Prioritize reducing repeated movement and targeting work while
retaining their live-state semantics. Separate pixel composition and
actor admission when profiling draw work. A local trial reusing the
painter's already-checked visibility saves 966 cycles per update in
this short window (0.061%). Across 2,071 calls its mean changes from
1,495,764 to 1,495,500 cycles (0.018%). The two builds match all 24
sampled world, indexed display, palette, camera and fog cuts and SRAM.
The visibility trial stays local. The active battle and all-race
skirmishes still need broader holds for future rendering changes.

| Local receipt | SHA-256 |
| --- | --- |
| Exact 16-call instruction attribution | `a1c4fb3b6ea240764fd5c297daca60b9a542e0848fac83c450570cd5c71d3150` |
| Matching unprofiled readings | `4baafa262324d6360c7e3738499ca2a9e5703391e1347707271b7de928fc5ae2` |
| 2,071-call visibility control | `777595c1a8f200dee4df2f7026cef09593a3000c2c761914c99983cd2e1e8c5b` |
| 2,071-call visibility trial | `645a27e2ab3128619d4a8f4ffa0b22c8c676eaf605ebffa43f865bb60b1c919c` |

## Early Z7 movement and target indexing

The corrected rendered controller drives the exact `6c9b4482` Z10
cartridge into Z7. All retained observations through the profile
boundary match. Host calls 20,000–20,015 cover 16 advancing guest
frames 19,076–19,091. The preceding observation has 24 friendly and 91
enemy units.

| Stage | Mean guest cycles per advancing update |
| --- | ---: |
| Full loop | 880,257 |
| Game update | 396,059 |
| Orders, inside game update | 201,208 |
| Rebuild target index from owner lists | 39,280 |
| Link target cells, inside index rebuild | 23,701 |
| Movement, approximate symbol attribution | 55,347 |
| Uncached contact mask, approximate symbol attribution | 24,685 |

Every sample reconciles the instruction, call-stack and guest clock
totals without profiler overflow. Movement attribution includes inlined
code and uses approximate symbol boundaries. The route build counter
stays at 481. Hits rise from 24,458 to 24,497 and the complete call-stack
report contains no route fill in these cuts. Capture the later Z7
battle separately.

The recurring target-index work is still worth measuring. The retained
index in [draft #590](https://github.com/ggu/starcraft-gba/pull/590)
found matching membership at 26 adjacent late Z7 cuts and matching
canonical chain order at nine. Preserve canonical insertion order and
same-update relocations. A bounded next trial can hold each owner's
head-array base and occupancy masks within the existing fresh build.
Compare complete ordered chains, geometry failure exits, owner and
cell changes, fallback selection, and ordinary high-density replays.

The Z7 validation SHA-256 is
`2fd3e8fff9eb85b180f18d1373e4bf33b09a9a0eeeb3650bb71919c6295a2c7b`.
The full call-stack report SHA-256 is
`e65b08419b39d3a5b1f01df29067d1ef6bb2d0c4ac39061e6e94604f4e835b2c`.

## Owner-local index trial

A private trial based on `98678f70` kept the same fresh owner traversal
and batched each owner's cell occupancy writes. All nine target-index
sanitizer configurations pass. The same 16 P10 calls match frame,
screen and route counters. Index rebuild cost falls from 61,023 to
58,403 cycles per update. Mean full-loop cycles fall from 1,572,861
to 1,569,808 (0.19%) and the function's ARM stack grows from 52 to
76 bytes. This trial stays local. Further index work should preserve
one insertion path and demonstrate a sustained gain in the late
campaign and busy skirmish holdouts.

The control validation SHA-256 is
`9482a301ab811e640268d6a6e73dcc1d9ae960bcf501ee7ebd901ce30c3b6b71`.
The candidate validation SHA-256 is
`726361eb8f43274cdf0fa58819ebac1c2861da6f244d6c31ab41cc6d1d955fd2`.

## Later Z7 orders and point routing on the steering release

The normal Z10 ROM from `c2624205` starts a fresh emulator from the
verified pause at call 31,892. The physical tape matches the retained
observations through call 78,020. Instruction samples at calls
80,000–80,015 each advance the game and cover frames 78,513–78,528.

| Inclusive stage | Mean guest cycles per update |
| --- | ---: |
| Full loop | 1,606,097 |
| Orders | 886,645 |
| Movement within orders | 347,069 |
| Point-prefix fill within movement | 149,833 |
| FIFO expansion within point-prefix fill | 94,514 |
| Packing output within point-prefix fill | 25,540 |
| Contact steering within movement | 142,500 |
| Uncached contact mask within steering | 132,143 |

The six increments in the logical build counter establish when routing
work recurs. The instruction profile separately attributes the executed
work, including prefix resumption and frontier expansion. Orders rises
to roughly 1.36–1.48 million cycles on the sampled build updates,
compared with roughly 0.57–0.68 million on the other updates.
Contact-mask geometry and FIFO expansion have the largest measured
inner costs. The detailed report SHA-256 is
`5aa4bab70b3d117925e895826b6668ea99607d6c390c19b3ee38355444c62487`.

A separate P10 raster trial cached per-row spans for packed sprites.
The better direct-mapped variant raises mean actor-depth cost from
341,779 to 379,962 cycles per update in the 16-call profile and
uses about 4.3 KiB additional EWRAM. It also exceeds the IWRAM
reserve. This trial stays local. The packed compositor already skips
fully transparent groups and copies fully opaque groups directly.
Profile mixed-word and byte-sprite coverage before revisiting span
skipping.
