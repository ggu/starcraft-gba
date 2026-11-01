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
There is no Z8 victory or next-mission admission claim.

## Bounded host result

At frozen source `7e873a4`, the canonical Z8 ASan/UBSan host trial compiled
and ran under the exclusive native lock, with the 109 pinned entries and
five existing sidecars checked before and after. It cleared the original
first Templar at the unchanged frame 2188. The ordinary retreat was accepted
at 2347, and the original hero entered the 32-pixel arrival threshold at 2678.
At the final frame 11009 she remained alive, raw identity 51, source owner 3,
human-owned, with 328 HP at `(1318,1675)`. The earlier hero-death failure did
not recur in this run. This only establishes survival through this trial.

The run still ended in source defeat, with no scheduler fault or sanitizer
report. Human instance T16 entered its sequence at 10990 and completed at
11009, changing outcome from running to defeat; T1 did not change then.
T16's source condition concerns a hero Dark Templar in location 19, the
northeastern hive region. The observed northern Templar became detected at
10760 and lost some shields, then left detection with about 32 shields and
reached that region. The southern detector remained alive; all five original
southern ground escorts were removed between 9194 and 9344. Their later
slot contents are not surviving escorts.

The explicit verdict is `LOSS`, return status 1, at native frame 8807 after
11161 ordinary update calls and 7202 regular-loop iterations. The retained
log is `/tmp/sc64-z8-hero-retreat-7e873a4-20261101.log`; the separate local
result receipt binds its hash and the driver. This is a host policy result,
not a physical GBA run, a current merged-branch binary qualification or a
demonstration that the position remains safe through later waves. The next
policy needs an earlier northern detector/attacker interception and continued
coverage of all three hive regions while pursuing both enemy Nexuses.
