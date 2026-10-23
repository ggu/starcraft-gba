# Fixed-race campaign builds

The standard Terran, Zerg and Protoss targets remain separate ROMs. The
integrated Terran runtime now has controller-driven host wins for
all ten Terran missions. Full ARM replay evidence is available for Terran
1–6 and 10; the full replays for Terran 7–9 are still pending. The current
full-profile ARM T10 result is below the 30-fps target, and no physical GBA
result is claimed. The canonical `terran` profile includes all ten Terran
missions; Zerg and Protoss profiles each include their first three missions.
The catalog records all 30 mission identifiers.

The stage sections below preserve dated implementation history. The
cumulative T1–T10 status and current limits near the end of this guide are the
current summary.

These are source-only profiles. The repository contains the engine, menu code, conversion tools, and authored Outpost map, but it does not contain a StarCraft 64 ROM or converted proprietary maps and graphics. To prepare a profile, supply a supported USA ROM that you are entitled to use:

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile terran
./dev sc64 build --profile terran
./dev sc64 test --profile terran
./dev sc64 play --profile terran
```

`make terran` builds the integrated campaign ROM; `make test-terran` runs its
campaign startup/save checks and the Outpost/Blood Bath race-and-AI matrix.

Use `zerg` or `protoss` in place of `terran` for the other builds. `prepare` accepts a `.z64`, `.v64`, or `.n64` image, or a ZIP containing exactly one supported image. It verifies the image and uses the pinned `sc64-maps` extractor; the first preparation requires Git and network access to fetch that tool. `status` checks preparation and build outputs. Build and test do not need an emulator; `play` requires mGBA. See [Development](DEVELOPMENT.md) for tool requirements and [SC64 development notes](SC64.md) for import details.

The ROMs are `build/StarCraft-Terran-GBA.gba`, `build/StarCraft-Zerg-GBA.gba`, and `build/StarCraft-Protoss-GBA.gba`. Each profile has its own save identity, so its saves do not load as another race's campaign. Extracted inputs and generated headers stay under ignored `.local/sc64/`; ROMs, ELF files, and test outputs stay under ignored `build/`. Do not commit or distribute the supplied ROM, imported map or art data, generated headers, saves, or captures containing imported game graphics.

To build and test all three race campaigns, first prepare the shared inputs
with `./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile terran`, then run
`make race-campaigns test-race-campaigns`. Preparing Zerg or Protoss alone
does not generate the complete shared input set. The asset-free `./dev test`
also exercises all three frontend variants without requiring those inputs.

## Campaign progress

The Terran profile presents ten missions; Zerg and Protoss present their first three missions and advance after a win. Controller-driven host runs complete 16 of the 30 missions: Terran 1–10, Zerg 1–3 and Protoss 1–3. Terran 7–9 still need full ARM replay qualification, and the other 14 missions have no completed runtime.

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

A controller-only host run completes the full survival objective. At this
earlier implementation stage, the `terran` build contained its first two
missions. The combined T4 target described below added this third mission and
Jacobs Installation to the normal campaign flow. Performance remains below the
30-fps target; see the measured builds in [Validation](VALIDATION.md).

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

The separate internal Jacobs Installation replay target remains available for
development. After preparing its local inputs as described in
[Development](DEVELOPMENT.md), use `make sc64-t4-replay`. On HOME,
START+SELECT opens its briefing; A deploys. This internal target uses save
generation 10 and remains distinct from both the standard builds and the
combined campaign target below. It is a bounded adaptation with the
compatibility limits in [Validation](VALIDATION.md).

## Combined Terran T1–T4 experiment

After preparing the local Terran inputs, build the combined campaign ROM with:

```sh
make terran-t4
```

This creates `build/StarCraft-Terran-T4-GBA.gba`. It exposes Terran missions
1–4 through the normal campaign menu: HOME → A → mission list, RIGHT to the
second page, DOWN to Jacobs Installation, A for its briefing, then A to deploy.
Terran 3 and Terran 4 share the same campaign progression and saved completion
state. The standard Terran ROM remains unchanged, while `sc64-t4-replay`
continues to use its separate internal START+SELECT entry.

The combined ROM uses save generation 11 and rejects saves from earlier
generations, including generation 10 from the internal T4 target. Keep separate
save files for these builds. Its controller-only host and ARM runs complete
Jacobs Installation through the ordinary menu. The earlier `181a5063` emulator replay averaged
23.15 effective fps over 7,971 counted live frames, with a worst counted
interval of 20.71 fps; this is below the 30-fps target and is not physical GBA
timing. See [Validation](VALIDATION.md) for the ROM/ELF identities and other
reproducible measurements. This T1–T4 build excludes later Terran missions;
the separate T1–T5 target is described below.

The current combined default enables `SC64_UNARMED_QUERY_SKIP`. Its frozen
2 MiB ROM SHA-256 is
`d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`; its
matching ELF SHA-256 is
`3788c697cafe3f5693ee9b752013a6dee9cdc9a1c6673a90d838d920985d1202`. Its
controller win, 11 gameplay checkpoints and three captures match the earlier
combined build byte-for-byte for reported state and images. It averages
23.53 effective fps across 7,971 counted frames, with a worst interval of
21.17 fps. This is emulator performance below the 30-fps target, not physical
GBA evidence. The earlier 29.48-fps skirmish measurement belongs to the
`181a5063` ROM and is not a measurement of this current build.

## Custom skirmishes

The standard fixed-race ROMs and combined Terran T4 ROM include the same
map-select skirmish setup and the same two maps:

| Map | Source and adaptation |
| --- | --- |
| Outpost | Project-authored map. |
| Blood Bath | Imported from the supplied ROM. |

The human player can choose Terran, Zerg, or Protoss independently of the campaign race used by that ROM. Choose one, two, or three computer opponents, then set each opponent's race independently. The engine and available gameplay mechanics remain shared among the profiles.

On the current frozen combined ROM `d8678867`, a 10,000-tick neutral Blood
Bath run with a Terran human and three independently selected Zerg, Protoss
and Terran opponents averaged 29.49 effective fps in mGBA; observed intervals
ranged from 28.58 to 29.86 fps. Every observed checkpoint remained in play.
This bounded sample does not establish full-battle or campaign performance.

Imported skirmish data is fitted to the engine's bounded resource storage. The packer selects at most 32 distinct mineral patches, distributing the nearest available patches among the selected starts, with a target of up to eight per start. It validates map connectivity, source geyser placements, and resource-building footprints. The imported map keeps its source terrain and resource locations.

## T5 internal runtime stage

Antigan Revolt is reserved at Terran global mission ID 8. Its import path
retains the full 96×96 Badlands terrain and 168 non-start UNIT rows: 79 units,
48 buildings and 41 mineral fields. Four start markers are not emitted as live
placements. An internal runtime now binds the retained trigger program to
bounded world queries/actions, rescue conversion, paid AI policies and mine
combat. Sanitizer fixtures cover region-gated resource grants, the rebel-base
sequence, and original victory/defeat predicates through direct world setup.
These are engine checks, not ordinary-controller completion evidence.

At that stage, the standard profiles and combined T1–T4 ROM excluded T5. The
combined T1–T5 build then added a normal-menu entry and passed its controller
win and fresh-process generation-14 save check. The current integrated profile
and qualification status are summarized below.

## Combined Terran T1–T5 experimental stage (historical)

After preparing the local Terran inputs, build and run the focused host checks
with:

```sh
make terran-t5 test-sc64-t5-combined
```

This creates `build/StarCraft-Terran-T5-GBA.gba` and adds Antigan Revolt to the
ordinary campaign menu. It is a distinct save profile: generation 14 is not
compatible with the combined T1–T4 generation 11, so keep separate save files.

A repeated controller-driven host run wins at input tick 121,547 and game
frame 121,261, with both heroes and the command center alive, no enemy
buildings remaining, and no trigger faults. The normal-menu ARM replay also
wins at frame 121,261; the fresh-process generation-14 save/load check passes.
The ARM run averages 20.85 effective fps, with an 11.95-fps worst counted
observer interval, below the 30-fps target. This is emulator evidence, not
physical-GBA performance validation.

## T6 packed-storage milestone (historical)

The internal T6 storage probe retains the Norad II map's 96×96 terrain and
216 non-start UNIT placements, including 35 mineral fields and three geysers. The importer
classifies 105 source UNIT rows as units and 73 as buildings; raw type 195 is
stored as a runtime building, so the mapped roster is 104 units and 74
buildings. The probe supports pools of 160 units, 80 buildings, and 48
minerals. Packed building rows use a 15,384-byte save record and generation
15, distinct from T5 generation 14.
See [the preparation and verification commands](DEVELOPMENT.md#t6-packed-storage-foundation-internal-only).

This section records the earlier storage-only step. Later T6 world integration
and controller-completion results supersede its statement that T6 is not in the
campaign runtime.

## Terran T6–T10 controller status

The late Terran runtime has host-controller wins for Terran missions 6–10;
combined with the previously completed missions 1–5, all ten Terran missions
now have host completion runs. T6 also has a full normal-menu ARM win at
frame 21,046, averaging 17.26 effective fps. T7, T8 and T9 full ARM replays
remain pending.

The frozen full ARM replay for T10 wins at frame 38,491: the Ion Cannon
(source raw type 127) is destroyed, Raynor is alive and the trigger fault
count is zero. The run averages 11.78 effective fps, below the 30-fps target.
It uses frozen ROM SHA-256
`329793a4707e0d3c4b33ed169391b2b887374d5eb788943ceef8828604c7ce44`.
This is emulator evidence; physical GBA testing has not been performed.

Missions 7–10 use bounded authored AI behavior where the original AI script
bodies are not implemented. This is documented adaptation behavior, not a
claim of original N64 AI execution. These missions are available through the
canonical normal-menu `terran` profile and the `./dev sc64 ... --profile
terran` workflow documented in [Development](DEVELOPMENT.md).

## Terran campaign objectives and late controls

The objective text below follows the campaign adapter's win/loss checks. Keep
the named heroes and protected forces alive as well as completing the main
goal.

| Mission | Objective |
| --- | --- |
| T1 | Find Raynor, build a Barracks, and train ten Marines. |
| T2 | Destroy the Infested Command Center; Raynor must survive. |
| T3 | Survive the countdown, or unlock and complete the alternate Zerg Hatchery objective. |
| T4 | Find the data disks; Raynor must survive. |
| T5 | Reach the rebel base and destroy Confederate buildings; Kerrigan and Raynor must survive. |
| T6 | Bring Raynor and two Dropships to Norad II; Raynor and Norad II must survive. |
| T7 | Deliver the Psi Emitter; Kerrigan must survive. An SCV picks up the emitter when it gets within a short distance, then carries it as it moves. |
| T8 | Clear the opposing bases and protect Norad II. |
| T9 | Destroy the remaining hostile buildings while protecting Kerrigan and the Zerg buildings. |
| T10 | Destroy the Ion Cannon; Raynor must survive. |

For T7, use **Move** to take your forces past Zerg Spore Colonies before
ordering attacks on the Protoss base. Attack Move may engage Zerg defenders
along the route. Keep Kerrigan away from the fighting; use an SCV to collect
and deliver the emitter.

In T8, the Lift / Land command helps reposition eligible starting structures.
Select a completed, player-owned Command Center, Barracks, Factory, Starport,
Engineering Bay, or Science Facility with an empty production queue. Press B,
scroll to **LIFT / LAND**, and press A to lift it. To move an airborne
building, use B → **MOVE** → A and choose a destination. To land, use B →
**LIFT / LAND** → A and click a valid clear site. The landing check rejects
blocked terrain, occupied ground sites, map-edge sites and overlapping
landing destinations. A completed building with a queued unit cannot lift.

The separate **COMMAND CENTER** command in the same menu starts construction:
select an SCV, press B, scroll to **COMMAND CENTER**, press A, choose a valid
site, and press A again to place it. It does not select an existing Command
Center; tap A on the existing structure to select it.

## Current limits

Higher Ground (Protoss 3, global catalog ID 22) now has an experimental runtime.
After preparing a race profile, run `make protoss-p3 test-sc64-p3`. This creates
`build/StarCraft-Protoss-P3-GBA.gba`, with Higher Ground on campaign page two.
It retains the full 128×96 terrain, all 96 units, 42 buildings, 48 mineral
fields and three geysers; source start markers do not become live entities.
The two Zerg forces are allies with separate owners, wallets and production
queues. The player starts with 300 minerals and 100 gas; source mission-start
actions grant each Zerg force its own resources. Destroy both forces' buildings
to start the persisted victory sequence. Losing every player unit and building
causes defeat, including during that sequence.

The original opaque AI scripts are replaced by bounded paid Zergling production,
mining assignments and periodic small guard attacks. Original transmission
dialogue/audio are absent; their holds use documented adapter timings. Scourge
combat has an air-only suicide attack with fixed source-backed damage. This
profile has a separate save generation, including quest actions and their
remaining hold time. A controller-only ASan/UBSan host run now completes the
mission at frame 151,088 after building a gas economy, Cybernetics Core and
Gateway army, then destroying all 38 hostile buildings. It retains 14 units
and 11 buildings. The frozen 5,809-transition input trace fits the paged
256-entry debugger queue, with at most 90 transitions per 1,000-tick page.
After the build/test command, replay it with:

```sh
python3 tools/sc64_p3_controller_replay.py --trace .local/sc64/p3-controller-asan-win.json
```

The helper supports frozen `--rom`/`--elf` paths, separate `--output` folders,
and bounded `--ticks` prefixes with optional normal-control `--save-at-end`.
It verifies the final quest model, completion bit, roster and global wallets.
The current default completes the full 152,107-tick ARM input replay at the
same frame, with 154 retained observations and verified final quest, roster
and wallet state. It averages 23.28 effective fps over 150,004 counted live
frames; the worst observed interval is 17.72 fps. The final captured result
matches the earlier baseline's RGB pixels. A fresh ARM process has also
restored and continued a normal controller-created save. In that same
saved scene, filtering defense candidates raised effective performance from
19.72 to 28.92 fps with identical reported gameplay state and captured pixels.
That is a bounded scene comparison, not a full-mission frame-rate result.
The near-30-fps mission target and physical GBA validation remain unmet.

P3 enables sorted defense candidate lists, a paged debugger input queue and
clear-free live rendering after full-frame coverage checks, terrain reuse
across small camera moves, exact drawable-object keys and visibility spans.
For an unoptimized
comparison build, use `make -B protoss-p3 P3_OPTIMIZATION_FLAGS=`; retain ROM/ELF
pairs separately, then restore the default with `make -B protoss-p3`.

The same 10,000-tick controller prefix improves from 21.53 to 23.39 effective
fps with the current default, matching all 11 reported gameplay checkpoints
and the three captured screens. The earlier baseline's final-report error is
documented in [Validation](VALIDATION.md); it does not affect the current
complete report.

The private Jacobs Installation import fixture remains available for verifying
the source records independently of the runtime. Regenerate the private race
tables, then run it:

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64
make test-sc64-t4-import
```

