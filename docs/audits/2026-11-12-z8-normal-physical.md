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

### Three Dark Templar interceptions and a fresh HOME save

From the 22-Hydralisk save, a smaller box excludes Kerrigan
and sends four Hydralisks and an Overlord on the Dark Templar
already near the southwest force. A direct order kills that
Templar by frame 41,220. The same guard moves toward the
central approach. It kills the eastern Templar as it crosses
toward the northwest objective after the main army advances.
Both are absent at the fresh HOME load at frame 43,339.

A separate east-first route uses an Overlord to reveal the
Templar near the eastern base. The assault clears a Cannon and
Pylon, and the Templar is absent by frame 43,296. That Overlord
survives at 10 HP. The southwest force kills the other Templar
after loading this save. Most forward Hydralisks die by frame
43,959, when one is still near the eastern Nexus approach.

In a western continuation from the first save, the guard
destroys an outer Cannon and Pylon. The western Nexus stays at
750 HP. The guard is lost and a new Dark Templar reaches the
southern objective, triggering defeat at frame 46,620 with
Kerrigan at 400 HP. A passive guard with detection also loses
when the new attacker passes it.

The verified third interception stages the guard farther
forward and issues a direct attack while the new Templar is
still approaching. It is absent by frame 46,515. A fresh HOME
load at frame 46,916 verifies the mission is active, Kerrigan
and the Overlord at full health, and all three guarding
Hydralisks alive. The eastern Nexus still has 750 HP, with
only one forward Hydralisk left from the attack. Keep the
guard in place while training a new offense.

| Local receipt | SHA-256 |
| --- | --- |
| First two interceptions | `54f241e851dec682df7e6707625fc0a655b0b8f076c4a37efaae82c6302d9aac` |
| Fresh HOME after the first two | `cbbbb6b322cb900b1f2f45db106e56118d68b968d396e24d1f7cbf1d634144b7` |
| East-first interception | `3c2da6cb5c6237060203fc2ebde5449b6ad76732433ad5ae78520372fa4097c3` |
| Western advance and third defeat | `3092ff695bdab4ddfb3cf237c2430ceffc36051111b24b2c4e49b89a7adfd663` |
| Third interception | `d1f445cb2f28f442ab9948f6816b18742752c2551ff4a372042aa5b6ac30b9f8` |
| Post-interception SRAM | `0f27355f431000a9a1f021996cda104b17f37a2920cc26e1054df3cff3ca2bed` |
| Fresh HOME after the third | `8a2ab7c93ef79b3a49f502e3a8a56e9809890cfb7aa34f6d7bb5170bce15bdc4` |

### Late gas supply and the northwest defeat trigger

From the third-interception save, an ordinary Drone order
places an Extractor at the cleared southern approach. It is
complete by frame 48,128. A Drone assigned to it carries
eight gas per trip. The far Hatchery works as the depot and
the wallet grows from 185 to 225 by frame 52,418.

A second Drone starts a Hatchery nearer the Extractor. The
Hatchery completes by frame 54,991. After reissuing the gas
order, the same worker deposits at frames 55,437, 55,619,
55,800 and 55,983. A fresh HOME load recovers the completed
Hatchery, the worker order and the 281 gas balance. Three
further credits before frame 56,531 confirm that the route
survives the new process.

| Depot | Consecutive credit intervals in game updates |
| --- | --- |
| Original southern Hatchery, neutral control | 869, 870, 871 |
| Closer Hatchery, before and after fresh HOME | 182, 181, 183, 182, 183, 182 |

Both resource-only branches lose at frame 56,579. In a
separate replay a new hero Dark Templar appears on the
western approach and reaches the northwest objective at
that frame. Scheduler trigger 15 changes from pending to
done, and the human outcome becomes defeat while Kerrigan
still has 400 HP. The attacker's slot differs across branches.

An independent reinforcing branch uses the original 185
gas to place seven paid Hydralisk orders and adds
mineral-only Zerglings. Fresh HOME at frame 55,421 recovers
nine Hydralisks and ten Zerglings staged on the eastern
approach, plus the separate detector guard. The main army
reaches a forward waypoint intact. It takes heavy losses
approaching the next defenses and leaves the eastern
Nexus undamaged. Intercept the fourth attacker before a
renewed assault.

Sending new northern Hydralisks and the western Overlord
toward a rendezvous loses both groups before they reach the
intercept lane. Fresh HOME at frame 26,125 still has three
full-health Hydralisks and an Overlord already in the
northwest. By the later southern-production save, only
the Overlord remains there. Stage that early guard before
the subsequent southern work.

The post-Cannon replay moves the four original northwest
defenders to a safer holding position. All four remain at
full health when the pause save is written at frame 32,900.
From a fresh HOME load, a Templar approaches that guard
earlier than in the later gas and reinforcement branches.
A direct order takes it to 31 HP, but it reaches the
northwest trigger at frame 47,628. The early guard solves
the reinforcement route. It still needs earlier contact
with the attacker.

Moving the guard farther forward and giving the direct
order at frame 47,374 improves net damage to 19 HP. The
Templar reaches the trigger at the same frame with 21 HP.
All four guard units are still healthy. The original
guard can reach the lane in time. Additional damage or
longer detector coverage remains necessary.

