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

An entire bucket rarely shares the mover's coordinate. Nearby members
often share a subcell with each other while other actors occupy other
subcells in that bucket. The 75,090 figure includes the mover when present
and counts complete lists. An exact-position feature would need to test
the subgroup and measure saved visits before adding maintenance to
ordinary movement.

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
