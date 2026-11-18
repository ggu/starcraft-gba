# Steering collision inside occupied buckets

The steering mask tests candidates in 16-pixel buckets. The scalar narrow
phase uses each pair's radius and can reject an actor inside that coarse
query window. These measurements count work a spatial filter could exclude
using the existing query reach, before introducing a second index.

The diagnostic tracks both a complete read of each encountered bucket and
the stock loop's actual node visits. The stock loop can stop when every
direction is blocked. Complete-list counts therefore estimate opportunity
and include some nodes the stock loop never reads. A production filter
would also need to account for index maintenance and queries that reuse an
unchanged bucket.

## Busy Protoss 7

The observer reads the full bucket through the original links inside the
same physical input window as the [class census](2026-11-18-p7-current-mgba.md#work-inside-the-first-collision-lookup).
All 1,000 route, frame and terrain timeline entries and 11 state, palette
and display samples match the normal P10 ROM at `2655cb22`.

| Counter | Visits |
| --- | ---: |
| Queries that reached the candidate pass | 36,610 |
| Empty buckets | 48,415 |
| Occupied buckets | 143,888 |
| Nodes in complete lists | 743,291 |
| Nodes actually visited by the stock scan | 730,400 |
| Nodes outside intersecting 8-pixel subcells | 293,532 |
| Nodes outside intersecting 4-pixel subcells | 376,680 |
| Nodes in wholly excluded buckets using 8-pixel occupancy | 7,904 |
| Nodes in wholly excluded buckets using 4-pixel occupancy | 47,586 |
| Nodes in wholly excluded buckets using exact coordinate bounds | 147,683 |

An 8-pixel partition cuts off part of 91,314 occupied buckets. Those
queries would read up to 157,721 admitted subcell heads. Of those,
138,107 heads lead to a nonempty list. This makes the prospective cost
of a second chain measurable. A single-mask filter which can only drop a
whole bucket saves far fewer actor visits than a filter which can skip
part of a list. The observer found no actors outside their linked bucket.

The diagnostic is enabled under `SC64_CONTACT_FILTER_CENSUS` with a
closed game-frame interval. P7 uses
`SC64_CENSUS_FIRST_FRAME=7796 SC64_CENSUS_LAST_FRAME=8752`. Its timing
includes the census traversal. Candidate timing uses observer-off ROMs.

## Dense Zerg 7

The same diagnostic in the 477 advancing updates of the dense Z7 tape
matches all 131 historical checkpoints, 480 route and frame entries, and
nine raw unit, building, mineral, palette and two-page display cuts.
The linked-bucket coordinate check again finds zero mismatches.
The closed game-frame interval is 46,500–46,976.

| Counter | Visits |
| --- | ---: |
| Queries that reached the candidate pass | 5,389 |
| Occupied buckets | 11,565 |
| Nodes in complete lists | 426,167 |
| Nodes actually visited by the stock scan | 418,506 |
| Nodes outside intersecting 8-pixel subcells | 7,960 |
| Nodes outside intersecting 4-pixel subcells | 11,914 |
| Nodes in wholly excluded buckets using 8-pixel occupancy | 198 |
| Nodes in wholly excluded buckets using 4-pixel occupancy | 1,455 |
| Nodes in wholly excluded buckets using exact coordinate bounds | 3,251 |
| Nodes in an exact-position whole bucket | 86 |
| Nodes in an exact-position 8-pixel subgroup | 75,090 |

In this sample, a subcell can contain a uniform group while other
subcells in the bucket contain actors at different coordinates. The
75,090 figure includes the mover when present and counts complete lists.
An exact-position feature would need to test the subgroup and measure
saved visits before adding maintenance to ordinary movement.

## Whole-bucket probes

Two opt-in steering prototypes keep the original linked lists. A one-byte
summary certifies that all actors in a bucket share one position. A
16-bit summary records occupied 4-pixel regions and rejects a whole bucket
only when none intersect the query envelope. Both maintain conservative
head-owned metadata across births, same-bucket moves, head changes,
and legacy and persistent bucket setup. Host sanitizer checks cover
those transitions and the scalar fallback. The occupancy proof checks
type-derived radius ceilings before rejecting a bucket.

Both prototypes match the 11 busy-P7 world and display cuts and the
1,000-call route and terrain timeline. Relative to the shipping mean
of 1,883,264 guest cycles, the uniform-position probe costs 1,890,846
cycles (+0.40%), and 4-pixel occupancy costs 1,899,873 (+0.88%).
Their P95s are 2,185,501 and 2,196,380 against 2,179,462. Setup and
maintenance outweigh the whole-bucket opportunity in this replay.

The position probe also matches all 131 dense Z7 checkpoints, 480
route and frame entries, and nine raw world and display cuts. Mean
cycles rise from 2,481,146 to 2,494,689 (+0.55%). P95 rises from
3,628,357 to 3,639,650. Keep both whole-bucket versions outside the
release profiles.

## Lazy 8-pixel child lists

The 8-pixel prototype retains the stock list for full coverage and
uses secondary child lists only when a rectangle excludes part of a
bucket. The child lists rebuild after a position crosses a child edge,
an actor enters or leaves its parent, or the head changes. Host
sanitizers compare the mask to the scalar query for both bucket
implementations, including head transfer, same-cell moves during and
between orders, and the fallback for a radius outside the certified
bound.

The normal P10 link hits the existing 11 KiB IWRAM stack guard.
Moving the steering function to ROM on both sides preserves the
guard and permits a placement-matched comparison. The candidate
adds 1,264 EWRAM bytes. Both builds match the shipping 11 sampled
world and display states and the 1,000-call route and terrain timeline.

| Busy P7 build | Mean cycles | P95 | Peak |
| --- | ---: | ---: | ---: |
| Shipping P10 | 1,883,264 | 2,179,462 | 2,554,931 |
| Stock steering in ROM | 2,004,697 | 2,308,998 | 2,674,840 |
| 8-pixel lists and steering in ROM | 2,001,572 | 2,303,795 | 2,684,366 |

The child lists save 0.16% mean against the placement-matched control
and raise peak cost 0.36%. The viable candidate is 6.28% slower than
the shipping build. Keep this prototype outside release and revisit
it only if a design can retain fast-code placement and demonstrate a
larger cycle reduction.

- Placement-matched P7 receipt SHA-256 `fd1451f96e0a142c99f485a9953a261b4089ddbc30e4d03c7b6e23495016443c`.

## Native overlap and retask

A retained Ares Blood Bath capture tracks two independent human SCVs
through field mining and an ordinary group Move. At step 4,230 both have
position words `(496640, 75008)`, which decode to `(1940, 293)` at Q8
scale. Their motion words are zero and their mining orders differ. At
step 4,273 both have Move order 6 and a common destination. One still has
the original position and the other has moved to approximately
`(1940.25, 292.28)`. After the next field retarget, step 4,363 shows
distinct positions and waypoints with the same new destination.

This demonstrates that native workers can occupy the same coordinates
and separate on retask. A separate native Wasteland capture has distinct
positions for five Marines in the three retained samples. General native
collision and separation rules need further controlled traces.

- Native Blood Bath observation SHA-256 `884d4fadf377bc4fd039448f0def4c410bbe833c5470041d59be0076762ac64d`.

## Remaining cycle budget

With the separate routing candidate enabled, the same 477 advancing Z7
updates average 2,221,786
cycles. Orders consume about 1,112,800 cycles and draw consumes
557,908. The interval after orders and before sight adds 312,195
cycles, and sight adds 228,981. These timed intervals show why actor
collision filtering alone cannot reach the 561,792-cycle target.
In particular, orders could take zero cycles and the measured
remainder would still exceed the full budget. [Routing replay evidence](https://github.com/ggu/starcraft-gba/blob/fe95427aa2f565c089da97c1eb33e9b1afb0b24e/docs/audits/2026-11-16-resumable-frontier-budget.md#reject-a-connected-lookup-with-exact-distance-bounds)

The existing update diagnostic on the shipping Z10 vector separates
the post-orders interval. It matches 131 historical checkpoints, 480
route and frame checks, and all nine world and display cuts. Average
costs over 477 advancing updates are 170,171 cycles in mission
dispatch, 80,037 in defenses, 47,138 in race state, and 13,840 in
production. Research and the remainder add 1,055 cycles. Profile Z7's
mission dispatcher internally before deciding which simulation data
to cache or update incrementally.

For the next sight pass, count friendly movers with the same old and
new absolute tile, footprint phase and effective sight radius after
the existing no-change check. Batching is useful only when several
eligible movers have identical row deltas. Preserve per-actor
snapshots and exact source counts and compare fog, exploration and
minimap output before adopting the change.

- Combined census and whole-bucket replay receipt SHA-256 `921010d10575bb70efe1cf38dd13a5841db6dc04cc4675063ef3054d28bbf77d`.
- Z7 update-stage receipt SHA-256 `e689eacc557196a0dd64feab85ceb9d634ee91500240072bea189cacdb30cadd`.
- Paired Z7 update diagnostic SHA-256 `521387ae0ac42ef0c283eab359b2f45a71390a0ca29caa138e285cf7eee99874`.

## Prepared sprite fixture

The prepared-input roster test raises a missing raw-85 Scarab sprite
in the selected Terran and Protoss headers. A clean `2655cb22` archive
reproduces both failures with the same private inputs. The current
exporter has raw 85, and Zerg's selected header already exports it.
Protoss's alternate bank has the sprite but follows the stale P7
header in the compiler include order. Refresh the selected complete
private banks before using the roster fixture as a prepared-build
release gate.
