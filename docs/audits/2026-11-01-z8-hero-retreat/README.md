# Z8 original-hero retreat trial

This source-only controller trial changes no engine, mission, unit statistics,
source permissions or gameplay cadence. It builds on the observed parked-hero
trial at `efa160c`: its original Kerrigan cleared the first exposed Dark
Templar at frame 2188, remained near the western hostile base, took damage
starting at 7907 and died at 8660. The authored hero-absence defeat then
completed at 8713. The replacement actor later in slot 10 was not Kerrigan.

After the first raid is cleared, this trial issues one ordinary Move command
to the original hero at terrain candidate `(1320,1680)` before continuing the
existing opening. It logs command acceptance and actual arrival separately;
a terrain-only path is not evidence of dynamic reachability or safety. The
hero must still be alive with source identity 51, source owner 3 and current
human ownership. Its final identity is checked independently of the slot's
alive bit. No teleport, statistics change or hidden-target attack is used.

The controller reads engine world state to decide its inputs and observes
the source scheduler. It is a reproducible host trial, not a fair-play AI.
The added menu/cursor input consumes time, so this is a policy change rather
than an isolated movement-mechanics benchmark. The existing detector, other
defenders, economic opening and later relocation policy are retained. They
may still fail later raid locations. The 20,000-iteration bound is retained;
bound exhaustion is not a win.

The branch remains based on the parked trial's phase-enabled source. No
normal campaign menu, main branch, release or SD contents are changed.
The new trial has not been compiled or run yet. There is no Z8 victory,
safe-position or next-mission admission claim.