The fixture checks all 95 non-start placements and the full 128×128 Installation
terrain. Eight special Terran Beacons retain their raw type and use dedicated
internal actor and save handling. The importer retains the framed location,
doodad, sprite and trigger records with hashes under ignored
`.local/sc64/races/generated/t4/`. The import fixture does not itself expose
the mission; the combined experimental build above is the opt-in normal-menu
entry.

`make test-sc64-t4-storage sc64-t4-storage-probe` separately checks the T4-only
128×128 storage profile and links `build/starcraft-t4-storage-probe.elf`.
It uses packed fog/path grids, 14 route fields, a full-height targeting index
and the shared 256-event debugger queue. It has a distinct save generation.
The tests cover the bottom-right cell and max-size fog persistence using an
existing available mission; they do not load or complete Jacobs Installation.
This probe produces an ELF for memory validation, with no playable T4 ROM.

The gated internal actor/world probe can be checked with
`make test-sc64-t4-actors`. It loads the 95 source actors and tests their
supported roles, sight, bounded integer Ghost/concussive behavior, canonical
Beacon save encoding, and the authored Rescue Passive contact/transfer policy.
Rescue contact currently uses an inclusive 32-pixel square in GBA coordinates
as a center-based approximation; it does not claim exact native query edges or
ordering. At that actor-only stage, the T4 catalog was disabled. Door and trap
collision/state, teleports, triggers and completion were later integrated into
the combined experimental build described above; the isolated actor fixture
does not establish those behaviors or a frame-rate result.

