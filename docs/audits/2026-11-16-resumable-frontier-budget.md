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

## Wide point prefix on the current dense Z7 replay

The opt-in point-prefix prototype pauses a wide flood after the requested
origin is discovered. Four ordered pending queues retain up to 128 cells
each. The queue restores the exact assignments when a later reader needs
more of the field. Logical insertions, ages and route counters keep the
previous order. Saturation, blocked origins and unsupported layouts use the
complete flood. Topology reset discards the parked work.

The host sanitizer pair matches 386,646 route queries and the full logical
residency stream. Focused oracles check all 16,384 directions after resume,
blocked seeds, four simultaneous queues, forced overflow, the range
reader, and fixed range scratch bindings. The residency receipt SHA-256 is
`829634c7cb6709f6cff3745a23427180ef5d283cef158355d08d7386e849f516`.

On Z10, `SC64_ROUTE_POINT_PREFIX_PROBE` with point-tail retention off
matches all 131 ordinary HOME checkpoints, all 480 frame and route
samples, and nine raw world and display samples through call 48,237.
The comparison includes the separately measured shipping tail for context.

| Guest cycles | `e7daddb1` control | Shipping tail | Point prefix |
| --- | ---: | ---: | ---: |
| Mean advancing call | 2,511,018 | 2,481,146 | 2,282,745 |
| Mean advancing orders | 1,402,693 | 1,372,750 | 1,172,333 |
| All-call p95 | 3,979,392 | 3,628,357 | 3,384,700 |
| All-call maximum | 4,877,226 | 4,841,218 | 5,253,274 |

The prefix cuts mean cost by 9.09% and p95 by 14.94% against `e7daddb1`.
The maximum rises 7.71%. At call 47,995, game frame 46,734, orders cost
251,543 fewer cycles than the no-tail control, while work outside that
orders counter grows by 877,724 cycles. Trace that call by stage before
changing the frontier. Call 48,100 adds 1,021,162 cycles against the
shipping tail, mostly within orders. Both ROMs count two builds and
three hits on that call. Trace both paths before enabling the prefix.

The Z10 ARM link stays within its original guards at `0x03004e28` IWRAM
and `0x02038d10` EWRAM. The probe stays experimental pending other profile
links, save/load and timing on other wide workloads. These are guest-cycle
results, with about 7.35 emulated updates per second in this dense sample.

- Point-prefix comparison SHA-256 `160d75b965e5f6623f64087a12ccb41cffb6f4359b63ea79d76e7b0f8c35e617`.

### Where the two wide-prefix spikes occur

Read-only breakpoints on the exact shipping tail and first prefix ROMs
reproduce the recorded loop costs and every sampled world check. At call
47,995, an ordinary A press issues 37 formation reachability checks.
The prefix's first completion runs between two reads from the first
formation pair. `update_orders` begins 871,003 cycles later with the
prefix. The render interval is effectively flat.

At call 48,100 there is no formation work. The shipping tail does one
wide linear fill and one verified cached-field adoption. The first
prefix ROM enters its completion helper during orders. Orders cost
1,026,713 cycles more even though both runs report two logical builds
and three hits for that call. A logical build alone does not identify
physical route work.

| Call and counter | Shipping tail | First point prefix |
| --- | ---: | ---: |
| 47,995 start to orders entry | 253,979 | 1,124,982 |
| 47,995 game update | 3,866,299 | 4,481,493 |
| 47,995 draw | 774,479 | 771,395 |
| 48,100 orders | 1,487,649 | 2,514,362 |
| 48,100 draw | 505,888 | 506,299 |

The component-witness variant answers boolean reachability from up to
four complete, fresh route fields. The host oracle covers a positive
proof, a negative proof, and exact fallback with the same cache ages.
On ARM it moves the first completion after `update_orders` begins in
call 47,995. The full step costs 5,286,590 cycles, 33,316 more than the
first prefix ROM. An orders-stage reader still enters completion in that
same update. Test a second bounded cutoff for exact cell requests.