| Local receipt | SHA-256 |
| --- | --- |
| Extractor placement and completion | `f1c5143fbb306e21b4aa5bc0fb4e8df00aebcb6c79da3dcae9daebf3b4c5dbee` |
| Far-depot gas cycles | `33fb9e79636fa3c4f15572ebc9ce8986e5e83bd857bfef33c3bdffa28fb3ecaf` |
| Closer Hatchery and gas cycles | `e5eed74c8375f6df2602fbd63aad8a1a1dcb915240864ca9072163f31f7f0139` |
| Closer depot pause SRAM | `de3e1b043e1279c0127591ac9427263de42f3ffc0ffbab368481763a151acdd8` |
| Far-depot neutral control | `ec1a25cadbee105b00a5295509c5191713b0b1d5f744a53eb382099ab179e1b7` |
| Closer depot fresh HOME control | `d6d7134ce0fbf9196fc6adf3c2015eca1ce0c2fc489385b3f1d5918cbe177411` |
| Pre-assault 19-unit pause SRAM | `e603cebaab7bd8640b3db8c79296cb5815b48a1e0920c292a4068d4c3af10641` |
| Fresh HOME with the staged force | `dd419dc8c55985d24ad483671952d6bfc94537679e521b269a8bd4f5133a835e` |
| Fourth attacker and scheduler outcome | `12d7abb8f33ac1570ff4de0fe1f0ef6ff30474f21ea306e3b44c1eae122206b0` |
| Late northern reinforcement and Overlord route | `f04aa0963c3412d08ea47c8798e55a48263a2ca86040890e0ff5fc4b52e3c297` |
| Northwest guard across six fresh HOME loads | `f2cc53b38f1e745ed0cf0355bd752cddf1c0f573ca37cc387738abdda9f72351` |
| Staged northwest guard | `1539d61dce21f85caf9dc5300d79385475fa300112c76cae1e40784537697c1c` |
| Early guard pause SRAM | `c4bc47dd795f7886058289079a06258c446ffaabe65e7b461a54c5e509ee2529` |
| Earlier attack on the northwest approach | `df867c2c2fbe41c4acbc874a2b472362beeb5e19629aa80cacdec5bca8a91c73` |

### Two-detector northwest formations

Three replays start from the same early guard checkpoint at
frame 32,901. Each routes a second Overlord to the northwest
and tests a different formation for the original three
Hydralisks and Overlord: the staged guard, a forward guard,
and Hydralisks held near the rear detector. The direct attack
orders occur at frames 47,374, 47,331 and 47,509 respectively.

All three end in defeat at frame 47,628 with the fourth
Templar at 21 HP. The sampled guard units remain at full
health. These trials do not establish that two detectors
cannot stop the attacker: they show only that these three
formations and orders did not do so from this checkpoint.

| Local receipt | SHA-256 |
| --- | --- |
| Staged guard, two detectors | `808fc0950c1c95c7a9cb6990648671364cb7b75a48ff437917a2be75a0c32aef` |
| Forward guard, two detectors | `c3f431c83653070fdf8c7424870ad6112de8c3b16ae1e2a8408d9cd7e39c53ba` |
| Rear Hydralisks, two detectors | `5706cef748bcacbcf94ea1583fe451af8d3a53bc7826b8fa49d167ba56e31146` |

### Northern Hydra reinforcement attempts

At fresh HOME frame 32,901, two additional healthy Hydralisks
are available near the northern Hatcheries. An ordinary westward
movement order leaves one dead and the other at 24 HP by frame
33,924. A second order stalls the survivor south of the route.
Neither reaches the northwest guard. The original group attacks
the fourth Templar directly at frame 47,330. The Templar again
triggers defeat at 47,628 with 21 HP.

An earlier HOME load at frame 26,125 has five Hydralisks and an
Overlord near the Hatcheries. The larger group attempts the top
edge through (1,500, 60) and (1,100, 60). Four Hydralisks disappear
en route. At frame 30,859 only the Overlord has reached the later
waypoint. One original guard Hydralisk has also disappeared while
the guard is unattended. The branch is still active at that cut.
Neither trial supplies additional firepower to the northwest guard.

| Local receipt | SHA-256 |
| --- | --- |
| Two Hydralisks on the western route | `5802b06483f82b246382d488031498d192ed4f6669cb06d079c76a433f105fa3` |
| Five Hydralisks and an Overlord via the top edge | `210c7f38babd300f5888158ab37a5fddda29469a6b15d33cfb92916f2a953a3e` |

### Lower corridor and Overlord transport

The lower route from the same frame 26,125 save brings the
five northern Hydralisks toward (1,450, 600) and then west.
Only two remain by frame 27,971 and all five are gone by
28,700. The Overlord reaches (168, 266) at 32,587 with 200 HP.
The mission remains active and the northwest guard has lost
one Hydralisk while unattended. The separate controller trace
SHA-256 is `38fe9ae23909679f124a9896cae280b9229b6e8800457c88a71aa3cb4626fef8`.

The retained Z10 cartridge has the shared transport service and
the Z6 Overlord path. In this Z8 campaign save, the race-mode
Ventral Sacs route and the Z6 mission route are both unavailable.
Fresh HOME readback at frame 26,125 confirms race mode is off and
the shared transport research bytes are zero. The shared research
menu also requires race mode. The successful Overlord flight
therefore supplies detection without carrying the Hydralisks.
Native Z8 transport availability remains a parity check.

### Early western Nydus and Hatchery

A fresh HOME load at frame 4,861 from the frame 4,859 pause
save retains two friendly Nydus links. The north entrance at
(1,632, 256) links to the west exit at (400, 192). Both are at
250 HP. Earlier qualified transfers use the separate southern
link. The historical replay has the western endpoint down to
128 HP by 9,737 and the pair gone by 14,599.

Ordinary unit selection and A at the north entrance send a
Drone through at full health. Two eastern Hydralisks also
cross on a separate run. A fresh HOME load at frame 7,135
recovers those two and the two original western Hydralisks,
all at 80 HP, grouped near the 200-HP Overlord. All four
canal endpoints remain at full health.

The western area already has a completed Hatchery. A new
Hatchery foundation starts legally on a separate branch and
is destroyed before completion. On a fresh load of the rally
save, the existing Hatchery accepts three Hydralisk orders.
At frame 7,705, a fresh HOME load recovers the first new Hydra
at 80 HP, with the other two still queued. The four original
guard Hydras and their detector are present.

### Mobile defense past the early ground attack

Three 100-HP Zealots approach well before the first Dark
Templar documented later in the campaign. Holding the five
Hydralisks and Overlord near (300, 200) and issuing an attack
loses the group and two newly produced Hydralisks by 9,100.

