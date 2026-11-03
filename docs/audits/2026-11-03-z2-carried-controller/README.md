# Carried Z2 controller across build vectors

At source `63665045`, `build/test-sc64-z7-carried-z2 --controller 1` and
`build/test-race-campaign --controller 1` stopped with `Z2 carrier selection
failed`, while `build/test-sc64-zerg-carried-z2` passed. This is a defect in
`tests/race_campaign_run.c`, not in the engine. No flag in the Z6 vector protects
it: Z6 passed by timing and fails the same way at 33 of 144 start timings.

## Cause of the reported failure

The failure printed `selected=0` with nine units selected, which looks like A
being consumed by a group order. A was never pressed.

`controller_select_worker` holds R, which moves the cursor four pixels a frame,
and steps each axis toward the unit until the squared distance is at most 8. It
stepped every axis that was not exactly aligned. Against a stopped unit whose
offset is odd on both axes, both axes overshoot together and the cursor cycles.
The reported Z7 run shows it:

| Step | Cursor | Carrier | Offset | Squared distance |
| ---: | --- | --- | --- | ---: |
| 117 | 1206,1138 | 1207,1141 | -1,-3 | 10 |
| 118 | 1210,1142 | 1207,1141 | 3,1 | 10 |
| 119 | 1206,1138 | 1207,1141 | -1,-3 | 10 |
| 120 | 1210,1142 | 1207,1141 | 3,1 | 10 |

The 120-step guard then expires and the function returns without pressing A.
Whether a run hits this depends only on where the carrier stops relative to the
cursor's position modulo four, so any change to the simulation can move a run
into or out of it. The vectors differ in simulation, not in this logic.

`controller_select_worker_box` had the same loop and now shares the fix.

## Start-timing sweep

The controller was rerun with 0 to 143 idle frames added at mission start, in
scratch builds without sanitizers that reproduced the reported lines. Offsets
give about 50 distinct outcomes per vector, so treat the counts as coarse.

| Controller | Z7 wins | Z6 wins | Races wins |
| --- | ---: | ---: | ---: |
| Base | 101 | 104 | 95 |
| All changes | 143 | 144 | 144 |
| Without the cursor change | 110 | 115 | 105 |
| Without cannon clearing | 136 | 131 | 140 |
| Without the result-screen guard | 143 | 144 | 144 |

Base failures split into the cursor cycle (Z7 41, Z6 33, races 48) and a carrier
already dead when selected or walking (Z7 2, Z6 7, races 1). A further 576 offsets
(144 to 719) were run on the final controller, and every vector won all of them.

## Other causes the sweep exposed

Fixing the cursor left 4 to 13 failures per vector, reported under the same
message or as `Z2 convoy stopped`, in which the carrier had already died.

**Photon Cannons kill the carrier.** A Drone has 40 HP. A cannon does 20 damage
every 28 ticks at range 112, per `race_tick_defenses()`, and the traced hits were
28 ticks apart. The cannons at 752,816 and 832,992 cover the chokepoint around
route legs 12 to 14, so no walkable route avoids them; a run survived when the
cannon happened to pick an escort. The escort now destroys any cannon within 128
pixels of the carrier's next walk while the carrier waits out of range. The
margin covers walks that bend away from the sampled line. This adds about 700
ticks to a median win. It is not a claim about how the mission should be played.

**A win could be dismissed.** With cannon clearing checked only at waypoints, Z7
offset 24 won by tick 9,000 and sat on the result screen until tick 11,000, when
a scripted press started the next mission and reset `won`. Input now stops once
the mission ends. The final controller's 144-offset sweep does not reach this
state, which is why that row matches, so this corrects an observed failure and is
not a source of the gain. `tests/race_z1_controls.c` had the same defect.

## Still failing

- Z7 offset 88 loses the carrier at about 1430,1042 on leg 19 to two raw-66 enemy
  units that catch it late in the route. That is one of 720 Z7 offsets, ordinary
  combat with no scripted cause, and it was left alone.
- `--controller 3` (P2) fails on the races vector with `lost=1` at tick 45,892,
  before and after this change, and at all 12 offsets tried. The base run never
  reached it because `make test-races-controller` stopped at Z2. It is separate
  and still needs its own diagnosis, so that target stays red.
- `--controller 2` (P1) on the races vector now passes at offset 0, but that is
  not qualification. It wins 19 of the first 36 offsets, against 17 without the
  cursor change, so it remains timing-dependent for reasons not examined here.
- `test-sc64-z7-canonical-controller` is unrelated and was not changed here.

## Results

These are bounded native host results, not a physical-key replay or hardware
measurement. The engine and generated inputs are unchanged, and no ROM or mission
is promoted. The commands and their results for the tested commit are recorded in
the pull request.
