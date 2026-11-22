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

The byte lookup, originally opt-in via `CANONICAL_ROUTE_LINEAR_GRID_LUT8=1`,
expands each packed walkability byte with one 2 KiB ROM table. The entry stores the same two
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
the adjacent words with `ldmia` and `stmia`.

[The isolated loop benchmark](../../tools/sc64_route_grid_loop_bench.py)
relocates those exact emitted instructions and their authored tables
into micro ROMs. It uses the game's wait-state register value, brackets
each loop with the read-only mGBA observer and compares every output
byte with an independent bit pattern. It does not boot either game.

| Packed input | Stock cycles | Byte lookup cycles |
| --- | ---: | ---: |
| Wide, 2,048 all-open bytes | 106,490 | 69,626 |
| Wide, 2,048 mixed bytes, either run order | 106,490 | 69,626 |
| Midwidth, 1,536 mixed bytes | 79,866 | 52,218 |

The isolated loop uses 34.6% fewer emulated cycles. The result says
nothing about seeding, traversal, packing, rendering or complete frame
rate. Reproduce with the pinned ROM pair above and physical observer:

```sh
python3 tools/sc64_route_grid_loop_bench.py \
  --stock STOCK.gba --candidate CANDIDATE.gba --runner OBSERVER --out NEW_DIR
```

The receipt SHA-256 is
`f321c41740e811cdebc1717b560f81eaecf7f34b145fbeeb969dd23e0fb2744d`.

### Paired gameplay on the exact dense Z7 release

The physical-key replay on the same release and candidate digests
matched all 131 named checkpoints and all 480 sampled call indices,
simulation frames and route counters. Both endpoints also matched
the resident route fields, walkability, cache identities and ages,
and the raw unit, building and map-state bytes. The replay executed
477 active calls in the timed window.

| Guest cycles, active calls | Packaged release | Byte lookup | Change |
| --- | ---: | ---: | ---: |
| Complete update, mean | 2,640,646 | 2,603,901 | −1.39% |
| Complete update, p95 | 4,201,699 | 4,084,306 | −2.79% |
| Complete update, maximum (call 47,823) | 5,174,688 | 5,027,328 | −2.85% |
| Orders, mean | 1,483,645 | 1,447,017 | −2.47% |
| Orders, p95 | 3,061,150 | 2,945,794 | −3.77% |
| Orders, maximum (call 47,855) | 4,004,291 | 3,855,167 | −3.72% |

