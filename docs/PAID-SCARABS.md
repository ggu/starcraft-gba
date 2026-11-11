# Paid Scarabs (Reaver ammunition)

Status 2026-11-12: working in the **Protoss P10 product** (campaign missions P1 to
P10 and custom games). Terran, Zerg Z9/Z10 and Showcase do not compile it; see
[Other products](#other-products).

Code: `starcraft/sc64_paid_scarab.h`, enabled by `-DSC64_PAID_SCARAB`
(`mk/paid-scarab/Makefile`, part of `PROTOSS_P10_FLAGS` only). The staged
`SC64_SCARAB_*` Flash-era probes are untouched and cannot be combined with it.

## Rules

| | |
| --- | --- |
| Price, build time | the `U_SCARAB` rule row: 15 minerals, 105 source ticks (131 updates) |
| Capacity | 5 (`sc64_psc_capacity()`; no Reaver Capacity research exists in the product, so 10 is never reached; the saved fields already hold 10) |
| Buying | one Scarab per command, paid from the owner's own bank when ordered; at most 5 on order; stock plus orders never exceed capacity; no cancel command (as for Interceptors), a dead Reaver's orders are lost |
| Attack | a Reaver in reach of a ground target with a Scarab held, none of its own in flight and its 60-tick cooldown expired launches one; with none held it never fires (it still walks into reach) |
| Flight | a `U_SCARAB` row: 3 ticks of launch delay, then 8 runtime pixels (16 source) per source tick toward the target, explodes within 10 source pixels of the target bounds; 90-tick life |
| Splash | the Scarab weapon's 100 Normal damage to every hostile ground actor within 60 source pixels of its bounds: full inside 20, half inside 40, quarter inside 60; shields then armor; burrowed actors only inside 20; air, own and allied actors never |
| Lost | target dead or its slot reused: the Scarab flies on and explodes at the last position; route blocked for the rest of its life: it expires with no damage; Reaver dead: the Scarab goes on, detached |
| Starting stock | production hands over an empty Reaver. A Reaver a mission places, reinforces or rescues (either side) and a Reaver from a save written before this service existed starts full, the first time the service looks at it. |
| Campaign enemies | start full and never buy: their money is a scripted source bank, not an engine bank. A finite load keeps enemy Reavers out of free ammunition. |
| Custom opponents | a Protoss opponent builds a Robotics Facility and a Support Bay, trains up to three Reavers (one per three Zealots/Dragoons) and buys their Scarabs through the human's command from its own bank |

Approximations: straight-line homing at constant speed (no turn rate or
acceleration), centre-distance reach, the integer combat path (no fractional HP),
and a one-in-flight limit per Reaver. The starting-stock rule for placed Reavers
is an assumption (no source data was checked).

## Storage and saves

A Reaver keeps its state in `carry` (bits 0-3 held, 4-6 on order, 7-12 cooldown,
13 established) and `work_timer` (progress of the front order). A flying Scarab
is one unit row (`build` parent or -1, `target`, `tx/ty`, `work_timer` life,
`flags` 16). `RaceSave` does not grow; all fields are in the typed unit row the
P10 save already writes. The candidate validator (`race_paid_scarab_candidate_valid`,
called from the Carrier validator) rejects over-capacity, over-cooldown, stale or
unestablished-but-nonzero Reavers, and Scarabs with bad life, parent, target,
flags or duplicates per Reaver. A save written by the base ROM loads: its
Reavers hold zero in both fields, which means "not yet established".

## Other products

See the measurement section of the pull request text; summary in
[PROJECT-TODO](PROJECT-TODO.md) row C2.

## Checks

`make test-sc64-paid-scarab` (sanitized HOST product test, fresh-process save
and old-save check with `PSC_BASE_TREE=`), `make test-sc64-protoss-px` (all ten
Protoss mission controllers), and the Protoss profiles of the skirmish matrices,
which now build the P10 product (`match-reaver` cases).
