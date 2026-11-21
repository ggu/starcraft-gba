# Deferred point certificates on the packed Z10 build

The exact seven opt-in route flags from [PR #616](https://github.com/ggu/starcraft-gba/pull/616)
compose with the packed-building Z10 recipe on `60dd3d94`. The control
keeps the release sparse packing and the same prepared building and
actor banks. The candidate adds these flags to its frozen ARM compiler
vector.

```text
SC64_ROUTE_POINT_PREFIX_PROBE
SC64_ROUTE_POINT_COMPONENT_PROBE
SC64_ROUTE_POINT_NEAR_COMPONENT_PROBE
SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
SC64_ROUTE_POINT_SECOND_EXACT_HIT
SC64_ROUTE_POINT_COLD_CERTIFICATE
SC64_ROUTE_POINT_FAR_READ
```

Normal point movement stays on the non-monotone path. The cartridge
header, ELF payload and prepared input pins pass. The packed control
ends at IWRAM `0x03004d60` and EWRAM `0x02039b30`. The candidate ends
at `0x03004ea0` and `0x02039f60`, leaving 352 and 16,544 bytes below
its link guards. Runtime stack watermarks are a separate check.

## Ordinary-input Z8 replay

Load the same ordinary pause SRAM from blank HOME with L. The resulting
field starts at simulation frame 16,659 and tape call 17,122. Resume the
same held controls through call 33,900. Ordinary A or START button edges
acknowledge the three recorded one-page modals at calls 27,625, 27,746,
and 27,894. The actor frame stays fixed during each acknowledgment.

All 17 sampled checkpoints match unit, building, resource, fog and
walkability hashes, logical route metadata, palette, both indexed pages
and the display register. Across all 203 timed advancing calls, game
frames and logical route counters match. A separate read-only ARM
observer records 668 identical argument and return records in those
windows. The movement calls return 627 matching coordinate pairs and
24 matching declines. All 17 cached weapon-range lookups decline.
The observer reproduces the ordinary runs' timed cycle samples.

| Tape calls | Mean guest cycles, control → trial | Peak guest cycles, control → trial |
| --- | ---: | ---: |
| 26,330–26,370 | 1,555,862 → 1,499,317 | 9,677,315 → 9,299,317 |
| 31,050–31,090 | 1,647,316 → 1,627,816 | 4,751,489 → 4,749,853 |
| 33,780–33,900 | 1,758,274 → 1,581,133 | 12,056,654 → 10,909,326 |
| All 203 samples | 1,694,983 → 1,574,037 | 12,056,654 → 10,909,326 |

Across the sampled calls, mean cycles drop 7.14% and peak cycles drop
9.52%. Nearest-rank p95 rises from 3,235,449 to 3,309,269 (+2.28%).
These measurements cover selected emulated updates after fresh load.
The control enters 94 full linear fills. The trial enters three and
attempts 103 local certificates. The far-reader helper has zero visits.

Call 33,900 adds 883,950 cycles, including 883,883 in orders. Both
builds make the same six successful movement queries, all with the
same outputs. Both retain 423 logical builds across that update.
The trial enters a local certificate and a full linear fill. The
control enters zero full fills. This is the [resident retry cut](#resident-retry-at-call-33900)
for the follow-up. Keep the seven trial flags off in the normal recipes
while the p95 cost and the real far-reader gate remain open.

The combined sparse and non-monotone HOST oracle checks 194 certified
shortest paths, including 45 blocked goals, and 62 declines. The sparse
boundary fixtures match canonical BFS. The separate far-reader fixture
checks 108 randomized full fields and 8,320 range scratch cases.

| Local input or receipt | SHA-256 |
| --- | --- |
| Packed Z10 control ROM | `ea3faa53ff31553e91e3888c58746161d9ac9681bffd6a45885df4e14ea32308` |
| Seven-flag trial ROM | `944e3cc7837b5154c9729bfaf86889c48ff0c0f5edc67a56acaaa942c826a4d7` |
| Retained pause SRAM | `bf99add2606817dc705ba1c03d5a15ca8f72a6133c2e8b1b73915b2606f130d4` |
| Ordinary input tape | `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb` |
| Paired comparison with direct route answers | `2051f2a93c46b6348addf297070bbb9dec67c03f8b19f11870ae6e4709b7a6d4` |
| Trial link and exact compiler vector | `7e038214a43de5fead42ff0ad0c2cc6c4171a9d5a64b4cc565cb0edd126d02ef` |

## Resident retry at call 33,900

The second movement query enters `sc64_route_slot_for` with goal 13,540
and open origin 15,209. At the preceding input boundary slot 6 is a live
certificate-only field with that exact goal. The trace enters
`sc64_point_certificate(6, 15209, 13540, 0)` from `0x0802ace8`, inside
`sc64_point_certificate_require`. It then enters the full linear fill
for goal 13,540 from `0x0802a6be`. The attachment clears after that
update. The caller uses an exact cached goal, and the route build
counter remains 423 throughout.

The origin at (105,118) is 18 cells from the blocked goal at (100,105).
The old limit of 16 rejects it before the scratch search. An independent
ordered reverse BFS on the observed walkability finds an 18-step path
with a nearest-seed Manhattan lower bound of 17. The existing four-step
detour proof needs 273 of 625 scratch cells and reaches the same entire
canonical chain after 85 pops. The first step is west to cell (104,118).

The follow-up permits gaps 17 and 18 only for retries. The initial
insertion limit stays 16. The expanded retry checks the existing scratch
capacity before writing and initializes the lower-bound reduction above
all possible seed distances. Canonical seed ordering, neighbor ordering
and chain publication use the existing proof. Whole-field readers retain
the exact materialization path.

| Operation | Packed control | Original trial | Retry trial |
| --- | ---: | ---: | ---: |
| Call 33,900 | 1,140,508 | 2,024,458 | 1,167,764 |
| Linear fills at call 33,900 | 0 | 1 | 0 |
| All 203 calls, mean | 1,694,983 | 1,574,037 | 1,569,972 |
| All 203 calls, nearest-rank p95 | 3,235,449 | 3,309,269 | 3,320,958 |
| All 203 calls, peak | 12,056,654 | 10,909,326 | 10,909,507 |

All 668 direct route answers, 17 world and display cuts, and 203 frame
and route counter samples match in the revised run. The revised trial
executes two full linear fills across the three windows and has zero
far-reader visits. Its worst added cost versus the original trial is
21,629 cycles at call 33,890. Outside call 33,900, mean cost rises by
156 cycles and three individual samples rise by more than 1,000 cycles.
The work remains opt-in with a p95 increase of 2.64% versus the packed
control, despite fixing the measured displaced fill.

The sanitizer fixture covers both gaps with a blocked goal and an
isolated nearer seed. It verifies the original cold decline, rejection
at gap 19, same resident backing and hit counters, and every direction
against independent BFS after an explicit full-field read. The ARM
header and ELF payload checks pass. IWRAM and EWRAM endpoints are the
same as the previous seven-flag trial.

| Retry evidence | SHA-256 |
| --- | --- |
| Read-only caller and slot attribution | `06adf3a448ff420fbe56742e62c1ea3e7f40b47a7aace47947fc139ea5b375ac` |
| Revised ARM ROM | `83374d50b901b1ec23bc123878b5702e59226cb8061b03caf2927fb61251bf56` |
| Revised ARM ELF | `2097ea64eaf3856f54d38bd69e3bac7c793a12a11532998405858cbb0866da58` |
| Revised link receipt | `0226ec75eaad02e05d6b12303f245fc9f31ab368ae83cee5816cc4f8e51b2a8b` |
| Three-way comparison | `8858d7f1058de0a48163a8460eddc36aa16f41ba06d37881d755b01c13dffe37` |

## Sparse packing after a cold prefix exhausts its component

At call 33,852, the old trial rejects a local certificate at goal 2,419
and exhausts a two-cell component. The packed control takes 90,329
cycles to fill and publish this route. The trial's cold prefix takes
227,548 cycles because it packs the entire dense grid after finding the
same two cells. An unmodified input tape and read-only function-entry and
return traces establish both timings.

`SC64_ROUTE_POINT_PREFIX_SPARSE_PACK_PROBE` lets the completed cold
prefix use the same packing helper as an eager fill. A 128-column field
with at most 384 discoveries and at least four spare queue slots still
contains the complete discovery list, so the helper clears the packed
field and writes only the discovered cells. Resuming a parked frontier
uses dense packing because its queue starts after earlier visits. HOST
sanitizers compare all packed directions with canonical BFS at 2, 384 and
385 discoveries and pass the existing sparse, range and far-read cases.

| Guest cycles | Packed control | Retry trial | Sparse prefix trial |
| --- | ---: | ---: | ---: |
| Call 33,852 | 3,170,524 | 3,330,888 | 3,210,153 |
| Mean across 203 calls | 1,694,983 | 1,569,972 | 1,565,906 |
| Nearest-rank p95 | 3,235,449 | 3,320,958 | 3,210,153 |
| Peak | 12,056,654 | 10,909,507 | 10,915,192 |

Seven calls save more than 114,000 cycles versus the retry trial. The
largest individual increase is 6,797 cycles. All 668 route answers and
17 world and display cuts match, as do the three modal boundaries and
all 203 frame and logical route counts. The ARM link keeps IWRAM and
EWRAM endpoints at `0x03004ea0` and `0x02039f60`. This experiment uses
the prior seven trial flags, the normal sparse pack flag and the new
flag, with the wider cold rectangle experiment disabled.
[Opt-in sparse prefix source](https://github.com/ggu/starcraft-gba/commit/d9de8cf895085333f576338cca56c3fd103c7b19)

At call 26,357, a separate nearby-root read takes 993,135 cycles and
still returns false. The stored 60-entry frontier for root 9,323 matches
canonical BFS. Its requested origin 5,608 becomes known after 60 more
pops, and its path excludes requested goal 8,939. The existing bounded
second-cutoff machinery can be tested on this reader without changing
whole-field callers. Keep the trial opt-in until the remaining nearby
root materializations and wider-profile gates are covered.

| Sparse prefix evidence | SHA-256 |
| --- | --- |
| Packed control function timings | `bf059ac9b2a50d925f23ce647093ccfb759c6c59653d55d5795d1188ae01b6e8` |
| Retry trial function timings | `91a64e48b0c6d3ea24ac00e5cf643c740bc2be0356db1be6c578181f83d5240c` |
| Candidate ARM ROM | `b137e02cf9f30c1618b060759f18ac67e1479273cf7195d3832a1bcd9337ad8b` |
| Candidate ARM ELF | `9fe37c9b0e35f4f4eab0d29404160ea1b86138a235c7ba0ef8d6f964682f704b` |
| Candidate link and vector | `eb3e17f244088cc5d68adb7f55ea2ffbaac043057e184636fd8ee6e118f12155` |
| Candidate ordinary replay | `597bfcf6aab3f934d9abf84003c7a64f18677f705fe31d263489277e217a845b` |

## Bounded resumption for nearby-root readers

`SC64_ROUTE_POINT_NEAR_DEMAND_PROBE` gives the nearby-root gradient reader
the same bounded resumption already used by exact point hits. The reader
starts at an open origin and follows canonical parents that are already
discovered once that origin enters the FIFO. A real parked frontier can
resume for up to 1,024 pops once. A second miss or a declined cutoff
completes the field through the existing path. Certificate-only and
whole-field callers keep their existing paths. HOST sanitizers compare
all discovered cells and the later full field with canonical BFS for
both acceptance and rejection of a nearby goal.

The paired ARM replay compares the same sparse-prefix Z10 build with and
without the demand flag. It matches all 668 route answers, 17 world and
display cuts, three modal boundaries and 203 logical route counts.
Read-only guest traces confirm that call 26,357 resumes slot 4 for origin
5,608 instead of requesting the complete field. The nearby-root reader
returns false in both builds. Its time falls from 993,105 to 262,315
cycles. The equivalent reader at call 26,346 falls from 986,930 to
259,238 cycles, also with the same false result.

| Guest cycles | Sparse prefix | Sparse prefix with nearby demand |
| --- | ---: | ---: |
| Call 26,346 | 9,299,442 | 8,569,221 |
| Call 26,357 | 2,587,525 | 1,856,031 |
| Call 33,890 | 5,231,068 | 6,260,211 |
| Mean across 203 calls | 1,565,906 | 1,574,460 |
| Nearest-rank p95 | 3,210,153 | 3,174,411 |
| Peak | 10,915,192 | 10,921,023 |

Full linear fill visits rise from two to 22, including three additional
visits on call 33,890. The worst increase and higher mean keep this
variant opt-in. Both link endpoints remain unchanged at `0x03004ea0`
IWRAM and `0x02039f60` EWRAM. This comparison keeps the wider cold
rectangle experiment disabled.

Read-only pressure traces match six world and display cuts around calls
33,852, 33,853 and 33,890. At each cut, the demand variant retains an
additional frontier on slot 2, root 15,224, with 42 pending cells and its
second cutoff used. That fills the fourth cold row. On call 33,890 the
first four cold candidates all have open origins, valid seeds and no
claimed range points. The capacity check sends all four to eager fills.

| Cold candidate root at call 33,890 | Sparse prefix cycles | Unreserved nearby demand cycles |
| --- | ---: | ---: |
| 13,538 | 197,433 | 876,894 |
| 13,540 | 176,249 | 877,767 |
| 13,667 | 219,660 | 876,413 |
| 13,668 | 23,754 | 876,480 |

The sparse prefix spends two later full fills on roots 13,668 and 13,796.
Unreserved demand only needs the later fill for root 13,796, bringing its
total to five for the call. The extra parked row causes the lost cold
certificates and the displaced cost.

| Nearby demand evidence | SHA-256 |
| --- | --- |
| Original frontier and canonical path | `d40221122f7eb676a34df508e0fe2448f1d0058413fdfe3437cf0920d3cb3df9` |
| Sparse prefix function timings | `79423251d4ac47214e1e83377ab225cc8757b2e8781f5aec4d1d74c07d808500` |
| Nearby demand function timings | `526f4a41dde9e080faf99977a3d57f33ecd7ab73d41420d79299a54233aa9d76` |
| Sparse prefix cold-row pressure | `9e6bec9bdeb75fa2aabd402b1f8f40256d3cd8afbc1984886d85351bdc5631b3` |
| Nearby demand cold-row pressure | `ed4ac995f75556ed2554f1a048f2a46624c1cc82be71c806e5d3d8d6cf5f28f4` |
| Candidate ARM ROM | `65fecf65708467102cc4c5f3a0f1af64a75915387a5a980999a73cebfdb8591a` |
| Candidate link and vector | `0469be2a73d2b0f35a60c92c6ceef26d66e1e87005183db4312a2fbe377f081d` |
| Candidate ordinary replay | `d6b1afe479d3e859acd79e949ffe3d1374d4e9982e068b0fbef1f0cf55670c3b` |

## Reserved nearby-root frontier

`SC64_ROUTE_POINT_NEAR_RESERVE_PROBE` adds one 264-byte frontier row
for successful nearby-root resumptions. The four cold rows remain
available to the original cold selector. An eligible nearby resume
writes its new FIFO and bookkeeping into the reserve, switches the
logical route's owner index, and frees its old row. A live reserved
owner sends the next nearby request through full completion. Ordinary
exact hits continue to repark in their original row, and whole-field
readers still complete their routes.

Sanitizer checks fill all four cold rows alongside the reserve, verify
the eager fallback for a fifth ordinary request, and verify canonical
full completion when the reserved row is busy. They also exercise
transfer after a route slot is repurposed and release after an exact
full-field read. The flag-off, certificate, range and far-read checks
pass. The normal Z10 ARM link keeps IWRAM at `0x03004ea0` and moves the
EWRAM endpoint from `0x02039f60` to `0x0203a068`.

| Guest cycles | Packed control | Sparse prefix | Nearby demand | Nearby reserve |
| --- | ---: | ---: | ---: | ---: |
| Call 26,346 | 9,677,315 | 9,299,442 | 8,569,221 | 8,571,227 |
| Call 26,357 | 2,232,150 | 2,587,525 | 1,856,031 | 1,856,454 |
| Call 33,890 | 8,486,297 | 5,231,068 | 6,260,211 | 5,228,422 |
| Mean across 203 calls | 1,694,983 | 1,565,906 | 1,574,460 | 1,555,839 |
| Nearest-rank p95 | 3,235,449 | 3,210,153 | 3,174,411 | 3,210,981 |
| Peak | 12,056,654 | 10,915,192 | 10,921,023 | 10,923,237 |

The reserve restores the sparse prefix's two direct full fills and 103
certificate attempts. All 668 route answers, 17 world and display cuts,
three modal boundaries and 203 logical route counts match. Compared with
the sparse prefix, the mean falls 0.64%, p95 rises 828 cycles and the
peak rises 8,045 cycles. The largest paired increase is 10,254 cycles.
At the six pressure cuts, the 42 pending cells for root 15,224 occupy
the reserve. Three active cold rows leave one available to new requests. Each of
the first four cold candidates at call 33,890 takes its prefix path.
The two later full fills again belong to roots 13,668 and 13,796.
This is an opt-in Z10 result on the same retained Z8 input and save. Other
cartridges, busy custom games and physical GBA timing remain untested.
[Opt-in reserve source](https://github.com/ggu/starcraft-gba/commit/794524686eee5529c25ff6853c69c5bdde4d64ef)

| Reserve evidence | SHA-256 |
| --- | --- |
| Candidate ARM ROM | `7d29e32c8d9dcdc3af6db54b0c82877ef8c2a743754f7b961c629652d422f41b` |
| Candidate link and vector | `b5231b62da3a2aa0c9a15f1889016e82cee99616e2bd5a0a2c1fbe6ad4d1af3d` |
| Candidate ordinary replay | `242972f600d3c32769ca502674d9e475f23f01cc2ba684e5b7b587421ed715c9` |
| Four-way comparison | `de67351d63430419afe7aa3e7de835e88ac4f130a34b77246dc47a26bbc74cef` |
| Reserved frontier and cold-row pressure | `f62c3e92a82a9e868518fbfa711599968a050c7a4221fac3c657b4963d2fb8bc` |

## Reserved frontier on the combat-facts release

The opt-in recipe on `63d11258` adds the ten route flags to the
shipping `9c04c73a` compiler vector. Z10 keeps its packed terrain,
sparse full-field packing and immutable combat facts. Each pair
uses identical ordinary input, SRAM and prepared banks.

Z8 resumes from frame 16,659 and traverses tape calls 17,122–33,900
and three modal transitions. All 668 logged route answers, 17 world
and two-page framebuffer samples, and logical route counts at all 203
timed updates match. Full linear fills in those windows fall from
94 to 2. The candidate makes 103 certificate calls.

| Guest cycles across 203 sampled updates | Release | Reserved frontier |
| --- | ---: | ---: |
| Mean | 1,694,349 | 1,554,958 |
| p95 | 3,235,970 | 3,208,667 |
| Peak | 12,056,077 | 10,911,067 |

The sampled mean falls 8.23% and the peak 9.50%. The largest
single-update saving is 3,619,390 cycles at call 33,892. The largest
increase is 257,246 cycles at call 26,359.

The full resumed interval gives a smaller gain. Without breakpoints,
16,784 controller calls use 20.158B versus 20.080B guest cycles
(0.387% lower). Seventeen display and world samples and the three
modal boundaries match. The maximum since boot is nearly unchanged.

### Dense Z7 stress window

Fresh HOME load and ordinary Hatchery reselection reach tape calls
47,758–48,237 with matching 114 named world checks, seven raw world
and two-page samples, 480 logical update rows and five cache metadata
cuts. All 48 commonly complete nonstale fields match. A partial
field is expected to contain less than its complete control
counterpart. Reserve row 5 is live at call 48,149.

| Guest cycles across 477 advancing calls | Release | Reserved frontier |
| --- | ---: | ---: |
| Mean | 2,193,360 | 1,928,747 |
| p95 | 3,182,885 | 2,608,287 |
| Peak at 47,995 | 5,113,194 | 5,687,320 |

Mean and p95 fall 12.06% and 18.05%. The peak rises 11.23%,
with 500,427 of the extra 574,126 cycles in orders despite matching
4 builds and 74 hits. Calls 48,148 and 48,149 change by −702,328
and +419,349 cycles, respectively. Keep the normal route flags at
release settings while isolating the peak and checking the late Z7 tail.

### Cross-product checks

All four links pass the ROM, payload and RAM guards. The ten flags
add 1,336 bytes of EWRAM per product, of which the reserved row uses
264. IWRAM rises by 320 bytes in Terran, Z10 and P10.

| Trial product | IWRAM endpoint | EWRAM endpoint |
| --- | --- | --- |
| Terran | `0x03004b38` | `0x0203af44` |
| Z10 | `0x03004ea0` | `0x0203a068` |
| P10 | `0x03004f60` | `0x0203bb64` |
| Showcase | `0x03004fb8` | `0x020396a0` |

The three-CPU Blood Bath replay resumes with Zerg, Terran and Protoss
opponents at frame 17,305. Both builds match 3,600 route counter
pairs, 31 world, projectile and framebuffer observations, and the
exact pause journal. They match their uninterrupted peers after a
fresh reload for 240 more updates. Mean cycles rise 0.046%, p95
falls 0.072%, and peak rises 0.031%. The 64×64 map tests fallback
because frontier admission requires 128×128.

The P10 replay resumes at frame 36,761 and wins at frame 46,057.
All 68 world, palette and indexed-page checkpoints and the victory
journal match after 9,489 ordinary controller updates. Mean guest
cycles fall 0.024%. The Campaign openings, first Showcase scene
and both Custom Game maps match on the trial builds.

Run the sanitizer checks with `make test-sc64-route-point-reserve
test-sc64-route-point-prefix test-sc64-route-point-far-read`. The
row-span fixture uses the non-certificate recipe because certified
rows can omit the partial span entirely.

| Current evidence | SHA-256 |
| --- | --- |
| Z8 comparison | `e3746b9b68b78afdaa2e0262d11af59bc1a0131d9c5ca770bda783857c78adb2` |
| Full resumed Z8 interval | `693fa560bf3ce81b0718557e327df69159be8f91e0b35d1e7878deabf72a79bb` |
| Dense Z7 comparison | `7b5800c5be7cf1d1f6a0b005c305c460f0f52ac41fe763e252c2502000a0836c` |
| Active Blood Bath comparison | `67b5752a1629c42fec16b2e623837c41fc07bb84fae9433bbe02b11c570fc40a` |
| P10 victory comparison | `891694fdb0c469abf5dd3d7f5de6c71fdbd49d15106fbde46419bfd53a216afe` |

### Transfer after the ordinary cutoff

Removing the demand and reserve flags from the same Z10 recipe
leaves the Z7 peak at call 47,995 and raises it 11.42% over the
release. That sparse-prefix lane matches the same gameplay and
display checks. The register trace of the reserve lane finds two
nearby-root misses for roots 6,033 and 6,161. Each has already
spent its ordinary cutoff. The reserve is empty, but both fall through
to full materialization and spend 841,857 and 833,984 cycles.

`ffc58226` gives an ordinary row one bounded transfer into a free
reserve after that row has used its regular cutoff. An occupied
reserve or a further miss on its owner still completes the field.
The sanitizer fixture exercises both cutoffs on a fresh 128×128
grid, verifies ownership and counters, then matches all 16,384
directions against a full breadth-first search. A flags-off Z10
link on `ab215718` reproduces the shipping ROM byte for byte.

| Guest cycles | Release | Original reserve | Transfer |
| --- | ---: | ---: | ---: |
| Z7 mean, 477 advancing updates | 2,193,360 | 1,928,747 | 1,920,886 |
| Z7 p95 | 3,182,885 | 2,608,287 | 2,595,075 |
| Z7 peak, call 47,995 | 5,113,194 | 5,687,320 | 4,412,647 |
| Z8 mean, 203 sampled updates | 1,694,349 | 1,554,958 | 1,551,249 |
| Z8 peak | 12,056,077 | 10,911,067 | 10,168,345 |

The transfer matches the Z7 world, page, route counts and complete
field checks, and all 668 Z8 answers and 17 world and display samples.
All four transfer builds pass their previous RAM bounds, Campaign
and Showcase openings, and both Custom Game map starts. The
3,600-update Blood Bath replay and its 240-update save continuation
again match the release, with the same timing as the original reserve
trial on this map. The full resumed Z8 interval saves 0.391%, with
matching modal input and both indexed pages. The P10 transfer build
repeats all 68 world and display checkpoints through victory and
exports the same SRAM journal. Its mean falls 0.024%.

Z7 call 48,149 still spends about 419,000 extra cycles. Its traced
root 6,033 begins as a certificate-only row, so the warm retry and
nearby read enter a different fallback. Profile that call and run
both late Z7 windows from ordinary input before changing the
release flags. Physical timing remains open.

| Transfer evidence | SHA-256 |
| --- | --- |
| Z7 sparse-prefix comparison | `f2c1ad5981d663a30c28d18c84845d953bcc9efa273ce3d1bd8af9af200c81b3` |
| Original reserve Z7 route trace | `7772c929b2818387b341789c2cd6cd0ac58bba576cf5940d1f64a5b2d28532c7` |
| Z7 transfer comparison | `9fb2f00f078a9ff11a9e24d59e883da9abebd80ebfb11a99e23bc34e1cff0d78` |
| Z8 transfer comparison | `fa1c922f7fececf5539eb82cfd64fa2ce7c680d3c5d75fd8d947c8d2e136fb6e` |
| Full Z8 transfer interval | `613f018edb4a4013ec89e568f87e44884f33f60da86decd584adf5b15dbcdb6f` |
| P10 transfer victory | `4a26e983d76f02f3969750bae019aaebf3f5789b92700867ca7d04f668d56b44` |
| Blood Bath transfer comparison | `4cb1dcc7186071d66c017a5830dd392aa99c170808737c9326386dac95642c0c` |

### Build the reserved frontier

Use the normal route settings and prepared profile inputs.

```sh
CANONICAL_ROUTE_POINT_PREFIX=reserve make zerg-z10
```

The same option applies to the normal `terran`, `protoss-p10`, and
`showcase` Make targets. Mode `0` is the release default. Mode `1`
selects the original four-flag point-prefix trial. Each mode has a
separate build configuration stamp so changing modes rebuilds the
affected ROM.

The dry-run compiler plans for all four products add exactly the
ten tested flags in `reserve` mode. The Z10 plan also compiles its
fact table generator with those flags. Running the commands from
that plan with the pinned prepared inputs regenerates the same
immutable combat facts and the exact transfer ROM
`ddbf5d1c9e6715a33b1b3d47ac8d576faf667a67f7231799e6a9138059039831`.

### Nearby certificate after a short detour

The exact Z7 call 48,149 trace captures an open root at (17,47)
and origin at (20,40). The root's canonical shortest path is 16
steps for a Manhattan gap of 10. It crosses at column 14, three
cells west of the root, and passes west of the previous two-cell
margin. The nearby query for (17,49) returns false. The original
transfer run spends 753,971 cycles in the nearby read and
516,023 of those cycles in full-field expansion.

This follow-up gives an open-root nearby certificate a three-cell
margin and at most six extra path steps. The largest rectangle
under the existing 18-cell admission is 18×18, within the 625-cell
scratch allocation. The fixture checks the exact 16-step path and
all its published directions. An 18-step path still uses full
completion. Cold insertion and ordinary retries retain the two-cell
margin. The reserve, certificate and far-read sanitizer checks pass.

| Z7 guest cycles | Shipping Z10 | Free reserve transfer | Nearby certificate |
| --- | ---: | ---: | ---: |
| Call 48,149 | 2,402,668 | 2,821,420 | 2,095,952 |
| Mean over 477 advancing calls | 2,193,360 | 1,920,886 | 1,913,035 |
| p95 | 3,182,885 | 2,595,075 | 2,588,899 |
| Peak | 5,113,194 | 4,412,647 | 4,413,513 |

The three Z7 runs match 114 named world observations, seven
display samples, logical route counters and all 48 commonly
complete nonstale fields. Z8 matches all 668 answers and 17
world and display samples. Compared with the transfer, Z8's
sampled mean rises 55 cycles and its peak rises 426 cycles.
The new Z10 link retains the same RAM endpoints. Only the earlier
transfer build has the full late Z7 qualification.

| Nearby-certificate evidence | SHA-256 |
| --- | --- |
| Trace and captured walkability grid | `c812a402f6a7f5c365d37c3811060e418e9ed88ebf93dc532ad8ee9c331cd911` |
| Z7 comparison | `dd0c7adf32d4e120afde9380b00b22f4e46e8061337f2873138f9017e0b7e6e9` |
| Z8 comparison | `5fe67799cd9d35392841cd6ceac233745ff97d450aed95921e24ab482632748c` |
