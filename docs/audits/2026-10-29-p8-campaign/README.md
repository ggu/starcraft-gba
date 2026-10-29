# P8 "The Trial of Tassadar" candidate (2026-10-29)

This audit records the playable P8 build: an explicit `protoss-p8`
candidate that carries the canonical P1–P7 Protoss profile and adds P8 as the
eighth campaign entry. Canonical Protoss stays the qualified P7 product; its
ROM is byte-identical to the base before this change.

The evidence is one ordinary-input host controller win, an input-only ARM
replay of that controller's key tape, and a two-process SRAM save and
continuation taken while a structure is under construction. All runs are
emulator or host evidence. No physical-GBA or native-AI equivalence claim is
made.

## What is wired

- **Catalog and menu.** `SC64_P8_CAMPAIGN_ENTRY` makes catalog entry 27
  ("THE TRIAL") playable. It is the second row of the fourth mission page.
- **Runtime.** `starcraft/sc64_p8_runtime_adapter.h` binds the merged P8
  source adapter, trigger runner, world adapter, timed service and strict
  property-create bridge to the live pools. It runs one trigger runner per
  executing source owner: the human (slot 7, force 1), the Stasis Cell
  defenders (slot 0, force 2) and the Conclave towns (slot 3, force 3).
  Census, owner, location and resource actions use the shared Z8 world
  services. Every engine-created actor is tagged with a source slot; products
  inherit their producing building's slot.
- **Victory and defeat.** The human runner's Victory and Defeat rows set the
  mission result. They are the Stasis Cell's destruction and the source
  epilogue, Fenix's or the Hyperion's death after Fenix is created, and the
  human commanding no actors.
- **Rescue.** Fenix (created for rescuable slot 2) and the Hyperion (slot 2)
  join the human side on contact, using the same bounded 32-pixel rule as
  P4/P6. They keep their source slot, which the defeat census counts.
- **Source pause.** PauseGame freezes combat, production and commands while
  the trigger clock and its waits continue, following the P4 pattern.
- **Saves.** `SCP8`, generation 44. The P8 state is 148 bytes in the existing
  180-byte Protoss mission union, so the 16,292-byte save record keeps its
  size. P7 (generation 43) saves are rejected by tag; no conversion is
  performed.
- **Profile.** `make protoss-p8` builds `build/StarCraft-Protoss-P8-GBA.gba`
  from `PROTOSS_P7_CANONICAL_FLAGS`, plus the P8 data/runtime/entry flags and
  separate fact tables for the P8 roster (`build/generated/p8-fact-v2`). The
  P7 sprite bank already holds every P8 actor (raw 28 reuses raw 12's
  verified art), so no new art bank is generated.

The Stasis Cell had no source footprint in the Protoss profile, so the shared
census faulted on it. Its pinned BWAPI `unitDimensions` row 168
(`{4,3,64,48,63,47}`) is now in the P7 rectangle helper, under
`SC64_P8_DATA_ENABLED` only. The Z8 pool bridge guard now also accepts the
P8 runtime, and its Z7 raw-table fallback is compiled only with Z7.

## Shared Protoss unit services

The imported P8 map places a human Carrier (source slot 7, empty hangar),
three enemy Carriers, three Arbiters, an Observer and two Shield Batteries.
The P7 services for these units were gated on P7 mission state. They are now
unit services that every mission carrying a source-owner map and a service
clock runs; P8 is the first such mission.

- **The gate.** `race_protoss_services_active()`,
  `race_protoss_source_owner()` and `RACE_PROTOSS_SERVICE_CLOCK` (game.c)
  replace `race_p7_active()`, `race_p7_source_owner()` and `race_p7.reserved`
  in the Carrier, Interceptor flight, projectile, Arbiter cloak, Observer and
  Cannon detection, and Shield Battery code. Without
  `SC64_PROTOSS_SHARED_SERVICES` (every build except P8 and later) they
  expand to the original P7 tokens, and canonical P7 is byte-identical.
- **Mission state that stays P7's.** P7's three scripted Carrier creates and
  its slot reservation, its map and actor checks, and its two authenticated
  Battery rows stay P7-only.
