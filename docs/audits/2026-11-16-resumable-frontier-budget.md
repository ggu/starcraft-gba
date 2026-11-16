# Work budget for resumable route fields

Read-only register and memory attribution on the exact `5d4b085` Z10
cartridge matches named gameplay checks and every sampled frame, total
and order counter against the independent release runs. Host reconstruction
in canonical seed and neighbor order matches all observed ARM pop counts.

| Release workload | Named checks | Wide fills / pops | Setup / expand / pack cycles per update | Share of total loop | First / farthest observed queried origin at percent of all pops | Pending frontier p95 / max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Zerg 7, calls 47,758–48,237 | 131 | 448 / 4,312,000 | 110,853 / 485,886 / 134,247 | 25.95% | 38.76% / 38.82% | 41 / 52 |
| Zerg 9, calls 31,001–31,500 | 15 | 56 / 614,044 | 13,380 / 66,794 / 16,140 | 4.95% | 2.37% / 2.42% | 38 / 52 |

With the first-origin check every 32 pops, a cutoff at 1,024 pops and
a 128-cell frontier, 285 of 448 Z7 fills and 54 of 56 Z9 fills would
qualify for early pause. The expansion-only upper bound is 10.54% of
the sampled Z7 loop and 3.29% of the Z9 loop. This assumes the measured
pop rate, ignores staging and field access cost, and cannot rule out
additional readers or demand after the window. It is not a measured
speedup or a hardware result.

The prior partial-fill restart spiked worst orders; skipping logical
insertion changed subsequent range moves; deferring and restarting full
work did not reduce mean cost. A new probe should retain discovered
directions and the ordered pending FIFO, publish the same logical
slot and age, and resume without clearing the field. At the 1,024-pop
admission cutoff the existing wide queue still represents all discovered
cells, so those cells can be packed without scanning the whole plane.
Start with one pending field on donor-free wide geometry. Audit direct
range and save readers, blocked/unreachable starts, topology changes,
and scratch ownership before comparing authoritative ordinary replays.

A separate render probe on the same Z7 release tests adding the existing
fog row-mask and cached minimap raster paths individually. Each matches
131 paired raw framebuffer/palette/fog observations and the raw state
endpoints. Row masks cost 0.12% more per active update; the persistent
minimap reduces HUD cycles 11.64% but saves only 0.62% overall in this
window. Neither change was enabled. Prioritize reducing route work
over these rendering switches.

| Local receipt | SHA-256 |
| --- | --- |
| Zerg 7 route attribution | `31ad233e50a616836fd60cef7ec3b5ff4736695dfc1219b144ffd8518c9e1eab` |
| Zerg 9 route attribution | `acacbeaeef21465c76520b5b901a848c641786e87909c7dd48b474abcc6cbd98` |
| Paired Zerg render checks | `e21b768f69d2edc4064a8ab142e22d0a09420c6b249712be4073f398e4f3bf70` |

## Paired Z9 prototype: continue a declined search in place

The cold-prefix probe starts in the normal padded grid. If the query cannot
park within its budget, it continues from the same ordered frontier without
clearing or reseeding. Sanitizer tests match the independent FIFO oracle
across parked, declined and forced-overflow paths. The ARM guards pass for
Z10 and P10, and opt-out Z10 is byte-identical to the release.

On the pinned Z9 prefix, all 12 historical checkpoints, 34 raw observations,
317 completed route-field pairs and 21,987 sampled frame IDs match.

| Guest cycles | Exact release | Prior probe | Continue in place |
| --- | ---: | ---: | ---: |
| Worst loop | 23,327,324 | 27,816,467 | 23,793,795 |
| Busy interval, active mean | 1,725,542 | 1,733,404 | 1,711,820 |
| Busy interval, active p95 | 4,355,658 | 4,235,409 | 3,863,254 |

The busy p95 is 11.3% lower than the release, but the worst call still costs
2.0% more. Its 19 cold searches all declined parking and continued in the
same grid. None restored a previously parked field. Leave the probe off;
reduce the extra cost of declines before considering a broader rollout.
These are guest cycle counts, not GBA SP frame-rate measurements.

| Local receipt | SHA-256 |
| --- | --- |
| Paired Z9 prefix | `9cee2e36562a3a4e68e689c67b0fc2d506fa8a960541a926f097c8cd37bb299c` |
| Peak-only attribution | `7fee9ae77e6ce1104b375e9fc40f9b83f85f8c73d75d51700e56437a7b7c9763` |

## Cut the wide grid setup cost before changing admission

The optional `CANONICAL_ROUTE_LINEAR_GRID_LUT8=1` expands each packed
walkability byte with one 2 KiB ROM table. The entry stores the same two
words that the two nibble lookups generate. The 64-column dedicated path,
border values, queue bounds, seed ordering and field packing stay unchanged.
The wide setup shared by point and optional depot floods uses the table.
The configuration key includes the switch so a stale build cannot reuse
the other route compiler flags.

The prepared 96- and 128-column HOST route oracle and forced-overflow
fallback have identical field, cache and residency receipts. A Z10 ARM
build with the option off is byte-identical to packaged release
`2b907cdecb5769dde471de04b280ffe7de79a3b43874b2eaabfc2a9b8be43eb8`.
Both links end at EWRAM `0x02038b18` and IWRAM `0x03004d70`.
The optional ROM is
`f0d4ae5395cd4ed9fb1eeb6200891a7973f4e53196103e187b265848fe29450f`.

In the emitted Z10 Thumb code the inner grid setup loop goes from
14 instructions to eight per packed byte. The new loop loads and stores
the adjacent words with `ldmia` and `stmia`. This is an instruction
count, not a cycle or frame-rate measurement: ROM and EWRAM access timing
also matter. The comparison against the exact dense Z7 release reached
the disk floor before its timed window. Leave the switch off until a
paired replay matches all checkpoints and route bytes and measures the
mean and worst calls.