The [isolated-loop](#cut-the-wide-grid-setup-cost-before-changing-admission)
and gameplay results measure different scopes. The complete frame
still exceeds the 30 fps target in this workload. The paired receipt SHA-256 is
`b85c153ff69db8205ad08ca88e79690656ac4bb2c9fda1a1cd51e40813fcf52e`.

### Paired Z9 run with both changes

Combine the byte lookup with the experimental cold-prefix dense FIFO
continuation. The same physical Z9 replay stops at call 22,000. Across
the release, cold-prefix-only build and combined build, all 12 named
checkpoints and 21,987 call IDs, physical inputs and game frames match.
All 34 raw world snapshots and their observed cache metadata, along
with 317 completed route fields, match the release. The combined and
cold-prefix-only builds also match the 53 sampled pending fields and
all route-event counts exactly.

| Guest cycles, 979 active updates in calls 21,001–22,000 | Packaged release | Byte lookup only | Cold prefix only | Both |
| --- | ---: | ---: | ---: | ---: |
| Complete update, mean | 1,725,542 | 1,720,501 | 1,711,820 | 1,704,440 |
| Complete update, p95 | 4,355,658 | 4,255,649 | 3,863,254 | 3,774,253 |
| Complete update, maximum (call 21,879) | 23,327,324 | 22,395,567 | 23,793,795 | 22,892,902 |
| Orders at call 21,879 | 21,653,977 | 20,722,301 | 22,099,797 | 21,198,917 |

The combined build uses 900,893 fewer cycles than cold prefix alone at
the peak, and 434,422 fewer than the release. Relative to the release,
the active mean improves by 1.22%, p95 by 13.35%, and the peak by 1.86%.
The combined link passes the Z10 RAM checks (EWRAM end `0x02038f18`,
IWRAM end `0x03004d80`). The byte lookup alone also matches all 12
checkpoints, all 21,987 call and frame IDs and all 34 raw snapshots with
370 field observations, their residency and route counters. It cuts the
peak by 3.99% against release, compared with 1.86% for the combined
build. Adding the cold prefix lifts the byte lookup's peak by 497,335
cycles while cutting p95 by 481,396. Both peaks still exceed 22 million
guest cycles. The cold prefix still needs a cheaper decline. The
standalone lookup is the one qualified below; it does not depend on
the cold prefix.

| Local artifact | SHA-256 |
| --- | --- |
| Combined Z10 replay ROM | `1bd96a61205f981d70ee81535ba010a11432f9206c6c6c7b45c818bf20bbb281` |
| Matched checkpoints | `f27de8d6bf00393e4105527374623a6f37183e93d2e8e230a4c6b0eba083f971` |
| Three-way comparison | `dabacc77e8b0f3a3e3939fe606ed41350afef2c4eacb1a1bd76ab52d4cb8eb1e` |
| Comparison including lookup alone | `461439d9eca6d9351d33a7b294d9ff8d99591f9df0bb6798985e8ee284062b74` |

### Enable the standalone lookup in regular builds

Regular Terran 10, Zerg 10, Protoss 10 and Showcase now select the byte
lookup for wide linear grids. All four flags select the same implementation;
the 64-column path and builds without either linear or facility FIFO
are unaffected. `CANONICAL_ROUTE_LINEAR_GRID_LUT8=0` selects the earlier
nibble code and configuration key. Cold-prefix point fills remain disabled.

All four links pass the header and RAM guards. EWRAM and IWRAM ends match
their corresponding `cb210bc` packages exactly.

| Product | EWRAM end | IWRAM end | Lookup ROM SHA-256 |
| --- | --- | --- | --- |
| Terran 10 | `0x02039c70` | `0x030048c8` | `cc68120edf8bf0c0b85d93b7e9a8759778c36025100360dc57409c7d45f74529` |
| Zerg 10 | `0x02038b18` | `0x03004d70` | `f0d4ae5395cd4ed9fb1eeb6200891a7973f4e53196103e187b265848fe29450f` |
| Protoss 10 | `0x0203c0f8` | `0x03004c80` | `69edd2cf2893b784c01fbe1cd5a55abd9fe6a70ca005b450e345ee9b6bd182d6` |
| Showcase | `0x02039c34` | `0x03004ed0` | `ccaa72dcc4c09d094d82ad42c515dc37a126fe3be5a1147f234f1284ab2685da` |

Eight physical-key skirmish pairs boot from the menu, two per product,
covering Outpost and Blood Bath, every human faction and one to three
opponents, with named, Random and Closed seats in the sample. Each
restores frame 601 in a fresh emulator process at title L, then matches
the watched uninterrupted world at frame 781. All 21 watched fields
and both 32 KiB SRAM exports match between the packaged and lookup builds.

Both Protoss 10 builds reach the same victory in 47,194 input calls, with
20 matching world checkpoints, all 10 briefing pages and 14 authored
dialogue pages. The full emulated mean changes by −0.005% and the peak by
+0.003%; no practical change there. The dense Zerg samples above see a modest
benefit. These runs do not establish 30 fps on the GBA SP or cover all lobby
combinations and arbitrary mid-battle saves.

| Local receipt | SHA-256 |
| --- | --- |
| Eight skirmish comparisons | `85e78368c9c6be7ba1f9472af50ec7e2203e7a7234275a5af8c2dee0fd25dd6c` |
| Protoss 10 victory comparison | `4f85ff68fda54ef92285f71cb7abc302ed2e0ccf7e265462843905da2a7ee94b` |

## Cached range read on a compressed wide grid

The opt-in experiment retains the endpoints of identical row and column
runs, plus the origin and goal axes. Edges retain their original lengths.
Dijkstra computes distances from the origin on graphs of at most 128
intersections. A descent from the goal in left, right, up, down order
matches the reverse FIFO's choice of parent. In an identical-column run
a vertical step can move after a horizontal step, and in an identical-row
run a horizontal step can move before a vertical step. These exchanges
put the first shortest path in FIFO order on retained coordinates.

The host oracle matches 2,084 queries, including 164 disconnected
endpoints. Blocked endpoints and excess intersections take the existing
path. The integration fixture checks that the cached read preserves the
pending frontier, packed field, logical counters and route scratch tail.

The same-source forced P10 pair, composed with release `0f4d63c4`, uses
the existing long wall fixture. It queries a field of 16,270 reachable
tiles using 77 intersections. The two reads return `(1608, 1624)`, with one
logical build and zero hits. RAM endpoints match.

| Work in ARM fixture | Prefix control | Compressed query |
| --- | ---: | ---: |
| Initial prefix fill | 103,402 | 103,402 |
| Cached range read | 1,086,782 | 252,139 |
| Later explicit full-field read | 271 | 1,085,534 |

The immediate read saves 834,643 cycles (76.8%). The experiment leaves the
prefix parked, and the observer verifies byte-identical pending, field,
walkability and tail data before explicit completion. Afterwards all
6,144 packed bytes equal the control and all 16,384 directions equal the
independent FIFO. The later read shows that completion can still cause a
stall. A parked slot also changes the available prefix budget, witness
selection and retention on eviction.

The ordinary-button Z9 probe samples 86 live 128-by-128 grids on Z10
prefix-preview ROM `f38cdb2f` through controller call 31,500. All 31
named checks in that window match. The samples contain 20 distinct
grids. Each requires retaining all 128 rows and columns, for 16,384
intersections before adding query endpoints. None could use the shortcut.
Keep global axis compression out of production. The next route design
should measure on real terrain and cover the exact and nearby-root
completion stalls seen in dense Z7.

| Local receipt | SHA-256 |
| --- | --- |
| Forced P10 comparison | `faba21247030427ffc0597ec1fe42b13099ccf66bbf91dfa7d896cd03a8339e9` |
| Z9 walkability sample | `b9298d8e5fa0ec179d07bece2e2c6e3abeec614de8429730be1fd5b149e3aff6` |


## Z8 check on the current mineral-routing release

The route code from #576 at `f556e9ba` was composed locally with release
`916bb897`. The Z10 trial enabled point prefixes, the component and
distance witnesses, and the second exact-hit cutoff. Prepared inputs
and controller tape are identical to the release control. All 47
world checkpoints, 24 two-page video samples and 19 HOST checks match.
The frame, screen and dialogue timeline matches across all 46,487 calls.
The paired replay is uninterrupted.

| Metric | Control cycles | Point prefix cycles |
| --- | ---: | ---: |
| Mean advancing play update | 1,020,920 | 1,016,666 |
| p95 advancing play update | 1,500,353 | 1,470,299 |
| Peak advancing play update | 12,264,221 | 11,052,465 |
| Call 33,800 entire observer interval | 12,264,714 | 11,052,958 |
| Calls 33,889–33,896 mean observer interval | 3,281,773 | 2,517,638 |

The mean gain on this full route is 0.42%. The 33,800 movement spike
is 9.9% shorter. The route and reserved range-plane sanitizer checks
pass. Z10 IWRAM ends at `0x03004ec8` and EWRAM at `0x02038328`, both
within their guards. Terran, P10 and Showcase also pass the current
five-flag links. All four profiles build successfully with 72 bytes or
more below the IWRAM ceiling. Three-AI Blood Bath smoke pairs write the
same pause journals and match the named world 240 updates past a fresh
HOME load. The cross-profile build and save receipts have SHA-256
`13eb58c2fa4379f1dc2934b9f16ddbd32853c2283ac092a47d3e47fc6c9e0608` and
`2c69b7a22387a51834fa9226d5ee1aad0e29673dd5ee00c2117237ed8d94d911`.

The current forced P10 long-wall ARM fixture returns `(1608, 1624)` and
records one logical build and zero hits with either variant. The release
fills the whole field in 1,087,127 cycles and reads the cached step in
1,228 cycles. The five-flag preview parks the field in 103,404 cycles,
then spends 1,086,880 completing it on the cached read. This reproduces
the deferred range stall on the current code. The result SHA-256 is
`0dbd4113fd903e1e5d91ff4d23fbfa794d83272ead9c2272e068b94e55e80861`.
The second exact-hit flag still needs a normal selector entry and the
cached-read cost needs a solution that can apply on real terrain.

The trial ROM SHA-256 is
`3b9e27ac57008b2a852a7cffc44802c252d85115289717ce810b9f6c570f78a6`.
The replay receipt SHA-256 is
`c78c41503fe8c112faa45ad0beef65808bffbb59e79394e56cb02f9e19dcc839`.
The paired comparison SHA-256 is
`ae04bf3920fc5f7166fef435b82ea8d2dcdb43ff9df95b5f6d67ddfef185d3e5`.