- **P8's own state.** P8 saves the shared service clock (phase and stagger,
  laid out like P7's three reserved bytes) in its union payload, and resets
  the Arbiter/detection cache at mission start and on load as P7 does. Its
  source-owner map is read from the canonical descriptor, so save candidates
  are checked before the world is bound. Load preflight checks every unit's
  cache word against the candidate mission's own source mask.
- **Shield Batteries.** `tools/sc64_p7_battery_pack.py --mission P8`
  authenticates P8's two map Batteries (slot 3, 100 % hit points, shields and
  energy) from the private CHK into this worktree's
  `build/generated/p8-battery`. At start both rows bind to those properties.
  Save, load and live-world checks accept those two rows at their source
  positions and human Batteries; recharge orders validate in P8 saves too.
- **What the player gets.** The human Carrier offers BUILD INTERCEPTOR, its
  Interceptors train, launch and fight. Enemy Arbiters cloak nearby units,
  enemy Observers are cloaked, Photon Cannons detect, and Probes can build
  Shield Batteries that recharge shields.
- **AI limit.** The authored AI does not order enemy Carriers to train
  Interceptors, use Arbiter abilities or recharge at Batteries. Canonical
  P7's AI does not do this either.

## Defects found and fixed

- **Building and unit name tables.** P8 added `B_STASIS_CELL_PRISON` and
  `U_P8_HYPERION` to the positional rule enums without names. Every later
  building name was shifted by one, and the last one (Shield Battery) was
  null. With the Battery now buildable in P8, the command HUD read that null
  name and crashed the input-only replay (the host controller does not
  render). Both tables now have the P8 entries.
- **Save/resume and UI state.** The fresh-process continuation first
  diverged because the controller saved while UI state that is not saved was
  live: a selected building, the orders menu's remembered command
  (`menu_last`), and an open orders menu, which holds the game. A load
  starts without them, so the same keys then meant different commands. The
  controller now returns to that state with ordinary inputs before pausing;
  the save format is unchanged.

## Authored policies

These choices are authored for the GBA build. They are not native
measurements.

| Area | Policy |
| --- | --- |
| Source clock | One engine update is 42 ms of source time (native Fastest speed). ElapsedTime, Wait and Transmission use this clock. |
| Trigger passes | Every 8 updates; waits tick every update. |
| Transmission | All P8 transmissions name `blank.wav` with time 0 and modifier 9 (subtract), so the native duration is that sound's length. The length is not imported; a fixed 4,000 ms is used. Text and sound are not shown or played. |
| CenterView | The camera snaps to the location. The indefinite pan wait is released on the next update. |
| AI scripts | See the next section. |

### AI backend

The native SC64 AI script programs are still undecoded (see
`docs/research/z8-native-ai-policy-programs.md`), so `starcraft/sc64_p8_ai.h`
is a bounded authored backend, not a script interpreter. It accepts only the
three authenticated RunAIScriptAtLocation rows: `Pro8` and `Pr8B` for slot 3,
and `Pr8D` for slot 0.

- **Production.** Every 120 updates, each town builds a request heap from a
  fixed three-type composition.
  - Conclave towns: Zealot, Dragoon and Reaver, capped at 16, 10 and 4.
  - Defenders: Dragoon, Zealot and Scout, capped at 24, 12 and 12.
  - The heap uses the merged native-shaped request queue, and requests pass
    the native cap/count admission gate. Requests pop in heap order. Each is
    trained on an idle, powered producer of that town, paid from its source
    wallet through the engine's ordinary `race_queue_unit()`.
  - The heap is rebuilt on every step and never saved. The settlement
    service is not used.
- **Attack waves.** The two Conclave towns release idle army units near the
  town toward the human Nexus from update 9,000, then every 4,500 updates.
  Waves start at four units and grow by two, up to 12. The defenders do not
  attack.

Native composition, cadence, targeting and the meaning of the native counts
are not claimed.

## Qualification

