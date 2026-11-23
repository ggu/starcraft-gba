# Carried Z8 on the normal Z10 cartridge

The retained Z10-carried controller tape for Eye for an Eye still wins on
`6e843d814da131950c92da79901985fc2e05ff78`. A new ASan/UBSan HOST run used
the same Z10 flag vector as the normal cartridge, including packed building
rendering. A local test hook hashed the covered route state immediately after
each game update and again before the next. No observer changed that state.
The run produced the same 561,911-byte tape and reached frame 45,468 at
call 46,487. It destroyed both source Nexuses, retained Kerrigan at 400 HP,
and reported no scheduler faults. The controller and game source did not
need changes.

The normal Z10 cartridge reached the same WIN from a fresh HOME boot in
mGBA. The 46,487-call HOST tape contains 5,605 held-key transitions and an
ordinary pause-menu save; the cartridge followed those buttons without
loading a save. Seven additional A presses and releases dismissed authored
text at the same simulation frame, bringing the total to 46,501 physical
updates. The runner made no guest-memory writes.

All 19 HOST checkpoints matched across 16 named fields, including each
Nexus's health, Kerrigan, the economy and supply, and the three guard counts.
The southeast guard moves after a raid. The observer used the actual HOST
guard center at each checkpoint to count nearby ARM actors; a first pass
using the initial center understated that guard's late count. The center
only defines where to observe, and no controller logic runs in the ROM.

The result had both starting Nexuses gone, Kerrigan at 400 HP, the human
scheduler outcome set to victory, the primary objective and campaign
completion bits set, and zero scheduler faults. Other enemy buildings
remained; destroying the two Nexuses is the mission objective.

The mission uses the authenticated Z8 trigger program, which binds 39
trigger rows. The adapter treats each destroyed starting Nexus as a human
kill because only the human fights those two Nexus owners. This is the
documented stand-in for the original game's per-player kill census; the
route does not claim native kill attribution or AI parity.

The full replay averaged 1,242,715 guest cycles per main loop (13.50
emulated updates per second). That figure includes rendering, the UI and
presentation waits. It is below the near-30-fps target and is not a
physical GBA frame-rate measurement.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- HOST tape SHA-256: `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb`.
- Full physical win receipt SHA-256: `d5e20e7bdb6ece945732c92f535f99afe8f6c81a4e8ef901876483a2b80ebfc1`.

This qualifies one complete route from HOME. The replay did not test
loading a save on the Z10 cartridge.

## Exact November 13 KODAK cartridge

The same held-key tape also won on the exact `cbba1c5` KODAK Z10
cartridge from fresh HOME. All 19 host observations matched across 16
named fields. The run used 46,487 tape calls and 46,501 physical
calls, including seven authored dialogue acknowledgments. It issued
the tape's ordinary pause-menu save and did not load SRAM.

Victory occurred at frame 45,468 with both objective Nexuses destroyed,
Kerrigan at 400 HP, the human scheduler outcome set to victory, the
objective and campaign completion bits set, and zero faults. Mean loop
cost was 1,229,243 guest cycles (13.65 emulated updates per second).
This verifies one route on the existing ROM; it does not qualify
hardware speed or a fresh save continuation.

| Pinned input or receipt | SHA-256 |
| --- | --- |
| KODAK Z10 ROM | `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907` |
| Matching ELF | `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1` |
| Port-free observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Guard center plan | `5f899fffa2d96000071ac5671d4cdd6da2af59925b21e794391cf3154ae554f9` |
| Replay harness | `c4335751285134f2fe83864ccad228a0503c8a993a96012463f5ac3800f0c73e` |
| Full physical receipt | `6a5ad24890bbf7ce333ff6b6450d9d0b44542fb653fab3b300b9a09c5a31b6df` |
| Ordered checkpoints | `3b13b9c953e1af58f2bbec9e452ed078acd4825219c37697202fa2c19457e303` |

## November 20 normal Z10 with fresh SRAM

