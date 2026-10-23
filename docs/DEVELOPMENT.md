# Development

The default StarCraft skirmish builds without ROMs, extracted assets, or external reference checkouts. The required tools are Git, GNU Make, `arm-none-eabi-gcc` with its `objcopy` and `size` tools, Clang, and Python 3.10 or newer. The project uses Python's standard library; no `pip install` step is needed. mGBA is only needed to play or run emulator workflows.

## Install the tools

### macOS

Install Apple's Xcode Command Line Tools for Clang, Make, and Git, then install the ARM toolchain and Python with Homebrew:

```sh
xcode-select --install
brew install arm-none-eabi-gcc python
```

Homebrew's [ARM GCC formula](https://formulae.brew.sh/formula/arm-none-eabi-gcc) provides the `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, and `arm-none-eabi-size` commands. If you want to play, install the optional [mGBA app](https://formulae.brew.sh/cask/mgba-app):

```sh
brew install --cask mgba-app
```

The wrapper checks `PATH` and also looks for `/Applications/mGBA.app/Contents/MacOS/mGBA`.

### Ubuntu 24.04

Install the build and test tools with the system package manager:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends gcc-arm-none-eabi binutils-arm-none-eabi clang make python3 git
```

To play, install the optional mGBA SDL package from Ubuntu's [package archive](https://packages.ubuntu.com/search?keywords=mgba):

```sh
sudo apt-get install mgba-sdl
```

On Ubuntu, the emulator executable may be `/usr/games/mgba`; if it is not on `PATH`, set `MGBA=/usr/games/mgba` when running the play command. If the package is unavailable for your Ubuntu release, use the package search page to check the release and enabled repositories.

Other systems need equivalent versions of these tools. `./dev doctor` reports any missing required command. To check the optional emulator too, use `./dev doctor --emulator`.

## Asset-free workflow

After cloning as described in the [README](../README.md), run:

```sh
./dev doctor
./dev build
./dev test
```

Doctor should print `OK` for ARM GCC, ARM objcopy, ARM size, Make, Clang, Python 3, and Git. Build creates `build/StarCraft-GBA.gba` and verifies the GBA ROM. Test runs the asset-free host checks and exits with status 0 when they pass. `make build/StarCraft-GBA.gba` builds the ROM without the wrapper; `make verify` builds and verifies it.

Playing is optional and requires mGBA:

```sh
./dev doctor --emulator
./dev play starcraft
```

The wrapper launches the built ROM and keeps local emulator state in `.local/play/starcraft/`. The game code is in `starcraft/`, including `starcraft/game.c`; GBA display and input support is in `common/`. Host-side behavioral tests are in `tests/`, and build, conversion, validation, and emulator helper scripts are in `tools/`.

## Optional unified profile

The unified campaign/custom-game profile requires a user-supplied StarCraft 64 USA ROM that you are entitled to use. The supported input is a `.z64`, `.v64`, or `.n64` file, or a ZIP containing exactly one 32 MiB N64 image. Preparation normalizes byte order and verifies SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. It does not download a ROM. The pinned `sc64-maps` extractor is GPL-3.0-or-later and is not vendored; the first preparation clones it from its public upstream into ignored `references/`, which requires Git and network access.

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile unified
make unified
make test-unified
./dev play unified
```

Preparation writes extracted and generated inputs under ignored `.local/`; ROM builds and reports go under `build/`. Do not commit or distribute the ROM, imported maps or art, generated headers, or captures containing game graphics. Default-profile contributors do not need this workflow.

See [SC64 unified](SC64-UNIFIED.md) for implemented missions and limitations, and [SC64 development notes](SC64.md) for import details.

## T4 internal actor/world checks

The Jacobs Installation data and actor rules have an internal, gated host-test
path. This is not a playable campaign entry: the frontend catalog remains
closed to T4, and no frame-rate result is claimed. After preparing the local
unified inputs, run:

```sh
make test-sc64-t4-actors
make test-sc64-t4-storage sc64-t4-storage-probe
```

The actor tests load the 95 source placements, check supported actor roles,
sight, combat and bounded rescue behavior, and validate the special Beacon's
save representation. Storage tests cover the T4-only 128×128 layout. Both
targets use private generated inputs under ignored `.local/` storage. The
initial actor/storage stage did not implement doors, traps, teleports, trigger
objectives or completion behavior. Later gated runtime checks are described
below. See [Validation](VALIDATION.md) for the exact test coverage and limits.

The private numeric scenario import and the asset-free trigger/query core have
separate checks:

```sh
make sc64-t4-scenario test-sc64-t4-scenario
make test-sc64-t4-triggers test-sc64-t4-queries test-sc64-t4-actions
```

The first line needs the retained records produced by local race preparation.
Re-run preparation after updating an older checkout: T4 retention now includes
FORC, OWNR, SIDE, UPRP and UPUS, in addition to the original eight sections.
It reconstructs the full source sections from the generated C schema and
checks their canonical digests. The second line needs no ROM or generated
data and is included in `./dev test`. These commands document the earlier
numeric-import and callback-only stage; see the gated runtime status below for
subsequent integration.

World-effect callbacks have a separate internal fixture:

```sh
make test-sc64-t4-effects
```

This requires the same private prepared inputs. It runs queries, removals,
location movement and bounded group teleport effects against synthetic engine
states, with creation and doodad calls stubbed at their callback boundaries.
It does not start the T4 trigger scheduler, save that state or prove mission
completion. This callback-only fixture remains useful for the standalone
effect API; the later runtime fixture also exercises those effects in context.

## Gated T4 scenario runtime

The internal Jacobs Installation runtime binds all 95 imported UNIT rows and
26 unit-backed THG2 props into the fixed pools: 121 initial actors total
(87 units and 34 buildings). It evaluates the 20 retained doodad-state
actions, handles supported creation and map-revealer actions, updates door
collision/path blocking, and runs the bounded trigger contexts. Standard
profiles still exclude T4; the combined Terran experiment described below is
the opt-in normal-menu build.

Door presentation uses an authored immediate-state policy: enabled/closed
doors draw source frame 0, disabled/open doors draw frame 9; the paired right
door types are mirrored. Traps draw static frame 0 when enabled and are hidden
when disabled. Door animation timing and trap animation are not reproduced.
Disabling a trap suppresses firing and drawing but leaves its actor targetable;
native T4 initial targetability has not been established. This is an authored
compatibility policy, alongside the immediate door endpoints.
These visual choices are bounded compatibility policies, not proof of native
N64 IScript behavior. The runtime save generation is version 10 and persists
the trigger and doodad state.

T4 distinguishes raw-20 Marine Raynor from the older raw-19 Vulture alias.
Marine Raynor has the pinned source weapon and armor rules; older T4 saves
are rejected after this compact actor-type change. To check acquisition and
damage against trap, ground and air targets, run
`make test-sc64-t4-raynor-combat` after local preparation.

After preparing the local T4 inputs, the focused host checks and ARM link probe
are:

```sh
make sc64-t4-scenario \
  test-sc64-t4-effects test-sc64-t4-creation test-sc64-t4-traps \
  test-sc64-t4-prop-art test-sc64-t4-scenario-save test-sc64-t4-runtime \
  sc64-t4-scenario-probe
```

The runtime test checks the full opening roster, 20 doodad toggles, door
collision/path-grid changes, pause/save/load continuity, and trigger execution.
Its win/loss fixtures directly place or remove scenario actors; they are
synthetic rule-integration checks, not controller-played mission completion.
The link probe checks the complete opt-in engine profile. No claim is made for
full native trigger eligibility, original animation behavior, full campaign
playability, or near-30-fps operation.

For controller/replay development, build the separate internal ROM:

```sh
make sc64-t4-replay test-sc64-t4-frontend-entry test-sc64-t4-ping \
  build/test-sc64-t4-controller
```

`build/StarCraft-T4-Internal-GBA.gba` has an explicit
`SC64_T4_REPLAY_ENTRY` flag. On HOME, press START+SELECT together to open the
T4 briefing, then A to deploy. Standard Terran builds still list only their
first two missions. The on/off frontend fixtures exercise this distinction without
private inputs. The ping fixture checks the live minimap marker, pulse,
clipping and expiry. The controller runner records input attempts under ignored
`.local/` storage and exits successfully only on a win; a limit/failed attempt
is useful diagnostic evidence but does not establish completion.

Replay a recorded attempt on ARM, or measure a bounded prefix:

```sh
python3 tools/sc64_t4_controller_replay.py --trace .local/sc64/t4-controller-attempt.json
python3 tools/sc64_t4_controller_replay.py --trace .local/sc64/t4-controller-attempt.json --ticks 5000
```

The helper validates launch inputs and checkpoint fields before starting
mGBA. It writes controller scheduler entries, observes gameplay through ELF
symbols/DWARF, and saves private checkpoint/progress reports with ROM and trace
hashes. A replay that reaches a recorded limit or loss verifies that attempt;
only a matching source-trigger win can report `verified_win`.

For changes to cached terrain or prop presentation, run all eight combinations
of scrolling, clear suppression and static-key culling:

```sh
make test-sc64-t4-frame-coverage
```

The fixture compares both poisoned alternating framebuffers against an
independently cleared, freshly rendered full frame. It covers door/trap state
and binding changes, camera edges, fog, pause/results and restart/save/load.
`T4_OPTIMIZATION_FLAGS` enables count memoization, scrolling, clear suppression
and static-key culling for this internal profile. For a comparison build, use
`make -B sc64-t4-replay T4_OPTIMIZATION_FLAGS=` and freeze its ROM/ELF pair
before rebuilding the optimized default.
Diagnostic builds with `SC64_DIAGNOSTIC_UPDATE` expose separate rescue, trigger
and trap cycle counters, which the controller replay report records when present.

## Combined Terran T1–T4 experimental campaign

The combined target builds Terran missions 1–4 together, including T3 and
Jacobs Installation (global mission ID 7). It requires the same local Terran
preparation and private generated T4 data as the internal runtime:

```sh
make terran-t4
make test-sc64-t3t4-runtime test-sc64-t3t4-save \
  test-sc64-t3t4-skirmish test-sc64-t3t4-controller \
  test-sc64-t4-campaign-frontend
```

The combined runtime suite also covers the T3 survival win, save/resume and
HUD behavior. The skirmish fixture exercises the map/race/opponent setup
matrix for this profile.

This produces `build/StarCraft-Terran-T4-GBA.gba`. T4 is reached from the
ordinary campaign menu: press A from HOME, move RIGHT to page two, move DOWN
to Jacobs Installation, press A for its briefing, then A again to deploy.
The distinct `sc64-t4-replay` build still uses START+SELECT on HOME as an
internal development shortcut. The standard campaign catalog remains
unchanged; the internal replay target retains its separate entry and save
identity.

The combined ROM has save generation 11. It rejects saves from older
generations, including generation 10 used by the standalone internal T4
target, so use a separate save file. Its host and ARM controller runs both
complete the mission through the normal campaign entry. On the earlier
`181a5063` mGBA run, the win occurs at frame 9,331 (host tick 9,378); 160 input transitions
peak at 62 per scheduler page, and 11 observations match. The run averages
23.15 effective fps over 7,971 counted live frames, with a worst observed
interval of 20.71 fps. This remains below the 30-fps goal and is emulator
timing, not a physical GBA result. The ROM is 2 MiB; ROM SHA-256 is
`181a5063515a362970ce5902561ddbb61410ad43d91c248160d03439244c78bb`, and ELF
SHA-256 is
`5fc7296475ac58c2f20f624ca4bcc7a127b0bb27e01dbeabd6858e7f3a6da5cd`.
The ELF has 1,857,188 bytes of text, 388 bytes of data and 241,872 bytes of
BSS; BSS ends at `0x0203b258` and IWRAM at `0x03004ec8`, within the existing
guards.

