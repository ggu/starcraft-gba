# P8 "The Trial of Tassadar" candidate (2026-10-29)

This audit records the first playable P8 build: an explicit `protoss-p8`
candidate that carries the canonical P1–P7 Protoss profile and adds P8 as the
eighth campaign entry. Canonical Protoss stays the qualified P7 product; its
ROM is byte-identical to the base before this change.

The evidence is one ordinary-input host controller win, an input-only ARM
replay of that controller's key tape, and a two-process ARM SRAM save and
continuation. All runs are emulator or host evidence. No physical-GBA or
native-AI equivalence claim is made.

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
- **Saves.** `SCP8`, generation 44. The P8 state is 144 bytes in the existing
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

## Not active in P8

Several ability services are still gated to P7 mission state, so they do not
run in P8:

- Carrier interceptor training and flight
- Arbiter cloaking
- Observer detection policy
- Shield Battery recharge and energy

P8 uses these unit types (its human Carrier, and enemy Carriers, Arbiters,
Observers and Shield Batteries). They behave as the generic engine treats
them, and the source tutorial text about building Interceptors has no effect.
Extending these services to P8 is separate work.

## Qualification

Base `codex/source-release` 9ff7f7c, plus this change. Inputs are the locally
verified USA ROM (SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`) and the
P8 source CHK (SHA-256
`e2257c48a119bfde3da075e08628b312857e3aad9222c47678fc29d57c578cd9`). The
audio bank is the shared format-2 bank. No ROM, CHK, generated header, trace
or save is tracked.

### Host controller

`tests/sc64_p8_campaign_run.c` starts from the title screen and drives the
ordinary campaign menu and gameplay inputs through `game_update()`. It
inspects live state only to choose inputs. Its policy:

- Rescue Fenix, then keep both heroes in the base.
- Grow to 14 Probes, build Pylons on supply, six Gateways, an Assimilator and
  a Cybernetics Core.
- Train Zealots and Dragoons and defend the base.
- Once the army reaches 24 units, attack-move along a breadth-first route
  over the engine's walkability grid toward the Stasis Cell.
- Save once through the pause menu (START, B, START) at frame 30,001.
- Close any field menu that a refused placement leaves open, with an ordinary
  B press.

The run is deterministic: two runs gave byte-identical traces. With the save
in place it wins at frame 65,553 after 67,083 input calls, with no runtime
faults.

| Item | Value |
| --- | --- |
| Final state | Fenix 229 HP, Hyperion 850 HP, 61 enemy units and 88 enemy buildings left |
| Human runner | Triggers 3, 4 and 7 completed |
| Computer runners | Triggers 6 (defenders) and 5 (Conclave) completed |
| Controller trace | 5,080 key transitions; SHA-256 `979e5962091521c3db2d77e1804a8fd886de8978129e71b466be0471e3fbf203` |
| Tape header | SHA-256 `2616b2576879d1d0717ecca7f59300f8e3a0f5d8473c9a610158ab1530fbd247` |

The defeat rows were evaluated on every pass and never fired in this run. A
development run in which Fenix died did end in the source Defeat; that run is
not retained as evidence.

### Input-only replays

`tests/sc64_p8_arm_replay.c` replays the controller's key transitions from
power-on. It runs the shipped `game_update()`, `audio_update()` and
`game_render()` loop and writes no game state. It reports counts and a state
digest every 1,000 tape ticks and at the end. The same source builds as a
host program and as ARM ROMs:

| Build | Behavior |
| --- | --- |
| Full | Plays the whole tape. |
| Prefix | Stops after the pause-menu save sequence (tick 30,807), leaving the save in SRAM. |
| Resume | Boots a fresh process, quick-loads with L on the title screen, then continues from tick 30,807. |

Host (`make test-sc64-p8-replay`):

- The full replay wins at frame 65,553.
- The fresh-process resume matches the full replay at all 38 common
  observations, including the win and state digest `3542633619`.

ARM (mGBA 0.10.5, one isolated process per run, log mask 7, no debugger):

- **ROM image `24d2fca9…53` (full).** Matches all 68 host observations and
  wins at frame 65,553. 346.7 s wall time.
- **ROM image `6fc2ad2f…b2` (prefix).** Matches all 31 host prefix
  observations. Its save record (tag `0x5343505c`, 16,292-byte payload,
  sequence 1) is byte-identical to the host prefix's record.
- **ROM image `a449fecb…19` (resume).** Seeded with the ARM prefix's save
  file. Loads at frame 30,001 with the prefix's final digest (`2007949239`).
  Matches the host resume at all 39 observations and the uninterrupted host
  replay at all 38 common ticks, ending in the same win.

| ARM timing | Value |
| --- | --- |
| Frames (whole tape, menu and briefing included) | 67,083 |
| Mean cycles/frame | 1,669,669 (about 10.05 fps equivalent) |
| Mean update cycles | 907,814 |
| Peak frame | 26,745,418 cycles |

The tape includes menu and briefing frames. The peak frame has not been
broken down. None of these are physical-GBA measurements.

### Builds and guards

| Build | `__bss_end` | `__iwram_end` |
| --- | --- | --- |
| Playable candidate | `0x0203898c` | `0x03004c40` |
| Replay ROMs | `0x0203824c` | `0x03004c40` |

Both are below the unchanged guards (`0x0203e000` and `0x03005000`). IWRAM
end is unchanged from canonical P7. Rebuilding canonical Protoss gives the
base image SHA-256
`5ca1dba7a94b4cd21f7796a747b31eed421f11786043fdddeb77739902727d39`.

### Canonical P7 non-regression

Every P8 code path is behind `SC64_P8_*` flags, so canonical Protoss is
unchanged: the rebuilt P7 ROM is byte-identical to the base image. The P7
save/interceptor and OBJ VRAM fixtures pass.

The canonical P7 ordinary-input controller does not reach its win on the
unmodified base either. It stalls on cursor-guard failures at frame 11,953
within its 60,000-call limit, and the base produces a byte-identical trace.
`make test-sc64-protoss-p7-canonical` also fails to link on the base, because
its controller rule is read before `P7_CAMPAIGN_CONTROLLER` is defined. Both
are pre-existing and are reported separately rather than changed here.

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
python3 tools/sc64_p8_arm_replay.py compare build/p8-arm/host-full.txt build/emulator/p8-arm-resume/p8-lines.txt --from-tick 30807
```

`./dev sc64` does not list `protoss-p8` yet; use the make targets above.

## Limits

- One controller policy and one save point are qualified. Other saves,
  defeat endings and arbitrary input are not.
- The AI backend, the transmission length and the clock and pass cadence are
  authored. Native AI behavior is not reproduced.
- The P7-only ability services listed above are inactive in P8.
- Mission text, transmissions and sounds are not presented.
- Performance is far below the 30-fps goal and has no hardware measurement.
- The candidate is not promoted. Canonical Protoss remains P7.