- Stage and entry comparison SHA-256 `002edc7270d4a83e459740c378ef64e7d5a9a6c1c51c5159d6dc1e9d2174f931`.

### Keep the complete tail alongside parked prefixes

A separate composition restores shipping tail reuse for complete fields
while allowing new point prefixes. Only complete victims can enter the
saved tail. A hit installs the complete field and retires the displaced
prefix record, if present. The existing large-queue and save fences still
release the borrowed backing. The focused host fixtures cover both exchanges,
forced overflow and completing a field after its backing has moved.

The host sanitizer pair matches the 386,646 logical residency samples
for the shipping tail and combined build. Started physical fills differ
by six in the tail-plus-prefix host pair. The physical ROM samples below
match every named checkpoint, sampled actor/mineral bank, palette and
framebuffer, and all sampled route counters.

| Guest cycles | Shipping tail | Tail and prefix | Tail, prefix, and component proof |
| --- | ---: | ---: | ---: |
| Mean advancing call | 2,481,146 | 2,271,542 | 2,271,609 |
| All-call p95 | 3,628,357 | 3,340,809 | 3,339,859 |
| All-call maximum | 4,841,218 | 5,271,133 | 5,310,998 |
| Call 47,995 | 4,641,164 | 5,271,133 | 5,310,998 |
| Call 48,100 | 2,552,639 | 2,575,496 | 2,574,898 |

Both composed variants avoid entering prefix completion on call 48,100.
Tail plus prefix lowers mean cost by 8.45% and p95 by 7.92% against the
shipping build. Call 47,995 still needs a continuation. The component
proof relocates it into orders and grows the maximum by a further 39,865
cycles. Keep the options off in release and prioritize the resumed search
budget for that call before cross-profile qualification.

| Local receipt | SHA-256 |
| --- | --- |
| Verified shipping tail adoption | `8a56b15d58a8060bf6b879de3875ecde9eef1ff7d5861fb734371c959c795d4b` |
| Component and first prefix | `68327ecfa5d551c9480335b3fdc4da59479ea1cb7473b726e1ce457be208a887` |
| Tail and prefix | `bcddd1aa4b372beef7172ce6f1daa4d55f2713ab81d3045c221c929803a41ca7` |
| Tail, prefix, and component proof | `6e5c7ccf7a189719fab42601caea7280a9af61899606e120dfc058ee2e8536c4` |

### A second bounded cutoff makes the tail heavier

The demand prototype allows a resumed search to park once more after
discovering a newly requested open cell. It writes only newly discovered
steps and retains the remaining FIFO. The host sanitizer comparison
matches 386,646 checks against the combined control, including the range
reader, forced overflow, and a later full completion.

The ARM sample averages 2,219,308 cycles, down 10.55% against the shipping
tail. Its p95 is 3,011,967, down 16.99%, and its maximum rises to 5,583,455,
up 15.33%. Call 47,995 takes all of that maximum. The read-only snapshots
show the point field rooted at cell 6,161 park again during orders. A
register-only observer reproduces the timing and state exactly and
identifies the requests. Its first resume comes from the nearby-root
test, which needs cell 445 from the field rooted at 12,730, 99 Manhattan
steps away. The later two requests both use the field rooted at 6,161.
The cutoff for cell 6,300 reparks the field, then a nearby-root request
for diagonally adjacent cell 6,427 enters the resume path again. An exact
early rejection for the first distant scan is the larger opportunity.
A small halo around the cutoff is a separate test for the repeated
nearby requests.

All 131 named world checkpoints, all 480 frame/route entries, and nine
actor/resource and palette cuts match. The bytes of both 240×160 frame
pages match at all nine cuts. The selected display page differs at the
last cut, so the stricter presentation check remains open. Keep the
demand prototype local while attributing the repeat requests.