The regular Z10 ROM with the Sunken health rule wins both from HOME
without a reload and after a fresh-emulator HOME load at frame 16,659.
Both runs use 46,487 tape calls and match all 19 retained scalar
checkpoints. The reload replaces the pause-exit START at call 17,122
with title-screen L after the tape has written SRAM through the pause
menu. Both runs win at frame 45,468 with zero unsupported-mission faults.

All 30 shared post-load checkpoints match the result flags, wallet,
unit array, building array and mineral fields. The 16 shared framebuffer
cuts after reload have identical palettes and visibility masks. When
compared by displayed and hidden page, 13 cuts match both pages.
The displayed page differs at three cuts, with 692, 329 and 193
different indexed pixels at calls 22,000, 32,000 and 38,000.
The changed pixels are in the battlefield area. Their cause remains
open.

Over 45,467 advancing play updates per run, mean loop cost is
1,025,307 guest cycles without the load and 1,025,157 with it.
Those means represent 16.36 and 16.37 updates per emulated second.
The first run averages 485,086 draw cycles and 279,867 orders cycles.
This Z8 route is below 30 fps in the emulator and hardware timing
remains open.

| Pinned input or receipt | SHA-256 |
| --- | --- |
| Z10 ROM | `1d356f0bcb2e14491e6fa0e5ecc2414e0e09730f0b141878d813b33de02bc763` |
| Controller tape | `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb` |
| Port-free observer | `ff2c5a8b1f6764ed399d8a1b59df6ffde135ae2cd01a6d67500684b8429d23b7` |
| Pause SRAM | `bf99add2606817dc705ba1c03d5a15ca8f72a6133c2e8b1b73915b2606f130d4` |
| Uninterrupted receipt | `764572ad403651e59227dc25dd28d346ac41691d47e7c6b29a8ff128c10ec63e` |
| Fresh-load receipt | `3afbb7b70bf3a0197d13e5bf4b658c8d6c7da6d1994c60e1d8833af5f4cb6d20` |
| World checkpoint comparison | `7806a0659f5e59f7593d0e2dc0b75d578c0fbd458ede3f42e9fe704ad833c2a8` |

## November 23 current controller comparison

The older physical-button tape no longer follows the prior victory
route on the current regular Z10. The route-table build and direct
Bring candidate agree on call indices, physical input counts, frames,
screens and dialogue counts throughout their 36,496-call common prefix.
The emulator's per-file log limit stopped both comparisons.

The same current host controller was then compiled with the
prepared Z10 feature and input vectors for both sources. It destroys
the second Nexus and loses Kerrigan at frame 62,535. The two complete
63,637-call input tapes and the pause SRAM agree byte for byte. Mineral
reserves stay under 60 at the sampled checkpoints after frame 30,400.
The shared controller tape SHA-256 is
`c83249a72f856617df0fa50c5a1b69ab7a6daa7d94bd7a6e0553bbd8cf787a8e`.

A read-only census reproduces the exact input tape. Across five
samples from frame 28,000 through 32,588, 16 drones remain active,
11–13 are idle, and none are mining. There are 37–38 stocked
mineral patches with zero visible to the player. By frame 33,601
one stocked patch is visible, about 1,359 pixels from the nearest
unladen gas drone. The controller searches at most 360 pixels.
Its ordinary economy route needs earlier scouting and worker
transfer before the existing miners deplete their fields. The
census receipt SHA-256 is
`c0bd7ff7ceef5b94d7e99dc032a7532ca24096f1982a3b3dde89284b78a734b1`.

Three additional host trials change only the ordinary-input
controller on `537d09f7`. A 1,500-pixel search still considers only
visible, stocked fields. Counting attempts after the cursor reach
check yields the same tape as stock. Increasing the radius alone
yields the same tape as both changes together.

| Controller policy | Calls | Loss frame | Drones | Army |
| --- | ---: | ---: | ---: | ---: |
| Stock or attempt-count change | 63,637 | 62,535 | 1 | 9 |
| Wider radius, with or without the attempt-count change | 80,452 | 79,353 | 3 | 11 |

