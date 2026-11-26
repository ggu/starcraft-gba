# N64 parity field map, mission entry and opening timelines

Date: 2026-11-26. Scope: the remaining guest-RAM fields the parity checks
need (S10, S12), entry to every Episode I–III campaign mission (the S7
fallback and WP7), and a first slice of campaign opening timelines (WP10).
Every capture here ran the pinned ares core in a fresh process from power-on
with the frontend described in
[`experiments/ares-headless/CAPTURE.md`](../../experiments/ares-headless/CAPTURE.md);
the GBA side ran the host parity binaries of
[`tests/sc64_parity_run.c`](../../tests/sc64_parity_run.c). Captures, dumps,
tapes and goldens are ROM-derived and stay private; this page holds derived
numbers only.

| Item | Verdict |
| --- | --- |
| Mission entry: all 30 campaign missions from a fresh boot | PASS (staged: one guest edit, the progress bytes) |
| Launch census against the CHK (placements, minerals, geysers, start banks) | 27 of 30 exact; T10, Z10 and Z5 differ by a few unexplained positions (and Z5's data disk) |
| Game speed at launch | selector 4 (56 ms) in every launch, recorded per run |
| Orders (`+0x4D`, names by controlled command) | proven |
| Fog and visibility tiles | proven |
| Sprite and image records, iscript frame state | proven |
| Bullets | proven |
| Hit points (`+0x08`) and resource amounts (`+0xCC`) | proven |
| Research and upgrade state (unit timers, per-player arrays) | proven, except maximum levels and availability (launch cross-checks only) |
| `.dat` table anchor (S12) | located for all 107 columns; 104 equal the ROM's BOLT tables, 3 differ at run time, and each mission's CHK unit settings are applied on top |
| Golden-backed checks | `t1-fields` (13 observations, 11 gating), `t3-opening` (6, report only) |
| Opening timelines (T3, Z2, P1) | measured; the GBA side differs in every mission |

## Frontend additions

Three tape commands and one environment variable; tests in
`make test-ares-capture-view`.

| Addition | Purpose |
| --- | --- |
| `poke ADDR HEX` | The only guest write: RDRAM plus any data-cache line holding the bytes (both alias lines), read back through the guest view. Logged in `events.csv`, counted in `status.log`; `capture.py` records `pokes` and `n64_staged` in the manifest and `parity_decode.py golden` tags such bundles `n64-staged` (owner decision 1, 2026-11-26) |
| `region ADDR BYTES` / `region every K` / `region on` | Extra guest blocks recorded into `extra.bin` on every follow-up read of `tap tick`; `parity_decode.py` joins them to the tick's record and to its hash chain, so captures without `extra.bin` keep their old chains |
| `pulse ADDR BYTES OP VALUE STEPS MASK HOLD GAP` | Press a button only while a field meets a condition, for a fixed number of steps. `pulse 800F0C20 2 ne 0 N 2 3 60` dismisses every text box (Start only while the pause halfword is set) without ever opening the pause menu |
| `SC64_CAPTURE_LOCK` | Lets a session use its own lock file instead of the shared native lock |

Binaries (SHA-256): `9363c669a872da374e71f9775693d3c605cc0980096f807e07db97572e8ad998`
(`poke` and `region`; menu exploration only) and
`db9de872a12ee159bdd5aa1f0df51088dbd528cb28b00734ab2a0ead8f9e603c`
(adds `pulse`; every capture below). The tracked `capture.cpp`,
`capture_view.h`, `capture.py` and `inject.cmake` are byte for byte the
sources of the second build.

**Parallel captures.** For most of the afternoon a long-running capture
service of another session held the shared lock, so the queue stalled for
more than ten minutes at a time. From then on my two workers each used their
own lock (at most two of my captures at once). The check the brief asked for
holds: `f-t1-1` ran under the shared lock and `f-t1-2` ran alongside my
other worker and other sessions' captures, and the two have the same per-tick
hash chain over all 1,190 records, including the 72 MB of extra regions
(chain `208d17ed…`).

## Mission entry

**Progress bytes.** The six bytes at `0x800D13C4` are the number of unlocked
rows per episode (Episode I, II, III, then the three Brood War episodes),
read when the Episode list is drawn; the episode shown is the byte at
`0x800D1182`. A fresh boot with a blank save reads `03 01 01 01 01 01` once
the save loads at step 1,038, which overwrites anything written earlier (a
poke at step 1,000 was lost). Writing `0C 0A 0A` at step 1,700 (on the main
menu) unlocks every row: the Terran list shows Boot Camp, Officer Training
and 1–10, the Zerg and Protoss lists 1–10 (frames checked for all three).

**The episode is chosen on the main menu.** S7 looked for an episode control
on the Single Player screen. The episode is instead the main menu's campaign
carousel (the screen with the Terran, Zerg and Protoss heroes before Single
Player): holding the stick left for 12 steps rotates it once (Zerg Episode
II), twice for Protoss Episode III; Start then opens that episode's list
(`0x800D1182` reads 1 or 2). Zerg and Protoss first show the "play the
episodes in order" warning, which Start continues.

**Launch tape** (`mktape.py` in the private home, every mission the same way):
boot (700 idle steps, Start three times 300 steps apart), `poke 800D13C4
0C0A0A` at step 1,700, carousel rotation, Start, Start on the warning, 1,700
steps, Down once per row (Terran rows 2–11 are T1–T10, Zerg and Protoss rows
0–9), A, then `until 800A5AB4 2 eq 3 12000 2 0 0 10 290` (Start pulses through
the location card and the text briefing until the map mode halfword reads 3),
`until 800B1E84 4 ge 2` and a full-RAM dump, then `pulse 800F0C20 2 ne 0 3000
2 3 60` and a second dump. `tap tick rec` records from the A press.

**Results.** All 30 missions launched on the first attempt with the same
recipe; no tape needed a mission-specific step. Map mode was reached at
frontend steps 4,569 to 5,905 from power-on, the unit table appeared a few
hundred steps later, and every launch read game speed selector 4 at every
in-mission tick, the fresh-campaign default (56 ms a step). The census joins the first in-mission tick's live rows to
the mission's CHK (UNIT placements and THG2 unit doodads) on type, owner and
position, with these classes, none of which is a defect:

- *moved*: a placed unit already 1–8 px from its spot on the first tick
  (computer workers that start mining);
- *displaced*: a placed unit up to 64 px away whose spot has another
  placement within 16 px (a stack the cartridge spread; the L0 census uses
  the same radius);
- *owner changed*: a placed unit whose owner a start trigger changed before
  the first tick (Raynor's Vulture in T2 starts as player 2 and is the
  human's by tick 1);
- *doodads*: THG2 unit doodads (doors, traps) join on type and spot; the
  cartridge gives enabled ones to player 11 and keeps the CHK owner of
  disabled ones;
- *turrets*, *larvae* and *hangar*: subunit rows on their parent's spot, the
  larvae of placed Hatcheries, and the Interceptors and Scarabs on a placed
  Carrier's or Reaver's spot.

Banks are the first-pass SetResources rows of the CHK (the L0 census's
`start_banks`), compared with the N64 bank arrays during the first 32
unpaused ticks.

| Mission | Title (CHK) | Map mode, step | Census | Placements matched | Moved / displaced / owner changed | Turrets, larvae, hangar | Minerals | Geysers | Amounts differ | Start banks | Selector | Player arrays | Table columns differing from BOLT |
| --- | --- | ---: | --- | ---: | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| T1 | T1) Wasteland | 4569 | exact | 46/46 | 0 / 0 / 0 | 0, 0, 0 | 10/10 | 0/0 | 0 | 1/1 | [4] | equal | acquire_range, right_click |
| T2 | T2) Backwater Station | 4670 | exact | 92/93 | 1 / 0 / 1 | 0, 0, 0 | 13/13 | 2/2 | 0 | 2/2 | [4] | equal | acquire_range, right_click, unknown |
| T3 | T3) Desperate Alliance | 4770 | exact | 196/196 | 2 / 0 / 0 | 0, 1, 0 | 25/25 | 2/2 | 0 | 3/3 | [4] | equal | acquire_range, right_click |
| T4 | Untitled Scenario | 4871 | exact | 120/121 | 0 / 0 / 1 | 1, 0, 0 | 0/0 | 0/0 | 0 | 0/0 | [4] | equal | acquire_range, right_click |
| T5 | T5) The Antigan Revolt | 4970 | exact | 167/168 | 2 / 0 / 1 | 9, 0, 0 | 41/41 | 0/0 | 0 | 2/2 | [4] | equal | acquire_range, right_click |
| T6 | T6) New Year's Day | 5070 | exact | 215/216 | 1 / 0 / 1 | 4, 2, 0 | 35/35 | 3/3 | 0 | 1/1 | [4] | equal | acquire_range, right_click, unknown |
| T7 | T8) The Trump Card | 5171 | exact | 209/210 | 5 / 0 / 1 | 22, 0, 0 | 23/23 | 2/2 | 0 | 3/3 | [4] | equal | acquire_range, requirements, right_click, unknown |
| T8 | The Big Push | 5271 | exact | 275/276 | 5 / 0 / 1 | 26, 0, 0 | 76/76 | 2/2 | 0 | 2/2 | [4] | equal | acquire_range, right_click |
| T9 | T11) New Gettysburg | 5370 | exact | 245/246 | 7 / 0 / 1 | 0, 1, 0 | 59/59 | 3/3 | 0 | 6/6 | [4] | equal | acquire_range, right_click |
| T10 | T12) The Hammer Falls | 5471 | differs (2 missing, 2 extra) | 324/327 | 11 / 0 / 1 | 32, 0, 0 | 56/56 | 3/3 | 0 | 6/6 | [4] | equal | acquire_range, right_click |
| Z1 | Z1) Among the Ruins | 4841 | exact | 99/99 | 3 / 0 / 0 | 12, 1, 0 | 24/24 | 3/3 | 0 | 1/1 | [4] | equal | acquire_range, build_time, map_string, right_click, shields |
| Z2 | Z2) Egression | 4942 | exact | 143/143 | 3 / 0 / 0 | 0, 2, 0 | 30/30 | 1/1 | 0 | 3/3 | [4] | equal | acquire_range, build_time, map_string, right_click, shields |
| Z3 | New Dominion | 5042 | exact | 122/122 | 4 / 0 / 0 | 8, 1, 0 | 28/28 | 3/3 | 0 | 4/4 | [4] | equal | acquire_range, build_time, map_string, right_click, shields |
| Z4 | Z4) Agent of the Swarm | 5142 | exact | 181/181 | 4 / 0 / 0 | 15, 1, 0 | 39/39 | 2/2 | 0 | 4/4 | [4] | equal | acquire_range, build_time, map_string, requirements, right_click, shields |
| Z5 | Assimilation | 5244 | differs (10 missing, 9 extra) | 171/183 | 1 / 2 / 2 | 6, 0, 0 | 0/0 | 0/0 | 0 | 0/0 | [4] | equal | acquire_range, right_click |
| Z6 | Z6) The Dark Templar | 5343 | exact | 185/185 | 4 / 0 / 0 | 0, 1, 0 | 60/60 | 3/3 | 0 | 4/4 | [4] | equal | acquire_range, build_time, map_string, requirements, right_click, shields |
| Z7 | Z7) The Culling | 5443 | exact | 231/231 | 0 / 0 / 0 | 0, 5, 0 | 60/60 | 1/1 | 0 | 2/2 | [4] | equal | acquire_range, right_click |
| Z8 | Z8) The Dark Templar | 5544 | exact | 194/194 | 4 / 0 / 0 | 0, 3, 0 | 51/51 | 5/5 | 0 | 6/6 | [4] | equal | acquire_range, requirements, right_click |
| Z9 | Z9) The Invasion of Aiur | 5643 | exact | 371/371 | 13 / 0 / 0 | 0, 1, 0 | 81/81 | 5/5 | 0 | 10/10 | [4] | equal | acquire_range, requirements, right_click |
| Z10 | Untitled Scenario | 5743 | differs (2 missing, 2 extra) | 335/337 | 7 / 0 / 0 | 0, 1, 0 | 92/92 | 2/2 | 0 | 8/8 | [4] | equal | acquire_range, requirements, right_click |
| P1 | P1) First Strike | 5004 | exact | 117/117 | 0 / 0 / 0 | 0, 1, 0 | 26/26 | 2/2 | 0 | 4/4 | [4] | equal | acquire_range, right_click |
| P2 | P2) Into the Flames | 5104 | exact | 141/141 | 3 / 0 / 0 | 0, 1, 0 | 28/28 | 1/1 | 0 | 3/3 | [4] | equal | acquire_range, right_click, unknown |
| P3 | P3) Higher Ground | 5204 | exact | 189/189 | 3 / 0 / 0 | 0, 2, 0 | 48/48 | 3/3 | 0 | 6/6 | [4] | equal | acquire_range, right_click |
| P4 | P4) The Hunt for Tassadar | 5305 | exact | 196/196 | 3 / 0 / 0 | 0, 1, 0 | 26/26 | 0/0 | 0 | 0/0 | [4] | equal | acquire_range, right_click |
| P5 | P5) Choosing Sides | 5405 | exact | 238/239 | 4 / 0 / 1 | 0, 3, 0 | 57/57 | 3/3 | 0 | 5/5 | [4] | equal | acquire_range, map_string, right_click, shields, unknown |
| P6 | Protoss 06 | 5505 | exact | 230/231 | 0 / 0 / 1 | 0, 0, 0 | 0/0 | 0/0 | 0 | 0/0 | [4] | equal | acquire_range, right_click, unknown |
| P7 | P7) Homeland | 5605 | exact | 354/357 | 10 / 0 / 3 | 0, 0, 10 | 64/64 | 1/1 | 0 | 6/6 | [4] | equal | acquire_range, requirements, right_click |
| P8 | P8) The Trial of Tassadar | 5705 | exact | 255/256 | 7 / 0 / 1 | 0, 0, 0 | 50/50 | 2/2 | 0 | 5/5 | [4] | equal | acquire_range, requirements, right_click |
| P9 | P9) Shadow Hunters | 5805 | exact | 341/343 | 0 / 0 / 2 | 0, 7, 0 | 63/63 | 4/4 | 0 | 10/10 | [4] | equal | acquire_range, right_click |
| P10 | Untitled Scenario | 5905 | exact | 339/341 | 3 / 0 / 2 | 0, 8, 0 | 142/142 | 12/12 | 0 | 6/6 | [4] | equal | acquire_range, right_click |

