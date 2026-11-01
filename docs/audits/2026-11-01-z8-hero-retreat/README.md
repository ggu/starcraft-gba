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

## Northern interception follow-up

`tests/sc64_z8_north_intercept_probe.c` retains the ordinary hero retreat and
first southern interception, but stages the northern Overlord at `(1640,832)`
and the northern ground group at `(1664,832)` rather than `(1770,500)`. It
also removes the former single-Hydralisk pursuit toward `(1900,350)` at 8600.
The stage points are fixed controller inputs; no order follows a hidden
Templar's coordinates. Read-only logs include raid identity, detection and
HP/shield transitions, plus sampled defender positions, targets and cooldowns.

The old observed raid crossed `(1656,831)`, but that does not prove that the
adjacent formation target is reachable with current dynamic obstructions.
At a stationary detector's exact 144-pixel radius, the old trajectory would
enter detection earlier; its total detection residence would be similar.
The previous northern engagement removed only about 48 shield points, so
earlier vision alone does not establish lethal interception. Defender arrival,
attack-move pursuit, survival, collision and paid economic development must
all be observed in a fresh ordinary-input trial.

The controller branch was rebased onto experimental engine `2a21677` before
this run, including the admitted prepared visibility rows and caller binding
fix. At frozen trial `fced3c7`, compilation and ordinary-input host execution
passed ASan/UBSan without a report; the engine's source scheduler fault stayed
zero. All pinned inputs were unchanged. The first southern raid and hero
retreat retained their earlier timing. The next northern Templar, source
identity 74/owner 1, spawned into slot 54 at 10448, became detected at 10569,
lost its 80 shields and 40 HP, and was dead at 10643. T16 did not transition
in this run. The actual formation included two original Ultralisks, two
Hydralisks and surviving Zerglings; slots 21 and 22 contained enemy replacements,
not the original Zerglings.

The mission still lost: the original Kerrigan took damage at 12654, was dead
after 12853 (prior HP 32, then -66), and T1 completed the hero-absence defeat
at 12937/native 10349. The regular loop completed 8444 iterations and 13083
ordinary update calls. A later northern raid was already present at the end.
This qualifies the observed northern interception, not redundant long-term
defense or a mission win. The local log is
`/tmp/sc64-z8-north-intercept-fced3c7-20261101.log`, SHA-256
`d0773a8b17f8cf6d5e9d6d34004297b91a152e3ab57f566b03b9b9fac5f0c47e`.
