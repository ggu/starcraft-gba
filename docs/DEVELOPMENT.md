# Development

The default StarCraft skirmish builds without ROMs, extracted assets, or external reference checkouts. The required tools are Git, GNU Make, `arm-none-eabi-gcc` with its `objcopy` and `size` tools, Clang, and Python 3.10 or newer. The project uses Python's standard library; no `pip install` step is needed. mGBA is only needed to play or run emulator workflows.

The stage-by-stage campaign sections below preserve dated implementation
history; older profile descriptions are snapshots of those stages.
For the current combined Terran profile and qualification status, see
[Combined Terran T1–T10 campaign](#combined-terran-t1t10-campaign) near the
end of this guide.

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

## T5 source import checks (historical stage)

Antigan Revolt (Terran global mission ID 8) has an import-only data path. The
private importer retains its 96×96 Badlands map and all 168 non-start UNIT
rows: 79 units, 48 buildings and 41 mineral fields. Four start markers are
excluded from the live placement rows. At this import-only stage, these checks
did not add a mission to the catalog or establish playability; later combined
campaign completion is recorded in the current summary at the end of this
guide.

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
policies. At that stage, T5 was still an experimental profile; its normal-menu
ARM win and fresh-process save check had passed. The campaign count then stood
at 11 of 30.

The import-only T5 checks cover retained rows, metadata and digests, bounded source actor
rules, and isolated SetResources arithmetic. The actor facts do not establish
cloak or mine-arming behavior, and the resource helper intentionally does not
interpret player-group selection. Those isolated fixtures do not claim T5 runtime,
mission completion, or playability.

### Combined Terran T1–T5 experimental stage (historical)

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
At that historical stage, the build was opt-in and did not change the standard
race ROMs or combined T1–T4 target. The experimental builds then had 11 of 30
missions qualified as playable; 19 remained unavailable.

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
the new save layout on the existing T5 scenario only. At that stage, T6
mission-world integration, controller completion, and performance
qualification remained future work; the campaign count was 11 of 30.

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
that earlier stage note predates the completed host-controller runs recorded
below.

## Combined Terran T1–T10 campaign

The canonical Terran profile integrates all ten campaign missions and the
existing Outpost/Blood Bath skirmish setup in one ROM.
Controller-driven host runs and normal-menu ARM replays have completed all ten
missions across the recorded frozen builds. T7, T8 and T10 have canonical-ROM
ARM wins; T9's ARM win is on the earlier frozen ROM listed below. On the
canonical ROM, T7 wins at frame 12,151 (15.76 effective fps), T8 at frame
86,596 (14.37 fps), and T10 at frame 38,491 (11.78 fps). T10 ends with Raynor
alive, the Ion Cannon destroyed and zero trigger faults. These emulator rates
are below the 30-fps target; physical-GBA qualification is not claimed.
All six fresh-process generation-18 saves for missions T5–T10 also pass on
the canonical ROM: each loads exactly at frame 123, continues to frame 183
with zero faults, and retains the mission-specific late state exactly for
T7–T10.

Prepare the Terran inputs, then build, test and play the normal-menu ROM:

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile terran
./dev sc64 build --profile terran
./dev sc64 test --profile terran
./dev sc64 play --profile terran
```

This produces `build/StarCraft-Terran-GBA.gba`, an 8 MiB ROM. To build all
three race campaigns, first prepare the shared inputs with the Terran profile,
then run `make race-campaigns test-race-campaigns`. Preparing Zerg or Protoss
alone is not sufficient for that aggregate target. The original 32 MiB N64
image, imported content and generated headers remain under ignored local
storage and must not be committed or distributed.

The integrated profile uses 192 unit slots, 96 building slots and 80 mineral
fields, with 128×128 terrain support for the largest retained maps. Its save
generation is 18, so keep its SRAM separate from earlier campaign builds.
The map-select skirmish still supports any human race and one to three
opponents whose races are chosen independently. Current adaptations include
bounded trigger execution, authored AI fallbacks and simplified presentation;
they do not reproduce the original N64 engine or every original behavior.

The canonical ROM SHA-256 is
`00f0939878a51320b8fc252d69b2947ffc442e6d3cc39efa12d0d118511bf3ee`.
T9's ARM replay used earlier frozen ROM
`329793a4707e0d3c4b33ed169391b2b887374d5eb788943ceef8828604c7ce44`; it
wins at frame 358,906 with all 43 Protoss Force2 buildings destroyed, all 21
Zerg Force3 buildings intact, Kerrigan alive, all eight trigger clocks at frame 358,906,
zero faults, and 16.94 effective fps. T6's separately qualified ARM replay
averaged 17.26 fps. All reported campaign rates are below target; physical
GBA testing remains pending.

### Repeatable Terran performance comparison

For a repeatable T10 sample, prepare and build the Terran profile, then run
`./dev sc64 test --profile terran`. This runs the private controller profile
and writes its T10 trace to the ignored local path
`.local/sc64/terran-late/full-t10-controller.json`. Replay that trace through
the normal menu using a frozen ROM and its matching ELF for each build:

```sh
python3 tools/sc64_terran_controller_replay.py \
  --trace .local/sc64/terran-late/full-t10-controller.json \
  --rom /path/to/frozen-terran.gba --elf /path/to/matching-terran.elf \
  --ticks 4000
```

This bounded prefix reports progress and performance without claiming a win.
Omit `--ticks` to run the complete trace through the mission win. Keep
immutable, matching ROM/ELF copies for the before and after builds, and record
their SHA-256 hashes, compiler/toolchain versions, replay command, and observed
mission state with each result. Compare runs only when they use the same trace,
emulator setup, and observed state/frame range, with a matching ROM/ELF pair
for each build. The replay's
effective fps uses the emulated GBA hardware timer and completed simulation
frames, not host wall time. Diagnostic counters describe only the final tick;
collect them from a diagnostic build separately from the uninstrumented FPS
comparison. `tools/sc64_compare_campaign_replays.py` is Protoss-only and does not
apply to Terran measurements.

### Zerg 4 source staging

The opt-in Z4 importer prepares the complete source for **Agent of the Swarm**
without changing the canonical race pack or enabling a playable mission. Use
the verified USA ROM and the existing pinned `references/sc64-maps` checkout:

```sh
python3 tools/sc64_z4_pack.py --rom /path/to/StarCraft64-USA.z64 \
  --out .local/sc64/z4
python3 tools/sc64_z4_pack.py --verify-staged --out .local/sc64/z4
python3 tests/test_sc64_z4_import.py
```

Packing verifies the normalized ROM, extractor revision and exact extractor
source bytes, and the authenticated Z4 CHK. The ignored output contains
`source.chk`, 16 framed source records, `manifest.json`, and `source-audit.json`.
Standalone verification checks retained files against the pinned CHK without
requiring the original ROM or running the extractor. Keep all output private.

The audit preserves all 184 placement rows, seven trigger records, source
properties, ownership, locations and other CHK sections. It reports runtime
mapping gaps and pool requirements rather than truncating source rows. Z4
still needs gameplay mappings, larger building/mineral pools, its countdown
and hatch sequence, victory/defeat behavior, persistence and controller/ARM
qualification. Source staging alone does not establish a playable mission.

### Canonical Zerg and Protoss chapter 3 builds

The standard Zerg and Protoss build targets include their first three
missions. Prepare each profile from your verified ROM, then use the standard
commands; no extra staged-profile selection is needed:

```sh
./dev sc64 prepare --rom /path/to/StarCraft64-USA.z64 --profile zerg
./dev sc64 build --profile zerg
./dev sc64 test --profile zerg
./dev sc64 play --profile zerg
```

Replace `zerg` with `protoss` for the Protoss campaign. The canonical output
pairs are `build/StarCraft-Zerg-GBA.gba` / `build/starcraft-zerg.elf` and
`build/StarCraft-Protoss-GBA.gba` / `build/starcraft-protoss.elf`. The existing
`zerg-z3` and `protoss-p3` Make targets and ROM/ELF filenames are compatibility
aliases to these outputs. Retain matching frozen ROM/ELF pairs for replay.

Both test targets run the profile's full third-mission runtime/controller
suite, normal-menu campaign startup/save fixtures, and all 234 combinations
of map, human race and independently selected AI races. The setup/save matrix
does not prove a completed match for every combination. Build guards still
enforce the same IWRAM and EWRAM reserves.

The canonical Zerg build uses `SCZ3`, save generation 5, and the existing Z3
optimization flags. Protoss uses `SCP3`, generation 7, and the P3 large-map
geometry and optimization flags. Older standard two-mission generation-2
saves are incompatible and are rejected. The asset-free default build
continues to require no imported inputs.

### Staged Zerg and Protoss chapter 4

`zerg-z4` and `protoss-p4` are internal fourth-chapter targets. The canonical
race outputs above remain the separately qualified three-chapter builds.
These targets load the complete starting rosters and numeric scenario
metadata for Agent of the Swarm and The Hunt for Tassadar, using isolated
private generated headers. Preparing later map data does not enable those
later missions in the frontend.

After the canonical profile preparation has supplied the common assets,
prepare the additional ignored inputs from the same verified source ROM:

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64 --out .local/sc64/zp-stage/generated/sc64_races_generated.h
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --out .local/sc64/zp-stage/generated/sprites --header .local/sc64/zp-stage/generated/sc64_race_sprites_generated.h
python3 tools/sc64_z4_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_z4_scenario_pack.py
python3 tools/sc64_p4_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_p4_scenario_pack.py
make zerg-z4 protoss-p4
make test-sc64-z4-runtime test-sc64-p4-runtime test-sc64-zp-storage
```

Outputs are `build/StarCraft-Zerg-Z4-GBA.gba` and
`build/StarCraft-Protoss-P4-GBA.gba`, with matching
`build/starcraft-zerg-z4.elf` and `build/starcraft-protoss-p4.elf` files.
Use the matching pairs for emulator qualification. All imported data, ROMs,
saves and graphical captures must remain local and ignored.

The staged layout uses 192 Zerg or 240 Protoss unit slots, 96 buildings,
48 mineral fields, four geysers, compact save rows and 128×128 geometry.
Its save generation is 20; older canonical saves are incompatible. Protoss
production reserves the remaining 87 source-script creations, including
queued products, instead of allowing those later actors to be dropped.
These bounds still do not fit every later campaign map; see
[the remaining-chapter audit](ZP-CAMPAIGN-IMPLEMENTATION.md).

The runtime tests use real engine/menu/update/save paths under sanitizers.
Their controlled actor deaths, position changes and protected worlds are
synthetic predicate tests. They do not establish a normal controller win or
hardware frame rate. The imported opaque AI routines use bounded authored
pressure policies; text/audio actions use documented presentation holds.
Original N64 AI, animation, abilities and trigger scheduling are not fully
reproduced. Controller and ARM qualification remain separate requirements.

The staged Zerg field menu adds **Upgrade to Lair** for a selected, idle
Hatchery and **Spire** for a selected Drone. Select the completed Hatchery or
Lair and press **R** to cycle to **Mutalisk**, then **A** to train. The Lair
requires a Spawning Pool; the Spire requires a completed Lair; Mutalisks
require a completed Spire. This makes the source-permitted air route
accessible through ordinary controls. The Lair upgrade uses the existing
construction policy: its larva timer pauses during the upgrade and completion
fills its larva count to three. Native equivalence of that timing has not been
established.

An additional P4 ability experiment is isolated from the normal P4 build:

```sh
python3 tools/sc64_p4_scenario_pack.py
make protoss-p4-storm test-sc64-p4-storm
```

This writes `build/StarCraft-Protoss-P4-Storm-GBA.gba` and its matching
`build/starcraft-protoss-p4-storm.elf`. Its generation-21 save persists four
bounded Psionic Storm effects and caster energy. In P4, select one High
Templar or Tassadar, open **B** commands, choose **STORM**, place the cursor
within cast range and press **A**; **B** cancels targeting. The row appears
only when the selected caster has enough energy. Storm affects allied units
too. Its pulse cadence and visual effect use documented authored policies;
see the frozen development-image qualification in [Validation](VALIDATION.md).

The P4 controller now has a repeatable host win. Run its full input-only
playthrough with `make test-sc64-p4-campaign`; matching-image ARM replay and
fresh-process save checks are separate. In this profile, **B+R** selects the combat escort
without Tassadar and Raynor, so army orders can clear their route first.
Ordinary **R** still selects the heroes with the army.

For later-chapter implementation, authenticate and generate the retained
numeric scenario metadata separately. For example:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z5 --verify-staged
python3 tools/sc64_zp_scenario_pack.py --mission Z5
```

The tool accepts Z5–Z10 and P5–P10 and reads the staged source-map directory
created by `sc64_races_pack.py`. Its default output is an ignored header under
`.local/sc64/zp-stage/generated/scenarios/`. It preserves numeric trigger,
location, player, property and availability records after authenticating the
complete CHK and section hashes. Generation does not implement or enable a
mission; actor support, runtime actions, save rules and controller
qualification remain required.

The isolated Z5/P5 schedulers have asset-free checks:

```sh
make test-sc64-zp-chapter-five-models
```

After generating both numeric headers, run their adapter checks with
`make test-sc64-z5-adapter test-sc64-p5-adapter`. The adapters preserve source
operands, sequence waits and switches, and reject failed world callbacks.
They still require actual engine implementations of source conditions and
effects; these fixtures do not establish playable fifth missions. Neither
model or adapter enables a frontend entry.

### Staged Zerg chapter 5 and expanded storage

After preparing the chapter-four inputs above, generate the additional Z5
numeric records and isolated sprite bank:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z5
python3 tools/sc64_z5_doodad_pack.py
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --include-z5-actors --include-z5-doodads --out .local/sc64/z5/generated/sprites --header .local/sc64/z5/generated/sc64_race_sprites_generated.h
make zerg-z5 test-sc64-z5-runtime test-sc64-z5-world
```

`build/StarCraft-Zerg-Z5-GBA.gba` and `build/starcraft-zerg-z5.elf` are an
internal five-entry Zerg build. Its complete source placements include all
41 unit-backed doors and traps; the other 30 THG2 rows are sprite-only
decoration and have no world actor. The runtime tests cover import, trigger
effects, door collision/cache changes, save/load and malformed-state
rejection. They do not prove a controller win. The source PauseGame action
requires an authored **A** acknowledgment; **B** saves while that pause is
shown. Native speech, cinematic and AI-script behavior remain approximations.

This profile uses generation 23 and an opt-in full-storage layout: 240 units,
160 buildings, 144 mineral fields and 12 geysers. Compact 48-byte runtime
buildings, 35-byte unit save rows and 5-byte mineral save rows keep the
unchanged SRAM and RAM guards. Older save generations are incompatible.
These are fixed pools, not a guarantee that unlimited repeated triggers or
production in every later mission can fit. `make sc64-zp-full-storage-probe
test-sc64-zp-full-storage` separately checks the storage layout with P4.

### Optional Z5 cloak and staged Protoss chapter 5

Z5's optional ability profile authenticates PTEC and the hero's UNIT energy
before enabling Personnel Cloaking. With the Z5 inputs above prepared, run:

```sh
python3 tools/sc64_z5_ability_pack.py --verify-staged
python3 tools/sc64_z5_ability_pack.py
make zerg-z5-cloak test-sc64-z5-cloak-runtime
```

Select Kerrigan, press **B**, and choose **CLOAK** or **UNCLOAK** in the command
list. Her HUD shows energy and cloak state. STOP preserves energy. Live,
enabled hostile detector traps reveal her within their source bounding-box
sight range; tying detector activity to the authored trap-enabled state is a
GBA policy, not verified native doodad-order behavior. Source PTEC disables
Psionic Storm here. This profile uses generation 25 and rejects older saves;
it does not establish a completed Z5 playthrough.

P5 has a separate five-entry Protoss build. After the chapter-four staging
and P4 inputs are ready, prepare its numeric descriptor and sprite bank:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission P5
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --base-only --include-zp-campaign-actors --include-z6-actors --out .local/sc64/p5/generated/sprites --header .local/sc64/p5/generated/sc64_race_sprites_generated.h
make protoss-p5 test-sc64-p5-runtime test-sc64-p5-world
```

`build/StarCraft-Protoss-P5-GBA.gba` uses save generation 24. Choosing Sides
retains every starting placement, separate hostile forces, source-player
death counters, rescue, and all 11 source creates. Production reserves slots
for unexecuted creates. The reinforcements wait for a source event: entering
one of the choice regions or killing a source-player-6 actor sets switch 0.
Bring Tassadar and two Zealots to the extraction point to satisfy the source
victory predicates. The three opaque AI script actions use a bounded authored
paid-pressure policy; native AI equivalence is unverified. Startup, source
actions, saves and render checks do not establish a normal-controller win.
The extraction terrain requires Shuttle transport, which this generation-24
profile does not enable. To test the separate generation-26 cargo profile:

```sh
make protoss-p5-shuttle test-sc64-p5-shuttle-runtime
```

Select passengers and press **A** on the friendly Shuttle to board. It has
eight space points: Probe 1, Zealot/High Templar/Tassadar 2, Dragoon 4. Select
the loaded Shuttle, press **B**, choose **UNLOAD CARGO**, then press **A** on
a clear landing area. Passengers retain HP and supply while aboard and
cannot be selected, attacked or counted in a Bring objective until unloaded.
Cargo dies if the Shuttle is destroyed. These bounded GBA orders reuse
existing unit/save fields; generation-24 saves are incompatible. Lifecycle
checks cover the transport lifecycle. A complete normal-controller P5 win, ARM replay,
and fresh-process cargo save are now qualified (see `docs/VALIDATION.md`).
Run `make test-sc64-p5-campaign-run` to repeat the host win with sanitizers.

`make test-sc64-z5-ability-models test-sc64-z6-model` requires no imported data.
After generating the Z6 numeric descriptor with the scenario packer,
`make test-sc64-z6-adapter` checks its isolated callback scheduler. Z6 is not
an enabled runtime mission.

`make test-sc64-z6-world` checks its engine-backed world callback foundation,
including selectors, locations, resources, both MoveLocation targets,
KillUnitAtLocation and source properties. Its mutable location state still
needs save integration. Source hallucination, energy and opaque AI actions
require explicit integration callbacks and fail closed when absent; this
fixture does not establish a playable Z6 mission.

### Z5 Consume and remaining ability models

The optional Z5 Consume profile shares Kerrigan's source-checked Q8 energy and
cloak state. Prepare the existing Z5 inputs, regenerate the authenticated
ability facts, then build and test:

```sh
python3 tools/sc64_z5_ability_pack.py
make zerg-z5-consume test-sc64-z5-consume-runtime
```

Select Kerrigan, press **B**, choose **CONSUME** after the cloak command, then
press **A** on a friendly Zergling. She approaches using ordinary movement and
consumes it for 50 energy, capped at 250, with no energy cost. Hallucinated food
is killed without giving energy. This adapter supports the authenticated Z5
human target roster (source-owner-3 raw37 Zerglings); the standalone model
records the broader native target filters without inventing an organic-only
restriction. Native spell animation, projectile delay and inertia are not
emulated. The range uses native collision-box geometry at the GBA's 2:1 scale
with zero movement allowance.

`build/StarCraft-Zerg-Z5-Consume-GBA.gba` has distinct identity SCZD and save
generation 27, rejecting earlier generations. Pending Consume orders preserve
validated caster/target identities through SRAM; invalid links reject before
world mutation or SRAM replacement. The unused circle fallback is kept in ROM
for this profile so the unchanged IWRAM stack reserve remains intact.

The source audit also authenticates Ensnare permission. Its standalone model
covers friendly-fire area filters, energy cost and the 75-status-decrement
lifetime (600 native frames, nominally 750 GBA updates). Ensnare is **not yet
integrated into the playable runtime**. Ability fixtures and startup/save
coverage do not establish a complete Z5 controller win.

### P6 numeric foundation

P6 has isolated source-program and actor-fact checks:

```sh
make test-sc64-p6-model test-sc64-p6-actors
python3 tools/sc64_zp_scenario_pack.py --mission P6
make test-sc64-p6-adapter test-sc64-p6-world
```

The numeric adapter validates 26 one-shot records, 99 actions, 29 conditions,
source selectors, waits, switches and the referenced UPRP identities. The isolated world adapter tests all source rows with synthetic callbacks. World
operations still require live engine bindings and explicit capabilities; the opaque AI row fails closed
without an authored policy. P6 is not yet a selectable playable mission. Its
remaining runtime work includes property-driven enemy orders, permanent-cloak
detection for Dark Templar/Zeratul, Infested Terran suicide damage, THG2 props,
and full normal-input/ARM/save qualification. Source door types 205–208 are
invincible non-buildings; placement in the GBA Building pool is a prop policy.


The opt-in P6 sprite importer prepares the missing actor and wall-trap art
without changing earlier sprite banks:

```sh
python3 tools/sc64_race_sprites.py --rom /path/to/StarCraft64-USA.z64 --include-p6-actors --include-z5-actors --include-z6-actors --out .local/sc64/p6/generated/sprites --header .local/sc64/p6/generated/sc64_race_sprites_generated.h
python3 tools/sc64_p6_doodad_pack.py --verify-staged
```

Keep T4 imports enabled (omit `--base-only`) for the inherited props. The P6
bundle reuses ZP beacons and Z5 indexed doors/traps and adds raw50/74/75 unit
images plus all seven raw210 wall-missile-trap frames. This prepares art only;
new raw actor behavior and the raw210 prop policy remain unimplemented. The
THG2 audit preserves all 37 rows: 27 unit-backed props and ten sprite-only
rows. The sprite-only type IDs are SpriteType IDs, not unit IDs. The two raw210
unit-backed placements explicitly prevent runtime-ready prop generation until
their mapping is implemented. `test-sc64-p6-actor-source` audits source UPRP
values separately from the asset-free actor-fact checks.


### Z5 complete controller and shared query checks

The generation-27 Consume build now has a full ordinary-input host win, a
complete ARM replay, and an exact fresh-process mid-mission save restore.
Repeat the host run with `make test-sc64-z5-campaign-run`. It launches through
HOME, retains Kerrigan and uses movement, cloak and Consume through the normal
controls; victory follows the source gate and extraction program.

`make test-sc64-z5-count-queries` checks the optional started-trigger pruning
and phase-local Command/Bring snapshot against their full-query oracles. The
three preserved source triggers continue to poll; copied or unsupported rows
fall back to full queries. Neither optimization changes SRAM. See Validation
for frozen replay comparisons and measured performance.

### P6 internal live probe

After preparing the P6 sprite and numeric THG2 inputs above, run:

```sh
make protoss-p6-probe
make test-sc64-p6-game test-sc64-p6-runtime test-sc64-p6-save
make test-sc64-p6-combat test-sc64-p6-profile test-sc64-p6-frame-coverage
```

`build/StarCraft-Protoss-P6-Probe-GBA.gba` has an internal sixth menu entry and
separate generation-30 identity SCPC. It imports 200 units, four beacons and
27 unit-backed THG2 props. The source 26-record program drives gates, creates
and the Zeratul extraction objective. The bounded GBA actor policy includes
permanent-cloak detection, property-driven burrow/release, Infested Terran
suicide splash and raw210 trap attacks. Ten sprite-only decorations remain
metadata rather than rendered actors. The opaque Suic script is an authored
single AttackMove for source Force 2; native AI, animation timing and full
combat equivalence are not claimed.

The probe passes live startup, candidate-only save validation, combat and
framebuffer checks, including source door changes. These checks do not yet
establish a complete controller win, ARM mission replay or hardware viability.
The standard Protoss output remains the separately qualified P1–P3 build.


The Consume build enables the qualified Z5 query options by default. For a
comparison build, use `make -B zerg-z5-consume Z5_QUERY_FLAGS=` and retain that
ROM/ELF separately, then restore with `make -B zerg-z5-consume`. The complete
optimized replay averages 17.73 effective fps versus 8.78 for the baseline,
with matching observed state/captures and exact fresh-process save restore.


### Z6 live probe and seventh-chapter foundations

After the staged map and Z5 inputs are prepared, import Z6's complete optional
actor/building bank and build the internal sixth-entry ROM:

```sh
python3 tools/sc64_race_sprites.py --rom "/path/to/StarCraft 64.z64" --base-only --include-zp-campaign-actors --include-z5-actors --include-z5-doodads --include-z6-actors --include-z6-buildings --out .local/sc64/z6/generated/sprites --header .local/sc64/z6/generated/sc64_race_sprites_generated.h
python3 tools/sc64_zp_scenario_pack.py --mission Z6
make zerg-z6-probe
make test-sc64-z6-world test-sc64-z6-runtime test-sc64-z6-save
make test-sc64-z6-profile test-sc64-z6-frame-coverage
```

This produces `build/StarCraft-Zerg-Z6-Probe-GBA.gba` (SCZE, generation 32).
It retains the complete 185-row source placement, adds distinct Robotics
Support Bay and Shield Battery identities, and runs Z6's source trigger
program. Hallucinated Tassadar actors have a bounded lifetime, cannot deal
damage, and take doubled damage before armor and shields. The Zer6 script
uses an explicit paid-production/order policy rather than native AI bytecode.
Kerrigan now starts at the authored full energy and has Cloak and Storm in
her field menu. Her source disables Consume. Ensnare and active Shield
Battery recharge remain incomplete. Do not count this as a completed sixth
mission.

Z7 and P7 source schedulers can be checked independently:

```sh
python3 tools/sc64_zp_scenario_pack.py --mission Z7
python3 tools/sc64_zp_scenario_pack.py --mission P7
make test-sc64-z7-model test-sc64-z7-adapter test-sc64-z7-runtime
make test-sc64-p7-model test-sc64-p7-adapter
```

Z7's world/runtime fixture uses all 231 canonical rows and a bounded paid
Zerg AI policy. Its opt-in shared-engine profile now includes a seventh menu
entry, source objective outcomes and generation-34 saves:

```sh
make zerg-z7-probe
make test-sc64-z7-profile test-sc64-z7-game-save test-sc64-z7-frame-coverage test-sc64-z7-colonies
```

This produces `build/StarCraft-Zerg-Z7-Probe-GBA.gba` (SCZF). It requires the
complete Z6 inputs above and the Z7 scenario descriptor. It retains the source
starting roster and resources; build a base and destroy the hostile source
owner's buildings while preserving at least one human actor. Menu/startup,
save rejection and render checks establish integration readiness, not a
completed playthrough or full-mission performance result. Its save generation
differs from the Z6 probe and earlier generation-33 Z7 probe; older saves are
rejected rather than converted. With only Drones selected, scroll below SPIRE
to CREEP COLONY and place a 75-mineral colony on creep. After it completes,
select it and choose SUNKEN from the contextual row. A completed Spawning
Pool and 50 minerals are required; this morph consumes no additional Drone.
It preserves damage through the 375-tick morph and applies the reference's
max-HP adjustment at completion. The explicit colony-defense gate is enabled
only in the Z7 development profile.

P7 has an isolated world foundation with separate source-Force wallets; its
canonical roster fails closed on missing raw68/71/72 actor bindings. It still
lacks shared menu/save integration. Neither seventh mission has a completed
controller playthrough.


The sixth-mission probes require their authenticated spell metadata:

```sh
python3 tools/sc64_p6_ability_pack.py
python3 tools/sc64_z6_ability_pack.py
make test-sc64-campaign-spells test-sc64-p6-storm
make test-sc64-six-query-options test-sc64-p7-runtime
```

P6's Tassadar has Storm at `B` → `STORM`; select him alone before casting.
Z6's Kerrigan has `CLOAK`/`UNCLOAK` and `STORM` in the field menu. Stop retains
spell energy. Active effects and energy survive SRAM restore; the new probes
reject their older generation-28/29 saves. P6 uses generation 30 and Z6 uses
32, while the separately qualified P5/Z5 outputs keep their existing formats.

Optional query experiments remain off by default. Compare a frozen baseline
with `make -B protoss-p6-probe P6_QUERY_FLAGS='-DSC64_P6_PRUNE_STARTED -DSC64_P6_PREVALIDATED_TICK'`.
The analogous Z6 experiment uses `Z6_QUERY_FLAGS=-DSC64_Z6_PRUNE_STARTED`.
An additional default-off Z6 count experiment is available after preparing
the private numeric scenario inputs:

```sh
make test-sc64-z6-count-queries
make -B zerg-z6-probe Z6_QUERY_FLAGS=-DSC64_Z6_COUNT_SNAPSHOT
```

It caches the eight authenticated Command/Bring counts for one condition
phase, with full-scan fallback and invalidation before actions. It retains
the current center-based location predicate; source bounding-box/elevation
fidelity remains a separate correction. Host equivalence and a full host win
are established; ARM timing and replay equivalence remain pending. Restore
the standard build with `make -B zerg-z6-probe` after retaining any experiment.
Restore a default build with `make -B protoss-p6-probe` or
`make -B zerg-z6-probe`; Make does not track command-line flag changes.
`tools/sc64_zp_start_profile.py --help` describes a bounded P6 startup
comparison driven by ordinary menu inputs. It does not qualify mission wins.

### Z6 transport integration and P6 query experiments

The internal Z6 probe now requires authenticated Ventral Sacs facts:

```sh
python3 tools/sc64_z6_transport_pack.py
make test-sc64-z6-transport test-sc64-z6-save test-sc64-campaign-spells
make zerg-z6-probe
```

Z6's objective island is disconnected from the opening ground component in
native tile walkability. Morph the starting Hatchery to a Lair, select the
completed Lair, then use `B` → `VENTRAL SACS`. Research costs 200 minerals and
200 gas and takes 3,000 active 30-Hz updates; pausing freezes it. It begins
unresearched. Once complete, select ground passengers and press `A` on a
friendly Overlord to board. Select the loaded Overlord and use `B` →
`UNLOAD CARGO`, then `A` on traversable ground. Capacity is eight source space
points, including one for Kerrigan and two for each Hydralisk. Research,
embarked actors and source identities survive saves. Destroying the carrier
kills its passengers and records their source deaths. Boarding and landing
remain bounded GBA adaptations of the native order machine.

The Z6 save format is generation 32. It also persists the bounded Drone-morph
census used to distinguish construction from killed starting Drones, and
accepts the imported Hatchery's in-place Lair morph. Older internal Z6 saves
are intentionally rejected; the qualified standalone Z5 output is unchanged.

P6 and Z6 heroes can enter Storm targeting with `B+R` when selected alone.
Insufficient energy keeps that selection and reports failure. Successful casts
stop the caster's previous movement order. P6 Storm also damages native
NonBuilding traps held in the engine's Building pool.

The optional P6 query experiments include conservative trap range rejection,
one-shot trigger polling removal, preflight-backed immutable metadata checks,
and a transient Command/Bring snapshot. The snapshot groups repeated source
selectors and has room for every condition's query rectangle. It is built
before predicates and discarded before any action or opaque AI callback;
saves still use full candidate validation.

```sh
make test-sc64-p6-query-experiments
make -B protoss-p6-probe P6_QUERY_FLAGS='-DSC64_P6_PRUNE_STARTED -DSC64_P6_PREVALIDATED_TICK -DSC64_P6_TRAP_CULL -DSC64_P6_COUNT_SNAPSHOT'
```

These flags remain off by default while full playthrough comparisons are in
progress. The sixth missions remain internal probes until normal-input wins,
fresh emulator replay, save continuity and performance are qualified. The
completed staged campaign count remains Terran 10, Zerg 5 and Protoss 5.


Both sixth probes now support the bounded startup observer. Z6 also has a
full-bank replay observer, ready for a genuinely winning controller trace:

```sh
python3 tools/sc64_zp_start_profile.py --profile zerg-z6 --rom build/StarCraft-Zerg-Z6-Probe-GBA.gba --elf build/starcraft-zerg-z6-probe.elf --live-frames 1000 --checkpoint 100
```

The observer reads model, source deaths, mutable locations, ability/research
state, Drone morph history and complete compact actor banks through the
matching ELF's DWARF offsets. `--profile protoss-p6` remains the default.

With the Z6 private inputs prepared, `make test-sc64-z6-controller` runs the
full normal-input mainland-clearance and Ventral Sacs ferry playthrough under
ASan/UBSan. It writes the private replay trace to `build/z6-controller.json`.
The controller also accepts `--trace PATH` when invoked directly. A host win
does not qualify the ARM replay, SRAM handoff or physical GBA performance.

Two further P6 experiments remain default-off: `SC64_P6_RESCUE_GROUPED`
groups live friendly rescue candidates while preserving in-frame rescue
chains; `SC64_P6_STORM_POSTCHECK_LOCAL` skips the unchanged mission-model
post-validation after a successful Storm tick. It retains full entry checks
and the live raw-79 energy scan. Run their private-data host checks with
`make test-sc64-p6-rescue-grouped test-sc64-p6-storm-validation`.
Append either flag to an explicit `P6_QUERY_FLAGS` experiment and use `-B`
when changing flags. Neither option has a qualified full-mission FPS result.
Startup reports are explicitly separate from mission-win evidence.

In the Z6 probe, **R** selects the combat group including Kerrigan. **B+R**
selects the army while leaving Kerrigan out, so group orders can keep the
mission-critical hero safe. With only Kerrigan selected, **B+R** instead
opens Storm targeting; an exhausted Kerrigan keeps her selection. This is
an authored GBA control, not an original N64 binding.

P6 burrow-release options are also default-off experiments. To run their
four-combination differential checks after preparing the private probe inputs:

```sh
make test-sc64-p6-burrow-cull
```

Add `-DSC64_P6_BURROW_CULL -DSC64_P6_BURROW_GROUPED` to the existing
`P6_QUERY_FLAGS` experiment to compare the grouped candidate. Retain separate
ROM/ELF pairs and use the startup profiler's `--compare` option. These flags
are not enabled in a qualified full-mission build yet; current evidence and
frozen hashes are in [Validation](VALIDATION.md#p6-burrow-release-experiments).

### Complete skirmish controller matches

After preparing the three canonical race profiles, run:

```sh
make test-sc64-skirmish-matches
```

This compiles an input-only controller against each canonical profile and runs
both available maps, all three human races and one to three AI opponents.
Opponent races rotate from the human race; this is 18 configurations per ROM,
not every possible opponent-race permutation. The matrix allows 120,000 input
ticks and fails on an unfinished match, launch failure or sanitizer error.
Human defeats are valid completed matches. Private traces, logs and matrix
reports are written under `.local/sc64/skirmish/{terran,zerg,protoss}-matches/`.
To run one case directly:

```sh
build/test-sc64-skirmish-match-protoss 1 0 2 .local/sc64/skirmish/example.json 120000
```

The arguments are map (0 built-in arena, 1 imported), human race (0 Terran,
1 Zerg, 2 Protoss), opponent count, output trace and optional input-tick cap.
The controller uses ordinary HOME/menu navigation and `game_update` inputs;
its world reads guide decisions without modifying resources, actors or AI.

### Dense input replay without a larger queue

Some full controller routes exceed 256 transitions in a 1,000-tick window.
`SC64_DENSE_SCHEDULE` reduces replay pages to 250 ticks while retaining the
existing 256-entry queue and RAM layout. It changes the debugger observation
cadence; ordinary play continues to use the same game rules. For the P6 probe:

```sh
make -B protoss-p6-probe P6_QUERY_FLAGS=-DSC64_DENSE_SCHEDULE
python3 tools/sc64_zp_controller_replay.py --help
```

The Z3 and staged Zerg/Protoss replay tools discover the shorter period from
the ELF. Other existing profilers still expect their documented 1,000-tick
builds. Rebuild with the original flags before using those profilers.
For a dense skirmish ROM/ELF, use the dedicated observer:

```sh
python3 tools/sc64_skirmish_controller_replay.py \
  --trace .local/sc64/skirmish/example.json \
  --rom /absolute/path/to/matching.gba \
  --elf /absolute/path/to/matching.elf
```

Supply the matching build used to generate the trace; profile-specific actor
capacities can change the outcome. The tool verifies a fresh HOME start,
feeds only scheduler input fields, and checks the final configuration, frame,
result and live actor counts. Its FPS mean uses GBA timer deltas between live
PLAY checkpoints, excluding HOME/start and final RESULT intervals.

### Prepare the qualified staged Zerg and Protoss builds

The wrapper also supports the six-mission Zerg development profile and the
five-mission Protoss Shuttle profile. From a fresh checkout, use your supported
StarCraft 64 USA ROM:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft64-USA.z64" --profile zerg-z6
./dev sc64 build --profile zerg-z6
./dev sc64 test --profile zerg-z6
./dev sc64 prepare --rom "/path/to/StarCraft64-USA.z64" --profile protoss-p5-shuttle
./dev sc64 build --profile protoss-p5-shuttle
./dev sc64 test --profile protoss-p5-shuttle
```

These generate the required maps, art, scenario, ability and transport headers
under `.local/`; Z6 also generates its inherited Z5 sprite bank. Inspect
readiness with `./dev sc64 status --profile zerg-z6` or the corresponding
Protoss profile. The output ROMs are `build/StarCraft-Zerg-Z6-Probe-GBA.gba`
and `build/StarCraft-Protoss-P5-Shuttle-GBA.gba`. The ordinary `zerg` and
`protoss` profile names still build their canonical three-mission versions.

### Reproduce the P6 host win

After preparing the P6 probe inputs described above, run:

```sh
make test-sc64-p6-controller
```

The host test replays an authored opening input fixture through the ordinary
HOME/menu controls, then uses the adaptive Marine/hero escort policy. The
fixture contains only held-key masks and run lengths; game assets still come
from local preparation. No private opening input file is required. The default
180,000-gameplay-frame limit covers the full win. To write a separate trace:

```sh
build/test-sc64-p6-controller --trace .local/sc64/p6-my-run.json
```

Full trace replay on ARM requires a dense probe build because some input
windows exceed the standard queue's 1,000-tick capacity; see the dense replay
section. This target is a deterministic regression route, with exact source
hero/extraction/completion assertions at victory.