From the unchanged production save, the five-Hydralisk group
retreats north and alternates direct attacks with movement.
By frame 9,382, fresh HOME recovers all five and their Overlord
after the three initial attackers disappear. Two separately
produced Hydralisks are lost. The remaining enemy at (297, 168)
falls during a subsequent ordinary direct-attack run. The fresh
HOME load at frame 9,685 recovers five Hydralisks at 80, 80, 80,
64 and 50 HP, plus the 200-HP Overlord. No enemy is observed in
the surveyed western area. The completed Hatchery has 647/1,250
HP and three larvae. The mission is active, and the continuation
has yet to reach the first Dark Templar interception.

These continuations use the retained ordinary Z10 ROM with
SHA-256 `86f25892cb127bcd325c954e02d9ae5aa3122635901c167a74b85ff569109105`.
Existing saves were copied before use. Controller runs and fresh
HOME loads used ordinary buttons and read-only state checks.

| Local receipt | SHA-256 |
| --- | --- |
| Early pause source | `73044e9968b1dc903d68298f571da6e6351284497b5a9b074b89eefacc7f55f5` |
| Two-Hydralisk transfer trace | `e08d8abf3f6e2e2a75e54ee1ecb8f500741ae64e7bcabf3b69ea2f5830584799` |
| Four-Hydralisk rally and fresh load | `3832778ec39e37a2ef4c11a65d9c564a5cafc2424b868895ea0daabccceb8b50` |
| First local production and fresh load | `a294c12993e743375bc094c972d2ed2345c453ef7fec563d89b0ae6dc0708136` |
| Static defense loss | `1e2f25f3fa6e1f07ba01e1a0d13d2b8f91b06f313e0a9a184daa56abb4260dc2` |
| Mobile defense and first fresh load | `5c2a604ad642d03a698955c2239796fa217e152ef6355a55ee8baa2edc36d7d3` |
| Follow-up attack and fresh load | `a0bc440d39e35380a5e16dc8178379b2d56a9e9caee30d9d3fa908d3eeff3278` |
| Frame-9,685 HOME continuation SRAM | `d0759141780cec67904b5e5ca69038d8343560c57a799cd2ae9881c2bbb7aacc` |

### Western reinforcements and the first Dark Templar

From the frame-9,685 western save, queue three Hydralisks in
the existing Hatchery. Fresh HOME at frame 11,310 recovers
eight Hydralisks, a full-health Overlord, and the 647-HP
Hatchery with three larvae and an empty queue. One Hydralisk
has 2 HP. A box selection and a move toward (340, 80) regroup
all nine units, with the selection intact at fresh HOME
frame 11,638.

Holding near (150, 270) against three ground attackers and
trying to finish the full group leaves only three Hydralisks.
On the successful branch, focus the approaching Reaver, then
withdraw the five surviving Hydralisks toward (120, 300).
Move the Overlord separately to (218, 383) and box-select the
Hydralisks for the interception. The 40-HP Dark Templar
enters detection at frame 21,544 near (325, 471). All five
Hydralisks accept a direct attack at frame 21,591, and the
Templar is gone at frame 21,676.

Retreat the Hydralisks toward (120, 300) again and save.
Fresh HOME at frame 21,883 recovers the five Hydralisks
at 80, 80, 80, 80 and 64 HP and the 200-HP Overlord. After
320 neutral updates, the mission remains active with the
Templar absent and four ground attackers still near (340,
136). This branch uses the same retained Z10 ROM and
ordinary controller inputs as the preceding defense.

After the interception, the northern Hatchery trains three
full-health Hydralisks. Give each the Nydus order individually
and check that it arrives before selecting the next. Fresh HOME
at frame 25,643 retains all three at the southern exit and both
canals at 250 HP. The southern Hatchery then trains three more
Hydralisks using the remaining 75 gas. Fresh HOME at frame
27,641 retains six new full-health units in the south, the two
earlier southern Hydralisks, and the western guard with its
Overlord. The mission remains active with zero gas. Further
Hydralisk production needs a fresh gas income.

Give a Drone a gather order at the idle Extractor near
(1824, 240). Gas reaches 24 before the save and stays at
24 on fresh HOME at frame 28,908. After 500 neutral updates,
the worker is still gathering and gas reaches 40. Supply
capacity fell from 84 to 68 before the save. A northern
Reaver is advancing while the Spawning Pool takes damage in the
500-update continuation. Defend this base to sustain the
gas route.

| Local receipt | SHA-256 |
| --- | --- |
| Reinforcement trace | `0a5b993c9efba0ffb468a086ef707637c24a2f99157b189786d51bc51ee55a7d` |
| Frame-11,310 HOME SRAM | `061753383f4d0895035dd50e71447514216d57f77176e1e597efec81b1173439` |
| Regroup trace | `5ea802893769a3cdc9c15a26738519ceeecf724f51039a0a9ec7094db1eb2e5b` |
| Frame-11,638 HOME SRAM | `54dec7edc80a61e1d73c96e2db32f55b30ba9e8d9bc868d75458ebfa9ae35e45` |
| Interception trace | `e1aceec77adccec551ff9d4a6f857e612c3773669ec3122bd089cbf70b65accf` |
| Frame-21,883 HOME SRAM | `7967c0b18c1e9cb62e5a782db8819cde1ae2894509050f758acea00d99cce203` |
| Fresh HOME and 320-update continuation | `58a0c00a83422f41e7b4103f6d1f44676633aaa4b68e63206da748b4673db710` |
| Third canal transfer and fresh HOME trace | `aa4564f4be551626f8bace6d2ee991d0c06a77b7840c14ad03ade0ece20b3d4e` |
| Frame-25,643 HOME SRAM | `85412b1e0d5b7d3c7dcadfdc62f1cf0c4fd6137ba33bc5cf39daa59b898eb180` |
| Southern production and fresh HOME trace | `ec68c73edf2009df140b356e1f4377fbb33be75a5ee92ef605c00cb2e9c2a789` |
| Frame-27,641 HOME SRAM | `c70dfaf602773831c8e9c5f632a483943cbeea3409705f6e7ac1b2dcc997f5a3` |
| Extractor order trace | `c0b408fe8c736dd9c31a660e8a1ab4aff2fc9a87b54d6106b1b16af205e3878d` |
| Frame-28,908 HOME SRAM | `5c0f669623345a043b23322b8583efcdedab608ac5d881de163442dfc81ba343` |
| Extractor fresh HOME and 500-update continuation | `d9471a6f7837b29db3f59e806dbe6f208edd1093676d0e4b026fdc5a2518fe5b` |