The next source-only foundation preserves all numeric trigger/location/THG2
operands in private immutable C tables. `make test-sc64-t4-scenario` checks the
complete reconstructed source sections; `make test-sc64-t4-triggers
test-sc64-t4-queries test-sc64-t4-actions` checks the asset-free core. The runner
supports source ordering, shared per-player waits, a global switch bank,
preserve/retry behavior and validated resumable state. It is not wired into
the mission yet, and does not make T4 playable. Unknown effects stop at their
action cursor instead of being treated as completed.

`make test-sc64-large-map sc64-large-probe` checks the opt-in
`SC64_LARGE_MAPS` storage layout, including packed fog/path grids, larger
target indexing and 48 mineral slots. The ARM probe links the existing Protoss
missions with this layout; it does not enable Higher Ground. The synthetic
save check covers the maximum fog buffer and high resource slots, not a
128-column catalog mission's complete load/resume behavior. Standard build
flags and save layouts remain unchanged.

- Host controller runs complete 16 missions: Terran 1–10, Zerg 1–3 and Protoss 1–3. Full ARM replay qualification for Terran 7–9 remains pending; 14 campaign missions have no completed runtime.
- Skirmishes offer Outpost and Blood Bath, with one to three independently-raced opponents. They exercise the shared reduced engine rather than the original N64 engine or complete scenario scripting.
- Imported data must be generated locally from the user's own supported ROM. The source repository alone cannot build imported campaign profiles until that preparation has been done.
- Emulated and physical-hardware performance must be measured separately. This guide makes no frame-rate guarantee, and physical GBA validation remains a separate step.