The longer run ends with 7 minerals and 3,976 gas. It still loses
Kerrigan before the remaining objective. The test changed reach for
already visible fields. It did not establish reachability, sustained
income, or a winning route. Next, make the controller observe a
stocked field before local depletion and track actual mineral returns
after issuing worker orders. Then qualify victory on the normal ARM
cartridge from HOME with a fresh pause load.

The wider-radius input tape SHA-256 is
`2e97076792d32848adb3e258f19c124a18c4163914f8ef75792bf0c15194ccb3`.
The stock and attempt-only tape hash is the same `c83249a7` hash above.

### Observed mineral returns in the recovery trials

The next two host trials instrument actual mineral returns. An idle
Drone accepts a scout move near a completed Hatchery at frame 34,046.
Several stocked fields become visible, but the Drone stalls in its
move order. A second trial retains the wider search and reverses idle
worker selection after frame 30,000. Both use ordinary controller input.

| Controller trial | Minerals returned, frames 30,000–39,999 | Minerals returned, frames 40,000–59,999 | Loss frame |
| --- | ---: | ---: | ---: |
| Stock | 24 | 320 | 62,535 |
| Wider visible-field search | 0 | 136 | 79,353 |
| Scout move | 8 | 8 | 59,939 |
| Wider search with delayed worker priority reversal | 64 | 192 | 60,598 |

The accepted scout move does not establish arrival or a return trip.
The trials leave the remaining objective intact and fail to qualify a
victory. Keep the controller changes experimental. Check movement and
gather completion together with the timing of Kerrigan's retreat.

| Local input and return trace | SHA-256 |
| --- | --- |
| Stock | `6340fcfbb69703246e2a1cc5873233932236211833c22fb18e87cf526dba11a3` |
| Wider visible-field search | `fcd6bc8719169f56d6c060cb35fe88ef0b0f060de9373f4bc00e15e6b9f089be` |
| Scout move | `bec380cd70fae4e7c700a0fe6fc2235550ce49cb728c307662e9a4d3057ac130` |
| Delayed worker priority reversal | `7e3c380dc483f558ac3626b54c4cf1e78f2e2537962cdb39e15ffbfe239206b8` |

### Late hero survival on the Melee release

An instrumented host run on `964193dd` reproduces the stock tape
and loss at frame 62,535. Kerrigan is idle and unselected in the
sampled late interval. Between frames 58,780 and 62,380 her health
falls from 392 to 12 beside a visible enemy. The controller's raid
lookup reports no active raid throughout that interval.

A host trial searches for a completed friendly town hall farther than
160 pixels from Kerrigan, clear of visible enemies within 320 pixels
of the hall. The search runs on health-loss updates while an enemy
is visible within 240 pixels of Kerrigan. None of the 99 searches
finds a destination. The controller issues no retreat input and
repeats the stock loss and pause journal.

A separate fallback issues an ordinary Move toward a visible
friendly combat unit farther from the immediate threat. Kerrigan
accepts the order at frame 58,900, moves over the next 101 frames,
and reaches the rally point by frame 59,101 with 372 HP. She
still has 372 HP at frame 62,501. A later attack reaches her
and the host run loses at frame 78,849. It ends after 79,977
calls with zero Drones, four army units, and the surviving Nexus
at 1,500 HP. This confirms the retreat works through the old loss
window. Check worker arrival and deposits as well as hero survival
before attempting a full normal-cartridge run.

| Local observer trace | SHA-256 |
| --- | --- |
| Stock late hero trace | `0d4ad88e855768e40fb4026a48adca2896f9766a6ceeddc70c2d4e7df3ae15e5` |
| Town-hall retreat probe | `eb61fb08fffbe21a3673e2689fabebedf48e4d7f4c439a22900b3c156c246409` |
| Friendly rally retreat input tape | `7845f3b22bcfa33725f71b5931e176d494a908c3af915fff94916c1fd829dba9` |
| Friendly rally retreat observer trace | `483bb5137ebc2738c85e3ea37b28224b21ea6e971ce7e90523255ff023d4b925` |

### Observed deposits after the worker transfer