A separate 10,000-tick neutral skirmish run on Blood Bath with three opponents
of independently selected Zerg, Protoss and Terran races on the earlier
`181a5063` build averaged 29.48 fps (observed intervals 28.58–29.86). This is
historical bounded-scenario evidence, not a measurement of the current ROM, a
full-battle worst case or a campaign performance result. Across current
experimental progression builds, Terran 1–5, Zerg 1–3 and Protoss 1–3 are
playable; the remaining 19 catalogued missions are unavailable.
`SC64_DIAGNOSTIC_UPDATE` exposes rescue, trigger and trap counters;
`SC64_DIAGNOSTIC_TERRAIN` exposes terrain reuse/event and minimap counters.
These are instrumentation, not separate performance results.

The current combined default enables `SC64_UNARMED_QUERY_SKIP`. Its frozen
2 MiB ROM SHA-256 is
`d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`; the
matching ELF SHA-256 is
`3788c697cafe3f5693ee9b752013a6dee9cdc9a1c6673a90d838d920985d1202`. The
same controller win, 11 gameplay checkpoints and three captures match the
earlier combined build byte-for-byte for reported state and images. This
version averages 23.53 effective fps over 7,971 counted frames, with a worst
interval of 21.17 fps. It remains below the 30-fps target. The experimental
576-byte minimap-interior buffer was removed after an exact 5,000-tick
comparison showed no measurable gain.
A fresh 10,000-tick Blood Bath skirmish on this current ROM, with a Terran
human and Zerg, Protoss and Terran AIs, averages 29.49 fps (intervals
28.58–29.86). Every observed checkpoint remains in play; this is a bounded
neutral-input scenario, not a full-battle performance guarantee.

To replay the recorded normal-menu controller trace against the combined ROM,
pass the matching ROM and ELF explicitly because the helper defaults to the
separate internal T4 target:

```sh
python3 tools/sc64_t4_controller_replay.py \
  --trace .local/sc64/t3t4-controller-smoke.json \
  --rom build/StarCraft-Terran-T4-GBA.gba \
  --elf build/starcraft-t4-combined.elf
```

## T5 source import checks

Antigan Revolt (Terran global mission ID 8) has an import-only data path. The
private importer retains its 96×96 Badlands map and all 168 non-start UNIT
rows: 79 units, 48 buildings and 41 mineral fields. Four start markers are
excluded from the live placement rows. Import checks alone do not add a mission
to the catalog or establish playability; the experimental combined T1–T5
runtime candidate and its qualification are described below. Eleven of the
30 missions are currently qualified as playable.

After preparing the local ROM-derived input, regenerate the race tables, then
the T5 scenario tables and run the focused source/import checks:

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_t4_scenario_pack.py --profile t5
make test-sc64-t5-import test-sc64-t5-scenario \
  test-sc64-t5-actor-rules test-sc64-resource-actions