- Demand prototype ARM receipt SHA-256 `dbe4ff97e391f8225d911cb1fa3865275a00366e5a2153c70381218c0b465378`.
- Read-only request and caller receipt SHA-256 `c85d50ffdff7b1b8d45c86f87b923807ad366ab83d4c6418e19d4d158fc2e47f`.

### Prove the distant lookup is disconnected

The exact live walkability grid and parked-field metadata at call 47,995
identify two nearby-root requests. A separate L/R/U/D reference flood
from cell 12,730 visits 9,625 cells and cannot reach origin 445. Six
fresh, complete open-root fields reach the former cell and exclude the
latter. The runtime now uses that existing component proof before it
completes a parked nearby-root candidate. Selection order, the two
candidate budget, cache ages, and counters follow the existing path.

The 480-call Z7 replay matches all 131 gameplay checks, frame and route
timelines, nine raw world cuts, and both video pages with their selected
page. The original group-move spike falls from 5,310,998 to 4,445,672
cycles compared with the component-witness composition, below the
shipping cost of 4,641,164 for that call.

| Guest cycles | Shipping tail | Nearby-root rejection |
| --- | ---: | ---: |
| Mean advancing call | 2,481,146 | 2,250,150 |
| All-call p95 | 3,628,357 | 2,993,653 |
| All-call maximum | 4,841,218 | 4,873,873 |

The average falls 9.31% and p95 falls 17.49%. The maximum rises 0.67%
and moves to call 47,823, where orders cost 733,859 cycles more than the
shipping ROM on the same call. The dense rate is 7.46 emulated updates
per second. The first and second known spikes no longer coincide with
the maximum.

The regular Blood Bath Z10 pair matches all 25 world, route, video and
SRAM cuts through a fresh-process continuation. Mean changes from
802,530 to 802,657 cycles. All four configured ROMs pass the original
RAM guards. Showcase keeps prefix expansion in ROM to preserve the
existing IWRAM stack reservation.

The configured builds match all 30 campaign launches, 72 skirmish setups
and eight ordinary SRAM transfers. The 958-update busy P7 sample matches
11 world and display cuts and its complete route and terrain timeline.
Its mean changes from 1,883,264 to 1,883,899 cycles, and its maximum rises
from 2,554,931 to 2,583,019.

An independent packed-continuation experiment starts the second cutoff
directly in the packed field before dense setup. Matching host receipts
cover 386,646 queries and 288 complete packed-field and pending FIFO
comparisons. Its ARM mean improves only 0.075% versus the dense second
cutoff, p95 remains flat, the maximum rises another 40,635 cycles, and
the selected page differs at one cut. Keep this variant local.

| Local receipt | SHA-256 |
| --- | --- |
| Live nearby-root geometry and independent route checks | `38cd99a334313d09a3961a17ba48a2ac8eaf377ef617be9d1200d7ea3140ba9b` |
| Nearby-root rejection in Z7 | `dfadc48446af0721cf726590d7d6f016c31d519d688c182c4726909e8317b83c` |
| Nearby-root rejection in three-AI Blood Bath and SRAM | `448fbb1b39db0e9e181877a924d198454159778e868a88fac40f0fbeff58c426` |
| Campaign and skirmish launch matrix | `f8644030ebaa75ae0ea73e128b591e09757f1b6367c5bd96da5cf44bf7e86922` |
| Bidirectional SRAM matrix | `d4028523e6fff80e9f67908805ba38c415a7721265b314d296f0c7ce42d90d3d` |
| Packed second-cutoff replay | `f3ec65f364bd59859fac7d8b0266c169af40ecaafbd18c3423d8e564f2d50c12` |

### Reject a connected lookup with exact distance bounds

The second live request can reach the cached root, though its preferred
route misses the newly requested goal. The captured complete point field
rooted at cell 6,033 gives a 42-step upper bound from its one-tile
neighbor, the cached root, to the origin. A separate field rooted at
6,290 gives a 44-step lower bound for any route through the requested
goal. Neither the preferred L nor a Manhattan-only proof covers this
request. Compare at most four fresh, complete point fields beside either
endpoint, and use the strict bound before resuming the cached search.
The existing exact path handles ties and unavailable witnesses.

