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
