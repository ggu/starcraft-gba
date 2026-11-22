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

A private Z10 trial applies the scalar early-axis gate directly in the
normal ROM contact-steering kernel. It keeps the RAM endpoints and passes
650,046 live-mask comparisons and 4,220,593 step checks under ASan and
UBSan. The same late window matches frame and route counters. Full-loop
mean changes from 1,606,097 to 1,606,079 cycles, while the contact-mask
mean rises from 132,143 to 132,182. The trial stays local. Algorithmic
reuse of exact contact work has higher priority than this branch.

The candidate ROM SHA-256 is
`01f8059999fb30a91743bf7011b61c61e44a5ed8b12a90dcebbd2ed5025ac958`.

A separate P10 raster trial cached per-row spans for packed sprites.
The better direct-mapped variant raises mean actor-depth cost from
341,779 to 379,962 cycles per update in the 16-call profile and
uses about 4.3 KiB additional EWRAM. It also exceeds the IWRAM
reserve. This trial stays local. The packed compositor already skips
fully transparent groups and copies fully opaque groups directly.
Profile mixed-word and byte-sprite coverage before revisiting span
skipping.

## Full FIFO completions

On the same late Z7 replay, all six baseline point fills exhaust the
1,024-pop prefix. They then spend another 52,404 pops on full FIFO
completion. Increasing the cutoff to 2,048 and the parked frontier
limit to 128 still leaves all six origins undiscovered. Each frontier
fits at the cutoff. The 6,144 additional prefix pops replace exactly
6,144 full FIFO pops, and every field still needs complete packing.

Across all callers, the measured point-fill total rises from
4,397,916 to 4,402,615 guest cycles for the 16-update window.
The full mean moves from 1,606,097 to 1,605,900 cycles.
The earlier table reports point-fill costs within recorded movement
ancestry. This count includes calls through the rest of the orders
path. A separate whole-bucket rejection trial lowers full mean to
1,604,786 cycles on the same window. Both effects are smaller than
0.1%. Keep the trials local and trace actual first visits, frontier
widths and later demand to choose what work can be deferred.

A separate early skirmish hostility branch preserves the sampled
world, video, routing and SRAM. Mean cycles fall from 817,911
to 817,286 on the 3,600-update Blood Bath sample and from 1,535,936
to 1,533,977 on the 9,489-call P10 tail. Keep this branch local
while pursuing larger reductions in the measured full-fill work.

| Private receipt | SHA-256 |
| --- | --- |
| Prefix work baseline | `559fcd578378722c0b8c19d97f344d7f86995ab0dd5d1cdf00e109e6d252189b` |
| 2,048-pop prefix trial | `8564ea0592bcadf0be1d2a8474f7e73a65cb1ee42e3c32d26c3cdd5b3fda1546` |
| Whole-bucket rejection trial | `ba5bcd5121f060a9896b811fb2ed81b51151fba7df7df013d0288db5645c53a9` |
| Early hostility Blood Bath | `a4e98491e150c67a3e03fef25ae215b4bfc9190375998184a2c7e75774f06db1` |
| Early hostility P10 | `ec9ddd63c3d89f68e843620cc92b10ac6ea8cdb17f1786477e13e65206d8607c` |

A diagnostic FIFO records the first origin visit for each of the
six full completions at 7,855, 6,334, 5,469, 7,779, 6,272 and 5,400
pops. Each full field completes after 9,758 pops. The six total
pop counts match the release run. The actual origin hits all exceed
4,096 pops and all fall within 8,192. A 32-pop boundary rounds their
first observed hits to a total of 39,168 pops. The remaining 19,380
pops (33.1%) could be deferred before charging for incomplete field
packing and later reads. Pending queues count 63–78 records at the
individual hits. Measure frontier widths at the actual batch
boundaries and track later resumption and eviction.

The instrumented replay matches retained checkpoints to call 78,020
and the sampled frame and route counters. Its diagnostic window
uses a separate FIFO in ROM. Use its receipt for work counts.
The receipt SHA-256 is
`819480138f209e7c420642f2dde0be79170a703859ed44c3f489840750c5ecb9`.
