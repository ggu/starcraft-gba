# Scarab source-order mapping prerequisite

Pinned OpenBW `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`,
`bwgame.h` `order_ScarabAttack`, uses source state 0 for initialization,
2 for the no-collision period, 3 for normal pursuit and 6 for awaiting
the arrival animation signal. Initialization sets remove timer 90 and order
timer 7; global unit value updates decrement those timers before orders.
Arrival may occur while still in state 2. Reaver STOP changes outside children
to the separate SelfDestructing order, which kills them. Target loss retains
the move target; it is not a native state transition.

`sc64_scarab_order_state.h` explicitly maps these four native states to bridge
codes 1, 2, 3 and 4; bridge code 0 stays reserved for hidden children. The
earlier bridge documentation already used 1 for fresh release but did not
establish the remainder. The current experimental namespace rejects all
outside children, so no accepted staged save is reinterpreted by this mapping.
No current gameplay or save callsite enables this mapping or flight.

The asset-free sanitizer fixture checks all four mappings, reverse mappings,
and rejection of other byte values. The retained source-only far oracle trace
independently contains native states 0, 2, 3 and 6; the east trace demonstrates
arrival from state 2 while collision grace remains. Neither trace alone proves
general movement, collision, impacts, source/N64 equivalence or hardware timing.
The oracle logs remain private. Before registration, implement actual bounded
source movement and scheduler integration, post-arrival animation/bullet and
failure semantics, lifetime and STOP/parent cleanup, and fresh-process save
qualification with an explicit compatible wire policy.

## Actual Unit control experiment

The separate, normally undefined `SC64_SCARAB_FLIGHT_CONTROL_PROBE` gate
changes the unit adapter's launch callback to retain source fresh state until
the first explicit control tick. The control function then initializes 90/7,
decrements timers on subsequent supplied source ticks, transitions out of
collision grace, accepts a caller-supplied arrival result, and returns
animation or removal requests. It does not move units, calculate arrival,
dispatch itself from the game loop, generate damage or clean up a removed
unit. Callers must not treat those requests as completed engine actions.

The ASan/UBSan actual-engine Unit fixture uses the real paid enqueue, build,
launch, STOP and parent-loss adapters, and checks initialization, 7-tick grace,
arrival both during and after grace, signal/expiry/self-destruct removal
requests and detached parent identity. Compile argv and run outcome are kept
in `.local/decoder-proof/scarab-control-{compile,run}.json`. This is an
explicitly invoked control experiment, not an automatic flight implementation,
position or target-follow proof, or qualification for outside-child saves.

## Experimental dispatch slice

Under the same disabled probe gate, the actual unit update loop invokes the
control on the existing BC source cadence. A fresh zero-life release can be
found by STOP and parent-loss hooks before its first order tick. While a target
is live, the probe stores its current position; on target loss it clears the
reference but retains that position. On STOP or expiry it requests removal
through the existing unit death path and clears pending-impact and target-index
state. The actual `game_update` host fixture checks fresh STOP, one pending
impact reference, fresh parent detachment, live-target position retention and
expiry. Compile and run receipts are `scarab-dispatch-{compile,run}.json` in
`.local/decoder-proof`.

The probe does not move toward the target, detect arrival or schedule an
animation or bullet. It can consequently expire while stationary and must not
be enabled as gameplay. Other hidden-target classes, complete death/reference
cleanup, cadence across profiles, Q8 motion and native/N64 execution remain
unqualified. The test's terminal screen is reset to PLAY for its second update;
it is not an ordinary playable match.

## Host-only aligned clear-motion slice

The independent, host-only `SC64_SCARAB_CLEAR_MOTION_PROBE` adds a much
narrower motion experiment to that dispatcher. It accepts only an explicitly
supplied, already-aligned east or south source heading and a target at least
64 source pixels away. It keeps source coordinates in Q8 for its calculation,
waits through the observed startup, then moves by 16 source pixels per due
source tick. The adapter converts the integer GBA position at half scale and
checks every intermediate host-engine point against conservative terrain,
unit and building guards. It does not implement turns, arbitrary fractions,
source pathing, blocked-route recovery or arrival clamping. The host test
supplies the heading in an otherwise unused field; this is not a product field
allocation or save format.

The clear-motion fixture's four post-initialization positions match the
retained pinned OpenBW far/east and south source-frame traces. The comparator
is `tools/sc64_scarab_clear_oracle_compare.py`; the separate actual game
update fixture moves an aligned actor by eight GBA pixels on the corresponding
due tick. Sanitized build and run receipts are `scarab-clear-{compile,run}.json`
in `.local/decoder-proof`. The source traces show only this bounded straight
case; 27-unit source turning, general Q8 fractions, obstacles, pursuit and
impact still require reference-backed implementation. The probe can stall or
expire outside its narrow case and must not be enabled in a product.