A host controller trial on the same engine issues an ordinary gather
order at frame 34,045. The Drone selects a visible stocked field
with 678 minerals after the controller observes no active special raid
and at most 1,000 visible minerals in the current search. The trace
records 22 deposits of eight minerals by that Drone between frames
34,684 and 52,600, with its cargo clearing at each credit. This
verifies the transfer, pickup and actual return to the depot.

Kerrigan has 400 HP at frame 62,541. The attack reaches her later and
the friendly rally filter finds no eligible unit after she starts
taking damage. The trial ends in defeat at frame 69,355, with 421
minerals, one Drone, six army units and the surviving Nexus at
1,500 HP. A guarded attempt to reuse the earlier rally point sent
no retreat order and produced the same tape and result. The replay
proves the replenishment route and leaves a repeatable retreat and
a complete mission win open.

| Local worker trial | SHA-256 |
| --- | --- |
| Ordinary worker transfer tape | `73cf0e274ffc3247c445675287d66f1439fc5edf685e52a9c5d6662997bf1883` |
| Deposit and hero observer trace | `70a1f0d80265bcd8086eac9492e3e83c7baaf74d022841053cf40c4749d753da` |

### November 23 replay of the historical winning tape

The recovered tape matches the SHA-256 of the November 13 winning
controller above. A fresh HOME replay on the `1f0ad66e` normal Z10
ROM runs the first 20,000 calls unchanged. Its earliest seven
retained HOST checks match. At call 16,538, frame 16,091, the old
HOST bank is 1,305 minerals and 406 gas. The current ARM bank is
1,313 minerals and 398 gas. The frame, screen and result flags
agree. This is the first sampled discrepancy. The exact historical
ARM ROM from the earlier verification record is absent from the
searched worktrees, so the cause of the earlier bank discrepancy
remains open.

At CSV call 18,423, the old tape's Select shortcut opens the pause
menu on the new ARM run. The button combination at call 18,425
includes R, which restarts the mission from the pause menu. The
historical HOST tape continues in the field through those inputs.
The base run ends in play at frame 17 without a result. The current
field control clears an existing selection on Select and opens the
pause menu when the selection is empty. Use the current field
controls when refreshing the controller tape.

A diagnostic replay responds to each unintended field pause with
neutral and Start, using physical inputs only. It handles 25 such
pauses before the first result. The replay advances in the original
mission past the early R-button resets, then loses at frame 21,782,
tape call 22,376. Kerrigan is alive with 299 HP. Between frames
21,750 and 21,782 the scheduler marks trigger 15 done and transitions
the human outcome to defeat. The existing trigger condition detects
an enemy hero Dark Templar reaching the designated area. Verify
the interception leading into that result when adapting the tape.

A separate HOST trial issues the hero's retreat earlier after the
verified worker transfer. The move is accepted at frame 53,570
and Kerrigan reaches the rally near `(1574, 1743)`. She has 400 HP
at frame 59,279. The run loses at frame 59,745 with two Drones and a
1,500 HP Nexus remaining. This trial ends 9,610 frames earlier
than the worker-only run. Keep the earlier retreat out of the
winning controller until there is a repeatable survival route.

| Local receipt | SHA-256 |
| --- | --- |
| Current Z10 ROM | `86f25892cb127bcd325c954e02d9ae5aa3122635901c167a74b85ff569109105` |
| Historical tape | `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb` |
| Fresh HOME prefix | `e9bca5af637660b0dfcbbf2a245dc680e4c047c3d20bc0c6aea7799808c8e962` |
| Fresh HOME update log | `90c01fb6f680c99740656f34414112ba9aecf8d3136a4baa6add71ce180ca621` |
| Pause recovery replay | `4ca8c261186fde9af4026e14a6e2e2e17688d861bfe4ae9b5bd9fd467981ed56` |
| Recovered physical pauses | `18a496fa7b428af1c2a2533086ccd47be4c08462a351c312ca74b046118e290a` |
| Bounded defeat observer | `e7eca9d9cbbae7c22e7e8ae896d45995bb1d2df38dbc2231d7de8ab450825c3b` |
| Earlier-retreat observer | `2955f45d383a350c277b7a60038833de1aced9842337a7963c475a10b6759379` |