make sc64-t5-availability test-sc64-unit-availability test-sc64-t5-availability-engine
```

Current T5 preparation also retains the exact PUNI record and generates a
342-byte availability bitmap in ignored storage. Re-run preparation after
updating an older checkout whose T5 records lack `PUNI.bin`. The internal
runtime uses this source table to reject disabled production before payment;
existing placed units remain usable. It also checks queued products before
accepting a saved T5 game.

`make test-sc64-t5-actors test-sc64-t5-ai` also runs asset-free sanitizer
checks for distinct Kerrigan, Raynor Vulture and Spider Mine actor rules,
source bounding boxes, weapon upgrades, and the opaque AI-script dispatch
boundary. Dispatch records accepted requests; it does not execute the original
script bodies. The mine remains excluded from ordinary weapon targeting.

### Internal T5 runtime probe

The separate internal runtime profile now binds the retained trigger program
to the engine. With both T4 and T5 private scenario headers prepared, run:

```sh
make test-sc64-t5-engine test-sc64-t5-world test-sc64-t5-combat test-sc64-t5-save
make test-sc64-transport-rules test-sc64-t5-tech
make test-sc64-transport-engine test-sc64-transport-queries
make test-sc64-t5-render
make sc64-t5-engine-probe
python3 tools/sc64_t5_engine_probe.py --ticks 3000
python3 tools/sc64_t5_save_smoke.py
```

The first command runs sanitizer fixtures against the real engine. They use
direct world setup to check the full roster, region-gated enemy resource
grant, rescue conversion, mine and hero combat, and mission outcome rules.
They do not demonstrate ordinary-controller completion.

The probe ROM automatically enters global mission 8 for a bounded neutral-input
ARM run. Its internal entry remains separate from the combined normal-menu
campaign build described below.
The probe uses save generation 14; the normal T1–T4 build keeps generation 11.
Use separate save files for these profiles. T5 shares the existing 16,328-byte
SRAM payload with T4: four active trigger contexts are stored, and omitted
contexts are checked against their initial state and reconstructed on load.
The save checks cover trigger waits, mine continuation, executed resource/AI
actions, invalid-state rejection before world mutation, and other mission saves.
Reports and captures remain under ignored local output directories.

`make sc64-t5-controller-probe` runs a bounded 60,000-tick host controller
attempt and writes its private trace under `.local/sc64/t5/`. It exercises
the rescue/base sequence, paid tech and supply construction, safe hero
positioning, and grouped Wraith attacks. Its input budget is enforced at each
step. Selection helpers cancel stale command modes, use the actual cursor
lattice for construction, and isolate a clickable group seed when necessary.
Private checkpoints include live Marine/Wraith counts and hero survival;
the replay reads those optional counts from the ARM actor pool. Queue/supply
diagnostics in a host trace are not independently verified by the ARM helper.
Ending at its limit is an unfinished attempt, not a mission-completion test.
For a longer bounded attempt, use `./build/sc64-t5-controller-probe --limit
75000 --trace .local/sc64/t5/controller-probe.json` after building the target.
The earlier ferry policy and evidence are retained in prior source revisions.
The separate
`SC64_T5_CAMPAIGN_ENTRY` menu flag is covered by asset-free frontend checks
and enabled only in the internal replay target:

```sh
make sc64-t5-controller-replay
python3 tools/sc64_t5_controller_replay.py --trace /path/to/private-controller-trace.json
```

That diagnostic ROM starts at HOME and uses the ordinary campaign menus.
Standard race builds and the combined Terran T1–T4 build keep T5 unavailable.
The replay helper writes controller/scheduler inputs and reads mission state;
its timing uses the GBA hardware clock. Private traces, reports and captures
stay local.

For a diagnostic update-stage breakdown, build `make sc64-t5-profile` and pass
`--rom build/StarCraft-T5-Profile.gba --elf build/starcraft-t5-profile.elf`
to the same replay helper. It adds update timing counters, including rescue
contact, mines, and trigger/AI work. These counters add measurement overhead;
use the ordinary replay ROM for final performance measurements. `--ticks`
limits a run to a prefix, which does not establish a complete mission win.
The renderer fixture compares T5 cached frames with cleared fresh renders
across timer boundaries, visible buildings/minerals, depletion, fog and camera
movement. Its key reuse adds no persistent runtime storage.

The internal T5 profile now supports paid Starport/Control Tower/Dropship
production and eight-point cargo capacity. Select ground troops and press A
on a friendly Dropship to board. Select the ship, open B commands, choose
UNLOAD CARGO and press A on the destination. Factory, Starport and Control
Tower entries follow STOP / HOLD in the scrolling command list. Select a
Factory or Starport and press R to cycle its production types, then A to queue.
The Control Tower command requires a selected, completed Starport and builds
its attached add-on automatically. Cargo stays in the existing actor/save
records; one derived activity word avoids cargo scans when no transport
orders are active. Generation 14 rejects generation-13 internal saves.
These controls remain experimental and do not enable the mission catalog.

Opaque AI scripts use explicit GBA policies: rescue conversion, passive guards,
clear-combat orders, paid production and small periodic attack groups. Hero
art reuses existing Ghost/Vulture frames. Cloaking, full mine visibility and
animation timing, THG2 sprite decorations, and original AI bytecode are still
absent. The mine's
source-based targeting and splash retain documented integer/timing/movement
approximations; ordinary hero movement and sight still use the compact engine
policies. T5 remains an experimental profile, but its normal-menu ARM win and
fresh-process save check have passed. The qualified playable mission count is
11 of 30.

The import-only T5 checks cover retained rows, metadata and digests, bounded source actor
rules, and isolated SetResources arithmetic. The actor facts do not establish
cloak or mine-arming behavior, and the resource helper intentionally does not
interpret player-group selection. Those isolated fixtures do not claim T5 runtime,
mission completion, or playability.

### Combined Terran T1–T5 experimental candidate

With the local Terran and T5 private inputs prepared, build the experimental
combined profile and run its focused host checks:

```sh
make terran-t5 test-sc64-t5-combined
```

The ROM is `build/StarCraft-Terran-T5-GBA.gba`; its matching ELF is
`build/starcraft-t5-combined.elf`. It exposes Antigan Revolt through the normal
campaign menu. The ROM uses save generation 14, distinct from combined T1–T4
generation 11. Use separate save files.

The full controller-driven host trace
`.local/sc64/t5/controller-wave20-visible-clusters-200k.json` wins at input
tick 121,547 and game frame 121,261. Repeating the run produces the same
trace; both heroes and the command center survive, all enemy buildings are
destroyed, and no trigger faults are reported. This validates the host
controller scenario. The frozen normal-menu ARM replay also wins at frame
121,261 with both heroes and the command center alive, no enemy units or
buildings, and no trigger faults. It averages 20.85 effective fps over 120,714
counted frames; the worst counted observer interval is 11.95 fps. This is
below the 30-fps target and is emulator timing, not physical GBA validation.

Generate the private winning trace, then repeat the ordinary-menu ARM replay
and fresh-process save check with:

```sh
make build/sc64-t5-controller-probe
./build/sc64-t5-controller-probe --limit 200000 \
  --trace .local/sc64/t5/controller-wave20-visible-clusters-200k.json