| Guest cycles | Shipping tail | Component and distance checks |
| --- | ---: | ---: |
| Mean advancing call | 2,481,146 | 2,221,786 |
| All-call p95 | 3,628,357 | 2,914,594 |
| All-call maximum | 4,841,218 | 4,451,529 |
| Call 47,823 | 4,140,591 | 4,039,121 |
| Call 47,855 | 4,841,218 | 4,011,998 |

The paired Z10 replay again matches all 131 named checkpoints, every
sampled route and frame counter, and nine world and display cuts. Mean
falls 10.45%, p95 falls 19.67%, and the maximum falls 8.05% against the
shipping build. This window runs at 7.55 emulated gameplay updates per
second. The 30 fps goal remains open.

`CANONICAL_ROUTE_POINT_PREFIX=1` selects the combined policy in configured
builds. `make test-sc64-route-point-prefix` runs the focused sanitizer cases
and compares the logical residency stream with the reference. The range
fixture covers cached reads that need a parked route and checks both reserved
range planes. An uncovered cached range read can still complete a parked
search synchronously. Keep the policy opt-in until dense ranged-combat
timing and wider campaign save continuation are qualified.

The repeated 958-update P7 combat sample matches the 11 world and display
cuts and the complete route and terrain timeline. Mean moves from 1,883,264
to 1,883,991 cycles, p95 from 2,179,462 to 2,176,128, and the maximum from
2,554,931 to 2,583,074. The three-AI Blood Bath replay again matches all 25
world, route and display cuts and exact SRAM through a fresh-process reload.
It reproduces the component-only timing receipt byte for byte. Its mean of
802,657 cycles remains close to the shipping build's 802,530 cycles.

All four profiles pass the standard header and RAM checks. The combined
builds reproduce the launch matrix for all 30 campaign entries and 72
skirmish setups, spanning both maps, three human races, and one to three AI
opponents. Eight custom-game SRAM transfers cover both directions between the
shipping and opt-in builds across the four profiles. Both matrix receipts
match the earlier component-only run. These checks cover startup and sampled
custom saves. Campaign continuation needs longer input and save traces.

With the opt-in disabled, all four rebuilds retain the shipping memory
limits. Each ROM differs by four bytes. Disassembly shows the same two
independent stack loads into `r4` and `r6` in reversed order in the linear
flood. All other ROM bytes match the shipping build.

- Combined distance-bound replay SHA-256 `c3c0177c98a6afd34b164798a855a243a597d4fb2dc7ced3cad9c252645c89ae`.
- Combined busy P7 pair SHA-256 `52b3fde1c97ee0c115eda8967ca252d51e9ad2a40d68f87ab10ef29d04eb4a1f`.
- Combined three-AI Blood Bath receipt SHA-256 `448fbb1b39db0e9e181877a924d198454159778e868a88fac40f0fbeff58c426`.

### Full Z9 replay and campaign saves

The current normal Z10 pair also plays the retained [Z9 controller tape](2026-11-13-z9-normal-physical.md)
from a blank title screen. Each run takes 74,386 recorded controller calls
and 74,398 emulator calls after six modal page acknowledgments. Both match
the 32 historical checkpoints and win at campaign frame 72,628 with no
trigger faults. A second replay confirms all 106 sampled state and display
hashes and the separate cut at physical call 46,500. The comparator verifies
the relocated mission pointer against each ELF's Z9 descriptor and compares
the resolved mission identity. Both finish with 6,612 route builds and
233,939 hits.

| Guest cycles | Shipping tail | Component and distance checks |
| --- | ---: | ---: |
| Mean across full replay | 1,607,043 | 1,585,775 |
| Full-replay p95 | 2,544,025 | 2,344,594 |
| Full-replay maximum | 62,344,943 | 62,368,713 |
| Mean across first 46,500 calls | 1,519,410 | 1,498,907 |

