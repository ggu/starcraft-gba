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