### Keep the northern Extractor working

From frame 28,908, the two northern Ultralisks can defeat
the approaching Reaver alone, leaving one at 4 HP. A better
branch box-selects those Ultralisks and six Zerglings and
focuses the Reaver. By frame 29,338 it is gone, both
Ultralisks are at 400 HP and three Zerglings survive.
Reassign the Drone to the Extractor. Fresh HOME at frame
29,626 shows 32 gas and an active gas order.

Track each approaching Dragoon with the cursor and issue
direct attacks. After four such eliminations, fresh HOME
at frame 30,626 retains Ultralisks at 191 and 343 HP, two
Zerglings, and eight northern Drones. The gas order remains
active, gas is at 72, and the surveyed northern area has
no enemies. All eight northern and southern buildings
remain at full health.

The southern Hatchery then adds three 80-HP Hydralisks.
Fresh HOME at frame 32,594 retains them, five western
Hydralisks, and Kerrigan at 400 HP. Gas has recovered to
77. The western Overlord is already absent in this save.
An unattended continuation ends in defeat at frame
34,840 after the Dark Templar takes the southern approach.

| Local receipt | SHA-256 |
| --- | --- |
| First mixed-defense trace | `01c27f6368edd460b7936b0df79fb5e21d71b8442bd0de3b4da0056c32c969e8` |
| Frame-29,626 HOME SRAM | `849cb351886f8067eb08fb0be44d63a24eb9cf210f7ef1f9d58dac992e4c4317` |
| Four-Dragoon defense trace | `1cadb9b4e4c7fae2ac0acc40353720a81389a66fe05b7cf0769563b05a6b13a1` |
| Frame-30,626 HOME SRAM | `0b67ba54b978887c81202c502765e03e3c2633c7e5367e0d1f3bed7ea1cd6d2a` |
| Southern reserve trace | `70c5dcd180f57afaf1ea6507cb81abaf4a10861dd96a3017bcc1d40b225c9127` |
| Frame-32,594 HOME SRAM | `5ea7da5d48ddd251c0f4a2db8264ec8d0025b3e691128d06763c3cecfe947957` |

### Southern interception and northwest detector

From frame 32,594, hold the existing southern Overlord
near (1320, 1789) and send two Hydralisks and two Zerglings
toward (1310, 1710), with Kerrigan staying behind. The
40-HP Dark Templar enters detection at frame 34,633.
An ordinary direct attack at frame 34,646 leaves it at
6 HP by frame 34,697. It is absent at frame 34,747.
Fresh HOME at frame 34,998 retains both Hydralisks at
80 HP, the Overlord at 200 HP and Kerrigan at 400 HP.

From that save, fly a separate northern Overlord toward
(299, 384). Fresh HOME at frame 40,031 has the five
northwest Hydralisks and full-health replacement detector
in position, and the southern force is intact. The Drone
has gathered 365 gas, but the northern Extractor has
104 HP, the northern canal has 151 HP, and the northern
Spawning Pool and Hydralisk Den are gone. Supply is 53/52.
The earlier save preserves more of the northern economy.

| Local receipt | SHA-256 |
| --- | --- |
| Southern interception and fresh HOME trace | `e06ada5af303a2abe94fa3bb6152f6f58c17fa9a726c5a8864f382f044175782` |
| Frame-34,998 HOME SRAM | `c9ba1245c72b2ea5a29d8b635ef8accebc7964f4203a43e192bcebd21e7c468e` |
| Northwest detector flight and fresh HOME trace | `33c61a4ecfc00633001f843323f5176b28b71b42526caf4a712ff615cba9285d` |
| Frame-40,031 HOME SRAM | `3f2ec81640bc880669ce38f6c85bad2f753aa5eebeee5c8db63b905f3910726a` |

### Reinforce the north through the Nydus

A direct defense from frame 40,031 loses both Ultralisks
and the gas Drone. The Reaver and three Dragoons are
already around the damaged Extractor. An earlier branch
from frame 34,998 sends six reserve Hydralisks north through
the Nydus as a box-selected group. Fresh HOME at frame
35,753 has all six at 80 HP at the northern exit and the
southern Templar guard still in place.

Group the reinforcements with the northern Ultralisks and
focus the approaching Dragoons and Reaver. Fresh HOME at
frame 39,680 retains six Hydralisks at 80, 80, 80, 80,
80 and 35 HP, one Ultralisk at 96 HP, all eight Drones and
357 gas. All eight buildings are at full health. The
second Ultralisk is lost. Kerrigan remains at 400 HP
and the southern guard is present. A later Overlord
flight reaches the northwest guard by frame 44,458,
while the northern Hatchery and all eight Drones fall.
Resume from the earlier save and defend the northern
base during the detector transfer.

| Local receipt | SHA-256 |
| --- | --- |
| Northern wave trace | `4ae3f3fc7743faf66cd51f87a0f39c832c3ac68d41db969a9e1b5157b603cbb6` |
| Reaver and fresh HOME trace | `4d026dc8787102b3a83c40c190c1c7dbb85ff3776a4d2161da881dda8349eb3c` |
| Frame-39,680 HOME SRAM | `27b57d29d1f27c7ea32261900f24c96e6d6d4a8e5af9a14a8cd9380d83729446` |

### Hold the northern approach and evacuate workers

Move the northern force to (1400, 400) from frame 39,680.
The approaching Dragoon is gone by frame 40,252 and the
next Zealot by frame 40,552. Fresh HOME at frame 41,353
retains six Hydralisks and the 96-HP Ultralisk. All eight
Drones and all eight buildings are at full health. A
neutral branch advances 5,000 updates with no entering
enemy of the surveyed types in the northern area. Fresh
HOME at frame 46,355 has 613 gas and the same buildings.
Seven mineral Drones are now idle.

On a separate branch from frame 41,353, fly a replacement
Overlord toward the northwest while the northern force
holds. Intercept the approaching Templar with the southern
detector and guard near (1350, 1800). The Templar is
absent at frame 46,922. Fresh HOME at frame 46,924
retains Kerrigan at 400 HP, the southern guard, all eight
Drones and all eight buildings. Gas is 645. The northwest
Overlord is still moving at (364, 384).

