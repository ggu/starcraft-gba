# StarCraft 64 computer players on Blood Bath, and the GBA planner

The custom-game planner ([SKIRMISH-AI.md](../SKIRMISH-AI.md)) was authored
without reference timings. This audit watches the StarCraft 64 computer
players on Blood Bath, compares the same lobbies on the GBA build, ranks the
differences and records the one that was fixed: **when each race starts its
first offensive and how much army it gathers first.**

## Method

The owned USA ROM ran in a private ares copy with scripted controller input
(Scenario, Setup Custom, Free for All, Blood Bath, human Terran). The human
gave no in-game input. Every 60 to 700 game frames, about 200 as a rule, a
save state was written and the game's own unit table (900 records of 312 bytes) was read from it:
owner, unit type, completion flag, hit points and position of every actor.
The game-frame counter at `0x800B1E84` is the time base. Build durations
read this way match the rule tables within the sampling interval (a
Barracks 1,115 frames from first seen to complete against 1,200; a Gateway
939 against 900), so one native step is one game frame. The GBA engine runs
each game frame as 1.25 updates (`unit_build_time`), so update numbers
below are game frames times 1.25, and match time is updates divided by 30.

Five matches were recorded:

| Run | Lobby | Recorded game frames | Ended by |
| --- | --- | ---: | --- |
| 1 | Zerg, Terran and Protoss computers | 1,474 to 14,055 | Human defeated at 13,869 (result screen 13:05 at the default speed) |
| 2 | Zerg, Terran and Protoss computers | 1,069 to 38,267 | Stopped. Defeat was disabled, see below |
| 3 | Zerg computer only | 92 to 14,854 | Human defeated |
| 4 | Terran computer only | 531 to 10,025 | Human defeated |
| 5 | Protoss computer only | 471 to 9,317 | Human defeated |