python3 tools/sc64_t5_controller_replay.py \
  --trace .local/sc64/t5/controller-wave20-visible-clusters-200k.json \
  --rom build/StarCraft-Terran-T5-GBA.gba \
  --elf build/starcraft-t5-combined.elf
python3 tools/sc64_t5_save_smoke.py \
  --rom build/StarCraft-Terran-T5-GBA.gba \
  --elf build/starcraft-t5-combined.elf \
  --menu-entry --output .local/sc64/t5/combined-save-menu
```

The fresh-process save check passes with generation 14 and the 16,328-byte
payload. It resumes frame 123 at frame 123 and continues to frame 183 without
player-context faults. The ROM SHA-256 is
`9344a66258c3a37e24c61eca04f3b9e95414999d8496f9ea3865f1416ad4ba7b`; ELF
SHA-256 for the frozen replay ELF is
`1a9d2b9a6a8bebdd144502aa57c445f4a150202558a694db4a16b53143141f3a`.
Rebuilding produces the same ROM; GCC's temporary object filename can change
the ELF's nonloaded string table and hash without changing code or layouts.
The build stays opt-in and does not change the standard race ROMs or combined
T1–T4 target. Across current experimental builds, 11 of 30 missions are
qualified as playable; 19 remain unavailable.

## T6 packed storage foundation (internal only)

The T6 storage probe exercises capacity for the retained Norad II source map;
it does not add Norad II to the campaign catalog or implement its mission.
First prepare the usual local Terran inputs with `./dev sc64 prepare --rom
"/path/to/StarCraft 64 ROM or ZIP" --profile terran`, then run:

```sh
make sc64-t6-prepare
make sc64-t6-storage-probe
make test-sc64-t6-import test-sc64-t6-scenario test-sc64-t6-actors
make test-sc64-t6-availability
make test-sc64-compact-building-save test-sc64-t6-storage test-sc64-t6-combined
```

The dedicated preparation command retains the T6 records, generates its
source availability bitmap, and adds its gated source sprites. The optional
availability check compares all 2,736 player/type values with the retained
raw PUNI record. That source permits the human Dropship, Starport and Control
Tower; this check does not exercise acquiring two Dropships in the game.
Run preparation again after ordinary profile preparation, which omits
T6 sprites. A storage build rejects incomplete T6 preparation.

The import retains the 96×96 map and all 216 non-start UNIT placements,
including 35 mineral fields and three geysers. The source parser classifies 105 UNIT rows as
units and 73 as buildings; runtime mapping moves raw type 195 from the source
UNIT category into the building pool, yielding 104 ordinary units and 74
buildings. The storage probe keeps pools of 160 units, 80 buildings, and 48
minerals. Packed building rows reduce the save record to 15,384 bytes and use
save generation 15, separate from T5 generation 14.

To repeat the ARM persistence check through ordinary campaign controls:

```sh
python3 tools/sc64_t5_save_smoke.py \
  --rom build/StarCraft-T6-Storage-Probe.gba \
  --elf build/starcraft-t6-storage-probe.elf \
  --menu-entry --generation 15 --output .local/sc64/t6/storage-arm-save
