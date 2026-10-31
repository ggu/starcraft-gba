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
