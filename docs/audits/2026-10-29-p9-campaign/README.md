# P9 "Shadow Hunters" candidate (2026-10-29)

This audit records the first playable P9 build: an explicit `protoss-p9`
candidate that builds on the unmerged P8 candidate (#151) and adds P9 as the
ninth campaign entry. Canonical Protoss stays the qualified P7 product; its
ROM is byte-identical to the base before this change.

The evidence is one ordinary-input host controller win, an input-only ARM
replay of that controller's key tape, and a two-process ARM SRAM save and
continuation. All runs are emulator or host evidence. No physical-GBA or
native-AI equivalence claim is made.

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
- **Saves.** `SCP9`, generation 45. The P9 state is 128 bytes in the
  existing 180-byte Protoss mission union, so the 16,292-byte save record
  keeps its size. P7 and P8 saves are rejected by tag; no conversion is
  performed.
- **Profile.** `make protoss-p9` builds `build/StarCraft-Protoss-P9-GBA.gba`
  from the P8 candidate's flags, plus the P9 data/runtime/entry flags and
  separate fact tables for the P9 roster (`build/generated/p9-fact-v2`).
- **Bindings.** P9-gated bindings add raw 46 (Defiler) and raw 136 (Defiler
  Mound), with pinned BWAPI dimensions and stock Defiler armor 1. Defiler
  spells are not implemented.
- **Shared bridge.** Without Z7, the Z8 pool bridge now falls back to the
  pinned BWAPI `unitDimensions` table for actors without a feature fact,
  which covers P9's Zerg roster.

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

Native composition, cadence and targeting are not claimed.

## Not active in P9

- Zeratul's permanent cloak. P6's cloak and detection services are gated to
  P6, so here he is an ordinary target.
- Defiler spells.
- The P7-only ability services listed in the P8 audit.

## Qualification

Base: the P8 candidate (#151) on `codex/source-release` 9ff7f7c, plus this
change. Inputs are the locally verified USA ROM (SHA-1
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
  Cybernetics Core, and a Forge with four cannons by Fenix's shelter. Keep a
  walkable lane around every structure.
- Expand to the western mineral field before the start field runs out.
- Re-mine stalled builders and Probes that drift to distant patches.
- Order the army by double-tapping Zealot and Dragoon groups, so Fenix and
  Zeratul are never part of army orders.
- Attack the Baelrog Cerebrate, then the Leviathan, along a breadth-first
  route over the engine's walkability grid. Gather short of the target, then
  strike, clearing its Sunken Colonies first.
- Keep Zeratul behind the army's front. Send him into the Cerebrate's
  location only once it is nearly dead, its Sunken Colonies are gone and an
  escort is present. Pull him back when his shields drop.
- Save once through the pause menu (START, B, START) at frame 30,660.

The run is deterministic: two runs gave byte-identical traces and saves. It
wins at frame 285,137 after 287,900 input calls, with no runtime faults.
The controller stops recording at the deciding call, so a replay ends on the
same frame.

| Item | Value |
| --- | --- |
| Baelrog | Killed with Zeratul present by frame 93,146 (switch 0 set) |
| Leviathan | Killed with Zeratul present at frame 285,137 (both switches, victory) |
| Final state | Zeratul 60 HP and 400 shields, Fenix 240 HP, 61 army units, 143 enemy units and 68 enemy buildings left |
| Controller trace | 13,534 key transitions; SHA-256 `738738ff64010fe6273550f9b86ca06722c3c3bbfbc919191618c92b4b5b46fa` |
| Tape header | SHA-256 `8581786015c2c613d19700205baf63e819a1c74b39f470629245074c7f9f42eb` |

Development runs also showed the source respawn: a Cerebrate killed without
Zeratul present was recreated by its PreserveTrigger rows. Those runs are
not retained as evidence.

### Defect found by the save check

The first qualification attempt wrote an empty SRAM image. The save encoder
rejected every map-placed Defiler: the stock unit row gives them armor 1,
but the canonical armor table still returned 0 for `U_P9_DEFILER`, so the
typed save row failed its canonical check. The table now returns stock armor
1 under `SC64_P9_DATA_ENABLED`. The controller trace was regenerated after
the fix, so the evidence below includes it.

### Input-only replays

`tests/sc64_p9_arm_replay.c` replays the controller's key transitions from
power-on. It runs the shipped `game_update()`, `audio_update()` and
`game_render()` loop and writes no game state. It reports counts and a state
digest every 1,000 tape ticks and at the end. The same source builds as a
host program and as ARM ROMs:

| Build | Behavior |
| --- | --- |
| Full | Plays the whole tape. |
| Prefix | Stops after the pause-menu save sequence (tick 31,295), leaving the save in SRAM. |
| Resume | Boots a fresh process, quick-loads with L on the title screen, then continues from tick 31,295. |

Host (`make test-sc64-protoss-p9`):

- The full replay wins at frame 285,137. Its digests match the controller
  run's own digests at every 1,000-call mark.
- The prefix's SRAM image is byte-identical to the controller's save.
- The fresh-process resume loads at frame 30,660 and matches the full replay
  at every common observation, ending in the same win (state digest
  `4284766394`).

ARM (mGBA 0.10.5, one isolated process per run, log mask 7, no debugger):

- **ROM image `78ffa6f3…58` (full).** Matches all 288 host observations and
  wins at frame 285,137 with state digest `4284766394`. 1,171.8 s wall time.
- **ROM image `1ad92d8f…fb` (prefix).** Matches all 32 host prefix
  observations. Its save record (tag `0x5343505d`, 16,292-byte payload,
  sequence 1) is byte-identical to the host prefix's record. The two SRAM
  files differ only in the unwritten bytes after the record (0x00 on host,
  0xFF in mGBA).
- **ROM image `15cf8ce7…c5` (resume).** Seeded with the ARM prefix's save
  file. Loads at frame 30,660 with the prefix's final digest (`2682390247`).
  Matches the host resume at all 258 observations and the uninterrupted host
  replay at all 257 common ticks, ending in the same win.

| ARM timing | Value |
| --- | --- |
| Frames (whole tape, menu and briefing included) | 287,900 |
| Mean cycles/frame | 1,725,369 (about 9.7 fps equivalent) |
| Mean update cycles | 1,081,104 |
| Peak frame | 19,096,004 cycles |

The tape includes menu and briefing frames. The peak frame has not been
broken down. None of these are physical-GBA measurements.

### Builds and guards

| Build | `__bss_end` | `__iwram_end` |
| --- | --- | --- |
| Playable candidate | `0x02038d64` | `0x03004c70` |
| Replay ROMs | `0x02038624` | `0x03004c70` |

Both are below the unchanged guards (`0x0203e000` and `0x03005000`). The
playable ROM image is SHA-256
`18650f359072bf04487e35b8f98609f1ba3492be2e9c8012e33e2a53ed8469ee`.

### Non-regression

Every P9 code path is behind `SC64_P9_*` flags, except the Z8 bridge's
dimension fallback. That fallback is compiled only into Z8-bridge builds
without Z7, which are P8 and P9; P8 never reaches it.

- Rebuilding canonical Protoss gives the base image SHA-256
  `5ca1dba7a94b4cd21f7796a747b31eed421f11786043fdddeb77739902727d39`.
- `make test-sc64-protoss-p8` on this branch regenerates the P8 controller
  trace and tape with the same SHA-256 values as the P8 qualification, and
  its host full and resume replays reach the same win.
- `./dev build` and `./dev test` pass.

The pre-existing canonical P7 controller stall and
`make test-sc64-protoss-p7-canonical` link failure noted in the P8 audit are
unchanged.

## Reproduce

With the private P7, P8 and P9 inputs already prepared:

```sh
make protoss-p9
make test-sc64-protoss-p9        # controller trace, tape, host full/prefix/resume
make sc64-p9-arm-replay           # three replay ROMs
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay.gba --out build/emulator/p9-arm-full
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 compare build/p9-arm/host-full.txt build/emulator/p9-arm-full/p9-lines.txt
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay-Prefix.gba --out build/emulator/p9-arm-prefix
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 run --rom build/StarCraft-Protoss-P9-Replay-Resume.gba --out build/emulator/p9-arm-resume \
    --sav-in build/emulator/p9-arm-prefix/saves/StarCraft-Protoss-P9-Replay-Prefix.sav
python3 tools/sc64_p8_arm_replay.py --mission 28 --prefix P9 compare build/p9-arm/host-full.txt build/emulator/p9-arm-resume/p9-lines.txt --from-tick 31295
```

`./dev sc64` does not list `protoss-p9` yet; use the make targets above.

## Limits

- One controller policy and one save point are qualified. Other saves,
  defeat endings and arbitrary input are not.
- The AI backend and the clock and pass cadence are authored. Native AI
  behavior is not reproduced.
- Zeratul's cloak, Defiler spells and the P7-only ability services are
  inactive in P9.
- Mission text, transmissions and sounds are not presented.
- Performance is far below the 30-fps goal and has no hardware measurement.
- The candidate is not promoted. Canonical Protoss remains P7.