Base `codex/source-release` 8d1e653 (merged into this branch), plus this
change. Inputs are the locally verified USA ROM (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and the P8 source CHK (SHA-256
`e2257c48a119bfde3da075e08628b312857e3aad9222c47678fc29d57c578cd9`). The
audio bank is the shared format-2 bank. No ROM, CHK, generated header, trace
or save is tracked.

### Host controller

`tests/sc64_p8_campaign_run.c` starts from the title screen and drives the
ordinary campaign menu and gameplay inputs through `game_update()`. It
inspects live state only to choose inputs. Its policy:

- Rescue Fenix. Keep Fenix and the Hyperion out of army orders: the army is
  ordered by double-tapping Zealot and Dragoon groups, not with R. A hero
  with an enemy within 192 pixels steps away past the Nexus.
- Grow to 14 Probes. Build Pylons on supply, six Gateways, an Assimilator, a
  Cybernetics Core, a Forge and four Photon Cannons, keeping a walkable lane
  around every structure. Re-mine any Probe whose build stays pending for
  900 frames.
- Fill the human Carrier's hangar through its BUILD INTERCEPTOR row, and
  send it with the army once two Interceptors are aboard.
- Attack once the army has 24 units, routing from the front unit over the
  engine's walkability grid; a front unit whose route stops shrinking is
  skipped. At the Stasis Cell, Zealots attack the Cell and Dragoons the
  Arbiters, since the army has no mobile detector.
- From frame 30,000, save once through the pause menu (START, B, START)
  while a structure is under construction, after returning the UI state
  above to its load-time form.

The run is deterministic: two runs gave byte-identical traces and saves. It
wins at frame 104,385 after 106,288 input calls, with no runtime faults. The
controller stops recording at the deciding call, so a replay ends on the
same frame. Moving the save point to frames 28,000, 32,000 or 34,000 also
wins (at frames 101,393, 88,385 and 88,217), so the result does not hang on
the qualified save point.

| Item | Value |
| --- | --- |
| Final state | Fenix 197 HP, the Hyperion 850 HP, 60 enemy units left |
| Carrier | Four Interceptors trained by frame 3,595; up to four launched at once |
| Cloak | Six enemies still cloaked at the win |
| Controller trace | 9,993 key transitions; SHA-256 `8fb73251ed05f5fa6af634d93e006932aed3f0feaa853657b2232d1dafb23aaf` |
| Tape header | SHA-256 `499513f57868f8e98962c70c3d1d79271d7f34ee12a0a7dde163a47974260a02` |

The save is taken at frame 31,205 with one Pylon warping in. Its record
carries the newly merged construction (`CHP1`) and building-damage (`BD01`)
markers.

### Input-only replays

`tests/sc64_p8_arm_replay.c` replays the controller's key transitions from
power-on. It runs the shipped `game_update()`, `audio_update()` and
`game_render()` loop and writes no game state. It reports counts and a state
digest every 1,000 tape ticks and at the end. The same source builds as a
host program and as ARM ROMs:

| Build | Behavior |
| --- | --- |
| Full | Plays the whole tape. |
| Prefix | Stops after the pause-menu save sequence (tick 31,943), leaving the save in SRAM. |
| Resume | Boots a fresh process, quick-loads with L on the title screen, then continues from tick 31,943. |

Host (`make test-sc64-protoss-p8`):

- The full replay wins at frame 104,385.
- The prefix's SRAM image is byte-identical to the controller's save.
- The fresh-process resume matches the full replay at all 76 common
  observations, including the win and state digest `2300458568`.

ARM (mGBA 0.10.5, one isolated process per run, log mask 7, no debugger):

- **ROM image `c571c769…32` (full).** Matches all 107 host observations and
  wins at frame 104,385. 471.2 s wall time.
- **ROM image `73b3d46b…a7` (prefix).** Matches all 32 host prefix
  observations. Its save record (tag `0x5343505c`, 16,292-byte payload,
  sequence 1) is byte-identical to the host prefix's record.
- **ROM image `c8c6841d…2d` (resume).** Seeded with the ARM prefix's save
  file. Loads at frame 31,205 with the prefix's final digest (`2532240828`).
  Matches the host resume at all 77 observations and the uninterrupted host
  replay at all 76 common ticks, ending in the same win.

| ARM timing | Value |
| --- | --- |
| Frames (whole tape, menu and briefing included) | 106,288 |
| Mean cycles/frame | 1,994,247 (about 8.4 fps equivalent) |
| Mean update cycles | 1,236,684 |
| Peak frame | 22,156,674 cycles |

The tape includes menu and briefing frames. The peak frame has not been
broken down. None of these are physical-GBA measurements; frame rate is
not qualified.

### Build provenance

`tools/sc64_build_provenance.py record` deletes the named build outputs,
rebuilds them with an ordinary `make`, and keeps what make executed. It
refuses to run if make would regenerate any input outside `build/`. The
records are in [`provenance/`](provenance/):

- `commands.txt`: the executed compile, link, objcopy and ROM-fix commands,
  with every `-D`/`-I` in its real order.
- `deps/*.deps`: the raw compiler `-M` output for each compile command.
- `inputs.txt`: the SHA-256 of every dependency and every file named on a
  command line, including the linker script and the audio blob.
- `outputs.txt`: the SHA-256 of each output.

| Record | Outputs |
| --- | --- |
| `roms` | Playable ELF and ROM (`0c9c2ab7…71`), replay ELFs and ROMs (the three images above) |
| `host-replay` | Host full, prefix and resume replay programs |
| `controller` | Host controller program |

The rebuilt ROM images are the images the emulator ran.

### Builds and guards

| Build | `__bss_end` | `__iwram_end` |
| --- | --- | --- |
| Playable candidate | `0x02038994` | `0x03004c40` |
| Replay ROMs | `0x02038254` | `0x03004c40` |

Both are below the unchanged guards (`0x0203e000` and `0x03005000`).

### Canonical P7 non-regression

Rebuilding canonical Protoss on this branch gives SHA-256
`33148e729053641f7d22f8a9064f2c4ba1146d15cf20b2178854216df71bdca9`, the same
image as `codex/source-release` 8d1e653. The P7 carrier-training,
interceptor-flight, observer, projectile, OBJ VRAM and adapter fixtures pass.
`./dev build` and `./dev test` pass.

Pre-existing on 8d1e653 and unchanged here:

- `make test-sc64-p7-battery` and `make test-sc64-p7-arbiter-host` stop at
  `sc64_p7_battery_game.c:411`, which expects a Probe's build order to be
  immediate. The merged pending-build order makes it `ORDER_BUILD_PENDING`
  first.
- `make test-sc64-protoss-p7-canonical` links the wrong `$<`, and the
  canonical P7 controller stalls on cursor-guard failures at frame 11,953.

## Reproduce

With the private P7 and P8 inputs already prepared:

```sh
make protoss-p8
make test-sc64-protoss-p8        # controller trace, tape, host full/prefix/resume
make sc64-p8-arm-replay           # three replay ROMs
python3 tools/sc64_p8_arm_replay.py run --rom build/StarCraft-Protoss-P8-Replay.gba --out build/emulator/p8-arm-full
python3 tools/sc64_p8_arm_replay.py compare build/p8-arm/host-full.txt build/emulator/p8-arm-full/p8-lines.txt
python3 tools/sc64_p8_arm_replay.py run --rom build/StarCraft-Protoss-P8-Replay-Prefix.gba --out build/emulator/p8-arm-prefix
python3 tools/sc64_p8_arm_replay.py run --rom build/StarCraft-Protoss-P8-Replay-Resume.gba --out build/emulator/p8-arm-resume \
    --sav-in build/emulator/p8-arm-prefix/saves/StarCraft-Protoss-P8-Replay-Prefix.sav
python3 tools/sc64_p8_arm_replay.py compare build/p8-arm/host-full.txt build/emulator/p8-arm-resume/p8-lines.txt --from-tick 31943
python3 tools/sc64_build_provenance.py record build/provenance/p8/roms \
    build/starcraft-protoss-p8.elf build/StarCraft-Protoss-P8-GBA.gba
```

`./dev sc64` does not list `protoss-p8` yet; use the make targets above.

## Limits

- One controller policy and one save point are qualified. Other saves,
  defeat endings and arbitrary input are not.
- The AI backend, the transmission length and the clock and pass cadence are
  authored. Native AI behavior is not reproduced, and the AI does not use
  the shared Carrier, Arbiter or Battery services.
- Mission text, transmissions and sounds are not presented.
- Performance is far below the 30-fps goal and has no hardware measurement.
- The candidate is not promoted. Canonical Protoss remains P7.