"Placements matched" counts rows joined on type, owner and spot (moved and
displaced rows included, owner-changed ones not). The CHK titles are the
map files' own and differ from the menu names in places (Z5 is "The
Amerigo" on the menu). The differences, unexplained and reported as such: in T10 two computer
Wraiths and in Z10 two Scouts stand 30–56 px from their CHK spots on the
first tick, with no other placement near; in Z5 a Civilian, a Ghost, three Marines and
four critters do the same (up to 75 px on the larger axis), two more Z5 units are displaced from
stacks, and the CHK's `Powerup_Data_Disk` (type 218, player 5) has no unit
row on the first tick. Start banks match in every mission, including the 5,000–10,000
computer grants the GBA trace cannot show (they land in the N64 bank arrays
on the first tick).

The player upgrade and tech arrays at launch are compared with the CHK's
effective PUPx and PTEx settings (each player's own row or the default row)
in the same table.

## Field map

All offsets are added to `tools/parity/sc64-usa-v1.json` with one evidence
line each; decoding is in `tools/parity_decode.py` (`Capture.extra_bytes`,
`fog`, `unit_images`, `orders`) and `tools/sc64_parity_fieldmap.py`.
"Proven" here always means a closed loop: a change and its effect, seen tick
by tick in a capture. Launch-time agreement with the CHK across 30 missions
is a cross-check and leaves a field `candidate`.

### Orders (`+0x4D`, `+0x4E`)

The T1 field capture (`f-t1`) box-selects the five Marines and issues, with
marks, Move (C-Right, A), Stop (C-Down), Hold (C-Left), a second Move and an
attack-move (B, A). The order byte reads, on every Marine, in both captures:

| Command | Order byte |
| --- | --- |
| none (launch) | 3 |
| Move | 6 from the next tick; 2 for one tick on arrival, then 3 |
| Stop | 2 within the tick, then 3 |
| Hold | 107 |
| attack-move | 14 |

These are the PC `orders.dat` numbers (PlayerGuard 3, Guard 2, Move 6,
AttackMove 14, HoldPosition 107), so the layout names them and promotes
`unit.order` to proven. Values seen only on computer units are kept as
candidates: 156 (ComputerAI, every computer unit), 75 (ResearchTech) and 76
(Upgrade), each matching the PC number for what the unit was doing. `+0x4E`
behaves as the PC order state (0 when issued, 1 from the next tick) and is a
candidate.

### Fog and visibility

The fog tiles are a heap block of width × height × 4 bytes (T1: 64 × 64 at
`0x80274520` on the launch tape; the address moves with the launch path, so
the decoder finds the block by its size word and a consistency test). Each
tile is `[u16 tile flags][u8 explored mask][u8 visible mask]`, big-endian; a
clear bit p means player p has explored or sees the tile (the BWAPI sense with
inverted bits). Evidence: tile by tile, the human's visible set grows along
the path of the five moving Marines; a tile is never visible but unexplored
(0 of 4,096); the computer's bits clear only around its Zerglings.

**Update cadence.** Tiles join the visible set on the tick a unit's sight
reaches them, but leave it only at refreshes about 100 ticks apart (frames
504, 807 and 1,009 of `f-t1`): after the Marines walk away, the tiles they
left stay visible for up to 100 ticks. This is the Brood War refresh the
design anticipated.

### Sprites, images and iscript frame state

A unit's `+0x0C` points into the sprite pool (`0x80207C70`, 2,200 × 36 bytes);
the sprite's position (`+0x14/+0x16`) equals the unit's on every tick, and
`+0x18` points at its primary image in the image pool (`0x801CA960`, 4,000 ×
60 bytes; both pools hand out records from the top). Image fields, all checked
against the ROM's `images.dat` and `iscript.bin`:

| Offset | Field | Evidence |
| --- | --- | --- |
| `+0x08` u16 | image id | the unit's images.dat row (Marine 239, SCV 247, Zergling 54; shadows 240, 248, 55) |
| `+0x0B` u8 | draw function | 10 on every shadow image |
| `+0x0C` u16 | 4 × frame set | 4 × the argument of the last `playfram` executed |
| `+0x0E` u16 | frame index << 6, direction | the frame follows the Marine's heading (direction 12 moving south-east, 8 east) |
| `+0x14` u16 | iscript header | equals the iscript.bin header offset of the image's script id, for every image read |
| `+0x16` u16 | iscript offset | steps through the ROM's Walking loop, 7 bytes a tick (`playfram 85, 102 … 204, 68`) |
| `+0x1A` u8 | animation | 11 (Walking) while the Marine walks |
| `+0x1B` u8 | wait | counts down one a tick from the ROM's `wait` argument while idle |
| `+0x38` u32 | owning sprite | points back at the sprite |

Sprite `+0x1C` is the primary image until the unit is selected and then a
selection-circle image from a separate small pool, so the decoder follows
image links only from `+0x18` and only through recorded memory.

### Bullets

The bullet pool is the heap block of 100 × 112 bytes at `0x8021EA40` (PC
`CBullet` size), recorded per tick in the T3, Z2 and P1 opening captures.
Live rows (nonzero sprite pointer) carry: flingy `+0x24` u16, position
`+0x28/+0x2A`, owner `+0x4C`, target unit `+0x5C`, weapon `+0x60`, time
remaining `+0x61`, source unit `+0x64`. Closed loops in the T3 opening:

- every row's weapon is its source unit's `units.dat` weapon (Marine 0,
  Vulture 4, SCV harvest 14), and its flingy is that weapon's
  `weapons.dat` flingy;
- a Gauss Rifle row appears on its target, which loses exactly 6 hit points
  (the weapon's damage) on the same tick; the row stays live 15 ticks (the
  impact animation);
- a Fragmentation Grenade row starts at the Vulture, moves about 7 px a tick
  toward its target with `+0x61` counting down from 255, and the target loses
  20 hit points 9 ticks after the row appeared;
- SCV mining sparks (weapon 14, damage 0) live 10 ticks, one per mining
  cycle.

The same events prove `unit.hp` (`+0x08`, 8.8 fixed point).

### Resources

`unit.resource_amount` (`+0xCC` u16) equals the CHK amount of every mineral
field and geyser at every launch, and in the T3 opening the mined fields fell
by exactly 8 at each of 177 trips while `minerals_gathered` rose 173 times
(four loads still in workers' hands at the end).

### Research, upgrades and per-player arrays

Per-player arrays, one byte per entry, Brood War sizes (61 upgrades, 44
techs), player-major:

| Address | Contents | Status |
| --- | --- | --- |
| `0x800B9FB4` | tech available `[12][44]` | candidate (equal to the CHK at every launch) |
| `0x800BA1C4` | tech researched `[12][44]` | proven |
| `0x800BA3D4` | tech in progress, bits `[12][48]` | proven |
| `0x800BA41C` | upgrade maximum `[12][61]` | candidate (equal to the CHK at every launch) |
| `0x800BA6F8` | upgrade level `[12][61]` | proven |
| `0x800BA9D4` | upgrade in progress, bits `[12][64]` | proven |

The T3 computer (player 4) starts Burrowing at its Hatchery on unpaused tick
96 of the opening: the bank falls 100/100, the Hatchery's order becomes 75,
its row reads tech `+0xC4` = 11 and timer `+0xC2` = 1,200 (the ROM's
`techdata.dat` research time), and player 4's in-progress bit 11 is set. The
timer counts down one a tick; when it runs out the bit clears, the researched
entry turns 1 and the timer reads 0xFFFF. Measured on the tick counter:
1,201 ticks, the ROM's 1,200 plus the one-tick uncertainty of the start
read. In the P1 opening the computer's Lair upgrades Pneumatized Carapace
(order 76, `+0xC5` = 26, the same timer from about 2,000); its in-progress
bit clears and the upgrade level rises to 1 on the same tick.

One change is unexplained: on the tick the P1 Lair starts its upgrade, every
player's level of upgrades 20, 32 and 41 (Ocular Implants, Metasynaptic Node,
Apial Sensors) turns from 0 to 1.

### `.dat` table anchor (S12)

The cartridge runs on resident copies of its `.dat` tables: big-endian column
arrays in file order, each column starting on a 4-byte boundary
(`units.dat` from `0x800C7E64`, `weapons.dat` from `0x800CD3BC`, `flingy.dat`
from `0x800A4DA0`, `upgrades.dat` from `0x800CCCAC`, `techdata.dat` from
`0x800C6D40`). They are not stored raw in the ROM image, so the boot builds or
unpacks them. `tools/sc64_parity_fieldmap.py tables DUMP` compares every
column with the BOLT tables the ROM carries (and that the WP1 differ reads).
In every launch dump, apart from the per-mission settings described below:

- 104 or 105 of the 107 columns equal the BOLT tables byte for byte (101 to
  105 counting the per-mission changes);
- `units.acquire_range` differs in 44 rows: BOLT has 0 for most units, RAM has
  PC-like values (Marine 4, Ghost 7, Vulture 5 …);
- `units.right_click` differs for six buildings (Hatchery, Lair, Hive and three
  Protoss production buildings): BOLT 0, RAM 7;
- `techdata.unknown` differs for the Brood War rows in some launches (an
  unused column).

Beyond those three, the tables differ per mission exactly where the mission's
CHK sets its own unit settings (UNIx rows whose "use default" byte is 0):
Zerg missions Z1–Z6 give special buildings 150 and 151 a 15-frame build
time, no shields and their own name strings, and P5 does the same for unit
29; every changed cell equals the CHK value. So the cartridge applies each
mission's unit settings to its runtime tables. `units.requirements` also
changes in T7, Z4, Z6, Z8–Z10, P7 and P8, for no reason found yet.

So the cartridge's runtime truth is the RAM table: the BOLT `units.dat` is
not what runs for target acquisition range and right-click action. The WP1
differ compares neither column today; a check that does must read the
runtime values.

## Golden-backed checks

`./dev parity` judges two new scenarios (`tools/sc64_parity_metrics.py`
`SCENARIOS`, measured in `tools/sc64_parity_fieldmap.py`).

**`t1-fields`** (golden from `f-t1-1` and `f-t1-2`, identical chains): the
t1-move checks on the capture's first Move, plus order, fog and animation
checks. The GBA side is the existing `move` runner with the same parameters.

| Check | N64 | Band | GBA | Verdict |
| --- | ---: | --- | ---: | --- |
| move.cruise_speed (px per frame) | 3.88 | 3.22–4.27 | 3.61 | PASS |
| move.arrival_first (frames) | 78 | 76–80 | 61 | KNOWN |
| move.arrival_last | 79 | 77–81 | 100 | KNOWN |
| move.group_spread (px) | 29.4 | 25.4–33.4 | 21.3 | KNOWN |
| move.start_latency | 6 | report | 1 | REPORT |
| order.move_latency | 4 | report | 1 | REPORT |
| order.move_end_first | 85 | 83–87 | 63 | KNOWN |
| order.move_end_last | 91 | 89–93 | 102 | KNOWN |
| fog.first_reveal | 10 | 8–12 | 1 | KNOWN |
| fog.first_clear | 107 | 105–109 | 29 | KNOWN |
| fog.visible_at_rest (tiles) | 780 | 757–803 | 648 | KNOWN |
| fog.explored_at_rest (tiles) | 792 | 768–816 | 665 | KNOWN |
| anim.walk_cycle (frames) | 9 | 8–10 | 6 | KNOWN |

The N64 move values agree with the S4 t1-move capture within a frame or two
(arrival 80–81 there). The golden refuses frames without the fog and image
regions or skipped ticks inside the window, and the GBA run is sized to
reach the rest frame.

**`t3-opening`** (golden from two T3 opening captures): research, bullet and
timeline numbers, report only. T3 is busy enough that 40 of 4,436 ticks fall
one frontend step apart and get no follow-up record, and about one follow-up
read in eleven sees a tick half done (the Hatchery's timer reads the same
value twice, then drops by two). Times there are taken from the tick counter,
not from record counts, and the scenario is the only one allowed tick skips
(`tick_skips_ok`); the golden writer refuses such a scenario if any of its
observations gates.

| Check | N64 | GBA |
| --- | ---: | --- |
| research.duration (Burrowing, computer Hatchery) | 1,201 ticks | not measured: the GBA computer does not research |
| bullet.grenade_hit (Vulture grenade, spawn to damage) | 9 ticks | not measured: no projectile record in the GBA trace |
| bullet.gauss_life (Marine Gauss Rifle row) | 15 ticks | not measured |
| timeline.first_hydralisk (computer) | 765 | none within 4,480 frames |
| timeline.first_human_hit | 1,233 | none within 4,480 frames |
| timeline.first_human_delivery | 173 | none: the human's SCVs never mine |

## Opening timelines (WP10, first slice)

T3 (Terran), Z2 (Zerg) and P1 (Protoss) were each captured for 15,000
in-mission steps with no input after the text boxes (4,436, ~4,400 and 4,478
unpaused ticks), and compared with the GBA census runner on the same mission
for as many source frames (`sc64_parity_fieldmap.py timeline`). Times are
unpaused ticks from launch.

| Mission (computer) | Event | N64 | GBA |
| --- | --- | ---: | --- |
| T3 (Zerg, player 4) | start grant 5,000 / 5,000 | tick 1 | tick 1 |
| | first spending (Burrowing, 100/100) | 96 | never |
| | new Larva / Egg | 325 / 327 | not traced as units |
| | second Extractor | 424 | never |
| | first new Hydralisk | 765 | never |
| | first new Zergling | 3,287 | never |
| | first damage to a human unit | 1,233 (a Vulture) | never |
| | human bank starts rising (SCVs mining) | 169 | never (stays 100) |
| Z2 (Protoss, player 1) | start grant 5,000 / 5,000 | tick 1 | not in the trace (computer wallet) |
| | first new Probe / Dragoon / Gateway / Zealot | 127 / 271 / 433 / 3,388 | no raw types in the zerg-z10 trace |
| | first damage to a human unit | 2,347 (a Hydralisk) | never |
| | human bank starts rising (Drones mining) | 163 | never (stays 300) |
| P1 (Zerg, player 0) | start grant 4,000 / 4,000 | tick 1 | not in the trace |
| | first spending (Pneumatized Carapace, 150/150) | 139 | never |
| | new Egg / Larva / Hydralisk | 253 / 675 / 691 | no raw types in the protoss-p10 trace |
| | Pneumatized Carapace done; Burrowing done | 2,140; 3,340 | never |
| | human bank | starts at 400 / 100, no workers | 400 / 100 |

GBA runs: `terran-fast-trace`, `zerg-z10-fast-trace` and
`protoss-p10-fast-trace --scenario census --mission ID` for the N64
horizon (×5/4 steps). The Zerg and Protoss traces write raw type −1 for
unit rows, so per-type firsts are compared only in T3.

## GBA defects found

Every difference is reported as a GBA defect; none is accepted as fine. The
gating ones are baseline lines (KNOWN) so that a further drift fails.

1. **Fog refresh.** N64 drops a tile from the visible set only at refreshes
   about 100 ticks apart; the GBA drops it on the next update
   (`fog.first_clear` 107 vs 29 frames).
2. **Fog reveal timing.** The GBA reveals new tiles one frame after the
   command, the N64 ten frames after it (`fog.first_reveal` 1 vs 10; the
   Marines start moving 6 frames after the command on the N64, 1 on the GBA).
3. **Sight area.** At rest after the Move the human sees 648 tiles on the
   GBA against 780 on the N64 (17% fewer) and has explored 665 against 792.
4. **Walk animation.** A Marine's walk cycle takes 6 frames on the GBA, 9 on
   the N64 (the ROM's Walking loop is nine one-tick frames).
5. **Group move.** First and last Marine arrive at 61 and 100 frames on the
   GBA against 78 and 79 on the N64, and the group ends tighter (spread 21.3
   vs 29.4 px); the order returns to idle 63–102 frames after the command
   against 85–91. N64 also shows a one-tick Guard order (2) between Move and
   PlayerGuard that the GBA has no counterpart for.
6. **Campaign workers stay idle.** On the N64 the human's starting workers in
   T3 and Z2 begin mining by themselves (bank rises from tick 169 and 163);
   on the GBA they never do in 4,480 frames.
7. **Campaign computers are inert in the opening.** T3: the N64 Zerg spends
   its grant at tick 96 (Burrowing), builds an Extractor and Hydralisks and
   first hits a human unit at 1,233; the GBA computer only mines. Z2 and P1
   show the same: production and research on the N64, none on the GBA.
8. **No computer research.** The GBA computer never researches or upgrades;
   the N64 computer researched Burrowing (T3, P1) and Pneumatized Carapace
   (P1) within the first 3,400 ticks.
9. **Trace gaps.** The GBA trace has no projectile records (so bullet travel
   cannot be compared), no raw unit types on the zerg-z10 and protoss-p10
   profiles, and no computer wallet banks.
10. **Runtime tables.** The ROM's BOLT `units.dat` is not what the cartridge
    runs for `acquire_range` (44 rows) and `right_click` (6 buildings), and
    each mission's CHK unit settings change the runtime tables. Any GBA value
    taken from the BOLT columns, or from tables without the mission's unit
    settings, is a defect where they differ; neither column is in the WP1
    differ yet.

The gating gaps (1–5) are baseline lines in `tools/parity/baseline.json`
(KNOWN), so a change that moves the GBA further from the N64 fails
`./dev parity`. The rest are report-only (`t3-opening`) or recorded here.

## Not proven

- `player.upgrade_max` and `player.tech_available` agree with the CHK at every
  launch but never changed in a capture.
- Order names other than 2, 3, 6, 14 and 107 are candidates; only commands a
  tape issues can prove them.
- The cause of the three energy upgrades turning to level 1 in P1.
- Why 13 isolated units in T10, Z10 and Z5 start up to 75 px (larger axis) from their CHK
  spots, and why `units.requirements` changes in eight launches.
- Bullet `+0x61` was seen counting down only for the grenade; it stays 255 on
  appear-at-target rows.
- No research or upgrade was started by input: the GBA injector has no
  research verb and the N64 command panel was not driven, so the research
  check is N64-only (the GBA computer does not research).
- Fog was measured for the human only, in one 64 × 64 map; the computer's
  masks were read but not checked.
- Busy-mission records: the S3 follow-step rule is not enough in T3-sized
  missions (above). A settled read (one step later, or the step before the
  next change) would remove the torn records; the frontend does not offer one
  yet.

## Reproduce

Private home: the main checkout's `.local/parity-harness/fieldmap`
(`mktape.py`, `cap.sh`, the two binaries with their pinned sources, tapes and
the runs named here). Goldens: `$SC64_PARITY_HOME/golden/t1-fields.json` and
`t3-opening.json`; launch and timeline reports in
`$SC64_PARITY_HOME/fieldmap/`.

```sh
python3 tools/sc64_parity_fieldmap.py census-all RUNS_DIR
python3 tools/sc64_parity_fieldmap.py tables RUN/start.ram
python3 tools/sc64_parity_fieldmap.py timeline RUN --mission T3
python3 tools/parity_decode.py golden --scenario t1-fields RUN_A RUN_B
./dev parity
```