### Interception trials on the normal Z10 cartridge

The observed attacker passes within the stationary Overlord's
144-pixel detection radius by frame 21,658, 124 updates before
defeat. The old route has three Hydralisks near the threatened
area at frame 18,483 and none within 500 pixels by frame 19,454.
The remaining army is too far away for a last-minute interception.

Three fresh HOME runs detach from the old input tape at the
earlier position. Each selects the three Hydralisks and moves
them with ordinary buttons. The first holds near the objective.
The second holds farther up the approach. The third gives an
explicit attack order once the attacker is detectable. All three
Hydralisks pursue in the last trial.

| Trial | Attacker HP at arrival | Result frame |
| --- | ---: | ---: |
| Hold near the objective | 35 of 40 | Defeat at 21,782 |
| Hold on the approach | 21 of 40 | Defeat at 21,782 |
| Attack on the approach | 21 of 40 | Defeat at 21,782 |

The approach trials both observe 40 HP at frame 21,680, 36 at
21,700 and 21 at 21,720. The selected force survives. These samples
give net damage and cannot establish exact shot or regeneration
timing. Test earlier detection or a larger defending group on the
approach before returning to the attack and mining route.

| Local receipt | SHA-256 |
| --- | --- |
| Attacker and detector paths | `17805bfe1483e00da5d673c64a5af75e95c86ca12179cb2327c40e794527c01e` |
| Hold near the objective | `6ab7f1ce0a0242fc5356ff26928f8a6c115743f15ae2e9fe15790867d2162b2b` |
| Hold on the approach | `d3345840c3c6a20f733fd894f422cf441dde92739560d3adceeafb2eed56f4c9` |
| Explicit attack on the approach | `40d5511b700c6e00969cbd474b3a32e42120089c34731a4d5c56121085c4d1f8` |

### Forward detection and fresh HOME continuation

The next ordinary-input run sends the Overlord farther forward.
Its movement is observed through arrival and it is idle at the
new position by frame 20,000. The game's visibility flag shows the
Dark Templar first revealed at frame 21,544. The three Hydralisks
accept the attack order at frame 21,584. The target falls from
40 HP to 16 by frame 21,700 and is absent by frame 21,800.
The game stays in play through frame 23,100, beyond the old
defeat at 21,782. Victory and defeat flags are both clear.

A separate replay reproduces the interception, saves using Start
and B at frame 23,101, and loads the SRAM with L from fresh HOME.
The frame, result flags, resources, mission and 91 observed live
unit records match between the original and loaded sessions.
After aligning both sessions at frame 23,102, another 120 neutral
updates produce matching observations at frame 23,222 with the
attacker still absent. The guest exported the checkpoint used
for this comparison successfully. The harness later returned an
error on a redundant final export. These checks establish a
repeatable recovery and continuation. The remainder of the
mission and its victory sequence still need a winning replay.

| Local receipt | SHA-256 |
| --- | --- |
| Forward detection and interception | `9d15b57d5080fad49a2d499f67e0c89ee4d26d390192ff9431cecadf3ecf1edf` |
| Fresh HOME comparison | `8a1c216ee9f9b71f940bfe51ebe50138239993dab2f9018a4d9ac38f8677c02f` |
| Interception save | `2bbf10edccb8b208a77e7fa54e77d52421d0e062baf033f8ac3734d18bf90fc6` |

### Production and the southern approach after interception

These runs start with the verified fresh HOME load at frame 23,101.
Ordinary training input pays for three Hydralisks at each of two
Hatcheries. By frame 25,173 both queues empty and the live
Hydralisk count rises from 11 to 17. Inputs at four other selected
Hatcheries do not produce paid orders in this run. Breakpoints
confirm that the first refusal, at frame 23,479, fails the
completed-building prerequisite. The player's Hydralisk Den
at the northern base drops to 7 HP at frame 23,470 and is gone
by frame 23,475.

