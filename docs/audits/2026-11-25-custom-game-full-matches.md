# Custom Game full matches on the three race cartridges

Whole Custom Game matches, from the lobby to RESULT, on the Terran, Zerg Z10
and Protoss P10 cartridges. Every match is played with ordinary button
input, won by the human, saved through the pause menu during a battle,
continued from that save in a fresh process, and its victory record reloaded
in another. Emulator evidence only: nothing here was measured on a GBA.

The cartridges were built from this branch with `codex/source-release` at
`5252cfc3` merged in, and with the private skirmish pack regenerated for the
56-patch Blood Bath of [#887](https://github.com/ggu/starcraft-gba/pull/887)
(see [Reproduce](#reproduce)). Later base changes up to `81e76943`
([#884](https://github.com/ggu/starcraft-gba/pull/884) audio interrupt
cost, [#891](https://github.com/ggu/starcraft-gba/pull/891) weapon-art
extraction, [#889](https://github.com/ggu/starcraft-gba/pull/889) parity
tooling) touch only audio, linker and tooling sources, not the game or
skirmish code, and were not replayed. The same work on the earlier
`7316734e` cartridges is kept [below](#earlier-build-7316734e).

## Results

| Cartridge | Map | Opponents | Result | Updates | Frame | Pause save (update, units on screen) | Fresh-process continuation | Victory reload | Emulated updates/s |
| --- | --- | --- | --- | ---: | ---: | --- | --- | --- | ---: |
| Terran | Outpost | Zerg, Protoss, Terran | won | 34,721 | 33,770 | 15,167 (21 own, 26 hostile) | 136 checks match, same result and victory save | opens RESULT, SRAM unchanged | 22.56 |
| Terran | Outpost | Terran, Zerg | won | 31,781 | 31,210 | 16,350 (19 own, 16 hostile) | 108 checks match, same result and victory save | opens RESULT, SRAM unchanged | 21.86 |
| Terran | Blood Bath Melee | Zerg, Protoss, Terran | won | 37,461 | 34,919 | 18,797 (40 own, 7 hostile) | 222 checks match, same result and victory save | opens RESULT, SRAM unchanged | 17.18 |
| Terran | Blood Bath Melee | Protoss, Zerg | won | 32,115 | 29,816 | 12,248 (37 own, 21 hostile) | 213 checks match, same result and victory save | opens RESULT, SRAM unchanged | 16.61 |
| Zerg Z10 | Outpost | Terran, Zerg, Protoss | won | 27,669 | 26,934 | 12,111 (63 own, 8 hostile) | 116 checks match, same result and victory save | opens RESULT, SRAM unchanged | 18.73 |
| Zerg Z10 | Outpost | Protoss, Zerg | won | 26,371 | 25,772 | 12,115 (30 own, 7 hostile) | 130 checks match, same result and victory save | opens RESULT, SRAM unchanged | 20.04 |
| Zerg Z10 | Blood Bath Melee | Terran, Zerg | won | 35,904 | 34,880 | 13,433 (28 own, 17 hostile) | 254 checks match, same result and victory save | opens RESULT, SRAM unchanged | 18.48 |
| Protoss P10 | Outpost | Protoss, Terran, Zerg | won | 37,725 | 36,567 | 13,982 (28 own, 21 hostile) | 169 checks match, same result and victory save | opens RESULT, SRAM unchanged | 20.08 |
| Protoss P10 | Outpost | Terran, Zerg | won | 35,401 | 34,460 | 19,436 (24 own, 24 hostile) | 103 checks match, same result and victory save | opens RESULT, SRAM unchanged | 19.57 |
| Protoss P10 | Blood Bath Melee | Terran, Zerg, Protoss | won | 32,964 | 30,966 | 14,624 (22 own, 15 hostile) | 192 checks match, same result and victory save | opens RESULT, SRAM unchanged | 15.85 |
| Protoss P10 | Blood Bath Melee | Terran, Zerg | won | 38,114 | 35,182 | 15,008 (55 own, 34 hostile) | 238 checks match, same result and victory save | opens RESULT, SRAM unchanged | 14.40 |

"Updates" counts main-loop updates from power-on, lobby and pause included;
"frame" is the game's own counter at RESULT. "Pause save" gives the update of
the save and the units inside the 240 × 108 field viewport at that moment,
counted by the player and again by the replay from guest memory.
Emulated updates/s is the guest cycle clock over the whole uninterrupted
replay, with menus and presentation waits; it is not hardware frame rate.

Opponents are listed in seat order. Outpost is Free for All (the computers
also fight each other); Blood Bath is Melee (the computers are allied).

### What remains open

| Cartridge | Map | Opponents | Result | Cause or smallest reproduction |
| --- | --- | --- | --- | --- |
| Zerg Z10 | Blood Bath Melee | three | not won | No player win found. On the 56-patch layout 960 HOST plans across the 24 start orders lost (opening size, Zergling or Hydralisk line, one to three Hatcheries, Sunken Colonies, attack timing), after about 936 on the old layout. A typical loss: `SC64_CFM_START_ORDER=114 build/test-sc64-custom-full-match-zerg 1 z tzp trace.json` loses at update 15,938 to the allied computers' waves. The computers build, pay and attack normally in these HOST matches and nothing observed points at the engine; the losses were not replayed on ARM. This is a limit of the scripted player, not a recorded engine fault. |

Every other cell of the brief (both maps, two and three computers, on all
three cartridges) has one winning lineup in the results table. Other
lineups of the same size are not all won: of 13 Terran Outpost lineups
played with the default plan on HOST, 8 won; on Blood Bath, 61 of 96 Terran
plans against two computers and 2 of 96 against three won.

Not covered by any row: Random or closed-first seats, a human Random race,
Blood Bath Free for All, a second pause save in one match, and hardware.

## How a match is produced and checked

1. **HOST author.** `tests/sc64_custom_full_match.c` is built once per
   cartridge from that cartridge's product vector
   (`mk/custom-full-match/Makefile`, unsanitized `-O2`). It reads game state
   to decide what to do, as the existing skirmish controllers do, so it is a
   test policy and not a visibility-only strategy. Every change to the game
   goes through `game_update()` as held keys: the lobby rows, cursor travel,
   B and the command list for structures and attack-move, A on a selected
   producer with R to cycle its product, R for the army, SELECT to drop a
   selection, START and B to save. It writes the held-key transitions and a
   check record every 250 updates and after every accepted command.
2. **ARM replay.** `tools/sc64_custom_full_match.py replay` drives the
   unchanged ROM in the port-free mGBA backend through `PhysicalGame`:
   physical buttons only, guest memory read and never written. Three fresh
   processes:
   - *Uninterrupted.* Blank SRAM, HOME, the whole tape. Each check compares
     the frame, the bank, live units and buildings per owner, and an FNV-1a
     hash over every live unit's and building's slot, owner, type, position
     and hit points. The SRAM is exported at the pause save and at RESULT.
   - *Continuation.* The pause save is loaded with L on HOME. The restored
     world's fingerprint (the `sc64_skirmish_sram_continuation.py` one: bank,
     mineral fields, and the persisted fields of every unit and building with
     its queue) must equal the saved one. The rest of the tape then has to
     pass every remaining check and end in the same RESULT state.
   - *Victory record.* The SRAM exported at RESULT is loaded with L on HOME.
     It must open RESULT with the same state, and the SRAM must be unchanged
     by the load.
3. The tape pauses when at least 5 hostile units, 6 of the player's and 16
   units in all share the viewport. The replay counts them again from the
   guest cursor and unit table at the save (units inside a transport left
   out) and requires the same counts. Before START the player drops its
   selection and leaves the remembered command row on MOVE, which is where a
   fresh load leaves it, so the same keys follow in both continuations.
4. The trace must carry a check at every 250th update; a missing or
   incomplete check record, a resume record that differs, a different
   victory save from the continuation, or a changed ROM, ELF, trace or
   backend during the run all fail the case.

The HOST build must repeat the cartridge ELF's target-specific flags
(collision buckets, contact axis, shared sight rows, the Melee lobby row).
Without `SC64_NATIVE_MELEE_STARTS` the HOST lobby had one row fewer and the
first replay stopped on HOME; with the full vector a 14,565-update Terran
Outpost defeat matched the ROM at all 379 checks before any of the wins below
were attempted. The replay refuses a trace at the first check that differs.

### Reproduce

Since [#887](https://github.com/ggu/starcraft-gba/pull/887) the Blood Bath
tapes need the regenerated private skirmish pack in both places the
cartridges read it. Terran and Protoss include the Battlecruiser folder's
cover-encoded copy first; a stale copy there keeps the old 32-patch table
with no build error. `tools/sc64_skirmish_pack.py` needs the pinned
`references/sc64-maps` checkout, and the cover copy needs CV5-cover
Backwater assets (here generated into a scratch folder):

```sh
python3 tools/sc64_skirmish_pack.py
python3 tools/sc64_assets.py --rom .local/sc64/rom/StarCraft64-USA.z64 \
  --profile backwater --out .local/cover-assets --cover
python3 tools/sc64_skirmish_pack.py --rom .local/sc64/rom/StarCraft64-USA.z64 \
  --backwater-generated .local/cover-assets \
  --out .local/sc64/bc/generated/sc64_skirmish_generated.h --cover
```

The same two pack commands with the pre-#887 tool reproduced the earlier
plain and cover headers byte for byte. Each ELF's `sc64_sk_blood_bath_minerals`
is 448 bytes (56 rows) with the current pack and 192 bytes with the old one.

With the prepared inputs and the three ROMs built:

```sh
make sc64-custom-full-match-authors
make check-sc64-custom-full-matches SC64_HEADLESS_REPLAY=/path/to/sc64-headless-replay
```

That authors and replays every lobby in `QUALIFIED`
(`tools/sc64_custom_full_match.py`) into a new
`.local/sc64/custom-full-matches`. One lobby:

```sh
python3 tools/sc64_custom_full_match.py run \
  --controller build/test-sc64-custom-full-match-terran \
  --rom build/StarCraft-Terran-Transport1-GBA.gba --elf build/starcraft-terran.elf \
  --runner /path/to/sc64-headless-replay --output .local/sc64/terran-outpost-zpt \
  --map 0 --human t --cpus zpt --plan PRODUCERS=3
```

`--plan` sets the player's opening (`SC64_CFM_*` in the author). The
settings used for each row:

| Cartridge | Map | CPUs | `--plan` | Start order | Key transitions | Checks | Player at RESULT | Trace SHA-256 (prefix) | Victory SRAM SHA-256 (prefix) |
| --- | --- | --- | --- | ---: | ---: | ---: | --- | --- | --- |
| Terran | Outpost | `zpt` | `PRODUCERS=3` | - | 5,595 | 270 | 27 units, 13 buildings | `7101c88e0fdf03ca` | `9cfb0351723b0acb` |
| Terran | Outpost | `tz` | `(default)` | - | 7,535 | 241 | 58 units, 17 buildings | `20def6c4f77adcbe` | `cd72d3a4ccd6d5a8` |
| Terran | Blood Bath | `zpt` | `LOBBY_WAIT=8` | 57 | 6,555 | 448 | 102 units, 25 buildings | `d20425cafbe90e37` | `5b969ecd6a584656` |
| Terran | Blood Bath | `pz` | `LOBBY_WAIT=8,FIRST_ATTACK=7000` | 11 | 6,114 | 347 | 102 units, 27 buildings | `3e095edaa89c59ea` | `654e80c91cc444ff` |
| Zerg Z10 | Outpost | `tzp` | `WORKERS=10,LINE=38` | - | 2,576 | 233 | 27 units, 10 buildings | `3b9a87d8c6e1a733` | `0ee86714b12df560` |
| Zerg Z10 | Outpost | `pz` | `WORKERS=12,LINE=38` | - | 2,503 | 242 | 54 units, 10 buildings | `0a293290fc03e2b7` | `f29cf079928032da` |
| Zerg Z10 | Blood Bath | `tz` | `LINE=38,PRODUCERS=1,LOBBY_WAIT=22` | 7 | 6,341 | 386 | 130 units, 10 buildings | `eddd30ecbc76dd06` | `32d907af8a59e2ee` |
| Protoss P10 | Outpost | `ptz` | `LINE=65,DEFENSE=162,DEFENSES=4` | - | 5,872 | 270 | 18 units, 14 buildings | `8d24ccaa4081f5e1` | `bf39c7e29226f5bc` |
| Protoss P10 | Outpost | `tz` | `WORKERS=14,PRODUCERS=3,LINE=66,DEFENSE=162,DEFENSES=8` | - | 4,451 | 228 | 23 units, 22 buildings | `3bc99da83caea218` | `1576087b582560e3` |
| Protoss P10 | Blood Bath | `tzp` | `LINE=66,TECH_FRAME=8000,LOBBY_WAIT=20` | 141 | 5,445 | 351 | 80 units, 27 buildings | `bcf308d038e50f57` | `3103b4c8e582308e` |
| Protoss P10 | Blood Bath | `tz` | `LINE=65,TECH_FRAME=8000` | 24 | 6,768 | 403 | 73 units, 28 buildings | `b5363b92dac7886a` | `2b06fcc2bccfdf5a` |

Traces and saves stay in ignored local folders. A trace is reproducible from
this source and the prepared inputs: the author is deterministic.

| Input | SHA-256 |
| --- | --- |
| Terran ROM | `1d83715420b3b424e84069ed5af78e32a21b550d99e4e5d46bed14f79907e70a` |
| Zerg Z10 ROM | `26a2da208c0aba3dbf3e0c45e6c0d934f9e512c8bfe3cb891b797d5a71f4996d` |
| Protoss P10 ROM | `95eac6f0fecfb7fedf3f23b01d94fa1ae0d3e8721a4184a7af35af9026ea6a63` |
| Terran ELF | `c94d99cc31e01009dec4ffc620544073cbfddb20fd7dd459623a6fac9ee164c1` |
| Zerg Z10 ELF | `b9bedf45b07893877f85ba492ce74146c506ec1ea011e689e398251fc8087b2b` |
| Protoss P10 ELF | `3f01eb50c954049f76e1177baea0c113f105cc4f82b9ee8aad23f35ba8868b88` |
| Port-free mGBA backend (`--physical` protocol) | `10189afccbaa815dc9eb98eaf08bf871622210ab0b85e752d72e8a45a92aad15` |

`make` regenerated the Z10 fact-table header on each run here and relinked
the Z10 ELF. The ROM bytes were checked unchanged after each relink, and the
replay refuses a run if the ROM, ELF, trace or backend changes while it plays.

## What the matches showed

**No engine or save defect was found.** In every replayed match the ROM
matched the HOST at every check, each pause save was accepted and loaded, the
loaded world equalled the saved one, both continuations reached the same
RESULT state and wrote the same victory SRAM, and the victory record
reloaded without changing the SRAM. No computer opponent stalled: each either
attacked or was destroyed.

**Outpost is a fixed-budget map.** Each base has four mineral fields of 1,500.
In the matches inspected all four bases were mined out by about update
12,000 (one Terran match had 3,916 / 3,496 / 3,272 / 3,928 left at update
4,118 and none at 12,036), so each owner plays the whole match on about
6,000 minerals. The wins come from spending that on few workers and
structures, holding the base while the computers attack each other and then
the player, and clearing what is left. The first Terran attempt, with 20
SCVs and six Barracks, lost at update 23,874.

**Structures that shoot have to be targeted.** Attack-move into a defended
base fights whatever is nearest. Before the player sent the whole selection
at the nearest Bunker, Sunken Colony or Photon Cannon first (attack, then A
on it), the Terran Outpost match against three computers lost its army at
the last Protoss base and ran to the update limit; with that rule, and with
the army gathering short of a distant target, the same lineup wins.

**A spent map can leave a match open.** With every field mined out, a player
with no army and no minerals facing an opponent in the same state cannot end
the match, and a player loses with an army alive once its last structure
falls (one Blood Bath attempt ended that way with 69 combat units). Both are the
existing end-state rule recorded under K3 in the project TODO; nothing was
changed.

**Blood Bath Melee depends on the draw.** The start locations are drawn from
the time since power-on when Launch is confirmed, so a tape that waits *n*
updates on the Launch row gets a different draw. The author's `LOBBY_WAIT`
setting does that wait and the replay reads the order the ROM resolved. For
Terran against Zerg, Protoss and Terran on the 56-patch layout the 24
possible orders were each tried on HOST with four openings, and two orders
had a winning one (57 and 27). On the current Terran ROM, waits 0–23 reach
16 of the 24 orders, and wait 8 gives order 57. On the old layout the same
search found four winning orders (114, 57, 177, 120), and wait 9 gave 114.

**The unit pool decided long Blood Bath matches on the old layout.** With 32
patches (eight per base) on the Terran cartridge (192 units, 96 buildings),
a player that only defended reached 88 units while the three computers held
39, 30 and 35: the pool was full, nothing changed for 10,000 updates, and the
computers kept 7,169, 3,898 and 4,097 minerals in the bank. When they
attacked after update 40,068 they replaced their losses from those banks and
won at update 51,501. The Terran wins instead attack one opponent before its
first offensive (no offensive before update 10,631 for Protoss, 12,331 for
Terran and 17,225 for Zerg) and keep pressing it, which also frees its share
of the pool. The 56-patch layout keeps the same eight patches at each start
and adds 24 elsewhere on the map; Protoss against three computers became
winnable there (17 of 192 HOST plans), the other cells kept wins, and Zerg
against three stayed unwon.

**The Protoss P10 command list refuses WEAPONS and ARMOR in a custom game.**
The cartridge compiles the producer research jobs, and `start_research()`
refuses the generic rows while that stage is active for the human; the
player does not use the research jobs. It backs off for 600 updates after a
refusal and plays without upgrades (0/0 in every Protoss run). The Terran
and Zerg players research both to level 2.

## Limits

- The player is a scripted policy with full knowledge of the game state. A
  win shows that the match can be played, saved, continued and finished with
  ordinary input on this build; it does not rate the computer opponents or
  show that a person would win.
- Each row is one deterministic tape. Small changes to the opening change the
  outcome: of 144 Terran Outpost openings tried on HOST against three
  computers, 94 won.
- The opponents, the lobby, the maps and the engine are unchanged. Only the
  player's opening differs between rows, and the Blood Bath rows wait on the
  Launch row for their draw. The HOST author sets its elapsed-time clock to
  match that draw (the HOST has no real clock); the ROM draws it itself and
  the replay checks the resolved start order.
- Timing is emulator guest time and includes UI. Physical GBA pacing, and the
  owner's three-computer match with a mid-battle save on a GBA SP, remain to
  be done on hardware.

## Earlier build (`7316734e`)

Before the base merge the same tools qualified ten lobbies on cartridges
built from `7316734e`, with the old 32-patch Blood Bath (ROMs
`a8a4bae4…` Terran, `b954fde5…` Zerg Z10, `ced2565d…` Protoss P10). The six
Outpost tapes are byte-identical to the current ones and replay on both
builds. The Blood Bath tapes do not apply to the 56-patch layout. Protoss
Blood Bath against three computers was not won on the old layout (576 HOST
plans lost); with all 56 patches 17 of 192 won.

| Cartridge | Map | Opponents | Result | Updates | Frame | Pause save (update, units on screen) | Fresh-process continuation | Victory reload | Emulated updates/s |
| --- | --- | --- | --- | ---: | ---: | --- | --- | --- | ---: |
| Terran | Outpost | Zerg, Protoss, Terran | won | 34,721 | 33,770 | 15,167 (21 own, 26 hostile) | 136 checks match, same result and victory save | opens RESULT, SRAM unchanged | 21.57 |
| Terran | Outpost | Terran, Zerg | won | 31,781 | 31,210 | 16,350 (19 own, 16 hostile) | 108 checks match, same result and victory save | opens RESULT, SRAM unchanged | 20.85 |
| Terran | Blood Bath Melee | Zerg, Protoss, Terran | won | 39,413 | 37,798 | 14,954 (12 own, 13 hostile) | 232 checks match, same result and victory save | opens RESULT, SRAM unchanged | 15.58 |
| Terran | Blood Bath Melee | Protoss, Zerg | won | 39,182 | 37,471 | 24,515 (36 own, 8 hostile) | 94 checks match, same result and victory save | opens RESULT, SRAM unchanged | 16.49 |
| Zerg Z10 | Outpost | Terran, Zerg, Protoss | won | 27,669 | 26,934 | 12,111 (63 own, 8 hostile) | 116 checks match, same result and victory save | opens RESULT, SRAM unchanged | 17.65 |
| Zerg Z10 | Outpost | Protoss, Zerg | won | 26,371 | 25,772 | 12,115 (30 own, 7 hostile) | 130 checks match, same result and victory save | opens RESULT, SRAM unchanged | 19.18 |
| Zerg Z10 | Blood Bath Melee | Terran, Zerg | won | 44,048 | 42,459 | 13,512 (20 own, 13 hostile) | 302 checks match, same result and victory save | opens RESULT, SRAM unchanged | 17.71 |
| Protoss P10 | Outpost | Protoss, Terran, Zerg | won | 37,725 | 36,567 | 13,982 (28 own, 21 hostile) | 169 checks match, same result and victory save | opens RESULT, SRAM unchanged | 19.23 |
| Protoss P10 | Outpost | Terran, Zerg | won | 35,401 | 34,460 | 19,436 (24 own, 24 hostile) | 103 checks match, same result and victory save | opens RESULT, SRAM unchanged | 18.88 |
| Protoss P10 | Blood Bath Melee | Terran, Zerg | won | 35,370 | 34,019 | 14,441 (25 own, 25 hostile) | 146 checks match, same result and victory save | opens RESULT, SRAM unchanged | 17.63 |

| Cartridge | Map | CPUs | `--plan` | Start order | Key transitions | Checks | Player at RESULT | Trace SHA-256 (prefix) | Victory SRAM SHA-256 (prefix) |
| --- | --- | --- | --- | ---: | ---: | ---: | --- | --- | --- |
| Terran | Outpost | `zpt` | `PRODUCERS=3` | - | 5,595 | 270 | 27 units, 13 buildings | `7101c88e0fdf03ca` | `9cfb0351723b0acb` |
| Terran | Outpost | `tz` | `(default)` | - | 7,535 | 241 | 58 units, 17 buildings | `20def6c4f77adcbe` | `cd72d3a4ccd6d5a8` |
| Terran | Blood Bath | `zpt` | `LOBBY_WAIT=9` | 114 | 5,759 | 394 | 56 units, 3 buildings | `f0d96e9286bd3353` | `544f5cb8344bed03` |
| Terran | Blood Bath | `pz` | `LOBBY_WAIT=1` | 28 | 5,247 | 369 | 83 units, 17 buildings | `0b30ba6d96dc2b52` | `a61ae61b33053c3e` |
| Zerg Z10 | Outpost | `tzp` | `WORKERS=10,LINE=38` | - | 2,576 | 233 | 27 units, 10 buildings | `3b9a87d8c6e1a733` | `0ee86714b12df560` |
| Zerg Z10 | Outpost | `pz` | `WORKERS=12,LINE=38` | - | 2,503 | 242 | 54 units, 10 buildings | `0a293290fc03e2b7` | `f29cf079928032da` |
| Zerg Z10 | Blood Bath | `tz` | `LINE=38,LOBBY_WAIT=21` | 49 | 6,281 | 447 | 35 units, 3 buildings | `6669727db40fce28` | `9f94b9c493e68159` |
| Protoss P10 | Outpost | `ptz` | `LINE=65,DEFENSE=162,DEFENSES=4` | - | 5,872 | 270 | 18 units, 14 buildings | `8d24ccaa4081f5e1` | `bf39c7e29226f5bc` |
| Protoss P10 | Outpost | `tz` | `WORKERS=14,PRODUCERS=3,LINE=66,DEFENSE=162,DEFENSES=8` | - | 4,451 | 228 | 23 units, 22 buildings | `3bc99da83caea218` | `1576087b582560e3` |
| Protoss P10 | Blood Bath | `tz` | `LINE=65,LOBBY_WAIT=3` | 56 | 4,826 | 293 | 25 units, 19 buildings | `337cc9b3c8af7554` | `b186e76e9eb71441` |
