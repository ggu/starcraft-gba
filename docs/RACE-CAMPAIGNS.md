# Fixed-race campaign builds

The standard Terran, Zerg and Protoss targets remain separate ROMs. The
integrated Terran runtime now has controller-driven host wins and recorded
normal-menu ARM wins for all ten Terran missions across frozen builds. The
recorded optimization proofs include full T7 and T10 ARM wins; T8 and T9 remain
qualified on earlier frozen ROMs. These Terran timings precede the latest
minimap change, whose current ARM measurements cover Protoss. The current
full-profile ARM T10 result is below the 30-fps target, and no physical GBA
result is claimed. The canonical `terran` profile includes all ten Terran
missions. Canonical Zerg includes its first six missions. The default Protoss
target is the qualified P7 candidate. Explicit `protoss-p6` remains a separate
P1–P6 product with its generation-30 save identity.
`zerg-z4`, `zerg-z3` and `protoss-p3` remain explicit legacy builds with their original
ROMs and save identities; `protoss-p4-storm` also preserves its real four-entry files. The P7 candidate carries the P1–P6 campaign and the seventh menu entry; explicit
`protoss-p5-shuttle` preserves the earlier five-entry product. The
catalog records all 30 mission identifiers.

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

The default ROM targets are `build/StarCraft-Terran-GBA.gba`, `build/StarCraft-Zerg-Z6-GBA.gba`, and the P7 candidate `build/StarCraft-Protoss-P7-GBA.gba`. Explicit `protoss-p6` retains `build/StarCraft-Protoss-P6-GBA.gba` and its generation-30 identity. The candidate uses `.local/play/protoss-p7`; P6 uses `.local/play/protoss-p6`. Z6 uses `.local/play/zerg-z6`, Z4 uses `.local/play/zerg-z4`, explicit P5 uses `.local/play/protoss-p5-shuttle`, and explicit P4 uses `.local/play/protoss-p4-storm`; the legacy profiles continue using `.local/play/zerg` and `.local/play/protoss`. Extracted inputs and generated headers stay under ignored `.local/sc64/`; ROMs, ELF files, and test outputs stay under ignored `build/`. Do not commit or distribute the supplied ROM, imported map or art data, generated headers, saves, or captures containing imported game graphics.

To build and test all three race campaigns, prepare `terran`, `zerg` and
`protoss` with the same verified ROM, then run
`make race-campaigns test-race-campaigns`. Preparing Zerg or Protoss alone
does not generate the full Terran input set; preparing Terran alone does not
authenticate the P4 scenario. The asset-free `./dev test`
also exercises all three frontend variants without requiring those inputs.

With all three canonical profiles prepared,
`make test-sc64-canonical-skirmish-resume` checks their mid-match SRAM continuation.
It is also included in `test-race-campaigns`. Each profile runs 24 menu-driven
cases: both maps, every human race and one to three mixed-race opponents,
plus three-opponent groups of each race. The checks save after 1,800 economy
updates, load through HOME after reinitializing the game, and compare live
state and 900 resumed updates with the uninterrupted path. This is a host
regression check, not fresh-process emulator or physical-GBA qualification.
See the [canonical skirmish resume audit](audits/2026-10-27-canonical-skirmish-resume/README.md).

## Campaign progress

The Terran profile presents ten missions, canonical Zerg six and Protoss seven,
exposing 23 of 30 entries. All six Zerg chapters have current-source host
controller wins and input-only ARM wins in the canonical Z6 build; Z5 and Z6
also have exact fresh-process SRAM restores followed by continued wins. See
the [Z6 promotion audit](audits/2026-10-26-z6-promotion/README.md). P1–P5 have current-source host and input-only ARM wins in the canonical
P5 build; P5 also has an exact fresh-process cargo SRAM restore and continued
win. P1/P2 retain Fenix. See the
[P5 promotion audit](audits/2026-10-26-p5-promotion/README.md).

The P6 promotion audit records normal-input host wins for P1–P6. That P3 controller
selects reachable targets across both enemy forces, recovers blocked
builders through STOP, balances worker income and rebuilds before the final
assault; it wins at frame 120,733. A separate input-only rendered host replay
matches all 122 recorded state checkpoints without controller path queries.
P6 completes at frame 49,599, preserving both heroes, with an input-only ARM
replay and fresh-process SRAM continued win. See the
[P6 promotion audit](audits/2026-10-27-p6-promotion/README.md).