Run 2 used one private memory edit so the match would outlast the passive
human: the shared defeat condition (`Command at most 0 buildings`) was
changed to one that is never true, for every player. Nothing else was
written. It is an observation aid, and nothing in run 2 after game frame
10,984 (the human's last building) is evidence about results or elimination.
Emulator frames, save states and the census logs stay in ignored local
folders. Computer players' banks could not be read: the only mineral word
found was the human's displayed count.

The GBA side is `tests/sc64_skirmish_ai_progress.c`, Terran profile, Blood
Bath, the same lobbies, a passive human, at base `2655cb22`. The change
was then rebased onto `b7a7a614` (one more planner fix for the first
Protoss Gateway); the after-change numbers and the cost table are from the
rebased code.

## What the StarCraft 64 computer players do

Game frames, with the worker count at that moment in parentheses.

| | Terran (runs 1, 2, 4) | Zerg (runs 1, 2, 3) | Protoss (runs 1, 2, 5) |
| --- | --- | --- | --- |
| First production building | Barracks at 1,391 to 1,394 (7 SCVs), before the first Supply Depot at 1,908 to 2,065 (8) | Spawning Pool at 3,236 to 3,372 (11 Drones), after a second Hatchery at 2,737 to 2,935 (11) | Gateway at 2,301 to 2,486 (10 Probes) and a second at 3,049 to 3,076 (12); or, in run 2, a second Nexus at 2,513 (7) and a Forge at 4,683 (11) |
| Gas | Refinery at 2,935 to 3,076 (10 to 11) | Extractor at 3,616 to 3,816 (11 to 12) | Assimilator at 6,197 to 6,407 (17), or 9,602 (24) with the Cannon opening |
| Next steps | Second Barracks 3,564 to 3,818; Academy 4,848 to 5,124; Factory 8,834 to 12,807 | Hydralisk Den 6,499 to 6,634; Lair 7,705 to 7,861 | Third Gateway 5,762 to 5,764; Cybernetics Core 9,317 to 12,550 |
| Static defense before the first wave | Bunker at 7,151 to 8,271 (run 2: 793 native pixels in front of the base) | Two Creep Colonies at 3,725 to 4,263, four Sunken Colonies by 6,001 to 6,417, in all three runs | Shield Battery at 4,047 to 4,229; or 11 Photon Cannons by 8,845 |
| Workers at game frame 8,000 | 17 to 20 | 16 | 18 to 22 |
| Town halls | One until a second Command Center at 18,748 (run 2) | Two through 38,000 in run 2 (the second 326 pixels from the first); a third at 13,998 in run 3 | Second Nexus at 2,513 and a third at 13,075 on the dead human's start (run 2); one through the end of runs 1 and 5 |
| After losses (run 2) | A destroyed Comsat Station was started again 289 frames later | | Destroyed Photon Cannons were started again 1,002 and 1,465 frames later |

The Protoss opening differs between run 2 and runs 1 and 5, so the game
chooses among builds. The Zerg opening was the same within 200 frames in
all three runs, and the two Gateway openings within 190.

### First offensive

A wave starts at the first sample with six or more army units more than 640
native pixels from the owner's start. Samples are 60 to 260 frames apart,
and the order to leave was given a few hundred frames before that.

| Run | Race | First wave, game frames | Units sent, of army | Army supply held (doubled units) | Target |
| --- | --- | ---: | --- | ---: | --- |
| 1 | Protoss | 8,505 (4 away at 8,271) | 12 Zealots of 14 | 56 | Terran computer |
| 1 | Terran | 12,807 | 17 of 23 (10 Marines, 4 Firebats, 3 Medics) | 46 | Human |
| 1 | Zerg | none by 14,055 | 42 units at home | | |
| 2 | Terran | 9,865 | 12, then 22 of 28 | 46 | Human |
| 2 | Protoss (Cannon opening) | 13,363 | 6, then 9 of 10 | 36 to 40 | Human's ruins |
| 2 | Zerg | 13,780 | 6 Zerglings, then 18 of 33 | 46 to 55 | Protoss computer |
| 3 | Zerg | 13,396 | 12, then 38 of 39 | 74 | Human |
| 4 | Terran | 8,688 | 14 of 17 (11 Marines, 3 Medics) | 34 | Human |
| 5 | Protoss | 8,253 | 6, then 12 Zealots of 13 | 52 | Human |

Medians: Protoss 8,505, Terran 9,865, Zerg 13,780 (its third value is only
known to be later than 14,055), and 46 to 50 supply held across the eight
waves. In run 2 the Terran computer's second wave left 3,600 frames after
its first (13,634, 17 to 22 units).

## The same lobbies on the GBA build, before the change

In 60 lobbies (both maps, the three human races, ten opponent sets of one to
three) every opponent's first offensive left between update 10,800 and
11,000, that is game frame 8,640 to 8,800, whatever its race. The only
exceptions were small pursuits on the smaller map. With one opponent the
passive human had lost by update 11,640 to 11,920 in all 18 lobbies.

| Difference | StarCraft 64 | GBA base | How well verified |
| --- | --- | --- | --- |
| **First offensive** | Protoss 8,253 to 8,505 (13,363 with the Cannon opening); Terran 8,688 to 12,807; Zerg 13,396 or later. Army held: 34 to 74 supply, median 46 to 50 | 8,640 for every race; 24 supply | Eight N64 first waves in five matches; 60 GBA lobbies, deterministic |
| Static defense before the first wave | Four Sunken Colonies, a Bunker, a Shield Battery or Cannons, in every run | None before update 18,000 unless the enemy has air units | Every N64 run; every GBA lobby |
| Zerg town halls at game frame 8,000 | 2 | 5 | Three N64 runs. The GBA count follows from serial larva queues, an engine limit |
| Tech path | Terran stays on Barracks units to 10,000; Protoss on Zealots to 11,000 | Terran Factory at 3,800; Protoss Cybernetics Core at 2,800 | The N64 game varies its builds, so two or three runs do not bound it |
| Minerals per worker | 0.048 per game frame (passive human, four SCVs, runs 1, 3 and 4) | 0.087 | Engine rate, outside the planner |
| Protoss production | 13 to 17 Zealots by 8,500 from three Gateways | 8 army units at 8,000 with 3,400 minerals banked (one lobby); two Gateways for a whole match in another | Placement of powered sites delays Gateways |

The first offensive is the largest difference a player meets directly. On
the GBA three opponents of different races all arrived at six minutes with
half the army, and a passive player was dead 40 seconds later whatever the
opponent. In the original the Zerg computer attacks last, about five
thousand game frames after the GBA one did (60% later), Terran about a
thousand later, and the three are staggered over more than five thousand
frames. It is also the best verified row, and it is a planner constant
rather than an engine rule. The other rows are left as they are.

## The change

`starcraft/race_skirmish_ai.h` replaces the shared six-minute gate with one
per race, the median first wave measured for that race, and raises the army
threshold from 24 to 46 supply, the median army a computer player held when
its first wave left. Two paths around the gate and the threshold are closed
with it. Units that are far from home before the gate (born at a distant
Hatchery, or left where they defended an expansion) return to the rally
point, where before they could start an attack through the reinforcement
branch at any time. And choosing an objective from the field now needs the
full threshold, as it does from home: a dozen supply of such units could
otherwise launch the moment the gate opened. The cost is that an army worn
down below the threshold goes home after its objective falls instead of
taking the next one, so mirrored test matches last longer (below).

| Race | N64 first waves, game frames | Median | Gate | Match time | Was |
| --- | --- | ---: | ---: | ---: | ---: |
| Protoss | 8,253; 8,505; 13,363 | 8,505 | update 10,631 | 5:54 | 6:00 |
| Terran | 8,688; 9,865; 12,807 | 9,865 | update 12,331 | 6:51 | 6:00 |
| Zerg | 13,396; 13,780; after 14,055 | 13,780 | update 17,225 | 9:34 | 6:00 |

GBA workers mine about 1.8 times as fast as the original's, so a GBA
opponent usually holds the threshold before its gate, and the gate decides
when the wave leaves. The gates come from three matches per race of a game
that varies its play. They are a calibration, not a decoded script, and
they do not make the GBA army compositions, targets or later waves match
the original.

After the change, in all 72 single-opponent lobbies (two maps, three human
races, three opponent races, four build profiles), the first offensive
order comes at update 10,656 for Protoss (18 to 22 units, 72 to 92 supply),
12,336 for Terran (38 to 42 units, 94 to 102 supply) and 17,232 for Zerg
(31 to 35 units, 52 to 70 supply), the whole army on that update, and no
unit carries such an order before the gate. The passive human then loses at
update 11,482 to 11,675 against Protoss, 13,157 to 13,406 against Terran
and 17,969 to 18,331 against Zerg in the matrix's 24 single-opponent runs;
before, at 11,629 to 11,930 against any of them.

In the 78 mirrored full matches of the test matrix (the planner drives both
sides), every match still ends in the engine's victory or defeat. They last
longer: 28,988 updates on average against 22,221, the longest 89,976
against 43,648. The longest run of minute snapshots with 1,500 minerals
banked is 14 before and after, and of idle workers 1. Small-map lobbies
with four owners that end with nothing left able to act (the
mirrored-fixture outcome `draw: exhausted`) were 18 of 108 before; with the
gates alone they were 20 of 108.

## Recovery after a raid

The N64 computer players rebuilt single lost structures within 289 to
1,465 game frames (above). The GBA planner's answer to a raid on its
economy is measured by the `--raid` fixture in 88 mirrored lobbies: at
update 9,000 or later every worker of one opponent dies (56 lobbies), or
its main depot and all but two workers (32). In all 88 it has six workers
with four mining again 1,080 to 2,760 updates after losing the worker line
and 1,200 to 4,160 after losing the depot as well, delivers 200 minerals
by 1,577 to 4,772 updates after the raid, and the match ends in the
engine's result between update 13,613 and 49,183; the raided opponent goes
on to win 43 of them. Two single-opponent lobbies are left out because the
mirrored Terran human destroys the raided Protoss before it has delivered
200 minerals; on the smaller map its rebuilt workers haul from its second
Nexus to its first, where the engine returns all cargo.

## Cost

Emulator cycles per main-loop update on the Terran ROM (mGBA core, the
port-free replay backend), Blood Bath, a scripted Zerg human against
Protoss, Terran and Zerg opponents, each ROM replaying the input tape its
own host controller recorded. The two matches differ after update 10,631,
so the busiest windows are different fights.

| | Base `b7a7a614` | After |
| --- | ---: | ---: |
| Busiest 1,000 updates | 10,904 to 11,903 (136 to 115 units) | 11,835 to 12,834 (122 to 105 units) |
| Mean, whole update | 1,045,148 | 977,846 |
| 95th percentile | 1,287,931 | 1,258,643 |
| Worst update in the window | 1,705,824 | 1,666,247 |
| Mean `game_update` in the window | 611,829 | 546,135 |
| Whole match: mean, worst update | 758,725; 2,561,092 | 697,416; 1,980,582 |
| `game_update` on the 375 planning updates of frames 3,000 to 8,999 (identical play) | 438,719 | 438,705 |

A 30 fps update is 561,792 cycles. The planner's own cost is unchanged; the
busiest window is lighter because the three opponents no longer attack on
the same update. RAM is unchanged: `.data`, `.iwram` and `.bss` have the
same sizes, and the ROM's code grows by 392 bytes.