The full mean falls 1.32% and full p95 falls 7.84%. These totals include
menus, modal pages and the result transition. The last 500 calls of the
46,500-call interval show nearly flat mean and order cost.

Both builds also load a retained Z9 campaign SRAM through the title screen
at frame 6,069. After 600 updates each writes the same 32 KiB SRAM through
the pause menu. A fresh process then loads the other build's save over both
directions and reaches frame 6,849 with matching raw actor, building,
resource and mission state. This qualifies that campaign checkpoint in
addition to the custom-game matrix.

A retained P9 campaign save also loads in both builds at frame 30,109.
Each build writes the same journal at frame 30,709, and fresh loads in
both directions continue to frame 30,889 with matching simulation state.
Both builds launch T9 from a blank title screen and produce the same
opening journal at frame 601. Fresh cross-loads continue to frame 781
with matching simulation state.

- Full timed Z9 receipt SHA-256 `5bfea9ac5c5b1f85e8c91ad905cede0a46e0c05c92e322e9ed3bfe142a7d2c69`.
- Full normalized Z9 replay SHA-256 `1fe8979e0c1c42fca721066f830eb40fffd1f7a9e148b440e9c26cb0e54bace0`.
- Z9 fresh campaign cross-load receipt SHA-256 `0ea1b4025ff52287655a4fcef0ab65c2e198b1be0be712cb325ee90136d91219`.
- P9 fresh campaign cross-load receipt SHA-256 `ae946c4e5fb9f38648f0133ac0d6840a01b973299c98501467e112c857a276e2`.
- T9 fresh campaign cross-load receipt SHA-256 `30de9534a6788c16558439c45eb28d6ea8aff793347e046bed29a1812b425303`.
- Disabled configuration link comparison SHA-256 `17bff2280fada694646227b91aca19d0f5398dd77bf6f4b7f2de92cd3b30e393`.

### Bounded read of a parked range route

An uncovered open origin next to a pending FIFO cell gets its exact step
from the first adjacent pending cell. A visited neighbor would have already
discovered the origin, and later neighbors enter the FIFO after the pending
cells. A local range-reader prototype uses this witness without updating the
packed field. The host tests cover FIFO ties, blocked and covered origins, and
a full scan of 128 entries. Distant reads retain the existing fallback.

In dense Z7 the prototype matches all 131 checkpoints and the 480-call
route and display comparison. It changes mean cost by 15 cycles and p95
by 46 cycles across the 477 advancing calls relative to the combined opt-in
build. Keep it local and count hits on a ranged-heavy workload before more
qualification. Receipt SHA-256 `118a7c9708afb4d71394bbc0b4f7713f75a5a153f5684ec52e0a8062f22511df`.

### Qualification with merged P10 steering

The paired build uses composed source `e2126e74` on release `03b5db64`.
Within each cartridge pair the prepared assets and compiler recipe match
apart from the route-prefix selector. The P10 control differs from the
released ROM by four bytes that swap two independent stack loads in
`sc64_route_fill_linear`. Both orders write the same registers before the
next instruction uses them.

The dense Z7 tape matches all 131 checkpoints, 480 frame and logical
route entries, and nine raw actor, resource and framebuffer cuts.
The average covers 477 advancing updates. The other timing statistics
cover the full 480-update window.

| Dense Z7 ARM cycles | Former route search | Resumable route search | Change |
| --- | ---: | ---: | ---: |
| Advancing mean | 2,410,142 | 2,149,956 | −10.80% |
| All-update p95 | 3,547,005 | 2,844,577 | −19.80% |
| All-update peak | 4,760,373 | 4,369,053 | −8.22% |

On P10 both cartridges win after 47,194 ordinary inputs at campaign frame
46,057. The twenty named checks, ten briefing pages, 14 dialogue
acknowledgments and all 48 sampled world, fog, route, displayed pixel
and palette states match. The replay includes an in-session save. It
uses the same ordinary HOME launch and victory tape in both arms.