A Drone places a replacement Den near the northern workers
through ordinary input. The Den reaches its full 850 HP and
completes by frame 24,313. At frame 24,765 a Hatchery accepts
another Hydralisk order, paying 75 minerals and 25 gas and
consuming one larva.

The direct attack trial selects two southern Hydralisks and sends
them toward the second Nexus. Across the next 4,000 updates the
Nexus stays at 750 HP and both units settle near their starting
area. A shorter duplicate observes active waypoints and zero
retry counters while the units alternate by one or two pixels
without advancing toward those waypoints.

A fresh trial first issues a westward move through an intermediate
waypoint. Both units reach it and advance toward the Nexus on a
second attack order. The last observed survivor has 42 of 80 HP
at frame 26,105. Both units are absent by frame 26,305, and the
Nexus remains at 750 HP through frame 27,705. The waypoint bypasses
the observed stall.

In a separate recovery, six Hatcheries accept three Hydralisk
orders each after the replacement completes. The live count
rises to 29. Rallying the entire combat group on a southern
Move order loses the northern force en route. The later southern
box selects 21 combat units, including Kerrigan. Attack Move
advances into the defenders and Kerrigan dies at frame 31,443
with both target Nexuses at 750 HP. Stage the northern reinforcements
in shorter moves and keep Kerrigan out of the assault group on
the next replay. The current cartridge still needs a full win.

The north and south Nydus endpoints are still linked at HOME
load. During a neutral continuation the northern endpoint falls
from 198 HP at frame 23,102 to 5 HP at 23,302 and both endpoints
are gone by 23,327. A transfer through that link remains
unverified. Test a defense before this loss or use a different
route for the later reinforcements.

| Local receipt | SHA-256 |
| --- | --- |
| Paid training and live arrivals | `059c9080369ec1fb559c41649852769320bc07791555127640a6cc076ed65bce` |
| Prerequisite check at the first refused producer | `06ff44699e24c36e513ef7d3a38f2a6054e6987ad1dd493c64f448abae47fcce` |
| Original Den health and loss | `371789bb70f214f9798927c852430c81efa418579881b4a04551e9169364bf65` |
| Replacement Den and resumed production | `f9f626289b3673e069ef12d3e590a20f59a5fbc2e4cd9b7fe48058c11179b812` |
| Direct attack | `f2891fa2f0d897df3de3fc7f0a857743884b358b025276cc855f8b21c6566375` |
| Direct attack waypoint and retry samples | `a8e03004e93b8487552329280a569bcfa0adb8b0ebe1e5990049f60f6ed8802a` |
| Intermediate waypoint assault | `9998e2fb3d7687270a837992ed13c467293561af661df3282c9651f4b5fd8cfe` |
| Six-Hatchery production and combined assault | `2c4a9170b5fcee73250217df0308368037ed48883f0baf37144dddf2ffbc9584` |
| Nydus link and health after load | `e8869ad4815123817c5e73ed9d606b2419b9208d49a076526fcd71ebf59ad204` |

### Earlier Nydus transfers and fresh HOME recovery

An earlier diversion of the current Z10 controller run pays for
Hydralisks at four northern Hatcheries and sends three of the
new units and a Drone through the existing Nydus link. Each
transit uses an individual ground-unit selection and an ordinary
order on the northern entrance. At frame 23,040 the four units
are at full health and beside the southern exit. Both canals
have 250 HP and Kerrigan has 400 HP.

The ordinary pause save at frame 23,291 captures the group
and both endpoints. The north entrance is down to 5 HP. A
fresh HOME load reaches frame 23,292 and retains all four
transported units, both canals and Kerrigan. The outer replay
reports an error on a redundant final export after the pause
save. The separate fresh-process check verifies the earlier
SRAM file. This qualifies the transit and recovery. Preserving
the link beyond the subsequent attack remains open.

A separate continuation from the new save sends two existing
southern Hydralisks through the intermediate waypoint. They
reduce an outer Photon Cannon from 100 to 25 HP before both
die. Kerrigan remains at 400 HP and the mission is active at
frame 25,881. The transported group stays at the southern
exit during this trial.