## Source-waypoint steering experiment

The separate host-only `SC64_SCARAB_STEER_MOTION_PROBE` tests a source-Q8
movement kernel using the existing pinned direction tables, type-2 turn
limit 27, and the Walking script's 16-source-pixel speed command. It tracks
velocity direction, visual heading, turn state, subpixel position and
waypoint transitions. It takes its next source waypoint from the host fixture;
it does not compute a path or decide arrival. The game-update adapter keeps
this experimental state in host-only storage and retains conservative engine
collision/terrain checks. Its waypoint and initial heading are injected test
inputs, not game or save-format fields.

After authenticating the owned USA ROM (normalized SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and pinned OpenBW revision,
seven synthetic-flat-map full-world runs captured source Q8 position, heading,
velocity and next waypoint. A comparison using the oracle-supplied waypoints
matched every observed source tick from state-2 initialization through state-6
arrival: far/east 8, south 5, diagonal 11, north 7, west 8 and slope 10.
The seventh is a 9-tick controlled injected initial-heading turn, not a
natural player action. The slope case includes an intermediate waypoint turn
and the cases include final waypoint clamping. These are finite comparisons,
not a universal path, movement or N64-equivalence proof. The actual game
update sanitizer fixture also checks one translated diagonal Q8 step.

Oracle argv and logs are retained under `.local/scarab-motion-oracle-20261031`;
`tools/sc64_scarab_steer_oracle_compare.py` replays their waypoint inputs.
The host build/run receipts are `.local/decoder-proof/scarab-steer-{compile,run}.json`.
Neither stage is registered for product gameplay or saves. Actual source
waypoint generation, obstacles, target following and loss, animation/bullet
allocation, cleanup, and all-product fit remain open.

## Packed Unit motion probe

The disabled `SC64_SCARAB_PACKED_MOTION_PROBE` replaces the previous host-only
per-slot steering array. For outside probe children, `carry` holds the parent
and phase in bits 0–9 and the two source-Q8 fractions in bits 10–18 and 19–27;
`q.x*512 + frac_x` and `q.y*512 + frac_y` reconstruct source-Q8 positions.
`cd` holds the heading and velocity bytes, `weapon` holds turning and
previous-waypoint-valid bits, and `path_timer/path_retry` hold the prior source
waypoint. The fixture supplies the next waypoint in `path_x/path_y`; age comes
from the source lifetime. These fields are an experimental runtime contract,
not an outside save layout or a normal product feature.

The existing hidden-child decoder remains strict. A probe-specific reader
accepts fractions only for outside/self-destruct rows and the setter preserves
them through timer, STOP and parent-loss transitions. Outside saves still fail
closed. Through actual `game_update` calls, the packed replay matches all seven
retained OpenBW waypoint traces in source Q8 positions and headings through
their observed arrival frames; the inputs remain oracle-supplied waypoints.
The separate actual Unit fixture checks fresh STOP, parent loss, retained target,
expiry, preserved fractions and outside-save rejection under ASan/UBSan.

A direct link using the Terran normal Make recipe plus the explicit probe flags
initially failed its unchanged IWRAM stack guard. Keeping the experimental
dispatcher out of the hot section allowed the link to pass at
`__iwram_end=0x03004e30` and `__bss_end=0x020370c0`; no guard or capacity was
reduced. This single, older-branch experimental link is not a current-product
matrix, gameplay performance measurement or hardware qualification. The
failed and passing link receipts are `scarab-packed-arm-link-first-failure.json`
and `scarab-packed-arm-link.json` in `.local/decoder-proof`; no generators ran.
Source path generation, obstacles, arrival and weapon scheduling, complete
cleanup and outside-save migration remain open.

## Autonomous route-to-arrival experiment

The additional disabled `SC64_SCARAB_AUTONOMOUS_PROBE` derives an initial
heading and target from the launching parent and requests waypoints from the
existing GBA direct-path and bounded route helpers. These are GBA pathing
adaptations, not OpenBW's path generator. For an axial live unit target it
chooses a range-boundary endpoint to avoid the GBA collision guard preventing
arrival. Arrival uses the source inclusive raw-unit bounds, Scarab bounds 2,
integer length approximation and ten-source-pixel range; detached target
loss retains the last destination and requires reaching it exactly. Buildings,
diagonal endpoint behavior and general obstacles remain unqualified.

The actual game-update fixture trains and launches a paid child, explicitly
detaches its parent to isolate the unadmitted generic Reaver attack, and needs
no injected heading or waypoint. One axial living-target case reaches the
source range boundary; a target-death case reaches the retained position. Both
then wait until lifetime expiry and retire through the probe's death path.
Neither emits weapon 82 or changes victim health: state 6 animation, bullet
allocation and delayed splash are not implemented, so this is not combat or
a complete Reaver service. The fixture resets the result screen to PLAY each
update and does not constitute a normal match.

Sanitized compile and both run receipts are in `.local/decoder-proof/`
`scarab-autonomous-{compile,runs}.json`. A direct Terran normal-recipe link
with explicit probe flags retained unchanged RAM guards at
`__bss_end=0x020370c0`, `__iwram_end=0x03004e30`; its argv and output are in
`scarab-autonomous-arm-link.json`. This is one older-branch profile, not a
current-product matrix, ARM replay, performance or hardware qualification.

## Paid-child attack and delayed splash experiment

The normally disabled `SC64_SCARAB_WEAPON_PROBE` extends the autonomous probe.
It diverts Reaver combat before the generic hit path, selects an existing paid
inside child, and launches it; generic Reaver damage does not also occur. At
source-range arrival the probe creates a separate bounded four-packet pool
entry and requests child removal on the next source tick. A full pool produces
no fabricated damage. Pending packets and the probe's test-only per-owner
Scarab-damage upgrade bit are not saved; a save is refused while either exists.

The probe's five-source-tick impact delay is the bounded observation in the
pinned full-world far trace (arrival frame 120, victim damage frame 125), not
a universal derivation of IScript and bullet scheduling. The experimental
resolver uses the source unit rectangle and length helper, enemy-splash radii
20/40/60, 100 base or test-injected Scarab upgrade 125, the engine's existing
normal-damage armor/shield path, same-owner exclusion and ground-only unit
filtering. At that checkpoint, buildings, explicit-friendly-target exceptions,
owner/slot reuse, upgrade research/persistence and fractional half/quarter
damage fidelity were not implemented. The fixed pool and separate owner bit are global experimental
state; no per-child array was added. This does not register normal gameplay.

The ASan/UBSan actual game-update fixture uses ordinary attack-move input with
the parent alive and a real paid child. A synthetic durable Marine witnesses
100 and 125 damage exactly five source ticks after arrival, no earlier or
duplicate damage, and unchanged friendly ground worker and enemy flying unit.
The upgrade bit is injected after launch to test impact-time lookup; it is not
a research or save test. A synthetic full-pool case causes no damage. Receipts
are `.local/decoder-proof/scarab-weapon-{compile,runs,arm-link}.json`. A direct
single Terran old-branch normal-recipe link with all probe flags kept the
existing guards (`__bss_end=0x020370e0`, `__iwram_end=0x03004f00`), only
256 bytes below the strict IWRAM limit; this is not a current-product matrix,
ARM replay, timing or hardware qualification. All probes remain off.

### Building and explicit-target extension

The same disabled experiment now admits grounded building targets and includes
them in splash, using their source raw bounds and the existing building damage
and destruction path. Its axial GBA route adaptation checks the target's actual
source rectangle rather than treating the engine's larger navigation padding
as physical collision. This is not source-equivalent building pathing; diagonal,
blocked and airborne cases need further work.

Pinned OpenBW `bullet_deal_splash_damage` excludes same-owner units except
the bullet's explicit target. The probe now carries that target identity and
invalidates it on the existing unit/building damage-death and slot-reuse hooks,
so an ownership change of the same live target is distinguished from a reused
slot. Other removal/transfer paths must still be audited. Source damage divides
the Q8 packet before armor and shields; the probe now retains Q8 through that
calculation, but the engine stores integer HP and shield values, so fractional
residue is discarded at that boundary. It does not claim exact repeated
fractional shield or HP outcomes, shield-upgrade semantics, hallucinations or
all source modifiers.

Nine ASan/UBSan actual-update cases cover the previous base/upgrade/full-pool
checks plus a paid child targeting and damaging or destroying a building,
an explicit target changing to the shooter's owner, target death followed by
same-slot friendly reuse, shield absorption and a 62.5-Q8 middle splash
quantized to 62 integer HP. The controlled ownership change, reuse and durable
health values are fixture setup. No generic parent hit is allowed. The retained
one-case five-source-tick delay is not general source-bullet scheduling proof.
Compile and run receipts are `scarab-weapon-building-{compile,runs}.json` in
`.local/decoder-proof`. The single older-branch Terran experimental ARM link
at `scarab-weapon-building-arm-link.json` passes unchanged guards at
`__bss_end=0x020370e8`, `__iwram_end=0x03004f00`; this is neither four-profile
qualification nor evidence to enable the probe in products.