Performance remains unfinished. P6's current canonical timing is recorded in
that audit; the mission/order profile shows update work dominates. The
preserved P5 build measured about 26.6 fps equivalent on P1/P2, 18.9 on P3,
16.1 on P4 and 17.1 on P5. Those earlier timings and P1–P5 ARM wins are scoped
to the P5 image, rather than new measurements of every carried mission in P6.
Current Terran T7/T10 probe-entry traces average about 22.4/18.8 fps equivalent
after packed rendering; see the
[Terran rendering audit](audits/2026-10-26-terran-word-promotion/README.md).
None of these are physical GBA guarantees.

Z7 remains an internal seventh-entry candidate with a full ordinary-input ARM
replay to the source win and bounded fresh-process SRAM continuation. P7 is
now in the canonical Protoss profile. Its host campaign/save fixtures, 18-match
skirmish matrix and menu-driven save/reload pass. The P7 controller itself was
qualified on host; ARM replayed its recorded inputs, including an ordinary
pause-menu SRAM save and fresh-title-load continuation through the source win.
See the [Z7 full-win audit](audits/2026-10-27-z7-full-win/README.md), [P7
controller qualification](research/p7-full-controller-qualification.md) and
[canonical P7 qualification](research/p7-canonical-candidate-qualification.md)
for exact profiles and limits.
The current experimental P7 cache profile also replays the full winning tape
on ARM with all 67 state records matching; see its
[cache qualification](research/p7-fact-v2-qualification.md#full-arm-replay-of-the-current-cache-profile).

P7's canonical profile has passed those link, campaign and bounded save
continuation checks. A separate canonical ARM sample measured mean full costs
of 2,155,200 cycles in 600 opening updates and 2,426,615 cycles in 1,400 attack
updates, below the 30-fps target. This is a bounded sample, not a mission-wide
average. Neither profile establishes native enemy-AI equivalence, arbitrary
save-state coverage or physical-GBA behavior. Zerg 7–10 and Protoss 8–10
remain unqualified in the canonical profiles.

For P7, the canonical-render candidate passes its host startup/save fixtures,
normal-input campaign win, 18-match skirmish matrix and menu-driven save/reload.
The paired full-tape ARM SRAM replay saves at frame 28,000, resumes in a fresh
process and reaches the strict source win. P3's 180,000-call test-limit result
is byte-identical to the retained P6 canonical baseline; this remains a
separately documented controller limitation. See the [canonical P7 candidate
qualification](research/p7-canonical-candidate-qualification.md).
The internal P6 probe retains its own `SCPC` identity and its historical
[live-route audit](audits/2026-10-27-p6-live-route/README.md).

The internal Z7 profile also exposes the missing colony defense commands:
Drone-only selection adds CREEP COLONY below SPIRE (75 minerals), and a
selected completed Creep Colony offers SUNKEN with a completed Pool and
50 minerals. With a completed Evolution Chamber, the same Colony also offers
SPORE for 50 minerals. Both morphs use the same building and no additional
Drone. Spore attacks use the existing air-only defense policy; native detector
behavior and full source sight are still missing.
This feature rejects earlier Z7 generation-33/34 saves; it is not yet enabled
in the canonical six-entry Zerg profile or skirmishes.

Z7 also restores a neutral gas node when a source Extractor is destroyed.
The restored location persists through SRAM and can be rebuilt with a paid
Extractor. Gas depletion is still outside the shared engine model.

Zerg 3 **The New Dominion**, Zerg 4 **Agent of the Swarm**, and Protoss 3
**Higher Ground** are on the second campaign menu page. Canonical Zerg is
`StarCraft-Zerg-Z6-GBA.gba` / `starcraft-zerg-z6.elf`, code `SCZ6`, generation
32. The explicit legacy `zerg-z4` target retains `StarCraft-Zerg-Z4-GBA.gba` /
`starcraft-zerg-z4.elf`, code `SCZ4`, generation 20. The explicit legacy `zerg-z3` target retains `StarCraft-Zerg-GBA.gba` /
`starcraft-zerg.elf` (versioned `-Z3` names alias these), code `SCZ3`,
generation 5. The P7 candidate uses `StarCraft-Protoss-P7-GBA.gba` /
`starcraft-protoss-p7.elf`, code `SCP7`, generation 43. Explicit `protoss-p6`
retains `StarCraft-Protoss-P6-GBA.gba` / `starcraft-protoss-p6.elf`, code
`SCP6`, generation 30. Explicit
`protoss-p5-shuttle` keeps `StarCraft-Protoss-P5-Shuttle-GBA.gba` /
`starcraft-protoss-p5-shuttle.elf`, `SCPB`, generation 26. Explicit
`protoss-p4-storm` preserves its real P4 Storm files, `SCP5`, generation 21. Explicit legacy
`protoss-p3` retains `StarCraft-Protoss-GBA.gba` / `starcraft-protoss.elf`,
code `SCP3`, generation 7. Loaders reject incompatible saves without changing live game state; no save
conversion is performed.

Zerg 4 **Agent of the Swarm** is the canonical fourth entry, followed by Zerg 5
and Zerg 6; `zerg-z4` builds the separate earlier four-entry ROM. Protoss 4
**The Hunt for Tassadar** remains included in the six-entry canonical build;
`protoss-p4-storm` retains the separate four-entry product. See
[staged chapter setup](DEVELOPMENT.md#staged-zerg-and-protoss-chapter-4) and
[Validation](VALIDATION.md) for its controls, hashes and limits.

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

Historical Z3 implementation and performance evidence
------------------------------------------------------

The following commands and measurements document the legacy Z3 build.
Canonical Zerg now uses the separate Z4 ROM; the `zerg-z3` target explicitly
builds the older three-mission profile.

Use `make zerg-z3 test-zerg-z3` to build and test the preserved Z3 profile.
Use `make zerg test-zerg` for the canonical four-entry Zerg profile.

The historical target enabled three-bit route fields and exact row-span sight
updates.
The extra route slots retain more destinations within the same field-memory
budget. Sight updates preserve the existing circles and change only cells
entering or leaving them. These optimizations are enabled in the current Z3
profile. For a comparison build, use `make -B zerg Z3_OPTIMIZATION_FLAGS=`;
rebuild with `make -B zerg` to restore the optimized version. Retain each
ROM/ELF pair separately before comparing runs.

New Dominion appears on page two of the Zerg campaign. The legacy ROM is
`build/StarCraft-Zerg-GBA.gba`. Protect the Mature Chrysalis and eliminate
the Terran presence.
The source victory gates are at most six remaining enemy units/buildings
combined, or no enemy buildings. Losing the Chrysalis starts defeat.
All 58 runtime units, 33 runtime buildings, 28 mineral fields and three
geysers are imported. This includes both Dropships; raw type 150, the Mature
Chrysalis, is classified as a building by the runtime.
The Mature Chrysalis is a stationary structure with 250 HP and one armor;
the Young Chrysalis in the first two missions remains a separate powerup.
The 128-unit engine reserves sixteen slots from enemy training for the player.

The controller-only host test completes the mission by building an Extractor
and Hydralisk Den, assigning gas workers and combining forward defense with
attacks on enemy structures. It wins with the Chrysalis intact. This is a
host gameplay regression; ARM full-mission timing and physical GBA validation
remain separate checks.

The optimized ARM replay of that frozen experimental build also wins and
matches the host's final state. It averaged 19.19 fps during counted play, up
from 16.69 fps before those changes. Its worst complete observed interval was
15.23 fps. Reported gameplay state matched the earlier run at all 73
checkpoints. These are historical measurements, not measurements of the
current canonical profile; physical GBA testing remains pending.

To replay that host test's complete input trace on a retained frozen
experimental ARM build, run this after building that target:

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

That experimental Z3 build used `SC64_Z3_WINDOWED_SCHEDULE` to reduce its
debugger input buffer from 2,048 to 256 entries. The helper detects this queue
from ELF symbols and refills it at the existing
1,000-tick observer stops. Events retain absolute ticks; held buttons and
press edges carry across empty windows. It checks each page's capacity before
launch and verifies consumption before refill. The complete proof trace had
1,388 transitions and a peak of 49 per window. This option freed 14,328 bytes
of BSS after bookkeeping, preserved legacy count-only scheduling for simple
profilers, and had no input effect during ordinary play. Use a replay helper
that matches the retained windowed ELF.

Enemy pressure uses paid Marines every two minutes and small attacks from
the imported guards every three minutes. This is an authored substitute for
the source AI script. Result actions use explicit presentation timing policies:
30/15-tick waits, 90-tick transmission/text holds and a 13-tick loss-audio
placeholder. Original dialogue/audio and Dropship transport are absent.
The experimental save identity rejected older Zerg saves and retained the
pending action cursor and timer. The current canonical Zerg profile uses the
Z3 campaign identity and includes the first three missions. See
[Validation](VALIDATION.md) for the current qualification status and limits.

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
combined with missions 1–5, all ten missions have host completion runs and
normal-menu ARM wins across the recorded frozen builds. T6's earlier
generation-16 ARM run won at frame 21,046 and averaged 17.26 effective fps.
The baseline canonical ROM also has full T7, T8 and T10 ARM wins.

On that baseline ROM, T7 wins at frame 12,151 and averages 15.76 effective
fps; T8 wins at frame 86,596 and averages 14.37 fps; T10 wins at frame
38,491 and averages 11.78 fps. T10 ends with the Ion Cannon (source raw type
127) destroyed, Raynor alive and zero trigger faults. T9's ARM win is from
the earlier frozen ROM SHA-256
`329793a4707e0d3c4b33ed169391b2b887374d5eb788943ceef8828604c7ce44`: it
finishes at frame 358,906 with all 43 Protoss Force2 buildings destroyed, all
21 Zerg Force3 buildings intact, Kerrigan alive, all eight trigger clocks aligned and
zero faults, averaging 16.94 fps. These emulator results are below the
30-fps target; physical GBA testing has not been performed.

The latest optimization build improves T7 to 25.00 effective fps and T10 to
18.77 fps, with identical observed checkpoints and winning frames. All ten
host controllers still win, and fresh-process T7/T10 saves pass. See the
[optimization validation record](VALIDATION.md#terran-cpu-and-presentation-optimization-2026-10-23)
for exact build hashes, comparison scope and remaining performance limits.

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

Higher Ground (Protoss 3, global catalog ID 22) is in the canonical Protoss ROM.
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
remaining hold time. The current stock-stat controller completes the unchanged
native mission at frame 59,586 after building a protected economy, Photon
Cannons and a Gateway army, then destroying all hostile buildings. Its trace
records 2,999 input transitions and 59 live checkpoints. A fresh input-only
host replay reaches the native result. The earlier ARM proof and performance
measurement used commit `9a2e281`, before the imported-worker healing
correction; an ARM replay of the current source has not yet been run.
After the build/test command, replay it with:

```sh
python3 tools/sc64_p3_controller_replay.py --route stock --trace .local/sc64/p3-controller-stock-asan-win.json
```

The helper supports frozen `--rom`/`--elf` paths, separate `--output` folders,
and bounded `--ticks` prefixes with optional normal-control `--save-at-end`.
It verifies the final quest model, completion bit, roster and global wallets.
The stock route is the replay CLI default. Explicit `--route garrison` and
`--route expansion` retain their historical contracts; their older wins do not
qualify the current stock-stat engine. The previously measured 58,063-update
ARM replay and 839,963-cycle whole-route mean (about 20.0 fps equivalent) apply
to commit `9a2e281` before the worker-heal correction. Opening skirmish samples
near 29 fps do not establish
mission-wide performance. The near-30-fps mission target, current-stock
fresh-process SRAM replay and physical GBA validation remain unmet. See the
[current redraw and P3 audit](audits/2026-10-26-static-art-and-p3/README.md).

P3 enables sorted defense candidate lists, a paged debugger input queue and
clear-free live rendering after full-frame coverage checks, terrain reuse
across small camera moves, exact drawable-object keys and visibility spans.
For an unoptimized
comparison build, use `make -B protoss-p3 P3_OPTIMIZATION_FLAGS= CANONICAL_DRAW_FLAGS= CANONICAL_STATIC_FLAGS= PROTOSS_CANONICAL_FLAGS=`; retain ROM/ELF
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

- Canonical profiles expose T1–T10, Z1–Z6 and P1–P7 (23/30). P7 has a current canonical host win, a fixed-input ARM replay and fresh-process SRAM continuation. Explicit P6 retains its separate generation-30 product. Earlier P1–P5 ARM/cargo evidence remains scoped to P5. Zerg controllers complete all six chapters, with canonical Z6 ARM wins and Z5/Z6 save continuity. Terran ARM wins span all ten missions across frozen builds. Seven campaign chapters and near-30-fps performance remain unfinished.
- Skirmishes offer Outpost and Blood Bath, with one to three independently-raced opponents. They exercise the shared reduced engine rather than the original N64 engine or complete scenario scripting.
- Imported data must be generated locally from the user's own supported ROM. The source repository alone cannot build imported campaign profiles until that preparation has been done.
- Emulated and physical-hardware performance must be measured separately. This guide makes no frame-rate guarantee, and physical GBA validation remains a separate step.