A larger ordinary box selects the three transported Hydralisks,
four other Hydralisks, six Zerglings and a Drone. It leaves
Kerrigan behind. After the same intermediate waypoint, the
group destroys the outer Cannon by frame 24,924. At frame
26,124 all seven Hydralisks and two Zerglings remain selected.
Kerrigan has 400 HP and the eastern Nexus still has 750 HP.

A pause save at that point loads in a fresh process at frame
26,125 with the Cannon still absent, the seven Hydralisks
selected and Kerrigan at full health. That group destroys
the Pylon southeast of the Nexus by frame 27,801. The
remaining attackers die on the follow-up order and the
Nexus still has 750 HP. The post-Cannon save preserves a
larger force for testing reinforcement and a sustained attack.
Three later visits to the southern Hatchery fail to pay for
Hydralisks while the Den is absent. Selecting the transported
Drone works on a close box around its current position. The
northern Spawning Pool has also fallen by then. Ten southern
placement attempts leave the Den command armed. The Den requires
a completed Pool, so replace or preserve the Pool before the
next Den order.

An ordinary recovery spends a Drone on a new Pool near the
southern Hatchery and queues three workers there. The Pool
is complete by frame 28,803. One of the new Drones starts a
southern Den, which completes by frame 30,597. The Hatchery
then accepts three paid Hydralisk orders for 225 minerals
and 75 gas. All three are beside the Hatchery at frame
32,222. A fresh HOME load at frame 32,223 recovers the new
Pool, Den and Hydralisks with the outer Cannon still gone.
All seven original forward Hydralisks are still at their
holding positions and Kerrigan has 400 HP. This save supports
further production and a new grouped attack.

| Local receipt | SHA-256 |
| --- | --- |
| Northern production, transit and pause save events | `35e74543ffa7dfc36f12a38f1baf63ca34846df37bc8adc14ae665ebaa9757db` |
| Post-transit pause SRAM | `0e40ad4650f9f7f2d971499380f0dc147a32482788230d9585caa4d0d13bf841` |
| Fresh HOME load | `33fd47c8a8d285a2c75439c4863e234d48331e890c6b68e39562daa94873c30f` |
| Southern Photon Cannon attack | `a79fff16864fdbfbf5b26267eeb5209d11cdf695a2056a02788d3341167ffc4b` |
| Southern box selection and Cannon destruction | `31ab6e5a0bc9118d3138f1c96066d1a539d017ebe331e9ba7b6dcc69e3eae619` |
| Post-Cannon pause SRAM | `b59a01a29530ec3d5c0a1cf3d0313ec66d892967b790133c7363162d2b0fc45c` |
| Fresh HOME and the Pylon assault | `f479270ead8de41124adf7d311e9246daefc409f0c4e7396220eb741b7e92f66` |
| Southern Hatchery after the Den is lost | `3b94e287ef17ab70e84c37cfba5ce2bf01361c12fc4de3ccc21897357f48f859` |
| Southern replacement placement attempts | `658287cd015efec91376b57cf1fb30c197e92db6d6b5d63abb261bc0d9f21aec` |
| Owned buildings at the armed Den command | `2682ec12cff549134d817a29123271d598cd54552e3a21b8995f1e6a6a1818b7` |
| Southern Pool, Den and fresh HOME production check | `23a65a5f866fc75fcf26aee66618aa6dd67882888053b051bbfa37bc51bcc9c5` |
| Southern production SRAM | `4fa2c92494136377ee12b555995df23d15b24a734a404c33b0beb3e975a5cca7` |

### Second Dark Templar defeat after the southern attack

Repeated production at the southern Hatchery grows the live
Hydralisk count to 29 by frame 38,582. A two-waypoint rally
joins 15 southern reinforcements with the seven forward
Hydralisks. The resulting 22-unit Hydralisk group and
full-health Kerrigan reload from a new pause save at frame
40,823.