Another northern wave enters by frame 48,441. A neutral
flight gets the detector to (227, 384), while the northern
economy is lost and no further Templar is seen through
7,000 updates. On the evacuation branch, transfer seven
mineral Drones and then the gas Drone through the Nydus.
Fresh HOME at frame 48,594 retains all eight Drones at
40 HP at the southern exit, both guards and Kerrigan.
All eight buildings are still at full health, gas is 709,
and four enemies are approaching the northern base.
Gas production is paused.

| Local receipt | SHA-256 |
| --- | --- |
| Northern interception and fresh HOME trace | `6a74636cf6dfa0030ecb820234d71047550167fb7192f187d3dd802b241b2841` |
| Frame-41,353 HOME SRAM | `8efdb57f212aef3de4808f327efddbdd507f2c49c2e865c47aa85012d0386ce8` |
| Frame-46,355 HOME SRAM | `cbf43e84c37793218e6f8557421f23117caee45eb27e065256051ed7b9b847bf` |
| Southern interception and fresh HOME trace | `482b1c6d26680ff5b56512186dc7e37721d001a516b967e73638c94c7cf78f8b` |
| Frame-46,924 HOME SRAM | `63acf668cc11946614e8b2556d7103359e80cfc397eaf668184ccb2817c1be9f` |
| Final worker evacuation and fresh HOME trace | `aeefaed1d5f217be547cc3a130b6251ed98903128dccf3ad48cb4c8edfed0845` |
| Frame-48,594 HOME SRAM | `a1a4562e2876fcbe3156591ce179bc57b38b3adcf32990c23c288912286c3cc3` |

### Southern objective probe and production recovery

An attack from the frame-48,594 evacuation save sends two
Hydralisks, two Zerglings and an Overlord toward the outer
eastern defenses while Kerrigan stays behind. Both
Hydralisks and Zerglings are gone by frame 50,352. The
outer Cannon remains at 100 HP, the eastern Nexus at
750 HP, and Kerrigan at 400 HP.

Use the evacuated workers to build a Spawning Pool at
(1792, 1792) and a Hydralisk Den at (1752, 1856) on
another branch. Both complete at full health and the
southern Hatchery produces a Hydralisk. Fresh HOME at
frame 54,812 retains the southern Hatchery, Pool and
Den at 1,250, 750 and 850 HP, respectively. Kerrigan
remains at 400 HP and gas is 634. Supply was 47/50,
allowing one paid Hydralisk order out of three attempted.

Train a southern Overlord to raise capacity from 50 to
66 and queue three Hydralisks. The neutral branch loses
at frame 56,579. On a separate branch, the Dark Templar
enters detection at frame 56,338, and the five northwest
Hydralisks focus it at 56,375. It is absent by 56,476.
Fresh HOME at frame 56,627 keeps the mission active past
the earlier defeat.
The northwest Overlord remains at 200 HP.

The queued Hydralisks complete and another three finish
at the southern Hatchery. Fresh HOME at frame 58,760
retains all six new Hydralisks at 80 HP, Kerrigan at
400 HP and the three southern production buildings
at full health. Gas is 484. The old southern detector
is lost during the wait, cutting capacity to 50 with
61 used. The new Overlord and northwest detector remain.

| Local receipt | SHA-256 |
| --- | --- |
| Eastern objective probe | `791b76e61a7b1b0ea205ab44dee650e2d5e0404872d7398a22c98f4a924d6302` |
| Worker spread and southern Pool | `fc5c7c3a3eeed7d5c2bade4c468ad4069993c17d1f39f41dda0f22b351c1b59a` |
| Southern Den | `0622252d8265a67df99ae573f5cedd6a65d8540d531b6650c817e452c317971d` |
| Southern production and fresh HOME | `9add7d83caba0e6cecbbabeebccdbc5cefec36d8cb2852aefba3ef7f5e9e66dd` |
| Frame-54,812 HOME SRAM | `b58a38f2ecdff66381870b873f5a7d52296b460ae93f81e8c15eb80359d05b45` |
| Northwest Templar defense and fresh HOME | `84e630f7c6419b0aae3635b18176ccdb5c440b423858672a8a2d1ca37463460d` |
| Two southern reinforcement batches and fresh HOME | `0a7dca37562094509f5617e490146aaeb3465a77a1bab6588911b46dc529e88f` |
| Frame-58,760 HOME SRAM | `7ed8d0f99510327ff700d287b7d78adae9945aa1cca1bbb9bb40b0bd59886113` |

### Eastern outer defenses and Nexus shield readings

Start from the frame-58,760 southern reserve. Move toward
(1750, 1750), then double-tap a Hydralisk at the waypoint
to select ten Hydralisks and leave the Drones behind.
Attack the outer Cannon at (1840, 1504). Fresh HOME at
frame 60,626 retains eight attacking Hydralisks and all
six Drones, with the Cannon absent. The southern Den is
at 356/850 HP. Kerrigan has 400 HP. The first mixed-group
attempt also destroyed the Cannon, with nine attacking
Hydralisks and only one of six selected Drones surviving.

The follow-up destroys the eastern Pylon, leaving seven
attackers and full-health Kerrigan at fresh HOME frame
61,867. Direct and southern approaches from this save
both lose all seven attackers. Observe the Nexus's
shields separately from its 750 HP to see the damage.

| Fresh HOME cut | Nexus HP | Nexus shields |
| --- | ---: | ---: |
| 61,867, after Pylon destruction | 750 | 750 |
| 62,380, direct eastern attack | 750 | 80 |
| 62,359, southern firing lane | 750 | 356 |
| 62,376, remaining-Cannon approach | 750 | 750 |

Positions around (1792, 1202) are roughly 80 pixels from
the target center at (1712, 1208). The GBA Hydralisk's
64-pixel ground range plus the 18-pixel building allowance
admits that distance. The observed shield losses establish
that the seven-unit direct attack hits. The follow-up must
sustain damage after the shields fall. Kerrigan remains
at 400 HP in all three failed branches.

