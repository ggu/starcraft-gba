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
