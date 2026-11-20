# P10 short-path certificate trial

## Native controller replay

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