The attack clears another Photon Cannon and Pylon. The eastern
Nexus retains 750 HP. The mission loses at frame 42,745 while
Kerrigan is still at 400 HP. The fresh-load repeat records the
human outcome changing to defeat and trigger 15 becoming done.
A hero Dark Templar observed on the eastern approach at load
is at the northwest objective area at defeat. A neutral run
from the same save remains in play through frame 45,123 with
trigger 15 still pending. The two runs establish a second
interception requirement for this attack route.

| Local receipt | SHA-256 |
| --- | --- |
| Southern training, rally and attack | `850b63e8572fc9e3f3572864a3882f542862d7b2ef0a47612c379b0ed8d0f5ea` |
| Pre-attack pause SRAM | `a918f6facaad270e43abaf27d8d2462b632f490917adb8f1b206b10c197073a3` |
| Fresh load of the 22-Hydralisk group | `60f5f72526947196642a538f7f787ccc0d8c71d0719d440b8cb484039c95f5c7` |
| Attack and late defeat trigger | `0a5a0abd4d6cb92a0b39ed1e28b027316a15b2ca1086dc6b33c6f6629c830b8d` |
| Neutral result control | `3e69866e5d75d2ca6836536f94fb2708b5f2453378e6d6db73e776a23024fbf2` |

## Attack-pose lookup timing

An ARM trial indexed the live tracer slots by shooter once per draw.
The lookup retained ascending tracer slot order, current shooter
identity and position checks, and the youngest renderable pose.
Against the exact November 20 Z10 ROM, the trial matched all 47
sampled world checkpoints and all 24 palette, fog and framebuffer
samples. The frame, screen and dialogue timeline matched at all
46,487 tape calls. The same prepared inputs and memory endpoints
were used in both builds.

| Z8 window | Advancing play updates | Mean loop cycles, original → trial | Mean actor/effect cycles, original → trial |
| --- | ---: | ---: | ---: |
| Full route | 45,467 | 1,025,307 → 1,029,473 | 117,384 → 121,523 |
| Calls 30,996–31,196 | 201 | 1,734,738 → 1,737,838 | 500,307 → 504,347 |
| Calls 42,000–42,203 | 201 | 1,553,910 → 1,546,724 | 632,088 → 624,727 |

The trial raises full-route mean cycles by 0.41%. Keep the existing
lookup in the production builds. On the original build, frame 21,667
spends 14,671,053 of 15,352,732 loop cycles in orders. The actor/effect
counter includes sprite drawing, death effects and projectiles.

The trial ROM SHA-256 is
`e7e18198fbb89256ca407e7dec11c94629bdabb72546022bd065ee57c79dfb3b`.
The comparison receipt SHA-256 is
`aa193287361651e52418222d4b37d3e9593fe283c4ab96668e07d2c3f5769e51`.


## Instruction attribution of the Z8 stall

The pinned November 20 Z10 ROM replay samples individual updates at
controller calls 22,253, 31,098, 42,000 and 42,100. All 42,100 calls
match the control clocks, screens and frames. All checkpoint states
match. The four instruction samples conserve cycles and report zero
shadow-stack overflows.

At call 22,253, advancing from frame 21,666 to 21,667, the full
observer interval takes 15,353,225 guest cycles. The call stack
attributes 14,065,864 cycles to two invocations of
`worker_continue_minerals`. Their 16 nested `sc64_route_fill` calls take
13,954,604 cycles, or 90.89% of the update. The controller mask stays
held at 912 across the stall. Compare nearest eligible mineral
selection against the existing reachable-field scan, including route
cache effects, before changing the flood implementation.

| Controller call | Whole update cycles | Renderer cycles | Sorted actor pass cycles |
| --- | ---: | ---: | ---: |
| 31,098 | 2,238,886 | 1,562,800 | 1,003,011 |
| 42,000 | 1,348,467 | 764,144 | 202,378 |
| 42,100 | 1,852,834 | 1,211,905 | 314,363 |

These are three individual draw samples. Use full advancing-update
windows to qualify changes to recurring renderer cost. The instruction
profile receipt SHA-256 is
`fba05b2221c06872dfcb02409893301b71eaf565e4cdad4bdfb3e41b422f56ed`.
