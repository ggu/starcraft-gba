# Custom Game Larva and Eggs

Checked on 2026-11-25 and 26 on branch `claude/zerg-larva-eggs-0a3380`
merged with base `81e76943`, with the same prepared inputs for base and
change. The rules and their references are in
[Custom Game Larva and Eggs](../DEVELOPMENT.md#custom-game-larva-and-eggs).
Host results were first taken against base `7316734e`; the AI matrix scores
the two bases identically case by case, and every result below was repeated
on the merged tree.

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

## ARM

`tools/sc64_custom_larva_eggs_arm.py`, port-free mGBA runner
(`10189afc…`), physical buttons only, blank save, Z10 ROM
`09c8bfa8…d4cc` on the merged tree, Outpost, Zerg against one Zerg opponent:

| Point | Update | Human Eggs (timers) | Computer Eggs (timers) | Queue entries |
| --- | --- | --- | --- | --- |
| Before the orders | 272 | 0 | 2 (151, 295) | 0 |
| After three Drone orders | 278 | 3 (369, 371, 373) at three places | 2 | 0 |
| Pause save | 309 | 3 (338, 340, 342) | 3 (114, 258, 354) | 0 |
| Fresh process after HOME load | 309 | identical fingerprint | | |
| 900 updates later, both processes | 1,209 | identical fingerprint | | |

The SRAM image is `5cea47ac…f3dc`; the load leaves it unchanged. Runs on two
earlier builds of this branch wrote byte-identical SRAM.

## Computer opponents

The AI matrix (607 cases, four profiles) passes 600 cases, as both bases do.

- Two raid recoveries that failed in the base now pass (Outpost, Protoss
  human against a Terran opponent, Protoss and Showcase profiles).
- Two now fail: Blood Bath, mirrored Zerg against a Zerg opponent whose main
  and workers are destroyed, Protoss and Showcase profiles. The opponent
  recovers, both armies destroy each other, and the world ends with one Drone
  that alternates between gas and minerals without delivering, so the
  fixture's required winner never comes. The Terran and Zerg profiles finish
  the same case.
- Across the 28 full matches with a Zerg opponent, the median match length is
  0.92 of the base, the mean 29,085 updates against 47,147 (the base mean
  includes a draw at the cap and three matches of 135,245 updates), and 7
  change winner. The 46 without a Zerg opponent keep their median length; 2
  of them, with a mirrored Zerg human, change winner.
- The Zerg opponents' peak army supply has median 51 against 46.

## Performance

Emulated cycles per update over updates 6,000 to 9,000 of a passive human on
Blood Bath, the same inputs for base `1015c70e…92fb` and change
`09c8bfa8…d4cc` (both Z10, merged base):

| Lobby | Base | Change | Difference | Units at window end (base, change) |
| --- | --- | --- | --- | --- |
| Zerg human, three Zerg opponents | 579,012 | 607,834 | +5.0% | 109, 136 |
| Zerg human, Terran, Zerg, Protoss | 575,719 | 588,466 | +2.2% | 109, 117 |
| Terran human, Terran, Protoss, Terran | 572,680 | 572,568 | −0.02% | 110, 110 |

The Zerg lobbies are different worlds: with Eggs the Zerg opponents field
more units. With no Zerg player both ROMs play the identical match, so the
code itself costs nothing measurable. The same three windows on base
`7316734e` gave +7.9%, +3.9% and −0.009%.

## Memory

Every delivered ROM links and verifies: Terran, Z7, Z8, Z9, Z10, P7, P10 and
Showcase. On the merged base:

| ROM | `__bss_end` base → change | `__iwram_end` base → change | Hot-IWRAM stack check, spare bytes base → change |
| --- | --- | --- | --- |
| Z10 | 0x020399a0 → 0x020399a0 | 0x030062d0 → 0x030062d0 | 592 → 588 |
| Terran | 0x0203a7fc → 0x0203a800 | 0x03005c58 → 0x03005cf0 | 304 → 184 |
| P10 | 0x0203b268 → 0x0203b26c | 0x03006190 → 0x030062a8 | 828 → 444 |
| Showcase | 0x02038acc → 0x02038ad0 | 0x03005078 → 0x03005138 | no hot list |

The growth is the Egg branch of the order loop and the Egg priority in target
selection. In products without campaign actor tables the Custom Game Egg keeps
the default actor flags and collision radius, so the movement loops are
unchanged. The older chapter ROMs that do not take the capability (Z3, Z4,
Z6, P3, P6, P8, P9, races, unified, skirmish, Backwater) also link and
verify; `verify-campaign` and `verify-sc64` need private inputs that were not
prepared.

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
linked. All 32 save points continue identically. Z7's mission Eggs use the
same pair spawn and are unchanged here.

## Not established

Real hardware; campaign adoption (missions keep their production); native
Larva positions, Egg animation and birth timing; diagonal-only exits for a
newborn.