Two reinforcement attempts from the Cannon-clear save
did not assemble a larger force. The first rally selection
left six original Hydralisks unselected near the Cannon.
A deliberate southern regroup retains seven originals and
adds one reserve Hydralisk at frame 62,305. The second
reserve starts at 5 HP and is lost during the rally.
The earlier Cannon-clear save retains eight healthier
attackers and six Drones for a new production branch.

| Local receipt | SHA-256 |
| --- | --- |
| Hydralisk-only Cannon assault | `7d5300742db4aba49e3fe54a4cb8c411624522b8247ddb8edfae14d524c5cc59` |
| Frame-60,626 HOME SRAM | `b5f09458f597587d3cb4fa083250954842d6611060bd0adcf915b1a7e1b859c7` |
| Pylon assault and fresh HOME | `f8f057eb79a48fa14f2d84c8b3d340040a9d6f75c8790ab07a563553a6472b27` |
| Frame-61,867 HOME SRAM | `a16d5e29c6c2ba554279bf69c42499169c4a7d34ca954928c14c425a0e086f06` |
| Direct eastern attack | `c33b3693237f84aff8148ff1c92db0a703d2c1fcb7e49ae1cc6bacda6f79e68d` |
| Southern firing lane | `239481c782d97e6bf3bdece7353e0c79592f94bd8d02b163f4eaea0566a49486` |
| Remaining-Cannon approach | `3035a422b113598072de6ff353c9298109f2ad7670ca41f1b4d103ebc2d719b7` |
| Independent fresh HOME shields and attacker survival | `803642ccec8a96099c4885619d82ed9144bf51f63a6ab2cc32680f79ea239862` |
| Southern regroup and fresh HOME | `0b1f796778503bb706aa1f70c66f42d68e70dd240ef1c55ca6146eb85b646c31b4` |

### Rebuild the eastern attack to eleven Hydralisks

From the frame-60,626 Cannon-clear save, train two
Overlords and three Hydralisks in the south. Fresh HOME
at frame 63,932 has supply 63/82, 409 gas, and all eight
original attackers alive. The three new Hydralisks are
at full health. Group all eleven with the two Overlords,
and verify that the selection survives fresh HOME at
frame 64,983. Kerrigan stays separate at 400 HP.

The Pylon falls at frame 66,244, and seven Hydralisks
remain for the Nexus order. All seven fall. Fresh HOME
at frame 66,460 shows the Nexus at 750 HP and 523 shields,
with the mission active. The earlier direct attempt
depleted 670 shields from the same full value.

A direct bypass from the eleven-unit formation splits
the force into eastern and western routes. No Nexus
damage is observed during the 1,800-update trial. The
last selected surviving Hydralisk fires at the Pylon.
Fresh HOME at frame 66,877 shows the Nexus at 750 HP
and 750 shields and the Pylon at 244 HP and one shield.
Kerrigan remains at 400 HP. The formation checkpoint
preserves the entire force for staging closer to one
approach before committing it.

| Local receipt | SHA-256 |
| --- | --- |
| Train two Overlords and three Hydralisks | `2e71b60696938355505d5b35db522895acf816c55e43003d4fe92bf2dfc829a1` |
| Frame-63,932 HOME SRAM | `685c5f34facb1ad6061f0466d4ff23ce8ce3b54ae647ed64abe4d39bab769645` |
| Form and reload the eleven-Hydralisk force | `fd0dc3fe5a1fa52822fae134dc81cfb94eeb71d595acecce51624739ada66d2e` |
| Frame-64,983 HOME SRAM | `d183a4e95023269b0fbf76b9e35b304ff29206388e02d3f51c9bc37c6d5c9cc8` |
| Pylon and Nexus attack trace | `3f71039d42b6a3752ca93b4d20d156193cde8d32336e056dff5a7135056a14d6` |
| Frame-66,460 HOME SRAM | `42d790027e45f91c58009c9a9b076bfeaee53b6b86ac1c04a0f43d9792a44c7b` |
| Direct bypass trace | `f8e9f1c8861ca940edb5e8bfb3abb72cf506582800d38a952c45946631b6a139` |
| Frame-66,877 HOME SRAM | `2e5671fb9b0a38abee46de8e3bf675344d09a55d6ac654e2d10a9254b2a7f35a` |

### Stage the eastern formation together

The installed Zerg cartridge loads the retained formation
journal and moves the eleven Hydralisks near (1848,1400)
through the ordinary command menu. At frame 65,165 all eleven
are within x1836–1868 and y1401–1417. Kerrigan stays near
(1350,1865) at 400 HP. An independent replay matches the
world fingerprint, followed by a fresh HOME load with the
same friendly roster and byte-identical SRAM.

Moving this group to (1880,1320) and then issuing Attack Move
toward (1768,1264) reaches firing range. The Nexus remains
at 750 HP with 702 shields at frame 66,866. Ten of the
eleven Hydralisks die, and Kerrigan keeps 400 HP. Fresh HOME
restores the assault result and exact SRAM. The intact
formation remains available, with a 9,067-mineral and 409-gas
bank. Reinforce at that position before committing the next
attack.

| Local receipt | SHA-256 |
| --- | --- |
| Independent eleven-Hydralisk staging replay and fresh HOME | `8403e3e4258222774f468cee87c5f75c518bf7d12cc76cb564f284e1abaa90bb` |
| Eastern staging SRAM | `cc444b662f6de1ee57859a7357e5d7b4465f73be425c7ac4d4c04faed9565264` |
| Staged Nexus attack | `c5cd2592000ad471e1e022d5983120466ca1cf17e4374ec1962067b5396d7eee` |
| Assault fresh HOME | `6cffd5bbf2b6659436836d8c9a1968a2dc72f04125be17c352e1d3bde5c2e94e` |

### Reinforce and destroy the eastern Nexus

Five paid Hydralisks from the southern Hatchery join the eleven
at the eastern staging point. The completed sixteen-unit group
and two Overlords fresh-load at frame 69,098. The bank is 8,617
minerals and 259 gas.