```

The frozen ARM storage probe passes the existing memory guards: text/data/BSS
are 2,474,724/388/245,632 bytes, IWRAM ends at `0x03004f90`, and BSS ends at
`0x0203c108`. A fresh-process ordinary-menu T5 save test on that build saves at
frame 123, reloads at frame 123, and continues to frame 183 with all eight
source-player contexts fault-free and campaign/AI clocks aligned. This proves
the new save layout on the existing T5 scenario only. T6 mission-world
integration, controller completion, and performance qualification remain
future work; the playable campaign count remains 11 of 30.

The standalone T6 trigger adapter has separate host checks:

```sh
make test-sc64-t6-runtime-synthetic  # Asset-free typed callback/core checks
make test-sc64-t6-runtime            # Requires the prepared private T6 records
```

It reuses the bounded T4 runner and retains all eight source-player contexts,
including inactive controller slots enabled by source Force masks. Its 1,184
bytes hold the contexts and shared switches. World queries, creation,
resources, AI requests and presentation use typed callbacks; they are not yet
bound to the playable GBA world. Waits use `ceil(milliseconds * 30 / 1000)`
ticks and elapsed conditions use 30 ticks per second. These are GBA adapter
policies. The checked entry validates the program and state; a separate
prevalidated entry avoids rescanning immutable tables each tick after
init/load validation. The structural state validator is not a save execution
provenance validator, and this stage does not change any ROM or SRAM format.

## Internal Terran mission and capacity probes

Norad II has a separate live-world probe, still unavailable in the normal
campaign menu until a controller completion run is qualified:

```sh
make sc64-t6-prepare sc64-t6-engine-probe
make test-sc64-t6-engine test-sc64-t6-world test-sc64-t6-save test-sc64-t6-tech
make test-sc64-t6-repair
```

The probe imports the entire starting roster, executes the source trigger
contexts, and supports paid Starport/Control Tower/Dropship construction.
Select SCVs, then press A over a damaged friendly completed Terran structure
to repair it. Repair spends resources and uses the reference construction HP
rate at 24 source steps per 30 GBA ticks; mobile mechanical repair is pending.
Original AI script bodies are still replaced by bounded authored behavior.

The late-campaign storage probe tests a distinct generation-17 layout:

```sh
make sc64-terran-late-storage test-sc64-late-terran-storage
make test-sc64-compact-unit-save test-sc64-terran-late-actors
make test-sc64-terran-late-runtime
python3 tools/sc64_t5_save_smoke.py --mission 6 --generation 17 \
  --rom build/StarCraft-Terran-Late-Storage.gba \
  --elf build/starcraft-terran-late-storage.elf \
  --output .local/sc64/terran-late/arm-save --ticks 300
