# Custom Game Larva and Eggs

Checked on 2026-11-25 and 26 on branch `claude/zerg-larva-eggs-0a3380`
merged with base `b6b601f2`; comparisons are against base `d39bd015`, and the
later base commits change tools, documents and the skirmish pack gate only.
Base and change used the same prepared inputs, including the 56-patch Blood
Bath skirmish pack of
[#887](https://github.com/ggu/starcraft-gba/pull/887). The rules and their
references are in
[Custom Game Larva and Eggs](../DEVELOPMENT.md#custom-game-larva-and-eggs).
Earlier rounds against bases `7316734e` and `81e76943` used the 32-patch pack;
their figures are superseded by the ones below.

## Host lifecycle

`make test-custom-larva-eggs` builds `tests/sc64_custom_larva_eggs.c` under
ASan and UBSan with the flags of the Terran, Z10, P10 and Showcase ROMs and of
the canonical Z7 and P7 profiles. In each it plays a Custom Game with a Zerg
human and a Zerg computer opponent through the controls and checks:

- three Larva at the start; three Drone orders from one Hatchery are three
  Eggs, each charged its price, a Larva and its supply at the order; with no
  Larva nothing is charged;
- all three count down together and each hatches on the update its own timer
  ends, where a ground unit can walk away;
- one more Larva every 428 updates, up to three;
- a Zergling and a Scourge Egg each hatch two, holding both units' supply from
  the order;
- **CANCEL EGG** refunds minerals and gas and releases supply; a killed Egg
  refunds nothing and hatches nothing;
- at the supply cap an order is refused without charge, while an Overlord is
  still accepted;
- a Hatchery finished during the match starts with one Larva;
- a pause save after a cancel and a death is written and loads in the same
  process through HOME with its Eggs;
- in the Terran, Showcase and P7 profiles, the loaded record marked as a
  campaign record has its Egg rows refused. Z7, Z10 and P10 leave `race_data`
  transformed after a load, so that direct check is skipped there; the real
  loader runs the same function;
- the computer opponent holds up to five or six Eggs at once and never a queue
  entry.

All six profiles pass.

## Saves

In every profile, a pause save with three human Eggs and two or three
computer Eggs, a Larva timer running and no queue entry, is written to a
32 KiB SRAM image. A fresh process loads it through HOME, and the world
digest (every live unit and building field, banks, Larva and timers) is
identical at the save and 900 updates later. The same check passes on Blood
Bath against three Zerg opponents.

An earlier serial-queue record loads: its queued orders complete and a new
order is an Egg. In the Zerg profiles the record is written by the same
sources built without the capability; its SRAM image is byte-identical to the
one the base sources write. That build also refuses the record with Eggs and
stays on HOME. Products that append the Egg cannot be built without it, so
their earlier record is placed by the test.

## Campaign missions

The Z10 campaign controller (`build/test-sc64-z10-campaign-run`) writes a
byte-identical input and checkpoint log on the base and on this branch: the
mission is won at frame 49,302 in both. Its tape replay
(`make test-sc64-z10-replay`) fails the same way on the base and here: the
replayed match leaves the recorded one and restarts the mission. That failure
is not caused by this change.

## ARM

`tools/sc64_custom_larva_eggs_arm.py`, port-free mGBA runner
(`10189afc…`), physical buttons only, blank save, Z10 ROM
`f22d24c2…6ed7e`, Outpost, Zerg against one Zerg opponent:

| Point | Update | Human Eggs (timers) | Computer Eggs (timers) | Queue entries |
| --- | --- | --- | --- | --- |
| Before the orders | 272 | 0 | 2 (151, 295) | 0 |
| After three Drone orders | 278 | 3 (369, 371, 373) at three places | 2 | 0 |
| Pause save | 309 | 3 (338, 340, 342) | 3 (114, 258, 354) | 0 |
| Fresh process after HOME load | 309 | identical fingerprint | | |
| 900 updates later, both processes | 1,209 | identical fingerprint | | |

The SRAM image is `5cea47ac…f3dc`; the load leaves it unchanged. Every
earlier build of this branch wrote the same SRAM image.

## Computer opponents

The AI matrix (607 cases, four profiles, unsanitized `-O2` harness) on the
56-patch pack:

| Tree | Passed | Failed |
| --- | ---: | ---: |
| Base `d39bd015` | 530 | 77 |
| This branch | 541 | 66 |

The base itself fails 77 cases on this pack; those failures are not examined
here. Thirteen of them pass with this branch. Two cases fail only here:

- `match-terran-0-1-zpt`: on Outpost a Zerg opponent is destroyed at update
  21,036 having made only Zerglings, so the fixture's second army type is
  missing (on the base it dies at 19,877 after making Hydralisks too);
- `match-zerg-1-1-tzp`: a Terran opponent banks 1,500 minerals below its unit
  share for 19 one-minute snapshots, over the limit of 15 (9 on the base).

Both are full matches whose course changed, not stalls. The 56-patch layout
exposed two planner faults that the matrix flagged and that this branch
fixes (`starcraft/race_skirmish_ai.h`):

- a Zerg owner with no ore near its halls held a Hatchery's price it could
  never reach, so its bank stayed above the all-in limit and its army never
  left (the `air-pin` fixtures). Both Hatchery reserves now hold only when the
  bank and the ore its Drones carry back reach the price;
- a raided Zerg owner kept thousands of minerals with three Hatcheries,
  because its next planned Hatchery waited for 12 or 14 workers. With every
  Larva spent and 1,000 minerals banked, that step no longer waits for the
  worker count.

Across the 28 full matches with a Zerg opponent, the median length is 0.82 of
the base, the mean 31,578 updates against 47,873 (the base includes two
matches of 136,814 updates; the longest here is 52,787), and 4 change winner,
all with a mirrored Zerg human. The 46 without a Zerg opponent keep their
median length; 6 change winner, all with a mirrored Zerg human. The Zerg
opponents' median peak army supply is 43 against 42.

## Custom Game full matches

The whole-match tapes of
[#890](https://github.com/ggu/starcraft-gba/pull/890)
(`make check-sc64-custom-full-matches`) were authored before this change and
before [#892](https://github.com/ggu/starcraft-gba/pull/892). On base
`d39bd015` with the 56-patch pack, 5 of its 11 qualified lobbies still win;
Terran Blood Bath against three computers, Zerg Blood Bath against two, and
all four Protoss lobbies lose without this change. On this branch six lobbies
win, replay on the ROM, continue from their pause save in a fresh process and
reload their victory save; three of them needed a new plan for the player:

| Cartridge | Map | Opponents | `--plan` | Updates | Frame | Pause save (frame, units on screen) | Fresh-process continuation | Victory reload |
| --- | --- | --- | --- | ---: | ---: | --- | --- | --- |
| Terran | Outpost | Zerg, Protoss, Terran | `PRODUCERS=3` | 30,765 | 29,794 | 13,746 (61 own, 7 hostile) | 111 checks match, same result and victory save | opens RESULT, SRAM unchanged |
| Terran | Outpost | Terran, Zerg | `PRODUCERS=2,WORKERS=14` (new) | 28,081 | 27,308 | 14,085 (52 own, 10 hostile) | 82 checks match, same result and victory save | opens RESULT, SRAM unchanged |
| Terran | Blood Bath Melee | Protoss, Zerg | `LOBBY_WAIT=8,FIRST_ATTACK=7000` | 46,414 | 43,822 | 11,379 (11 own, 21 hostile) | 382 checks match, same result and victory save | opens RESULT, SRAM unchanged |
| Zerg Z10 | Outpost | Terran, Zerg, Protoss | `WORKERS=12,LINE=38` (new) | 30,818 | 30,019 | 18,896 (14 own, 5 hostile) | 72 checks match, same result and victory save | opens RESULT, SRAM unchanged |
| Zerg Z10 | Outpost | Protoss, Zerg | `WORKERS=10,LINE=38` (new) | 24,425 | 23,645 | 21,749 (12 own, 6 hostile) | 20 checks match, same result and victory save | opens RESULT, SRAM unchanged |
| Zerg Z10 | Blood Bath Melee | Terran, Zerg | `LINE=38,PRODUCERS=1,LOBBY_WAIT=22` | 23,238 | 22,587 | 12,829 (27 own, 12 hostile) | 128 checks match, same result and victory save | opens RESULT, SRAM unchanged |

The two Zerg Outpost lobbies swap the worker counts #890 gave them. With
#890's counts the player still wins, but never has enough hostile units on
one screen for its pause save.

The other five lobbies of the list lose on the base as well and are not
requalified here.

## Performance

Emulated cycles per update over updates 6,000 to 9,000 of a passive human on
Blood Bath, with the same inputs for base `d39bd015` (Z10 ROM `f4a221a3…`) and
this branch (`f22d24c2…`). The base cartridge was built with the 56-patch pack
before revision 2, which adds only geyser amounts that the game does not read:

| Lobby | Base | Change | Difference | Units at window end (base, change) |
| --- | --- | --- | --- | --- |
| Zerg human, three Zerg opponents | 577,575 | 596,849 | +3.3% | 110, 138 |
| Zerg human, Terran, Zerg, Protoss | 579,791 | 583,385 | +0.6% | 111, 113 |
| Terran human, Terran, Protoss, Terran | 567,526 | 567,431 | −0.02% | 109, 109 |

The Zerg lobbies are different worlds: with Eggs the Zerg opponents field
more units. With no Zerg player both ROMs play the identical match, so the
code itself costs nothing measurable.

## Memory

Every delivered ROM links and verifies: Terran, Z7, Z8, Z9, Z10, P7, P10 and
Showcase. Against base `d39bd015`:

| ROM | `__bss_end` base → change | `__iwram_end` base → change | Hot-IWRAM stack check, spare bytes base → change |
| --- | --- | --- | --- |
| Z10 | 0x020399a0 → 0x020399a0 | 0x030062d8 → 0x030062d8 | 584 → 584 |
| Terran | 0x0203a7fc → 0x0203a800 | 0x03005c88 → 0x03005d28 | 256 → 128 |
| P10 | 0x0203b268 → 0x0203b26c | 0x03006190 → 0x030062a8 | 820 → 464 |
| Showcase | 0x02038acc → 0x02038ad0 | 0x03005078 → 0x03005138 | no hot list |

The growth is the Egg branch of the order loop and the Egg priority in target
selection. In products without campaign actor tables the Custom Game Egg keeps
the default actor flags and collision radius, so the movement loops are
unchanged. The Terran stack check keeps 128 spare bytes. The older chapter
ROMs that do not take the capability (Z3, Z4, Z6, P3, P6, P8, P9, races,
unified, skirmish, Backwater) also link and verify; `verify-campaign` and
`verify-sc64` need private inputs that were not prepared.

## A defect found on the way

When a pair hatches, the second unit is born inside the order loop after the
target index was built, and `spawn_unit` reconciles only the collision
buckets. If its slot came later in the loop and it moved, the index unlinked
it through the stale links of the slot's previous occupant. A dropped cell
head first made late saves continue differently after a load (5 of 32 save
points from update 12,051 to 16,551 of one Blood Bath lobby; 9 with the
earlier opt-in preview, which has the same pair birth), and later a cell's
list became a cycle and hung a match. The Custom Game hatch now links that
unit, or stops the index for the rest of the update when the slot is still
linked. All 32 save points continue identically (measured on base
`7316734e` with the 32-patch pack). Z7's mission Eggs use the
same pair spawn and are unchanged here.

## Not established

Real hardware; campaign adoption (missions keep their production); native
Larva positions, Egg animation and birth timing; diagonal-only exits for a
newborn.
