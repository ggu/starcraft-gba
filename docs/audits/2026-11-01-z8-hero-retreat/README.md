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

The first branch version used the parked trial's phase-enabled source. Later
follow-ups explicitly identify their rebased engine below. No
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

## Controller selection correction

Source inspection of the northern trial found a controller timing error:
`select_one` could spend up to 240 selection cycles trying to select a dead or
enemy-owned slot. Later northern observations showed enemy replacements in
slots 21 and 22, but did not establish that they were unavailable when their
opening orders ran. The corrected helper rejects a non-live/non-human
actor before issuing input and aborts if its source identity or ownership
changes during selection. It preserves ordinary L-button selection for valid
actors. It cannot distinguish replacement by an identical source identity in
the same slot without a generation ID.

This shortens previously futile UI sequences and can change all subsequent
timing, economic and combat behavior; earlier results do not qualify it. The
same northern and hero staging policy was retained to test this correction
before choosing another retreat or offensive plan. It is a host controller
fix only.

The corrected source `f8b465b` compiled and ran under ASan/UBSan without a
report, with source and pins unchanged. It rejected an unavailable traveler
in slot 6 at 7899. That removed 480 futile selection updates, but the ordinary
idle loop absorbed those updates before the next scheduled action. The named
first interception, northern kill at 10643, original hero death at 12853 and
T1 defeat at 12937 were unchanged. The regular-loop count rose from 8444 to
8924, while total ordinary updates stayed 13083. This is not whole-world
equivalence or a new mission win. The local log is
`/tmp/sc64-z8-selection-guard-f8b465b-20261101.log`.

## Early Nexus offensive trial (unrun)

`tests/sc64_z8_nexus_push_probe.c` proposes an offensive continuation using
the same ordinary controls. After the first southern raid, it sends Kerrigan
to `(1904,1968)` rather than the previously unsafe retreat position. The
static terrain grid has a cardinal path to that cell, but dynamic reachability
and long-term safety are unverified. At frame 5500 it attack-moves surviving
original northern heavy units and Zerglings toward the source-owner-2 Nexus
at `(1712,1208)`. If that Nexus is destroyed, surviving actors receive ordinary
attack orders toward the source-owner-1 Nexus at `(400,1704)`, accompanied by
the surviving southern detector. The source liveness check is explicitly a
host-world oracle; this is not a fair-play AI or a player scouting example.

This changes several tactical and input-timing choices together. It may
abandon a hive, expose the army or detector, or fail to acquire a valid path.
No HP, damage, resource, source objective or AI permission is changed. An
offensive order being accepted will not be treated as destruction or a win;
the existing strict engine/source victory predicate and bound remain intact.
At its initial source checkpoint this trial was uncompiled and unrun.

The offensive trial's order helper rechecks the selected actor's live
ownership/source identity and the armed mode after cursor travel, before
issuing A. It records the click coordinates and the ordinary direct-target
selector's result, then verifies the same live actor's observed move/attack
destination or direct target. A previous unrelated attack order no longer
counts as applied. This is conservative state confirmation after the two
input updates: a target that dies or a victory reached during the tap can
leave the order unconfirmed even when it was issued. It is not a generation
identifier or proof that the attack caused a later death. Build-consumption
observations retain their earlier separate rules.

At frozen `90f361b`, the individual offensive trial compiled and ran under
ASan/UBSan without a report, with source and input hashes unchanged. The hero's
farther retreat was issued at 2493 and entered its 32-pixel arrival threshold
at 3377. Eight individual attacks toward the eastern Nexus were observed
applied from frames 5603 through 6316. Those were real movement orders: by
6360 both original Ultralisk slots held enemy replacements and both original
Hydralisks were dead near the approach. The last ordered Zergling was still
alive at 6360 and dead by 7200. The uncommitted Lings 21 and 22 remained at
their staging position at 6360. The observations establish army losses, not
a pathfinding stall or a unique killing attacker.

No eastern-Nexus-cleared event occurred. The strict result remained LOSS at
13412/native 10729: T17's hive-region sequence began at 13393 and completed
at 13412, with location-18 census one. Original Kerrigan remained alive at
`(1897,1968)` with 328 HP. The log SHA-256 is
`320e7a8a409ad198bc13f488dd362040a87071b6e08e072728f3bf109ee46b63`.
The subsequent boxed-offense experiment is recorded separately; neither
trial is a Z8 win or promotion.