An ordinary Attack Move order takes down the Pylon and then the
eastern Nexus. At frame 70,971 the eastern Nexus is gone, two of the
attacking Hydralisks survive and Kerrigan retains 400 HP. Independent
replays of the production, staging and assault each match the sampled
world and SRAM. Fresh HOME confirms the result. The western Nexus at
(400,1704) retains 750 HP and 750 shields, and the mission is active.
At this save the southern Hatchery is also under attack and the Den
is gone. Preserve the base and rebuild production for the western
attack.

A separate branch at the reinforcement checkpoint keeps the
original eastern force in place. The five paid Hydralisks
intercept a Reaver and two Dragoons approaching the southern
base. All three attackers are gone at frame 69,399, with the
Hatchery at 1,250 HP, the Pool at 750 HP, the Den at 356 HP and
Kerrigan at 400 HP. The independent replay and fresh HOME
match SRAM exactly. The eastern Nexus remains on this branch.
Protect production and Kerrigan through the next wave before
combining defense with the objective attack.

The defensive branch moves Kerrigan to (1216,1984) and pays
for a second Den at (1856,1904). Both Dens remain on the
fresh frame-71,099 save, with the new Den at 850 HP and the
Hatchery and Kerrigan at full health. The eleven eastern
Hydralisks remain staged. Both Nexuses persist on this branch.

Six more paid Hydralisks stop the second Reaver wave by frame
74,254. The replacement Den stays at 850 HP and Kerrigan at
400 HP. The Hatchery falls to 359 HP and the branch loses its
Pool and workers. Fresh HOME restores the surviving buildings
and the exact SRAM. Worker recovery is the next test on this
branch.

Two paid Drones hatch by frame 75,130. One starts a replacement
Pool at (1792,1800), and the other resumes mining. Fresh HOME
at frame 75,299 restores the unfinished Pool, the remaining
40-HP Drone, a queued Drone and Kerrigan at 400 HP. SRAM is
byte-identical. A later passive continuation loses the workers
and leaves Kerrigan at 204 HP with the Pool still unfinished.
The saved recovery still needs active defense before it can
sustain production.

| Local receipt | SHA-256 |
| --- | --- |
| Independent production, staging and eastern attack replay | `f93f73071a2371c917d3c986cbedf905b5105999c632fcd8d54b3355ac89cb58` |
| Eastern Nexus destruction SRAM | `fe7f4dd2fc1a9b8901dec10aa30640c4369dd0b7d151d9b0495b0d464bf921ed` |
| Eastern Nexus destruction fresh HOME | `a3d1a7ee3432e632862a8303f02871dfb511bd7b5412bd8938b72521c88b4215` |
| Separate southern defense, independent replay and fresh HOME | `e9ddfb4fbe70ad677701c4302ce2ae00f5ed99bc932b8d6d9ce7cae107bbdb66` |
| Paid replacement Den and fresh HOME | `0795ee44ac206154e8f8c66199d083c2d225f1bef32a33d980a19a98178adfb1` / `c0bdc284519aa4258c8e605dd0662f8106de6fbe78e366a1b2224c0a5277af5b` |
| Second Reaver wave and fresh HOME | `ab2620100cf6b32887b4374f00297f1195c59ce97321f43d22618faf18b934e3` / `f9fb9a51dc12e56c744be87fe1d45adabbcbc344086e8e89abb48a2e1dabd492` |
| Worker recovery and replacement Pool fresh HOME | `756e85581be9f42c917bc51b2739e60bee9845e7c98b0bdf97ba0ff6d5b30967` / `e9435467f708ae3521ab362d2cc4d90cbad0c14b599f9dbefe4761b7e4fad8e0` |

### Rebuild after the eastern Nexus falls

Continue from the verified frame-70,971 eastern victory over
the first Nexus. Acknowledge the authored text, then move Kerrigan
south and evacuate all three Drones east ahead of the Reaver.
At frame 71,985 Kerrigan has 400 HP at (1205,2000), and the
Drones survive with 32, 32 and 12 HP. One Drone pays 300
minerals for a Hatchery at (1952,1664). It completes with 1,250
HP at frame 74,303. The original buildings have fallen and the
eastern Nexus stays destroyed.

Three more paid Drones hatch during the next continuation.
The recovered base finishes a Pool at (2000,1664) with 750 HP
by frame 77,041. Fresh HOME restores the full-health Hatchery,
the Pool, three Drones, Kerrigan at 400 HP and the 8,959/259
bank. The western Nexus still stands. Independent replays of
the evacuation, Hatchery, paid Drones and Pool reproduce all
five checkpoints and their SRAM.

The next Den order is paid but a Reaver damages the new Hatchery
before the nearest Hydralisks can reinforce it. Two miners die
before the Den finishes. Preserve the frame-77,041 save and
prepare a defense before extending the base.

A separate trial from frame 74,303 sends Kerrigan from
(1205,2000) toward (1860,1700) using the ordinary Move command.
At frame 77,160 she remains at the starting position, at 400 HP,
with the order and destination retained. The Reaver reaches
(1808,1640) and reduces the Hatchery from 1,250 to 953 HP.
This establishes that giving the direct order earlier does not
protect the Hatchery. Check intermediate waypoints or stage the
defenders on the eastern side before the evacuation.

#### Intercept the Reaver from the connected approach

The read-only walk grid at frame 74,304 puts the earlier
destination (1860,1700) in a different connected region
from Kerrigan. The Reaver's approach at (1808,1640) is
reachable. Six ordinary move orders through (1400,2008),
(1512,1928), (1576,1800), (1592,1624), (1768,1608) and
(1816,1640) take Kerrigan to (1812,1640) by frame 75,525.
Fresh HOME restores her at 400 HP and the Hatchery at
1,250 HP with byte-identical SRAM.

Continuing from that save, an attack order toward (1684,1640)
kills the incoming Reaver by frame 76,966. Fresh HOME at
77,066 restores Kerrigan at 204 HP, the Hatchery at 1,250 HP
and identical SRAM. The eastern Nexus is still gone and the
western Nexus still has 750 HP and 750 shields. A longer
continuation takes further damage from another assault.
An independent replay of both recorded controller sequences
matches the two saved states and their SRAM byte for byte.
Recover workers and prepare a second defense from this checkpoint.

