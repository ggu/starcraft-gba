# Dense Zerg 7 movement cost beyond point floods

A read-only instruction trace on the exact released Z10 census ROM
matches all 131 named gameplay checkpoints through call 48,237. It
samples 15 calls in the established dense window (47,758–48,237).
For each observed point-flood count (zero through four), take the
10th, 50th and 90th percentiles by order-stage cost from the earlier
pinned run. This deliberately includes expensive calls; the 15 samples
are not a mean for the full 480 calls. Per-sample instruction and call
stack cycle totals match the emulator clock, with no unresolved events.

| Sample group | Count | Order-stage mean | Collision-mask uncached inclusive mean |
| --- | ---: | ---: | ---: |
| Zero point fills | 3 | 1,048,335 | 524,012 |
| One to four point fills | 12 | 3,005,697 | 473,573 |

The named mask function's own instructions account for 506,708 cycles
per no-fill sample. Its caller's memo and wrappers bring the combined
mask path to 539,392 cycles. Most of this cost is inside the scan even
when a return from a cached route avoids all point expansion. Other
repeatable costs include building the target index (about 78k inclusive)
and its nearest-target work (about 68k inclusive) in the no-fill samples.
Treat overlapping inclusive scopes separately.

The current Z10 lookup already uses the exact generated contact mask
table and its four-entry memo. The next candidate should reduce the
candidate scan cost while preserving collidable classes, movement bounds,
zero-speed queries, burrow and cargo semantics. Moving the entire mask
to IWRAM does not fit the current Z10 guard: the ROM function is 1,048
bytes and the available placement margin is 688 bytes. Prefer less work
per candidate or a small hot loop; measure on the same window and verify
against the existing differential and all four cartridge guards.

- Released Z10 ROM SHA-256: `8e4737c9085dda0ed6f6084ed6f3150ae7dfd722dd97e466f766d6c6bad20efb`.
- Local instruction and call-stack receipt SHA-256: `92943604967ce17f7c8d14c233732b584b317a6d1ba8f4c3a9ca0c13f48ad3d8`.

## Candidate scan experiment

Hoisting the mover coordinates and row stride, combining the axis
checks and postponing the burrow query passed the existing contact
oracles on the Zerg and Protoss vectors (637,282 and 636,892 mask
queries, plus the step, slot mutation and memo checks). The exact
current Z10 comparison matches 131 checkpoints, 480 frame and route
count samples and raw route/world endpoints. It lowers average active
loop cycles only 0.19%, from 2,941,970 to 2,936,433; order cost drops
0.30% and maximum rises slightly (5,583,946 → 5,584,766). Do not enable
this experiment. Measure how many empty buckets and wrong-layer actors
are visited before trying to prune candidates earlier, and account for
bucket changes on same-cell moves and transport transitions.

- Candidate dense replay SHA-256: `505bb098e2aa34e81eacb3014eb4fb0c3a3be6aef9b85143d626d87fe8f72ffa`.
- Matching raw endpoint receipt SHA-256: `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4`.

## Exact-position crowds (November 16)

A diagnostic Z10 build counts every visited bucket and classifies each
candidate without changing the decisions. It matches 131 named checks
and all 480 sampled frames and route counters on the released tape.
The 477 advancing updates visit 31,608 buckets, of which 11,422 are
empty. But even after excluding empty cells, the scan visits 964,535
actors: 422,477 at the mover's exact coordinates (43.8%), 389,531
outside the relevant axis interval, and none from the wrong flight
layer in this sample. Only 142,897 reach the full geometry check.

A word-at-a-time empty bucket scan matches the same gameplay and raw
endpoints, but saves only 0.33% mean. It also needs 416 extra IWRAM
bytes in the tested P10 build. Do not enable it.

The compact Unit layout already puts signed X and Y in consecutive
halfwords. The shipping candidate compares them together before actor
facts and cargo lookups. A positive steering offset increases squared
distance from an actor at the origin; neither the stock nor the reference
non-strict query can block that step. Zero and out-of-range speeds keep
the original path. The code checks the alignment and field layout at
compile time. No new persistent state or EWRAM is used.

| Dense Z7 metric | Released | Candidate | Change |
| --- | ---: | ---: | ---: |
| Mean active loop cycles | 2,941,970 | 2,824,509 | −3.99% |
| Mean active order cycles | 1,784,473 | 1,667,932 | −6.53% |
| All-call p95 cycles | 4,420,530 | 4,317,436 | −2.33% |
| All-call peak cycles | 5,583,946 | 5,440,259 | −2.57% |

All 131 checkpoints, 480 frame and route samples and raw route and world
endpoints match. The dense mean is about 5.94 emulated updates/s, still
below the frame-rate target. Two sanitizer variants pass more than 1.27M
mask comparisons combined and a specific stacked-Marine check for
zero speed, steps of 1–14 and a one-pixel move within the same bucket.
All four normal cartridge vectors pass their RAM guards (IWRAM grows
80 bytes in P10 and Showcase; the other two do not grow).

Four paired Blood Bath skirmishes match all 21 tested fields and both
SRAM exports per product after ordinary save, fresh-process load, and
180 further updates. Paired Terran 4 and Protoss 10 input sequences
match 12 world, video and route samples each. In a separate full P10
replay, 20 campaign checks, 10 briefing pages, 14 authored pages and 19
raw video/state checks match through victory at frame 46,057. Mean P10
cycles rise by 0.22% (1,722,799 to 1,726,522); the larger improvement is
specific to the crowded Z7 window. Use `CANONICAL_CONTACT_COINCIDENT=0`
to rebuild the unchanged prior comparison vector.

| Local receipt | SHA-256 |
| --- | --- |
| Candidate classification counts | `0a3fd7401ea5cdfed2e9c15cadacf32d086f83465621e792c26c582bdf0a4bbf` |
| Empty bucket word scan | `8bec63f898e99670ace303ef2d3d839d507024ac050126bca1a05d34c5f934be` |
| Coincident-coordinate comparison | `fc8fd166fe91c4b74ea70d4c779c1c92bd1ab9e748826dc8c8ab3ee21a020be1` |
| Four paired Blood Bath save/load cases | `f878ed928cd0e0a7e87b71edd1dbedeec13148cef705e39d396b5e5f92f1063f` |
| T4 and P10 gameplay comparisons | `eebae1bf87620c5f6255afb52cc8e9e4dcfb0b85907cebe0ca91c17c380d8972` |
| Full P10 win comparison | `f2da9241d836aff4c23919c55cd78e2bdf9e6112053666db0db1c6013f587598` |
