# P9 "Shadow Hunters" candidate (2026-10-29)

This audit records the playable P9 build: an explicit `protoss-p9`
candidate that builds on the P8 candidate (#151) and adds P9 as the ninth
campaign entry. Canonical Protoss stays the qualified P7 product; its ROM is
byte-identical to the base before this change, and so is the qualified P8
candidate ROM when built from this branch.

The evidence is one ordinary-input host controller win, an input-only ARM
replay of that controller's key tape, and a two-process SRAM save and
continuation taken while a structure is under construction. All runs are
emulator or host evidence. No physical-GBA or native-AI equivalence claim is
made.

## What is wired

- **Catalog and menu.** `SC64_P9_CAMPAIGN_ENTRY` makes catalog entry 28
  ("SHADOW HUNTERS") playable. It is the menu entry after P8.
- **Runtime.** `starcraft/sc64_p9_runtime_adapter.h` validates the
  authenticated compact program: 12 triggers, 34 actions, 17 conditions.
  - It runs one runner per executing owner: the human (slot 7, force 1), the
    Red Zerg (slot 0), the Baelrog brood (slot 6) and the Leviathan brood
    (slot 5).
  - The runner follows the shared P8 cursor, pending and blocked contract,
    with 16-bit trigger masks.
  - New source behavior P8 did not need:
    - **PreserveTrigger:** a completed trigger re-arms for a later pass.
    - **KillUnitAtLocation:** kills all matching actors, checking the whole
      set first.
    - **MoveUnit:** moves all matching units to the destination centre;
      native placement is not modeled.
    - **Force-wide SetResources:** applies per slot, including the
      rescuable player in force 1.
    - **Building create:** creates the Cerebrate respawn.
- **Victory logic.** Each Cerebrate that dies while Zeratul is inside its
  location sets a switch. Victory needs both switches. A Cerebrate that dies
  without Zeratul present is recreated by the source PreserveTrigger rows,
  which also kill and move units in its area.
- **Rescue.** Zeratul and Fenix are rescued on contact. Native rescue
  transfers ownership, so they take the human source slot. The source
  "current player Zeratul" census needs that.
- **Saves.** `SCP9`, generation 45. The P9 state is 132 bytes in the
  existing 180-byte Protoss mission union, so the 16,292-byte save record
  keeps its size. P7 and P8 saves are rejected by tag; no conversion is
  performed.
- **Profile.** `make protoss-p9` builds `build/StarCraft-Protoss-P9-GBA.gba`
  from the P8 candidate's flags, plus the P9 data/runtime/entry flags and
  separate fact tables for the P9 roster (`build/generated/p9-fact-v2`).
- **Bindings.** P9-gated bindings add raw 46 (Defiler) and raw 136 (Defiler
  Mound), with pinned BWAPI dimensions, stock Defiler armor 1 and names.
  Defiler spells are not implemented.
- **Bridge.** Without Z7, and only with P9 data, the Z8 pool bridge falls
  back to the pinned BWAPI `unitDimensions` table for actors without a
  feature fact, which covers P9's Zerg roster.

## Shared Protoss unit services, cloak and detection

P9 runs the shared Protoss unit services introduced with P8 (see the
[P8 audit](../2026-10-29-p8-campaign/README.md)). Its map gives the human an
Arbiter (slot 7) and Zeratul (rescuable slot 2), and fields no Shield
Battery.

- **Zeratul's permanent cloak.** The shared cloak service treats raw 75 as
  intrinsically cloaked, as P7 does. Enemies cannot target Zeratul unless
  one of their detectors sees him.
- **Arbiter aura.** The human Arbiter cloaks nearby human units.
- **Zerg detection.** P7's detectors are Protoss: Observers at sight range
  and powered Photon Cannons at 7 tiles. P9 adds, under
  `SC64_P9_DATA_ENABLED` only, the Overlord (raw 42, detects at its pinned
  11-tile sight range, no Antennae) and the completed Spore Colony (raw 144,
  7 tiles, the structure rule the Cannon uses). Their pinned BWAPI
  dimensions (`{25,25,24,24}` and `{24,24,23,23}`) are now in the P7
  rectangle helper under the same flag. The fact tables had no rectangle
  for either type, so they could not detect.
- **P9's own state.** P9 saves the service clock in its union payload,
  resets the Arbiter/detection cache at mission start and on load, and
  reads its source-owner map from the canonical descriptor. Load preflight
  checks each unit's cache word against P9's own source mask. Only
  human-built Batteries are valid in P9.
- **Isolation.** In P9 builds, game.c renames P8's service hooks and the P9
  adapter dispatches between P8 and P9. Every P9 extension is behind a P9
  flag, so the P7 and P8 builds are unchanged.

In the qualified run Zeratul is cloaked at every checkpoint. At most one
enemy source slot detected him at once (the Leviathan brood, at the end).
The human Arbiter's aura covered up to 14 human units at once.

## Defects found and fixed

- **Defiler armor (earlier P9 work).** Map-placed Defilers carried stock
  armor 1 while the canonical armor table returned 0, so the typed save
  encoder rejected them and every P9 save was empty.
- **Missing names.** As for P8's Stasis Cell and Hyperion, P9's appended
  Defiler and Defiler Mound had no positional names.
- **Missing detector rectangles.** See above.

## Authored policies

These choices are authored for the GBA build. They are not native
measurements.

| Area | Policy |
| --- | --- |
| Source clock | One engine update is 42 ms of source time, as in P8. |
| Trigger passes | Every 8 updates; waits tick every update. |
| Transmissions and text | Not shown or played. |
| AI scripts | See the next section. |

### AI backend

The native SC64 AI script programs are still undecoded, so
`starcraft/sc64_p9_ai.h` is a bounded authored backend built like P8's. It
accepts only the three authenticated RunAIScriptAtLocation rows: `Pro9` for
slot 0, `Pr9W` for slot 6 (Baelrog) and `Pr9Y` for slot 5 (Leviathan).

- **Production.** Every 120 updates, each town builds a request heap from
  Zergling, Hydralisk and Mutalisk requests and trains from its Hatcheries'
  larva, paid from its source wallet through `race_queue_unit()`. The heap
  passes the native cap/count admission gate, is rebuilt on every step and
  is never saved.
- **Attack waves.** Only `Pro9` attacks: idle units near its town move on the
  human Nexus from update 9,000, then every 4,500 updates, starting at four
  units and growing by two up to 12. The Cerebrate broods hold their ground.

Native composition, cadence and targeting are not claimed. The AI does not
build detectors on purpose; it uses the Overlords and Spore Colonies it has.

## Not active in P9

- Defiler spells.
- AI use of the Carrier, Arbiter or Battery services.

## Qualification

Base: the P8 candidate (#151, 8927812, which merges `codex/source-release`
8d1e653), plus this change. Inputs are the locally verified USA ROM (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and the P9 source CHK
(SHA-256 `eecccb0220c7456cacccf8b84f9bd82535f5cdce9192ba7d88986ea2867ba601`).
The audio bank is the shared format-2 bank. No ROM, CHK, generated header,
trace or save is tracked.

### Host controller

`tests/sc64_p9_campaign_run.c` starts from the title screen and drives the
ordinary campaign menu and gameplay inputs through `game_update()`. It
inspects live state only to choose inputs. Its policy:

- Rescue Zeratul and Fenix, and build a Nexus by the start's mineral line.
- Grow to 14 Probes. Build Pylons on supply, six Gateways, an Assimilator, a
  Cybernetics Core, a Forge, four Photon Cannons by Fenix's shelter and two
  by the expansion. Keep a walkable lane around every structure.
- Expand to the western mineral field before the start field runs out.
  Defend every Nexus; Overlords and threats the army has not reached after
  five orders are ignored.
- Re-mine stalled builders and Probes that drift to distant patches.
- Order the army by double-tapping Zealot and Dragoon groups, so Fenix and
  Zeratul are never part of army orders.
- Attack the Baelrog Cerebrate, then the Leviathan, along a breadth-first
  route over the engine's walkability grid. Gather short of the target, then
  strike, clearing its Sunken Colonies first. The second assault starts at
  56 units, or at 40 once 60,000 frames pass without reaching 56.
- Keep cloaked Zeratul home until the front is within 30 route cells, then
  trail it. Send him into the Cerebrate's location only once it is nearly
  dead, its Sunken Colonies are gone and an escort is present; pull him back
  when his shields drop, and keep him home while the army is under 16.
- After a Cerebrate falls, Zeratul leaves first while the army holds for 900
  frames, then travels home inside the regrouping army.
- From frame 30,000, save once through the pause menu (START, B, START)
  while a structure is under construction, after returning the unsaved UI
  state (building selection, the orders menu's remembered command, an open
  orders menu) to its load-time form with ordinary inputs.

The run is deterministic: two runs gave byte-identical traces and saves. It
wins at frame 180,921 after 182,807 input calls, with no runtime faults.
The controller stops recording at the deciding call, so a replay ends on the
same frame.

| Item | Value |
| --- | --- |
| Baelrog | Killed with Zeratul present by frame 93,154 (switch 0 set) |
| Leviathan | Killed with Zeratul present at frame 180,921 (both switches, victory) |
| Final state | Zeratul 60 HP and 400 shields, Fenix 240 HP, 57 army units, 130 enemy units and 74 enemy buildings left |
| Save | Frame 33,585, one structure warping in; the record carries the construction (`CHP1`) and building-damage (`BD01`) markers |
| Controller trace | 9,017 key transitions; SHA-256 `1b38e2bb272e715fc3d923339b44997c5e4ef27e017455a4b0ced89c18650aa9` |
| Tape header | SHA-256 `c4896b87e40973370b0c91891acdb6f5ce60d0d478b8cf77d16b33d1ea13a91a` |

Moving the save point changes the trajectory. Saving from frame 28,000 also
wins. Saving from frame 32,000 or 36,000 loses Zeratul shortly after the
first Cerebrate falls, which ends the mission by the source Defeat rule.
These losses come from where the controller takes Zeratul, not from a
runtime fault; the source rules, cloak and detection behave as above.

### Input-only replays

`tests/sc64_p9_arm_replay.c` replays the controller's key transitions from
power-on. It runs the shipped `game_update()`, `audio_update()` and
`game_render()` loop and writes no game state. It reports counts and a state
digest every 1,000 tape ticks and at the end. The same source builds as a
host program and as ARM ROMs:

| Build | Behavior |
| --- | --- |
| Full | Plays the whole tape. |
| Prefix | Stops after the pause-menu save sequence (tick 34,243), leaving the save in SRAM. |
| Resume | Boots a fresh process, quick-loads with L on the title screen, then continues from tick 34,243. |

Host (`make test-sc64-protoss-p9`):

- The full replay wins at frame 180,921.
- The prefix's SRAM image is byte-identical to the controller's save.
- The fresh-process resume matches the full replay at all 149 common
  observations, ending in the same win.

ARM (mGBA 0.10.5, one isolated process per run, log mask 7, no debugger):

- **ROM image `9d7067f9…84` (full).** Matches all 183 host observations and
  wins at frame 180,921 with state digest `3574889178`. 805.1 s wall time.
- **ROM image `02b4a057…5d` (prefix).** Matches all 35 host prefix
  observations. Its save record (tag `0x5343505d`, 16,292-byte payload,
  sequence 1, with the `CHP1` and `BD01` markers) is byte-identical to the
  host prefix's record. The SRAM files differ only in the unwritten bytes
  after the record (0x00 on host, 0xFF in mGBA).
- **ROM image `730eafd2…3f` (resume).** Seeded with the ARM prefix's save
  file. Loads at frame 33,585 with the prefix's final digest (`1412215360`).
  Matches the host resume at all 150 observations and the uninterrupted host
  replay at all 149 common ticks, ending in the same win.

| ARM timing | Value |
| --- | --- |
| Frames (whole tape, menu and briefing included) | 182,807 |
| Mean cycles/frame | 2,035,338 (about 8.2 fps equivalent) |
| Mean update cycles | 1,309,306 |
| Peak frame | 18,828,429 cycles |

The tape includes menu and briefing frames. The peak frame has not been
broken down. None of these are physical-GBA measurements.

### Build provenance

As for P8, `tools/sc64_build_provenance.py record` rebuilt every qualified
binary with an ordinary `make` and kept, in [`provenance/`](provenance/),
the executed commands with every `-D`/`-I` in its real order, the raw
compiler `-M` output, and the SHA-256 of every input (including the linker
script and audio blob) and output.

| Record | Outputs |
| --- | --- |
| `roms` | Playable ELF and ROM (`8bb979eb…fd`), replay ELFs and ROMs (the three images above) |
| `host-replay` | Host full, prefix and resume replay programs |
| `controller` | Host controller program |

The rebuilt ROM images are the images the emulator ran.

### Builds and guards

| Build | `__bss_end` | `__iwram_end` |
| --- | --- | --- |
| Playable candidate | `0x02038d6c` | `0x03004c70` |
| Replay ROMs | `0x0203862c` | `0x03004c70` |

Both are below the unchanged guards (`0x0203e000` and `0x03005000`).

### Non-regression

- Canonical Protoss rebuilt on this branch: SHA-256
  `33148e729053641f7d22f8a9064f2c4ba1146d15cf20b2178854216df71bdca9`, the
  image of `codex/source-release` 8d1e653.
- The P8 candidate rebuilt on this branch: SHA-256
  `0c9c2ab74fa8301fc6932a520d4e4c473e7f57accbfc358f654bb29865373471`, the
  qualified P8 image. Its controller trace and tape regenerate with the P8
  qualification's hashes, and its host full and resume replays reach the
  same win.
- `./dev build` and `./dev test` pass.

## Reproduce

With the private P7, P8 and P9 inputs already prepared:

```sh
make protoss-p9
make test-sc64-protoss-p9        # controller trace, tape, host full/prefix/resume
make sc64-p9-arm-replay           # three replay ROMs
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay.gba --out build/emulator/p9-arm-full
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 compare build/p9-arm/host-full.txt build/emulator/p9-arm-full/p8-lines.txt
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay-Prefix.gba --out build/emulator/p9-arm-prefix
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay-Resume.gba --out build/emulator/p9-arm-resume \
    --sav-in build/emulator/p9-arm-prefix/saves/StarCraft-Protoss-P9-Replay-Prefix.sav
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 compare build/p9-arm/host-full.txt build/emulator/p9-arm-resume/p8-lines.txt --from-tick 34243
```

`./dev sc64` does not list `protoss-p9` yet; use the make targets above.

## Limits

- One controller policy and one save point are qualified; two of the four
  save points tried win. Other saves, defeat endings and arbitrary input
  are not qualified.
- The AI backend and the clock and pass cadence are authored. Native AI
  behavior is not reproduced.
- Defiler spells are not implemented.
- Mission text, transmissions and sounds are not presented.
- Performance is far below the 30-fps goal and has no hardware measurement;
  frame rate is not qualified.
- The candidate is not promoted. Canonical Protoss remains P7.
