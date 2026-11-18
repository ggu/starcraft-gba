# Dense Zerg 7: measure same-goal retention before spending RAM

The read-only mGBA observer from [#492](https://github.com/ggu/starcraft-gba/pull/492)
replayed calls 47,758–48,237 on the exact November 13 KODAK Z10 ROM.
All 131 checkpoints matched. The 480 updates produced 448 point fills
across 308 updates, to 11 distinct goals, with no topology resets. All
fills came from the ordinary miss at `0x0802c8a0` in the pinned ELF.
None had a resident field with the same ordered walkable seeds, so the
blocked-goal alias would not help this workload.

The stock 14-slot bank kept identity physical mappings and had no donor
planes. Of 448 misses, 319 replaced a fresh complete point field. An
offline sequence retaining one, two or three evicted point fields
would have matched 51, 92 or 132 subsequent misses (11.4%, 20.5% or
29.5% of all point fills). It tracks only this verified point-miss
callsite, excludes victims of facility and range work, and preserves
logical eviction. It does not run the GBA copy or a candidate build.

A retained 128×128 packed plane costs 6,144 bytes. First test one plane
on the same tape and measure the extra copies, paired game state and
all production link reserves; compare it with the wide FIFO already
profiled on this window. More than one plane would exceed historical
P10 EWRAM headroom. The replay result is an opportunity estimate;
neither a speedup nor default activation is established.

| Artifact | SHA-256 |
| --- | --- |
| Exact KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| Stock ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| Authenticated HOME tape | `e1c7693d633f1ecd486036e797baea555dcaddf1e0a409d4da397f3b2f3fbe17` |
| Register observer | `b0b9abdf79604b7ebca7f877c3b1e8774d0a4c19e0ffe2996fbd054ecc0ae1f6` |
| Stock seed receipt | `96c975a51fc0c4e54a234abe1195850120227c479ea46b94c0d83bfd1380f3b7` |
| Retention receipt | `8f0a39d5b70702e6ce4572911199186100dfe8bb4d0eac57a40a69ada902b162` |

## One-plane ARM result: do not enable

A Z10 probe preserves the stock logical insertion and adds one private
field for point misses on 128×128 maps with no stock donor planes.
On a miss it can retain a fresh point victim; on a match it copies or
swaps the packed bytes. It invalidates on route resets and geometry
changes. The asset-free sanitizer fixture compared the retained result
with an independent BFS across all 16,384 cells, checked the logical
miss counters, and verified that topology invalidation forces a fill.

The paired ARM build uses the same KODAK source vector as the wide FIFO
probe and adds only the retention flag. All 131 checkpoints, the route
build and hit counters on all 480 calls, and raw world, grid and logical
field bytes at both endpoints matched the wide FIFO. Of 480 calls, 477
advanced the simulation. Mean sampled active draw/update cost grew
**0.55%**; orders grew **0.89%**. The 95th-percentile call fell from
4.704M to 4.567M cycles, but the worst call grew from 5.970M to 6.096M.
The dense speed falls from 5.54 to 5.51 emulated updates/s. Z10 passes
its guards but uses another 6,152 bytes of EWRAM; IWRAM is unchanged.
Skip the copy-based scheme in production. The wide FIFO and narrower
floods that can prove the same route answers remain better candidates.

| Artifact | SHA-256 |
| --- | --- |
| Retention candidate ROM | `5fd918f89087f1780202cc03d59cb17d0388b4f58f50ccc53608e3fcce867a10` |
| Matching ELF | `5e63aa55632f7bfd2f8674d75f5e22b1fad3f4dbe2f96cf2edda3c8316714b68` |
| Paired timing receipt | `982716b63b52b23526f15fb1d750323a00fccadcef30491474301512bb943f2f` |
| Shared endpoint state receipt | `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4` |

## One-plane pointer retention on the current FIFO

The default-off `SC64_ROUTE_POINT_POINTER_PROBE` rotates one spare
backing pointer into a safe logical slot on a 128×128 point miss.
The victim's bytes become the spare without a copy. It preserves the
logical slot, key, age, counts and eviction order, and excludes the two
slots backing the fixed range scratch planes. A reset or grid rebuild
invalidates the retained key. On asset-free ASan/UBSan tests, the recovered
bytes match an independent BFS, the logical build still increments, and
both range scratch pointers remain fixed across an actual plane claim.
A 386,646-query differential also matches the logical residency receipt.

The paired Z10 build adds only this probe to the production pointer FIFO.
All 131 checkpoints, all 480 frame and route counter samples, and raw
world, walkability and valid route-field bytes at both endpoints match.
Of the 480 calls, 477 advanced the simulation.

| Guest cycle counter | Pointer FIFO | Pointer retention | Change |
| --- | ---: | ---: | ---: |
| Whole loop, active mean | 2,941,972 | 2,858,242 | −2.85% |
| Movement orders, active mean | 1,784,487 | 1,700,683 | −4.70% |
| Whole loop, all calls p95 | 4,420,534 | 4,053,982 | −8.29% |
| Whole loop, all calls maximum | 5,583,925 | 5,361,778 | −3.98% |

All four 32 MiB ARM builds verify and pass the unchanged linker guards.
The tightest EWRAM margin is 1,816 bytes on Protoss P10. IWRAM does not
change on any of the four products. Keep the probe off pending fresh
process SRAM continuation and physical input parity on the other wide
maps. The measured improvement is in guest cycles for one Z7 interval.

| Product | EWRAM end | IWRAM end |
| --- | ---: | ---: |
| Terran | `0x0203b468` | `0x030048a8` |
| Zerg Z10 | `0x0203a310` | `0x03004d50` |
| Protoss P10 | `0x0203d8e8` | `0x03004c10` |
| Showcase | `0x0203b424` | `0x03004e60` |

- Candidate Z10 ROM: `1152e11de4b6bf2426558f8fb33af015215563b2ce1f89ddfec6224d25aff869`.
- Paired dense replay receipt: `e92822c5dc324e492065127a63b471cd88d6478add9dea18850055d31b5bb074`.
- Matching raw endpoints: `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4`.

## Current-cartridge composition (November 15)

`CANONICAL_ROUTE_POINT_RETENTION=1` builds this variant through the normal
make targets. The default is off. Against release `33224096`, the four
Blood Bath pairs use ordinary buttons to play, save, load in a fresh
process and continue for 180 updates. All 21 compared fields and both
32 KiB SRAM exports match within every pair. Separate physical T4 and
P10 sequences match all 12 checkpoints each, including world data,
video, walkability, route counters and valid fresh field bytes. Both
executed point floods on 128×128 maps; neither sequence exercised a
retained hit or establishes a new campaign win.

Z10 and P10 were independently re-linked with the current Command
census. The strict EWRAM guards still have 15,600 and 1,808 bytes of
headroom respectively, and IWRAM does not grow. The current Zerg 7
control and retention ROMs match all 131 checkpoints, 480 count and
frame samples, and raw endpoint hashes. Active cost falls from
2,941,970 to 2,858,242 guest cycles (−2.85%); all-call p95 falls from
4,420,530 to 4,053,982 (−8.29%). The mean corresponds to 5.87 emulated
updates per second in this dense window. The extra plane costs about
6 KiB even on ROMs where no retained hit has been observed; leave the
switch off pending broader dense-map timing and hardware checks.

| Local receipt | SHA-256 |
| --- | --- |
| Current Z10 and P10 link bounds | `c649e6ac787f67cafb27d310422415a6ed59488b09d6633ad1444891d72f5ce9` |
| Four paired Blood Bath save/load runs | `0096745b571d7c94c1bcf9ffeae9798f1b39cc3b12a72b42dd2d158f5160f187` |
| T4 and P10 physical sequences | `f041133e2fedd2e22fd626b2d5ccc9214d46793afef5621b9a38645c69fedaae` |
| Dense Zerg 7 comparison | `14a4d821be9bb50805541a0a5865a645b858a2b805ac2d0fe2520f8551e27748` |

## Borrow the route queue tail (November 16)

`CANONICAL_ROUTE_POINT_RETENTION=tail` replaces the dedicated plane with
the last 6,144 bytes of the existing 32 KiB route queue on donor-free
128×128 maps. The default remains `0`; `1` still uses the dedicated plane.
The build key keeps all three modes separate.

At most one logical slot references the borrowed tail. Before a full
packed queue or a save/load operation can overwrite it, the slot moves
back to the displaced pool backing and the parked key is forgotten. A
reset drops an invalid binding without copying. The two range scratch
planes keep their original addresses. Both wide linear writers cap their
queues at 4,470 entries so their padded byte grid stays below the tail.
The bounded monotone writer also stays below it; the old-grid writer
explicitly releases it. The save fence matters: the P10 carrier
candidate reaches byte 29,772, beyond the tail boundary at byte 26,624.

On release `7b099fc7`, all four baselines match the packaged ROM hashes.
All four candidate ROMs verify and pass unchanged ARM memory guards.
The variant adds eight EWRAM bytes on Terran, Z10 and P10 and none on
Showcase; it leaves 32 more IWRAM bytes before each guard. P10 has
7,952 bytes of EWRAM headroom.

The host ASan/UBSan differential covers 386,646 route queries with
matching logical state, including donor-free wide geometry. A forced
32-entry queue fallback verifies both fields against independent BFS
after the whole-queue writer reuses the tail. The live-field release
test clobbers the borrowed bytes after evacuation, and the range fixture
checks both reserved plane addresses across a claim.

On the same dense Zerg 7 window, the release and candidate match all
131 checkpoints, 480 sampled states and route counters, and the raw
world and packed route bytes at both endpoints.

| Guest cycle counter | Release | Scratch tail | Change |
| --- | ---: | ---: | ---: |
| Whole loop, active mean | 2,941,970 | 2,905,182 | −1.25% |
| Movement orders, active mean | 1,784,473 | 1,747,737 | −2.06% |
| Whole loop, all calls p95 | 4,420,530 | 4,229,326 | −4.32% |
| Whole loop, all calls maximum | 5,583,946 | 5,346,327 | −4.25% |

All four ordinary Blood Bath pairs match 21 result fields and both raw
SRAM files after a save and 180 more updates in fresh processes. At
the separate Zerg 7 save cut at call 14,071, slot 6 holds live bytes in
the borrowed tail before Save and no slot uses the tail after Save.
Both ROMs write the same 32 KiB save and match five raw world samples
over a fresh load and 180 neutral updates. A later save at call 48,237
also matches through 180 neutral updates. The historical input suffix
after that load diverges from its continuous run identically on both
ROMs. The separate T4 and P10 physical prefixes match but avoid no
floods. An independent source review at `e8f97ee` found no ownership
blocker across queue writers, range copies, map shape changes, and
aliases of the save union. It did not run a second replay. Keep the
mode opt-in pending timing on another dense wide map and checking
the facility FIFO composition.

| Local receipt | SHA-256 |
| --- | --- |
| Dense Zerg 7 comparison | `7b4a8b157aa814549fcc76e78cb5fa6c0c5607775471b37dfacc055325a57af1` |
| Four paired Blood Bath save/load runs | `28f9f69c9aefbba16ba2d364ff241ed4e7c56687331d07ea65e3f19a7e1cc639` |
| T4 and P10 physical sequences | `a7680f913722c6c9aee10a2caeb9dc4920f6a1184da112f0014c95cd0a5501ec` |
| Zerg 7 SRAM written with a live borrowed slot | `b5f6c43a303f32f02299d0ff980431b86815a7d095c70b86f05adcd40236a0e1` |
| Matching fresh-load world samples from that save | `108f9aedc07a98fc1632bd25be6aa4ede19433ac09eeb6b47c24df2ec75b3f0f` |

## Other wide missions and facility composition (November 16)

Both sides of each replay include the shipping coincident-collision
shortcut. The baseline Z10 ROM matches the packaged `5d4b085` hash.
The point counts below are reads at compiled branch addresses; the
wide-flood deficit matches the direct retained-return count at every
sampled checkpoint. Z9 records one additional outer point-wrapper
observation after call 39,000; use the wide-entry and retained-return
probes to count avoided floods.

| Replay | Physical calls | Historical / raw snapshot pairs | Wide fills, control → tail | Retained returns | Mean active loop | All-call p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Zerg 8 | 26,506 | 11 / 38 | 235 → 219 | 16 | −0.05% | −0.47% |
| Zerg 9 | 46,500 | 21 / 68 | 3,162 → 2,700 | 462 | −0.53% | −1.66% |
| Zerg 9, facility FIFO on both sides | 33,500 | 16 / 50 | 1,486 → 1,187 | 299 | −0.53% | −3.33% |

The final 500-call Z8 window has 14 retained hits and cuts mean active
loop cost by 1.63%. The earlier window with the largest historical
army has zero hits and nearly flat timing. The known 240-unit Z9 window
has ten hits and a 0.98% mean reduction. The post-hoc 500-call Z9
window with the most hits has 29 and cuts mean by 2.58%; it does not
represent the whole match.

The facility pairing runs 747 facility FIFO entries on each build,
including 351 entries when the candidate already has a live borrowed
field; no natural fallback occurs. Separate ASan/UBSan paired fixtures
force the facility FIFO to overflow after 32 entries. Normal facility
FIFO leaves the live tail bytes unchanged. The forced fallback
evacuates byte-identical live point data to the pool before writing
over the tail. All checked game state, logical route fields, and
sampled counters match apart from expected physical work counts.

All four current contact-enabled candidate links pass the original
RAM bounds; the EWRAM difference is +8 bytes or zero. In four paired
Blood Bath custom games, ordinary save at 600 updates and fresh load
followed by 180 more updates match all 21 recorded fields and both
SRAM files against the exact release. The full physical Protoss 10
replay matches 20 checkpoints, ten briefing pages, 14 authored pages
and 19 raw video/state samples through victory at frame 46,057.
Mean cost is effectively unchanged (1,726,522 → 1,726,528 cycles).
The switch remains opt-in.

| Local receipt | SHA-256 |
| --- | --- |
| Paired Z8 timing and state | `f603ff1d346d4994175f013fb33004aa1499dea37cba5c46432b2e00bcdfaac6` |
| Paired Z9 timing and state | `bbabfa3c84eb59ab6184186e2870c1e86a84ccedb628887b18d9abac2247cd4f` |
| Exact Z10/P10 link receipt | `b85cad19ed2558ab6d73b2bc593a32c56d5f9d876e3c60159afb87640daee6e0` |
| Paired Z9 facility timing and state | `68f11e6efe8b585228e5e0d1d3ea12c82044b2b257dd2cb89bc77c0f9b33cf4c` |
| Facility composition link receipt | `e4f43e7b7fdb0ec60e58778c126b19e95e224995fc5c0eb6f30cd0c26639205c` |
| Facility and forced-overflow host checks | `52944c04b0936c8cfdaa2df4f166f100dd5e515e4608baff552595958a10984d` |
| Four paired current Blood Bath saves | `fe6e6b47703cf585fb44c8074bd4bc6cd6310346514097ed7b60d32add2329dd` |
| Fully matched current Protoss 10 win | `81b68281e30040b0d12e47dc8bbec04aca503f90c03637052545f531e4bdf241` |

## Coexistence with the shipping small-map frontier

The existing tail switch also removed the facility frontier from the
compiled game. The two route users now share the scratch by shape.
Small-map facility continuation is active on 64×64. Point retention
can rotate the borrowed field on 128×128. If a shape change precedes
route layout configuration, a live or retained wide field blocks the
first small-map claim until the layout resets. The sanitizer regression
changes shape with a real 6 KiB field still attached to the wide tail,
checks its bytes, then resets the layout and claims the small scratch.

The current release at `e7daddb1` and the composed build run the same
ordinary-input Z7 tape from HOME through call 48,237. Both match all
131 named gameplay checks, every sampled frame and route counter, and
nine raw unit, building, mineral, palette and framebuffer cuts. The
endpoints have 236–237 live units. The 477 advancing calls improve as
follows. This is guest timing in mGBA.

| Guest cycles | Release | Composed tail | Change |
| --- | ---: | ---: | ---: |
| Mean advancing call | 2,511,018 | 2,481,146 | −1.19% |
| Mean advancing orders | 1,402,693 | 1,372,750 | −2.13% |
| All-call p95 | 3,979,392 | 3,628,357 | −8.82% |
| All-call maximum | 4,877,226 | 4,841,218 | −0.74% |

The four normal ROMs pass header, size, and the existing ARM memory
guards. Their 30 campaign launches and 72 skirmish setups match the
current release matrix, and all eight custom save transfers match.
The six older Z7–Z9 and P7–P9 recipes also link within the original
guards. Regular three-AI Blood Bath matches all 25 raw world, route and
video cuts and the fresh SRAM continuation. Its active mean changes
from 802,552 to 802,530 cycles. The busy P7 sample also matches all 11
world and display cuts and the frame, route, and terrain timeline. Its
mean changes from 1,883,235 to 1,883,264 cycles.

The shared build now defaults to `CANONICAL_ROUTE_POINT_RETENTION=tail`.
Use `CANONICAL_ROUTE_POINT_RETENTION=0` for the earlier route path.
The largest qualification gain is lower dense-update p95. Physical
hardware timing and longer cases on other wide maps remain open.

| Local receipt | SHA-256 |
| --- | --- |
| Current dense replay | `3ddec50626ae082029cf9258c1af769b0066b9fa70321ed00616fd6b48d01fbb` |
| Three-AI Blood Bath and SRAM | `1c7e9894f3fbb9cd39bbaffc7c1292a92a0554b0357a4d27a1f3340fce991cb2` |
| Launch matrix | `f8644030ebaa75ae0ea73e128b591e09757f1b6367c5bd96da5cf44bf7e86922` |
| Bidirectional saves | `d4028523e6fff80e9f67908805ba38c415a7721265b314d296f0c7ce42d90d3d` |
