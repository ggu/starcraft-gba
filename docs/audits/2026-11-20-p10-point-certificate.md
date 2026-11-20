# P10 short-path certificate trial

## P10 ARM controller replay

The current source release `f6869829` has the same cartridge runtime as the
P10 control built from `00fd8efd`. Use the [late P10 pause save and
controller tape](2026-11-20-p10-blocked-depot.md#next-late-combat-stall).
The input SRAM SHA-256 begins `db0a7afa` and the tape SHA-256 begins
`9df379a2`. Load with L at HOME, resume at call 37,705, and play the
same controller input through the results screen at call 47,194.
Read timing at the controller boundary for calls 45,218–45,275.

At the A edge on call 45,231 the control fills three nonempty point
fields of 12,836 cells each. Three additional fields have no seed and
take the existing empty-goal path in both ROMs. All six use the
walkability grid with SHA-256 beginning `bd123cdab5feb791`. The trial
serves the three nonempty goals and later reads with nine successful
local searches, taking 197,479 guest cycles in total. Control point
fills and trial certificates were counted with read-only breakpoints.

| Guest cycles | Control | Certificate trial |
| --- | ---: | ---: |
| Full loop, call 45,231 | 4,762,510 | 2,218,360 |
| Orders, call 45,231 | 3,538,396 | 994,753 |
| Mean full loop, 58-call window | 2,018,288 | 1,975,810 |
| Full loop p95 | 2,705,245 | 2,675,375 |
| Peak full loop | 4,762,510 | 2,740,426 |
| Mean orders | 584,843 | 542,728 |

The maximum moves to call 45,261 and drops by 42.46%. Frames and
logical route build and hit counts match on all 58 measured calls.
Seven samples around the A edge match units, buildings, resources,
walkability, fog, and route validity, age, goal, clock, and counters.
The later samples match through results at simulation frame 46,057.
The control ROM SHA-256 begins `f7185043c021a2e6` and the trial ROM
begins `243815a222c89b46`. The timed control and trial receipts begin
`f0916e187d271743` and `7586df5475b27d58` respectively.

## Cache behavior and qualification

For an open origin near a cold goal on the 128×128 fixed bank, search
the bounding rectangle of the origin and possible seeds plus two
cells on each side. A path at most four steps longer than the seed
Manhattan lower bound cannot leave this rectangle. The search uses
the normal seed order and left/right/up/down FIFO, so a successful
search also has the ordinary tie result. It writes the entire path
into the ordinary packed slot and keeps the existing logical age,
identity, and counters. Each published entry has a continuous chain
back to a seed, so a nearby-goal gradient read can consume it.

The deferred marker uses the existing point-prefix directory. Warm
queries can add another certified chain. Distant and blocked starts,
failed proofs, and full-field readers materialize the existing exact
flood. Shape and topology changes invalidate the marker through the
existing route resets. Scratch uses the head of the shared route
queue and leaves the borrowed tail untouched.

Run `make test-sc64-route-point-certificate` for 256 local search
cases, the 386,646-query route and residency comparison, and the
range-scratch fixture. The local cases certify 194 paths, including
45 blocked goals. The range fixture checks an additional certified
read, a distant read that materializes the field, logical counters,
and both claimed range planes. The exact packed fallback is checked
against an independent full flood.

This experiment enables `SC64_ROUTE_POINT_COLD_CERTIFICATE` only in
the test ROM, over the five point-prefix flags from the current
[#576 trial](https://github.com/ggu/starcraft-gba/pull/576).
P10 links at `0x03004f60` IWRAM and `0x0203b8dc` EWRAM. This is
320 IWRAM bytes and 1,072 EWRAM bytes above the source release,
matching the prior prefix trial's overhead on P10. The certificate
adds no persistent RAM beyond that deferred-field directory.
Keep the flag opt-in until the four normal profiles link with their
stack margins, the dense Z7/Z8/Z9 and skirmish replays pass, and
late range readers confirm that deferred full fills stay bounded in
practice. Qualify fresh candidate save and result banks separately.

## Release `9ee056c2` follow-up

Paired control and certificate builds for all four normal profiles
pass the linker guards and header checks. The certificate itself
runs from ROM. Showcase also keeps the prefix expansion in ROM and
retains its original IWRAM end at `0x03004fb8`. The other three
profiles use 320 additional IWRAM bytes for the prefix expansion.
All four use 1,072 additional EWRAM bytes for the deferred fields.

| Candidate profile | IWRAM bytes below guard | EWRAM bytes below guard |
| --- | ---: | ---: |
| Terran | 1,560 | 19,956 |
| Z10 | 312 | 23,768 |
| P10 | 160 | 10,020 |
| Showcase | 72 | 19,432 |

The guarded-link receipt SHA-256 begins `41c31c2ed882e2d2`.

The forced P10 ARM case uses a 128×128 open grid with a vertical
wall at column 64 from rows 8 through 122 and one opening at row
110. It builds a point slot from the tile immediately right of
the goal at (32,32). Next it asks `sc64_range_cached_point_step`
for a nearby origin at (32,34) and a distant origin at (100,100).
The direct path from the distant origin is blocked.

| Guest cycles | Control | Prefix only | Certificate |
| --- | ---: | ---: | ---: |
| Cold point slot | 1,087,129 | 103,404 | 15,303 |
| Nearby cached read | 1,240 | 1,086,881 | 5,954 |
| Distant cached read | 1,228 | 1,317 | 1,087,475 |
| Combined | 1,089,597 | 1,191,602 | 1,108,732 |

The prefix completes during the nearby read. The certificate
serves that read while the slot stays deferred, then completes it
synchronously during the distant read. Both returned steps,
logical build and hit counts, age, clock, goal, and all 6,144
packed bytes match across the three builds. The field SHA-256
begins `9419296f2efeb40e` and the receipt begins `3425ac9ec8d89d08`.
These are forced reader costs. The genuine P10 controller gain
above measures a different workload.

Keep the certificate opt-in. The distant cached read alone takes
1.94 times the 561,792-cycle update budget for 30 fps. Activation
still needs a bound on distant cached reads that preserves their
returned step and timing of orders, broader dense-battle replays,
and fresh candidate save/result reloads.

## Distant cached-read certificate

`SC64_ROUTE_POINT_FAR_READ` adds a second opt-in to the cached
point reader. It applies to unknown entries in an existing deferred
field with a walkable origin and goal. An ordinary monotone path
certifies the Manhattan bound. Where the preferred L route meets an
interior barrier, every route still crosses the full row or column
through that barrier. Scan that separator for a unique walkable
cell minimizing the sum of Manhattan distances to the endpoints.
Two monotone legs through that cell attain a global lower bound.
The original FIFO chooses its first lexicographic path to that cell
and then the first lexicographic suffix, which the existing monotone
oracle supplies. With an ambiguous minimum, only a direct Manhattan
proof can answer. An exhausted budget, failed proof, blocked goal,
or blocked origin uses the existing fallback.

The helper borrows scratch at the head of the route queue and uses
at most 512 monotone states across both legs. It returns a direction
without writing the packed field or changing its cache metadata.
The existing full-field readers still materialize the field when
needed.

On the same forced wall case the distant read falls from 1,087,475
to 70,103 guest cycles. Cold build and nearby read take 15,303 and
6,382 cycles, respectively, for 91,788 cycles across the three
operations. Both returned steps and all logical metadata match.
The slot remains deferred after the distant read. An explicit
full-field request costs 1,086,096 cycles and produces the same
6,144 packed bytes as the other three builds. The P10 ARM probe
satisfies both linker guards. Its ROM SHA-256 begins `9fab8262eddc2ef9`
and the receipt begins `44fafae911f84f41`.

`make test-sc64-route-point-far-read` checks both wall orientations,
equal lower-bound crossings, blocked multi-seed goals, an open
monotone read, 108 accepted randomized proofs against full fields,
and the existing range scratch suite. Keep this probe opt-in until
normal-profile links and genuine busy replays measure how often the
fast path succeeds and what the remaining full-field fallback costs.

## Current death-depth release replay

The `cd833c8c` source release has the same cartridge runtime as the
previously linked `857f59cf` P10 control. The candidate at `353329e9`
adds all seven route probe flags to that frozen link recipe and uses
the same prepared death and weapon banks. It passes ROM header and
SRAM signature checks and links at `0x03004f60` IWRAM and
`0x0203b8dc` EWRAM with both route probes and the death depth painter
present. The control and candidate ROM hashes begin `cd4f77fe` and
`f346750a`. The normal compile flags stay off for the probes.

Both ROMs load the same historical pause SRAM from HOME and resume at
call 37,705, frame 36,761. Both win at call 47,194, frame 46,057 with
mission bit 29 set. Five named world and route state samples match,
including the palette and both indexed pages. The 58 measured
updates match frame progression and route build and hit counts.

| Guest cycles | Current control | Route trial |
| --- | ---: | ---: |
| Call 45,231, whole loop | 4,753,977 | 2,212,146 |
| Call 45,231, orders | 3,531,921 | 988,954 |
| Mean, calls 45,218–45,275 | 2,012,095 | 1,969,580 |
| p95, same window | 2,700,482 | 2,668,791 |
| Peak, same window | 4,753,977 | 2,737,515 |

At call 45,231, only the three empty-goal linear fills remain. Across
the 9,489-call continuation, a read-only observer sees 41 control
linear fills and 14 trial fills. Both have ten calls to the cached
weapon-range reader on exactly the same tape calls. None reaches
the distant-read helper. Its speedup remains a forced-case result.
The comparison receipt SHA-256 begins `375760e916cc9731`.

The regular Z10 vector omits the ordinary movement monotone flag.
Give the far reader access to the shared monotone oracle separately
before qualifying that profile. Keep normal movement and cache
decisions in the same configuration across the control and trial.