#### Pool completion under the next attack

Starting from the post-Reaver save, move Kerrigan west before
ordering the first Pool. At frame 78,803 she retains 204 HP
at (1405,1600). The Hatchery has 1,085 HP and the completed
Pool has 636 HP. Fresh HOME re-exports identical SRAM and
confirms that the western Nexus still stands. The eastern
Nexus remains absent.

An ordinary queue order adds three Zerglings. Before any
hatch, the attackers reduce Kerrigan to 141 HP, the Hatchery
to 714 and the Pool to 370. Retain the post-Reaver checkpoint
as the starting point to move in existing Hydralisks before
spending time on production.

One test moves the five reserve Hydralisks from the northwest.
The read-only walk grid gives a 184-cell connected route to
the previously verified Kerrigan corridor. All five arrive
near the first order at (392,500), and three lose 20 HP during
that first leg. Fresh HOME at frame 77,713 retains all five,
Kerrigan at 204 HP and the Hatchery at 1,250 HP. Hostile units
hold the approach ahead, so a route through the terrain needs
separate combat coverage.

Two further transfers start from the same fresh HOME, with all
five Hydralisks already selected. One orders them toward the
next southern waypoint and the other withdraws west. Both lose
two Hydralisks in about 150 updates. Kerrigan and the Hatchery
retain 204 and 1,250 HP. A Dragoon and two Zealots hold the
southern approach. Continue from the five-Hydralisk save while
testing an earlier intercept or an independently qualified
transport policy.

| Local receipt | SHA-256 |
| --- | --- |
| Independent post-Nexus evacuation and Hatchery | `050ed6fa7278eb19a8b89709da4181999011c2bcc4e504658bf48852b33559b5` |
| Independent paid Drones, Pool construction and fresh HOME | `ebe2ed0edb5bd9745e7aa4f9c45494509df26bc6d385cf4a5387bffc32fe24fe` |
| Recovered base SRAM | `7d4644978c21f581b8f026931ba79dcaa61297647142d3cbb246466e46dd46c9` |
| Earlier Kerrigan move order and Reaver arrival | `6919601987baf63e20bf4dbfe1141ba0b3bab9523a0baf31eefb5207ac5f3d74` |
| Connected approach and waypoint orders | `f9c5b455cabd969241cce5ff4f2caf8a27d01ca7862d577ff9a24b2a1159ce47` |
| Fresh HOME before the Reaver arrives | `9b2f51a46f1dbdfa4e66d129562de5269f7f72f8c2b3c79ac4bcbed9f468039a` |
| Reaver interception | `c5c7a7124d1b56e041ea7cd2ec5dd559fc1ea05cd0e184c92f359cda09b9f214` |
| Fresh HOME after the interception | `3bcce9106cdc40b817a93ec9c47afc23b3f7f67e86b73f4d96a81723656e2a5a` |
| Independent replay of both controller sequences | `fef400e34cae62e698496d4d7ff1a598c936c034c718b01c4a9bd8581df15e0c` |
| Post-Reaver SRAM | `4c8f492030fb634fbee9a244924fbc0a896e6c478b95daa7d748eca6cedff9a8` |
| Follow-up Pool construction | `25c8563db1ae473b6f9a7c558d3dd0779a3364c6c4fab44478bb1014ea67b88a` |
| Byte-identical fresh HOME for the new Pool | `5a8703885bc9e356a4099ee7073ca50eea9e037fc7580c4a4b5538d318a41ec3` |
| Zergling queue during the following attack | `c74a00c5a14f396e4c7a210d5fddf24d9421818f492e9f95506ce5de628c0664` |
| Hydralisk movement and first damage checkpoint | `70e8152c86bc073afc922148e4536eb44150685a87f273f4bc3ecfe88209679e` |
| Fresh HOME for the Hydralisk waypoint | `b7e07a0cb5d3f400cf687afceac85a85884c66725bd1b8b54dca689e94282464` |
| Southern transfer under ground attack | `f028a0303b694a168dd3a879e590b4b0349434f5c0631d3caefaa4e98f573660` |
| Western withdrawal under ground attack | `236b081e6aaba4d7420426962918e866f63f83e7a45f349c3d0422151b6a1634` |

### Evacuate workers before the eastern Reaver reaches the base

An earlier branch from frame 71,099 moves five Drones away
from the raid and pays for replacement Hydralisks. Fresh HOME
at frame 72,755 restores all five Drones, Kerrigan at 400 HP,
a full-health Hatchery, the new Den and 19 Hydralisks, including
11 staged to the east. Both remaining Nexuses and the approaching
Reaver are visible in the read-only actor census. The three
nearby Hydralisks lose their follow-up interception while
all five evacuated workers survive. An earlier attempt to
bring the large eastern force directly back through the contested
corridor also loses most of its Hydralisks. Retain the evacuation
save and combine the worker move with an earlier local defense
or a second production base.

| Local receipt | SHA-256 |
| --- | --- |
| Worker evacuation and paid Hydralisk inputs | `772c6a78ab490c0d23ae499aa1607fb26fb120f8b3aa212be83bb2514b52c3e1` |
| Evacuation checkpoint SRAM and readback | `71888030a6470bd692bf626be7edd148c1792bb9f82b8736dc68d38a50d33619` |
| Evacuation fresh HOME verification | `22db8614a82d6a89173455dc23bfae950e2637dd737724a67ec055389614e161` |

### Native Overlord research permission

The retained native Z8 scenario matches the recorded source
hash. Its human upgrade restrictions start Ventral Sacs at
level zero and permit level one. The same human slot allows
Overlords and Lairs. This is scenario data evidence. Native
controls still need a reachable research and boarding test.

The current GBA custom transport research job is scoped to
race mode, while Z6 has a separate campaign policy. The save
loader clears custom research state in campaigns and accepts
loaded campaign Overlords only under the Z6 policy. A new
campaign policy would need the same explicit owner and unit
admission in controls, research and validation, with fresh
HOME coverage for in-progress research and loaded cargo.

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
