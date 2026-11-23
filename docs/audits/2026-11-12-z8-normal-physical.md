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
