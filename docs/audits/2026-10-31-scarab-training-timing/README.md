# Scarab training timing correction

This corrects a source-only, gated adapter and does not wire it into ordinary
gameplay or enable Reaver admission. The authenticated raw-85 datum remains
20 maximum HP, 10 shields, 15 minerals and build time 105 source ticks.

The pinned OpenBW PC reference initializes a nongrounded unfinished child
hidden at one tenth maximum HP, while initializing its shields to their full
value. For this actor, that is 512 Q8 HP (2 HP) and 10 shields. The source's
integer fixed-point construction-rate formula yields 44 Q8 HP per progress
call. `resume_building_unit` adds and clamps HP, then returns after decrementing
a nonzero remaining time: the 105th progress call leaves an unfinished child
at full HP and zero remaining time; the next call completes it and removes the
paid front entry. The allocation call occurs earlier. This is a reference-
scheduler result, not a claim of native N64 execution or proof of whole-engine
source-clock conversion.

The helper now represents progress 0 through 105, with 105 retained as an
active ready-to-complete state. The integer `Unit.hp` stores the floor of the
reconstructed undamaged Q8 value, `min(512 + 44 * progress, 5120)`. This loses
fractional HP in the actor field, but the exact undamaged Q8 value can be
reconstructed from parent progress. The gated prototype refuses to advance
if the child's integer HP differs from that expected value; this deliberately
does not model external training-child damage or healing. A complete runtime
and save design must address those mutations and validate parent/child state
before accepting persisted worlds.

The independent reference fixture follows the remaining-time decrement/return
flow, checks every construction call and the 105/106 boundary, and compares Q8
values. Field tests exercise progress 105 through the existing raw-85 and
parent codec rows. The real-`Unit` fixture covers paid allocation, retry,
ready-state cancellation, and no-mutation rejection of unexpected HP. Prepared
profile fixtures and local default checks are recorded separately for their
exact tested source; these checks do not qualify gameplay admission, real SRAM,
art, combat or hardware performance.