| P10 ARM cycles | Former route search | Resumable route search |
| --- | ---: | ---: |
| Full mean | 1,664,780 | 1,664,708 |
| Full p95 | 2,351,458 | 2,351,224 |
| Full peak including result | 36,561,939 | 36,559,572 |
| Late combat mean, calls 41,913–47,193 | 1,885,647 | 1,886,070 |
| Late combat p95 | 2,565,223 | 2,566,304 |
| Late combat peak | 7,797,162 | 7,801,582 |

The separate P10 attribution run uses the original candidate ROM and
one emulator breakpoint at the direct call from
`sc64_range_cached_point_step` to `sc64_point_prefix_complete`
(ARM address `0x08047d92` for this ELF). It observes zero calls in the
complete victory tape, with the same final state, cuts and timing
counters as the uninstrumented candidate. The host range fixture
forces this fallback and verifies its returned step. Outpost and Blood
Bath are both 64×64, and parked prefixes require an exact 128×128 map.
Use a wide campaign or a synthetic 128×128 fixture with ranged units
following a field seeded by nearby units to test live synchronous
completion and its peak cost.

All eight ARM links pass the existing guards. The smallest candidate
IWRAM and EWRAM margins across products are 240 and 7,428 bytes.
The route sanitizer target passes on the composed source. Keep the
selector off by default while qualifying the remaining wide ranged
workloads. In these campaign windows both builds exceed the
561,792-cycle budget for 30 updates per second.

| Local receipt | SHA-256 |
| --- | --- |
| Dense Z7 current composition | `5dd45e37aa9915e831b1155fb2f41b42737e643d684edd2dd0b996b8a723de3e` |
| Full P10 control | `ba52668a599e893211538fc92cbfba1d626cc97e1e0513cc52c065e019242cbc` |
| Full P10 candidate | `d964d5fa6cbc5984725be085b36bc7946abb36568c822019e0911198b484f97a` |
| Cached range observer | `4862627b1699e59f7b94dc36b71679d48a4cca5ced0b1aca4f49d4db8ae27991` |

### Forced cached range completion on ARM

A second read-only observer sees zero cached range completions in the
first 48,237 ordinary calls of the Z7 tape. The observer matches the
candidate's recorded gameplay and timing cuts. Its completion branch
is at `0x080434e2` for the Z10 ELF. [P10 and Z7 comparisons](#qualification-with-merged-p10-steering)

A synthetic 128×128 ARM fixture starts from the canonical P10 recipe
and sets a wall in column 64 from rows 8 through 122, with row 110 open.
It opens the other cells. A point field rooted at (32,32) serves a
nearby request at (33,32) first. A cached range request from world
(1608,1608) to (520,520) then asks for a step across the wall. Both
builds return (1608,1624), count one logical build and zero hits, and
keep those counters fixed during the range read. The candidate parks
and then completes its prefix.

| ARM cycles in this forced case | Former route search | Resumable route search |
| --- | ---: | ---: |
| Initial point fill | 1,086,583 | 102,766 |
| Cached range read | 1,228 | 1,198,340 |
| Combined | 1,087,811 | 1,301,106 |

The test measures the two engine calls directly, outside full-frame
rendering. The forced read costs more than two 30 fps update budgets.
The replay observers establish frequency only for their actual
inputs. Before enabling resumable routes by default, reduce the
cost or bound the timing of the exact completion while preserving
the returned step and the shared range scratch rules.

| Local receipt | SHA-256 |
| --- | --- |
| Z7 range observer | `87235b9072d97309405f2673090343ea4ae9088ccaf40632f42d93cc8cca55ca` |
| Forced ARM control | `99af92b2fa9210981ed5ce0742d2e4d4db09a95ff59d8f3b7e0ccf19c68eb17d` |
| Forced ARM candidate | `f52a4895ac32b3639279b788c8d1c35785186b9936fadd6978a781948917c0d0` |