```

It holds 192 units, 96 buildings and 80 mineral fields. Its lossless 38-byte
unit save rows and 56-byte runtime units preserve all named fields, including
dead slots; the payload is 15,312 bytes. Combined target IDs use 16 bits and
visibility has exact nine-bit overlap counts. Keep its SRAM separate from
earlier profiles. These commands exercise storage and the internal Norad II
probe; they do not qualify Terran missions 6–10 or establish performance.

## Troubleshooting

- If `doctor` reports a missing required tool, install that tool and confirm its command is on `PATH`. ARM GCC also needs the matching `arm-none-eabi-objcopy` and `arm-none-eabi-size` commands. On a custom toolchain prefix, set `PREFIX` to the prefix used by all three commands.
- Cloning the public repository over HTTPS needs no GitHub login. If cloning fails, check the repository URL and network access. Pushing changes requires write access and authentication; for HTTPS, use `gh auth login` and `gh auth setup-git`, or configure a GitHub SSH key for SSH.
- If `./dev sc64 prepare` rejects the ROM, make sure it is the supported USA image and that a ZIP contains exactly one 32 MiB N64 image. The normalized SHA-1 must match the value above. The project cannot supply or identify a ROM for you.
- If an optional profile build says generated headers are missing, prepare that profile first with `./dev sc64 prepare --rom PATH --profile PROFILE`. These generated inputs are private local data and are not checked into the repository.
- If `./dev play` cannot find mGBA, install it or set `MGBA` to the executable path. For example, on Ubuntu: `MGBA=/usr/games/mgba ./dev play starcraft`.

## References and scope

Reference revisions, files consulted, and reduced-mechanics notes are in [REFERENCES.md](REFERENCES.md). License status and provenance are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Build and test coverage is summarized in [VALIDATION.md](VALIDATION.md). The engine uses bounded, fixed-size data for GBA memory. Campaign triggers, AI, unit systems, and presentation remain intentionally incomplete.

### T6 ordinary-menu qualification build

With the private T6 inputs prepared, `make sc64-terran-t6-controller` builds an
explicit six-mission development cartridge. `make
build/test-sc64-t6-campaign-run` builds its host controller runner. The ignored
trace can be replayed with `tools/sc64_terran_controller_replay.py`, supplying
that ROM and ELF through `--rom` and `--elf`; `--validate-only` checks the trace
without starting mGBA. The ordinary T6 menu route is A, RIGHT, RIGHT, DOWN, A,
A. The source-only repository deliberately excludes controller traces that
encode private map data, generated headers, ROMs, and emulator output.

The late Terran runtime adds LIFT / LAND to Terran field commands. Select a
completed liftable building, choose LIFT / LAND to lift off, use MOVE ORDER to
fly it, then choose LIFT / LAND and a clear destination to land. Airborne
buildings cannot produce units or receive worker resource returns. Accepted
landing goals are reserved against overlap and persist in generation-18
saves. This support is required by The Big Push's imported airborne opening;
it is still being exercised as part of the unqualified late campaign.
