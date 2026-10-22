# Fixed-race campaign builds

The Terran, Zerg, and Protoss profiles build three separate GBA ROMs from the same game engine. Each ROM offers one race's campaign and currently contains exactly its first two playable missions. The mission catalog records all 30 original campaign slots, but the remaining eight missions for each race are staged metadata only; they are not playable yet.

These are source-only profiles. The repository contains the engine, menu code, conversion tools, and authored Outpost map, but it does not contain a StarCraft 64 ROM or converted proprietary maps and graphics. To prepare a profile, supply a supported USA ROM that you are entitled to use:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile terran
./dev sc64 build --profile terran
./dev sc64 test --profile terran
./dev sc64 play --profile terran
```

Use `zerg` or `protoss` in place of `terran` for the other builds. `prepare` accepts a `.z64`, `.v64`, or `.n64` image, or a ZIP containing exactly one supported image. It verifies the image and uses the pinned `sc64-maps` extractor; the first preparation requires Git and network access to fetch that tool. `status` checks preparation and build outputs. Build and test do not need an emulator; `play` requires mGBA. See [Development](DEVELOPMENT.md) for tool requirements and [SC64 development notes](SC64.md) for import details.

The ROMs are `build/StarCraft-Terran-GBA.gba`, `build/StarCraft-Zerg-GBA.gba`, and `build/StarCraft-Protoss-GBA.gba`. Each profile has its own save identity, so its saves do not load as another race's campaign. Extracted inputs and generated headers stay under ignored `.local/sc64/`; ROMs, ELF files, and test outputs stay under ignored `build/`. Do not commit or distribute the supplied ROM, imported map or art data, generated headers, saves, or captures containing imported game graphics.

After preparing any one of these profiles, build and test all three with
`make race-campaigns test-race-campaigns`. Their shared preparation generates
all required local inputs. The asset-free `./dev test` also exercises all three
frontend variants without requiring those inputs.

## Campaign progress

Each fixed-race campaign presents only its two implemented missions and advances from the first to the second after a win. The catalog reserves identifiers for ten missions per race so future stages can add missions without renumbering the original six shared campaign entries. That catalog does not make later missions available: the current builds do not have their scenario content and implementations. The full 30-mission campaign remains unfinished.

Terran 3, Desperate Alliance, has an experimental runtime behind
`SC64_T3_ENABLED`. After preparing a race profile, build and check it with:

```sh
make terran-t3 test-sc64-t3-runtime
```

This produces `build/StarCraft-Terran-T3-GBA.gba`. Its campaign menu has
Desperate Alliance on page two. It imports all 138 starting units, 31 buildings,
25 mineral fields and two geysers into a 160-unit engine. Keep a Terran building
alive for the 30-minute countdown. Killing a Zerg building with at least two
minutes remaining unlocks an alternative win by destroying the Hatchery.

The enemy uses imported guards, bounded paid production, periodic small
attacks and a final assault. This is an authored approximation of the original
AI scripts; trigger-created extraction units are not implemented. Sixteen unit
slots are reserved from enemy production for the player. The new save layout
rejects older saves; use a separate save file for the experimental ROM.

A controller-only host run completes the full survival objective. The
production Terran build still contains its first two missions while ARM
performance and gameplay checks continue. The final assault still needs
performance work; see the measured builds in [Validation](VALIDATION.md).
The full 30-mission campaign remains in progress.

Zerg 3, The New Dominion, also has an experimental build:

```sh
make zerg-z3 test-sc64-z3-runtime
```

This target enables three-bit route fields and exact row-span sight updates.
The extra route slots retain more destinations within the same field-memory
budget. Sight updates preserve the existing circles and change only cells
entering or leaving them. These optimizations are currently enabled for Z3.
For a comparison build, use `make -B zerg-z3 Z3_OPTIMIZATION_FLAGS=`; rebuild
with plain `make -B zerg-z3` to restore the optimized version. Retain each
ROM/ELF pair separately before comparing runs.

`build/StarCraft-Zerg-Z3-GBA.gba` adds New Dominion on page two of the Zerg
campaign. Protect the Mature Chrysalis and eliminate the Terran presence.
The source victory gates are at most six remaining enemy units/buildings
combined, or no enemy buildings. Losing the Chrysalis starts defeat.
All 58 units, 33 buildings, 28 mineral fields and three geysers are retained.
The Mature Chrysalis is a stationary structure with 250 HP and one armor;
the Young Chrysalis in the first two missions remains a separate powerup.
The 128-unit engine reserves sixteen slots from enemy training for the player.

The controller-only host test completes the mission by building an Extractor
and Hydralisk Den, assigning gas workers and combining forward defense with
attacks on enemy structures. It wins with the Chrysalis intact. This is a
host gameplay regression; ARM full-mission timing and physical GBA validation
remain separate checks.

The optimized ARM replay also wins and matches the host's final state. It
averages 19.19 fps during counted play, up from 16.69 fps before these changes.
Its worst complete observed interval is 15.23 fps. Reported gameplay state
matches the earlier run at all 73 checkpoints. The mission stays experimental
while that performance gap is addressed; physical GBA testing remains pending.

To replay that host test's complete input trace on the current experimental
ARM build, run this after the build/test command above:

```sh
python3 tools/sc64_z3_controller_replay.py
```

The helper validates the current proof trace and derives the controller queue
capacity from the ELF. It starts from a fresh HOME screen, schedules inputs
and checks the final mission, roster, resources and objective state. Reports,
saves and captures stay local in ignored output directories. Use `--rom`,
`--elf`, `--trace` and `--out` to retain a frozen build and separate runs.

For a bounded performance checkpoint, add `--ticks 59000 --save-at-end`.
The helper stops that controller trace during play, pauses with START and
saves with B. Its report records the observed and saved frames and SRAM hash.
Prefixes are labeled explicitly and do not assert a complete mission win.

The experimental Z3 build uses `SC64_Z3_WINDOWED_SCHEDULE` to reduce its
debugger input buffer from 2,048 to 256 entries. The helper detects this queue
from ELF symbols and refills it at the existing
1,000-tick observer stops. Events retain absolute ticks; held buttons and
press edges carry across empty windows. It checks each page's capacity before
launch and verifies consumption before refill. The complete proof trace has
1,388 transitions and a peak of 49 per window. This option frees 14,328 bytes
of BSS after bookkeeping, preserves legacy count-only scheduling for simple
profilers, and has no input effect during ordinary play. Standard builds keep
their existing queues. Use a current replay helper with a windowed ELF.

Enemy pressure uses paid Marines every two minutes and small attacks from
the imported guards every three minutes. This is an authored substitute for
the source AI script. Result actions use explicit presentation timing policies:
30/15-tick waits, 90-tick transmission/text holds and a 13-tick loss-audio
placeholder. Original dialogue/audio and Dropship transport are absent.
The experimental save identity rejects older Zerg saves and retains the
pending action cursor and timer. Standard Zerg builds still enable only
their first two missions. See [Validation](VALIDATION.md) for what has been
tested and what remains pending.

After local preparation, `make test-sc64-t3-import` checks every retained T3
record, its runtime type mapping, coordinate bounds and terrain indices. It
does not launch the mission. The asset-free `./dev test` checks the standalone
timer, alternate objective, defeat policy and invalid-state handling.

The engine and its gameplay limits are shared across all three ROMs. These profiles select a campaign identity at build time; they are not separate ports of the original N64 game. The current engine is a bounded GBA adaptation with incomplete campaign triggers, enemy strategy, unit systems, and presentation. It does not interpret arbitrary scenario triggers or execute the original AI scripts. See [SC64 unified profile](SC64-UNIFIED.md) for more detail on shared mechanics and limitations.

## Custom skirmishes

All three builds include the same map-select skirmish setup and the same two maps:

| Map | Source and adaptation |
| --- | --- |
| Outpost | Project-authored map. |
| Blood Bath | Imported from the supplied ROM. |

The human player can choose Terran, Zerg, or Protoss independently of the campaign race used by that ROM. Choose one, two, or three computer opponents, then set each opponent's race independently. The engine and available gameplay mechanics remain shared among the profiles.

Imported skirmish data is fitted to the engine's bounded resource storage. The packer selects at most 32 distinct mineral patches, distributing the nearest available patches among the selected starts, with a target of up to eight per start. It validates map connectivity, source geyser placements, and resource-building footprints. The imported map keeps its source terrain and resource locations.

## Current limits

Higher Ground (Protoss 3, global catalog ID 22) is in preparation. The private
import preserves its 128×96 terrain, all 96 units, 42 buildings, 48 mineral
fields and three geysers; three start markers are excluded from live entities.
Both source computer-owner slots remain available for separate force handling.
`make test-sc64-p3-import` validates these records without making the mission
playable. `make test-sc64-p3-scourge` checks Scourge's air-only suicide attack
through the real combat and attack-move paths. That weapon is gated to the
P3 profile; independent force AI, campaign integration and saveable result
actions are still required before Higher Ground becomes playable.

`make test-sc64-large-map sc64-large-probe` checks the opt-in
`SC64_LARGE_MAPS` storage layout, including packed fog/path grids, larger
target indexing and 48 mineral slots. The ARM probe links the existing Protoss
missions with this layout; it does not enable Higher Ground. The synthetic
save check covers the maximum fog buffer and high resource slots, not a
128-column catalog mission's complete load/resume behavior. Standard build
flags and save layouts remain unchanged.

- Standard builds enable two campaign missions per race. Experimental Terran 3 is described above; the other later missions still need scenario and gameplay implementation.
- Skirmishes offer Outpost and Blood Bath, with one to three independently-raced opponents. They exercise the shared reduced engine rather than the original N64 engine or complete scenario scripting.
- Imported data must be generated locally from the user's own supported ROM. The source repository alone cannot build these three profiles until that preparation has been done.
- Emulated and physical-hardware performance must be measured separately. This guide makes no frame-rate guarantee, and physical GBA validation remains a separate step.
