# Validation

This page records reproducible checks for the source snapshot. Automated host checks validate selected rules and data handling; they do not establish complete gameplay balance or physical-hardware behavior.

## Protoss P8 candidate (2026-10-29)

The explicit `protoss-p8` candidate (`SCP8`, generation 44) adds P8 to the
canonical P1–P7 profile and binds the shared Protoss unit services (Carrier
Interceptors, Arbiter cloak, detection, Shield Batteries); custom games and
Showcase scenes bind the same services generically. Canonical Protoss stays
P7. Its image changes to `9952b59e…` because it compiles that generic
binding (8 more bytes of EWRAM, unchanged IWRAM and save size), and its P1,
P2 and P7 campaign controller traces are byte-identical to the default
branch's. One ordinary-input host controller run wins P8 at frame
82,521 with no runtime faults, and is deterministic across runs. The
input-only ARM replay of its 9,684-transition key tape matches all 85 host
observations, state digests included, through the same win. A pause-menu
save at frame 30,955, with a structure under construction, gives an SRAM
record that is byte-identical on host and ARM. Loading it from the title
screen in a fresh process continues to the same win: under the strict
comparator it holds every observation of the uninterrupted replay from the
documented resume tick, with no missing, duplicate or extra ticks.
Whole-tape ARM guest timing averages 2,100,638 cycles per frame (about 8.0
fps equivalent; frame rate is not qualified). The playable ROM links at
`__bss_end=0x0203895c` and `__iwram_end=0x03004c48`. Executed build
commands, raw dependency output and input hashes are recorded in the audit.
The AI backend, transmission duration and trigger cadence are authored.
These are emulator and host results only, with no hardware claim. See the
[P8 candidate audit](audits/2026-10-29-p8-campaign/README.md).

## Current race-build continuation (2026-10-28)

After worker-arrival construction, all three canonical race vectors pass a
normal-menu save followed by a fresh-process physical-SRAM load and 900
matching skirmish updates. Each uses Blood Bath, its profile's human race and
three mixed-race AIs. The bounded early-scene timing is about 28–29 fps
equivalent; later battles and campaign-wide near-30-fps play remain unproven.
A separate current-source P7 input-only ARM replay matches all 65 checkpoints
through a strict victory with all three heroes alive. See the
[current race-build audit](audits/2026-10-28-current-race-sram/README.md)
for exact images, protocol and limits. These checks cover the runtime shared
by PR113 and the test-only PR114, rather than requalifying every historical
mission result below. Current catalog coverage is 23/30 (Terran 10, Zerg 6,
Protoss 7); the all-30-mission and near-30-fps goals remain unfinished.

## Optional Nydus Canal runtime (2026-10-27)

`SC64_NYDUS_ENABLED` is off in every shipped profile. Flag-off default, Z7
probe and Z7 controller ROMs are byte-identical to `codex/source-release`
`0fd5a29`. A Z7-hosted actual-engine test and an mGBA debug-log probe
exercise linked transit with ordinary input. Neither is Z8 gameplay or
physical-GBA evidence. See [Nydus Canal](NYDUS-CANAL.md).

## Contact cache movement measurement (2026-10-27)

`SC64_CONTACT_CACHE` is now part of the canonical Terran, Zerg and Protoss
ROM flags. It caches each unit type's collision facts in one byte, answers
all eight blocked-steering offsets in one bucket pass and removes the
software division from the stock movement step, without changing the
collision predicate, steering order, speeds, routes or save formats. On the
public P7 movement workload the attack-move phase falls from 1,890,055 to
1,551,295 timer cycles per update (−17.9%, 8.88 → 10.82 fps) and the
opening from 1,582,381 to 1,426,952 (−9.8%); last-tick update cycles fall
21.3% and orders-stage cycles 30.1%, with all 23 state checkpoints matching.
Those figures are for the P7 probe, which keeps the steering pass and the
stock step in IWRAM. The canonical Zerg Z6 and Protoss P6 ROMs have no IWRAM
reserve for them and build with `CONTACT_ROM_PLACEMENT_FLAGS` (steering
pass and stock step in ROM); the P7 probe built with the same flags keeps
all 23 checkpoints and falls from 1,890,055 to 1,744,951 attack-move cycles
per update (−7.7%, 8.88 → 9.62 fps) and 1,582,381 to 1,447,575 in the
opening (−8.5%). Canonical Terran 3-bot skirmishes match every checkpoint
and stay at their frame cap. The differential fixture
(`make test-sc64-contact-cache`) checks about 10 million step, mask and
stock-step answers against the original code under six flag sets, including
the canonical Zerg and Protoss vectors, and `make test-sc64-p7-contact`
reproduces the reference P7 trace byte for byte. The canonical Terran, Zerg
Z6, Zerg Z4, Protoss P6, P5 Shuttle and P4 Storm products all link under the
unchanged linker guards (Zerg Z6 IWRAM end `0x03004ea8`, Protoss P6
`0x03004ae8`). No near-30-fps or hardware claim is made. See the
[contact cache audit](audits/2026-10-27-contact-cache/README.md).

## Current campaign status (2026-10-27)

The [stock roster follow-up](audits/2026-10-26-roster-contact-and-art/README.md)
corrects melee contact, defense cooldown expiry and selected building art.
The newer [P4 promotion](audits/2026-10-26-p4-promotion/README.md) and
[Terran renderer](audits/2026-10-26-terran-word-promotion/README.md) audits
requalify their stated ARM traces after those combat corrections. Older
measurement sections below remain records of their frozen images.

The [construction lifecycle recheck](audits/2026-10-26-construction-lifecycle/README.md)
corrects the Protoss pre-completion visual to the source Warp Anchor's `Init`
animation. It keeps the existing build timer and save layout. The older
campaign ARM timings below precede that artwork correction. The
[P4 prefix follow-up](audits/2026-10-26-p4-create-prefix/README.md) records a
current-art paired full-mission ARM replay, a 2.16% full-cost reduction and
fresh-process SRAM continued win. It does not establish physical-GBA performance.
The [minimap follow-up](audits/2026-10-26-minimap-terrain/README.md) reduces
P4's measured full cost by another 1.60%, to about 17.0 fps equivalent, with
pixel-identical terrain drawing and unchanged worst-frame stalls.

Canonical profiles expose Terran 1–10, Zerg 1–6 and Protoss 1–6: 22/30
entries (see the [Z6 promotion audit](audits/2026-10-26-z6-promotion/README.md)).
Canonical Zerg uses `SCZ6`, generation 32 and `.local/play/zerg-z6`; explicit
`zerg-z4` keeps `SCZ4`, generation 20 and `.local/play/zerg-z4`; canonical
Protoss uses the P6 files, `SCP6`, generation 30 and `.local/play/protoss-p6`.
Explicit `protoss-p5-shuttle` keeps its P5 files, `SCPB`, generation 26 and
`.local/play/protoss-p5-shuttle`. Explicit `protoss-p4-storm` retains the real
P4 files, `SCP5`, generation 21 and its own save path. Explicit `zerg-z3` and
`protoss-p3` preserve their earlier real files, identities and save paths. No save conversion is
provided. The old P3 compatibility filenames still point to the P3 real
files, never to the newer canonical ROM.

All ten Terran missions pass the actual canonical-flag host controller
target. T7/T10 have current paired ARM wins from explicit mission probe entry;
other Terran ARM wins remain evidence for their earlier frozen builds. All
six Zerg chapters have current host and input-only ARM wins in the canonical
Z6 build, with Z5/Z6 fresh-process SRAM continuity. P1–P5 have fresh host and input-only ARM wins in the
canonical P5 pools, retaining Fenix in P1/P2 and both heroes in P4. P5 has an
exact fresh-process cargo SRAM restore and continued win. Earlier P4 save
qualification remains scoped to its preserved P4 build. See the
[P5 audit](audits/2026-10-26-p5-promotion/README.md).

Current P6 adds six canonical-flag host mission wins, a 57-checkpoint input-only
ARM win and fresh-process SRAM continuation; the P3 input-only rendered host
replay matches 122 checkpoints. See the
[P6 promotion audit](audits/2026-10-27-p6-promotion/README.md) for exact scope.

The 30-fps goal remains unfinished. The preserved P5 build measures 26.6 fps equivalent
on P1/P2, 18.9 on P3, 16.1 on carried P4 and 17.1 on P5; carried P4 is about
5.3% slower than the standalone P4/minimap build. PR 35's earlier averages were about
22.4/18.8 fps equivalent for Terran T7/T10 and 16.3 for P4. These are scoped
emulator cycle measurements, not physical-GBA guarantees. Historical staged
coverage spans 22/30 across recorded builds, not fresh qualification of all
chapters on current source. The generic split-profile report checker only
covers earlier mission models; dedicated Z4/P4/P5 codec/input checks establish
their stated save continuity.

## Reproducible checks

For the asset-free skirmish, run:

```sh
./dev build
./dev test
```

These checks build the StarCraft ROM and run the host-side rule regressions. For the optional ROM-derived unified profile, first follow [Development](DEVELOPMENT.md#optional-unified-profile), then run `make unified test-unified`. The profile tests cover implemented mission/frontend behavior and save handling, not every possible campaign route.

To validate play in mGBA, use `./dev play starcraft` for the default game or `./dev play unified` after preparing the optional profile. Emulator results are not physical GBA results.

## Canonical skirmish resume checks (2026-10-27)

`make test-sc64-canonical-skirmish-resume` compiles the real canonical Terran,
Z6 and current P6 profile flags. The original audit used P5; the P6 promotion
reruns the Protoss cases with its expanded flags. The sanitizer-enabled fixture runs 24 normal-menu
cases per profile, covering both maps, all human races, one to three mixed
opponents and three-opponent groups of each race. All 72 cases pass: mining
depletes resources before saving, map/race/opponent configuration survives
SRAM loading, and the restored live-state hash and subsequent 900 updates
match the uninterrupted path. Each continuation also earns player minerals
and further depletes the map's resources.

This closes a current-profile continuation coverage gap; earlier split-profile
and match-controller checks remain separate. `game_init()` clears actors and
frontend state in the same host process while retaining emulated SRAM. This
does not prove fresh-process SRAM, physical-GBA behavior, a complete match,
every possible bot-race permutation, or near-30-fps performance. See the
[audit and reproduction commands](audits/2026-10-27-canonical-skirmish-resume/README.md).

## P6 live-route repair (2026-10-27)

The current P6 probe controller replaces its stale 102,005-tick opening with
live Marine rescue, regroup and escort orders. It wins at frame 49,599 after
50,252 controller updates, preserving Tassadar and bringing Zeratul into the
authenticated extraction area with zero trigger faults. A separate input-only
host replay and an owned no-GDB ARM replay match all 57 observed mission,
action-timer and hero checkpoints through that win.

The instrumented probe averages 1,148,439 update cycles and 448,493 audio/draw
cycles over 50,241 play-screen calls: about 10.51 fps equivalent throughput.
These calls include play-screen command-menu inputs that do not advance the
simulation. This is a different route/window from historical 12.75-fps P6
results, not an apples-to-apples speed comparison or physical-GBA guarantee.
The changed code is the host controller, not the game rules or player ROM.
Current codec fixtures pass, but no fresh-process SRAM proof or canonical
promotion is added. See the [qualification audit](audits/2026-10-27-p6-live-route/README.md).

## Standalone snapshot checks (2026-10-22)

- Before any private data or reference checkout was prepared, `make verify`
  built the 65,536-byte original-art skirmish ROM, and `make test` passed ten
  host C checks plus 21 Python importer/format tests. Host checks use ASan/UBSan.
- The pinned extractor was fetched into ignored `references/`. Preparing the
  unified profile from a locally supplied supported ROM and running
  `make unified test-unified` succeeded. The resulting 2 MiB ROM is byte-for-byte
  identical to the working snapshot's latest combined build: SHA-256
  `79d09f44d70d2c9476cdaf9d7a740e40b66a65026d4f60e10b24356230033e91`.
- The original-art ROM's SHA-256 is
  `cbbe5b8fcc59407208825504f3ec165bbe1c62190e31cbf4b78504e7388a0012`.
- `tools/check_source_tree.py` audits the Git index, excluding binary payloads,
  private directories, generated asset headers, oversized files and credential
  patterns. It passed the source snapshot and rejected a synthetic accidentally
  staged generated header. It supplements the ignore rules and manual review.
- The GitHub Actions workflow builds and tests the original-art profile without
  any ROM or extracted assets. Its remote result is separate from these local
  checks.

## Contributor onboarding checks (2026-10-22)

The documented no-ROM workflow is `./dev doctor`, `./dev build`, then
`./dev test`. The default doctor reports mGBA as optional; `--emulator` makes
it required. Five regression checks cover missing build tools, optional or
invalid emulator configuration, and `--help` on eight emulator entry points
without an emulator or private inputs. There are now 26 Python checks in the
asset-free suite. The CI workflow runs these same wrapper commands.

The optional unified host suite also passes. A real mGBA default-skirmish
smoke run verifies controller input and SRAM saving, and the unified Wasteland
1,000-tick opening profile runs through the corrected emulator launcher.

## Race campaign stage 1 (2026-10-22)

`./dev build`, `./dev test`, `make unified test-unified`, and
`make race-campaigns test-race-campaigns` pass with the prepared local inputs.
The asset-free suite includes the three fixed-race frontend variants and
33 Python checks. The split-profile host tests launch every combination of
two maps, three human races, and one to three opponents, round-trip those
selections through SRAM, reject invalid records, and run 10,000 ticks of
mining, construction, gas gathering, and AI production per profile.

`tools/sc64_split_smoke.py --profile all` passed all 60 distinct ARM menu
launches in mGBA: six campaign launches and 54 skirmishes. Separate emulator
processes and copied save files exercise fresh SRAM loading. Reports and
captures stay in unique ignored `build/emulator/` directories.

Three controller-neutral, three-opponent runs of
`tools/sc64_split_profile.py --ticks 10000` measured the following live-play
rates using chained GBA hardware timers:

| ROM profile | Map | Human race | Mean fps | Maximum live units observed |
| --- | --- | --- | ---: | ---: |
| Terran | Blood Bath | Terran | 29.53 | 50 |
| Zerg | Blood Bath | Zerg | 29.58 | 42 |
| Protoss | Outpost | Protoss | 29.38 | 45 |

All 30,000 measured ticks remained on the play screen. These results cover
those scenarios and populations; they do not establish full-pool or physical
GBA performance. The three ROMs are 2 MiB each, and the linked campaign tile
tables contain only the selected race's two missions. All 30 mission slots
have stable catalog IDs, but only the original six missions are playable in
this stage. See [race campaign builds](RACE-CAMPAIGNS.md).

## Campaign preparation checks (2026-10-22)

The source-only suite also checks bounded bit grids against a byte-array
oracle and coordinate conversion against division for all supported 64, 96
and 128 tile dimensions, including rectangular maps. These helpers are
preparation; the production runtime still has its existing map limits.

The standalone Desperate Alliance rules model passes its full 54,000-tick
countdown, alternate Hatchery objective, pause, defeat policy, save-copy
continuity and invalid-state tests under ASan/UBSan. The private-input
`make test-sc64-t3-import` check confirms all 196 non-start-marker records
are retained and mapped: 138 units, 31 buildings, 25 mineral fields and two
geysers. It also checks every placement coordinate and terrain lookup index.
Vulture rules and Engineering Bay raw-ID round trips have regression coverage.

Default, unified and all three split-profile build/host suites pass after
these additions. T3 remains disabled in production flags.

## Experimental Desperate Alliance runtime (2026-10-22)

`make terran-t3 test-sc64-t3-runtime` builds the opt-in ARM ROM and checks its
runtime under ASan/UBSan. The controller harness selects T3 through the actual
mission list and briefing, verifies the complete 138-unit/31-building roster,
and wins by survival after 54,000 live mission ticks. It renders the opening,
periodic checkpoints and result. The run includes paid Zergling production,
periodic attacks and the final combat-unit assault; it makes no direct writes
to mission state. The alternate victory and loss policies have separate model
coverage; they have not yet been completed through live controller play.

The save test fills all 160 unit slots, round-trips the 14,052-byte payload,
rejects checksum-valid inconsistent mission/timer/result records before any
live-state mutation, and rejects the prior save version. Physical GBA testing
remains pending.

The target broadphase uses bounded, owner-separated 64-pixel cells and tracks
same-frame movement. Differential tests cover dense and sparse rosters, flight
and scenario flags, strict range boundaries, lowest-index ties, highest entity
slots, movement, death, invalid coordinates and dirty-list overflow/reset.
The original scan remains the fallback. The complete controller trace is
identical before and after the optimization. Tower targeting has a separate
brute-force oracle; the full survival run also checks cached supply totals.

Hardware-timer measurements distinguish these local ROMs:

| Build | Observed ticks | Counted live ticks | Mean live fps | Outcome |
| --- | ---: | ---: | ---: | --- |
| Initial runtime (`b208a7a7` ROM hash prefix) | 54,000 | 53,000 | 19.20 | Survival win |
| Intermediate optimization (`c7487a79`) | 54,000 | 53,000 | 27.91 | Survival win |
| Validated target-index build (`69f7ea7e`) | 54,000 | 53,000 | 27.97 | Survival win |

The result-ending interval is excluded from full-run means because the exact
finish point within that interval is unknown. That build completes the
full mission with the same observed campaign progression, but its final
assault still drops to about 19 fps. T3 stays opt-in while that slowdown is
addressed. These emulator measurements do not establish physical GBA performance.

The controller-only host resume check advances T3 for 1,000 ticks, pauses it,
proves that the countdown and roster remain frozen, saves through the pause
menu, resets to HOME, loads with L, and verifies exact serialized state before
resuming the simulation. It runs under ASan/UBSan as part of
`make test-sc64-t3-runtime`.

The matching ARM ROM also passes `tools/sc64_split_smoke.py --profile terran
--mission 6 --rom build/StarCraft-Terran-T3-GBA.gba
--elf build/starcraft-terran-t3.elf`: normal menu launch, 1,000 live ticks,
pause stability, SRAM save, a separate mGBA process loading with HOME L,
exact restored model/resources/alive-entity state, and resumed live ticks.

The validated `69f7ea7e` link uses 388 bytes of data and 246,656 bytes of BSS, leaving
6,908 bytes below the EWRAM guard. Its IWRAM code ends at `0x03004e80`, below
the `0x03005000` guard that reserves stack space. Default, unified, three
race-profile and T3 host suites pass. Four T3-enabled frontend variants are
also part of the asset-free suite, alongside 35 Python checks.

For internal order profiling, build with
`make -B terran-t3 T3_EXTRA_FLAGS=-DSC64_DIAGNOSTIC_ORDERS`. The optional counters
separate setup, target queries and movement; they add measurement overhead.
Rebuild with plain `make -B terran-t3` before measuring final frame rate.
The profiler atomically publishes each observer snapshot in ignored
`progress.json` and writes the complete `profile.json` at completion.

## Experimental New Dominion (2026-10-22)

The asset-free suite includes the Z3 trigger model and a compiled matrix for
the generated map table. The matrix checks stable mission IDs, profile guards
and zeroed unavailable entries through global ID 14. The rules preserve the
two switch-7 victory gates, Mature Chrysalis loss, one-shot resource grants
and result-action order. Raw Wait and transmission fields remain explicit;
the standalone model does not infer their duration in simulation ticks.

`make test-sc64-z3-import` validates the locally generated 64-by-96 map:
58 units, 33 buildings, 28 mineral fields and three geysers, with only two
start markers excluded. Every retained placement has a supported runtime
mapping; the two Dropships now have a distinct flying, unarmed unit identity.
The fixture also checks coordinates, terrain indices and unavailable map
slots. Cargo behavior and the source enemy AI script remain unimplemented.

The separate `make zerg-z3` build enables the mission at global ID 14, with
an authored bounded enemy-pressure adapter and persisted result-action state.
The corrected 5,000-tick ARM run (`cbb7e7b8` ROM hash prefix) remained on the play
screen and averaged 29.22 fps. It observed up to 59 live units, including one
paid Marine produced by the adapter. This is opening/performance evidence;
it is not a controller-played mission completion or physical GBA measurement.
The corrected version represents raw type 150 as a stationary 250-HP, one-armor
structure; raw type 216 retains its Young Chrysalis identity. This changes
the retained roster classification from 59 units/32 buildings to 58/33.

`make test-sc64-z3-runtime` passes under ASan/UBSan. Its controller path
checks the complete opening roster and source resource grants, 1,000 live
ticks, pause stability, exact save/load continuity and resumed play. Separate
fixtures deliberately change scenario state to check both win predicates,
the raw-150 loss sequence and the enemy-production reserve. Those fixtures
are rule integration checks, not controller-played victories. Checksum-valid
invalid saves are rejected before live state changes, including inconsistent
action timers, phases and objectives, unavailable map IDs, cross-profile
completion bits and stale Z3 state on other missions.

The Z3 link uses 388 bytes of data and 240,240 bytes of BSS. Its IWRAM ends at
`0x03004e90`, below the stack-reserving `0x03005000` guard. Its 12,284-byte
SRAM payload fits one bounded record. Eight asset-free frontend variants
exercise Z3 with and without T3 across unified and fixed-race profiles.

The full controller-only host runner is now part of
`make test-sc64-z3-runtime`. Its hybrid policy builds an Extractor and
Hydralisk Den through the UI, assigns three Drones to gas, caps queued worker
production, protects a forward waypoint and attacks enemy structures. A
fresh ASan/UBSan run wins at frame 70,833 with the Mature Chrysalis at 250 HP,
79 human units/eight buildings and three enemy units/one building. The
remaining enemy presence of four satisfies the source at-most-six predicate.
All gameplay mutations come from `game_update` controller input; the policy
reads state to choose actions. Its 1,388 held-input transitions include key
releases and stay in an ignored local trace. Loss or the simulation/input
budget ending returns failure. This host result is not an ARM or physical
GBA frame-rate measurement.

The matching full ARM replay also passes. ROM hash prefix `b6d5b616` replays
all 1,388 inputs from fresh HOME through scheduler tick 71,186, winning at
mission frame 70,833. Its final roster, resources, completion flag, Z3 state
and 250-HP Mature Chrysalis match the host proof. It observes up to 95 live
units. Hardware-timer deltas over 69,659 counted simulation frames give a
mean of 16.69 fps; the worst complete observed interval is 9.03 fps. Initial
HOME/start and final-result intervals are excluded, and counted intervals
use actual simulation-frame deltas so command-menu pauses do not inflate
the rate. The mean was independently recomputed from the recorded frame and
cycle totals. This establishes completion on the emulator CPU model, while
the near-30-fps target and physical GBA validation remain unmet.

The source-only suite now includes 47 Python checks. Nine replay checks
cover tick conversion, queue capacity derived from ELF symbols, malformed
input transitions, unsupported event kinds and frozen-proof validation.

The replay helper also accepts a bounded `--ticks` prefix and optional
`--save-at-end`. A 59,000-tick diagnostic replay (`3a45b44f` ROM hash prefix)
remained in live play at frame 58,707, then used START/B to save frame 58,708
and flushed a 32-KiB SRAM file on normal emulator shutdown. Prefix reports
identify themselves as prefixes and skip the full-win assertion. Diagnostic
cycle counters are last-tick samples; only the cumulative route counters
produce interval deltas. At tick 59,000 the preceding interval had 2,381 route
hits and 1,097 field builds, identifying substantial late-battle cache churn.
These diagnostic timings include instrumentation overhead. The generic
split profiler now decodes Z3 model/action state in loaded-save snapshots
and observations. The asset-free suite includes 51 Python checks.

The result adapter retains source predicate/action order but uses explicit
presentation policies for unresolved timing: 30/15-tick waits, 90-tick
transmission/text holds and a 13-tick silent loss-audio placeholder. It does
not play the original transmission dialogue or audio. The two imported
Dropships remain unarmed flying targets without cargo mechanics.
Once a result sequence starts, this adapter serializes its actions and stops
reevaluating victory/loss predicates. Concurrent source-trigger behavior is
unresolved; this serialization is also an explicit adapter policy.

## Eligibility-cache experiment (2026-10-22)

An experiment computed attacker ground/air eligibility once per target query.
Its differential checks and complete host T3 trace passed. The full ARM run
(`982062b4`) won but averaged 27.84 fps, with final-assault intervals around
19.2 fps. The change was removed because it consumed 256 additional IWRAM
bytes without improving frame rate. The existing spatial target index remains.

Three 10,000-tick, three-opponent ARM runs of that experiment measured:

| Profile | ROM hash prefix | Map | Human | Mean fps | Peak observed units |
| --- | --- | --- | --- | ---: | ---: |
| Terran | `44a9917c` | Blood Bath | Terran | 29.57 | 50 |
| Zerg | `d746205d` | Blood Bath | Zerg | 29.61 | 42 |
| Protoss | `3962579a` | Outpost | Protoss | 29.41 | 45 |

All remained on the play screen. These bounded-population runs do not prove
full-pool or physical GBA performance. Its T3 link ended IWRAM at
`0x03004f80`; removing the experiment recovers space for further profiling
and later campaign work. These historical hashes identify the measured
binaries separately from subsequent changes.

## Campaign checkpoint profiling (2026-10-22)

`tools/sc64_split_profile.py --mission 6 --save-at-end` can pause and save a
live campaign after its measurement. `--load-save PATH --mission 6` copies
that SRAM into a fresh emulator process and loads it through HOME L before
profiling. Saves remain ignored local inputs. Use a compatible ROM/save
generation; the game's loader rejects unavailable missions and stale layouts.
The debugger schedules controller input and observes state; it does not write
units, resources or mission timers to create a checkpoint.

The frozen `a19385e` plain T3 build passed an ARM save/load smoke: measurement
ended at frame 1003, the normal pause/save path recorded frame 1004, and a new
process loaded exactly frame 1004 before continuing to frame 2005. The report
records both observer and saved frames because switching breakpoints can
advance one live tick before pause. Two asset-free checks reject missing or
wrong-sized SRAM and incompatible profiler options before emulator launch.

A separate instrumented full T3 run of that source completed all 54,000 ticks
and won. Last-tick movement cost rose from about 7,300 cycles at tick 50,000
to 251,000–270,000 during ticks 51,000–53,000. Target-query cost grew much less.
This points to dense local movement as the next optimization target. These
diagnostic counters add overhead and are not a final frame-rate measurement.

## Higher Ground preparation (2026-10-22)

Default build/tests, unified build/tests, all three race-profile suites and
both experimental mission suites pass after the storage/import preparation.
The asset-free suite includes 38 Python checks, the Higher Ground source-fact
and trigger-model fixtures, and packed 128×96 route fields checked against an
independent breadth-first search under ASan/UBSan.

`make test-sc64-p3-import` confirms all 189 non-start source records: 96 units,
42 buildings, 48 minerals and three geysers. It preserves source owner slots
0 and 6 separately, direct global ID 22, and zeroed unavailable table slots.
The model covers independent force grants, human AnyUnit defeat, ordered
delayed victory, pause/copy continuity and invalid persisted states. It leaves
timing conversion and same-frame trigger priority as explicit adapter policy.
Neither the import flag nor these model checks make Higher Ground playable.

`make test-sc64-large-map sc64-large-probe` passes with the existing Protoss
catalog and opt-in storage. The ARM probe uses 364 bytes of data and 243,920
bytes of BSS, an actual BSS increase of 3,696 bytes over the matching standard
Protoss build. Its IWRAM ends at `0x03004fe8`, within the `0x03005000` guard.
This narrow code margin remains a constraint on further hot-path additions.
The synthetic fixture checks the final grid cell, minimap/target/path indexes,
mineral slots 32–47 and full fog-buffer persistence. It does not validate
loading a real 128-column mission's geometry or gameplay.

A follow-up keeps the large-profile minimap calculation in exact integer
arithmetic after caller validation. Exhaustive tests check all 46,080 cells
across the six supported width/height combinations against a division-based
oracle. The updated probe ends IWRAM at `0x03004f48`, recovering 160 bytes of
code space while leaving its BSS size unchanged. The production branch is
unchanged.

`make test-sc64-race-collision` checks the race collision buckets against a
complete overlap scan over 959,577 candidates, including dense formations,
same-cell movement, flight classes, edge coordinates and dead neighbors.

## Collision reuse experiment (2026-10-22)

A bounded nearby-actor cache passed the full T3 host survival run with a
byte-identical controller trace. In ARM testing, both it and the matching
plain build loaded the same controller-created SRAM at frame 50,004 and won
at frame 54,000. Observed entity, resource and campaign states matched at
every checkpoint. The three complete 1,000-tick live intervals measured:

| Build | ROM hash prefix | Mean fps | Crowded interval fps |
| --- | --- | ---: | --- |
| Plain storage-preparation build | `bb229ac8` | 20.13 | 19.17, 19.16 |
| Cold candidate-cache helper | `45808a51` | 16.13 | 14.76, 14.89 |

The ending result interval is excluded. The cache was removed because it
increased CPU cost. A revision keeping its predicate in IWRAM passed host
differential checks but exceeded the stack-preserving code guard; that
revision was not run as a ROM. The original bucket implementation and the
expanded race collision tests remain. These are emulator measurements from
a specific late-mission checkpoint, not physical GBA results.

## HUD and town-hall follow-up (2026-10-22)

The resource and command HUD bands now invalidate independently. Wallet and
supply updates can refresh the top strip while restoring the command panel.
Full or empty health bars use one opaque fill and avoid the intermediate-value
division. A scalar framebuffer oracle covers health-bar values and clipping;
unified, T3 and Z3 fixtures compare cached HUD pixels against forced redraws
across resources, queued supply, selection, construction, commands, messages,
timers and the dynamic minimap. No frame-rate gain is claimed from host tests.

Completed imported Hatcheries, Lairs and Hives share doubled supply of two,
larva production, creep, prerequisite and producer compatibility. The host
fixture checks paid Drone/Overlord/Zergling/Hydralisk queues, regrowth and
caps, incomplete and cross-faction rejection, and actual mineral/gas returns.
Ordinary placement still has no Lair/Hive/Greater Spire morph operation.
This supports later campaign integration; it does not enable Higher Ground.

Default build/tests, unified build/tests, all three race-profile suites and
the T3/Z3 runtime suites pass. The plain Z3 link ends IWRAM at `0x03004ea8`;
the large-map probe ends at `0x03004f68`, within the unchanged stack guard.

In a controlled late-Z3 comparison, both plain `b6d5b616` and HUD/bar
`529e53e1` ROMs load the same controller-created SRAM at frame 58,708. Their
observed campaign/Z3 state, entity records and resources match at every
checkpoint. Both unattended runs lose at frame 67,997; this is a performance
comparison, not another mission-completion proof. Over the same 9,000 live
frames, the weighted rates are 18.66 and 18.94 fps respectively, a 1.50%
increase. The ending result interval is excluded. Route counters match;
the reports do not compare every byte of world RAM.

The follow-up route cache holds 18 fields in standard race profiles, 17 in
T3 and 15 in large-map builds. A capacity-plus-one fixture checks LRU touches,
eviction/rebuilds, reset invalidation and exact BFS tie directions, including
blocked starts and unreachable destinations. The complete Z3 host win trace
remains byte-identical. The larger fields are derived state and are discarded
on load; the SRAM layout is unchanged. BSS is 249,472 bytes in Z3, 251,272 in
T3 and 250,072 in the large-map probe. IWRAM ends at `0x03004ec0`,
`0x03004ec0` and `0x03004f80` respectively; neither linker guard was relaxed.
Race building sight also recognizes a Hive as a town hall, with edge-clipped
footprints checked against full recomputation. The legacy non-race visibility
oracle remains enabled alongside the race fixture.

The matching `d0972306` full ARM replay wins at frame 70,833 with the same
final roster, resources and Chrysalis health as the host proof. All reported
gameplay state matches the earlier `b6d5b616` run at all 73 checkpoints.
Independent arithmetic over the same 69,659 live frames gives 17.21 fps
versus 16.69 fps, a 3.12% increase. The worst complete observed interval is
10.90 fps at scheduler tick 59,000, compared with the earlier 9.03 fps.
The 30-fps target and physical hardware validation remain unmet.

## Packed routes and sight intervals (2026-10-23)

Experimental Z3 now enables three-bit route fields and exact row-span sight
updates. Three-bit fields hold 24 destinations in the space previously used
by 18 four-bit fields; directions, unreachable markers and BFS tie order
remain unchanged. The rare blocked-start walk stays in ROM. Asset-free
ASan/UBSan fixtures check all packing offsets, independent BFS directions,
blocked seeds, capacity-plus-one LRU eviction and reset for standard, T3 and
large-map field sizes. Other ROM targets retain their existing defaults.

Sight updates use exact integer row intervals to visit cells entering or
leaving the existing circles. Full-recomputation oracles check all fractional
offsets, small moves in both directions, overlapping sources, clipped map
edges and race town-hall sight. Both legacy and race visibility fixtures run
with and without the optional span implementation.

The optimized `ece77ae2` full ARM replay wins at frame 70,833 with the same
1,388-input controller trace. Every reported gameplay field, including entity
records, matches both earlier full runs at all 73 checkpoints. Independent
timing arithmetic over the same 69,659 counted live frames gives:

| Z3 build | ROM hash prefix | Weighted fps | Worst complete interval fps |
| --- | --- | ---: | ---: |
| Earlier plain build | `b6d5b616` | 16.69 | 9.03 |
| HUD and expanded four-bit cache | `d0972306` | 17.21 | 10.90 |
| Three-bit cache and sight intervals | `ece77ae2` | 19.19 | 15.23 |

The latest run counts 60,887,297,856 timer cycles. In a separate same-save
late-mission comparison over 9,000 live frames, the four-bit cache measured
19.30 fps, the three-bit cache 19.78, and three-bit plus sight intervals 20.42.
Those unattended tail runs all lose at frame 67,997 and have matching reported
gameplay state; they are performance comparisons, not completion proofs.

The latest Z3 ARM build uses 388 bytes of data and 249,512 bytes of BSS.
IWRAM ends at `0x03004ff8`, within the unchanged `0x03005000` stack guard.
The large-map probe with three-bit routes and baseline visibility also fits
its guards; the combined optimization is currently enabled only for Z3.
Default build/tests, unified build/tests and the Z3 runtime suite pass,
including 51 Python checks and controller-only host completion under
ASan/UBSan. Default and unified ROM hashes remain unchanged. A final source
rebuild matches the measured Z3 ROM byte for byte. Near 30 fps and physical
GBA validation remain unmet.

## Bounded replay memory (2026-10-23)

Experimental Z3 now enables `SC64_Z3_WINDOWED_SCHEDULE`. Its linked
`ee035b5c` candidate reduces BSS from 249,512 to 235,184 bytes, a net saving of
14,328 bytes. Its event array is 2,048 bytes; two four-byte control variables
replace the 16,384-byte array. IWRAM still ends at `0x03004ff8`, and neither
memory guard was relaxed. Ordinary profiles keep their previous scheduler.

Asset-free tests compare held and pressed masks at every tick against the
old preload loop across 40 randomized 50,000-tick streams with more than 256
total events. Boundary, empty-window, capacity and malformed-row checks run
under ASan/UBSan. A separate harness runs the actual `main` loop for paged
and count-only schedules, including physical/injected input after deactivation.
Sixteen Python replay checks cover validation, ELF detection and page slicing.

An ARM prefix with the unchanged 1,388-input proof trace completes 2,500 ticks,
including a partial last page. Its reported gameplay state matches the prior
optimized ROM at shared ticks 0, 1,000 and 2,000. The normal START/B controls
save at frame 2,478 after observer frame 2,477.

The complete windowed ARM replay also wins at frame 70,833 after 71,186
scheduler ticks. It consumes all 1,388 transitions in the unchanged proof
trace, with a peak window occupancy of 49. All reported gameplay state matches
the prior optimized build at all 73 checkpoints. Independent arithmetic gives
19.19 fps over 69,659 live frames and 60,887,859,641 timer cycles; the worst
complete interval remains 15.23 fps. The memory saving does not establish a
frame-rate improvement. The final default Z3 rebuild matches this measured ROM.
Default build/tests (56 Python checks), unified build/tests, the Z3 runtime
suite and the P3 Scourge fixture pass. The complete host input trace remains
byte-identical, and default/unified ROM hashes remain unchanged. Physical
hardware checks and the near-30-fps target remain outstanding.

## Higher Ground Scourge prerequisite (2026-10-23)

The P3-gated Scourge weapon uses pinned BWAPI facts: 110 normal damage, no
upgrade bonus, one-frame source cooldown and three-pixel source reach, scaled
to two engine pixels. It targets hostile, vulnerable air units and consumes
the attacker after a landed hit through the existing shield/armor/death path.
Ground targets and structures remain invalid. Ordinary profiles keep the
previous combat path; this does not enable mission 22.

`make test-sc64-p3-scourge` runs ASan/UBSan checks for ground/building/friendly
and invincible rejection, cooldown/range, shields/armor, fixed damage with
weapon upgrades, attack-move approach, lethal hits and supply cleanup.
The P3 large-map ARM probe links with IWRAM ending at `0x03004f28` and BSS
ending at `0x0203d248`, within the unchanged guards. This is a code/storage
probe, not an ARM gameplay or Higher Ground completion proof.

## Experimental Higher Ground runtime (2026-10-23)

`make protoss-p3 test-sc64-p3` now builds a separately gated P3 runtime and
checks its menu, full imported roster, diplomacy, result actions and SRAM.
The real-controller smoke test launches mission 22 from campaign page two,
checks all 96 units, 42 buildings, 48 mineral patches and three geysers against
their retained source records, constructs a paid Pylon and queues a Probe.
Both computer owners remain Zerg, allied with each other and hostile to the
Protoss player. A separate diagnostic observed paid Zergling pairs from both
forces' own wallets at the production cadence and mining orders for all eight
starting enemy workers.

Asset-free adapter tests cover the source AnyUnit defeat and Foes Buildings
victory predicates, independent one-shot grants, bounded paid production and
presentation holds. They intentionally arrange terminal populations to check
the predicates; they do not prove a gameplay win. Save checks cover the 12,924
byte record, pending action timers, results, owner races and wallets, earlier
generation rejection and corruption rejection before world mutation. HUD
fixtures compare immediate result-sequence changes with the uncached pixel
oracle. Full controller completion is still pending.

The final UI candidate has SHA-256
`f3115b437f87e099b725a3d17899be3675e956b3fc6eb073f9c49ade78cb003a`.
It uses 250,128 BSS bytes and 388 data bytes; IWRAM ends at `0x03004e80` and
BSS ends at `0x0203d298`, within both unchanged memory guards. Its predecessor
`077d359b` ran a 3,000-tick ARM opening at 19.86 effective fps, retaining both
forces and reaching 100 live units after paid production. START/B saved frame
3,004. A fresh emulator process running the final UI candidate restored that
exact frame and the P3 model (`fired=7`, running, no pending action), then ran
another 1,000 live ticks at 19.72 fps. No gameplay state was injected.

Default build/tests (58 Python checks), unified build/tests and the full P3
suite pass. The default and unified ROM hashes remain unchanged. The sprite
importer's reversed Defiler/Scourge labels were corrected without changing any
of the 88 distinct raw-ID pixel payloads or their geometry. This is a label
correction, not a change to the imported artwork.

Higher Ground remains experimental: the source AI interpreter, original
transmission dialogue/audio, a full controller win, near-30-fps performance
and physical GBA validation are not complete.

## Higher Ground controller win and defense optimization (2026-10-23)

The complete normal-controller host policy wins Higher Ground at frame
151,088 (scheduler tick 152,107) under ASan/UBSan. It uses the menus to build
Pylons, Gateways, an Assimilator and Cybernetics Core, assigns gas workers,
trains Zealots/Dragoons, defends the base and attacks both Zerg colonies. All
38 hostile buildings are destroyed; 14 player units, 11 player buildings and
seven Force 2 units remain. The final wallets are 44 minerals and 1,048 gas.
No unit, building, resource or objective state is written by the policy.
The original and optimized host builds produce the same complete trace:
5,809 input transitions, SHA-256
`490656dd167127e879126a2690ac32691e2a6f0d25ddb9f4b65b8f7de8a4a462`.

P3's 24 complete defensive buildings previously scanned all 128 unit slots
per frame, including allied forces. An opt-in P3 path builds two sorted
candidate lists once per call and retains the original slot order, strict
range, air/ground, invincibility, cooldown, tie and per-hit death behavior.
Randomized and dense differential fixtures compare the complete world and
tracers with the original scan. A diagnostic same-save comparison reduced
defense time from 120,824 to 30,857 cycles at the last observed frame, raising
effective performance from 19.72 to 28.92 fps over 1,000 scheduler ticks.
Both checkpoints match the baseline's reported gameplay fields and RGB pixels.

The live renderer can now select its backbuffer without clearing pixels that
the map/HUD immediately replace. Forty-seven scene/cache fixtures and seven
actual-render transitions match a cleared reference over poisoned buffers,
including both pages, camera edges, fog, overlays, pause, results and title.
The title keeps its regular clear. In the same diagnostic scene, removal of
the redundant DMA clear and rejection of a slower minimap batching experiment
reduced draw time from 302,255 to 268,129 cycles. The minimap experiment is not
enabled or retained in production source. These changes preserve state and
pixels; a full-mission performance result is still pending.

The P3 paged scheduler reuses the Z3 implementation, preserving held buttons
and press edges across empty windows. The actual-main fixture passes for
both profiles and legacy count-only scheduling. Its 256-entry queue admits
the host proof trace's peak of 90 transitions per page. The final P3 image is
SHA-256 `9cc2691671d948fccac09ab9187055a50a0c5a539b130ab48bc33e3bbe2fefea`;
it uses 235,800 BSS bytes, ends IWRAM at `0x03004e80` and BSS at `0x02039aa0`,
within both unchanged guards. The canonical rebuild matches this frozen ROM
byte for byte. Default, unified and Z3 ROM hashes remain unchanged.

Default build/tests, unified build/tests, the full P3 suite and 65 Python
checks pass. The P3 replay tool validates input transitions, ELF queue limits,
catalog availability and final quest/roster/wallet state. Its SRAM checks use
the human globals as the authoritative wallet; the owner arrays may retain
their initialization values for SIDE. The complete ARM controller replay is
running separately. A host win and this bounded 28.92-fps scene do not certify
the full ARM mission, its frame rate, or physical GBA performance.

## Higher Ground ARM completion and durable replay evidence (2026-10-23)

The frozen `9cc2691671d948fccac09ab9187055a50a0c5a539b130ab48bc33e3bbe2fefea`
ROM completed the full 152,107-tick controller replay in mGBA at game frame
151,088. The retained final observation has the P3 completion bit, won result,
quest outcome 1 with no pending hold, 14 player units, 11 player buildings,
seven Force 2 units and no hostile buildings. The global wallets are 44
minerals and 1,048 gas, matching the host policy's final contract. The final
capture displays mission clear and the game-saved message.

The run's original final verifier treated the host trace's Gateway presence
flag as an exact live count. Three surviving Gateways therefore caused a
reporting exception after gameplay had won. The corrected presence check
passes against the retained final observation, and a regression rejects zero
Gateways. This proves the observed ARM completion, but the old runner retained
only its latest checkpoint: complete checkpoint equality and an exact aggregate
full-mission frame rate cannot be recovered from this run. They are not claimed.

The runner now writes and flushes each checkpoint to a separate JSONL history,
persists the full report before final validation, and retains failure details
when validation raises. The comparison tool checks trace identity, matching
checkpoint extents, typed actor summaries and all reported gameplay fields;
timing/capture metadata is excluded. Prefix comparisons require an explicit
flag and cannot claim a win. This is reported-state comparison, not full RAM
equality. Eight replay-verifier and 11 comparator tests pass.

## Higher Ground rendering experiments (2026-10-23)

The opt-in `SC64_TERRAIN_SCROLL` path reuses the existing 240×108 cache across
small camera moves, redrawing uncovered or changed-fog tiles. Stationary views
retain the simple band-copy path. `SC64_STATIC_KEY_CULL` snapshots exact
drawable-object fields and includes raw building art identity; offscreen
payload changes do not invalidate the cache. No additional framebuffer is
allocated. The actual imported renderer matches a fresh-render pixel oracle
across 97 scene transitions, including camera edges, fog, ownership, depleted
resources, overlapping static art and raw-art changes. Asset-free cache
fixtures also cover both unchanged and scrolling paths under ASan/UBSan.

`SC64_VISIBILITY_SPANS` is checked against the byte-grid reference on the full
128×96 packed map, including edge, overlap, movement, death, ownership, reset
and randomized cases. With spans enabled for P3, the fallback circle routine
stays in ROM to preserve the unchanged IWRAM reserve. Other profiles retain
their original placement.

The frozen combined candidate has SHA-256
`d5ebd9cc03705546d9f6fba301991110d1d02daa9f1e6b539047cdec84877f67`.
IWRAM ends at `0x03004f88`, BSS at `0x020399e0`; BSS is 235,608 bytes and data
388 bytes. Both unchanged linker guards pass. Against the frozen `9cc26916`
baseline, the same 10,000-tick controller prefix improves from 21.53 to 23.39
effective fps over the same 8,889 counted live frames. All 11 reported gameplay
checkpoints match, as do RGB captures at HOME, tick 1,000 and tick 10,000. At
the last checkpoint, visibility falls from 62,104 to 36,625 cycles and drawing
from 248,385 to 216,755 cycles. This is an opening-prefix measurement, not a
full-mission frame-rate result. The full candidate replay is being validated;
the flags remain opt-in. Default build/tests (79 Python checks), unified
build/tests and the full P3 suite with all three flags pass. The host policy
still wins at frame 151,088. Default and unified ROM hashes are unchanged.

## Higher Ground complete scroll/span replay (2026-10-23)

The `d5ebd9cc03705546d9f6fba301991110d1d02daa9f1e6b539047cdec84877f67`
candidate completed all 152,107 scheduler ticks and passed the final victory
contract at game frame 151,088. Its durable report and JSONL history retain
154 observations. The final model is fired 31, outcome 1, sequence 5 with no
pending transmission or hold; all 38 hostile buildings are destroyed, and
14 player units, 11 player buildings and seven Force 2 units remain. Global
wallets are 44 minerals and 1,048 gas. The final result's RGB pixels match the
earlier `9cc26916` baseline.

The complete run averages 23.28 effective fps over 150,004 counted live frames;
its worst observed interval is 17.72 fps. Initial HOME and result-transition
intervals are excluded as specified by the report. The report validates its
full checkpoint structure, but the old baseline has no complete history, so
full-run checkpoint equality is not claimed. The shared opening prefix and
the final contract/pixels supply the cross-build comparisons described above.
The canonical default P3 rebuild matches the frozen candidate byte for byte.
Terrain scrolling, drawable-object keys and visibility spans are now P3
defaults. Near-30-fps mission performance and physical hardware proof remain
outstanding.

A separate RAM-composed minimap experiment matched 401 renderer fixtures,
all 11 opening checkpoints and three RGB captures, but regressed the same
10,000-tick prefix from 23.39 to 22.82 fps. Its last observed HUD time rose
from 82,595 to 98,684 cycles. The experiment was removed rather than enabled.

## Jacobs Installation import stage (2026-10-23)

The private race-map packer now imports T4 from the verified USA ROM using the
unchanged pinned extractor. `make test-sc64-t4-import` checks all 95 non-start
placements, their canonical row digest, all 16,384 terrain cells, the converted
terrain digest and the source owner/type counts under ASan/UBSan. Eight raw
type-195 special Terran Beacons remain unclassified rather than being silently
mapped to an ordinary unit or building. Compiler-matrix tests keep global ID
7 stable and restrict its data to the explicit Terran import profile.

The packer retains exactly one each of UNIT, DIM, ERA, MTXM, MRGN, DD2, THG2
and TRIG as original framed records under ignored local storage, with source
ROM/extractor/CHK provenance and per-record hashes. Asset-free fixtures check
the exact framed writes and rejection of a missing required record before any
output directory is created. No extracted records or generated arrays are
committed. The catalog does not enable T4; the runtime described below is an
internal world probe only.

## Jacobs Installation actor/world probe (2026-10-23)

`make test-sc64-t4-actors` checks the hidden internal T4 load and actor/world
rules under ASan/UBSan. The roster fixture loads all 95 non-start source
actors (87 unit records and eight Beacon buildings), retains source ownership,
and verifies the full 128×128 geometry. Actor fixtures cover selectability,
movement/collision roles, hidden Map Revealer behavior, sight circles,
Ghost weapon and bounded integer concussive damage. The concussive helper uses
integer division with a minimum one HP; source fractional damage can differ by
one HP. It does not model cloak, lockdown, or nuclear abilities.

Rescue Passive ownership conversion is exercised through the real T4 adapter:
individual contact uses an inclusive 32-pixel square in GBA coordinates,
derived from the source hook's 64-pixel half-extent. This is an authored
center-contact approximation to the opaque native bounds query, not a claim
that its edge or enumeration behavior matches the original engine. Fixtures
also check chain rescue, same-source-slot depot transfer, supply recount and
stale allied attack-target cleanup. Rescue conversion does not advance shield
or larva timers; the enclosing update advances them once. The private probe bypasses only the
frontend availability check; it does not make T4 selectable or prove a
controller-play win.

Special Terran Beacons retain their canonical 100,000 HP in live state. Since
the save record stores health words as signed 16-bit values, save writes zero
for the Beacon's HP and max-HP fields only after validating its source type,
invincibility and other canonical state. Load validates that encoding and
restores both values to 100,000. Malformed live Beacon state is rejected rather
than normalized during save.

Door and trap collision/state, teleports, source trigger actions, objective
logic and mission completion remain unavailable. The T4 probe makes no
playability or frame-rate claim.

The actor-stage ARM probe retains 249,304 BSS bytes and 364 data bytes, with
IWRAM ending at `0x03004f78` and BSS at `0x0203cf48`. The unused fallback circle
renderer stays in ROM for this profile; both unchanged stack/memory guards
pass. The default build/tests (83 Python checks) and unified build/tests pass.
The default, unified and P3 ROM hashes remain unchanged.

## Jacobs Installation storage integration (2026-10-23)

`make test-sc64-t4-storage sc64-t4-storage-probe` checks the separately gated
128×128 profile and links the full ARM engine without exposing T4 in the
catalog. Fourteen packed route fields consume 86,016 bytes. The target index
uses 32×32 cells, and the minimap scales 128 rows into its existing 24 rows.
The latter is checked against exact coordinate arithmetic: the previous
96-row shortcut was invalid for this new geometry. Save geometry validation
accepts 128×128 only in T4, whose generation 8 is distinct from other profiles.

The host integration checks the bottom-right cell in exploration, visibility,
path walkability, target indexing and minimap occupancy, plus persistence of
the last packed fog bit through an available Terran mission's save/load.
It is a storage fixture, not a Jacobs Installation load/resume proof. Packed
grid guard-byte tests cover all 16,384 cells. Route tests check packed values
through cell 16,383 and boundary BFS paths; both ordinary and span visibility
paths match the byte-grid reference at 128×128 under ASan/UBSan.

T4 now uses the supported paged scheduler with 256 entries; the unchanged
2,048-entry queue would exceed the EWRAM guard. The actual-main fixture checks
held keys, press edges, empty pages and legacy count-only use for T4 as well
as Z3/P3. Wrong-profile and missing-prerequisite compiler probes reject invalid
configurations. The earlier storage-stage ARM probe used 249,304 BSS bytes and 364 data
bytes, ends IWRAM at `0x03004e68` and BSS at `0x0203cf48`, and passes both
unchanged linker assertions. This is an ARM link/storage result, not gameplay
or frame-rate evidence. The subsequent actor stage above supplies actor and
rescue handling; doors, traps, teleports and triggers still prevent T4 playability.

At that storage stage, default build/tests (80 Python checks), unified build/tests and the existing
128×96 storage fixture pass. Default and unified ROM hashes remain unchanged.

## T4 numeric scenario and execution core (2026-10-23)

`tools/sc64_t4_scenario_pack.py` validates the retained record framing and
manifest hashes before emitting private immutable C tables. It preserves all
255 location records, 38 trigger records with fixed 16/64 condition/action
slots, player enables, execution flags, action cursor and opaque trailing
bytes, plus all 32 THG2 records. An independent C fixture reconstructs the
complete TRIG, MRGN and THG2 payloads and checks canonical digests. The data
contains 48 nonzero conditions and 141 nonzero actions. THG2 contains 26
unit-backed entries and six sprite entries; two unit entries start disabled.
Source arrays and generated headers remain ignored.

Native record-conversion and dispatch research resolved two operands used by
the source: opcode 42 state 0 toggles, and opcode 11 count 0 creates one unit.
The pure action fixtures check count bytes, enable/disable/toggle states and
all 16-bit THG2 flag words, including rejection of sprite-only entries.
Applied to the private scenario operands, those helpers identify 20 toggles
and five requested creations. They do not apply effects to the game world.
Provenance and the native addresses are in [References](REFERENCES.md).

The asset-free trigger runner checks source order, conditions, global switch
indices 0–255, one-shot/preserved execution, shared waits within each player,
independent player clocks, and retry without repeating completed effects.
Synthetic fixtures compare independent resumed state, switch banks and effect
logs through waits and allocation retries. Malformed state and unsupported
conditions/actions are rejected. A disabled final action slot still finishes
the trigger. The bounded state costs 144 bytes per player and 32 shared switch
bytes. The nominal 30 Hz clock and 30-tick condition pass are authored GBA
policies; they have not been compared with the N64 scheduler.
The shared-wait fixtures also retain a completed marker when an earlier
source-ordered trigger starts a new wait first. Callback return values must
match their documented success/retry contract; other values fault without
advancing the action cursor.

The query core checks source-pixel actor bounds and rectangle overlap, exact
edge exclusion, current owner/force membership after rescue, inactive slots,
and unsupported selectors. Integer overflow inputs and feature-off includes
are checked. These are helper tests, not movement or mission-completion tests.

At this execution-core checkpoint, the runner was not yet wired into T4
gameplay or SRAM. The later gated runtime section records the subsequent
integration. No gameplay, ARM memory or frame-rate claim follows from these
earlier fixtures alone.

Default build/tests (87 Python checks), unified build/tests and the existing
T4 actor/storage checks pass. Default and unified ROM hashes remain unchanged.
A separate ARM compilation checks the new core schemas and state-size
assertions; the core still needs a full engine link and gameplay validation
when it is connected to the mission.

## T4 setup records and scenario props (2026-10-23)

T4 source retention now covers 13 framed sections, adding FORC, OWNR, SIDE,
UPRP and UPUS. The scenario importer validates their framing, sizes and
manifest digests, retains all 64 property slots and checks force/used indices.
Independent C reconstruction now checks all eight scenario/setup payloads,
including unused property fields. Generated records and arrays stay private.
The setup records corroborate the existing source-slot/force mapping.

The trigger runner retains AllPlayers at selector 17 and Forces 1–4 at
18–21. Exhaustive enabled-selector tests cover eight owners and four forces;
programs with unsupported enabled selectors are rejected. T4's AllPlayers
records require independent player contexts, rather than a single global
cursor. A 1,268-byte prospective world-effect state holds eight contexts,
shared switches and bounded mutable locations/doodad bindings; it is not
allocated in the current game or persisted yet.

T4-only rules now distinguish eight door/trap types with verified raw IDs,
bounds, sight, armor and HP. Doors retain their intrinsic invincibility and
100,000-HP sentinel. They use the compact building pool; this does not imply
the original unit metadata classifies them as normal buildings. Scenario
props cannot be constructed by a worker. Private sprite extraction retains
every source frame for these eight types and exposes a checked frame accessor.
No open/closed frame meaning, door collision or trap combat has been assigned.

The actor/storage ARM probe still passes both unchanged memory guards. These
checks remain import, rule and storage evidence, with no new T4 playability
or frame-rate claim. The default and unified ROM hashes remain unchanged.

## T4 world-effect callback fixtures (2026-10-23)

`make test-sc64-t4-effects` exercises the separate callback adapter against
synthetic live engine states under ASan/UBSan. Bring uses validated 1-based
location IDs and source actor bounds; Command ignores its unused location
operand. Unsupported owner/type/geometry queries leave the output untouched.
Removal selects actors before mutation, recomputes supply and clears only
the appropriate stale attack/build/gas targets, preserving mineral indices.
Camera and presentation state affect only the human context.

MoveLocation preserves and clips location 30's extent. Move Unit teleports
all matching Men using an authored bounded nearest-fit search at the
destination, clearing old orders/path state. One fixture leaves only one
feasible destination for two actors: failure to place the second returns a
retry with both actors unmoved. Candidate order and placement are GBA
policies, not a reproduction of the native placement algorithm.

Creation and doodad effects are stubbed at the callback boundary in this
fixture. The separate property-index helper and private scenario test verify
that action numbers 1–64 map to UPRP slots 0–63; the source requests one
property-backed creation among five total. Applying those property overrides
is still required. An ARM compile-only check covers the callbacks and schema,
with the same stub boundaries; it is not a full runtime link or memory result.
At this callback-fixture checkpoint, the game did not call these callbacks or
persist their state. The later runtime integration is documented below; this
fixture itself remains callback evidence, not gameplay or performance proof.
The default build/tests pass, including 89 Python checks; its ROM hash is
unchanged. Unified build/tests also passed during this milestone.

## Gated T4 scenario runtime (2026-10-23)

The current opt-in scenario profile binds all 95 imported UNIT rows and 26
unit-backed THG2 props into the fixed pools: 121 initial actors total (87
units and 34 buildings). T4 remains excluded from the frontend catalog, so
this is internal runtime evidence rather than a playable T4 release.

With the locally prepared T4 inputs, run the scenario packer, host integration
checks and ARM scenario link probe with:

```sh
make sc64-t4-scenario \
  test-sc64-t4-effects test-sc64-t4-creation test-sc64-t4-traps \
  test-sc64-t4-prop-art test-sc64-t4-scenario-save test-sc64-t4-runtime \
  sc64-t4-scenario-probe
```

The runtime fixture checks source roster/slot bindings, all 20 retained
doodad-state actions, door collision and path-grid changes, creation and map
revealer handling, and pause/save/load continuity. Save generation 9 persists
the trigger and doodad state. Its visual adapter applies explicit immediate
state: enabled/closed doors draw source frame 0; disabled/open doors draw frame
9; paired right-facing door types are mirrored. Enabled traps draw static
frame 0, while disabled traps are hidden. This does not reproduce N64 IScript
timing or trap animation.

The win/loss fixtures directly place the human actor at the completion
location or remove Raynor to test the integrated trigger results and save
state. They are synthetic rule-integration checks, not controller-played
mission completion. Full native trigger eligibility, physical GBA timing,
catalog availability and near-30-fps performance are not claimed.

The default build/tests (including 89 Python checks), unified build/tests,
older T4 actor/storage fixtures and all focused scenario fixtures pass. The
scenario ARM link contains 1,509,984 bytes of text, 364 bytes of data and
251,840 bytes of BSS. IWRAM ends at `0x03004fc8`; BSS ends at `0x0203d930`.
The existing stack/reserve guards remain unchanged. These are link results,
not measured gameplay timing. Default and unified ROM checksums are unchanged.

## Internal T4 controller replay and validation cost (2026-10-23)

A separate `sc64-t4-replay` target adds a HOME START+SELECT route to the T4
briefing. This build-only entry leaves the ordinary catalog unchanged. Its
on/off frontend fixtures are asset-free. A live minimap ping now consumes the
saved coordinates/timer, with framebuffer checks for scaling, pulse, clipping,
expiry and mode suppression.

The controller attempt runner deploys through this entry and uses normal
selection and attack/move controls. The ARM replay helper validates its private
input trace, observes all eight trigger contexts and source actor identities
through DWARF, and compares the recorded checkpoint fields. Twelve asset-free
Python checks cover trace rejection, state decoding and result verification.
The group attempts reach corridor doors and a teleport, but a normal-controller
T4 win remains unproven. A recorded loss or unfinished attempt must retain that
result in the report.

The trigger runtime now uses a prevalidated step after full initialization or
save/load validation. The immutable ROM program and persisted state retain
their full validators; scalar/wait bounds are checked every tick, with
cursor/flag checks before ready-pass accesses. The standalone checked entry
point retains its validation behavior. Differential host execution and corrupt
scalar/index fixtures pass under ASan/UBSan.

On the same private 5,000-tick solo controller prefix (trace SHA-256
`307c16e5df97ebfd8d6f3c193d87edea2c349c7c68d8dde34b3db19f38b86f91`),
the internal ARM build improves from 13.35 to 17.19 effective fps over 3,997
counted live frames. All six observations match the compared gameplay fields,
including actor pools, campaign and complete T4 trigger/world state; all three
PNG captures match exactly. Baseline ROM hash begins `d00ebdec6c5b`; candidate
hash begins `f1c25b2b9043`. The measured prefix includes the controlled actor's
death and idle frames, so it establishes neither full mission performance nor
near-30-fps operation. Hardware timing remains unverified.

A separate group-controller ARM replay matches its host checkpoints through
the recorded Raynor loss at frame 10,156 (12 observations including boot and
the final result). The report correctly records `lost`, human trigger outcome
2, campaign phase 4, no trigger faults and `verified_win: false`.

The default build/tests (101 Python checks), unified build/tests and focused
T4 scenario checks pass. The internal replay ARM build uses 1,510,900 text
bytes, 388 data bytes and 251,840 BSS bytes. Its IWRAM end is `0x03004ff8` and
BSS end is `0x0203d948`; the existing reserve guards are unchanged. Standard
default/unified ROM checksums remain unchanged.

## T4 trap candidates and cached prop rendering (2026-10-23)

Trap targeting now builds one bounded, lazy stack-local list of eligible
actors per trap update. Every trap still checks current target life, inclusive
range and the original lowest-slot tie order. Differential fixtures compare
the candidate path with the original full scan across 80 deterministic random
worlds, edge/tie cases and a target killed between consecutive traps.

The static terrain keys now include mutable T4 doodad state and bindings, plus
raw building identity. This fixes stale door/trap art after a state change.
Eight ASan/UBSan framebuffer variants cover all combinations of terrain
scrolling, frame-clear suppression and static-key culling. Each compares
poisoned alternating buffers with an independently cleared fresh-render oracle,
including both toggle directions for all eight prop types and save/load.

On the same 5,000-tick solo prefix recorded above, the corrected trap candidate
build (`4850e084e6fd0c637d516cb4e6041a4f07ab2f76b87d006917dd8f6072aeefe9`)
averages 23.38 effective fps over 3,997 counted live frames, compared with 17.19
for the prevalidated build. All six compared gameplay observations and all
three PNG captures match. This prefix does not establish full mission
completion or near-30-fps operation; the prop fixtures separately exercise
the corrected visual invalidation.

Default build/tests (101 Python checks), unified build/tests and focused T4
runtime, save, trap, prop and ping checks pass. The internal ARM build uses
1,511,116 text bytes, 388 data bytes and 252,352 BSS bytes. IWRAM ends at
`0x03004ff8`, BSS at `0x0203db48`; both unchanged reserve guards pass. Default
and unified ROM hashes are unchanged. Diagnostic-only counters split rescue,
trigger and trap work without adding counters to release builds.

## T4 per-tick query cache and render options (2026-10-23)

The internal T4 profile now enables a bounded stack-local count cache shared
across its eight trigger contexts. It keys immutable condition records by
pointer and, only for Current Player selectors, execution owner. Every
delegated action invalidates it before invocation, including retry/failure;
each tick starts fresh. Capacity-full queries fall back to the original count
callback. The flag-off path remains available through
`make -B sc64-t4-replay T4_OPTIMIZATION_FLAGS=`.

The asset-free ASan/UBSan cache fixtures cover owner separation, tick reset,
failure output, capacity fallback, invalid owners, corrupt bounds, source-order
execution and action invalidation. They are included in `./dev test`.
Cache-enabled full-runtime and save fixtures also pass. A GCC static stack
audit reports a 1,264-byte `race_t4_tick` frame and an estimated deepest
trigger-action path of about 2.6 KiB within the unchanged 11 KiB reserve. This
is compiler frame accounting, not a runtime high-water measurement.

The same 5,000-tick prefix yields these effective ARM frame rates over 3,997
counted live frames:

| Internal T4 build | Effective fps |
| --- | ---: |
| Trap candidates and corrected prop cache keys | 23.38 |
| Plus scrolling, clear suppression and static-key culling | 23.71 |
| Plus count cache only | 25.37 |
| Plus count cache and all three render options | 25.75 |

All six compared gameplay observations and three PNG captures match the
corrected baseline in each comparison. The combined build is now the internal
profile default, with ROM SHA-256
`90ad795fe302267442488a7f0ca27f6374ea786226a8d3367e7c412e980def12`.
The canonical rebuild is byte-identical to the measured candidate. It uses
1,512,204 text bytes, 388 data bytes and 251,912 BSS bytes; IWRAM ends at
`0x03004fc8` and BSS at `0x0203d990`. Both reserve guards remain unchanged.

The longer frozen group trace also matches every shared gameplay field at
all 12 observations, including complete trigger contexts and actor pools;
all four PNG captures match. The optimized replay averages 21.11 fps over
8,980 counted live frames, up from 15.04. It still reaches the recorded loss
at frame 10,156, human outcome 2, phase 4 and no faults, with
`verified_win: false`. Default build/tests (101 Python checks), unified
build/tests and focused T4 runtime/save/trap/prop/ping checks pass. Default
and unified hashes are unchanged. Full mission completion and near-30-fps
operation remain unproven.

## T4 Raynor Marine combat identity (2026-10-23)

The imported raw-20 hero previously shared the legacy raw-19 Raynor type,
which has no weapon entry in this compact ruleset. Consequently attack-move
could not acquire or damage either traps or ordinary hostile units. T4 now
maps raw 20 to a distinct Marine Raynor type with the pinned source's 200 HP,
3 armor, 18-damage ground/air Gauss Rifle, 160-source-pixel range and 15-frame
weapon cooldown, zero supply and no production building. Existing range and
cooldown scaling applies at the engine boundary. Movement retains the engine's
one-pixel Marine approximation rather than reproducing the reference's top
speed/acceleration; the raw-19 alias keeps its previous behavior.

The focused ASan/UBSan engine fixture covers the distinct mappings and
movement classes, target acquisition and actual damage against a gun trap,
ground unit and flying unit, plus friendly and invincible target guards.
Non-T4 rule fixtures retain the legacy mapping. T4 scenario save generation
advances from 9 to 10 and the actor-only profile from 8 to 9, rejecting older
records rather than interpreting the changed compact identity.

The internal ARM link passes the unchanged reserve guards with 1,512,148 text
bytes, 388 data bytes and 251,912 BSS bytes. IWRAM ends at `0x03004fc8` and BSS
at `0x0203d990`. Its ROM SHA-256 is
`63474ac9ffdb559aa01478be2edc5b9b010aa7132694a00458cfb47c66e9a0f5`.
The earlier performance comparisons refer to their recorded ROM hashes;
this combat fix changes gameplay.

The cleaned normal-controller policy verifies its selected actors, uses the
ordinary L-cycle selection control and attack-move commands, and derives gate
and objective targets from the local source records. It wins in both the host
and the final ARM build at frame 9,331 (host tick 9,372). The final private trace
SHA-256 is `ba147b738e47bb684608bd8bdcb48c32b43ee499856933ee4b5a181c12510058`;
it has 154 input transitions, nine periodic checkpoints and a peak of 56
transitions per scheduler page. The ARM replay retains 11 observations and
matches the recorded actor/objective/result checks through the final source
victory: human outcome 1, phase 3, primary objective set, Raynor alive, no
trigger faults and `verified_win: true`.

The completed internal replay averages 23.12 effective fps over 7,971 counted
live play frames, with a worst counted interval of 20.49 fps. This establishes
controller completion in the adaptation, not near-30-fps performance or native
fidelity.
Default build/tests (101 Python checks), unified build/tests and the focused
T4 actor, runtime, save, combat, trap, prop and ping fixtures pass. Standard
default/unified hashes remain unchanged. Standard Terran and the separate
internal replay builds keep T4 gated; the combined campaign build below has a
normal-menu T4 entry.

## Combined Terran T1–T4 campaign (2026-10-23)

`make terran-t4` builds `build/StarCraft-Terran-T4-GBA.gba`, an opt-in ROM
containing Terran 1–4 in one campaign. Jacobs Installation (global ID 7) is
selected through the ordinary campaign menu: HOME, A, RIGHT to page two, DOWN
to T4, A for briefing, then A to deploy. The separate `sc64-t4-replay` target
retains its internal START+SELECT shortcut. The standard campaign catalog
remains unchanged; the standalone replay target keeps a separate internal
entry and save identity.

The combined campaign uses save generation 11 and rejects earlier save
generations, including generation 10 from the internal T4 build. Its
save payload is 16,328 bytes; host save/load and rejection checks pass. A
normal-controller save made by pressing START and then B at frame 1,004 was
loaded by pressing L from HOME in a fresh ARM emulator process. It resumed at
frame 1,004 and continued through frame 2,005 on the play screen. Its SRAM
SHA-256 is
`69e94f1f93e817a0dd90ea881b54080cc070a2a859c4b5ecc85cd93aade5fc89`.

The normal-menu controller run wins at frame 9,331 (host tick 9,378). Its 160
input transitions peak at 62 per scheduler page, and the ARM replay matches 11
observations. The 2 MiB ROM SHA-256 is
`181a5063515a362970ce5902561ddbb61410ad43d91c248160d03439244c78bb`; ELF
SHA-256 is
`5fc7296475ac58c2f20f624ca4bcc7a127b0bb27e01dbeabd6858e7f3a6da5cd`. The ELF
contains 1,857,188 bytes of text, 388 bytes of data and 241,872 bytes of BSS.
BSS ends at `0x0203b258`; IWRAM ends at `0x03004ec8`. Both existing linker
guards pass.

Across 7,971 counted live frames, the mGBA replay averages 23.15 effective fps
with a worst interval of 20.71 fps. This is below the 30-fps target and is
emulator timing, not physical GBA evidence. A separate 10,000-tick neutral
Blood Bath skirmish on the same ROM, with Terran human and three independently
selected Zerg, Protoss and Terran opponents, averages 29.48 fps (intervals
28.58–29.86). That bounded skirmish does not measure full-battle worst case or
campaign performance.

This build joins the current experimental progression: Terran 1–4, Zerg 1–3
and Protoss 1–3 are playable in their respective builds; 20 of the 30
catalogued missions remain unavailable. Physical GBA validation and the
near-30-fps campaign target remain outstanding.

The asset-free Python suite has 103 passing checks. Default build/tests,
unified build/tests, standard race-profile regressions, combined T3 runtime
and save/resume checks, campaign-menu fixtures and the bounded skirmish test
pass. The combined controller replay is separate from those host checks; its
matching ARM observations are listed above.

## Internal T4 visibility-span comparison (2026-10-23)

On a separate internal T4 optimization comparison, the `SC64_VISIBILITY_SPANS`
candidate identified by ROM prefix `caa5de31` averaged 23.41 effective fps,
compared with 23.12 for the comparison build on the same trace. All 11 gameplay
observations and three PNG captures matched. The gain is modest and is
separate from the combined campaign measurement above. This is a
release-candidate optimization comparison, not a diagnostic-counter run or a
claim of near-30-fps operation. The candidate links with IWRAM ending at
`0x03004ec0` and 251,912 bytes of BSS; both unchanged guards pass.

## Combined Terran current default and T5 import-only stage (2026-10-23)

The current combined Terran T1–T4 default enables `SC64_UNARMED_QUERY_SKIP`.
Its 2 MiB ROM SHA-256 is
`d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`, and its
ELF SHA-256 is
`3788c697cafe3f5693ee9b752013a6dee9cdc9a1c6673a90d838d920985d1202`. It
replays the same normal-menu win at frame 9,331, with no faults. All 11
reported gameplay checkpoints and all three captures match the earlier
combined build byte-for-byte. The current replay averages 23.53 effective fps
over 7,971 counted frames, with a worst interval of 21.17 fps. This remains
below the 30-fps target and is mGBA evidence only. The earlier exact
`181a5063` build measured 23.15 fps for this campaign run; its separate
29.48-fps Blood Bath sample is historical and is not a result for the current
ROM. An exact 5,000-tick test of the 576-byte minimap-interior buffer found no
measurable benefit, so that experiment was removed. The current ARM link uses
1,857,244 text bytes, 388 data bytes and 241,872 BSS bytes; IWRAM ends at
`0x03004f00` and BSS at `0x0203b258`. Both unchanged reserve guards pass.

The same current ROM also completes a fresh 10,000-tick neutral-input Blood
Bath skirmish with a Terran human and three independently selected Zerg,
Protoss and Terran AIs. It averages 29.49 fps, with observed intervals of
28.58–29.86; all checkpoints remain in play. This bounded scenario does not
establish full-battle worst-case performance.

Antigan Revolt (Terran global ID 8) has reached a source-import-only stage.
The private race import retains the complete 96×96 Badlands terrain and 168
non-start UNIT records: 79 units, 48 buildings and 41 mineral fields. Four
start markers are excluded from live placements. This stage does not enable
the T5 catalog entry or provide a playable runtime; the project still has 10
playable missions and 20 unavailable missions.

After preparing local ROM-derived inputs, regenerate the race tables and T5
scenario data, then run the focused import and helper checks:

```sh
python3 tools/sc64_races_pack.py --rom /path/to/StarCraft64-USA.z64
python3 tools/sc64_t4_scenario_pack.py --profile t5
make test-sc64-t5-import test-sc64-t5-scenario \
  test-sc64-t5-actor-rules test-sc64-resource-actions
```

The actor-rules fixture covers three source actor definitions. The
resource-action fixture covers only native-backed per-player arithmetic; it
does not implement player-group selection. Cloak and mine-arming behavior are
not established by these checks. Neither fixture demonstrates T5 gameplay or
completion. The import fixture checks every retained placement field and all
9,216 converted terrain cells/pixels against recorded digests. The numeric
fixture reconstructs all eight scenario/setup payloads and validates initial
trigger contexts for all eight source slots. Default checks pass, including
105 Python tests; unified and all three standard race build/test suites pass.
Their ROM hashes are unchanged. Combined T3 survival, T4 completion,
save/resume, targeting and all 18 skirmish launch combinations pass as well.

## T5 internal engine runtime (2026-10-23)

The opt-in `SC64_T5_RUNTIME` stage loads the complete imported roster and
binds the retained T5 program to its own source-player contexts, switches,
queries and world effects. It is separate from the playable campaign catalog.
`make test-sc64-t5-engine test-sc64-t5-world test-sc64-t5-combat` passes with
ASan/UBSan and `-Werror`. Direct-state fixtures cover all 168 placements,
the initial human grant and region-gated enemy grant, Kerrigan rescue and the
rebel-base scripted sequence, source building-based victory, defeat predicates,
atomic rejection of unsupported T5 saves, paid AI production and rescue policy,
and actual hero/mine combat. These fixtures do not establish controller
completion or full mission balance.

The 4 MiB internal auto-entry ARM probe has ROM SHA-256
`3c747e68fbde8237439a5a7a1cf4d228e9ccdacbe2a7abbfe9e1b6e6c2f57e5f`
and ELF SHA-256
`d1ea2951ee8c380b806231b117609377a09807311410772b15fce152719ff54a`.
Run `make sc64-t5-engine-probe`, then
`python3 tools/sc64_t5_engine_probe.py --ticks 3000`.
A 3,000-tick neutral-input mGBA run remains in play at every checkpoint with
no trigger faults. It averages 28.19 effective fps, with 1,000-tick intervals
of 27.72, 28.46 and 28.41 fps. This is an opening-area engine probe; it does
not cover a completed campaign or a full battle. The ARM link has 2,231,124
text bytes, 388 data bytes and 243,208 BSS bytes. IWRAM ends at `0x03004fe8`
and BSS at `0x0203b790`; the unchanged reserve guards pass.

Default build/tests (including 105 Python tests), unified build/tests, combined
T3 survival/save/resume/target/HUD checks, T4 controller/save checks, all 18
skirmish launch combinations and T5 import/scenario checks pass. The playable
combined T1–T4 ROM remains byte-identical with SHA-256 `d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`.
The Git source audit passes. No imported assets, binaries, captures, source
scenario records or saves are indexed.

At this stage T5 remained gated pending persistence and ordinary-controller completion.
Opaque AI scripts are explicit authored substitutes. Cloak, mine detection
and animation fidelity, unique hero art, THG2 sprite decorations and full
source movement/sight policies remain incomplete. Mine damage and movement
retain the documented integer and 30 Hz approximations. Physical GBA
validation and near-30-fps full-campaign performance remain outstanding.

## T5 compact persistence (2026-10-23)

The persistence-stage T5 profile used save generation 13 and retained the combined
profile's 16,328-byte payload. T4 and T5 share the scenario region in SRAM.
T5 stores its four executing source-player contexts in 764 bytes, including
switches, resource-action snapshots, accepted AI requests and presentation
timers. Omitted contexts must match their initial state and are reconstructed
on load. Unused payload bytes must be zero. The normal T1–T4 build retains
generation 11 and its existing ROM hash.

`make test-sc64-t5-save` runs against the real engine under ASan/UBSan and
`-Werror`. It checks round trips, active trigger-Wait continuation, both
active mine phases, win/loss saves, pause-menu commands and other mission
saves. Corrupted candidates are rejected before live world mutation. Validation ties the
campaign and AI clocks to the saved frame and reconstructs accepted scripts,
switches and exact resource grants from completed action prefixes. The tests
use direct actor placement for some trigger states; they do not demonstrate
controller completion.

The persistence candidate is a 4 MiB internal auto-entry ROM with SHA-256
`d922d1136b3239f1c9b1c5aa9127873598982e6ea4e357f926e3e0e6490812ba`
and ELF SHA-256
`8876f74a8c04f2b697cb1b65acebe19ac4611a55fe51ff97e8062ca8b463582c`.
Its text/data/BSS sizes are 2,233,840/388/243,208 bytes; IWRAM and BSS ends
remain `0x03004fe8` and `0x0203b790`. The unchanged reserve guards pass.
`tools/sc64_t5_save_smoke.py` uses START/B to save at frame 122, closes mGBA,
copies its flushed SRAM to a fresh process, and uses START/L to load. The
persisted gameplay state matches at frame 122 and continues exactly 60 ticks
to frame 182, with no trigger faults and matching campaign/AI clocks. Only
controller-input injection is written through the debugger; scenario state
inspection is read-only. Auto-entry is internal, so this does not establish
normal-menu availability or a controller-completed mission.
A separate 3,000-tick neutral-input ARM run averages 28.19 effective fps
(intervals 27.72, 28.46 and 28.41) with no trigger faults. This measures the
opening area, not a campaign completion or full battle.

## T5 controller route and transport requirement (2026-10-23)

The bounded host controller probe runs 60,000 input ticks under ASan/UBSan.
It rescues Kerrigan, executes the rebel-base trigger sequence and the gated
enemy grant, then queues SCVs and Marines with paid normal-controller orders.
It ends in live play at frame 59,918 with Kerrigan alive and no trigger faults;
21 enemy buildings remain. This is an unfinished controller route.

A terrain audit of the verified `008/00E` map finds the remaining enemy base
in a separate ground component using both the coarse GBA grid and the source
VF4 minitile walk masks. Ground routing alone cannot complete this route.
This finding motivated the internal transport stage below; the terrain has
not been altered to bridge the islands. The five-mission menu has guarded asset-free tests,
including its single-row third page, but the T5 entry flag stays off in ROM
targets. The playable campaign total remains 10 of 30.

## T5 internal Dropship transport (2026-10-23)

The T5-only profile adds paid Dropship production through a completed
Starport with its own completed Control Tower. The add-on costs 50/50, has
500 HP and one armor, and takes 750 GBA updates (600 source simulation frames
at the established 5:4 conversion). Its center offset is (48,8) GBA pixels,
derived from the source tile footprints and add-on anchor. It uses existing
Starport art as an explicit placeholder. New products receive canonical raw
type/owner identities; imported placements retain their original identities.

Cargo occupies the existing unit pool, retaining HP, owner and supply while
hidden from rendering, selection, collision, targeting and independent sight.
Capacity follows the pinned type-space values up to eight points. Boarding
requires a living same-owner carrier and eligible unburrowed ground passenger.
Carrier destruction kills its passengers synchronously, including invincible
heroes, before scenario defeat queries. Unloading retries blocked landings,
releases one passenger every 19 updates, and applies a 19-update weapon delay
independent of actor-slot ordering. The 16-pixel boarding radius, constant
2-pixel flight step, bounded 8/16-pixel landing search, collision footprints and
30 Hz timing conversion are authored GBA approximations. Original transport
acceleration, full order/AI behavior and animations remain incomplete.

The Command/Bring adapter follows the inspected OpenBW count distinction:
global Command includes cargo; completed Bring attributes passengers through
the carrier's location/elevation, while AtMost's ordinary Bring path excludes
them. Completed building counts exclude unfinished construction. Raw 11's
membership in the Men selector is an authored classification; the pinned
references do not supply that units.dat group bit.

The following passes under ASan/UBSan and `-Werror`:

```sh
make test-sc64-transport-rules test-sc64-t5-tech test-sc64-transport-engine test-sc64-transport-queries test-sc64-t5-save
```

Fixtures cover mixed capacity, prerequisites and payment, attachment
geometry and stale slot links, boarding, loaded-actor exclusions, blocked and
successful unloads in both slot orders, hero cargo death, cargo save/load and
atomic malformed-reference rejection. A forced combat fallback excludes cargo,
and source carrier removal destroys passengers before the next Command query.
These fixtures use direct world setup;
they are not controller completion evidence.

Save generation 14 validates cargo references, capacity, coordinates, order
state and add-on links before mutation. It keeps the 16,328-byte SRAM payload
and 764-byte compact T5 state; no per-actor fields are added. A derived activity
flag uses four bytes. Normal T1–T4 builds retain generation 11. Internal
generation-13 saves are incompatible with this profile.

A bounded 60,000-tick host controller attempt uses only normal game inputs
after internal auto-entry. It completes the rescue/base sequence, pays for
the attached Tower and Dropship, boards eight raw-0 Marines, and uses
anti-air-aware MOVE waypoints to reach a walkable enemy-island landing.
The carrier is alive with zero cargo at the landing checkpoint; seven
passengers survive nearby. The attempt ends in live play without a win or
trigger faults. This establishes a host controller ferry, not a full mission
win or an ARM transport replay. The earlier direct flight was destroyed by
air defenses before landing; the controller planner changes its orders, not
the game terrain, enemies or damage rules.

The frozen 4 MiB internal auto-entry candidate has ROM SHA-256
`84e0e9b803c3de5079c9c6db7c8be84adeb36840682a9b767e709480aff4fcd5`
and ELF SHA-256
`6e00c3860c6fadc88bea86621054f70f676973530d71f8bd291725264f0d8bda`.
Text/data/BSS are 2,239,456/388/243,216 bytes; IWRAM ends at `0x03004f78`
and BSS at `0x0203b798`. Both unchanged reserve guards pass. The once-per-frame
T5 collision-bucket setup runs from ROM to preserve the IWRAM stack reserve.
A 3,000-tick neutral-input ARM run averages 28.12 effective fps (intervals
27.69, 28.27 and 28.41), with no trigger faults. This is opening-area evidence,
not transport/battle or full-mission performance.

The same generation-14 candidate passes normal START/B save and START/L load
across fresh mGBA processes: frame 122 restores exactly and continues to frame
182, with matching campaign/AI clocks and zero trigger faults. That ARM check
uses the opening state; loaded cargo round trips are covered by the host
engine fixture. No controller-only Revolution victory or physical GBA result
is claimed. The mission remains gated and the playable total remains 10/30.

## T5 source production restrictions and menu replay (2026-10-23)

The internal Revolution runtime now uses the verified map's PUNI production
availability. The importer retains the exact 5,708-byte framed record in
ignored storage, validates its source and record hashes, and derives a
342-byte effective bitmap. The native N64 handler and pinned OpenBW loader
agree on the per-player/default-selection layout; see
[References](REFERENCES.md#campaign-production-availability).

Human production now rejects the source-disabled Ghost, Goliath, Tank,
Battlecruiser, Science Facility and Armory. Factory selection cycles only to
the allowed Vulture, and Starport selection offers Wraith and Dropship.
Imported actors remain usable regardless of whether new production is
allowed. Checks run before payment or queue mutation, use the current owner's
source-player bank after rescue, and reject disabled queued products in a
candidate T5 save before changing the live world. Save generation remains 14.
This does not add missing unit abilities; in particular, Medic healing is
still absent from the reduced engine.

The asset-free decoder tests and optional real-engine sanitizer fixtures pass:

```sh
make test-sc64-unit-availability test-sc64-t5-availability-engine
```

The internal `sc64-t5-controller-replay` target now enables Revolution through
the normal HOME/campaign/briefing sequence. Public race targets and the
combined Terran T1–T4 target retain their existing mission gates. Its helper
uses bounded controller/scheduler pages and reads the ARM world at source
checkpoints, including paid tech, loaded passenger identities and landing
survivors. It does not write scenario state. Hardware-cycle timing replaces
wall-time throughput in the performance report.

The first full 60,009-input-tick normal-menu ferry replay, before the PUNI
restriction integration, matched the frozen 60,000-tick host trace. It boarded
eight Marines and landed seven survivors with its Dropship alive, then
continued in active play at frame 59,816 with 21 enemy buildings remaining and
no trigger faults. It averaged 20.97 effective fps over counted live play;
the worst counted observer interval was 15.54 fps. Its ROM SHA-256 was
`eb58e51ddb58a9de5d84ce76b0b231a4d167e79585aa27a2c631a4ec9f7f66b3`.
This extends the earlier host ferry proof to ARM. It does not establish a
mission win or near-30-fps transport/battle performance.

The production-restricted normal-menu ROM repeats the full ferry replay with
all source checkpoints matching, the same final mission state, 20.97 mean fps
and a 15.54-fps worst counted interval. Its SHA-256 is
`27711523f53fa536137c1702cbe5d22d8428a73c07b0114b8706101b87dd5d7f`.
Text/data/BSS are 2,240,520/388/243,216 bytes; IWRAM ends at `0x03004f78`
and BSS at `0x0203b798`. Both existing reserve guards pass. Adding diagnostic
rescue/mine/trigger timing counters leaves this ordinary ROM byte-identical.

Default source-only build/tests (124 Python tests), unified checks, combined
T3/T4 runtime/save/controller checks and all 18 skirmish launch combinations
pass. The combined T1–T4 ROM retains SHA-256
`d867886700a4c0a7351165f135ab720f37a6442327d529074775f4ac29d45077`.
Revolution remains internal pending controller completion and further
performance work. The playable total remains 10/30; physical GBA validation
is still outstanding.

## T5 mine acquisition cost (2026-10-23)

Diagnostic stage counters isolate rescue contact, mine updates and the
trigger/AI runner. On the same 10,009-input-tick controller prefix, mine
processing accounts for 73,342 cycles at frame 9,977 of the baseline build.
Moving dead/out-of-square rejection before the unchanged targetability
predicate reduces that observation to 25,521 cycles. Candidate order,
inclusive square bounds and strict nearest-distance ties are unchanged.
All 24 reported gameplay observations, including actor/building records,
match across these two diagnostic runs. The instrumented prefix improves
from 22.60 to 27.03 effective fps; it is a diagnostic comparison, not a final
ordinary-ROM or complete-mission measurement.

An old-order full-scan oracle matches the optimized selection across 1,200
seeded rosters containing dead, airborne, hovering, invincible, loaded and
different-owner actors. Explicit fixtures check boundary targets, square
corners and equal-distance slot ties. T5 combat/engine/world/availability and
transport query sanitizer checks pass. No runtime cache, persistent state,
save generation or additional RAM is introduced.

The ordinary ROM then completes the same 60,009-input-tick menu/ferry replay.
All 122 reported gameplay observations match the production-restricted
baseline, including live actor/building records, source trigger state,
wallets and cargo observations. Mean effective fps rises from 20.97 to 25.13;
the worst counted observer interval rises from 15.54 to 16.66 fps. It ends at
frame 59,816 in active play with 21 enemy buildings and no trigger faults.
This is still an unfinished mission, with performance below the 30-fps target.

The optimized 4 MiB normal-menu ROM has SHA-256
`c30dbc425c0ec6336d75783320581b14a57cf140a1d47f3e51a0b65aaf4cd011`
and ELF SHA-256
`865b96468d6139cc5fe4e12c41428f51280cb89396caee92c9c66e9b6d033508`.
Text/data/BSS are 2,240,552/388/243,216 bytes. IWRAM and BSS still end at
`0x03004f78` and `0x0203b798`; both reserve guards are unchanged. Default
build/tests (124 Python tests), unified checks, combined T3/T4 runtime/save/
controller checks and all 18 skirmish launch combinations pass. The combined
T1–T4 ROM retains its earlier hash. Revolution remains internal and the
playable campaign total remains 10/30.

## T5 render keys and bounded grouped air assault (2026-10-23)

The T5 profile now computes the static-object key once per map draw and
passes it to terrain-cache lookup and capture. It also avoids invalidating
the command-band cache solely for elapsed seconds in Revolution, whose HUD
does not display a timer. Other profiles retain their existing cache paths;
T1/T3 timer invalidation is preserved within the T5 build. No persistent RAM
or save fields are added.

`make test-sc64-t5-render` compares actual cached frames with separately
cleared fresh renders. Fixtures cover the 29-to-30-frame boundary, on-screen
building visibility/exploration, mineral depletion, camera edge/panning and
ordinary HUD invalidation. On the common 10,000-input-tick ferry prefix, all
23 reported gameplay observations match the preceding build and effective
fps improves from 27.03 to 27.26. The new run continues to 10,009 ticks and
reports 27.25 fps overall. These are prefix measurements.

The controller probe also enforces its input budget inside each step, clears
stale construction/menu modes before selection, places paid Depots on the
actual cursor lattice, and uses a normal L-cycle/MOVE to isolate a group seed.
It clears producer selection before same-type double taps so a click is not
consumed as another training command. These are probe input-policy changes;
the engine's selection, damage and economy rules are unchanged.

A 75,000-input-tick host attempt pays for two additional Depots and a Wraith
wing, then issues grouped attacks. The normal-menu ARM replay matches its
checkpoints through 75,009 input ticks, including optional Raynor, Marine and
Wraith observations decoded from the live unit pool. Enemy buildings fall
from 21 to 16. It ends in active play at frame 74,801 with both heroes alive,
19 SIDE buildings, 16 Marines, nine Wraiths and no trigger faults. It averages
22.25 effective fps; the worst counted observer interval is 14.33 fps. This
is a different, larger-army workload from the earlier ferry benchmark. It is
not a full mission win or near-30-fps battle qualification.

The 4 MiB ordinary replay ROM has SHA-256
`936a486fea32322901bf0a8c44b44cf63655a454b8b73a46bf6b65a84bf122af`
and ELF SHA-256
`8df18dc44f61d4211f3d491b12e396e48e10dbaee11e5c304d0551f857f12c8b`.
Text/data/BSS are 2,240,544/388/243,216 bytes. IWRAM ends at `0x03004f90`
and BSS at `0x0203b798`; both existing reserve guards pass. Default checks
(128 Python tests), unified checks, combined T3/T4 runtime/save/controller
checks, all 18 skirmish launch combinations and focused T5 engine/world/save/
transport-query/render sanitizer checks pass. The combined T1–T4 ROM retains
its previous hash. Revolution remains internal; 10 of 30 campaign missions
are playable and physical GBA qualification remains outstanding.

## Combined Terran T1–T5 normal-menu completion (2026-10-23)

The opt-in `make terran-t5` target builds the 4 MiB
`build/StarCraft-Terran-T5-GBA.gba`. The frozen build wins Antigan Revolt
through the ordinary campaign menu. Its controller replay ends at input tick 121,556
and game frame 121,261, with phase 3, human outcome 1, completion bit 256,
zero trigger faults, both heroes alive, and one completed human command center
alive. The final world has 24 Marines, 20 Wraiths, 55 SIDE units and 20 SIDE
buildings, with no enemy units or buildings. The run averages 20.85 effective
fps over 120,714 counted live frames; the worst counted observer interval is
11.95 fps over nine frames (12,640,305 hardware cycles). This remains below
the 30-fps target and is emulator evidence, not physical GBA validation.

The fresh-process menu save test passes with generation 14 and the 16,328-byte
payload. It saves at frame 123, restores at frame 123 in a new emulator process,
continues to frame 183, and reports no source-player context faults. The
combined T1–T4 profile remains generation 11 and requires a separate save file.

The candidate ROM SHA-256 is
`9344a66258c3a37e24c61eca04f3b9e95414999d8496f9ea3865f1416ad4ba7b`; its ELF
SHA-256 is `1a9d2b9a6a8bebdd144502aa57c445f4a150202558a694db4a16b53143141f3a`.
Text/data/BSS are 2,240,544/388/243,216 bytes; IWRAM ends at `0x03004f90` and
BSS at `0x0203b798`, within the existing guards. A repeated Make-built full
host run is byte-identical and wins at tick 121,547/frame 121,261. Its private
trace SHA-256 is
`021979e685d8be2a9496e764fedbe357b5e3f9a4c7baf404ec674d3a083240df`.
The asset-free Python
suite (133 tests), default build/tests, unified build/tests, combined runtime
matrix, and all 18 skirmish launch combinations pass. This opt-in result raises
the experimental campaign total to 11 playable missions out of 30: Terran
1–5, Zerg 1–3 and Protoss 1–3. The standard race ROMs and combined T1–T4 ROM
remain unchanged; 19 catalog missions remain unavailable.

## T6 packed storage foundation (2026-10-23)

The internal storage probe retains Norad II's 96×96 map and 216 non-start
UNIT placements, including 35 mineral fields and three geysers. Source UNIT
classification is 105 units and 73 buildings; runtime type mapping moves raw
195 into the building pool, for 104 ordinary units and 74 buildings. The
probe supports 160 units, 80 buildings and 48 minerals. Its packed building
records produce a 15,384-byte save payload at generation 15, separate from
T5 generation 14.

The frozen ARM ROM is
`25fafeb17da140868c2614ffd1bf300531bd9819a56664793df2481c9a828d4b`, with
ELF `7873ffa8b199307f4d965741192ef6e47e904a827b5e1cd90a38f6a7f9f2d8ef`.
Text/data/BSS are 2,474,724/388/245,632 bytes; IWRAM ends at `0x03004f90`
and BSS at `0x0203c108`, within the existing guards. The fresh-process
ordinary-menu T5 save test passes on this build: it saves at frame 123,
restores at frame 123, continues to frame 183, and reports zero context faults
with aligned campaign and AI clocks.

The storage/import/scenario/actor and combined regression targets passed:
T3 completion/save/resume, T4 controller completion and exact source-prop save
validation, T5 persistence/mine checks, and all 18 skirmish launch/save
combinations. The full T5 host controller run wins at frame 121,261 and its
input trace is byte-identical to the generation-14 proof. Default checks
(135 Python tests), unified checks, and the legacy combined T5 checks pass.
The existing T4 and T5 ROM hashes remain unchanged. This is storage evidence
only: T6 mission-world trigger bindings, controller completion,
and performance are not implemented or qualified. T6 remains unavailable and
the current campaign count remains 11 of 30, with 19 missions unavailable.

## T6 availability preparation (2026-10-23)

`make sc64-t6-prepare` now emits a separate private T6 PUNI bitmap. Preparation
checks the verified map, ROM, extractor, CHK and framed PUNI hashes before
writing the output. `make test-sc64-t6-availability` independently compares
all 2,736 generated player/type values with the retained raw record and
confirms that the human Dropship/Starport/Control Tower route is permitted.
This does not test construction or acquiring two Dropships in the mission.

The asset-free Python tests cover profile isolation, wrong-map rejection,
rehashed input tampering and preservation of an existing output after failure.
All 138 Python tests, the default build/checks, and unified build/checks pass.
Regenerating the default T5 availability header leaves it byte-identical.
T6 remains unavailable; the playable count is still 11 of 30.

## T6 standalone trigger adapter (2026-10-23)

`make test-sc64-t6-runtime` passes under AddressSanitizer and
UndefinedBehaviorSanitizer with the prepared private scenario. The adapter
reuses the T4 runner with T6-gated Deaths query delegation, eight independent
source-player contexts and one shared switch bank (1,184 bytes). It checks
the source-supported condition/action shapes, setup and creation properties
before execution. It delegates world queries and effects to typed callbacks;
it does not implement the live mission world.

The actual-source fixture checks all 24 enabled trigger/slot pairs, including
execution of resource/AI callbacks for inactive Force4 controller slots. It
checks victory with two Dropships and rejection with one, the Norad/Raynor
loss branches and immediate loss with no human actors. A rules fixture makes
all elapsed gates ready and verifies all 30 distinct creation-property
references execute once across the six groups, without replay. This is not
a controller or real-time campaign run.

Pause freezes state. Copying an actually pending reinforcement Wait and its
callback-visible fixture state produces identical remaining effects under
checked and prevalidated execution, including an atomic creation retry. That
is an in-memory continuation test, not SRAM serialization or execution
provenance validation. World effects must honor the callback retry contract.
The explicit GBA policy uses 30 elapsed ticks per second, ceiling conversion
for millisecond waits, one-shot completion and same-pass defeat priority.
Native scheduling, presentation duration and later AI unburrowing are not
established by this evidence.

`make test-sc64-t6-runtime-synthetic` is included in the asset-free default
checks. It uses an invented small program to check typed Deaths forwarding,
wait resumption, action retries, missing/unsupported callback faults and
operand bounds. The default build/checks (138 Python tests), unified checks
and focused T6 import/scenario/availability checks pass. Rebuilt T4, T5 and
generation-15 storage ROMs remain byte-identical to the hashes recorded above.
This stage changes no ROM or SRAM format. T6 world/save integration, normal
controller completion and performance qualification remain unfinished; the
campaign stays at 11 playable missions out of 30.

## Terran live-world and late-capacity foundation (2026-10-23)

Norad II now initializes all 104 units, 74 buildings, 35 mineral fields and
three geysers without unsupported rows. Focused ASan/UBSan checks cover world
queries, source ownership, initial burrowing, resources, rescue, atomic spawn
retry, 300 live ticks, add-on construction, production availability, and
generation-16 persistence/provenance rejection. A separate repair arithmetic
fixture exercises fixed-point HP accumulation, payments, completion and
insufficient funds. This does not establish a campaign completion route.

The generation-17 storage profile keeps 192 units, 96 buildings and 80 mineral
fields. All entity slots round-trip in the host fixture; malformed compact
rows and invalid pending repair orders reject before world mutation. Ten
thousand deterministic randomized Unit codec round-trips, combined target
IDs 254/255/287, and visibility removal across 288 overlapping sources have
focused coverage. The 15,312-byte save uses 38-byte unit rows; the engine uses
56-byte runtime units. Both existing RAM reserve guards pass: IWRAM ends at
`0x03004e68`, BSS at `0x0203d440` for the tested storage build.

That ARM ROM has SHA-256
`502a97559d12f0809cc30eeb3360034210dc1d88333945df17ecc7e0d5daa664`.
An internal-entry persistence check closes mGBA, loads the SRAM in a fresh
process, restores frame 302 exactly and continues to frame 362. Every named
unit field, including dead rows, matches both the live pre-save state and the
decoded SRAM record; all eight trigger contexts remain fault-free and their
clocks match the campaign/frame clock. No win or fps claim follows from this
check. The earlier T4, T5 and generation-15 storage ROM hashes remain
byte-identical to their recorded releases.

The authenticated late-map audit and gated importer retain all four remaining
Terran maps, their complete placements, trigger records, properties, source
setup and availability tables. Asset-free synthetic late-trigger checks cover
native comparison value 10, disabled actions, waits, preserve/retry, pause and
checked/prevalidated parity. Runtime mission bindings and normal-controller
completion remain required. No additional mission is marked playable; the
qualified count remains Terran 5/10, Zerg 3/10 and Protoss 3/10.

## Known limits

The engine is a reduced adaptation with a small authored default map, fixed-size entity pools, simplified combat/pathfinding and faction-specific approximations. See the [game guide](../starcraft/README.md) and [unified profile guide](SC64-UNIFIED.md) for feature limits. Physical GBA hardware testing is pending unless a dated result is added here.

## Norad II controller qualification (2026-10-23)

The internal generation-16 T6 runtime now has a complete ordinary-controller
win. Two independent ASan/UBSan host runs produce identical input/observation
traces (SHA-256 `94704db783fc05030df8f8b2084e6ff8057cebf8ce41d9b89662ec9632c84238`).
The route rescues the base, pays for its economy and transport technology,
produces two Dropships, boards Raynor, and unloads him in source location 2.
It changes no world arrays after mission initialization.

The same input trace passes through the normal campaign menu on the ARM ROM
`7ff58f60e501442d8c57676492b82549d1ba15bf41e622f9e0ea42dd9cd4aad1`
(ELF `0e3b5066d46c04e556c7ef2b041cf141e2410f76b08b9b7f76688551582e7832`).
At simulation frame 21,046, the source human outcome is victory, the mission
completion bit is set, Raynor and Norad are alive, one unloaded Raynor and two
Dropships occupy the objective region, and all eight trigger clocks equal
21,046 with zero faults. This establishes six completable Terran missions in
explicit development profiles; it does not qualify missions 7–10.

The full ARM replay averaged **17.26 effective fps** over sampled PLAY
intervals, measured from emulated GBA hardware timer cycles. Its worst sampled
interval was 4.54 fps over six advancing frames. These short intervals are
reported for transparency, not as stable frame-rate percentiles. Near-30-fps
performance and physical GBA SP verification remain outstanding.

The current source checkpoint passes the asset-free default build and host
suite (158 Python tests), T6 world/tech/save fixtures, late-map world and save
fixtures, compact storage and expanded target-index fixtures. Late source AI
requests still use authored bounded behavior; native campaign AI scripts have
not been recovered.

## Canonical Terran T1–T10 profile (2026-10-23)

The canonical `terran` preparation and build workflow now includes the full
Terran campaign. This section records the pre-optimization baseline; the
performance pass below supersedes its current-build identity and rates. The
baseline frozen 8 MiB ROM is
`build/StarCraft-Terran-GBA.gba`, SHA-256
`00f0939878a51320b8fc252d69b2947ffc442e6d3cc39efa12d0d118511bf3ee`.
The recorded build uses ARM GCC 16.2.0 and GNU Binutils
2.47.20260726. Its generation-18 campaign save is 15,816 bytes, with 1,892
bytes of late mission state. The unchanged memory guards pass: IWRAM ends at `0x03004fb8`
and BSS at `0x0203dc40`.

Real `./dev sc64 prepare --rom PATH --profile terran`, the full Terran build/status flow,
and the complete regression suite passed. The run covered 175 Python tests,
the default and unified builds/tests, all ten Terran startup/save fixtures,
and all 18 Outpost/Blood Bath combinations (three human races and one to
three independently selected AI races). Host controller runs won all ten
Terran missions. The full-profile controller milestones include T1 at frame 24,018, T2 at
frame 34,513, T3 at frame 54,000, T4 at frame 9,331, T5 at frame 121,261,
and T6 at frame 21,046. T1 and T2 also enter through the normal campaign menu.

A separate fresh private output directory regenerated all race and sprite
headers, T4–T10 scenarios and T5–T10 availability tables. Their hashes and all
97 retained records match the canonical inputs; preparation does not depend
on stale retained records from earlier development profiles.

Repeated host winning traces for the late missions are deterministic:

| Mission | Final frame | Winning trace SHA-256 |
| --- | ---: | --- |
| T7 | 12,151 | `46ea2855aac31b97194370130aa4c1a60aae01715a756b2a66bf7439e90faca4` |
| T8 | 86,596 | `41c22e97d15d180b36453c679826e6cacb8c87372704226e0510e4f047ea3079` |
| T9 | 358,906 | `d44ea1acd3e882d06fce333c47480fe6c7034033d9761b3b42a7b32a9538e552` |
| T10 | 38,491 | `9a7d0aca12c46b5682247fa3d015076364cb60804a19559699c62ae9eb900514` |

Full normal-menu ARM wins are now recorded for all ten Terran missions across
the available frozen builds. The earlier generation-16 T6 replay won at frame
21,046 and averaged 17.26 effective fps. The canonical ROM's T7, T8 and T10
replays all won with zero trigger faults: T7 at frame 12,151 (15.76 effective
fps), T8 at frame 86,596 (14.37 fps), and T10 at frame 38,491 (11.78 fps).
T10 ends with Raynor alive and the Ion Cannon destroyed.

T9's full normal-menu ARM win is on the earlier frozen generation-18 ROM
`329793a4707e0d3c4b33ed169391b2b887374d5eb788943ceef8828604c7ce44`, not the
canonical ROM. It completed at frame 358,906 with all 43 Protoss Force2
buildings destroyed, all 21 Zerg Force3 buildings intact, Kerrigan alive, all eight
trigger clocks aligned to frame 358,906 and zero faults; it averaged 16.94
effective fps. Thus every Terran mission has a recorded ARM win, but T9 has
not been replayed on the final canonical image.

All six fresh-process generation-18 saves for T5–T10 passed on the canonical
ROM. Each 15,816-byte save loaded at frame 123, continued to frame 183, matched
the live state exactly and reported zero faults. Campaign clocks matched in
all six; T6–T10 trigger clocks matched, and the saved 1,892-byte late mission
payload matched runtime state exactly for T7–T10. T5–T10 saves used the
canonical ROM SHA above and the matching ELF (SHA-256
`5399b4318967a71814874c006d4cb2f2f7711e68be431ee0c6ef76446bd7b26d`).

Mission adapters use bounded authored AI behavior where original N64 AI
scripts are unavailable; these results do not claim native AI execution.
All measured campaign emulator rates remain below the 30-fps target, and no
physical GBA hardware result is claimed.

## Terran CPU and presentation optimization (2026-10-23)

The optimized 8 MiB `build/StarCraft-Terran-GBA.gba` has SHA-256
`8a9ddd41eab795dec08c1546e707aac04a093aa0a7e2ca147a59c1b351d57bb0`;
its matching ELF has SHA-256
`737dc9e388563d81680eff1adee8c06c76ea007988bf65f7372d36b3889024b5`.
It uses ARM GCC 16.2.0 and GNU Binutils 2.47.20260726. The unchanged reserve
guards pass with IWRAM ending at `0x03004fa0` and BSS at `0x0203d9ec`.
The generation-18 save layout remains 15,816 bytes.

This pass reduces repeated hostility and attack checks, retains collision
memberships across ticks, narrows defense candidate searches, caches repeated
late-trigger counts, and avoids redundant HUD band transfers. The late
trigger runner now uses its prevalidated entry point as intended, while
retaining mutable-slot validation before execution. The late profile can
present during an already-open safe VBlank window after at least two video
frame periods; it does not skip simulation or rendering ticks. Non-late
profiles retain their previous collision and presentation paths.

Full normal-menu ARM/mGBA input replays compare the baseline ROM above with
this optimized ROM, using identical controller traces and the same emulator
setup. These are mean effective rates over sampled advancing PLAY intervals,
measured with the emulated GBA hardware timer; menus and result-boundary
intervals are excluded.

| Mission | Baseline fps | Optimized fps | Increase | Winning frame | Matching observed checkpoints |
| --- | ---: | ---: | ---: | ---: | ---: |
| T7 | 15.76 | 25.00 | 58.6% | 12,151 | 45/45 |
| T10 | 11.78 | 18.77 | 59.3% | 38,491 | 89/89 |

T7 counts 12,148 PLAY frames over 8,153,259,467 timer cycles; T10 counts
38,478 over 34,387,849,364 cycles. The trace hashes remain those in the
baseline table. All exposed checkpoint state fields match the baseline after
excluding timing and profiling counters. Both wins have zero trigger faults,
all eight trigger clocks aligned, and the same objective results: T7 delivers
the emitter with Kerrigan alive; T10 destroys the Ion Cannon with Raynor alive.
This comparison does not establish equivalence of unobserved memory or pixels.

The default build and host suite (175 Python tests), unified build/tests, and
full Terran build/regressions pass. All ten host controllers still win; the
T7–T10 winning trace hashes are unchanged. Focused ASan/UBSan checks include
1,440,759 collision comparisons, target selection parity, full-tick defense
parity, trigger checked/prevalidated parity, framebuffer HUD comparison, and
presentation timing boundaries including timer wraparound.

Fresh-process normal-menu ARM saves for T7 and T10 also pass on this ROM:
both save and load frame 123 exactly, continue to frame 183, retain the exact
late-state payload, align campaign/trigger clocks, and report zero faults.
Other mission ARM wins and saves remain qualified on the earlier frozen
builds described above, rather than being implied to have been rerun here.

These improvements remain below the 30-fps target, particularly in T10's busy
battles. No physical GBA or GBA SP result is claimed. The bounded authored AI
and other adaptation limits remain unchanged.

## Zerg 4 source foundation (2026-10-24)

The opt-in `tools/sc64_z4_pack.py` stages the complete **Agent of the Swarm**
source at BOLT `008/017`, catalog ID 15, without modifying canonical race
headers or enabling a playable mission. The actual private import and its
standalone retained-record verification both pass. A second output directory
independently passes the same checks. The authenticated CHK SHA-256 is
`2cb8e516f07651801c1d6f519017d494af87c479a0ba8df9e33af34b14ec685a`.

The tool verifies the normalized USA ROM, pinned extractor revision and
extractor source bytes before execution. It preserves the full CHK, including
all 184 placement rows and seven trigger records, plus 16 authenticated framed
records. Thirteen asset-free tests cover malformed/truncated source, omitted or
unexpected placements, wrong ROM/extractor identity, dirty extractor code,
record/manifest tampering, standalone verification without a ROM, and the
distinct runtime pools for Mature Chrysalis (raw 150) and Chrysalis (raw 216).

The source audit identifies demand for 73 unit slots, 67 buildings, 39 mineral
fields and two geysers. The current Zerg profile's 64-building and 32-mineral
limits cannot retain the full scenario. Dropships, Control Towers, Machine
Shops, Engineering Bay and Armory also need runtime mappings for this profile.
The countdown, hatch sequence, Kerrigan/Chrysalis survival conditions,
Command Center objective, authored AI and persistence remain unimplemented.
There is no new controller win, ARM timing result or playable-mission claim;
the completed campaign count remains 16/30 across development profiles.

## Collision and complete skirmish lineup checks (2026-10-24)

The Terran collision broad phase now resolves the mover's flight and collision
classification once per query and rejects geometrically irrelevant candidates
before reading their actor metadata. The legacy zero-flags fallback and
overlap-separation rules are preserved. Focused ASan/UBSan comparison with the
full scan passes 1,440,762 cases, including hidden actors, loaded units, slot
reuse and the zero-flags case. The late build frees 328 bytes of IWRAM code;
the unchanged guards pass at IWRAM `0x03004e58` and BSS `0x0203d9ec`.

The frozen Terran ROM SHA-256 is
`55df7258ec66a55566434af6ef3cb2dcac08d09c8792066cde7c29c38fe0082f`,
with matching ELF SHA-256
`fecdec39c3c61c280b6f53c6deec4539c070c58145e6a97e8f3f4335d5e65468`.
Full normal-menu ARM/mGBA T7 and T10 wins retain the previous measured rates
to two decimal places: **25.00 fps** and **18.77 fps**. All 45 T7 and 89 T10
observed checkpoint states match the preceding `8a9ddd...` build, excluding
timing/profiling counters. Winning frames and zero-fault trigger results are
unchanged. Both generation-18 fresh-process saves restore frame 123 exactly
and continue to frame 183 with the same late-state payload and zero faults.
No meaningful FPS increase is established by this change; 30 fps remains unmet.

The full Terran startup/save fixture now tests every map, human race and
independent AI-race assignment: Outpost and Blood Bath, three human races, and
3/9/27 assignments for one/two/three AI opponents. All **234 configurations**
pass startup, active/inactive owner roster checks, 60 play ticks and exact
save/load round trips. These are setup/persistence checks, not 234 complete
skirmish victories. All ten Terran host controllers still win, with unchanged
T7–T10 trace hashes. Default build/host checks, the final 188 Python tests,
unified build/tests and all three race builds/tests pass. Save layouts and
pool sizes are unchanged.

Minimap row-packing experiments measured no FPS improvement on the matched
4,015-tick T10 prefix and were discarded. The shipped renderer is unchanged.
Physical GBA testing remains pending.

## Canonical Zerg and Protoss third missions (2026-10-24)

The standard `zerg` and `protoss` targets now build their three-mission
profiles. Legacy Z3/P3 ROM and ELF paths are compatibility symlinks to those
canonical outputs. The prepared source gates are checked by `dev prepare`
and `dev status`; both actual profile preparation commands pass.

The frozen Zerg ROM SHA-256 is
`616c45861d349d65f94e54d8872b35d67a3c611ea91362a9540cded433393e41`,
with matching ELF SHA-256
`ddce29f980413d6e03e1c9f26a420aa4332a2212a15bff3bb43f45ff3f1b10b2`.
It is a 2 MiB `SCZ3` image, save generation 5. The unchanged linker guards
pass at IWRAM `0x03004ff8` and BSS `0x02039848`.

The frozen Protoss ROM SHA-256 is
`8d0281c535e66cbf6cb61767912a5e0d364476ad31bb9cc79f49d55a7908f9a1`,
with matching ELF SHA-256
`ad72d0ac904744afef3e438b53d150485a5891370c532c463d0c0cfd8be03321`.
It is a 2 MiB `SCP3` image, save generation 7. IWRAM ends at `0x03004f88`
and BSS at `0x020399f0`, within the same guards. Rebuilt canonical ROMs match
these frozen ROM hashes; replay uses the matching frozen ELF pairs.

Full normal-menu ARM/mGBA controller replays win Z3 at frame 70,833 and P3
at frame 151,088. Their counted live-play means are **19.12 fps** and
**22.83 fps**, across 73 and 154 observed checkpoints respectively. These
are measurements of the promoted images, not evidence of a new optimization
gain. The 30-fps target remains unmet, and physical GBA testing is pending.

Each replay also saves a separate 30,000-input-tick prefix through START/B.
A fresh emulator process loads it through HOME/L and compares the observed
actor snapshots, resources, race banks, campaign/model state and completion
bits against the authenticated paused checkpoint. Z3 restores frame 29,858;
P3 restores frame 29,662. The profiler then verifies its one neutral handoff
frame plus 60 measured live ticks, ending at frames 29,919 and 29,723.
The report is bound to both ROM/ELF hashes and the SRAM SHA-256. This checks
the captured observations; host save tests independently check exact payload
round trips.

Default build/tests, 199 Python tests, unified build/tests, all race builds,
the Z3/P3 runtime and controller suites, and all three startup/save matrices
pass. The matrices cover 234 configurations per race ROM: two maps, three
human races, and every independent assignment of one to three AI races.
These 702 setups are not 702 completed matches. Zerg and Protoss also pass
all three campaign menu launches and save round trips. Both reject an older
generation-2 tagged save, and a current-tag container with an old payload
version, without changing live state.

No new campaign mission is enabled by this promotion: the completed count
remains Terran 10, Zerg 3 and Protoss 3, or 16/30.

## Weapon-family effects and Protoss shield display (2026-10-25)

Authored weapon-family effects now present the existing attack paths for the
implemented actors and defenses, including T4 traps and T5 mines. They only
change rendering: combat continues to apply damage on the existing attack
tick. Effect pixels are clipped to the playfield rows 12–119 so they cannot
overwrite the HUD. Protoss unit and building overlays draw shields separately
from health using canonical shield capacity; the selection panel shows current
shields for shielded selections. No campaign mission or combat/save rule was
changed by this visual work.

Focused host checks passed: `make test-sc64-weapon-vfx`,
`make test-sc64-weapon-vfx-game`, `make test-sc64-shield-bars`,
`make build/test-unified-hud && ./build/test-unified-hud`, and
`make build/test-starcraft`. The renderer test exercises palette styles,
extreme-coordinate clipping, visibility gating and the row-119 boundary. The
attack fixture checks 30 armed unit/target-class profiles, ground and air
attacks, effect lifetimes, camera/fog behavior, pool overflow and unchanged
immediate-damage behavior. Shield checks compare unit/building bar pixels and
verify shield-only HUD cache invalidation against a forced full redraw.

All three canonical race ROMs build and pass the unchanged reserve guards.
IWRAM/BSS ends are Terran `0x03004e58/0x0203dbd4`, Zerg
`0x03004fb0/0x02039a30` and Protoss `0x03004f30/0x02039bd8`.
Race HUD key construction runs in ROM so the new keys fit without reducing
the existing fast-RAM stack reserve. The race effect pool uses 32 transient
slots; original-art saves retain 12. Actor rows and save generations are
unchanged. The canonical campaign count remains T10/Z3/P3 (16/30).

Full regressions use `./dev build && ./dev test`, `make unified test-unified`,
`make race-campaigns test-race-campaigns`, and
`./dev sc64 test --profile terran`. These host/build checks do not establish
ARM performance, physical GBA behavior or a new full campaign replay.

### 2026-10-24 — staged Z4/P4 engine foundations

The next-stage source importer retains all remaining Zerg and Protoss CHKs
and source records in ignored storage, with stable catalog IDs. Numeric
scenario generators authenticate Z4's seven triggers/66 actions and P4's
13 triggers/153 actions; they do not copy original dialogue or audio into
tracked sources. Source retention does not make a mission playable.

The staged Z4 source census is 73 units, 67 buildings, 39 mineral fields and
two geysers. P4 has 137 units, 33 buildings and 26 mineral fields, with no
geysers. The raw-196 beacon belongs in the building pool. Source raw-owner
provenance is retained separately from runtime ownership during contact
rescue. Hidden map revealers contribute sight while remaining unselectable,
noncollidable, untargetable and undrawn; the Protoss beacon retains its exact
100000 HP and intrinsic invincibility.

Real-engine sanitizer fixtures cover the Z4 full 18000-tick countdown and
hatch handoff, plus pre/mid/post-hatch save round trips and corrupt-state
rejection. P4 checks its normal-menu launch, contact rescue, source pause,
all 87 scripted creations, mid-creation saves, and production-slot reserves.
Their forced victory/death setups are synthetic rule tests, not campaign
completion. A normal-controller Z4 ground-only attempt reached the hatch
with Kerrigan alive but could not reach the remaining Command Center;
there is no win claim for that attempt. The subsequent air-tech controller
built a Lair and Spire through the ordinary menu, produced Mutalisks, protected
Kerrigan, and won at frame 71777 / host tick 71936. An independent ASan/UBSan
run matched all 1217 trace events. The frozen Z4 ARM replay now also wins at
frame 71777, with 73 checkpoints and a 15.12-fps mean over live PLAY intervals.
A 30000-tick prefix saved at frame 29852; a fresh emulator process restored
the observed state exactly, then completed the one-tick input handoff and 60
neutral ticks at frame 29913. The source trace SHA-256 is
`00ccdf89720a697e87084a7d99842b25ddebcc2f540926175ec865ce04d2ad6a`.
The ROM SHA-256 is
`4990ff713183d19e54078e2c171b829725ab49445debbe820605438f53a16d3c`,
with ELF `77dd34fb7de332e29833843065d4abf1e2472041cd3a8c087532d910d5f0553c`.
These results qualify that frozen development image; subsequent optimized
images require their own comparisons. The Protoss controller is still in
progress and has no completion claim.

The asset-free default build/tests pass, including 217 Python tests, and
unified build/tests pass. Dedicated actor, storage and Z4 runtime checks
pass under ASan/UBSan. The stage uses a separate generation-20 save layout,
retaining the unchanged IWRAM and EWRAM reserve assertions. These results
are development evidence; the canonical qualified count remains 16/30.

The staged four-entry profiles additionally pass every normal-menu mission
launch, save round trip and 234 independent skirmish setups each under host
ASan/UBSan. Those setup checks do not establish mission wins.

### 2026-10-24 — fifth-chapter actor foundations

An opt-in Z5 actor gate adds source facts and bounded GBA policies for Ghosts,
Civilian, Rhynadon, Bengalaas, Ragnasaur, Data Disk and the Terran Beacon.
The gate is disabled in all current ROM profiles. Existing enum identities
and all four frozen Z3/P3/Z4/P4 ROM hashes remain unchanged with it disabled.
Separate sprite completeness checks prevent partial Z5 imports from being
treated as complete. The source-placement census counts Z5's seven raw-195
beacons as buildings without changing other mission classifications.

Actor tests cover the gate off, gate on and coexistence with the T4 actor set
under ASan/UBSan; sprite generator and source-census tests use synthetic input.
These foundations do not implement Z5's trigger effects, doodads or mission
controller. UI/pathing policy for unusual actors is documented separately
from source table facts; native equivalence is not implied.

### 2026-10-24 — race presentation performance comparison

Two isolated full ARM/mGBA controller replays tested deadline-based VBlank
presentation for both races and removal of the redundant framebuffer clear
for Zerg. The presentation gate uses the existing two-hardware-frame minimum
and flips only during safe VBlank scanlines. It avoids an extra wait when a
busy frame has already crossed its presentation deadline. Game update rates,
controller inputs and mission rules are unchanged.

| Full campaign replay | Baseline mean | Candidate mean | Matching checkpoints | Identical PNG captures |
|---|---:|---:|---:|---:|
| Z3 | 19.12 fps | 20.88 fps | 73 | 6 |
| P3 | 22.83 fps | 24.33 fps | 154 | 6 |

Both candidates win at the same frames as their baselines (70833 and
151088). Comparison excludes only timing/profiling fields and requires all
other recorded game-state fields and captured pixels to match. FPS uses
chained GBA hardware timers and positive live PLAY-to-PLAY frame intervals;
startup and the final result interval are excluded. These are emulator
measurements, not physical GBA results, and the 30-fps target remains unmet.

The Z3 candidate ROM SHA-256 is
`7878fc9914e06cbaa7d751a9d71e6df0e4d3220596c910f32bd79bfe8102b99b`,
with ELF `0d478a3a09f9ba095acd1c6e852c76f04304018f130ce58993fda6edd3158b40`.
The P3 candidate ROM SHA-256 is
`6a1d23c4e50e131eb0b9e76b321b7941fd46e706e8b75640727831800519fff6`,
with ELF `d04dcf9a328dd6ee2c66cc1ac7bf1d5f5de15fc2cb6f1460e2240dce2317ceac`.
These frozen comparisons precede the shared fourth-chapter engine additions;
newly linked images require their own ROM/ELF-bound qualification.

The Zerg no-clear renderer also has a full-clear pixel oracle covering camera
edges, fog, overlays, scene transitions, map-object mutations, save/load and
restart. Both HUD bands continue to be restored every frame. An additional
experiment that skipped unchanged HUD-band copies was rejected after its ARM
capture comparison found viewport-edge map graphics leaking into the HUD.
A consecutive-frame host fixture reproduced that failure; poisoning both
buffers before every comparison had previously hidden it by invalidating the
cache each time. No HUD-band skip is enabled by these performance flags.

### 2026-10-24 — race target occupancy optimization

The existing late-Terran target-query implementation is now also gated by
`SC64_TARGET_OCCUPANCY` for the race profiles. Conservative owner row/column
masks skip empty areas; a per-update hostility snapshot avoids repeated
alliance checks. Movement adds occupancy immediately, and stale bits remain
conservative until the next rebuild. `SC64_UNARMED_QUERY_SKIP` skips searches
for actors with neither weapon while preserving wildcard and air-only queries.
The default and other flag-off profiles retain their previous implementation.

Full Z4 ARM replay wins at frame 71777 with all 73 observed state checkpoints
and five PNG captures equal to the baseline. Its live-play mean rises from
15.12 to 16.61 fps. The candidate ROM SHA-256 is
`a80939e948466450466682786ee2fe8aa000d9c3aa8cbcab5a7e2f6ffe1a8cf8`,
with ELF `e1da9b0b7e7cfa5405094fe0041f5a893284fe0330534aae444558dea7c8ddf3`.
The shared-source build produces that same ROM. Separate 30000-tick prefixes
match all 31 states and four PNGs for Z4, Z3 and P3; measured means are
18.48, 23.99 and 26.43 fps versus their original 16.52, 20.31 and 24.56.
The full Z3 and P3 candidate replays subsequently pass as well:

| Full replay | Original mean | Optimized mean | Matching states | Identical PNGs |
|---|---:|---:|---:|---:|
| Z3 | 19.12 fps | 21.68 fps | 73 | 6 |
| P3 | 22.83 fps | 25.08 fps | 154 | 6 |

Both win at their original frames (70833 and 151088). Z3's ROM SHA-256 is
`3cacc5db20f9576525dca643980c8f0aecdbb60f1047fae0117435a707ef0bda`,
with ELF `28753e939c79a930427491418aca81a3c0437284ed17d68d7fe644e9f82f9805`.
P3's ROM is `b590cd00e40606c4da31a8014ea6eecb0fae42abb065df22ebd119904f813c97`,
with ELF `8a1c79d964f4302b63efcdf341f1724d4bc66dd73def2c56d2e78eb096037d78`.
The shared builds are byte-identical to these frozen candidates.

The optimized Z4 image additionally saves at frame 29852 and restores the
observed state exactly in a fresh emulator process. Its input handoff plus
60 neutral ticks ends at frame 29913. Optimized canonical Z3 and P3 also
restore exact saved snapshots in fresh emulator processes. Their loaded
frames 29858 and 29662 advance through one input-handoff frame and 60 neutral
ticks to 29919 and 29723, respectively; both continuity invariants pass.

All four race target-query differential fixtures pass ASan/UBSan, covering
five owners, every attacker type, strict range/tie rules, air-only targets,
movement/deaths, index overflow/fallback and rescue hostility transitions.
The existing unified and full-Terran target fixtures also compile and pass.
All four ARM images satisfy the unchanged memory guards. A source-only copy
of the staged performance changes passes `./dev build` and `./dev test`,
including 248 Python tests with four optional private-data skips.

To retain the IWRAM reserve with the larger query, the race HUD invalidation
pass runs from ROM, as it already did in expanded profiles; expanded Z/P
profiles also place `gfx_bar` in ROM. No HUD-band caching experiment is enabled.
These are emulator results; physical GBA qualification and the 30-fps target
remain outstanding.

### 2026-10-24 — opt-in P4 Psionic Storm

The separate `protoss-p4-storm` build adds a bounded four-effect Storm model
and menu targeting for High Templar and Tassadar in P4. The authenticated
PTEC decoder uses the five source tables at offsets 0/288/576/600/624 and
resolves player-7 defaults to the global availability and research flags.
UNIT energy comes from its energy-valid field and source percent byte.
Both caster types start at the source's 100 percent energy.

Pinned reference facts supply 75 energy cost, 14 damage per pulse, shield-first
Ignore Armor damage, air/ground nonbuilding targeting and friendly fire. The
GBA coordinate conversion gives a 24-pixel radius and 144-pixel cast range.
The eight pulses at 20/10 GBA ticks are an authored cadence assumption, not
verified native projectile timing; the VFX is also authored. Overlapping
effects deal at most one pulse per target per update. Energy regeneration
uses 32/5 Q8 per 30-Hz simulation tick; world and user pauses freeze it.

Host ASan/UBSan checks pass for normal-menu casting/cancel, energy/range/pool
denials, air and ground damage, shields, friendly fire, invincibility, all
eight pulses, active-effect saves and transactional corrupt-state rejection.
An inactive P4 payload is zeroed on save. The render check compares consecutive
real preserve-mode frames with full-clear references at both viewport edges.
The build also passes all four campaign startups, 234 skirmish setup/save
combinations, storage checks and the general frame-coverage oracle.

Its separate generation-21 save is 15364 bytes. The ARM build passes the
unchanged guards at IWRAM end `0x030040c0` and BSS end `0x0203d09c`.
No completed P4 controller or ARM playthrough is claimed by this ability
foundation. The ordinary P4 and canonical race builds leave the ability gate
disabled and retain their frozen ROM bytes.

### 2026-10-24 — P4 controller win and Z5 storage integration

The separate P4 Storm profile has a repeatable normal-controller host win at
frame 109776 (host tick 110452), fired mask `0x1ff8`, all 87 scripted creates,
zero adapter faults, and both heroes inside the extraction rectangle.
The controller rescues all four Probes, builds an army using normal input,
uses Storm, and routes the escort separately with the authored B+R shortcut.
Repeated ASan/UBSan runs produce the same complete trace (2185 transitions,
peak 109 in one 1000-tick scheduler page). Trace SHA-256 is
`b7d072d609ea9bd17c59024c81f6a27165b7b6ac3e8e0d0175b2227934111ef4`.
The frozen ARM image also completes that exact normal-input trace at frame
109776, with the same heroes at Tassadar (1551,1984) and Raynor (1520,1992),
all 87 creates and zero faults. Its full live-play mean is 15.17 fps. ROM
SHA-256 is `381f0c4bb751ab2005502a3d8505aa8c7eaf8fa6f48213a8f313a7d5eda7fcf6`,
with ELF `c5f785d26cc16ae5deb65d55007de50a413ab6c1e1c8718131a2dd83e74121ae`.
A separate 30000-tick prefix saves at frame 29903; a fresh process restores
the observed state exactly and advances one handoff frame plus 60 neutral
ticks to frame 29964. This qualifies that frozen development image; later
shared builds require their own matching-image checks. Physical hardware
performance and the 30-fps target remain unqualified.

The opt-in Z5 runtime imports every retained starting actor and all 41
unit-backed THG2 props. Source binding identity, enabled door/trap state,
trigger progress and authored pause acknowledgment survive generation-23
save/load; malformed padding is rejected before live-world mutation. A
source-action door toggle changes both collision behavior and the exact
static-art cache key. Focused real-engine host checks, all five normal-menu
start/save cases, 234 skirmish setup/save combinations, and full-clear pixel
coverage comparisons pass. No Z5 controller completion is claimed.

Expanded storage holds 240 units, 160 buildings, 144 minerals and 12 geysers.
Standalone codec tests, full P4 save round trips, final-row corruption,
geyser bounds, invalid-live-save preservation and old/truncated-record
rejection pass under sanitizers. The Z5 ARM build retains IWRAM end
`0x03004f60` and BSS end `0x0203d7f0`, inside the unchanged reserve guards.
Number formatting moves to ROM only for this expanded profile; glyph
drawing remains in IWRAM. These pool and setup checks do not qualify all
remaining missions.

A private aligned-halfword sprite-read experiment matched 62 prefix state
checkpoints and eight captures across P3/Z4, but improved their 30000-tick
means only from 26.43 to 26.45 and 18.48 to 18.55 fps. It is not enabled.

### 2026-10-24 — defense target-list candidate

An opt-in `SC64_ZP_DEFENSE_TARGETS` path builds fresh owner/class lists after
the mission tick, preserving lower-slot ties and rechecking targets killed by
earlier defenses. Invalid ownership falls back before any combat mutation.
The existing P3 path keeps its mission-specific dispatch. Three actual-engine
ASan/UBSan fixtures compare complete defense ticks against the original scan,
including 800 randomized worlds per profile, strict range, tie, early-death,
power, cooldown and invalid-owner cases.

Separate 30000-tick ARM prefixes match all 31 observed states and four captures
for each mission. Z4 improves from 18.48 to 21.09 fps; P4 Storm improves from
15.23 to 16.40 fps. Full wins also match: Z4 improves from 16.61 to 18.43 fps
with 73 matching states and five captures; P4 Storm improves from 15.17 to
16.51 fps with 112 states and five captures. Exact fresh-process restores
plus one handoff and 60 neutral updates reach frames 29913 and 29964.
The flag is enabled in the staged Z4/P4 profiles; fifth-mission profiles
filter it out pending their own qualification and memory checks.

The qualified Z4 ROM SHA-256 is
`29a03a107ae702ac181967635003abfeb327e5a84d9f26fc2c698230d6c6c36b`,
with ELF `b42e8791f717ef64738e4abda87be9ab9ac7da73cb235958a4d38f911dc87ba1`.
P4 Storm ROM is `f9cfb3b9fe016db8e4247acfc855e32a950e52720545b76b13025ae4c7a83e2a`,
with ELF `27efd0e9b2316becffde3cfc30746c86f195383ea69642e5fa7913f859646d7d`.
These are emulator measurements; neither mission meets the 30-fps target.

A separate private packed-minimap experiment passes its 1000-pattern host
pixel oracle and both ARM state/capture comparisons, but changes the Z4/P3
prefix means only from 18.48/26.43 to 18.65/26.49 fps. It remains disabled.

### 2026-10-24 — P5 runtime and optional Z5 cloak

The opt-in P5 build imports 110 units, 69 buildings, 57 minerals and three
geysers, matching every retained source placement. Its actual-engine fixture
checks distinct hostile force buckets, rescue, real combat death accounting,
source-triggered reinforcement activation and all 11 creates. Source Kill
Unit increments death counters; removal does not. Save/load and old or corrupt
candidate rejection pass transactionally under ASan/UBSan. All five campaign
entries, 234 skirmish setup/save combinations and the full-clear framebuffer
oracle pass. The generation-24 ARM build ends IWRAM at `0x030040e0` and BSS
at `0x0203d564`, inside the unchanged limits. No P5 controller win is claimed.

Z5 now accepts destroyed trap props in source bindings and saves while still
rejecting destroyed invincible doors. A regression reproduces a real trap
kill and continues the normal runtime instead of pausing with a binding fault.
Zergling/Kerrigan HP regeneration follows the authored 24-to-30-Hz conversion
of pinned reference fractional regeneration (one HP per 80 live GBA updates),
with its phase persisted and frozen by pause.

The separate generation-25 Z5 cloak profile authenticates its PTEC and UNIT
energy facts. Kerrigan starts uncloaked at 250 energy; activation costs 25,
drain is 0.9375 energy/second and regeneration is 0.75/second at 30 updates/sec.
The pinned reference's `_fp8` literals are raw Q8, including the per-update
drain and regeneration; these values are not multiplied by 256 a second time.
Geometry fixtures cover inclusive detector bbox ranges and one-pixel outside
boundaries. Actual-engine tests pass for B-menu toggle, STOP preservation,
pause, detection/target eligibility, exact active-cloak save round trips,
invalid energy/state/old-generation rejection without live-world mutation,
all five menu entries, skirmish setups and framebuffer coverage. Treating an
enabled prop as an active detector is an authored policy. Source PTEC does
not enable Psionic Storm. Z5 has no qualified controller win or frame-rate
result from these checks.

### 2026-10-24 — optional P5 Shuttle lifecycle

P5's extraction island is disconnected from its starting ground force under
the imported terrain; its source Shuttle is required. The separate
generation-26 profile enables eight-space transport using pinned passenger
capacities. The real-engine ASan/UBSan fixture boards through normal A input,
fills capacity and rejects excess cargo, checks moving/idle-carrier cargo
synchronization, and unloads through the normal B-menu command. Loaded units
are excluded from targets, collision, vision and location-scoped Bring;
Command still counts their presence. Carrier destruction kills cargo and
records the corresponding source-player deaths.

Active cargo and unloading save round trips, malformed links, foreign owner,
overcapacity and generation-24 rejection are checked before live mutation.
The ARM build ends IWRAM at `0x03004100` and BSS at `0x0203d564`, inside the
unchanged guards. No completed P5 controller or ARM playthrough is claimed
by these lifecycle checks.


### Protoss five Shuttle normal-controller qualification

The optional `protoss-p5-shuttle` build now has two identical ordinary-input
host wins and a matching complete ARM replay. The controller launches global
mission 24 through HOME, rescues Tassadar, boards the real source Shuttle with
Tassadar and two Zealots, flies around the source anti-air defenses, and unloads
all three within source extraction rectangle `[1696,1680,1808,1776]`. Victory
occurs at game frame 8106 / controller tick 8171, with zero runtime faults and
Shuttle HP 19. No actor positions, health, resources, triggers, or result flags
are injected by the replay; only the neutral input scheduler is written.
The started-trigger mask is 2012 and all final action cursors/wait timers are
zero. All 11 one-shot source creates have completed. Completed source programs
now release their creation reservation, including the saved-state capacity check.

Frozen private evidence is below `.local/build/promotion/p5-arm-qualified/`:

- ROM SHA-256: `31fd14c41fad48dc89eefeb2efb3cd7e453585b07d842aabf271fba833b28959`.
- ELF SHA-256: `91feeca6a4f574b998f2966bcdaa6549b7fb956c4d3aab6f04428153dfa6dd9e`.
- Controller trace SHA-256: `68bb8d49e8fd95b1ef0c3fb2f22ad1b866e9215e8c75a16377a90f68939d83c0`.
- Complete replay: `full-win/protoss-p5-1792819547611225000/profile.json`.
- Cargo save at controller tick 6000: `cargo-save/protoss-p5-1792819623935158000/profile.json`.
- Fresh-process resume: `cargo-resume/protoss-p5-resume-1792819678753722000/resume.json`.

The fresh process loads the exact authenticated 32 KiB SRAM through HOME L,
restores the observed live cargo links, source model, timers, death counters,
all persisted bytes and complete actor-bank hashes exactly, then advances one
neutral handoff frame and 60 live frames. Fifth-entry replay tooling reads the
real 48-byte mixed-width Building representation and nested persistent mission
state through DWARF; older fourth-entry observation schemas remain unchanged.

This raises staged completed missions to 19/30 (Terran 10, Zerg 4, Protoss 5).
Canonical distribution profiles remain Terran 10, Zerg 3, Protoss 3. P5's full
ARM mean is **12.69 effective fps** across counted live-play intervals, excluding
HOME/start and RESULT intervals. This is a playability qualification, not a
30-fps result or physical GBA verification. Z5 and later missions remain unfinished.


### Protoss five defense targeting optimization

The source-preserving defense lists are enabled in `P5_SHUTTLE_FLAGS` after a
complete ARM replay and cargo save qualification. The full win improves from
12.69 to **14.79 effective fps** with all ten checkpoint observations and three
PNG captures identical. This includes source action cursors, death counters,
persisted mission bytes and whole actor-bank hashes. Fresh-process SRAM
restoration is exact and continues through one handoff plus 60 live frames.
The unchanged IWRAM/EWRAM reserve guards pass. This remains below 30 fps.

Private evidence: `.local/build/promotion/p5-defense-qualified/qualification.json`.
Candidate ROM SHA-256: `bfad3cdb598cb2bc7dfb0542ec39e1315c44f67a4513f23ceb1bb1c3157cc1e5`.
Frozen ELF SHA-256: `6af9fc7e5bd2794196525558fd53b8a371a936d5843e727501438a1dc7b841ba`.
The actual-engine differential defense fixture also compares P5 source-player
death counters, ensuring earlier kills do not silently change later targeting.

### Z5 optional Consume integration

`zerg-z5-consume` adds a normal B-menu targeting mode and persisted approach
order. Actual-engine sanitizer checks cover a real `game_update(KEY_A)` cast,
source-box range 16 versus 18 source pixels (16 versus 17 GBA center offsets
for these actors), zero-cost consumption, +50 energy/cap250, hallucinated food,
wrong-source/foreign/invincible target rejection, pause, STOP and cancellation.
A valid synthetic script snapshot isolates that one-frame input test; it is
not a campaign playthrough. Active-order SRAM checks round-trip generation27
and reject corrupted links/caster identity or generation25 without changing
the live world or replacing the good SRAM.

The separate ability packer authenticates Consume and Ensnare permissions from
PTEC. Standalone Consume/Ensnare models pass sanitizer checks; Ensnare remains
unwired in the game. The door source facts were corrected against the pinned
UnitType flag expressions: raw205–208 are Invincible, not Building. Their GBA
building-pool policy and collision geometry are unchanged.


### Protoss five shared source-condition queries

The Shuttle profile now enables `SC64_P5_TYPE_FIRST_COUNTS`,
`SC64_P5_PRUNE_STARTED`, and `SC64_P5_COUNT_SNAPSHOT`. Already-started
one-shot records retain action/wait validation but stop querying unused
conditions. Six authenticated Command/Bring rows share a transient actor
snapshot built after rescue and discarded before action results are used.
The cache is absent from SRAM. Copied or modified condition rows fall back
to ordinary queries. Command includes cargo; Bring excludes it, and Current
Player follows rescue ownership while source provenance remains intact.
CHK Exactly uses comparison 10; opcode 12 is the authenticated Elapsed Time
>= 0 predicate, with malformed operands rejected.

Sanitizer checks compare 607,200 full/type-first count pairs, randomized
snapshot worlds and 192,000 scheduler ticks with matching action streams.
The combined full ARM win averages **18.28 effective fps**, versus 15.17 for
pruning/type-first alone and 14.79 for defense lists alone (original 12.69).
All ten observed checkpoints and three PNGs match the qualified win, including
persisted mission state and whole actor-bank hashes. A fresh process restores
the cargo save exactly and advances one handoff plus 60 live ticks. Normal
Make output matches the qualified ROM hash. IWRAM ends at `0x03004310` and
BSS at `0x0203d584`, inside unchanged reserve guards. This is below 30 fps.

Private evidence: `.local/build/promotion/p5-snapshot-qualified/qualification.json`.
ROM SHA-256: `d8906071993d2db4bc9ff8710314643d8d9103b844c2abc402d93336147de64f`.
ELF SHA-256: `256a023e655b555aaf89bab5214fd626c5267c2e305880324f0120b62cb2f51f`.
Trace SHA-256: `68bb8d49e8fd95b1ef0c3fb2f22ad1b866e9215e8c75a16377a90f68939d83c0`.

### Protoss six gated actor bindings and numeric props

`SC64_P6_ACTORS` appends distinct raw50/74/75 unit types and a raw210
building-pool prop without changing earlier enum values when the gate is off.
Source facts, mappings, bounds, supply and save-codec type capacity pass
asset-free sanitizer checks with the gate off, on, and mixed with prior actor
sets. The new units are untrainable scenario actors. Dedicated combat
capabilities remain disabled pending live integration; numeric weapon facts
alone do not enable ordinary attacks.

The private doodad emitter retains all 37 authenticated THG2 records, including
27 unit-backed props and ten SpriteType decorations. Three disabled raw205
rows retain their initial state. Emission does not claim raw210 trap behavior
is implemented, and runtime capability checks remain required. No P6 live
mission completion is claimed by these foundations.


### Zerg five complete normal-controller qualification

The generation-27 Consume profile has a full normal-input sanitizer win and
complete ARM replay. The controller launches through HOME and wins at game
frame 47901 / controller tick 48345, following the source gate-25 and
location-20 extraction sequence. Kerrigan survives with HP 185; two of the six
starting Zerglings survive. Her final cloak state is active and Q8 energy is
8637. The final source started mask is 198228763, with all action cursors and
wait timers zero and runtime fault zero. Replay writes only held-input events,
not actor positions, health, resources, source state or result flags.

Frozen private baseline evidence is under
`.local/build/promotion/z5-arm-qualified/qualification.json`:

- ROM SHA-256: `160c03ed21adc2e254fc81d40246c8ec2361c1fdcbe0faf9b7b1517a82dfa788`.
- ELF SHA-256: `1d29f0b5c2eb6d434f2696c79057de8ff223ca4bb3610e53872de9919c47fbe0`.
- Trace SHA-256: `fcf5615fd186d1db6531fbf6364b6ec321e34f3e9aefa04e876f37dac600c5e1`.
- Full replay: `full-win/zerg-z5-1792822969228526000/profile.json`.
- Save at tick 30000: `mid-mission-save/zerg-z5-1792823703306320000/profile.json`.
- Fresh resume: `mid-mission-resume/zerg-z5-resume-1792824117422200000/resume.json`.

The fresh process restores the observed saved world and persistent source
state exactly, then advances one handoff and 60 live frames to frame 29684.
This raises the staged total to 20/30 (T10/Z5/P5); canonical profiles remain
T10/Z3/P3. Baseline full-mission performance is **8.78 effective fps**.
Performance qualification is separate from a source-model or host win, and
physical GBA verification remains outstanding.

### Protoss six live integration checks

The internal generation-28 P6 probe launches the actual imported roster of
200 units, four beacons and 27 unit-backed props, retaining all 37 THG2 rows
in metadata. Sixty real game updates run without a source fault. The focused
sanitizer checks cover native size factors, shield/HP damage order, source-box
detection, property-driven burrow, suicide splash, trap cooldowns and death
provenance. Candidate-only saves allow moved source actors, reject malformed
identities before live mutation and round-trip a normally moved actor and a
destroyed trap with negative HP. P5/P6 share a bounded mission-state union;
P5-only builds retain their prior layout. Reserve guards are unchanged.

The source door dispatcher now succeeds on an empty orientation selection,
as required by trigger 2's raw208 location-1 action. Door/removal callbacks
validate their own effects while the scheduler cursor/timer transaction is
in flight; whole-runtime validation follows dispatch. The actual trigger-2
regression checks the neutral-owned raw207 row and the empty raw208 action.
The matching N64 handler routes raw modifier 0 and 6 to the same state-test
and opposite-state command path; 4 and 5 take direct command paths. This
establishes toggle semantics, not native visual/collision equivalence.

Framebuffer comparisons use the actual P6 scene, support its zero-mineral
roster, restore complete actor records before SRAM tests, and compare source
door actions with cold and warm caches. P5 and Z5 also pass the corrected
fixture on their actual fifth missions. The P6 ARM probe ends IWRAM at
`0x030045d8` and BSS at `0x0203d754`, below the unchanged limits. Full P6
controller/ARM wins and fresh-process save qualification remain incomplete;
these checks do not add P6 to the completed mission count.


### Z5 shared condition-query performance qualification

The Consume target enables `SC64_Z5_PRUNE_STARTED` and
`SC64_Z5_COUNT_SNAPSHOT` after the complete frozen comparison. Already-started
one-shot records stop querying unused conditions; preserved triggers 7, 8 and
34 continue to poll. The transient 96-byte snapshot batches 34 authenticated
Command/Bring conditions and is discarded before action results are observed.
Actor bounding-box overlap, source ownership and fallback errors remain the
same as the complete scan. Numeric CHK Exactly is comparison 10; switch
comparisons retain 2/3. Unsupported or copied query rows use full scans.

Sanitizer differential checks cover 150 randomized real-array worlds and
12000 scheduler ticks with equal persistent state and action streams. The
full ARM win rises from **8.78 to 17.73 effective fps**. All 50 observed
checkpoints and five PNGs match the baseline. The fresh mid-mission save
restores exactly and advances one handoff plus 60 live frames. IWRAM ends at
`0x03004fe0` and BSS at `0x0203d858`; both unchanged reserve guards pass.
This remains below the 30-fps goal and is not physical GBA verification.

Private evidence:
`.local/build/promotion/z5-snapshot-final-qualified/qualification.json`.
ROM SHA-256: `9d21dcfd73022378ecba6fed2315d8b1b10a5b02076059ccbd1dc5b5704613bf`.
ELF SHA-256: `de57d1e5d3b3f6336ca4c6ea04aa6e97794eab4b814a1be5c802b4bbd0632ddd`.
The controller trace is the same `fcf5615f…00c5e1` qualified above.


### Sixth-chapter live probes and seventh-chapter source foundations

Z6's generation-29 internal probe passes normal sixth-entry menu startup,
all six campaign start/save checks, the 234 skirmish race/map/AI configurations,
actual-map runtime and hallucination tests, and candidate-only SRAM rejection
without live mutation. A normally moved source actor survives save/load;
invalid timers, provenance, geysers and older generations are rejected. Its
actual-scene framebuffer checks match complete clears with cold/warm caches.
The frozen readiness build ends IWRAM at `0x03004ff0` and BSS at `0x0203d950`.
The fixed reserve limits remain unchanged. These are startup/readiness checks,
not a full controller/ARM win or a 30-fps claim. Z6 hero spell controls and
active Shield Battery recharge remain incomplete.

For P6, upper-level raw205/206 door props now follow the pinned PC reference's
dynamic noncollision policy. Authored raw207/208 pit-door gates retain their
state-dependent blocking. This resolves a disconnected route to Zeratul in
the GBA map; native N64 door/tile mutation is still unverified. Regression
checks retain source state changes, empty selections, save validation and
framebuffer parity. Full P6 playthrough qualification remains pending; its
source-enabled Tassadar Storm control is a known missing mechanic.

Z7's isolated scheduler/world fixture uses the actual 231 source rows without
owner overrides: 113 units, 57 buildings, 60 mineral fields and one geyser.
Queries use source-pixel actor bounds and numeric Exactly comparison 10;
switch comparisons remain 2/3. Setup resources, terminal outcomes, timers,
opaque-AI call mask and paid Zergling queues pass sanitizer checks. P7's
asset-free model and authenticated numeric adapter also pass. These foundations
are not additional completed missions. The staged total remains **20/30**.


### Sixth-chapter ability integration and optional query experiments

The generation-30 P6 probe adds authenticated full-energy Tassadar Storm;
generation-31 Z6 adds full-energy Kerrigan Cloak and Storm with the source's
hero research exemption. Consume remains disabled in Z6's PTEC. Ensnare and
Shield Battery recharge are still missing. The actual-game spell fixture
launches through the normal sixth-entry menu, casts through field controls,
round-trips an active effect and exact energy, preserves energy on Stop,
freezes effects/energy while paused, and rejects corrupt effect geometry
before mutating the live world. The separate Z6 cloak fixture tests real
Observer/Cannon geometry, ownership, liveness and completed-building detection,
plus active-cloak save/load and malformed energy/state rejection. Photon
Cannon's grounded detection range is seven tiles in the pinned PC policy,
independent of its eleven-tile sight entry. No new full playthrough is claimed.

These spell builds retain the SRAM/stack/RAM guards. The Z6 build uses a shared
ROM attack-eligibility function to avoid copying expanded policy into hot
loops; IWRAM ends at `0x03004f80`, BSS at `0x0203d978`. P6 ends at
`0x030045d8` / `0x0203d774`. Both ROMs are 4 MiB.

Optional P6/Z6 one-shot pruning passes two-translation-unit differential
checks over 12000 ticks, with equal state and action streams, pause, terminal
outcomes and invalid callbacks. The P6 prevalidated path skips only immutable
ROM-table validation after successful initialization/load preflight. Its public
adapter retains full validation. These options remain off by default.

A 1000-frame normal-input P6 startup comparison measures **3.54 fps** baseline
and **3.65 fps** with pruning plus prevalidation. The handoff and eleven later
observations, with twelve PNGs, match exactly. This is a bounded opening-scene
measurement, not full-mission performance or hardware verification. It remains
far below 30 fps. Private reports are under
`.local/build/promotion/p6-query-options/{baseline,prune-meta}/startup/`.

P7's isolated world tests preserve the actual human 2500/2500 resource row,
separate 10000/10000 Force wallets, exact source ownership for a raw69 creation,
and fail-closed raw72 creates. The canonical roster still rejects missing
raw68/71/72 bindings. Its opaque AI policy issues existing-unit orders only;
full native AI, production and playable P7 integration remain unfinished.
The staged completion total remains **20/30**.


### Z6 transport/save integration and P6 opening optimization

The Z6 probe adds paid Ventral Sacs research, shared Overlord boarding and
unloading, and generation-32 transactional saves. The authenticated source
starts with three Overlords, a Hatchery, and no transport upgrade. PUNI/UPGR/
UPGS facts require a Lair/Hive and 200 minerals + 200 gas; 2,400 native frames
become 3,000 active 30-Hz updates. Cancellation follows the pinned PC upgrade
policy: 75% on destruction, full refund on ownership-loss cancellation.
Native walkability places the objective island outside the opening ground
component; no terrain cells are changed to manufacture a route.

The save state also records up to four Drone morphs so construction does not
look like unrecorded starting-unit deaths. The source Hatchery can retain its
identity through an in-place Lair morph. Cargo validation uses candidate
research, source owner/type identity and embarked capacity; pending boarding
intents and the final empty unload tick retain the shared engine's behavior.

P6 Storm now hits source NonBuilding traps stored in Building slots. The
actual spell/control fixture checks their source-death accounting, successful
casts stop previous movement, and the B+R shortcut preserves a single exhausted
caster's selection rather than invoking bulk R selection.

The P6 opening query experiment combines four default-off options:
`SC64_P6_PRUNE_STARTED`, `SC64_P6_PREVALIDATED_TICK`,
`SC64_P6_TRAP_CULL`, and `SC64_P6_COUNT_SNAPSHOT`. The conservative trap reject
passes 100,000 randomized geometry proofs and 150 full-array target/damage
comparisons. The transient snapshot covers every queried source rectangle,
groups source-selector tests per actor, and is invalidated before action/AI
callbacks. It passes 150 randomized count worlds and a 12,000-tick scheduler
comparison against complete scans. Its bounded stack object stays below
512 bytes and adds no SRAM or BSS fields.

A frozen 1,000-frame normal-menu startup comparison improves from **3.54 to
5.81 effective fps**. Handoff plus eleven later complete observations and
twelve framebuffer captures match exactly. The candidate retains IWRAM
`0x030045d8` and BSS `0x0203d774`, within the unchanged reserve limits. This
is an opening-only improvement; full winning-route qualification and physical
GBA measurements are still pending, and it remains far below 30 fps.

Private reports:
`.local/build/promotion/p6-query-traps-snapshot/{baseline,selectors}/startup/`.
Baseline ROM SHA-256:
`a635dba2b33962447af11236ba22adf16ad6d558e6a424f192512d9e855254d8`.
Candidate ROM SHA-256:
`147ad597f7984c332dd8e3df6da7c27ffbd73513957a8bcea9de5f3c8b24d5f2`.
Candidate ELF SHA-256:
`e4839c0844b1ba44d4178a53d80add6be950ce148414422c978896f7304f5b14`.
The staged completed total is still **20/30 (T10/Z5/P5)**; sixth missions are
internal probes until complete normal-input and emulator wins are verified.


The transport/query source snapshot passes the original-art source-only build
and all default host regressions plus **314 Python tests (six private-data
checks skipped)** in a fresh index export with no staged game assets. The
Git index audit reports 498 text source files and no game/build/credential
payloads. Both sixth-profile normal-menu/save matrices retain all 234 skirmish
configurations; Z6 frame coverage remains pixel-identical to full clears. The
actual Z6 transport fixture passes paid timing, pause, capacity, unloading,
Kerrigan cargo, Drone-morph save continuity, malformed save rejection and
carrier/passenger death accounting under ASan/UBSan.


### Z6 full-state startup observer

The Z6 replay observer decodes the actual 176-byte persistent state from ELF
DWARF, including nine action cursors/timers, eight switch words, source deaths,
mutable locations, AI cadence, Cloak/Storm, transport research and Drone morphs.
It hashes all 240 compact Unit rows and 160 compact Building rows, including
retained dead provenance. A future full replay must require source victory
trigger 5, neither defeat trigger 0/6, and a live human source-3 Kerrigan.
Eleven startup-tool and 25 replay-tool asset-free tests pass; P6 defaults and
report comparisons are retained.

A normal-input 1,000-frame Z6 neutral opening runs at **15.24 effective fps**,
with no runtime fault or terminal outcome. It reads all persistent fields and
captures the handoff plus eleven later checkpoints. This is a startup baseline,
not a full mission win or a 30-fps/hardware claim. The frozen engine commit is
`b3bc50d`, with IWRAM `0x03004f00` and BSS `0x0203d980`. Private ROM/ELF copies
and the hash manifest are under
`.local/build/promotion/z6-transport-frozen/`; the report is under
`.local/build/promotion/z6-transport-startup/`.

The Z6 normal-menu controller and ARM startup observer both hand off at
host tick 12, live frame 1. The trace validator requires that exact frame
and rejects frame 0. Actual spell/control checks cover army B+R excluding
Kerrigan, regular R including her, and sole-Kerrigan Storm precedence at
both sufficient and insufficient energy.

The observer/control checkpoint passes a fresh source-only build, all default
host regressions and **320 Python tests (six private checks skipped)**. The
source-only ROM remains 65,536 bytes with SHA-256
`7c6b3068b747bd4b7375294bd0d63d5797665285f9bf622b226d6ffb2cf3de76`.
The unified ARM build and sanitizer suite pass; the Z6 ARM build retains
IWRAM `0x03004f00` and BSS `0x0203d980` with unchanged reserve guards.

### P6 burrow-release experiments

Two additional default-off options avoid repeated work in the P6 ambush
check. `SC64_P6_BURROW_CULL` rejects actors outside the exact 96-pixel circle's
axis bounds before hostility checks. `SC64_P6_BURROW_GROUPED` builds a list
of alive hostile actor slots once per burrower owner, using a bounded
240-byte local array and no new BSS or SRAM. Changes to the burrow flag, order
and target cannot change the target list's alive/owner/position predicate.
Unsupported live owner values fall back to the original scan.

All four option combinations pass ASan/UBSan comparisons against an independent
full-scan translation unit across 150 randomized worlds. Focused cases cover
96/97-pixel boundaries, every supported owner group and invalid-owner fallback;
the axis reject also passes 100,000 conservative geometry checks.

Combined with the four query flags above, the frozen 1,000-frame P6 opening
improves from **3.54 to 8.37 effective fps** (the axis reject alone reached
7.31). Handoff plus eleven later state observations and twelve framebuffer
captures match the baseline exactly. IWRAM remains `0x030045d8` and BSS
`0x0203d774`, with the original reserve guards. These are startup experiments;
the flags remain disabled by default pending complete winning-route
qualification, and neither 30 fps nor physical GBA performance is established.

Private comparison report:
`.local/build/promotion/p6-query-traps-snapshot/grouped/startup/protoss-p6-startup-1792834848572956000/profile.json`.
Candidate ROM SHA-256:
`cb8f7a1edf16fa18c0b0177c09a97f7ccb675b6d7467796634e84c98b5a3ddff`.
Candidate ELF SHA-256:
`be3a3a6ca4f654a8d27c3bb4c7803228cf64ebf925b9c4e1e20bc97f123e34cc`.
The staged completed total remains **20/30 (T10/Z5/P5)**.

The burrow/resume source snapshot passes the clean original-art build and all
default host checks, including **324 Python tests (six private checks skipped)**.
Z6 save-prefix authentication now checks all 176 persisted bytes and the source
hero using the observer's live-only entity rows, plus trigger/timer, switch,
MRGN, AI, Cloak/Storm and transport relations. Thirteen focused resume tests
pass, and the real retained startup snapshot passes this runtime validator.
This does not establish an actual fresh-process Z6 SRAM handoff yet.

### Probe expansion command

The shared race menu now exposes NEXUS after STOP / HOLD when a Probe is
selected. This uses the existing paid placement and construction path; a Nexus
costs 400 minerals and does not require Pylon power. The extra row scrolls
within the existing ten visible rows and leaves hero spell/transport menus in
their existing selection contexts. No command indices or save fields change.

Actual UI fixtures in basic unified, P6 and Z6 builds pass under ASan/UBSan:
menu selection/rendering, denial at 399 minerals, successful expansion at
400 minerals, Probe release, timed completion, added supply and Probe
production at the second Nexus. Economy/arrival setup isolates this regression;
these fixtures are not complete campaign wins.

### Construction and Hydralisk visual pass

This is the historical Hydralisk-only pass. The later weapon-family pass
described below replaces its renderer across implemented attack paths.

Hydralisk/Hunter Killer shots now carry a transient needle style in the existing
tracer color field. Four visible updates draw a moving segment capped at five
pixels with a bright head, instead of a full red/gold beam. The firing update
still applies damage immediately. Host tests cover geometry bounds, reversed
and degenerate paths, both shooter owners, actual firing/aging/render order and
unchanged tracer layout. No new save fields or runtime arrays are introduced.

Construction uses existing progress and game-clock values for authored Terran
scaffolding/sparks, Zerg morph pulses and Protoss warp pulses. The completed
source sprite is revealed from the bottom as progress advances. Worker-en-route
markers stay static. Incomplete art is kept out of the terrain cache; both
static-key implementations pack the construction discriminator into an existing
field so completion/removal invalidates cached art without adding RAM.

Asset-free framebuffer tests cover distinct races, animation, waiting states,
clamping and invalid input. P6 and Z6 actual-scene coverage compares repeated
waiting/building/completion/removal transitions for all three races against
full-clear rendering and passes. The extra Nexus command is restricted to
Probe-only selections, keeping mixed transport/spell contexts unambiguous.

This is a visual improvement, not native IScript reproduction. The local ROM
contains construction and Needle Spines images, but their sequence mapping is
not integrated. The native projectile timing/order and construction script
signals remain fidelity gaps. The later opt-in
[source construction art preview](#source-construction-art-preview) adds
primary Terran/Zerg frame sequences; canonical builds retain this authored pass.

The final UI/visual source snapshot passes a clean original-art build, all
default host regressions and **324 Python tests (six private checks skipped)**,
including the two new asset-free VFX suites. The source-only ROM is 65,536
bytes with SHA-256
`4ad80ce0319a9e9660a909c29414b7d8df86de036c6da459c0e158a0c50f9aee`.
Basic unified and both sixth-probe ARM builds pass the unchanged reserve guards:
P6 IWRAM `0x030045d8` / BSS `0x0203d774`; Z6 IWRAM `0x03004eb8` /
BSS `0x0203d980`. Unified sanitizer regressions and the three Probe expansion
profiles pass. The 506-file index audit contains no game/build/credential
payloads. Full campaign replays and physical-hardware checks remain separate.

### Visual build propagation and Z6 host playthrough

After the construction/needle changes, `make race-campaigns
protoss-p5-shuttle zerg-z5-consume` rebuilds and verifies all five outputs.
The full `./dev sc64 test --profile terran` sanitizer suite passes, including
all ten mission controllers. The existing reserve guards remain unchanged:

| Output | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004e20` | `0x0203d9ec` |
| Zerg | `0x03004ef8` | `0x02039878` |
| Protoss | `0x03004e80` | `0x02039a20` |
| P5 Shuttle | `0x03004310` | `0x0203d584` |
| Z5 Consume | `0x03004cd0` | `0x0203d858` |

`make test-sc64-z6-controller` now completes a normal-input host win: paid
Lair/Extractor/gas economy, mainland Force2 clearance, Ventral Sacs research,
Overlord boarding/flying/unloading and the source victory sequence. The final
state is tick 271799 / gameplay frame 268694, outcome WON, started mask
`0x1be`, no active action cursors and zero faults. Kerrigan is live raw51,
source owner3/runtime owner0, idle and unloaded at `(950,1009)`; the source
Force2 AnyUnit counts in locations 4 and 5 are both zero. Research is complete
and the carrier has no cargo. The replay validator accepts all 9448 recorded
held-state transitions. Sanitized controller cleanup and the Make profile
preserve every input transition and terminal core field of the original win.

The public-controller trace SHA-256 is
`5199d4329a303f81b20e289351a0ea6987e33b64e3fb193cce640d48394ad894`.
The first-win ARM qualification uses a frozen earlier trace with identical
inputs, SHA-256
`a5fde71c7b6b95f8e87b34edae404794826e3f5102a7a4c2b2c8523b4c0f2523`.
Its frozen engine source commit is `62cc58f`, ROM SHA-256
`8b9c54475c6b5cbe7f8d0871645e3d9e4c5c6cc4f498f80bdad471a38c0c0ffa`,
ELF SHA-256
`8ab1fe4d7994c2192cd62b4fb635f017d52c4cdc09ae44fbc0619c8bca0967f6`.
Private artifacts are under `.local/build/promotion/z6-first-win-frozen/`.
The full ARM replay now wins at gameplay frame 268694, matching the host
terminal state across 273 checkpoints. Its mean live-play effective rate is
17.83 fps on the frozen emulator build. This falls short of the near-30-fps
target and does not measure physical hardware. The 150,000-tick prefix saves
at frame 148268, but its fresh-process HOME-load check fails. SRAM continuity
is therefore not qualified, and the staged qualified total stays at 20/30.

### Further default-off P6 scan experiments

`SC64_P6_RESCUE_GROUPED` builds a frame-local list of live SIDE unit slots,
appending each newly rescued unit at its original outer-loop position. This
preserves forward rescue chains, the absence of backward revisits, and the
subsequent building rescue pass. The independent nested-scan oracle matches
whole production unit/building arrays and cache invalidation across 1200
random worlds, enemy/neutral/provenance rows, exact radius boundaries,
ascending/descending chains, full capacity and invalid-state/no-op gates.
The candidate list uses 240 stack bytes with no persistent storage.

`SC64_P6_STORM_POSTCHECK_LOCAL` retains full entry validation, Storm geometry,
all-owner live raw-79 energy/remainder scans, cast checks and save/restore
checks. It skips only repeated mission-model/death/doodad validation at the
successful tick exit: the model and doodad fields are unchanged during that
tick, and source-death updates saturate within their validated bound. This
does not remove a unit-pool scan. Two separately compiled adapter paths match
return values and state across 1320 seeded valid/invalid cases. The actual
campaign entry/cast/pause/save fixture passes with the option off and on.

Private ARM builds with the previous six query flags, seven including grouped
rescue, and all eight including the local Storm post-check pass the unchanged
reserve guards at IWRAM `0x030045d8` / BSS `0x0203d774`. The actual-game spell
fixture also passes with all eight flags together. A new 1,000-live-frame
startup comparison measures 8.37 effective fps with the previous six flags,
9.50 with grouped rescue, and 9.51 with all eight. Both candidates match the
handoff and all eleven observed states and captures exactly. This is startup
evidence only; the options remain disabled by default with no full-mission
performance or completion qualification. Private builds and manifests are under
`.local/build/promotion/p6-rescue-postcheck/`.

The final 512-file source index passes a clean original-art build and the
default host suite, including 324 Python tests with six private-data checks
skipped. Its 65,536-byte ROM retains SHA-256 `4ad80ce0…f9aee` recorded above.
The index audit contains no ROMs, imported/generated private payloads, build
outputs or credential files. The active P6 controller experiment is excluded.

### Z7 shared-engine integration readiness

The internal `zerg-z7-probe` profile exposes The Culling as the seventh Zerg
menu entry without changing canonical campaign profiles. It extends the Z6
storage/actor gates, retains all 231 non-start source rows (113 units,
57 buildings, 60 minerals and one geyser), and runs the source scheduler
through the shared mission update. Source owner 3 maps to the human and owner
4 to the hostile Zerg. The objective uses the hostile source building count;
enemy units do not also have to be eliminated. The explicit paid-production
AI policy remains an approximation of the opaque native scripts.

ASan/UBSan checks pass for all seven menu starts and the 234 skirmish
startup/save configurations. Full-clear rendering matches reused-terrain
rendering, including cold/warm frames, construction, resource mutation,
save/load and restart. These checks do not establish complete skirmish or
campaign wins.

Generation-33 saves reuse the existing larger Z5 union payload. Pure candidate
validation checks source owners, actor/building identities, source positions,
resource depletion, paid-production capacity and the two-unit Zergling egg
yield before touching live state. The actual-engine fixture accepts human
paid production and enemy paid egg queue/completion save round trips. It
rejects fabricated heroes, mismatched building types, invalid provenance,
resource overdraw and capacity overflow transactionally. Forged terminal
outcomes are rejected against decoded world counts; genuine source-condition
win/loss fixtures validate. Those synthetic terminal fixtures are not
normal-input mission playthroughs.

The ARM build passes unchanged reserve guards at IWRAM `0x03004eb8` and BSS
`0x0203d9e8`, producing a 4 MiB ROM. No Z7 normal-input controller win, full
ARM replay, fresh-process SRAM qualification or frame-rate result is claimed.
The importer capability report adds raw 39/44 units and 138/140 buildings
only for this opt-in profile; the legacy capability census remains unchanged.

The 514-file source index passes a clean original-art build, default host
checks and 326 Python tests with six private-data checks skipped. The default
ROM remains 65,536 bytes with SHA-256 `4ad80ce0…f9aee` recorded above.
Unified ARM and host regressions also pass. No game assets, generated private
headers, ROMs, captures or experimental P6/Z7 controller traces are included.

### Z6 cold HOME-load correction

The failed fresh load above was traced to candidate validation using the live
`geyser_count` before any map had been imported. The saved Z6 count is three;
a fresh title process has zero. Validation now derives the expected count
from immutable raw-188 map rows and matches each candidate geyser uniquely
against those rows. It no longer depends on the active world's resource
count. The generation-32 record layout and memory reserve guards are unchanged.

The actual-engine ASan/UBSan fixture now loads through HOME L after clearing
the resource globals to their C-startup values. It also rejects misplaced and
duplicate geysers without changing live state. Transport, Z6 runtime/save,
Z6 startup/skirmish matrix, Z7 save, default and unified checks pass.

The corrected ROM SHA-256 is
`bc24435f6e3bc47ce90512d1eb2ca814bc03168ce795b83bf5caa2a785a15e8c`.
A fresh emulator restores its 1,000-tick prefix snapshot exactly through HOME
L at frame 967, takes one neutral handoff frame and then 60 measured live
frames to frame 1028. That is an opening save check, not the late-mission
qualification. The 150,000-tick prefix now also passes exact fresh-process
HOME restore at frame 148268, one neutral handoff frame and 60 measured live
frames to frame 148329. Its resumed interval measures 17.15 effective fps;
this is a 60-frame sample, not a full-mission rate. The corrected build's
full replay remains running, so staged qualification remains 20/30.

### Default-off Z6 count-phase snapshot

`SC64_Z6_COUNT_SNAPSHOT` builds eight Command/Bring query counts with one scan
of the live unit/building banks. Current Player is resolved separately for
each source trigger, and positive unmapped raw IDs still count for AnyUnit as
they do in the existing scan. The cache is local to the scheduler's condition
phase and discarded before action dispatch, including failed dispatch;
unsupported metadata, locations or source owners retain full-scan fallback.
The snapshot occupies 30 stack bytes, with a builder bounded to 56 bytes and
seven small location rectangles. No BSS or saved state is added.

ASan/UBSan checks match 500 synthetic full-bank worlds and 100 randomized
actual-engine worlds against the production full scan, including Current
Player, raw-type fallback, unmapped IDs, buildings, moving rectangles,
half-open boundary cases, invalid-owner failure and action invalidation.
Runtime and cold-load/save checks pass with the option enabled. The complete
normal-input host controller still wins and produces the identical trace
SHA-256 `5199d432…ad894` recorded above.

The ARM candidate passes unchanged reserves at IWRAM `0x03004eb8` / BSS
`0x0203d980`, with ROM SHA-256
`435f18383b287f2315d230e8c338a0fff7184655b35d238c3859d455720975e3`
and ELF SHA-256
`8865a640f1f19e5fc69cff5c6caa45309598e288b0d0d968976410ab0f562eeb`.
Private frozen inputs are under `.local/build/promotion/z6-count-query-frozen/`.
The option remains disabled by default pending the queued ARM comparison;
no frame-rate gain or full ARM equivalence is claimed yet.

This optimization preserves the current center-in-location predicate. The
pinned reference instead uses actor-area intersection for Bring queries and
supports elevation filtering; those boundary semantics remain a separate Z6
fidelity gap and are not concealed by the count-cache equivalence checks.

### Z7 colony defense commands and native morph HP

The Z7 development profile enables `SC64_Z7_COLONY_DEFENSE_ENABLED` and uses
save generation 34. A Drone-only selection adds CREEP COLONY after SPIRE;
the ordinary placement path pays 75 minerals and consumes its Drone on
arrival. A selected Creep Colony exposes SUNKEN in the contextual command
row. Morph admission requires completion, source owner 3, a completed Pool
and 50 minerals, and consumes no additional worker.

The pinned OpenBW building-morph path retains Creep identity through the
morph, then applies `current HP + target maximum HP - prior maximum HP`,
with a floor of one, at completion. The gated adapter uses the existing
375-tick construction clock, preserves Creep armor and zero shields while
incomplete, and applies the 400-to-300 maximum-HP adjustment at completion.
It uses a reserved marker and the otherwise-unused producer progress field
of this non-producing building; no saved fields or actor arrays are added.
Pre/post hooks return immediately outside Z7.

The actual-engine ASan/UBSan fixture passes menu scrolling/dispatch,
occupied-site and insufficient-funds rejection, exact payment and worker
consumption, real damage before/during morph, completion armor/HP, full
save/load and cold HOME restore. Malformed morph rows and a generation-33
payload in a generation-34 record are rejected. A morph killed after the
production phase remains dead through save/load, clears its marker on the
next neutral frame and saves again. Mixed Drone/army selection hides the
construction row.

All seven campaign starts and 234 skirmish startup/save configurations pass,
as do full-clear/reused-terrain rendering checks. The 4 MiB ARM build passes
the fixed guards at IWRAM `0x03004eb8` / BSS `0x0203d9f0`. These checks do
not establish a Z7 playthrough, a full skirmish win or hardware performance.
Parallel native larva/egg production remains a separate fidelity gap: the
current Hatchery processes paid orders serially, despite three stored larvae.

### Full skirmish matches across the three canonical builds

The public `sc64_skirmish_campaign_run.c` controller now runs through normal
HOME/custom-game menus and paid construction, production and combat inputs.
The matrix checks two maps, three human races and one to three opponents,
with rotated opponent races, against each of the three canonical source
profiles. All **54** host sanitizer runs reach a terminal state within the
120,000-input-tick bound: Terran build 1 win/17 losses, Zerg build 2 wins/16
losses, Protoss build 3 wins/15 losses. These are engine match completions,
not winning strategies for every configuration or all bot-race permutations.

A Protoss-build match with human Terran against two AIs originally exceeded
the 60,000-tick cap. It wins at tick 65,606/frame 65,541 under the larger cap;
its entire input prefix through tick 60,000 is identical. No AI, economy or
world-state changes were used to obtain that result. Private reports live at
`.local/sc64/skirmish/{terran,zerg,protoss}-matches/matrix.json`.

The new ARM observer validates input transitions, normal HOME entry, configured
participants and terminal witnesses before launch, then reads the matching
ELF's real mixed-width actor banks. It checks terminal frame, result, map,
races, owners and each participant's live unit/building counts. Human wins
require surviving human buildings and zero buildings for every participating
bot; residual bot units are allowed by current skirmish victory rules. Its
17 asset-free tests pass. Full skirmish ARM execution and hardware timing
qualification remain pending.

### Dense scheduler and P6 host completion evidence

The P6 controller has a complete normal-input host win at frame **172327**
(input tick 174855), with living rescued Zeratul inside source extraction
location 15 and living Tassadar. Trigger state reports WON, zero fault and
completed action cursors/timers; the mission completion bit is asserted.
The private opening input prefix currently has no recovered public policy
generator, so this is private replay evidence, not a reproducible public
controller target. The experimental controller and trace remain untracked.

The corrected trace records its actual start at tick 12/frame 1, following
the normal sixth-entry menu route. It retains all 15,554 original input
transitions and their SHA-256 `e33412dc161b332e8ca66c2a93e8d01fa9945f22ea09c7c224c0139e4fff1c09`.
Its full trace SHA-256 is
`e66d25a4df98f017efbefc679c30bbbba91e8566643a3d3db65fc7a319000421`.

The opt-in dense scheduler observes every 250 ticks so those inputs fit the
existing 256-entry queue (peak 199 in this P6 trace). The real main-loop
sanitizer fixture also tests a transition every tick, empty windows, held keys
and pressed edges through page boundaries. Default 1,000-tick scheduling
remains unchanged. The dense P6 ARM build preserves IWRAM `0x030045d8` and
BSS `0x0203d774`; its 4 MiB ROM SHA-256 is
`b754e1f12cef830462110808834d56c04e20f8b7a5cfe4df85ad4ef9d51bdc81`.
Frozen inputs are under `.local/build/promotion/p6-dense-winning-frozen/`.
The full ARM replay and fresh-process SRAM checks are queued; this host win
does not increase the qualified campaign total or establish a frame rate.

### Z6 corrected-build full qualification

The corrected-loader frozen build now completes the full normal-input ARM
replay at tick 271799/frame 268694, with verified victory, living unloaded
Kerrigan, source objective clearance and the mission completion bit. Its mean
live-play rate is **17.83 effective fps**, below the near-30-fps target.
Both the opening and 150,000-tick saves restore exactly in fresh HOME processes;
the late save resumes from frame 148268 through one neutral handoff frame and
60 measured live frames to 148329. This qualifies staged Z6 and raises the
staged total to **21/30 (T10/Z6/P5)**. Canonical profiles remain T10/Z3/P3.

The qualified ROM SHA-256 is
`bc24435f6e3bc47ce90512d1eb2ca814bc03168ce795b83bf5caa2a785a15e8c`,
ELF SHA-256 is
`d48e8f637af53527a4b7318a24239fcdbbad683cc81a9a849c5ffa6fb04fcef1`,
and the frozen input trace is `a5fde71c…f2523` recorded above. Reports are
under `.local/build/promotion/z6-coldload-qualified/`. This supersedes the
first-win build's failed cold-load result without concealing that earlier
failure. The default-off count-snapshot performance comparison is separate.

The 523-file source-only export passes the original-art build and default
host regressions, including 344 Python tests with six private-input checks
skipped. The source index audit contains no game/build/credential payloads.
The local default/unified builds and host suites also pass. No physical GBA
performance or complete Z7/P6 campaign qualification is claimed.

### Z6 count-prefix comparison and first full skirmish ARM sample

The default-off count snapshot improves the frozen 12,000-input-tick Z6
opening from **9.52 to 9.82 effective fps** (3.15%). All 13 reported checkpoints
match after excluding timing/capture metadata, and the four HOME/1k/10k/12k
captures match as decoded pixels. The post-processing used bundled Python
and Pillow after the first environment lacked Pillow; neither replay was
restarted. This is prefix evidence, not a full-mission or physical-hardware
speedup. The option remains disabled by default. The private comparison is
`.local/build/promotion/z6-count-comparison/comparison.json`.

The first complete skirmish ARM sample replays the canonical Terran build,
built-in map, human Terran and one Zerg AI. It reaches the exact host loss at
tick 11539/frame 11522, with matching configuration and participant counts.
The live-interval mean is **29.72 effective fps**, excluding HOME/start and
RESULT intervals. This verifies one complete one-AI sample, not all maps,
races or opponent counts. Two-AI Zerg-build and three-AI Protoss-build samples
remain queued/running. Frozen pairs and reports are under
`.local/build/promotion/skirmish-dense-frozen/` and `skirmish-dense-qualified/`.

The two remaining frozen skirmish samples also complete with exact terminal
host witnesses. The Zerg ROM on the imported map, human Zerg against two AIs,
reaches its loss at tick 24318/frame 18280 and averages **28.41 effective fps**.
The Protoss ROM on the imported map, human Protoss against three AIs, reaches
its win at tick 16338/frame 16300 and averages **26.10 effective fps**. All
three samples preserve the configured human/bot races and owners, final
frame/result and live actor counts. These cover one representative match per
ROM and AI count, not all 54 host cases or physical GBA behavior. Summary:
`.local/build/promotion/skirmish-dense-qualified/summary.json`.

### Fresh staged preparation and promotion checks

The `dev` wrapper now exposes `zerg-z6` and `protoss-p5-shuttle` preparation,
build, test, status and play routes. Seventeen onboarding checks pass,
including empty-directory packer-output coverage. An actual isolated source
export, initially containing no generated private inputs, successfully prepares
and builds both profiles from the existing authenticated user ROM. Both status
commands report READY and both ARM builds pass the fixed memory guards.
The fresh Z6 ROM is byte-identical to the qualified `bc24435f…15e8c` build.
Fresh P5 Shuttle produces a 4 MiB ROM with SHA-256
`6b51962db99d068cf31627493cb211e4199a1e27281f8c60bcb2d19e8fdbfe81`.
Private commands/logs are under `.local/build/promotion/staged-prepare-fresh.json`.

Profile-aware controller launchers now retain exact mission/ordinal checks
without assuming that the catalog ends at chapter four. Under the larger Z6
bank, genuine-input controllers win Z1–Z5; under P5 Shuttle they win P1/P2/P4
and the P5 route. P3 remains a promotion gate: its starting roster/resources
match the canonical build, but its paid AI uses the larger pool (240 rather
than 128 units) and the old player policy does not complete at its full limit.
No AI cap or victory condition was reduced to force a pass. Canonical profile
promotion remains unfinished.

The legacy non-unified four-mission controller check completes Z1/Z2/P1 but
fails P2. Compiling the unmodified committed controller reproduces the identical
P2 loss trace (SHA-256 `dce74d7c…e45f5`); this is a pre-existing route failure,
not a changed non-unified launch sequence. The canonical expanded P2 route
passes separately. This failure is not counted as a green regression.

All eight default-off P6 query options now reproduce the entire frozen host
winning trace byte for byte (`e66d25a4…00421` above), including every input and
checkpoint. The fast dense ARM candidate retains the same memory ends and
has ROM SHA-256
`0e1f791e52773308b00925e88bfd949e4ff6d6be109a27e9d7552832457da447`.
It passes opening fresh-process SRAM restore; late-save and full ARM win
qualification are running. This supersedes the slower candidate's incomplete
late replay for qualification; its completed opening save evidence is retained.
The options remain disabled by default until full qualification completes.

### Public self-contained P6 normal-input regression

`make test-sc64-p6-controller` now reproduces the full host win without an
ignored opening file. Its authored fixture stores 6,418 held-mask/run-length
pairs for the initial 102,005 input ticks. Independent expansion matches the
original numeric input stream exactly (SHA-256 `fe242d01…aed79`), and every mask
is within the ten controller bits. The fixture contains no actor/world rows,
coordinates, sprites or ROM data. It is authored regression input data.

The default sanitizer Make target wins at gameplay frame 172327 and input
tick 174855, retaining living source Zeratul at extraction and living Tassadar,
zero faults and the completion bit. Its complete trace is byte-identical to
`e66d25a4…00421` above, including all 15,747 observations/input events. This
supersedes the earlier missing-public-opening-generator limitation. The GBA
runtime still uses the same ordinary controls; the fixture is host-test data.
Full fast ARM/save qualification remains in progress and is not counted yet.

The final 525-file source-only export passes `./dev build` and `./dev test`,
including all default host checks and 349 Python tests (six private-input
checks skipped). Re-running the public P6 Make target retains the exact full
trace hash; an explicit 1,000-frame limit stops at frame 1000 as requested.

### Default-off legacy collision fact hoist

`SC64_COLLISION_FACT_HOIST` resolves the mover's radius, flight class and
collision flags once per bucket query, and each candidate's flags once. It
only changes the expanded Z/P legacy bucket path and adds no persistent
state. `make test-sc64-z6-collision-facts` runs both builds against the
pre-change bucket predicate and an independent full-pool scan: 1,500 seeded
worlds, actor classes/zero facts, transported/dead actors and radius/cell
boundaries pass under ASan/UBSan with identical result digest `2f4f1cc9`.

A separate normal-input host replay covers every one of the 271,799 input
ticks in the qualified Z6 route. Both builds also enable the previously
checked count snapshot. Per-tick FNV-64 fingerprints cover the complete raw
unit/building/mineral banks, Z5/Z6 persistent mission states, campaign state,
economy arrays, random seed, result/frame and completion fields. The two
2,174,392-byte fingerprint streams compare byte-for-byte (SHA-256
`163cb1cd…20866`), ending at the same frame 268694 win with zero source faults.
This checks those gameplay fields, not all RAM or ARM behavior.

The ARM candidate passes the unchanged memory guards: IWRAM end
`0x03004e70`, BSS end `0x0203d980`; its ROM hash is `910ecce5…54fd8`.
A separate orders-diagnostic build also fits the guards and separates movement
from target-query cycles. The 12,000-tick timing/state/capture comparison is
queued behind P6; no ARM speedup or full-mission qualification is claimed yet.
The optimization remains disabled by default.

### Compact banks in the earlier-mission observer

The shared race observer now resolves DWARF typedef chains and scalar widths
instead of assuming every entity field is a signed 32-bit integer. It retains
the legacy banks and accepts the known compact Z/P tuple: 240 Unit56 actors
and 160 Building48 structures. Exact compact field widths and byte bounds are
validated; malformed aliases, widths and short debugger reads are rejected.
Ten asset-free tests pass, and the frozen Z6 ELF resolves the expected banks.
This enables earlier-mission ARM checks under the larger staged builds; it
does not itself promote the canonical campaign profiles.

The final 527-file source-only export passes `./dev build` and `./dev test`,
including 359 Python checks (six private-input checks skipped). No ROM,
imported header, asset, capture or private replay is included in the index.

### P6 full fast dense ARM qualification

The eight P6 query options pass the complete normal-input ARM route on ROM
`0e1f791e…da447` / ELF `3e201b40…96b0b`, with controller trace
`e66d25a4…00421`. All 174,855 input ticks and 701 observer checkpoints complete
at gameplay frame 172327. The live source Zeratul (raw 75, source owner 2,
runtime owner 0) has 60 HP at (1267,1701), inside source extraction location
15; Tassadar (raw 79, same owners) has 78 HP at (1523,1879). Source outcome is
WON, fault/cursors/action clocks are zero and completion bit 25 is set.

Opening and late saves also restore exactly in fresh HOME processes. The
opening prefix is frame 976; its saved frame 977 resumes to 1038 after the
neutral handoff and 60 live updates. The 150,000-input-tick late prefix is
frame 147765; its saved frame 147766 resumes to 147827 by the same protocol.
The complete run averages **12.75 effective fps**, measured from chained GBA
hardware timers over positive PLAY-to-PLAY intervals (171,988 live frames).
The late prefix averages 12.42 fps; these are emulator measurements.

This qualifies staged P6 and brings the total to **22/30 (T10/Z6/P6)**.
Canonical profiles remain T10/Z3/P3. The eight query options now form the P6
build defaults; dense scheduling is a separate observer option. The earlier
sections describe experiments before this qualification. No native-animation,
full-campaign 30-fps or physical-GBA claim follows from this result.

### Z6 collision timing and movement diagnosis

The 12,000-input-tick ARM comparison completes with all 13 reported states
and four decoded captures equal. With the count snapshot enabled in both
builds, collision fact hoisting improves the prefix from **9.82 to 10.47 fps**
(6.62%). This is a prefix measurement, not a complete mission or hardware gain;
the option remains disabled by default pending full qualification.

An instrumented reference build matches the same observations/captures. Its
last observed update uses 1,305,973 cycles, including 846,492 in orders:
70,972 setup, 36,426 target queries and 692,462 movement. Instrumented timing
is for diagnosis and is not substituted for the uninstrumented comparison.

### Cannon placement and optional P3 production exits

Probe-only selection exposes CANNON immediately after the existing NEXUS row.
The command uses the existing 150-mineral Photon Cannon rule, completed Forge,
Pylon power and paid Probe warp-in path. Contextual spell/transport rows keep
their selection context and the Nexus index is unchanged. Actual-engine
sanitizer fixtures pass normal menu/scroll dispatch, mixed-selection rejection,
missing Forge, unpowered-site and 149-mineral denial, payment at 150, Probe
release, timed completion, and powered ground/air defense with no fire after
power loss. Basic unified, Z6 and P6 command fixtures pass.

The source-backed P3 production-exit experiment avoids placing a newly trained
unit directly on an occupied exit. It preserves a clear preferred point, then
checks bounded outward candidates against the current GBA terrain, building,
unit and route predicates. A blocked order remains paid/queued and retries
after eight ticks; Zergling pairs plan both positions before either spawns.
Flying products ignore ground-building/terrain blockers and retain same-flight
unit checks. No persistent storage or save layout changes. The feature is
P3-only and default-off. Standalone and actual-game ASan/UBSan fixtures cover
these transitions, including atomic failure and later unblocking.

This is not native bounding-box placement equivalence: it uses the current GBA
collision radii, a 60-pixel bounded search and at most 514 candidates. Worst-case
placement cost and complete ARM playthrough remain unqualified. The old P3
controller still limits under the larger P5-bank enemy capacity, so this patch
does not close canonical Protoss promotion. Failed strategy/diagnostic drafts
remain private; the tracked controller is the committed reproducible route.

### Z7 source Extractors restore gas availability

Pinned OpenBW's Extractor death path changes it into a neutral Vespene Geyser
at the same position (`bwgame.h:16146–16158`). Z7's authenticated source map has
one explicit raw-188 node and five enemy raw-149 Extractors. The GBA death hook
now restores the latter locations when their original Extractor dies. Repeated
notifications and rebuilt human Extractor deaths do not duplicate the node;
there is no refund or resource grant. Starting source census remains unchanged.

The packed SRAM layout and reserve guards are unchanged. Cold-HOME candidate
validation requires exactly the neutral starting node plus source Extractor
positions whose original building no longer lives. It rejects duplicate,
off-source, missing and prematurely restored nodes before applying state. The
focused actual-death/SRAM fixture and existing Z7 save fixture pass, including
transactional rejection and capacity failure. A normal-input trial destroys the
nearby source Extractor, observes the restored node, builds a paid Extractor and
assigns gas workers; the trial still loses its army/economy. Full Z7 completion
remains unfinished. Gas reservoir depletion is still not implemented.

The final 532-file source-only snapshot passes `./dev build` and `./dev test`,
including 359 Python tests (six private-input checks skipped). Current unified
ARM/host checks and the canonical race build/test suite pass. The experimental
P3 controller draft was preserved privately and the committed controller
restored before the canonical checks. All 54 skirmish sanitizer matches finish
again (six wins, 48 defeats); Z7 runtime/profile/frame/save/colony/gas checks
pass. Current P6, P5 and Z7 ARM outputs and the opt-in P3-placement P5 build
pass the unchanged RAM reserve guards; the standard P5 output is restored.
These source/build/fixture checks do not establish a full Z7 win or qualify
P3 placement on ARM. The new P6 defaults reproduce the exact winning host
trace `e66d25a4…00421`.

### Earlier Z3 mission under the six-mission Zerg bank

The compact-bank observer now completes the full Z3 controller replay on the
frozen standard Z6 ROM `bc24435f…a15e8c`. All 71,186 input ticks match the host
route, ending at frame 70833 with 79/8 human units/buildings and 3/1 enemy
units/buildings. Source sequence 10/outcome WON, switch 7, objective flag,
completion bit 14 and zero action timer are verified. The run averages
**15.50 effective fps**, so the larger profile still needs performance work.
This closes the full earlier-mission ARM replay check; canonical promotion
and cold-save continuity across the earlier chapters remain separate work.

### Default-off campaign fact range rejection

`SC64_ZP_FACT_RANGE_CULL` rejects types outside the campaign enum suffix
before entering the gated actor/building fact lookup. Standard Zealot,
Dragoon and Reaver queries were overwhelmingly misses in the measured Z6
opening. The optimization adds no persistent storage or save fields.

An asset-free ASan/UBSan differential suite compares every returned fact
field and public wrapper output with the unchanged resolver, across 13
actor-gate configurations. It includes independent P5/P6 gates, P6 combat,
the late Terran enum prefix, every enum member, nearby invalid values and
`INT_MIN`/`INT_MAX`. Complete serialized outputs match in each configuration.

With count snapshots and collision fact hoisting enabled in both host
comparisons, all 271,799 input ticks produce identical actor/mission-state
fingerprints and win at frame 268694 with zero source fault. The fingerprint
file is 2,174,392 bytes with SHA-256 `163cb1cd…20866`; this checks the recorded
banks, resources, runtime and progression fields, not all RAM.

The frozen ARM candidate is ROM `00019997…2e103` / ELF `db00e37a…25074`.
It passes the unchanged guards with IWRAM end `0x03004f08` and BSS end
`0x0203d980`. Against the previous collision-hoist candidate, the same
12,000-input-tick opening improves from **10.47 to 10.97 fps (4.78%)**.
All 13 reported states and four decoded captures match exactly. These are
emulator prefix measurements; full ARM completion, fresh-process saves and
physical-GBA performance are not established by this comparison. The option
remains disabled by default pending the longer qualification run.

The combined count-snapshot/collision-hoist/range-rejection ARM build now
completes all 271,799 held-input ticks and wins at frame 268694, with source
fault zero and completion bit 17. The full route averages **19.01 effective
fps** over live PLAY intervals. The opening prefix also restores exactly in
a fresh HOME process and advances to frame 1028 after its neutral handoff and
60 live updates. The late save also restores exactly in a fresh HOME process,
then advances through one neutral handoff and 60 live updates to frame 148329.
The three options now form the Z6 query defaults; Z7 inherits the same flags.
Reference builds remain available through `Z6_QUERY_FLAGS=`. This does not
qualify 30 fps or physical GBA performance, and is separate from the
10.97-fps opening measurement above.

The 534-file source-only export passes `./dev build` and `./dev test`,
including 360 Python tests (six private-input checks skipped). The original-art
ROM remains byte-identical (`4ad80ce0…f9aee`). Imported data and private replay
artifacts remain outside the source index.

### Opt-in nearest active resource depots

`SC64_NEAREST_RESOURCE_DEPOT` replaces first-slot return-depot selection with
the nearest same-owner, grounded, reachable active depot. It follows the
pinned OpenBW active-depot/return-order rules (`bwgame.h:2191–2207` and
`4452–4490`) using the current GBA navigation grid and approach radius;
native path/footprint equivalence is not claimed. Lairs/Hives stay active
during their in-place morph. New unfinished Hatcheries remain unavailable.

The existing signed `Unit.build` field holds a per-trip depot slot or a
no-home sentinel. Topology changes invalidate these caches; depot completion
wakes no-home workers. Candidate distance/tie rejection precedes route
searches. No new persistent storage or save-format fields are added. Workers
keep their resource order and cargo when no depot can be reached; carrying
gas can be delivered even after its former refinery disappears.

Actual-engine ASan/UBSan fixtures pass in P5, Z6 and P6 builds. They cover
nearby second-base mineral/gas delivery, no credit before arrival, all three
worker types waiting without cargo loss, depot completion recovery, Lair
morphing, incomplete/enemy/non-depot rejection, deterministic ties, destroyed
refinery delivery, reused slots, disconnected/reconnected routes, packed
cache round trips and SRAM reload invalidation/reselection. Airborne rejection
is implemented in the late-Terran gate but is not exercised by this matrix.

The initial ARM experiment exceeded the unchanged IWRAM guard when GCC left
the enlarged order loop as a standalone hot function. Extracting only worker
resource handling into a non-inlined ROM helper resolves it. The frozen
candidate builds pass with these IWRAM/BSS ends: P5 plus production exits
`0x03004328` / `0x0203d584`; P6 `0x03004608` / `0x0203d774`; Z6 plus count
snapshot, collision hoisting and fact-range rejection `0x03004cd8` /
`0x0203d980`. These build checks do not measure a gameplay speedup.

The existing Z6 controller does not complete with the changed economy: its
bounded attempt ends at frame 294038 with two Force2 units still present.
During this check, the controller's old early-abort paths were found to
return exit code zero. The entry point now requires an observed source win
to pass. The reference route wins at frame 268694 with source fault zero;
non-winning attempts must fail the target. This experiment remains off by
default pending a winning route, ARM replay/save and performance qualification.

The 535-file source-only snapshot passes `./dev build` and `./dev test`
(360 Python tests, six private-input checks skipped), and current unified
ARM/host checks pass. The original-art ROM remains byte-identical
(`4ad80ce0…f9aee`). This does not advance the staged campaign count beyond
T10/Z6/P6 or promote the canonical Zerg/Protoss profiles.

### P7 scoped actor foundation

The default-off `SC64_P7_ACTORS` gate adds exact raw mappings and pinned
BWAPI facts for Archon 68, Arbiter 71, Carrier 72 and Observer 84, plus seven
Protoss structures (159/163/165/169/170/171/172). Its normalized rule rows
retain source race, cost and vitals, shields/power flags and non-product
sentinels; zero-initialized Terran products cannot leak into skirmish menus.
P7-scoped mappings take precedence over overlapping optional structure
aliases. Existing profiles keep their enum layout when the gate is off.

Asset-free sanitizer checks exercise six gate combinations, raw round trips,
normalized rules, source attributes and capability gaps. The actual staged
Z/P prefix plus P7 has 49 unit and 60 building types, within both current
six-bit packed enum fields. This verifies enum capacity only; it does not
validate a P7 save payload. The fact-range differential matrix also passes
with P7 and shifted prefixes (16 configurations).

Source metadata retains Carrier's special native can-attack flag and absent
weapon, while the GBA policy does not claim interceptor combat. Required,
unimplemented capabilities explicitly include Archon radial splash,
Carrier interceptors, Observer detection/permanent cloak, Arbiter fields and
Shield Battery recharge. P7 map/sprite binding, campaign/save wiring, opaque
AI behavior and a normal-input win remain required before promotion.

The 538-file source-only export passes `./dev build` and `./dev test`,
including 361 Python tests (six private-input checks skipped). The default
original-art ROM remains byte-identical (`4ad80ce0…f9aee`). These checks do
not add P7 to the qualified mission count.

### Larger-bank P3 expansion controller

The opt-in P5 Shuttle build with `SC64_P3_PRODUCTION_EXIT` and
`SC64_NEAREST_RESOURCE_DEPOT` now has a reproducible ASan/UBSan controller
win for Higher Ground. The public controller uses only normal button inputs
to mutate gameplay, builds paid Nexus expansions with powered Cannon
defenses, and orders the full army through both enemy bases. It does not
replace the existing canonical P3 controller.

The Make target completes 124,689 held-input ticks and wins at frame 123916.
It verifies source sequence 5, fired mask 31, outcome WON, no action timer,
primary objective/completion state, zero unsupported actors and no surviving
enemy buildings. The final roster is 67 player units and 35 buildings,
including 45 Probes and 11 Cannons. The trace SHA-256 is `321074f1…db01a`.
A bounded non-winning attempt returns exit status one.

The additive ARM replay route keeps the original garrison route as its
default and pins the expansion launch/final witness independently. Its
5,932 input transitions fit the larger build's 256-entry windowed scheduler
with a peak of 121 transitions per 1,000-tick page. ABI fixtures cover signed
selection/source-owner fields, transported units and both compact-byte and
legacy-int queue rings. This is host/controller and observer validation;
the complete larger-bank ARM replay now wins at frame 123916, preserving
the source objective/model witnesses with zero unsupported actors. Its
124,689-input run averages **12.35 effective fps**, below the performance
target. Opening and late-save continuity pass as described below;
canonical promotion remains pending.
The staged total remains T10/Z6/P6, and canonical profiles remain T10/Z3/P3.

The frozen larger-bank candidate (ROM `d26ca305…f4d1`, ELF
`9d80a53a…9e57`) also passes a 1,000-input-tick opening save and fresh HOME
load. The recorded persisted observations match exactly, with the intentional
building-selection reset reported separately: saved selection 5 becomes -1.
This is not labelled an exact whole snapshot. The handoff takes one neutral
live frame and then 60 scheduled live frames. The 90,000-input late prefix
also saves at frame 89287 and restores the reported persisted observations
exactly through fresh HOME+L. Its selected building resets from 61 to -1;
after one neutral handoff and 60 updates it reaches frame 89348. Both UI
resets are explicit, without claiming whole-RAM equality or a full
post-load mission completion. The full win, opening restore and late restore
now pass for this frozen expansion build.

### Gated P7 sprite preparation

The optional `--include-p7-actors` importer adds Arbiter raw 71 and all source
frames for Protoss structures 159/163/165/169/170/171/172. Shared raw
68/72/84 images are reused. The P7 completeness marker requires both the new
payload and those shared images. P7 building lookups take precedence over
overlapping late-Terran/Z6 aliases when the P7 payload is present; the
existing generated output stays unchanged when P7 inclusion is off.

Fourteen synthetic sprite-import tests pass, including compiled flag-off,
P7-only and combined P7/late-Terran/Z6 headers, exact synthetic pixel mappings,
indexed building frames, missing shared art and alias precedence. No game
assets are part of these fixtures. This prepares source images; it does not
establish P7 runtime binding, native animation selection or playable progress.

The combined 541-file source-only snapshot passes `./dev build` and
`./dev test` (379 Python tests, six private-input checks skipped). The
original-art ROM remains byte-identical (`4ad80ce0…f9aee`), and the existing
unified ARM build and host sanitizer checks pass. Imported assets and private
qualification artifacts remain outside the source index.

### Default-off collision-class cache

`SC64_COLLISION_CLASS_CACHE` caches only immutable radius, flying and
collidable facts in one transient byte per unit. Legacy bucket phases clear
the cache; spawning invalidates a reused slot. Alive, loaded and burrowed
state remain live checks. Invalid/unrepresentable packed values use the
uncached resolver. The option rejects late-Terran persistent bucket storage
and adds no save fields.

ASan/UBSan full-scan oracle fixtures pass for the reference, hoisted,
cache-only and combined variants, including 1,500 randomized worlds per
variant, actor classes, invalid types, invalidation, slot reuse and encoding
boundaries. The complete winning Z6 host route also preserves the recorded
2,174,392-byte actor/mission fingerprint stream (`163cb1cd…20866`).

With the three qualified query options enabled, the frozen cache candidate
(ROM `f1bc31a2…bfade`, ELF `55e6399d…19a0b`) retains IWRAM end
`0x03004fb8` and BSS end `0x0203da70`, within the unchanged reserves. Its
12,000-input ARM opening improves from **10.97 to 11.29 fps (2.92%)**;
all 13 reported states and four decoded captures match the no-cache build.
This is prefix evidence, not a full ARM mission/save or hardware
qualification. The cache remains disabled by default.

The current default Z6 ARM ROM is byte-identical to the fully qualified
`00019997…2e103` image. Runtime, save, startup/skirmish, framebuffer coverage,
count-query and all four collision variants pass under host sanitizers.
The 541-file source-only snapshot also passes `./dev build` and `./dev test`
(379 Python tests, six private-input skips), retaining the original-art ROM
hash `4ad80ce0…f9aee`.

### Z7 Spore morph command

The internal Z7 colony menu now retains Sunken and adds Spore after a
completed Evolution Chamber. A completed owned Creep Colony pays 50 minerals
once, morphs in place for 375 GBA updates and consumes no additional Drone.
It retains Creep identity and HP during the morph, then becomes source raw
144 with 400 max HP. The existing air-only defense policy supplies its
15-damage weapon; center-based targeting, projectile timing, detector
behavior and the reduced building sight remain fidelity limits.

Sanitizer fixtures cover payment denial at 49 minerals, completed tech,
menu reachability/dispatch, retained source identity/position/HP, preserved
Drone count, mid-morph save/load, air-only attacks, death cleanup and reused
slots. Transactional source-save checks permit only the authenticated
source-owned Creep-to-Sunken/Spore identities. Save generation 35 rejects
generation 34 so its new reserved morph flag cannot be misinterpreted.

The Z7 ARM build retains IWRAM end `0x03004f08` and BSS end `0x0203d9f0`;
the fixed reserve guards are unchanged. Startup/skirmish, game-save,
gas-restore and framebuffer checks pass. Private ordinary-input trials show
the morph clock advancing under real gameplay; one Colony dies at progress
321/375 under enemy fire. This is not a complete Z7 win, ARM replay/save
qualification or campaign-count promotion.

A separate fixture makes its test Colony invulnerable and advances 375
ordinary `game_update(0,0)` calls through completion, preserving HP and the
Drone count with no source fault. The final 541-file source-only snapshot
passes `./dev build` and `./dev test` (379 Python tests, six private-input
skips); unified ARM/host checks pass and original-art ROM bytes are unchanged.

### P7 source-coordinate geometry

The isolated P7 world now applies source collision extents after doubling the
runtime center. The previous path halved the source extents a second time,
which could miss location overlaps. A shared, asset-free helper returns
half-open source rectangles and rejects unmapped types, missing output
pointers, negative centers and coordinates that would overflow conversion.

Sanitizer fixtures check exact source edges, the complete 180-unit and
112-building placement census, unit/building location-boundary transitions,
and source-owner preservation for the two scripted Carrier creations. Both
actor-gate configurations pass. These checks do not establish a P7 campaign
entry, complete abilities, save integration or a winning playthrough.

The 543-file source-only snapshot passes `./dev build` and `./dev test`
(379 Python tests, six private-input skips). The original-art ROM remains
`4ad80ce0…f9aee`; unified ARM/host checks also pass.

### Experimental P7 Archon splash

The default-off `SC64_P7_ARCHON_SPLASH` path handles the complete Archon
attack before generic center-distance combat. It measures the 64-source-pixel
reach between collision rectangles and uses the source integer distance rule
for inclusive 3/15/30-pixel splash rings. Its packet is 30 damage plus three
per weapon upgrade, divided by one/two/four before HP armor. The cooldown
counter is 25 GBA updates. Enemy Splash excludes the attacker and same-owner
neighbors, retaining the explicit primary exception and distinct-owner allied
collateral.

Known source-owner IDs distinguish P7's players 0 and 4 even though both
map to the live opponent slot. A sanitizer regression routes an enemy
Archon through `tick_combat`, preserving same-source exclusion while hitting
distinct-source unit and building collateral. Its 30-damage base packet
also verifies that an enemy Archon does not receive the player's upgrade.
Unknown source provenance falls back to live ownership; P7 production
still needs to populate source owners before this distinction is complete.

Pure and actual-engine sanitizer fixtures cover axis and diagonal boundaries,
approach behavior, ground/building hits, upgrade damage, Normal damage,
shield overflow, loaded targets and invincibility flags/arrays. The latter
must reject damage before touching shields. Fractional Q8 damage follows the
existing integer HP/shield adaptation, with a nonzero fractional HP hit
represented as one point. Shield armor remains zero in this compact model.

The path adds no persistent fields. A private P6 engine build with both P7
actor and splash gates links on ARM with IWRAM end `0x030047e0` and BSS end
`0x0203d774`, below the unchanged reserves. This is a link check, without
P7 gameplay or emulator qualification. Immediate target-center impact,
projectile/windup timing and native burrow handling remain limitations; the
feature does not promote P7 or clear its complete-mechanics requirement.

The 547-file source-only snapshot passes the default build and test suite
(379 Python tests, six private-input skips). Its original-art ROM remains
`4ad80ce0…f9aee`, and unified ARM/host checks pass with the gate off.

The source-owner follow-up also passes its focused engine fixture and the
380-test source-only suite (six private-input skips), retaining these ARM
reserves and the original-art ROM hash.

### Packed route visited-grid experiment

The default-off `SC64_ROUTE_VISIT_GRID` gate borrows the packed walkability
grid during a cache-miss reverse flood. Enqueuing a cell clears its bit;
after the flood, the existing BFS queue restores every cleared bit before
the function returns. This replaces packed route-code reads for visited
checks without adding global storage or changing cache replacement.

The asset-free sanitizer fixture compares every field code against an
independent BFS and snapshots the complete packed walkability grid around
each query. It covers randomized 128-by-128 maps, blocked starts/goals,
empty/all-open floods, resets and eviction. The complete winning P3 host
route preserves both the trace (`321074f1…db01a`) and explicit-field
fingerprint stream (`3d766e63…51732`) exactly.

The gated P3 ARM link has IWRAM end `0x030042f8` and unchanged BSS end
`0x0203d584`. This establishes link/reserve checks only; GBA timing, full
ARM replay and fresh-save qualification remain pending. No profile default
enables the experiment. The 547-file source-only snapshot passes the default
build and suite (380 Python tests, six private-input skips); original-art
ROM bytes remain `4ad80ce0…f9aee`, and unified ARM/host checks pass.

### Experimental partial floods and frequency replacement

Two additional default-off route gates require `SC64_RACES` and
`SC64_ROUTE_PACK3`. `SC64_ROUTE_PARTIAL_FLOOD` stops a deterministic reverse
flood once the queried origin has its exact gradient. A blocked origin waits
for all walkable cardinal exits. Partial fields are extended by rebuilding
the same BFS prefix; the one-argument full-field API still completes a field.
The existing validity byte distinguishes partial and complete fields, so no
global storage is added. Borrowed visitation bits are restored for all
enqueued cells, including an early-stop frontier.

`SC64_ROUTE_DECAY_FREQ` packs a six-bit access frequency and 26-bit recency
stamp into each existing age word. It decays frequencies before every 128th
request, saturates at 63, and selects invalid slots before the least-frequent,
oldest entry. Extending a partial field preserves its frequency. Reset and
timestamp rollover invalidate derived cache metadata before starting a new
epoch. Both policies can be selected independently of the visitation gate;
the standard ROM profiles leave them off.

An asset-free sanitizer matrix exercises all eight combinations of the three
route gates on a packed 128-by-128 grid. Returned steps and complete fields
are compared with an independent BFS, including reachable blocked origins,
disconnected exits and blocked goals. Every query and full-field wrapper
checks packed-grid restoration. Focused replacement checks cover invalid
entries, frequency/age ties, decay-before-hit, saturation, extension, reset
and timestamp rollover.

The private P3 host run with VISIT, PARTIAL and DECAY_FREQ preserves the entire
winning input trace (`321074f1…db01a`) and per-update explicit-field fingerprint
stream (`3d766e63…51732`). Across the same 123 periodic sampling windows,
visited cells fall from 137,457,153 to 67,015,769 (51.2%). This run did not
enable the separate collision hoist, fact-range or class-cache gates.

Its ARM ROM is `9416f9cb…57bfc` and ELF `e9ad7a60…7bd95`, with IWRAM end
`0x03004608` and unchanged BSS end `0x0203d584`. Rebuilding the current public
source with the same compiler flags produces identical ROM bytes. A 12,000-input normal-menu
opening matches all 13 reported baseline snapshots and the three common
decoded captures. Its mean rises from 17.6234 to 17.83 effective updates per
second. Fresh HOME+L loading of that candidate's own opening SRAM restores
all reported persisted observations; one handoff plus 60 neutral updates
reaches frame 11,824.

A separate paired late-state benchmark loads the authenticated baseline's
90,000-input SRAM into fresh processes for both ROMs. After one handoff and
60 identical neutral updates, reported loaded/tick-zero/final states and all
three decoded captures match. Hardware-cycle time falls 11.9938%, from
13.8936 to 15.7870 effective updates per second. This cross-build seed is a
benchmark, not candidate-generated late-save qualification. Full ARM mission
and wider profile qualification remain required before enabling defaults.

The full ARM attempt reached a logged checkpoint at 114,000 of 124,689
inputs (live frame 113,232) before a checkpoint-history write failed with
`ENOSPC`. The runner exited nonzero and closed mGBA. This partial attempt is
not a qualified win; its log is retained separately for a retry with adequate
disk headroom.

### Experimental P7 Observer cloak and detection

`SC64_P7_OBSERVER_ENABLED` is default-off and requires the isolated P7 runtime
and actor facts. It derives permanent cloak from Observer raw type 84,
threads acting source-player identity through acquisition, direct combat and
building defenses, and hides undetected enemy Observers in world/minimap
rendering. Known source slots 0 and 4 retain separate optics and detector
sets despite sharing the live opponent slot. Only an unknown querying source
uses the existing live-owner fallback. New human products record source slot
7; enemy producer provenance remains unfinished.

Observers detect within their initial nine-tile sight; grounded Photon
Cannons use seven tiles and require completion plus the runtime's same-owner
Pylon power check. Source rectangle edges and the pinned integer distance
metric determine range. Loaded Observers cannot detect. Non-human optical
visibility is computed on demand using the current GBA cell-center circles;
human source slot 7 reuses the existing visibility grid. No extra visibility
grids or persistent save fields are added. Alive buildings provide optical
sight during construction, matching that grid; an unfinished Cannon still
cannot detect. Native construction-sight differences remain unmodeled.

Pure policy and actual-engine sanitizer fixtures cover owner-specific target
selection and damage, loaded detector exclusion, source identity, Cannon
power ownership, axial/diagonal distance boundaries and primary rejection
without detection. The combined Archon fixture confirms an undetected
Observer can still take radial collateral from a visible primary target.
Native terrain occlusion, all disabled/blinded/hallucination states, Sensor
Array research, source power-mask geometry and complete P7 gameplay/save
wiring remain outside this bounded implementation. This does not promote P7
or change the qualified campaign count.

The combined Observer/Archon ARM link retains IWRAM end `0x030047d8` and BSS
end `0x0203d774`, within the unchanged reserves. The P7 runtime's map-ID check
also no longer requires a hosted C string library. This is a link check,
without P7 ARM gameplay qualification.

The 552-file source-only snapshot passes `./dev build` and `./dev test`
(380 Python tests, six private-input skips), including the eight asset-free
route-policy variants and pure Observer rules. Local unified build/host
checks and the private-input P7 runtime, Observer and Archon fixtures pass.
The original-art and unified ROM hashes remain `4ad80ce0…f9aee` and
`6bab5be2…79f9` with these gates off.

### Experimental construction hit-point progression

The default-off `SC64_CONSTRUCTION_HP` gate requires race rules. Ordinary
foundations start at one tenth of their maximum HP (at least one point),
rather than staying at one HP until completion. Each construction update
adds the difference between adjacent cumulative integer progress targets to
the current HP. Damage therefore survives completion. Terran progress and
growth require a working SCV; Zerg and Protoss growth wait for builder arrival.
Protoss shields start at one tenth of their source maximum and receive
construction gains alongside the existing passive shield recharge.

This follows the pinned OpenBW construction lifecycle, with a documented
integer adaptation: the source uses rounded 8.8 construction rates, while
this path uses cumulative integer floors over the existing scaled 30 Hz
duration. Lair, Hive and Greater Spire morphs skip ordinary growth and apply
the absolute new-maximum minus old-maximum HP difference at completion.
Z7's existing Creep-to-Sunken/Spore hooks retain their separate damage-preserving
completion logic. Their transient completion validator accepts the saved
pre-tick HP under this gate instead of requiring the old generic full heal.

The existing pre-arrival Zerg/Protoss foundation remains a timing approximation:
its progress is negative until arrival, so construction HP cannot grow, but
the existing shield-recharge path can still run. Incomplete Zerg buildings
receive no passive HP regeneration, consistent with the reference's
completed-only HP regeneration check. No unit/building fields, save bytes or
global storage are added. The gate remains off in standard profiles pending
mission and fresh-process SRAM qualification.

Asset-free ASan/UBSan tests cover integer boundaries, completion, damage,
shield gains and morph clamps. Actual unified and Z7 fixtures cover SCV
pause/resume, Drone consumption, delayed growth, Protoss HP/shield growth and
Lair morph damage. The existing Z7 colony fixture also passes with the gate
enabled, including damaged Sunken/Spore morphs and packed-save continuity.
The gated Z7 ARM ROM (`4c97cd91…92596`) links with IWRAM end `0x03004f28`
and unchanged BSS end `0x0203d9f0`, below the unchanged reserves. This is
link and focused host evidence, without a qualified Z7 mission win.

A 555-file source-only export passes the default build and complete host suite
(380 Python tests, six private-input skips). Local unified build/host checks
and both gate-on/off Z7 colony fixtures also pass. With the gate off,
original-art and unified ROM bytes remain `4ad80ce0…f9aee` and
`6bab5be2…79f9` respectively.

### Combined route and collision ARM qualification

The three experimental route policies were also tested together with
`SC64_COLLISION_FACT_HOIST`, `SC64_ZP_FACT_RANGE_CULL` and
`SC64_COLLISION_CLASS_CACHE` in the P5 Shuttle bank's P3 expansion route.
The complete sanitizer host run preserves the frozen winning input trace
(`321074f1…db01a`) and per-update explicit-field fingerprints
(`3d766e63…51732`). It is separate from the earlier route-only run.

The combined ARM ROM (`bed9feb7…9bc2f`, ELF `86dd57c3…7340bf`) wins after
all 124,689 controller inputs at live frame 123,916. All 126 reported
checkpoints match the frozen baseline after excluding timing, route-cache
counters and capture metadata. The final source outcome is victory, with
no enemy buildings and no unsupported actors. Mean live-play performance
rises from 12.35 to 13.81 effective updates per second, about 11.8% faster.
This is progress toward the target, without reaching near 30 fps.

Its 12,000-input opening also matches all 13 reported snapshots and the three
common captures, improving 17.6234 to 17.99 fps. A fresh HOME+L process
restores that candidate's own opening SRAM with all reported persisted
observations exact. Separately, the paired baseline-SRAM late-state benchmark
improves 13.8936 to 15.9985 fps (13.1569% fewer hardware cycles), with the
reported loaded/tick-zero/final states and three captures matching. The
cross-build benchmark alone is not candidate-generated late-save qualification.
A subsequent 90,000-input run matches all 91 reported baseline snapshots and
the three common captures, then creates its own SRAM. A fresh HOME+L process
restores all reported persisted observations; the pause-screen and selected
building resets are explicit. One neutral handoff plus 60 updates reaches
frame 89,348 at 16.00 effective fps. Its SRAM bytes also match the frozen
baseline's late save. This verifies late loading and continuation, without a
complete post-load mission win.

The combined link retains IWRAM end `0x03004598` and BSS end `0x0203d674`.
Rebuilding current public source with construction HP disabled produces the
same combined ROM bytes. Profile defaults remain unchanged pending wider
campaign and save qualification. This does not add another qualified mission
or change the staged count of 22/30.

### Experimental terrain-key reuse

The default-off `SC64_RENDER_KEY_ONCE` gate requires an imported profile.
It computes the exact static-art key once after camera placement, then passes
it to terrain-cache lookup and capture. Rendering between those operations
only reads the keyed world/visibility/prop inputs. T5 retains its existing
keyed path; other profiles can select the same API without enabling T5
gameplay. No global storage, cache fields or save bytes are added.

Asset-free cache and real-framebuffer scroll fixtures pass under ASan/UBSan.
P5 Shuttle, P6 and Z6 actual-scene pixel oracles also pass, including cold/warm
cache, camera/fog changes, resource/building mutations, construction waiting,
progress/completion/removal, restart and load transitions. Local default and
unified build/host checks pass with the flag off. The combined P3 candidate
links as ROM `2020678d…0bc81`, ELF `8cab7c0d…3f3d1`, with unchanged IWRAM end
`0x03004598` and BSS end `0x0203d674`. Its gate-off rebuild reproduces the
qualified six-gate ROM exactly.

A paired ARM sample loads the same authenticated baseline SRAM into fresh
processes, then runs one neutral handoff and 60 identical left-held updates.
It verifies actual camera movement from x=1544 to x=1424, matching reported
loaded/tick-zero/final states, cursor/camera values and all three captures.
Both builds average 14.1089 effective fps: this sample shows no measurable
speed improvement. The gate remains off; broader timing and full replay are
unqualified. An earlier right-held sample also showed only 0.0108% fewer
cycles and is retained separately, without using it as an improvement claim.

The 555-file source-only export passes the default build and 380 Python
tests (six private-input skips). Original-art and unified ROM hashes remain
`4ad80ce0…f9aee` and `6bab5be2…79f9` with the option off. An invalid
non-imported game build is rejected at compile time.


### Experimental direct attack orders

The default-off `SC64_DIRECT_ATTACK` path distinguishes an ATTACK click on a
visible hostile actor from a ground Attack-Move. Capable selected human units
retain the clicked combined unit/building target through approach and cooldown,
and deal damage to that target even when another hostile is closer. The
existing source-aware weapon/detection predicate still determines whether each
shooter can attack. The cursor uses the existing coarse GBA selection radii
(31 pixels for units, 26 for buildings), rather than native sprite hitboxes.

While a target is fogged, the unit approaches only the last visible destination
stored when it could see that target. It does not follow hidden current
coordinates or damage the hidden actor. It drops the order within six pixels
of that destination. A visible actor that becomes weapon/detection-ineligible,
friendly, loaded, hidden or invulnerable is dropped; dead targets and references
to reused actor slots are also cleared. This is a bounded direct-order policy:
native order queues, target reacquisition and all native priority rules are not
implemented here. Nonhuman order dispatch remains unchanged.

The order uses the existing packed order/target fields without adding actor
fields, global storage or save bytes. Actual-engine ASan/UBSan fixtures pass
under plain race, unified and Z7-plus-construction-HP flags. They check direct
farther-target damage with a nearer hostile untouched, cooldown and moving
target identity, other hostile skirmish owners, weaponless actor rejection,
fogged coordinate isolation, unit/building death and slot reuse, invulnerability,
and supported hidden/transported actor guards. Last-seen arrival expiry,
unit/building invincibility arrays and ground-only versus flying targets also
pass. The Z7 compact unit row retains
the direct order and combined target through encode/decode. These focused host
checks do not qualify a full mission, physical GBA performance or a fresh-process
save/load. The gate remains off in standard profiles.

The gated Z7-plus-construction-HP ARM ROM (`bcecf4b5…28254`) links with
IWRAM end `0x03004f58` and BSS end `0x0203d9f0`, below the unchanged
reserves. The current controller can issue the direct order through the normal
menu and click path, including a four-Mutalisk attack on a visible hostile
Mutalisk. Its diagnostic Z7 run still loses at frame 18,612; this is evidence
of UI/order integration, not a qualified mission win. P7 Observer cloak/detector
integration is outside these fixture configurations and remains unqualified.


### Experimental packed route-field initialization

The default-off `SC64_ROUTE_WORD_CLEAR` gate retains PACK3 route contents and
replacement policy while changing only all-unreachable field initialization.
An explicitly aligned array and `may_alias` 32-bit stores avoid unaligned word
access and strict-aliasing violations. Byte prefix/tail handling supports row
sizes that are not multiples of four; no state, actor fields or save bytes are
added. The P5 campaign bank used for the P3 measurement allocates each field
for its 128×128 maximum, even while the active P3 map is 128×96. Each clear
therefore needs 1,536 word stores instead of 6,144 byte stores. A standalone
128×96 field uses 4,608 bytes; it is a separate fixture size. These operation
counts are not themselves FPS results.

ASan/UBSan fixtures compare every decoded field with an independent BFS across
128×128, 128×96 and 10×10 grids. Canary checks exercise start offsets zero
through three, including an unaligned real slot in the 38-byte-row case.
The complete P3 expansion controller also wins at frame 123,916 under host
sanitizers. Its trace (`321074f1…db01a`) and explicit per-update gameplay
fingerprints (`3d766e63…51732`) match the six-gate route/collision baseline.
These fingerprints cover actor, order, economy and named campaign fields;
they are not a whole-RAM comparison.

The gated ARM ROM (`7ba4743d…fa9270`) links with IWRAM end `0x030045a8`
and BSS end `0x0203d674`, below unchanged reserves. In a paired 600-update
late-P3 benchmark, both builds load the same authenticated baseline SRAM,
advance one handoff update and then 600 neutral updates to frame 89,888.
Loaded, tick-zero and final gameplay snapshots, three screenshots and route
counts match (74 builds and 2,159 hits). Effective FPS changes from 16.5374
to 16.6546, or 0.7039% fewer hardware cycles. This is a modest result for that
interval, not a full-mission average, physical GBA measurement or a candidate's
own fresh-process save qualification. The gate remains off pending wider
qualification. With the gate off, the six-gate ARM ROM is byte-identical to
`bed9feb7…9bc2f`.


### Experimental independent Z7 Egg production

The default-off `SC64_Z7_EGG_PRODUCTION` path creates a real raw-36 Egg in an
existing Unit slot for each paid larva product in Z7. Product identity and
remaining time use the existing target/timer fields. The Egg is stationary,
ground-targetable, has 200 HP, 10 armor, source collision bounds and four-tile
sight, and cannot attack. The pinned BWAPI facts and OpenBW morph, cancellation
and paired-birth lifecycle guide this path. Ordinary acquisition prefers an
eligible non-Egg target before an Egg, with nearest-target selection within
those categories; this is a bounded priority adaptation, not the full native
priority scoring system.

Jobs advance independently; three is the live-larva cap rather than an active
Egg cap. Regeneration can therefore produce more than three concurrent Eggs
from one Hatchery. Eggs do not depend on a producer link, so losing or reusing
its building slot cannot cancel or transfer a job. At hatch, the same actor
slot changes to the product and retains its integer HP fraction. Zergling and
Scourge pairs attempt the second actor once; pool exhaustion completes only
the first and releases the second output's reserved supply. Product costs,
source durations and the existing 30-update conversion are preserved. No
actor fields, persistent arrays or save bytes are added.

The selected-unit CANCEL EGG command refunds the full product cost and removes
the Egg; it does not turn it back into a Drone. STOP and ordinary movement
commands cannot erase its product. Contextual supply accounts for both pending
outputs through admission, hatch, cancellation, damage/death and save/load.
The gate uses save generation 36 and rejects older serial-queue records.
Candidate validation is read-only and checks Egg provenance, product, timer,
canonical combat fields, outcome participation, and enemy Ling obligations
against the unchanged authored AI pulse bound. Legitimate over-cap armies and
a paired Egg with no extra free actor slot remain valid.

Actual-engine ASan/UBSan fixtures cover three concurrent timers, five live Eggs
following larva regeneration, producer death and slot reuse, damaged hatch HP,
full-refund command dispatch, STOP preservation, both paired products and the
one-output fallback, source traits and ordinary/manual targeting. The save
fixture checks mid-Egg round trips, candidate byte immutability, valid over-cap
and full-pool states, enemy provenance and paid-product bounds, and transactional
rejection of forged products, timers, armor/shields and legacy larva queues.

This remains a staged runtime adaptation. Larvae are still producer counters
rather than wandering actors; Egg placement, hatch offsets and immediate timer
completion do not reproduce native Larva motion or IScript birth timing.
Existing product prerequisites and unsupported abilities retain their documented
limits. The gate is off in standard profiles pending full Z7 mission and
fresh-process SRAM qualification.


The Egg ARM build (`c8c5945f…d27ab9`) links with IWRAM end `0x03004fe0`
and BSS end `0x0203d9f0`. The hatch-processing body stays in ROM to preserve
the unchanged fast-RAM/stack reserve. With the Egg gate off, the prior
Z7 direct-attack-plus-construction-HP ROM is byte-identical to
`bcecf4b5…28254`. These are focused host and ARM link checks; a complete Z7
mission and a fresh-process mid-Egg SRAM continuation remain unqualified.

### Experimental Z7 source sight

The default-off `SC64_Z7_SOURCE_SIGHT` gate supplies pinned base sight ranges
for supported Zerg types only during Z7. Mutalisks and Drones use seven source
tiles, Hydralisks six, Overlords nine and Guardians eleven; each source tile
is 32 pixels and the GBA world is half scale. All 36 raw-type table entries
were independently compared with the pinned BWAPI sight-range table.

An ordinary unfinished grounded building uses four source tiles. Lair, Hive
and Greater Spire upgrades retain the predecessor's range until completion;
colony morphs retain Creep Colony sight. A derived key in the existing building
visibility snapshot makes completion/morph transitions at fixed coordinates
remove the old footprint and add the new one. No snapshot fields, persistent
arrays or save bytes are added.

ASan/UBSan fixtures pass in both the ordinary Z7 and Egg-enabled configurations.
They compare incremental source counts and fog bits against a fresh scan for
movement, loading/unloading, removal, map-edge clipping, construction completion
and a real Hatchery-to-Lair morph. Inactive profiles preserve their existing
range policy. The combined Egg/source-sight ARM ROM (`f8abb6c8…3cdadf`) links
at IWRAM `0x03004fe8` and unchanged BSS `0x0203d9f0` under the fixed reserves.

This uses the existing circle/cell-center visibility rasterizer; it does not
implement native terrain-height occlusion, blindness or sight upgrades.
The pre-arrival foundation still follows the existing placement approximation.
These tests do not qualify a full Z7 win, fresh-process SRAM or physical GBA
performance, and standard profiles leave this gate off.

The same normal-input Z7 controller was run with independent Eggs alone and
with Eggs plus source sight, retaining identical costs, timers and tactics.
Both lose: frame 15,988 (16,469 host updates) and frame 16,874 (17,406 host
updates), respectively. The traces verify paid Egg admissions, but workers,
army and the last Hatchery are lost before the objective is cleared. The
controller's delayed interception of northern Guardians remains under study;
the longer survival is not a mission qualification or performance measurement.

The final Egg/source-sight source snapshot passes `./dev build`, `./dev test`
(380 Python tests plus the default host fixtures), `make unified` and
`make test-unified`. With the experimental gates off, original-art and unified
ROMs remain byte-identical to `4ad80ce0…f9aee` and `6bab5be2…979f9`.
The separate focused Egg core/save and source-sight sanitizer suites pass,
including source sight with and without Egg production. A read-only source
review found no additional lifecycle, supply, save or visibility defects;
that review does not replace runtime campaign and SRAM qualification.


### PACK3 word-clear full ARM follow-up

The word-clear candidate also completes the full 124,689-tick P3 expansion
replay, using 5,932 input transitions. It wins at frame 123,916 and matches all
126 reported gameplay checkpoints against the frozen baseline. The mean over
live-play intervals is 13.88 effective fps, compared with 13.81 for the previous
six-gate candidate and 12.35 for the original baseline. This verifies a modest
full-mission gain, without closing the near-30-fps goal. Candidate-generated
SRAM and wider-profile qualification remain separate from this full win.

The same word-clear ROM separately passes a 12,000-input opening replay with
all 13 reported gameplay checkpoints and three captures matching the frozen
baseline. Its measured opening mean is 18.01 effective fps versus 17.6234.
It then resumes its own generated SRAM in a new emulator process through the
ordinary HOME / load path, with exact persisted observations at frame 11,824.
The 90,000-input late replay separately matches all 91 reported gameplay
checkpoints and the same three captures. A new emulator process loads that
candidate-generated SRAM through HOME / load with exact persisted observations
and resumes through frame 89,348. Wider-profile qualification and a full
post-load mission win remain separate.

### Experimental Z7 Scourge combat and Spire geometry

`SC64_Z7_SCOURGE_COMBAT` enables the existing Suicide Scourge combat path in the
Z7 experimental build, requiring independent Egg production. The new Zerg
production-cycle entry requires a completed Spire and buys a pair for the
pinned 25 minerals / 75 gas and 450 source-frame build time (563 updates).
The source weapon has 110 normal damage, zero upgrade bonus, air-only targets,
three source-pixel reach and a one-frame cooldown. The current half-scale
engine rounds reach to two world pixels and cooldown to two updates. A valid
hit goes through the ordinary shield/armor/death path before consuming the
attacker and releasing its supply. Common loaded, cloaked and hidden-actor
guards remain in the target path.

An actual-engine ASan/UBSan fixture launches Z7 through its normal menu, cycles
the selected Hatchery with real press/release R inputs, buys the Egg with A,
and saves/loads the pending product and selected training type. Isolated
production advances the complete 563-update timer and verifies both births.
Combat cases cover ground/allied/loaded/hidden/invincible and invalid targets,
range and cooldown, fixed damage with weapon upgrades, shields/armor, attacker
consumption, supply and prevention of repeat damage. The existing P3 Scourge
fixture also passes. A separate inactive-mode check keeps Scourge out of the
skirmish product cycle and rejects a serial single-unit purchase. These
fixtures use controlled setup and are not full
campaign controller wins or fresh-process SRAM tests.

That real-menu fixture exposed a separate Z7 condition bug: the building
rectangle fallback omitted ordinary Spire raw type 141. A Bring condition
could therefore fail with condition fault 2 and pause as soon as a Spire was
present. Raw 141 now uses its pinned left/up/right/down bounds 28/32/28/24,
matching Greater Spire raw 137; the fixture verifies the exact half-open
rectangle and that the live mission continues with the Spire. This correction
is independent of the optional combat gate, so the Z7 gate-off ROM changes
intentionally to `6f5822f8…3f1459`.

The combined Egg/source-sight/Scourge ARM ROM (`5a183717…76dc60`) links at IWRAM
`0x03004f78` and BSS `0x0203d9f0` under the unchanged reserves. Initial links
exceeded the fast-code limit; shared target guards and the dual weapon lookup
now stay in ROM. No actor fields, persistent arrays or save bytes are added;
the gate retains internal save generation 36. Native projectile travel and
IScript death timing, a complete Z7 win, cold SRAM and hardware performance
remain unqualified. Mutalisk Glaive Wurm bounces are still unimplemented.

The final source snapshot again passes `./dev build`, `./dev test` (380 Python
tests plus the default host fixtures), `make unified` and `make test-unified`.
The standard original-art/unified ROM hashes remain `4ad80ce0…f9aee` and
`6bab5be2…979f9`; their profiles leave the experimental combat gate off.


### Experimental clear-corridor routing

The default-off `SC64_ROUTE_L_FAST_PATH` gate requires race rules and PACK3.
The existing reverse BFS expands left, right, up, then down. Its preferred
Manhattan-shortest path from the goal therefore completes horizontal moves
before vertical moves. When that entire corridor and both endpoints are
walkable, reversing the path returns the same first step even when obstacles
exist elsewhere. A blocked corridor or endpoint falls through to the ordinary
cache and blocked-endpoint rules. The shortcut changes derived cache traffic,
not route tie order, and adds no persistent, scratch or save storage.

`make test-sc64-route-l-fast` passes ASan/UBSan configurations for 64×64,
128×96 and 128×128 maps, plus 128×96 with partial flooding and frequency
decay. The independent reference BFS checks exact first steps, full fallback
fields, blocked origins and goals, obstacles outside clear corridors and
cache capacity/eviction/reset. Fast-path queries correctly avoid cache
allocation; cache-accounting cases explicitly request a field as well. The
existing default-off assertions remain intact. A separate private exhaustive
3×3/random larger-grid oracle checks 184,483 applicable clear-corridor cases.

The public-source sanitized P3 controller wins at frame 123,916 with its
complete input trace and explicit per-update gameplay fingerprints exactly
matching the frozen baseline (`321074f1…db01a` and `3d766e63…a51732`). The
public gate-off ARM ROM remains byte-identical to word-clear candidate
`7ba4743d…fa9270`. The enabled public ROM is `ec5e6c58…a77264`, linking at
IWRAM `0x030046e0` and unchanged BSS `0x0203d674` under the fixed reserves.

The public ROM separately passes a 600-update late-P3 ARM pair against word
clear. Loaded, tick-zero and final gameplay snapshots and three capture
hashes match. Route builds fall from 74 to 16, and it measures 13.1523% fewer
hardware cycles, or 16.6546 to 19.1768 effective fps. This uses authenticated
cross-build seeded SRAM; it is neither candidate-generated save qualification
nor a full-mission performance measurement.

The exact public ROM completes all 124,689 input ticks and 5,932 input
transitions, wins at frame 123,916 and matches all 126 reported gameplay
checkpoints. Its full-mission mean is 14.08 effective fps versus word clear's
13.88 and the original baseline's 12.35. The smaller full-mission gain cannot
be inferred from the short late interval. The earlier private prototype
(`25f28551…cc4165`) had the same rounded mean; the public result is measured
separately against its own ROM and ELF.

The public ROM also passes a 12,000-input opening replay with 13 matching
gameplay checkpoints and three matching captures. Its opening mean is 18.01
effective fps versus 17.6234. A new emulator process loads the opening's own
candidate-generated SRAM through HOME / load, with exact persisted
observations and resume through frame 11,824. Its own late SRAM, full
post-load mission win, wider profiles and hardware remain separate checks.

The public source snapshot passes `./dev build`, `./dev test` (380 Python
tests plus the default host fixtures, including all four new corridor cases),
`make unified` and `make test-unified`. Original-art and unified gate-off
ROMs remain `4ad80ce0…f9aee` and `6bab5be2…979f9`. An independent read-only
review found no additional route correctness issue; this does not replace
the remaining save, wider-profile and hardware checks.

A further private packed-horizontal-span probe preserves the complete host
trace and gameplay fingerprints, but its paired 600-update ARM interval has
no meaningful elapsed-cycle gain: both builds report 19.1768 fps, differing
by only 95 cycles over roughly 525 million. It remains private. Reduced
host-side memory-read counts alone are not a performance qualification.

### Z7 economy/controller follow-up

The combined Z7 Egg/source-sight/Scourge/construction-HP/direct-attack build
also links with the existing `SC64_NEAREST_RESOURCE_DEPOT` option: ROM
`f0ee6313…6dd05a0`, IWRAM `0x03004a40`, BSS `0x0203d9f0`. A normal-input
controller verifies mineral delivery continues during the paid Lair morph,
using the existing morphing-depot rule. This is an ARM link and host witness,
without a complete mission, fresh-process SRAM or FPS qualification.

The controller initially sent only one visible Mutalisk to the source gas
position. It then misclassified the valid direct-attack order on a revealed
Extractor as a failed attack-move and repeatedly clicked it. Correcting that
test-controller check and using ordinary cursor/double-tap input to select
the nearby three-unit group clears the source Extractor and restores its
geyser at frame 9,029. The controller buys its own Extractor at frame 10,915,
but no gas-worker assignment succeeds before the economy and army are lost.
The attempt loses at frame 16,085; no additional campaign mission is counted.

### Z7 Eggs during a Lair or Hive morph

The pinned source keeps hatchery-class larva state and secondary-order
updates during building morphs. Products are bought from the individual
Larva, independently of whether its parent hall is complete. The GBA adapter
instead attributes larva products to the hall; its unconditional completed-
producer check and construction filter incorrectly blocked both purchases
and larva replenishment during upgrades.

With active Z7 and `SC64_Z7_EGG_PRODUCTION`, a live owned morphing Lair/Hive
now permits the existing paid Egg path. Owner, product, tech prerequisites,
larva availability, costs, supply and slot checks remain in that path.
Its live-larva counter continues at the existing 428-update approximation
and cap of three. Unfinished newly built Hatcheries, inactive profiles and
ineligible non-Egg products retain their rejection behavior. No fields,
arrays or save bytes are added, and internal save generation 36 is retained.

The focused actual-engine production fixture verifies cost/larva/supply
accounting, independent birth, morphing Hive, live cap/timer pause, unfinished
Hatchery and inactive/invalid producer rejection. The save fixture uses the
ordinary B/menu/A upgrade command and R/A paid Drone purchase in controlled
Z7 setup, then verifies the exact morph progress, live larva count/timer and
pending Egg product/timer through save/load. It advances the independent Egg
update directly after load to isolate birth from campaign combat; this is
not a full mission or fresh-process SRAM test. Both focused targets pass.

The combined Egg/source-sight/Scourge/construction-HP/direct-attack/nearest-
depot ARM ROM is `94eefe4d…66ba7f`, with unchanged IWRAM `0x03004a40` and
BSS `0x0203d9f0` under the fixed reserves. Separate old/new adapter links
with Eggs/Scourge disabled reproduce the same Z7 ROM `0ad661af…fc8637`.
The P3 corridor build also remains byte-identical to `ec5e6c58…a77264`.
Full Z7 playthrough, fresh-process SRAM and hardware performance qualification
remain required before promotion.

The final source passes both Egg fixtures, source sight with and without
Eggs, Z7 and P3 Scourge regressions, the ZP air-tech fixture, `./dev build`,
`./dev test` (380 Python tests plus default host fixtures), `make unified`
and `make test-unified`. Original-art and unified ROMs remain byte-identical
to `4ad80ce0…f9aee` and `6bab5be2…979f9` with the experimental gate off.

### Z7 tech and Extractor placement during town-hall morphs

The same active-Z7 Egg gate now retains a morphing hall's predecessor tech
tier and its existing 160-pixel creep proxy. The pinned OpenBW prerequisites
accept a Lair morph for Hatchery-tier structures, and a Hive morph for
Lair-tier structures. An incomplete Lair does not unlock Spire; a newly built
incomplete Hatchery supplies neither prerequisite tech nor this creep proxy.
The proxy remains an authored approximation of native creep coverage.

The source's Extractor requirement is a Drone and a valid geyser, without a
Hatchery prerequisite. Active Z7 removes only that artificial table bit;
the normal worker, owner, site occupancy, geyser and 50-mineral checks still
apply. Other mission profiles retain their existing rules. No persistent
fields, arrays or save bytes are added, and internal generation 36 is retained.

The actual-engine production fixture passes predecessor-tier and creep
checks, a paid Extractor order during Lair morph and a paid order without any
town hall. Missing geysers, occupied sites, insufficient minerals, wrong
owners/types and inactive-profile placement reject without changing resource
banks or worker orders. This is focused rule coverage; it is not a Z7 mission
win, gas-income witness or physical-hardware performance qualification.

The extension passes the focused Egg/save/sight/Scourge and ZP air-tech
fixtures, `./dev build`, `./dev test`, `make unified` and `make test-unified`.
Original-art and unified ROM hashes remain `4ad80ce0…f9aee` and
`6bab5be2…979f9`. The combined Z7 ARM candidate is `5d9c4edc…82cac0`,
with IWRAM `0x03004a40` and BSS `0x0203d9f0` below the unchanged reserves.
Separate old/new gate-off adapter links both reproduce `0ad661af…fc8637`;
the P3 corridor ROM remains byte-identical to `ec5e6c58…a77264`.

Further normal-input economy controllers clear the source Extractor at frame
9,029, start the paid replacement at 9,462 and observe completion by 10,855.
Individual worker selection preserves a safe gas return: the bank increases
from 100 to 108, with the worker's cargo returning to zero. The two attempts
still lose at frames 15,345 and 15,531; neither starts Spire or queues Scourge.
This supplies a gas-delivery witness for the ordinary worker path, not a Z7
mission qualification or additional completed campaign.

### Experimental route fields sized to the active map

`SC64_ROUTE_ACTIVE_STRIDE` retains the fixed 86,016-byte ZP PACK3 data bank
and uses `ceil(3 * active_cells / 8)` bytes per cached field. Capacity is
capped at 18: 64×64 and 128×96 maps use 18 fields, while 128×128 and 127×127
use 14. Shape changes clear derived validity/age metadata and rebind field
pointers; inactive pointers are null. Invalid dimensions are rejected before
index arithmetic or grid reads. Standard profiles retain their old layout.

Two standalone ASan/UBSan variants pass exact first-step comparisons against
an independent tie-ordered BFS, complete stored-field comparisons, randomized
routes and 64×64 → 128×96 → 128×128 → 127×127 → 64×64 changes. They exercise
partial/L/word-clear and full-flood/byte-clear paths. Canaries cover packed
last-cell writes, neighboring field starts, unaligned clears and unused pool
tails. Zero, negative, oversized and extreme signed dimensions leave query
outputs unchanged and cause no grid reads. Existing route fixtures are intact.

The full P3 host trace and per-update gameplay fingerprints match the frozen
baseline. The public enabled ARM ROM is `d1ac0694…a4e333`, byte-identical to
the measured prototype despite a declaration-only cleanup for the fallback
fixture. IWRAM ends at `0x030047f8`, BSS at `0x0203d6e4`; the extra metadata
uses 112 BSS bytes and no new save fields. Gate-off P3 and combined Z7 ROMs
remain `ec5e6c58…a77264` and `5d9c4edc…82cac0`.

A paired 600-update late-P3 ARM interval matches loaded, initial and final
gameplay observations and all three captures. Both configurations build 16
fields; the candidate measures 19.1539 effective fps versus 19.1768, or
0.1195% more cycles. This interval has no useful gain.

The byte-identical ROM completes all 124,689 normal ARM input ticks and 5,932
input transitions, wins at frame 123,916 and matches all 126 reported gameplay
checkpoints. Its full-mission mean improves from the public clear-corridor
ROM's 14.08 to 15.01 effective fps. The busy tick-73,000 → 74,000 interval
builds 471 fields instead of 769 and measures 8.67 effective fps instead of
7.01. These remain below the near-30-fps goal.

The public build also completes a 12,000-input opening with 13 matching
gameplay checkpoints and three matching captures. Its opening mean is 17.99
effective fps versus 18.0133, consistent with the small lookup overhead in
intervals without useful extra cache residency. A new emulator process loads
the opening's own candidate-generated SRAM through HOME / load, with exact
persisted observations and resume through frame 11,824. Its own late SRAM,
full post-load win, wider profiles and hardware remain separate checks.

The public source passes `./dev build`, `./dev test` (380 Python tests and
the default host fixtures), `make unified` and `make test-unified`.
Original-art and unified ROMs remain `4ad80ce0…f9aee` and
`6bab5be2…979f9`. The feature remains off in standard profiles pending the
remaining qualification.

### P7 Shield Battery property preparation

`tools/sc64_p7_battery_pack.py` authenticates the complete staged P7 CHK with
the shared scenario packer, checks the bytes again before decoding UNIT, and
derives Battery placements and starting HP/shield/energy properties into an
ignored generated header. Public source and synthetic fixtures contain no
copied source placements. Local generation is byte-identical to the private
Battery prototype's input header (`de39e075…91527`).

Seven asset-free Python checks cover record truncation, map-edge/owner bounds,
property validity and authentication failures without writing an output.
`./dev build` and `./dev test` pass (387 Python tests plus the default host
fixtures). This qualifies the importer only; Battery gameplay, shared P5/P6
save integration, ARM replay and complete P7 campaign qualification are separate.

### Experimental P7 Shield Battery

The default-off Battery gate adds paid Probe construction with Gateway and
power requirements, source-property initialization, explicit unit/area
recharge commands, bounded approach/transfer, energy regeneration and packed
save binding. New Batteries start at 100 energy; the two authenticated map
Batteries retain their original source owners and 100-percent initial energy.
No human starting Battery is invented. Active P7 uses the existing shield
fraction field at 1/1280-point precision, Q8 energy in the Battery queue-progress
field and its existing level field for the regeneration phase. Internal save
generation 37 distinguishes these meanings; no actor fields are added.

The actual-engine sanitizer fixture loads the authenticated P7 map, selects a
Probe through the contextual command, pays for construction and reaches the
site. It saves at three updates of construction progress, clears transient P7
bindings, loads and verifies their restoration, ticks the scheduler, then
finishes the real construction timer. It renders the Probe and Battery menus,
issues recharge, checks shield gain/energy cost and cooldown progression, and
round-trips the live recharge order, target, energy and fractional shields.
Candidate tests reject wrong mission/mode, dead/wrong-type/wrong-owner targets,
invalid transport orders, above-maximum shields, full shields with a nonzero
fraction and corrupt queue/phase fields. Exact full shields and empty energy
can occur between service slots and remain valid pending orders until the next
update. Reusing a dead Battery's building slot clears its old Recharge orders.
Combat destruction clears those orders immediately, including before a save.
Destroyed original map Batteries remain saveable.

The same fixture runs with Battery alone and combined with Observer/Archon.
Unchanged P5 runtime, P5 Shuttle and P6 save fixtures pass with Battery compiled
and the complete private P7 sprite bank. The P5/P7 members are each 124 bytes;
they fit the existing larger P6 payload in combined profiles. A candidate-only
tail check rejects unused bytes without mistaking another mission's nonzero
shared payload for corruption. No IWRAM, BSS or 16,368-byte save limit changes.
ARM debug types confirm a 16,348-byte RaceSave, 56-byte Unit and 48-byte
Building. The documented Make ARM target reproduces the measured ROM bytes.

The combined ARM link with Battery power checks uses ROM
`b70bf313…9e4a11` (4 MiB), IWRAM end
`0x030047d8` and BSS end `0x0203d844`. The first inline service attempt exceeded
the unchanged IWRAM guard; keeping the optional service in ROM fixes that.
The Battery-off link reproduces the previous complete-P7-art Observer/Archon
ROM `92ff35aa…175d5f` byte for byte, with BSS end `0x0203d774`.

This is mechanical and save-fixture coverage plus an ARM link, not a complete
P7 playthrough, fresh-process ARM SRAM proof or performance measurement.
Carrier Interceptors, Arbiter abilities and broader P7 production/AI remain
unfinished. Native animation/secondary orders, blinded state and disabled
statuses other than represented Pylon power remain outside this bounded
Battery implementation. Power for other P7 structures remains a separate
integration task.
The 24-to-30-Hz transfer scaling and final non-Q8 shield fraction are
approximations. Qualified staged progress remains 22/30.

The final C/header/Make inputs remain hash-stable throughout the focused
Battery suite, documented ARM build, `./dev build`, `./dev test` (387 Python
tests plus host fixtures), `make unified` and `make test-unified`; all pass.

The Battery power follow-up uses the pinned OpenBW 5-by-8 psi mask and its
outer-bound-before-negative-adjustment rule. Live placement and Recharge
availability require a completed raw-156 Pylon with both runtime and source
owner matching; source players 0 and 4 cannot share power just because the
adapter groups them under one opponent owner. Coordinates use the source
geometry at the represented half-resolution centers. Completed-Pylon activation
is derived from the represented construction state; exact source order/list
activation timing is not reproduced.

The asset-free geometry fixture checks every integer point in a 301-by-201
rectangle against independent horizontal-band bounds, signed interior/outer
edges and extreme integer inputs. Actual-engine fixtures reject
wrong-owner/incomplete/destroyed Pylon placement, cancel Recharge without
transfer on power loss, and kill supporting Pylons through combat before an
immediate pause-menu save/load. The loaded pending order cancels at its next
service update. Candidate validation checks stable identity without consulting
live power or changing the simulation. Unpowered energy regeneration and
construction progress also pass. Existing full-shield/empty-energy transients
remain saveable while forged shield bounds reject. No persistent fields,
actor sizes or reserve guards change.
Original-art and unified ROMs remain `4ad80ce0…f9aee` and `6bab5be2…979f9`.
The source-only index contains 572 text files; generated inputs and ROMs stay
local.

### Experimental Z7 derived save rows

The default-off `SC64_Z7_DERIVED_SAVE` profile uses internal save generation 38
and requires the complete Zerg ZP/Z5/Z6/Z7 Egg runtime. Constructor-derived
maximum HP and armor are omitted from Unit rows; maximum HP is omitted from
Building rows. No gameplay path reads `Unit.weapon`, so it decodes to canonical
zero instead of retaining a potentially stale constructor assignment. This
exception is semantic preservation, not a bitwise claim. Future constructor
fact changes or gameplay reads of that field require a format-generation
change or an explicit compatible migration.

The 29-byte Unit row contains 226 used bits, including an occupied-row marker;
the 25-byte Building row contains 194 used bits including its marker. Each has
six reserved zero bits. All-zero rows remain unused slots; the required marker
prevents an occupied dead actor from colliding with that encoding. Decoders
reject missing markers, nonzero padding, out-of-range fields and invalid type
facts before committing output. Building flags retain their full 16 bits,
including the Sunken/Spore morph discriminators. Unit flags are limited to the
current Zerg profile's eight-bit envelope; this format does not support future
Carrier child flags from the separate Protoss experiment.

Actual ARM types show 240 Units at 56 bytes and 160 Buildings at 48 bytes.
Reducing their saved rows from 35/28 to 29/25 bytes frees exactly 1,920 bytes:
RaceSave falls from 16,296 to 14,376, within the unchanged 16,368-byte limit.
No actor fields or runtime arrays are added. The documented full-feature probe
links to a 4-MiB ROM `10ac2f87…bd71b1`, with IWRAM end `0x03004a40` and BSS end
`0x0203d9f0`. Removing only the new gate reproduces the frozen full-feature Z7
ROM `5d9c4edc…82cac0` byte for byte after applying the same cartridge header.

The asset-free sanitizer fixture passes 100,000 randomized Unit/Building
round trips, field and dimension boundaries, negative HP/carry, unused slots,
the formerly all-zero occupied-building collision, marker/padding corruption,
and unchanged output on rejected rows. The actual-engine suite passes normal
menu save/load/next-tick/save for Z1 through Z7 and all 18 skirmish setup
combinations, paid Egg and Lair morph saves, producer death/reuse/provenance,
mid-Sunken/Spore morphs, and dead morph records. Legacy rows pass with the new
gate off, both with and without Egg production. The combined Scourge profile
also exercises its additional production-menu entry.

These are codec, candidate-validation, host lifecycle and ARM-link checks.
Fresh-process ARM SRAM qualification, a normal-input Z7 win and campaign
performance remain outstanding. No projectile pool is allocated and no
frame-rate gain is claimed. Qualified staged progress remains 22/30.

The final C/header/Make inputs remain hash-stable through the focused suite,
enabled/disabled ARM links, `./dev build`, `./dev test` (387 Python tests plus
host fixtures), `make unified` and `make test-unified`; all pass. Original-art
and unified ROMs retain `4ad80ce0…f9aee` and `6bab5be2…979f9` respectively.


### Source construction art preview

The later blue-glow correction preserves image 211's raw 1..40 strengths and
composites them through a destination-dependent 40x256 table. Ordinary palette
remapping had treated those strengths as color indices, producing an opaque,
washed-out orb. The new projection uses the displayed GBA palette, including
UI overrides, rather than only the source-remap preimages. The N64 table role
is structurally inferred and the RGB555 projection is approximate; this does
not establish pixel parity or finish the native post-timer materialization.

Local validation for this correction includes an asset-free compiled C palette
oracle (256 colors), a synthetic 10,240-cell lookup oracle, preparation guards,
and sanitizer fixtures for three source animation phases. The production
overlay fixture compares every visible effect pixel against its original
framebuffer destination, checks blue-dominant pixels, distinct frame hashes,
unchanged building state, and 371 above-map pixels with and without scene clip.
The construction lifecycle/save fixture covers all three races; the Cannon
fixture now checks exact modifier output while retaining its completed sprite,
firing, and power-loss assertions. All four production ARM builds retain their
existing IWRAM/BSS ends and save layout. These are local checks, not a new
physical GBA frame-rate measurement.


Historical preview evidence follows. The subsequent
[stock profile and art follow-up](audits/2026-10-26-stock-profiles-and-art/README.md)
enables layered source construction art in canonical prepared race builds,
adds Protoss source art and expands coverage to all 50 constructible buildings,
including the Greater Spire morph. The preview counts and ROM hashes below refer
to the earlier implementation, not the current builds. The newer
[lifecycle recheck](audits/2026-10-26-construction-lifecycle/README.md) selects
the source Warp Anchor during Protoss build progress; later materialization
states remain incomplete.

The default-off `SC64_CONSTRUCTION_SOURCE_ART` profile imports verified local
ROM art and compiles a bounded primary-frame timeline for supported Terran and
Zerg construction stages. The self-contained importer authenticates the ROM
and five DAT/IScript tables, resolves unit → flingy → sprite → image → GRP,
and writes only ignored private data. Its manifest records source, palette,
generator and output hashes. The local import contains 64 image banks, 367
frames and 64 stage programs (seven Terran/four stages each and twelve
Zerg/three each); all selected programs compile without rejected stages.
Protoss image banks are inventoried for future work, but no Protoss runtime
stages are emitted.

The original compiler accepts only `playfram`, `wait`, `goto`, `imgol`, `imgul`,
`sigorder` and `end`. It records image layers/signals as metadata; the sampler
renders only primary frames and never dispatches gameplay signals. Unsupported
control flow, malformed streams, invalid frame IDs, instantaneous loops and
work-limit overflows fail closed. Whitelisted frame waits retain their source
N+1 update duration. Intro/loop selection scans at most 64 steps and uses
integer arithmetic. Stage progress and a local 24/30 clock determine the frame;
rendering has no side effects and adds no persistent state or save generation.
An independent authored per-update interpreter matches all 64 generated
programs across 2,048 updates each (131,072 primary-frame comparisons). This
validates the compiler against the separate interpreter; it does not execute
OpenBW or establish native rendering parity.

Terran visual stage transitions approximate the native HP/order state using
strict 20/40/60-percent progress thresholds. Zerg uses strict 25/50-percent
progress thresholds and starts represented building morphs with SpecialState1.
The engine still completes construction at its existing timer boundary. Native
post-timer animation signals, shadows, SCV working animation, damage-dependent
Terran stage history, independent animation during stalled construction and
Protoss Warp Anchor/materialization phases remain outside this experiment.
Unsupported structures, negative waiting progress and Protoss use the existing
authored fallback. This is source art with adapted stage timing, not full
IScript or native construction-order parity.

The synthetic parser/importer tests cover schema alignment, frame geometry,
private header lookup and program emission, malformed/truncated inputs, loops
and stage selection. The sanitizer sampler fixture checks all progress values
for durations 1–120 plus INT_MAX boundaries, frame/wait bounds, intro/loop
wrapping and hold-last behavior. The actual-engine sanitizer fixture checks
primary sprite pixels across seven Terran and ten Zerg building types, morph
motion, frozen progress, worker waiting, completion, unsupported fallback and
same-process SRAM save/load frame continuity. It uses synthetic construction
states, not controller-built mission wins.

`make construction-visuals` produces a verified 4-MiB preview ROM, SHA-256
`ca7ad4743c6c9f95d80875c57f8308fb52b29a74db6b3656441a063d5ed2c5d0`.
The ARM link passes the unchanged reserve assertions: IWRAM ends at
`0x03004e88` (24 code bytes above canonical unified) and BSS remains
`0x0203cfe8`. No frame-rate, fresh-process ARM SRAM or GBA hardware qualification
is claimed. Canonical source-only/unified binaries retain hashes
`4ad80ce0…f9aee` and `6bab5be2…979f9`; qualified campaign progress remains 22/30.

The final snapshot passes `./dev build`, `./dev test` (404 Python tests plus
host fixtures), `make unified`, `make test-unified`, the focused source-art
renderer/save checks and the source-only Git-index audit. Imported game art,
the preview cartridge and visual captures remain private.


### P7 prevalidated runtime opening

The default-off `SC64_P7_PREVALIDATED_TICK` gate retains full immutable map,
actor-census and trigger-table validation at initialization and loaded-world
rebinding, while avoiding that repeated work on every active update. Mutable
model flags, cursors, action timers, switch bits, created-actor mask, moved
location, source wallets and transient binding identities remain checked.
The scoped world-callback flag is reset after the adapter returns, including
failure; callbacks outside that scope retain the checked path. Save and load
candidates always use the full validators. The persistent supplement remains
124 bytes, Unit/Building remain 56/48 bytes on ARM, and generation-37 RaceSave
remains 16,348 bytes within the unchanged 16,368-byte limit.

`SC64_P7_CAMPAIGN_ENTRY` separately exposes the seventh Protoss mission in
these experimental profiles. The fixture navigates HOME → the final mission
page → briefing → play, loads all 357 retained placement rows and checks the
180-Unit/112-Building opening. Reference and fast sanitizer builds emit
identical actor/runtime/wallet traces after 120 updates, normal pause/save/load,
600 further updates, a synthetic removal of the human force and real hero
deaths through combat hooks. Checksummed malformed saves reject without
destroying the live state. The fast build compares full/fast persistent
validator results for every byte offset and all 256 values (31,744 candidate
mutations), and preserves the original state. These condition stimuli do not
establish an ordinary-input mission win.

Both public ARM probes launch through ordinary menu inputs and run 600 opening
updates without tactical orders. Only the controller injection field is
written by the debugger. Effective fps derives from emulated GBA timer deltas:
reference **7.372 fps**, optimized **9.004 fps**, a 22.1-percent increase. All
14 recorded actor, mission, resource, upgrade and fog banks match at each of
seven checkpoints, with no runtime fault or unsupported source row. This
opening result does not qualify later combat, full mission completion,
fresh-process SRAM or physical hardware, and remains far below 30 fps.

The reference 4-MiB ROM is
`e75de54476958434e74f1b4fa6834aebe5735285eb8c027365686df66e11ec84`;
the optimized 4-MiB ROM is
`663d67fa38f85c9e84211a44c2ae70e72d53dbb6a8079ec84d442bababc63029`.
Both link with IWRAM end `0x030047d8`; BSS ends are `0x0203d844` and
`0x0203d84c` respectively. The unchanged internal Battery profile with the
new gates off reproduces `b70bf313…e4a11` byte for byte. Original-art and
canonical unified ROMs retain `4ad80ce0…f9aee` and `6bab5be2…979f9`.
Default build/tests (404 Python tests plus host fixtures), unified build/tests,
the focused differential fixture, ARM links and source-index audit pass.
Qualified staged campaign progress remains 22/30; the PR remains draft.

### P7 count snapshot opening

The default-off `SC64_P7_COUNT_SNAPSHOT` gate combines the eight authenticated
Command conditions into one Unit/Building census before condition evaluation.
Actions execute after those conditions. The transient snapshot is available
only inside the prevalidated tick scope, is cleared on return, and falls back
to the checked callback if actor ownership or type data is invalid. Unfinished
structures retain their reference counting behavior. Save data is unchanged.

Reference, prevalidated and count-snapshot sanitizer fixtures produce identical
traces through ordinary menu launch and pause/save/load, all three scripted
CreateUnit effects, hero deaths, Nexus-absence conditions and synthetic force
defeat. Invalid selectors/owners and out-of-scope cache access reject. The
31,744 persistent-state mutation comparisons remain in the fixture. These
stimuli test condition and action equivalence, not a normal-input mission win.

The same 600-update, no-tactical-input ARM opening improves from **9.004 fps**
to **10.512 fps** (16.7 percent). All 14 recorded banks match at seven
checkpoints, with no runtime fault or unsupported row. Later combat, complete
mission play, fresh-process SRAM and physical hardware remain unqualified;
this is still far below 30 fps.

The 4-MiB count probe hashes to
`f51042ecd245fed4b5db09ea2057a5702394fe105a9a1e22a29e2b32f0767358`.
IWRAM ends at `0x030047d8`; BSS ends at `0x0203d86c`, a 32-byte increase over
the prevalidated probe. RaceSave remains 16,348 bytes, its supplement 124 bytes,
and ARM Unit/Building sizes 56/48 bytes. Removing only this gate reproduces
the prevalidated `663d67fa…63029` ROM byte for byte. Default build/tests
(404 Python tests plus host fixtures), unified build/tests, focused sanitizer
comparisons and ARM links pass with the unchanged RAM guards. Qualified staged
campaign progress remains 22/30 and the PR stays draft.

### Mineral gathering without a completed base

The shared first-depot order loop no longer cancels a mineral trip merely
because no completed friendly townhall exists. Empty workers can approach and
gather; loaded workers hold their cargo without crediting resources or mining
again. Once a completed friendly depot exists, the unchanged return path
delivers the cargo. Gas orders and the separate opt-in nearest-depot path are
unchanged. No persistent field or save generation changes.

This follows the pinned reference's gather-then-ReturnMinerals behavior and
its preservation of cargo when a depot cannot be found. The GBA engine has
no separate ReturnMinerals state: it retries on ordinary order updates rather
than the native 75-update retry timer. Z7's authenticated opening has four
human Drones and no friendly townhall, making this a real mission blocker.

Actual-engine ASan/UBSan fixtures cover the asset-free SCV and the full Z7
profile's SCV, Probe and Drone. They verify no resource credit before delivery,
held cargo after patch depletion, no extra mining, rejection of enemy and
unfinished friendly depots, and delivery after a friendly depot completes.
The gas no-depot behavior retains its existing idle result. The focused
fixture uses nearby actors to isolate the order transition; separate private
Z7 controller attempts exercise movement and still do not qualify a win.

The default, unified and full Terran ARM builds pass the unchanged reserve
guards. Their IWRAM/BSS ends are respectively `0x03002ec0`/`0x020042ec`,
`0x03004ed8`/`0x0203cfe8` and `0x03004e68`/`0x0203d9ec`. This shared behavior
change intentionally changes their ROM identities: the 64-KiB default is
`2e387c7aab91324eca5e8eca0748f0c5d601fd2861acda1e9e77b85a34d78c04`,
the 4-MiB unified ROM is
`e12289dc36779173f25f5d43597e6c2c5bdd1d300ba6f5faaaa9efbb952c9a77`,
and the 8-MiB Terran ROM is
`737e97ae1d9712c4e94860a541736af06b7f27e3ecd2208e9343efbfd5c53d82`.
This does not establish new ARM playthrough, SRAM or performance qualification.

Default build/tests (404 Python tests plus host fixtures), unified build/tests,
both focused worker profiles and the full Terran build/regression pass with
the save-mask correction below. The ten Terran controllers observe source
wins; startup/skirmish, packed saves, repair and target fixtures also pass.
The tested source hashes remain unchanged throughout these checks.

### Terran completed-mission save mask

The campaign loader mistakenly removed T5 and T7–T10 completion bits from its
allowed mask before taking the complement. A valid completed T5 save with
progress `0x100` therefore failed even though the full Terran catalog permits
`0x3ff0`. The same failure reproduces on committed `4600901` and with the
mineral-order fix, establishing that it predates that worker change.

The runtime-specific mission bits are now ORed into the allowed set. The
existing schema, mission binding, checksums and transactional candidate
validators remain intact. The T5 sanitizer fixture reloads both victory and
defeat records after clearing process state; it also rejects an in-range
completion bit belonging to another race without mutating live state. The
late-Terran save fixture and the complete ten-mission Terran regression pass.
No persistent layout, save generation or RAM reserve changes.

### Active Lair and Hive resource depots

The pinned reference's active-depot predicate accepts grounded Lairs and Hives
even when incomplete (`bwgame.h:2191–2199`). It accepts Hatcheries when completed
or morphing. The GBA upgrade already changes a Hatchery into a Lair immediately,
but the standard first-depot lookup previously rejected every incomplete
building. Returning workers therefore held their cargo through the Lair's
entire construction clock.

The existing lookup now accepts incomplete Lairs/Hives while retaining its
ascending first-slot selection, same-owner/alive/type checks and late-Terran
airborne rejection. This does not enable the separate nearest-depot experiment
or introduce a marker, field, format generation or new persistent state.
Fresh incomplete Hatcheries remain ineligible. Incomplete Lair/Hive acceptance
also applies to imported placements, matching the explicit source predicate.
The shared lookup also supplies skirmish AI base-presence and targeting helpers,
so they continue to recognize the base during these upgrades.

Actual-engine Z6 and full Z7 sanitizer fixtures exercise the paid Hatchery-to-Lair
helper and mineral/gas deposits before completion. They also check first-slot
selection, incomplete Hatchery and enemy/non-depot rejection, an unfinished
Hive's mineral delivery, packed building roundtrip, death and slot reuse. The
Hive state is direct fixture setup; no Lair-to-Hive command is established.

A private ordinary-input Z7 comparison restores deposits during the upgrade:
the same controller has 25/49 minerals at frames 7826/8126 instead of 1/1.
Its bounded trial still has 52 enemy buildings remaining, with no qualified
win. Another controller advances Extractor construction by about 2000 updates
but loses at frame 16071. These results establish the resource-return bug and
remaining tactical/economy work, not mission completion or an fps improvement.

The integrated change passes `make test-sc64-active-hall-depots` under
ASan/UBSan, `./dev build`, `./dev test` (404 Python tests plus host fixtures),
`make unified test-unified`, and the full Terran build/test workflow, including
all ten mission controllers and the 234-configuration skirmish startup/save
matrix. The default ROM is byte-identical to the previous build. The updated
unified and Terran links end IWRAM at `0x03004ee8` and `0x03004e88`, respectively;
BSS remains `0x0203cfe8` and `0x0203d9ec`. These retain the existing RAM reserves.
These are host regression and ARM link checks; no new ARM playthrough or
hardware performance qualification is claimed.


### Experimental P7 Arbiter cloak and cached detection

`SC64_P7_ARBITER_CLOAK_CACHE_ENABLED` is a single default-off profile gate. It
requires the P7 Observer and Battery integrations, authenticated P7 data/runtime
and actor facts, Protoss campaign actors, and race support. When enabled, save
generation 39 assigns meaning to the previously reserved P7 phase/stagger bytes
and to cache bits in the existing `Unit.weapon` word. It adds no Unit field or
persistent supplement bytes; the generation change intentionally separates
these cache-bearing saves from earlier experimental formats.

The cache approximates the native source update schedule at four source
updates per five GBA frames. It performs the first refresh on the first ordinary
play tick after map setup. A unit newly entering the Arbiter aura or an intrinsic
cloak is refreshed at the next source update. Detector checks use current
optical visibility and authenticated source-owner identity; the cache retains
per-target detection state until the source timer expires. This means cloak
entry does not synchronously refresh every detector on that exact GBA frame.

`make test-sc64-p7-arbiter-host` is the opt-in host matrix. It builds full,
prevalidated and count-snapshot runtime variants, compares their traces, and
includes standalone cache-policy, merged Battery/cache save-load, Observer
without cache, and Battery without cache fixtures. The cache-off variants keep
the prior Observer and Battery policies. The cold-SRAM block exercises
`game_init()`, cleared host process state, and the ordinary title `KEY_L` path;
it checks exact host-side restoration of P7 state, units and each `Unit.weapon`,
buildings, player/resources, frame, seed and supply before the next tick. It is
a host fixture and does not qualify physical SRAM or emulator behavior.

The final optional count-snapshot/prevalidated Arbiter probe links at 4 MiB,
with IWRAM ending at `0x030047d8` and BSS at `0x0203d878`. Its SHA-256 is
`9679865837f763a1c7391b5129e1a567507ad30a60c9a1ff014086bdb1fca385`.
ARM debug types confirm Unit 56, Building 48, RaceSave 16,348 and P7 supplement
124 bytes. `update_orders` is placed in ROM only for this feature's ARM build.
No FPS improvement or mission win is claimed; no Arbiter emulator or hardware
gameplay qualification is included.

The native global refresh iterates a combined visible-unit list. This model
uses represented live, non-loaded actors in slot order, excluding map-revealer
actors; buildings and unrepresented hidden/child states do not take stagger
slots. It computes aura/detection before GBA order movement, while the native
engine has additional movement and sprite phases. These are scheduling and
representation approximations, not instruction-exact simulation. Native aura
also excludes units with the WarpIn order; the current GBA actor model has no
warp-in state and cannot express that exclusion. Arbiter Recall, Stasis,
Carrier production/projectiles and a full P7 win remain separate work. Asset-free negative compile checks require the actual game translation
unit to reject the bare, missing-dependency and old split feature gates.

The cache clock is zero before the first source update. Saving immediately
after launch preserves that pending update; loading this initial save follows
the same first-tick path as resuming without a reload. A separate normal-menu
fixture pauses and saves before any play tick, clears mission/cache process
state, loads from the title, and compares the first-tick units, buildings,
mission state, frame and RNG with the uninterrupted path. Running saves keep
their existing detection state and phase until the saved clock advances.

The final eight-fixture optional matrix passes under ASan/UBSan, including
full/prevalidated/count output parity and the immediate launch-save case.
The baseline fails the latter's exact Unit-array comparison; the corrected
path matches units, buildings, mission state, frame and RNG at clock 149,
phase 4 on the first tick. Default build/test (407 Python tests plus host
fixtures), unified build/tests and full ten-mission Terran host regressions
also pass. After the final P7-only launch-clock correction, rebuilt default,
unified and Terran ARM ROMs remain byte-identical to published `0530cd0`.
This establishes host behavior and ARM placement, not P7 completion or speed.

### Experimental P7 typed save rows

The default-off generation-40 typed codec stores Unit/Building rows in 28/25
bytes and preserves the full Unit weapon/cache word and existing P7 supplement.
Type-derived max HP and unit armor must match their constructors. Separate
occupied markers distinguish zero slots from occupied dead actors. Load
candidate validation decodes rows before mutating the live world. Invalid
padding, out-of-domain fields and malformed cache state reject transactionally.

The asset-free synthetic-fact fixture covers 12,000 randomized row pairs, all
64 type codes, field boundaries, zero/dead rows, markers/padding, rejection
atomicity and comparisons with established row codecs. These synthetic facts
validate packing only. Actual constructor facts and P1–P7/18-skirmish save/load
checks require the private-input engine fixture. Full/prevalidated/count cache
fixtures, Battery recharge, cache policy and the immediate-launch save fixture
also have typed variants. These establish represented engine/save behavior;
they do not establish native gameplay parity or physical SRAM behavior.

The optional ARM probe links at 4 MiB with IWRAM end `0x030047d8` and BSS end
`0x0203d878`. Its SHA-256 is
`3213c90da953b0a83742f79a1acba0709d3c99e3c13420e3704d62277061ad05`.
ARM debug types confirm Unit 56, Building 48 and RaceSave 14,188 bytes. The
payload is 2,160 bytes smaller than generation 39; runtime actor storage is
unchanged. A same-source gate-off ARM build reproduces the published
generation-39 Arbiter ROM byte for byte.

The actual-engine fixture and six-variant typed matrix pass under ASan/UBSan,
including full/prevalidated/count trace parity, active cached detection and
recharge saves, and immediate-launch save continuation. Default build/tests
(410 Python tests plus host fixtures), unified build/tests, and the full
ten-mission Terran build/regression also pass. Default, unified and Terran
ROMs remain byte-identical to published `ad0dde0`. After that complete run,
the asset-free packing fixture was extended to compare every randomized pair
with the legacy codecs and cover signed dead-HP boundaries; its dedicated
sanitizer target passes again. No runtime source changed during validation.

The payload bound preserves the existing two-slot journal: each 16 KiB slot
reserves 16 bytes for its header in the 32 KiB save area. The 2,180-byte gap
to that bound is not a proven projectile budget.
Lossless subpixel motion, stable owner/target references, source-update phases,
child-actor state and the native 100-bullet capacity remain required design
constraints. No projectile pool, additional mission win, fps result or hardware
qualification is claimed by the codec work.

#### Generation-40 movement composition

PR6's movement optimization also passes with the typed generation-40 P7
save rows and Arbiter cloak/cache enabled. Build it with
`make protoss-p7-typed-move-probe`; the opt-in flags compose
`P7_TYPED_COMPACT_FLAGS`, `P7_MOVE_FLAGS`, prevalidation and the count snapshot.
`make test-sc64-p7-typed-move` passes under ASan/UBSan: the combined runtime
trace matches typed count, the steering fixture matches that baseline while
checking 619,216 queries against `unit_step_clear()` (335,229 blocked,
77,402 list builds, 36,304 fallbacks), and Battery and initial-save fixtures
pass. Immediate-save continuation has clock 149, phase 4 and frame 1.

Both ARM probes are 4 MiB. DWARF reports Unit 56, Building 48, RaceSave 14,188
and P7 supplement 124 bytes in each. The typed count baseline reproduces ROM
`3213c90da953b0a83742f79a1acba0709d3c99e3c13420e3704d62277061ad05`,
with IWRAM/BSS ends `0x030047d8` / `0x0203d878`. The composition is
`654be346fc4f39e059fda3c2fd0c9099f8dc70da5bf0a708bbcb85bb000c7a73`,
with ends `0x03004a28` / `0x0203daec`. Both remain inside the unchanged RAM
reserve and two-slot SRAM journal guards. Generation 40 and the existing
source clock remain unchanged.

The same normal-menu, controller-only 600-update P7 opening measures
**7.880 -> 9.045 effective fps** (14.8 percent faster) using emulated GBA
hardware timer cycles. All seven checkpoints match in every recorded
non-timing field, including bank hashes, decoded actors, per-owner HP,
selection counts, input schedule and ending frame 602. Runtime faults and
unsupported counts remain zero. Reproduce both runs with:

```sh
make protoss-p7-typed-probe protoss-p7-typed-move-probe
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-typed-probe.elf \
    --rom build/StarCraft-Protoss-P7-Typed-Probe-GBA.gba --out build/p7-typed-opening \
    --neutral-ticks 600
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-typed-move-probe.elf \
    --rom build/StarCraft-Protoss-P7-Typed-Move-Probe-GBA.gba --out build/p7-typed-move-opening \
    --neutral-ticks 600
python3 tools/sc64_p7_move_workload.py --compare build/p7-typed-opening/result.json \
    build/p7-typed-move-opening/result.json
```

This comparison includes cloak/cache, unlike the earlier cache-off movement
probe below. It covers the opening only: no combined attack workload, mission
win, fresh-process ARM SRAM, hardware qualification or near-30-fps result is
claimed. Neither the default nor canonical build enables the composition;
staged campaign progress remains 22/30.

### P7 source-rectangle reuse

`SC64_P7_GEOMETRY_RECT_HOIST_ENABLED` reuses validated rectangles within the
full-feature typed/Arbiter/movement source tick. An Arbiter rectangle is
validated once per emitter; an aura target rectangle is reused for the
clipped finder and exact distance tests. Detection computes the target
rectangle once, then validates each Observer/Cannon rectangle as before.
The half-open rectangle conversion, inclusive 160-source-pixel threshold,
owner checks, power checks and source timers are unchanged. The flag requires
the complete Arbiter cache profile and remains disabled in default/canonical
builds.

`make test-sc64-p7-rect` passes under ASan/UBSan. Its fixture compares 28,710
actor/building rectangle pairs and 2,475 finder cases with the original
helpers, including unsupported rows and extreme coordinates. Existing live
cache geometry/cadence/reuse checks pass. Full-runtime and steering traces
match typed movement; the steering oracle covers 619,216 queries. Battery
and immediate-save continuation pass, with clock 149, phase 4 and frame 1.
The default ARM build, 414 Python tests plus asset-free host fixtures, and
unified build/tests pass. Gate-off typed movement reproduces its published
ROM hash byte for byte.

`make protoss-p7-rect-probe` creates a 4-MiB ROM with SHA-256
`a4efa9f3ea7c122714236b926b890d06eb704226c60284c03723a1d19d2772ab`.
ARM DWARF reports Unit 56, Building 48, RaceSave 14,188 and P7 supplement 124
bytes. IWRAM/BSS ends remain `0x03004a28` / `0x0203daec`, inside the unchanged
RAM guards. Save generation 40 and the two-slot journal are unchanged.

The normal-menu, controller-only runner selects the same 11 army units and
issues an attack-move after a 600-update opening, then measures 1,500 attack
updates. All 23 non-timing checkpoints, 25 input records, command details and
ending frame 2,605 match typed movement. Emulated GBA timer cycles measure:

| Full-feature workload | Typed movement | Rectangle reuse |
| --- | --- | --- |
| Opening, 600 updates | 9.045 | 9.205 |
| Attack-move, 1,500 updates | 6.869 | 7.162 |

This is 1.8 percent faster in the opening and 4.3 percent in attack. Reproduce
the comparison with:

```sh
make protoss-p7-typed-move-probe protoss-p7-rect-probe
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-typed-move-probe.elf \
    --rom build/StarCraft-Protoss-P7-Typed-Move-Probe-GBA.gba --out build/p7-move-attack \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-rect-probe.elf \
    --rom build/StarCraft-Protoss-P7-Rect-Probe-GBA.gba --out build/p7-rect-attack \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --compare build/p7-move-attack/result.json \
    build/p7-rect-attack/result.json
```

A separate private source-owner-mask cache also matched all 23 checkpoints,
but measured 9.031 / 6.913 fps. Its opening regresses and its attack gain is
only 0.64 percent, so it is not promoted as a standalone runtime option.
Neither experiment qualifies a mission win, fresh-process ARM SRAM,
physical GBA performance or near-30-fps campaign play. Staged progress remains
22/30; Carrier combat and a P7 win are still required.

The following private variant combined rectangle reuse with per-source owner
masks and lazily cached sight radii. Its 1,202-byte ARM context expires each
source tick; the compiler reports a 2,488-byte source-tick frame. Host checks
prove nonzero cache hits/misses, sentinel handling, per-query equivalence to
the original visibility scan, runtime/steering parity, Battery and initial
save continuation. All 23 ARM checkpoints match rectangle reuse, but fps is
9.128 / 7.147 versus 9.205 / 7.162. It consumes more stack and is slower in
both measured workloads, so the owner/radius variant is not promoted.

### Carrier pool and codec foundation

`starcraft/sc64_interceptor_pool_codec.[ch]` provides authored, asset-free
storage for a future 100-slot Interceptor projectile pool. It preserves the
reviewed ten-slot growth, last-free growth, physical-ordinal/native-index
mapping and active/free insertion order. The arena uses exactly 2,408
volatile halfwords (4,816 bytes), with explicit halfword reads/writes. The
2,103-byte packet has a three-byte header and 100 fixed 21-byte rows; row
padding, reserved fields, signed-coordinate limits, references, timers and
complete list partitions are validated. Decode preflights all rows/lists
before committing and reconstructs previous links and counts. Unallocated
rows and storage links must be canonical. Packet/arena aliases are rejected
before mutation.

`make test-sc64-interceptor-pool-codec`, also in `./dev test`, passes under
ASan/UBSan. It checks a deterministic 100-acquire/100-retire sequence and
20,000 successful randomized transitions (10,049 acquisitions, 9,951
retirements), comparing both full list orders after each transition. It also
checks 250 snapshots of 40 allocated rows: 10,000 randomized field rows,
including active/free rows, signed endpoints, sentinel/max references,
speed/timer limits and reused slots. Re-encoded packets match byte for byte
and decoded fields match their inputs. Canary, malformed-field, reserved
metadata, unused-link, alias and rejection-atomicity checks pass.

Private checks compare empty and size-20/13-active C packets byte for byte
with the previously reviewed C++ codec-v3 model. The earlier native pool-order
oracle checks the allocator model separately; this C test needs no upstream
headers or game data. ARM7TDMI Thumb `-O2` compilation passes. Static compiler
frames are 408 bytes for decode, 192 for validation and 88 for the row parser;
these are individual frames, not whole-game stack high-water. A separate
non-inlined access audit emits `strh`/`ldrh` for arena access.

The later optional generation-41 projectile profile now reserves the arena
and adds the packet to RaceSave; see [Carrier projectile development](CARRIER-PROJECTILES.md).
That profile adds C motion, source-clock updates, reference cleanup, damage
consumption and host save continuation. Child flight/training and the gameplay
weapon-event caller remain unfinished. No Carrier combat, P7 win, full native
world continuation, fresh-process ARM SRAM or hardware result is claimed.
Staged progress remains 22/30.

### P7 Carrier training and hidden hangar

The separate default-off generation-42 training profile adds paid Interceptor
queues and hidden training/inside Unit rows; see
[Carrier projectile development](CARRIER-PROJECTILES.md) for commands and scope.
It retains the 16,292-byte RaceSave, 56/48-byte actor rows and 240/160 actor
caps. Candidate decoding uses the unused tail of the existing route-save
scratch union, with a compile-time bound; BSS remains `0x0203daf4` and IWRAM
ends at `0x03004a30`, below the unchanged guards. It cannot launch children
and requires an empty projectile packet, so no generation-41 firing fixture
is presented as generation-42 gameplay.

`make test-sc64-p7-carrier-training` passes under ASan/UBSan. Checks cover
player menu/price, four-child capacity, initial source-clock cadence, exact
300-source-tick construction, save/load at construction and inside boundaries,
15 malformed candidate rejections without live actor/building/pool/P7-runtime/
wallet/frame/RNG mutation, child-rank compaction, scripted parent death,
unit-slot reuse, competing producer/script reservations, hidden-unit damage
and sight, pause, mission setup/save checks and 18 skirmish setups. Most
construction ticks are isolated from campaign armies; these are component
checks rather than a mission replay. Player tests supply a human-owned Carrier:
the authenticated starting Carriers belong to enemy source owner 4. A separate
case advances an explicitly supplied owner-4 paid queue; enemy autoqueueing is
not implemented. Both owners' effective Carrier Capacity current/max levels
are zero, so four slots match this mission's data.

The ARM probe builds as `StarCraft-Protoss-P7-Carrier-Training-Probe-GBA.gba`,
title `SC P7 HANG42`, code `SC7K`. Carrier production, launch orders, native
flight/IScript events and actual Carrier combat are still pending. This stage
does not qualify a P7 win, ARM SRAM continuation, whole-game stack high-water,
physical hardware or near-30-fps combat. With projectile/training gates off,
preprocessed P7 typed-rectangle game tokens match the `bfb4d45` baseline
(5,945,648 tokens); no ROM byte-equality claim is made. Staged campaign progress
remains 22/30.

### Hidden showcase menu and staged scenes

The optional `SC64_SHOWCASE_ENABLED` build adds a hidden HOME chord and eight
repeatable starting setups; see [Showcase scenes](SHOWCASE.md). It composes
the typed/Arbiter/movement/rectangle profile to provide the required actors,
but runs scenes as skirmishes, with campaign controllers and economy AI
inactive. Normal movement, target selection, damage, shield regeneration and
loss detection remain in use. The filming arena is flat and fully revealed.
Scene state is transient, and both save/load entry points reject while a
scene is active. The normal campaign and canonical build flags keep the
feature disabled. The frontend structure and save format are unchanged.
The disabled-feature ARM output matches a fresh `782539f` parent build made
with the same current private inputs, byte for byte; preprocessed game tokens
also match that parent.

`make test-sc64-showcase` passes under ASan/UBSan: ordinary menu entry,
all eight compositions, bounded simulation/rendering, pause/restart/exit,
scene-row retention and an actual normal-skirmish save/load surrounding
showcase play are checked. The three last stands lose at update 150 via
ordinary Marine damage and skirmish defeat detection, including the Nexus's
passive shield regeneration. No timed damage or forced outcome is injected.
The input-model test also runs in the asset-free default host suite.

The 4-MiB ARM showcase link retains Unit 56, Building 48, RaceSave 14,188 and
frontend 56 bytes, with IWRAM/BSS ends `0x03004a28` / `0x0203daf0`, inside the
unchanged reserve guards. The incomplete feature bundle is rejected at
compile time. Carrier Approach is labeled a preview: the current Carrier
actor has no Interceptor combat; its escorts engage normally. These are
authored filming setups, not additional qualified campaign missions or
native weapon/AI parity. Five seconds means 150 simulation updates at the
nominal 30-update clock; physical duration depends on performance.

A normal-menu, controller-only ARM replay exercises all eight scenes,
paused launches, replay and scene-menu return. All three defeat variants
remain alive at update 149 and lose at 150. Captures were visually checked
for menu readability, Carrier formation, the tank rush and the result panel.
The first 120 running updates measure effective fps from emulated GBA timer
cycles: Carrier Approach 13.175; Siege Line 19.965; each Last Stand 29.857;
Protoss Crossfire 19.164; Three Way Battle 20.020; Base Under Siege 24.961.
These are bounded emulator samples, not physical GBA or whole-scene speed
qualification. Default build/tests (414 Python tests plus host fixtures),
unified build/tests, the full-bank scene-reset comparison and source audit
pass. No additional campaign mission or near-30-fps battle qualification is
claimed.

### Full-feature P7 cache timing and rejected cell memo

A private diagnostic of the typed/Arbiter/movement profile separates the
source cache call from order setup. At the 600-update checkpoint it takes
about 420,000 cycles, versus 380,000 for movement and 322,000 for mission
processing. A second diagnostic splits that cache work into power checks,
aura/stagger work and detection refresh. At updates 100/200/300/400/500/600,
power is 44,474 cycles each; aura is
84,852/99,552/118,564/115,255/141,938/162,581; refresh is
132,737/209,386/132,335/444,120/210,038/210,945. These are sampled last-tick
counters, not whole-run average costs. Instrumented runs match all seven
non-timing opening checkpoints; their roughly 8.85 fps includes diagnostic
overhead and is not a production speed result.

The first candidate memoized only the optical source-owner mask for repeated
16-pixel target cells within one source-cache invocation. Its 32-entry,
96-byte stack table retained full cell keys, used exact scans on collisions,
and expired before unit orders could move actors. Exact detector rectangles,
ownership checks and timers remained per target. The host checks covered
source-0/4 separation, nonzero Observer detection, hit/miss/eviction and reset
after a sight source changed, runtime/steering parity, Battery and immediate
save continuation. The ARM link retained the existing actor/save sizes and
RAM ends; gate-off typed-movement output was byte-identical.

A normal-menu controller-only comparison ran the 600-update opening, selected
11 army units and issued the same attack-move, then ran 1,500 attack updates.
All 23 non-timing checkpoints, 25 recorded inputs, command details and the
ending frame 2,605 match. Neither run wins or loses within this workload.
Effective fps from emulated GBA timer cycles is:

| Full-feature workload | Typed movement | Private cell memo |
| --- | --- | --- |
| Opening, 600 updates | 9.045 | 9.068 |
| Attack-move, 1,500 updates | 6.869 | 6.911 |

The gains are only 0.25 and 0.61 percent, so the memo is rejected for runtime
promotion. No cell-memo flag or build target is added to the public project.
The measurements instead direct further work toward repeated rectangle/fact
calculations inside aura and detection, and mission processing. They do not
qualify a mission, fresh-process ARM SRAM, hardware or near-30-fps result;
staged campaign progress remains 22/30.

### P7 movement steering list

The P7 count probe spends most of each update in `update_orders()`. A
diagnostic build of the 600-update opening attributes about 0.85M of 1.16M
order cycles per update to movement, and about 0.60M of that movement to
blocked ground movers. When the next step is occupied, `move_toward()` tries
eight one-pixel side steps. Each attempt rescans the collision buckets and
repeats ROM unit-fact lookups for every nearby actor. Nearest-target queries
take under 0.1M cycles. Moving `sc64_target_nearest` into ROM, as tried in an
earlier private experiment, therefore addresses the smaller cost.

The default-off movement probe enables two existing options:
`SC64_ZP_FACT_RANGE_CULL` and `SC64_COLLISION_CLASS_CACHE`. It also adds
`SC64_COLLISION_STEER_LIST`. After the first side-step test, the list keeps the
nearby candidates that pass the unchanged alive, loaded, burrowed, Chrysalis,
flying and collidable filters. Each candidate keeps its position, combined
radius and bucket cell. Every side step then reapplies its own original cell
window and strict distance tests. Because the result only asks whether any
candidate blocks, visiting order cannot change it. Gathering workers, an
inactive bucket index, maps wider than 256 cells and lists over 48 candidates
use the original query. Terrain checks, steering order, cost ties, speed,
retry timers and RNG use are unchanged. No save fields are added.

`make test-sc64-p7-move` launches P7 through the menu in reference and
candidate ASan/UBSan builds. It hashes Unit, Building, P7 runtime, frame and
wallet state after every one of 1,200 neutral updates. It then issues one
controller-only attack-move and hashes 1,500 more updates. Both builds print
`e3584674` and `824e2f6d`. The candidate checks 619,216 side-step answers
against `unit_step_clear()`, 335,229 of them blocked. They come from live
states and 400 randomized dense crowds, including map edges and gathering
workers. The candidate also reproduces the existing menu, trigger and
pause/save/load fixture output byte for byte.

ARM timing uses `tools/sc64_p7_move_workload.py`. The debugger writes only the
controller field. Effective fps comes from emulated GBA timer cycles over
complete in-play 100-update intervals. The attack-move workload runs the
600-update opening, then R, B, DOWN and A. It moves the cursor with the D-pad to
the enemy building nearest the selected army, then presses A. Both builds lose
the mission at frame 2394. Builds were measured on base `ad0dde0`; both probes
rebuild byte for byte on `0135d33`.

```sh
make protoss-p7-count-probe protoss-p7-move-probe
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-move-probe.elf \
    --rom build/StarCraft-Protoss-P7-Move-Probe-GBA.gba --out build/p7-move-attack \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --compare REFERENCE/result.json CANDIDATE/result.json
```

| Workload | Count probe | Existing options only | Movement probe |
| --- | --- | --- | --- |
| Opening, 600 updates | 10.512 | 12.335 | 12.694 |
| Neutral, 2,400 updates | 7.876 | 10.235 | 11.188 |
| Attack-move, 1,200 complete updates | 6.760 | 8.958 | 10.055 |

Every checkpoint matches the count probe in all three builds. Each checkpoint
compares SHA-256 hashes of the actor, map, campaign, P7 runtime, wallet, supply,
upgrade and fog banks. It also compares decoded actors, per-owner HP and
selection counts. Runtime faults and unsupported rows are zero. The same
inputs and command phase are recorded. At update 2,400 of the neutral run,
order cycles fall from 2.12M to 1.02M. Drawing and non-order updates are
unchanged at about 0.63M cycles, so 30 fps is not reachable from movement work
alone.

The movement probe ROM is
`04706eb5c18296e3e6fbf85d7a258dba38c9c5d0df4173300f6cc36537719c46`.
IWRAM ends at `0x03004b98`, 960 bytes above the count probe, and BSS ends at
`0x0203dae0`, 628 bytes higher. Both remain inside the unchanged guards.
The `move_toward()` stack frame grows from 92 to 124 bytes. RaceSave remains 16,348 bytes, the P7 supplement 124 bytes, and Unit/Building
56/48 bytes. With these flags off, preprocessed `game.c` is identical for the
original-art, unified, full Terran, Z6 and P7 count-probe profiles; the count
probe ROM is `ac2ca616…f51fd`.

Rejected variants matched state but did not justify their cost. An 8-bit
blocked-direction mask was slower. IWRAM list storage and a per-build burrow
hoist each gained under 1.5 percent. An IWRAM target-index rebuild gained
0.8 percent for 400 IWRAM bytes. Inlining the P6 burrow guard at every call
site overflowed the IWRAM guard. These are emulator results only. They do not
qualify a mission win, fresh-process SRAM, physical hardware or 30 fps. Staged
campaign progress remains 22/30.

### P7 build-time fact tables

The movement probe still derives per-type facts at run time on several hot
paths. Each unit's `move_toward()` checks movability and flight. Collision
radius, flight and collidability are resolved on every class-cache miss.
Targeting checks flight and hidden policy, and `draw_minimap()` checks
drawability. Each lookup walks the gate-specific fact switches (P7, Z6, P5,
P6, Z5 and Z/P), then the skirmish flight list, often from ROM Thumb code.
`SC64_FACT_TABLES` is default-off. With it, a host generator evaluates the
unchanged functions under the probe's exact flags for all 49 unit and 58
building types. It emits packed ROM tables: a 32-bit word per unit (actor
flags, radius, flight) and a 16-bit word per building. The tables take 196 and
116 bytes. The hot paths read these tables; the original functions remain as
`*_reference`, and out-of-range types still use them. The generated header
lists the 23 actor gates it was built with. A build with different gates fails
with `#error`, and a static assertion checks the roster sizes. The per-slot
collision-class cache and steering list are unchanged.

`make test-sc64-fact-tables` compares every table entry with the reference
functions in 21 host gate configurations, each built with and without
`SC64_ZP_FACT_RANGE_CULL`. They are the 16 fact-range gate sets, four P7, Z6
and Z7 variants, and the P7 movement profile's gates. It checks 64 types past each end of both rosters,
and the integer extremes except in late-Terran builds. The late-Terran
reference lookup overflows on `INT_MIN`, so those builds skip the extremes. It
also checks the derived collidable, movable, drawable, hidden and selectable
policies, and that every fact-selecting macro in the fact headers is recorded.
Tables built for `zp_p7` are rejected by `zp_p7_projectiles`, `zp_p6` and `zp`
builds. `make test-sc64-p7-fact-tables` makes the same check under the P7 flags
(1,996 checks). The ASan/UBSan fact build also prints the movement trace hashes
`e3584674` and `824e2f6d` after 619,216 steering-list oracle checks. It
reproduces the count-snapshot menu, trigger and pause/save/load fixture byte
for byte.

ARM timing uses the steering-list workloads on base `fad7926`. The movement
probe was rebuilt on the same base:

```sh
make protoss-p7-move-probe protoss-p7-fact-probe
python3 tools/sc64_p7_move_workload.py --elf build/starcraft-protoss-p7-fact-probe.elf \
    --rom build/StarCraft-Protoss-P7-Fact-Probe-GBA.gba --out build/p7-fact-attack \
    --neutral-ticks 600 --attack-ticks 1500
python3 tools/sc64_p7_move_workload.py --compare REFERENCE/result.json CANDIDATE/result.json
```

| Workload | Movement probe | Fact tables, setup inlined | Fact-table probe |
| --- | --- | --- | --- |
| Opening, 600 updates | 12.579 | 12.610 | 12.677 |
| Neutral, 2,400 updates | 11.140 | 11.170 | 11.215 |
| Attack-move, 1,200 complete updates | 10.044 | 10.072 | 10.098 |

The gain is small, 0.5 to 0.8 percent. The existing range cull already
rejects the common skirmish-roster types early, and the class cache already
resolves collision facts once per slot each frame. Every checkpoint, input and
ending matches the movement probe in all three workloads. Both builds lose at
frame 2394 in the attack-move run. In the "setup inlined" build, the smaller
table-backed code let GCC inline the once-per-frame IWRAM bucket setup into
the ROM Thumb `update_orders()`. The submitted probe keeps that setup out of
line in IWRAM, but only when `SC64_FACT_TABLES` is defined.

The fact-table probe ROM is
`1f929e98f809483bad02449ed202813d9a03005cdbbaa9d2c4493459bb504ebd`.
IWRAM ends at `0x03004900`, 664 bytes below the movement probe's `0x03004b98`.
The IWRAM bodies of `move_toward()`, `unit_step_clear()` and the class lookup
shrink by 276, 132 and 236 bytes. BSS is unchanged at `0x0203dcc8`, and ROM text
is 4,248 bytes smaller. With the flag off, the original-art, unified and P7
movement-probe ROMs are byte-identical to `fad7926`. Saves and save layouts are
unchanged. These are emulator results only. They do not qualify a mission win,
fresh-process SRAM, physical hardware or 30 fps.

### Weapon-family effects across implemented attacks

The shared visual pass replaces ordinary straight-line tracers with authored
weapon families. Unit profiles cover guns, needles, missiles, plasma, flames,
melee, acid, shells, psionic arcs, contact explosions and laser pulses; Goliath,
Wraith and Scout choose different ground/air effects. Static defenses use
plasma, underground spines, spores and missiles. T4 traps, P6 wall missiles,
P6 Infested Terran explosions, T5 mines and the specialized P7 Archon splash
callback now emit corresponding effects. This does not implement unavailable
weapons, Carrier flight, Mutalisk bounce damage or imported IScript sequences.
Damage, target selection, cooldowns and RNG remain on their existing paths;
travel and impacts are cosmetic rather than delayed damage events.

`make test-sc64-weapon-vfx-profiles` passes under ASan/UBSan for unified (30),
full Terran (42), Z7 (37) and P7 (45) armed unit/target-class profiles. It checks
actual ground/air attack events, visible lifetimes, impact phases, clipping
to the battlefield and current fog, camera translation, saturation replacement
and the same-update damage contract. Pure renderer checks cover extreme
coordinates, distinct orb/blob/shell silhouettes, vertical beam orientation
and hidden pixels. T4 traps, P6 explosion/trap events, T5 mines and Archon's
specialized attack have separate emission assertions in their existing tests.
Host showcase checks retain all eight setups and update-150 last-stand defeats.

The race-profile transient effect pool grows from 12 to 32 entries (+480 BSS
bytes). Overflow deterministically replaces the oldest cosmetic effect. The
original-art pool stays at 12, retaining its save layout; RaceSave and actor
layouts are unchanged. All drawing clips before framebuffer writes and no
long source-to-target line scans through offscreen space. Mapping and rendering
remain in ROM. At the weapon-family stage, IWRAM/BSS ends are: Terran `0x03004e88/0x0203dbcc`,
Z3 `0x03004f88/0x02039a58`, P3 `0x03004f30/0x02039c00`, showcase
`0x03004a28/0x0203dcd0`, Z7 derived `0x03004a40/0x0203dbd0` and P7 training
`0x03004a30/0x0203dcd4`. The existing reserve guards pass.

A controller-only mGBA replay passes all eight filming scenes, including every
last-stand defeat at update 150. A fresh `e72fe11` source build with the same
current private inputs is the timing baseline. The five battle scenes measure
13.031, 19.854, 18.911, 19.745 and 24.275 effective fps, versus baseline
13.175, 19.965, 19.164, 20.020 and 24.961 (0.56–2.75% slower). All three
last stands remain at 29.857 fps. These are short emulator samples, not physical
hardware or a new 30-fps qualification. Captures were inspected alongside the
asset-free per-family animation gallery.

The current showcase ROM SHA-256 is
`56cec04bba21be1566a4847d2649f3df45aa5292fce8582666b6a71001fa711a`.
Default build/tests (414 Python checks plus host fixtures), unified build/tests
and full Terran build/tests pass, including the ten controller missions,
saves and skirmish matrix. This visual pass does not promote any new campaign
mission or repeat full ARM campaign/save qualification; staged progress remains
22/30. ROMs, captures and generated art remain local.

### Separate shield and health display

Protoss shields already absorb incoming damage before HP and regenerate/save
separately. This presentation fix draws a cyan shield bar above the existing
HP bar on shielded units and buildings. The blue empty track remains visible
at zero shields, and the selected actor has a separate SH number. Capacities
come from canonical actor facts with the existing rule-table fallback; Archon
350-shield/10-HP and Carrier 150-shield/300-HP ratios are checked explicitly.
Unshielded actors retain one health bar. Bars and weapon effects clip to the
actual battlefield rows 12–119; the lower HUD begins at row 120.

`make test-sc64-shield-bars` passes under ASan/UBSan in unified and P7 profiles,
checking actual unit/building overlay pixels, full/partial/zero shields, HP
independence, unshielded actors and viewport edges. The shared HUD fixture
checks shield-only changes on a selected unit and Pylon, requiring bottom-band
invalidation and exact equality with a forced redraw. The weapon-VFX game
regression and host showcase checks also pass. A rendered selected-Dragoon
fixture was visually inspected, including the adjacent minimap boundary.

Two HUD cache keys add eight BSS bytes; actor/save rows and combat rules are
unchanged. IWRAM ends remain unchanged in the rebuilt race/filming profiles.
BSS ends are Terran `0x0203dbd4`, Z3 `0x02039a60`, P3 `0x02039c08` and
showcase `0x0203dcd8`; all existing reserve guards pass. Default build/tests
(414 Python checks plus host fixtures) and unified build/tests pass. Full Terran build/controller/save/skirmish
checks also pass. This visual update does not qualify a new campaign mission, physical hardware,
ARM SRAM continuation or a new performance result; staged progress stays 22/30.

### Z6 controller final-zone cleanup

After the newer worker and depot economy changes, `make test-sc64-z6-controller`
stalled in source location 5 with a mining Probe and a Pylon left. Kerrigan
stood beside both targets, idle, while single Hydralisk reinforcements walked
in from the main base and ran out the round budget. When at most two Force2
actors remain and no escort is within 320 pixels, the controller now gives
Kerrigan an ordinary attack-move. The final-building Hydra stepping is skipped
on those rounds. When no target is found, the controller re-reads the source
count before deciding the zone is unfinished; previously a stale count
reported a Probe that had already died.

Only controller strategy changed. Victory still requires the result screen,
`won` and the source outcome WON. The host route now wins at host tick 297013,
gameplay frame 293497, with started mask `0x1be` and zero faults. Kerrigan is
alive and unloaded at `(950,1009)`, research is complete and the Overlord has
no cargo. The earlier route won at frame 268694. ARM replay, save continuity
and performance were not re-measured for this route.

### Sampled audio (2026-10-25)

Imported profiles can play StarCraft 64 sound effects, unit responses and
music from a private bank; the default ROM uses PSG placeholder tones for the
same events. The design and import details are in
[Development](DEVELOPMENT.md#optional-starcraft-64-sound-effects-and-music).
Audio is presentation only: it reads simulation state at the start of each
rendered frame, keeps its snapshot outside saves and the game RNG, and never
writes game state.

Host checks:

- `make test-audio` (in `./dev test`): mixer silence, single-voice copy,
  two/three-voice saturation, partial-frame retirement, exact multi-frame
  length, speech interruption, effect allocation, duplicate suppression,
  stealing and refusal, queue bounds and wrap, and the music start, loop,
  one-shot, change, stop and absent-track paths. The hook fixtures cover a
  real skirmish fight producing shot and death events, the cooldown shot
  contract, off-screen silence, the two-effect frame cap, selection, command
  response only on a new key press, new player units, completed buildings,
  and no events after a tick discontinuity or pause.
- The same 12,000-tick controller-driven run from the title screen gives the
  same state hash with the observer compiled in and with `GAME_AUDIO_OFF`:
  `4f8e47da` for the default profile (395 shot, 40 death, 95 selection and
  three command events in the audible build).
- `make test-game-audio-unified` (in `make test-unified`): the same hook
  fixtures with the imported bank, checking that a selection plays one of the
  unit's own response samples and that the race theme and menu theme are
  requested; the unified run's state hash `602d8534` matches with audio off.

Memory, from a full rebuild of every ARM ELF target whose inputs are prepared
(46 of the 55; the T6 storage family needs `make sc64-t6-prepare`, and
`zerg-z5-cloak` already exceeds the IWRAM guard at base `d085310` with the
same `__iwram_end = 0x03005040`):

- `__iwram_end` is byte-identical to the base on every build: the mixer runs
  as Thumb code from ROM and its small ARM interrupt entry is in ROM too.
- BSS grows by 144 bytes with the bank (mixer state, counters and observer
  state) and by 16-24 bytes without it. The tightest builds, the P7 Carrier
  training, projectile and showcase ROMs, keep 660 bytes below the
  `0x0203e000` guard. The mix buffers, a 64-byte zero guard and the
  per-unit/per-building snapshot use the unused VRAM between the mode-4
  pages (416 + 2 x (units + buildings) bytes, at most 1216 of 2560).
- The default ROM stays 64 KiB. Imported ROMs gain the 15.6 MB bank and pad
  to 32 MiB.

CPU, measured in mGBA from the emulated timer around the VBlank handler
(`audio_irq_cycles`, `audio_irq_frames`, `audio_irq_peak`). The unified ROM
sat on the title screen streaming the menu theme while a debugger forced 0-4
voices onto long bank samples for 120 VBlanks each. Cycles per video frame
(280,896 available):

| Active voices | Average | Peak | Share of a frame |
|---|---:|---:|---:|
| 0 (music streaming only) | 1,115 | 1,136 | 0.40% |
| 1 | 1,433 | 1,454 | 0.51% |
| 2 | 6,193 | 7,843 | 2.2% |
| 3 | 8,047 | 9,614 | 2.9% |
| 4 | 9,100 | 11,326 | 3.2% |

Music never costs mixing time because DMA streams it from ROM. At the
experimental P7 mission's ~1.3M cycles per game tick (about 4.6 video
frames), four busy voices cost about 42k cycles per tick. A first version
that added samples one by one cost 18,510 cycles with four voices; reading
four samples per word and repacking unclipped words halved that.

ARM gameplay differential: `tools/sc64_p7_move_workload.py --neutral-ticks
2400 --attack-ticks 1200` ran the experimental P7 move probe built with the
bank and with `SC64_AUDIO=0`, launched through the normal menus, with 2,400
neutral ticks and then a controller attack-move. `--compare` found every one
of the 38 checkpoints identical (unit, building, resource, visibility and
campaign state hashes), with the same 25 inputs and the same end at frame
4105. Measured fps, silent then audible: neutral 10.935 and 10.892 (-0.4%),
attack-move 9.054 and 8.957 (-1.1%). The runner now records the audio
counters: the handler averaged 1,123 cycles per video frame in the neutral
phase and 1,173 in the attack-move phase (0.40-0.42%), peaking at 6,415.

Limits: these are emulator measurements, not physical-GBA results, and the
audio itself was not checked by ear in this run. Direct Sound is 8-bit mono at
10512 Hz, below StarCraft 64's 16-bit stereo 11025 Hz mix. There are four
voices, with no distance attenuation or panning, and at most two new effects
per frame. Annoyed responses, transmissions, briefing audio and advisor lines
other than the funds and supply refusals (added later; see
[Advisor refusal lines](#advisor-refusal-lines-2026-10-28)) are not played. Building completion uses the building's selection sound.
Shots are inferred from a raised cooldown, so a weapon sounds once per
cooldown cycle. Victory and defeat stingers come from the N64 result flag and
have not been confirmed by ear.
### Compressed audio bank (2026-10-27)

The sampled-audio bank is now stored losslessly compressed (codec format 2,
described in [Development](DEVELOPMENT.md#optional-starcraft-64-sound-effects-and-music))
and decoded in the VBlank handler; the mixer, voice policy, music sequencing
and the presentation hooks are unchanged. The decoded PCM is the same signed
8-bit 10512 Hz data as before, except that each clip is padded with silence to
a multiple of eight samples.

Sizes, from the private bank generated with the default predictor policy
(`--predictors 01`):

| | PCM bytes | Encoded bytes | Saved |
|---|---:|---:|---:|
| 501 sound effect clips | 7,417,600 | 4,564,280 | 2,853,320 (38.5%) |
| 13 music tracks | 8,231,520 | 3,798,692 | 4,432,828 (53.9%) |
| Bank (with the 208-byte guard) | 15,648,312 | 8,363,180 | 7,285,132 (46.6%) |

Allowing the second-order predictor as well (`--predictors 012`) saves
7,355,706 bytes but makes most music streams about half again as expensive to
decode, so it is not the default. The experimental P7 move probe's ROM text
shrinks from 20,100,040 to 12,822,924 bytes, and the unified ROM text is 11,804,812 bytes (the previous audio
qualification build on an older base linked 17,766,376), so these images no
longer need the full 32 MiB cartridge.

Correctness:

- `make test-audio` (in `./dev test`) now also runs `build/test-audio-codec`
  (round trips through a plain reference encoder for silence, DC, ramps,
  full-scale squares, noise, random walks, every width and predictor, chunked
  versus whole-stream decoding, exact-size allocations under ASan and
  hand-built byte vectors) and `test-audio-codec-cross`, which encodes
  synthetic PCM with the Python pack tool and decodes it with the C decoder,
  then the reverse, comparing every sample. The mixer checks build their
  fixture bank with the reference encoder and keep every earlier assertion.
- `tools/sc64_audio_pack.py` decodes every stream with its own reference
  decoder before writing the bank and records the PCM hash;
  `--verify-bank` repeats that check on an existing bank.
- `make test-audio-bank` (private, needs the bank) decodes all 514 streams of
  the real bank frame by frame with the C decoder and matched the manifest
  hash `9979d9b9…b185eb10`: the GBA decoder reproduces the previous
  uncompressed bank exactly.
- `tools/sc64_p7_move_workload.py --compare` on the P7 move probe built with
  the compressed bank against the uncompressed baseline: all 14 checkpoints
  identical (unit, building, resource, visibility and campaign state hashes),
  the same 25 inputs and the same end at frame 1705.

Memory, from a rebuild of every ARM ELF target whose inputs are prepared:
`__iwram_end` is byte-identical to the base on every build (the decoder runs
as Thumb code from ROM; IWRAM has 32-270 bytes left in the shipping race
ROMs, so an IWRAM decoder was rejected). BSS shrinks by 12 bytes: the mixer
keeps only a pointer to the five stream decoder states (four voices and
music), which live in the VRAM arena as word and halfword fields. The arena
now holds the mix and music double buffers with their guards, the four voice
scratch slots and the decoder states (1,600 bytes) plus the event snapshot,
at most 2,400 of 2,560 bytes. The showcase ROM, the tightest build, ends BSS
at `0x0203dfc8` (56 bytes under the guard; an earlier draft that kept the
decoders in EWRAM overflowed it by 24 bytes).

CPU, measured in mGBA with the same workload (600 neutral ticks then a 600
tick attack-move on the P7 move probe, menu theme and mission music playing):

| Build | Audio VBlank cycles, average | Peak | Neutral fps | Attack-move fps |
|---|---:|---:|---:|---:|
| Uncompressed bank (base) | 1,134 | 1,719 | 10.603 | 9.646 |
| Compressed, first decoder | 8,805 | 16,397 | 10.262 | 9.277 |
| Compressed, tuned decoder, state in VRAM | 6,328 | 11,486 | 10.348 | 9.384 |

Music costs decode time now (it was free DMA from ROM before), so the audio
handler's steady cost is higher than with the raw bank; the tuned decoder
spends about 30 cycles per decoded sample: 9.8 (first-order) to 11.4
(second-order) Thumb instructions per sample on real streams, fetched
from ROM, with the per-block header at 6 instructions when the width repeats.

Limits: emulator measurements, not hardware; the audio was not checked by
ear in this run (the decoded samples are proven identical, so the mix is the
same as before); a physical cartridge test of the DMA2 double buffer is still
outstanding, as it was for DMA1.
### Advisor refusal lines (2026-10-28)

The imported profiles now play the player race's advisor when the player's
own Build, Train or research command is refused for minerals, vespene gas or
supply. The default ROM plays a short PSG tone for the same refusals.

Identification: every non-empty `014/000` clip (1,143) was decoded to PCM, and
ids 95-218 in full plus 99 other clips up to id 281 were transcribed with
Whisper (`small`, English).
The refusals are `sfxdata` ids 146-154 in three groups of Zerg, Terran,
Protoss: 146 "We require more minerals", 147 "Not enough minerals", 148 "You
have not enough minerals", 149-151 the vespene gas lines, 152 "Spawn more
Overlords", 153 "Additional supply depots required", 154 "You must construct
additional pylons". The neighbours are other advisor lines (116-133 attack,
research and upgrade notices, 155-157 "Not enough energy"). The same
transcription of the nine clips decoded back from the generated bank (8-bit,
10512 Hz) gave the same lines; Whisper renders "vespene" in the three gas
lines inconsistently, but each is the race's gas line by position and wording.
The lines were not checked by ear in this run.

Bank: the nine clips add 210,888 PCM bytes and 126,360 encoded bytes (510
clips; 8,489,540 bytes in all, up 1.5% from 8,363,180). `--verify-bank`
decoded all 523 streams against the manifest.

Behaviour (`starcraft/game_audio.h`, `common/audio_mixer.h`):

- The mineral, gas and supply refusal sites in `game.c` store the toast id in
  a presentation byte; `game_audio_observe()` takes it once per rendered frame
  and drops it after a tick discontinuity. In the race profiles, Train used to
  report every refusal as "Tech prerequisite missing"; `race_queue_unit()` now
  records a funds or supply shortfall, and a refused placement reports the
  first funds problem the placement HUD names, so those toasts also name the
  shortfall.
- An advisor line takes an effect voice at a priority above every effect, so
  combat sounds and unit responses never cut it off and voice 0 stays the unit
  response voice. The mixer refuses an advisor request while any advisor line
  is still playing, so pressing A again during the line neither restarts nor
  overlaps it; the first refusal after it ends plays it again. The
  cooldown is therefore the line itself (1.8-3.0 seconds).

Correctness:

- `make test-audio` (in `./dev test`): the mixer checks add an advisor taking
  the oldest effect voice beside busy speech, refusing a repeat and a second
  line while it plays, effects unable to take its voice, and a replay once it
  has ended. The default hook fixtures drive `queue_unit()` with no minerals
  and at the supply cap: each refusal raises one advisor event, an unchanged
  frame raises none, a repeated refusal raises another, and a discontinuity
  drops a pending cue. The 12,000-tick default run keeps state hash
  `c0b2c7a3` with the observer on and off, as at the base.
- `make test-game-audio-unified` with the advisor bank: the same fixtures
  through `race_queue_unit()` play `sc64_audio_advisor`'s minerals and gas
  clips for the player race. The unified run keeps state hash `441d33b8` with
  audio on and off, the same as the base.
- `make test-sc64-terran-full-campaign` (the `./dev sc64 test --profile
  terran` target) with the advisor bank: every step passes except the T9
  controller run, which reaches its 500,000-tick limit without a Wraith
  (2 minerals) and fails the same way at the base, with a byte-identical
  controller trace. T10 then passes on its own.

Memory, from ARM builds of the base `597ff6b` (with the previous 501-clip
bank) and of this change (with the advisor bank) on the same private inputs:

| Build | `__iwram_end` (both) | `__bss_end` base -> change | ROM text growth |
|---|---|---|---:|
| Unified | `0x03004e58` | `0x0203d3d8` -> `0x0203d3e0` | 126,868 |
| Terran | `0x03004eb0` | `0x0203df2c` -> `0x0203df34` | 127,012 |
| Zerg Z6 | `0x03004ee0` | `0x0203df00` (same) | 126,460 |
| Protoss P7 | `0x03004b68` | `0x0203ddf8` -> `0x0203de00` | 126,956 |
| P7 Carrier training probe | `0x03004ad8` | `0x0203df5c` (same) | |
| Showcase | `0x03004fa8` | `0x0203df60` (same) | |

`__iwram_end` is byte-identical on every build: the refusal sites gain one
store, and the hook runs from `game_render()`. Of each ROM's growth, 126,360
bytes are the clips; the rest is code. Imported ROMs keep their cartridge
size. The audio work per frame is unchanged in bound: an advisor line is
one of the four voices.

Limits: not checked by ear or on hardware in this run, and no CPU
measurement was made (the four-voice worst case measured above still
applies). Only Build, Train and research refusals speak; morph, add-on and
other contextual commands keep their "Tech prerequisite missing" toast.
### Death animations (2026-10-25)

Units and buildings now play death effects instead of disappearing. Deaths
are observed from actor rows that change from alive to dead with HP at or
below zero; the simulation never reads the pool or the alive shadow. The
authored, asset-free renderer follows the StarCraft 64 Death families:

- infantry, Zerg ground units and critters collapse and leave remains;
- vehicles and aircraft explode, with a large blast for heavy units;
- Zerg air units splash;
- Protoss units burst in blue;
- buildings explode and leave rubble.

Timelines use the source frame lengths. Remains last 153 source updates;
rubble is capped at 30 seconds.

The optional source preview compiles 167 actors from the authenticated ROM
(61 image banks, 770,522 pixel bytes). Nine actors keep the authored
fallback because their Death animation draws no frames. The fixture checks
every compiled layer frame. It also confirms that the drawn pixels equal the
clipped imported frames for:

- Marine, Zergling and Dragoon: collapse and remains;
- Vulture and Probe: orange and blue explosion overlays.

Approximations:

- The faintest remap blend band is dropped and the next is dithered,
  because GBA mode 4 cannot blend.
- Remnants are capped at 30 seconds.
- Hallucination puffs, Scourge suicides and spider-mine detonations play no
  death, because those removals keep positive HP.
- A death reaches the pool at most one update late.
- A death and a new unit in the same slot within one scan window would hide
  that death.

Asset-free checks run in `./dev test`: 420 Python checks and the default
fixtures. They cover timing, families, eviction, clipping at every viewport
edge, and real unit and building kills. They also cover expiry, fog,
scripted removals, load re-priming and pool overflow. The same game fixture
passes in the unified, full Terran, Z7 and P7 profiles. Each profile is also
built with `SC64_DEATH_VFX_DISABLED` and runs a combat workload that renders
every frame. Both builds print the same field-wise state digest and the same
SRAM digest after saving. The same holds for the source-art build.

The following pass:

- default build and tests;
- unified build and tests;
- full Terran, Zerg and Protoss profile builds and tests;
- `test-sc64-death-vfx-profiles`;
- `test-sc64-death-source` and `death-visuals`.

Memory: IWRAM ends are unchanged in all 21 rebuilt race, probe and showcase
ROMs. The race pool holds 16 rows of 20 bytes, plus a 50-byte alive shadow.
Race-profile BSS grows by 0x158–0x178 bytes:

| Profile | BSS end |
| --- | --- |
| Terran | `0x0203dd3c` |
| Zerg | `0x02039bc0` |
| Protoss | `0x02039d68` |
| Unified | `0x0203d328` |
| Showcase | `0x0203de50` |
| P7 Carrier training and projectile probes (largest) | `0x0203de54` |

The largest leaves 428 bytes under the reserve guard. The original-art ROM
ends BSS at `0x020043ec` with a 12-row pool. The new code is about 5 KB of
ROM per profile and adds no IWRAM.

Emulator timing comes from controller-only showcase replays. Cycle counters
compare this build with the base `d085310` over the first 60 updates of
each scene:

- The scan adds 6.4k cycles per update, about 1.5% of a dense battle update.
- Drawing adds 1k–19k cycles per frame.
- Effective fps falls 1.3–2.7% in four battle scenes and 5.2% in Base Under
  Siege (22.05 versus 23.27).
- The three last stands stay at 29.857 fps and still end at update 150.

The renderer was profiled and tightened to two fill calls per remnant and a
table-driven ellipse. This is emulator timing, not physical-hardware
qualification. ROMs, generated art and captures remain local.

### End-of-mission outro (2026-10-25)

The result screen now waits for the final deaths. The simulation still stops
on the outcome update (`won`/`lost`, the screen change and saves are
untouched). A presentation-only counter keeps death effects aging over the
frozen battlefield; it is neither saved nor read by input handling. The
result panel appears when every collapse or explosion has finished: at least
24 updates (0.8 s), at most 96 (3.2 s), then a six-update panel opening.

The default and unified game fixtures destroy the opposition and check:

- the outcome arrives on the next update;
- the final structures are exploding after the first result-screen update,
  which scans the final deaths;
- battlefield pixels, not the panel, are shown during the outro;
- the state digest and frame stay frozen through the outro;
- the panel opens after 45 updates (original art) and 53 updates (unified
  Protoss buildings);
- the full panel is drawn afterwards;
- A during the outro restarts at once, as before;
- a load shows the panel immediately.

The P3 frame-coverage oracle renders a mid-outro frame over poisoned buffers
with `SC64_NO_FRAME_CLEAR`, and it matches the clear reference. Both
frame-coverage oracles now render the result screen through the production
`draw_result_screen()` rather than calling `draw_result()` directly. The
feature-off builds print the same outcome frame and the same state and
SRAM digests after 120 frozen result updates. Host captures of a unified
skirmish victory show the final Protoss structures bursting blue and leaving
rubble, then the panel opening. The showcase controller replay still loses
all three last stands at update 150 and restarts from the result screen
with the same inputs.

The outro adds no IWRAM in any of the 21 rebuilt ROMs and 4–8 BSS bytes over
the death-animation branch. The largest BSS end is `0x0203de5c`, 420 bytes
under the guard. Moving the death scan to the start of each update shifts
which effects are on screen during the timing windows.

Controller-only showcase replay effective fps against base `d085310`:

| Scene | Base | With outro |
| --- | --- | --- |
| 0 | 12.800 | 12.665 |
| 1 | 19.529 | 18.714 |
| 5 | 17.965 | 17.526 |
| 6 | 19.476 | 19.062 |
| 7 | 23.270 | 21.785 |

The last stands stay at 29.857 fps and still lose at update 150. This is
emulator timing only.

### P7 mode-0 tile renderer

`SC64_TILE_RENDERER` is a default-off render probe over the movement probe.
The goal was to move terrain and fog onto GBA hardware layers. Profiling
the mode-4 renderer first showed that a still camera already restores a
cached 240x108 background, about 83k cycles per frame. Vertical panning
costs 270-350k cycles, while horizontal panning already stays near 83k. The
minimap costs about 122k cycles every frame in both renderers. Mode 4 cannot
share the screen with tile layers, so the probe moves the whole play screen
to mode 0:

- BG3 terrain: a 32x32 world ring with a 464-slot VRAM cache of 8x8 tiles.
  Tiles stay protected until the next present after their last use.
- BG2 fog: opaque fog and unexplored tiles.
- BG1 static art: minerals, geysers and building art are drawn by the
  unchanged code into the back buffer, then lifted into a double-buffered
  world ring. If its 183 slots run out, the art stays in the software layer
  for that frame.
- Sprites: eight 64x64 sprites carry everything else, drawn by the unchanged
  primitives into a persistent 240x160 EWRAM buffer. That buffer replaces the
  terrain and HUD band caches. Changed 8x8 tiles are copied top to bottom at
  VBlank.
- BG0: the bottom HUD band.

A WIN0 window hides the world layers behind the HUD bands. Palette entry 255
becomes an opaque copy of color 0. The build fails if any linked sprite or
tile pool uses it. Menus, briefings and skirmish maps keep mode 4.

Imported terrain moves into 8x8 pools, flip-deduplicated across maps
compiled under the same conditions (`tools/sc64_tile8_pack.py`). The worst
32x16 window across all 30 imported maps needs 460 unique tiles, which is
checked at build time against the cache. In this build, ROM text falls from
2,967,864 to 2,305,248 bytes (22.3 percent). IWRAM ends at `0x03004ca8` and
BSS at `0x0203dc58`, within the unchanged guards. The mode-4-only blitters
move to ROM in this build only. RaceSave and the actor ABI are unchanged.
After merging sampled audio, death animations, the mission outro and the
canonical race-rendering update, IWRAM ends at `0x03004c60` and BSS at
`0x0203dfdc`, 36 bytes inside the BSS guard. Scene clipping and full-width
band blits from that update stay outside this probe; its builds do not
define `GBA_SCENE_CLIP` or `GBA_WINDOW_BANDS`. Sampled audio keeps its mix
buffers in the VRAM gap between the mode-4 pages, which mode 0 uses, so the
tile probe builds without the imported audio bank and plays the PSG tones.

`make test-sc64-p7-tile` renders 2,095 frames in reference and candidate
ASan/UBSan builds. The session covers menu launch, still play, four 160-update
camera pans, pause/resume, a controller attack-move and 600 combat updates. The
host builds emulate the mode-0 registers, VRAM and OAM. Every frame's RGB555
hash and the final simulation hash match the mode-4 build. A private mGBA
check also compared both ARM ROMs at 17 checkpoints across still play and
four pans: every displayed pixel matched. That check reconstructs the
write-only scroll and window registers from the renderer's own state; the host
test exercises the register writes themselves.

The public workload runner shows no frame-rate gain. Movement probe → tile
probe: 600-update opening 12.694 → 12.632 fps, 2,400 neutral updates 11.188 →
11.156 fps, attack-move 10.055 → 10.060 fps. All checkpoints and inputs match.
During D-pad pans the averages are 10.87/9.96/10.66/9.70 fps for right, down,
left and up, versus 11.08/10.35/10.35/9.50. Draw time falls from about 308k
to 264k cycles on still frames and from roughly 500k to 410k during vertical
pans. VBlank tile copies (about 50-90k cycles) and static-art lifts while the
camera moves take most of that back. Frames that miss early VBlank could tear
in the top rows on hardware; this was not observed or measured. The probe
stays off by default. Its ROM saving is separable: the 8x8 pools could also
feed the mode-4 terrain blitter.


### Blood Bath gas placement, cargo cues, and crash investigation (2026-10-26)

Gas-building placement previously required the cursor to be inside a strict
15-pixel center circle. The imported geyser's offset 56x26 opaque crop extends
outside that circle. The hit test now retains center tolerance and accepts
opaque source-art pixels; gas-building validation, preview, and creation snap
to the same geyser center. Occupied geysers still reject construction.
`make test-sc64-protoss-gas-placement` covers three races at all four Blood Bath
geysers plus a Protoss field-menu build, completion, worker assignment, and
actual gas income under the Showcase flags and host sanitizers. The placement
matrix uses seeded setup; the final case exercises ordinary field controls.

Imported worker sprites did not draw their resource cargo. A small authored
blue/cyan mineral crystal or green gas cue now appears beside harvesting SCVs,
Drones, and Probes while their carry amount is positive. This is an authored UI
cue, not extracted cargo animation. It adds no actor fields or save changes.
`make test-sc64-worker-cargo` checks indexed and packed profiles, resource type,
empty/order clearing, nonworker exclusion, fog, HUD clipping, and unchanged
unit state.

A reported physical-GBA fuzzy-screen crash in Showcase's Blood Bath skirmish
as Protoss versus one AI remains unresolved. The card's last recorded Showcase
source was b2ff82f, predating the blue-orb modifier change. Host sanitizer runs
using both current and b2ff82f source with prepared inputs passed a synthetic
paid-construction stress workload reaching 147/160 building slots; this is not
a byte-identical replay of the shipped cartridge. A private no-GDB ARM test
also passed synthetic construction/rendering with sampled audio active and
canaries after BSS and below the main stack; its modified main loop is not a
production-controller or physical-hardware qualification.

`make test-sc64-protoss-skirmish-pylons` adds a bounded normal-controller check:
HOME/custom menu, Blood Bath, Protoss, one Terran AI, earned resources, two new
Pylons, and 5,400 updates with 5,380 full renders. It passed with three completed
Pylons and normal play still active. It does not exercise slot exhaustion or
prove that the hardware crash is fixed. The exact ROM/save from KODAK and the
freeze/audio behavior are still needed to narrow that report.


## Source weapon artwork qualification (2026-10-26)

The source weapon/attack presentation pipeline is described in
[Native weapon effects](native-weapon-effects.md). Local verification includes
asset-free importer/sampler tests, the integrated 66-profile weapon renderer,
canonical framebuffer/cache comparisons and four ARM ROM links. These checks
do not establish pixel-identical N64 output or physical GBA performance.

The full Terran controller suite currently stops at T9's 500000-update limit.
An isolated build of main commit `cd1b7b8a97553e13b51bd992225ba5b10485a822`
reproduces the same failure as this presentation change: frame 495064, 13
Protoss buildings, 21 Zerg buildings, 13 side units, wallet 26/73066 and zero
reported faults. T1–T8 passed before that stop; T10 was run separately and won
at update 3511 with zero faults. Treat the T9 automated completion qualification
as unresolved rather than describing the whole suite as passing.

The reported physical Showcase Blood Bath/Protoss multi-Pylon crash also
remains unresolved. Host rendering checks and synthetic ARM tests have not
reproduced that exact hardware failure.


## Canonical source death art (2026-10-26)

The Siege Line pink bands reproduced on the PR49 source: at update 60,
suppressing death effects removed all 333 fixed-palette purple pixels while
leaving the same living-unit state. Thirteen authored Zergling remnants in the
original spawn grid formed those bands. They were placeholder corpse puddles,
not surviving unit health bars. The earlier rounded-puddle change had not
resolved clustered deaths.

Canonical Terran, Zerg, Protoss and Showcase now select source death/corpse
art. The authenticated bank covers 167 actors in 61 image banks (770522
indexed pixel bytes); nine unsupported actors retain authored fallbacks.
Fixed waits use exact source ticks; random waits use a deterministic midpoint.
Combat-unit under layers have 891408 bytes of packed word sidecars. Other
actors retain the same source pixels through indexed drawing to keep the
full Terran ROM below 32 MiB. No RAM pool or save schema changed.

`make test-sc64-showcase-death-source` simulates ordinary Siege Line combat
through update 450, requires raw-37 deaths to use source art, compares the
rendered layers with source-frame composites and checks expiry. The packed
source tests compare indexed and packed pixels, including HUD-edge fallback
with and without scene clipping. Private before/after captures were inspected;
they are deliberately absent from this source-only repository.

An owned no-GDB mGBA benchmark ran identical scene-1 inputs for 180 updates,
measuring updates 60–179. All three variants reached the same simulation hash
`2519c37e018c2fe2`, with 14 average / 16 peak active corpses. Repeated results:

| Average cycles | Authored | Source indexed | Source packed |
| --- | ---: | ---: | ---: |
| Update | 336921 | 336616 | 336784 |
| Audio | 126 | 126 | 126 |
| Render | 579214 | 736355 | 651918 |
| Total | 916449 | 1073284 | 989015 |

Packing saves 7.85% of source-indexed total cost, but source art still costs
7.92% more on average than the authored placeholder in this crowded sample.
Peak render cost is 813470 cycles versus 811967 authored (1075219 indexed).
This scene exceeds a 30-fps frame budget in every variant; it remains a
performance target, not a physical-hardware or sustained-30-fps qualification.
Source palette ramps, unsupported script control flow and remnant lifetime
caps remain documented limitations of this importer.

## Z7 full-render winning ARM replay (2026-10-27)

The separate full-render Z7 candidate completed the normal-input winning
tape at host tick 209485/game frame 205764. All 365 host/ARM actor and
game-state digest checkpoints matched. The terminal predicate additionally
required the source victory, a surviving human force, zero source Force 1
buildings, no runtime fault, and no pending action cursors or wait clocks.
See the [full replay audit](audits/2026-10-27-z7-full-win/README.md) for the
exact flag composition, frozen artifact hashes, and measurement scope.

The instrumented combined mean was 5877025 GBA cycles per advancing PLAY
update (about 2.85 rendered updates/second), far below 30 fps. This is not a
physical-hardware measurement. This result supersedes the earlier absence
of a full Z7 ARM win; it does not qualify the seven-mission profile's carried
missions.
Canonical menus and qualified staged progress remain unchanged at 22/30.

The 2026-10-28 standalone Z7 release candidate changes the movement options to
the CONTACT cache and compiles out debugger input storage. Its standalone ARM
link passes with BSS end `0x0203d7e0` and IWRAM end `0x03004d28`; its CONTACT
host replay still matches all 365 frozen checkpoints. The earlier full ARM
win and SRAM continuation used the previous collision flags. This exact
release vector has no full ARM win or fresh-process SRAM qualification. See
the [CONTACT release-candidate audit](audits/2026-10-28-z7-contact-release/README.md).

The same replay with only `SC64_ROUTE_DECAY_FREQ` added also matched all 365
checkpoints through the source win. Its instrumented combined mean was
5617463 cycles (about 2.99 rendered updates/second), a 4.42% reduction from
the baseline. This is a whole-replay emulator comparison, still far below
the performance target; it does not promote a canonical profile. The audit
records both fixed ROM/ELF identities and separate results.

The frequency profile subsequently passed an actual ARM UI-save and
fresh-process HOME-L continuation. Its 16296-byte payload matched the host
save golden; 334 prefix checkpoints and 31 resumed checkpoints passed,
ending in the same source win. Only the inactive menu cursor is normalized
in persistent digests. The audit records source-force checks, the exact
SRAM/prefix/resume identities, and remaining hardware/performance limits.

## Collision coverage for larger flying actors (2026-10-27)

The Z/P bucket query previously searched only nine pixels from the proposed
position. Public actor facts include collidable flying actors with radii above
ten: Guardian (raw 44) has radius 11 and the P5 Battlecruiser (raw 12) has radius
19. A query moving from x=5 to x=6 can miss a blocker in the next cell at x=16
or x=24 respectively, although the full-scan predicate rejects the step.

The query now searches 18 pixels for flying movers and nine for ground movers.
These are the maximum strict-interior distances for the tested air/ground
radius bounds of 19/10. Collision pairs require matching flight state. The
optional bounded steering list remains ground-only; flying movers fall back
to the corrected bucket query. The strict Nydus placement query shares this
coverage. There is no new persistent state or save-format change.

`make test-sc64-collision-bounds` needs no ROM or generated assets. Four
ASan/UBSan variants exercise the actual header with public actor providers:
class cache plus steering list, cache only, fact hoisting, and plain queries.
Each checks more than 1.7 million cases against an independent full scan,
including all 16x16 cell phases, eight directions, speeds zero through three,
tangencies, slot reuse, filters, and strict placement. Bounds assertions cover
the 55 compiled actor types; extending the roster requires rechecking them.

A separate coordinate-culling optimization was rejected. On the corrected
baseline, the paired 2000-update late Z7 ARM replay matched all 20 checkpoints
in both variants, but mean combined cycles increased from 6667463 to 6722515
(0.83%). Both retained 7408 bytes of measured stack margin. This is bounded
emulator evidence, not a full campaign or physical-hardware qualification.
The coordinate-culling gate is not part of this change.

## Z7 packed collision-cache hits (2026-10-27)

The experimental Z7 steering profile enables `SC64_COLLISION_PACKED_HIT`
with its existing class cache. A nonzero cached byte is decoded in the caller,
avoiding a function call and three output-pointer writes on each warm lookup.
Zero bytes and invalid slots still call the original helper; its provider
lookups, unrepresentable-fact outputs, and cache-fill behavior are unchanged.
There is no added cache storage, persistent state, or save-format change.
Canonical release profiles do not enable this gate.

`make test-sc64-collision-bounds` also runs packed cache-only and packed
cache/list variants. They retain the public real-actor scalar oracle and test
cold/warm hits, maximum packed fields, invalid slots without cache writes,
and synthetic radius-64 and all-zero facts. Synthetic providers are local to
the fixture; no product actor facts change. The candidate and baseline full
host replays match all 365 checkpoints and the source win byte for byte.

In a separate paired mGBA replay of the late Z7 save (2000 advancing updates,
20 exact checkpoints), mean update cycles fell from 6227197 to 5374247
(13.70%), and combined update/audio/render cycles from 6667463 to 5815221
(12.78%). Both variants measured 7408 bytes of stack margin. Replay BSS is
unchanged at `0x0203d7a4`; IWRAM end rises from `0x03004c78` to `0x03004d10`.
The ordinary experimental steering-probe link ends BSS at `0x0203dff0`
(16 bytes before the reserve) and IWRAM at `0x03004d10`.

These measurements qualify the bounded workload, not a full ARM campaign
replay with the new gate, physical hardware, or sustained 30 fps. The mean
combined cost is still about 2.9 updates/second at GBA clock speed. The gate
does not promote a campaign chapter or change canonical progress from 22/30.

A separate attempt to postpone steering-list overflow until after filtering
also matched all 365 host checkpoints and all 20 bounded ARM checkpoints,
but increased combined ARM cycles by 2.56%; it remains private and disabled.

The separate Z7 CONTACT candidate has a [frozen ARM win and fresh-process
save audit](audits/2026-10-28-z7-arm-save-win/README.md). It remains below
the near-30-fps target; current-source host coverage and ARM source boundaries
are reported separately.

## Default menus and controls in every race ROM (2026-10-27)

`SC64_FIELD_CONTROLS` moved from the Showcase flags into `RACES_FLAGS`, so
every race-engine ROM uses `starcraft/field_controls.h`: the framed command
list, the HUD reason line, grid placement with a coloured footprint, and the
pause and result panels. All ROMs, including the default one, also gained A
on open ground to move the selection, SELECT to drop it, and placement that
picks the nearest free worker. UI state stays transient; `SAVE_VERSION` and
the campaign save identities are unchanged.

Linker results against base `0de9882` (same shared `.local` inputs):

| ROM | IWRAM end (base → now) | BSS end (base → now) |
| --- | --- | --- |
| Default `StarCraft-GBA` | `0x03002ee8` → `0x03002ee8` | `0x02004434` → `0x0200443c` |
| Unified `StarCraft64-GBA` | `0x03004e28` → `0x03004e28` | `0x0203d3c0` → `0x0203d368` |
| Races | `0x03004e28` → `0x03004e28` | `0x0203d1a0` → `0x0203d140` |
| Terran | `0x03004e08` → `0x03004e08` | `0x0203de94` → `0x0203de34` |
| Zerg Z3 | `0x03004f18` → `0x03004f18` | `0x02039c58` → `0x02039c00` |
| Zerg Z4 | `0x03004f20` → `0x03004f20` | `0x0203c4d0` → `0x0203c458` |
| Protoss P3 | `0x03004c70` → `0x03004c70` | `0x02039e00` → `0x02039da8` |
| Protoss P4 Storm | `0x03004730` → `0x03004730` | `0x0203d490` → `0x0203d420` |
| Showcase | `0x03004fd8` → `0x03004fd8` | `0x0203dfe8` → `0x0203df68` |
| Terran Skirmish (no race engine) | `0x030038f0` → `0x030038f0` | `0x02038f34` → `0x02038f34` |

The race label table used to be a writable array; it is now `const`, which
more than pays for the two new pause bytes and three HUD cache keys. The Zerg
Z6, Protoss P6 and legacy Terran-campaign inputs were not prepared on this
machine, so those ROMs were not linked here.

Host results: 261 of 264 commands from `make test`, `test-unified`,
`test-races`, `test-zerg-z3`, `test-zerg-z4`, `test-sc64-p3`,
`test-protoss-p4-storm` and `test-sc64-terran-full-campaign` pass. T9's
500,000-tick controller limit also fails on base `0de9882` in this
environment. Two Python checks fail on base too (the absent P6 inputs and the
Z8 pack's symlink refusal under a linked `.local`); targets needing P6 inputs
were not run.

Scripted controllers were adjusted for the new input rules: the list reopens
on the last command (a fresh open now starts on Move without extra input),
an R tap selects on release, and A trains only on the selected structure. The
P3 and T5 controllers now check the grid-snapped site and avoid sites that
strand the builder; the P3 controller also aims its sweep from a fighter that
can reach a target. Recorded ARM/mGBA input replays were not rerun and may
need re-recording, because R, A and SELECT now behave differently.

After merging base `7ff622c`, silent builds (`SC64_AUDIO=0`, because that
base requires a codec-format-2 audio bank and the shared bank here is raw)
keep IWRAM identical to the new base and lower BSS: Terran `0x0203df24` →
`0x0203dec4`, Races `0x0203d128` → `0x0203d0c8`, Showcase `0x0203df70` →
`0x0203def0`. `./dev build`, `test-sc64-showcase`, `test-races`,
`test-zerg-z3` and `test-sc64-p3` pass; `test-unified` stops only on the
absent P7 inputs.

## Render caches in spare VRAM and on the second page (2026-10-28)

The canonical Terran, Zerg and Protoss ROM flag sets now keep 68 of the 108
saved terrain rows (48 in the P7 profile, after its projectile arena) in the
OBJ tile VRAM that mode-4 products leave to data, restore the HUD bands from the
page on screen instead of EWRAM copies, maintain the 24x24 minimap raster at
visibility transitions, and compare cached fog rows as bit runs. The host
frame oracle compiled with the Zerg Z1-Z6, Protoss P7 and Terran ROM flags
reports all 22 comparisons pixel-identical, and the promoted-profile,
split-run, contact-cache, HUD-cache, minimap and terrain-cache fixtures pass.

On the Zerg Z1-Z6 ROM (Blood Bath, three opponents, 3,000 neutral ticks,
static camera) drawing costs about 43,000 fewer cycles per frame: the
background stage falls from about 80,000 to 57,000 cycles and the HUD from
about 65,000 to 47,000, with identical simulation state at every checkpoint.
That is 7.7 percent of the 30 Hz tick budget; this opening already ran near
29 fps, so the saving shows in crowded scenes rather than this rate. BSS ends
at `0x02037198` (Zerg) and `0x02037090` (Protoss P7), 28,120 bytes lower than
before, and IWRAM ends fall slightly. These are mGBA measurements, not
hardware results, and no mission qualification changes; see
[the audit](audits/2026-10-28-render-caches/README.md).

Review of the branch found that the first layout placed the cache rows at
`0x06014000`, over the P7 interceptor projectile arena enabled by the same
canonical flags; host fixtures could not see it because each owner had its
own host array. `starcraft/sc64_obj_vram.h` now assigns disjoint ranges with
static assertions, the host backs the whole area with one shared array, and
`tests/sc64_p7_obj_vram.c` (`make test-sc64-p7-obj-vram`, run by
`test-sc64-protoss-p7-canonical`) checks on that shared space that terrain
captures preserve the arena and its encoded packet, that arena writes
preserve the cached rows and the rendered frame, and that an arena placed
over the cache rows is detected; the fixture fails against the old layout.
After the fix, merged with main at `eaf07ca`, the P7 frame oracle and the
Protoss, Zerg and Terran HUD-cache fixtures still pass, the projectile probe
passes, and the P7, Zerg Z1-Z6 and Terran ROMs link inside both guards (P7
BSS end `0x02038258`, IWRAM end `0x03004b30`).

## StarCraft-style selected vital bars (2026-10-28)

Vital bars now follow the StarCraft/StarCraft 64 selection convention as
reproduced by OpenBW's `draw_health_bars`: only selected units and the
selected building show them, below the sprite rather than above it. Each bar is
a black frame of 2-pixel cells split by 1-pixel black dividers, filled in whole
cells. HP is green at 66% or more, yellow from 33% and red below 33%. Shields
are a blue strip stacked above HP, and caster energy is a purple strip one row
below it. Empty cells are grey. The GBA version keeps two shade rows per strip
instead of the reference's three and uses the existing fixed palette entries.
It is a visual approximation of the reference layout, not a pixel-exact
palette or size match. Energy appears only for casters whose energy a campaign
adapter persists: the Z5/Z6 Kerrigan cloak, the P4 High Templar/Tassadar and
P6 Tassadar Storm casters, and the Z6 Kerrigan Storm caster. Bars no longer
carry owner color. Because only player-side actors are selectable, enemy HP is
no longer drawn on the battlefield. Worker cargo badges still draw for every
worker.

`SC64_VITAL_BAR_FAST` is removed. It sped up the always-on two-row bar, which no
longer exists, and drawing now happens only for the current selection. The
asset-free `make test-sc64-vital-bars` oracle now derives every expected pixel
of the new bar independently of the rectangle calls. It covers 6,240 cases
across widths, viewport edges, scene clipping, health tiers, and shield and
energy fills. `make test-sc64-shield-bars` passes under ASan/UBSan in the unified
and P7 profiles. It checks frame and divider pixels, tier boundaries,
shield-only row changes, unselected actors, a selected Pylon, energy cells and
battlefield clipping. The late-building, unit-word spawn, construction-source and
Photon Cannon renderer fixtures now select the actor before comparing vitals.

`./dev build` and `./dev test` (614 Python checks, 18 skipped without private
inputs, plus host fixtures) pass, as does the unified ROM build. Every
`test-unified` binary passes except `test-unified-menu` ("briefing text exceeds
visible page"). That check also fails on an unmodified `codex/source-release`
built from the same private inputs. The full Terran build passes. Its test
command stops at the T9 controller (`won=0`, no Wraiths, wallet 34/14 at the
500,000-tick limit); the unmodified base fails identically with these inputs.
T1–T8 and the other Terran checks pass before that point, and T10 run directly
wins at tick 3,552. No frame-rate, physical-hardware or ARM replay result is
claimed.

### Z7 ARM win and SRAM continuation at runtime base 9ff7f7c (2026-10-29)

These results are historical. They were produced at `9ff7f7c`, and later
shared-runtime merges (#148, #149, #152, #153) leave current-main
qualification open. They come from a replay probe ROM that replaces
`common/main.c`, not from replaying the unchanged production ROM.

On `codex/source-release` `9ff7f7c`, `make test-sc64-z7-canonical-controller`
reproduces PR #123's ordinary-input Z7 trace byte for byte (SHA-256
`2d28b63d74ac4438630561b714444ba77a12a9e79e9b4062133c928c447b8198`, win at tick 821,565 / frame 808,493). The new
`tests/sc64_z7_arm_replay.c` replaces `common/main.c` and compiles the rest
under `ZERG_Z7_CANONICAL_FLAGS`. It plays that trace's 70,561 held-key
transitions from the title screen through the shipped
update/audio/render loop. Nothing writes game state. At each of the
trace's 909 checkpoint ticks, it reports the controller's trace fields to
the mGBA debug log. `tools/sc64_z7_arm_replay.py` builds the tape, runs one
isolated mGBA process per replay (no debugger) and compares every field.
Its verdict fails closed. A run qualifies only if all of these hold:
- the mode's expected checkpoint coverage is non-empty and fully matched;
- any cut is inside the tape;
- there is exactly one coherent final timing record;
- the SRAM records are verified, which happens before the verdict.

Asset-free unit tests cover the negative cases. The executed build commands,
ordered `-D`/`-I` vectors, dependencies and input hashes come from
`tools/sc64_build_provenance.py`. Each rebuilt ROM is byte-identical to its
evidence ROM.

The full ARM replay matches 909/909 and wins. A prefix ROM saves through the
ordinary pause menu at tick 403,482, after 669/669 matches. Its single
SRAM record (generation-36 tag, 16,296 bytes, valid checksum) matches the
host prefix payload byte for byte. A fresh mGBA process loads it with L on
the title screen and matches the remaining 240 checkpoints through the same
win. It writes the victory autosave as sequence 2 and keeps the pause
save. The cut must be a tick where the field UI's unsaved remembered command row
and gesture state equal their post-load defaults. A cut at tick 400,350
replayed the tape into a different command after loading, because the save
does not keep `menu_last`. The full run averages 1,992,668 guest
cycles per frame (about 8.4 frames/s); its peak frame is 56.7 M cycles. The
probe links at IWRAM `0x03004e08` (same as the release ROM) and BSS
`0x0203d818`; the release ROM is at BSS `0x0203d730`.

`make test-zerg-z7` fails on this base for reasons that predate this work.
The Scourge fixture fails under PR #133's human-fog gate. Carried Z1 and Z5
fail under PR #84's field controls, bisected to merge `83b264c` and also
failing in canonical `make test-zerg`. Carried Z6 does not clear Force 2
under Z7 flags but wins under canonical flags. Carried Z2–Z4, the Z7
profile/save/frame fixtures and the 18-match skirmish matrix pass. See the
[audit](audits/2026-10-29-z7-current-arm-sram/README.md). This is emulator
evidence for the shipped game loop, not physical-GBA or 30-fps
qualification. Z7 is not promoted.
## Moving-projectile damage at impact (2026-10-29)

Moving source projectiles now deal damage at their drawn impact frame through
a bounded, saved queue; see [Pending projectile impacts](IMPACT-EVENTS.md).
The candidate is stacked on `761597d`. Base and candidate were built from the
same private inputs.

`./dev build`, `./dev test`, `python3 tools/check_source_tree.py` and the
host SRAM checks pass. `make test-sc64-impact-events-game` passes under
ASan/UBSan for Terran, Zerg, Protoss P7 and Showcase (queue 32; save 32, 10,
11, 32 rows). Its `races` and `unified` vectors keep immediate damage.
`make test-unified` passes. Its `test-sc64-weapon-vfx-game` guard now pins the
compact race `Tracer`.

Five normal 32 MiB ARM products (Showcase, Terran, Zerg Z6/Z7, Protoss P7)
link within unchanged guards. IWRAM is byte-identical, and EWRAM headroom
grows by 56–72 B.

Campaign controllers, run with traces in a private workspace:

- **Terran.** T1–T6, T8, T10 and the save, repair, world-save, target, HUD
  and collision checks pass.
  - T5 now wins at tick 193,011. Unmodified base loses (Raynor dies), so this
    is a changed trajectory, not a claimed fix.
  - T7 (no win at 60,000), T9 (no win at 500,000) and `terran-late-world`'s
    Firebat splash assertion fail identically on base.
- **Zerg Z6/Z7.** The Z6 split controller, promoted profile, Z7 profile, save
  and frame checks pass.
  - The Z7 normal-input controller regresses. It reaches its 1,000,000-tick
    limit without a win, while base wins at 821,565.
  - The impacts-disabled build reproduces base's trace exactly.
  - At frame 47,500 the scripted policy sends its 11 drones to a distant
    mineral field through enemy Sunken Colonies, and its economy does not
    recover.
- **Protoss.** The split controller, P7 OBJ VRAM and P7 save/projectile checks
  pass.
- **Pre-existing on base**, failing identically:
  - the P4 canonical controller (`LOST`);
  - the P4 Storm save assertion;
  - the Z7 Scourge targeting assertion;
  - the generation-30 promoted/storage asserts;
  - the P7 controller rule's link error.

In the no-GDB mGBA probe:

- One shot's timing costs 6,161 cycles, and a full-queue step 1,794.
- The disabled build matches base's digest and update cycles.
- Equal-load windows cost the same, but the stress battle's heavy phase lasts
  longer.

The scene exceeds the 30 fps budget on base as well, so no hardware frame-rate
result is claimed. ARM replays and fresh-process SRAM qualification were not
run; the follow-up below adds them.

### Pending impacts on current default: Z7, ARM replay and saves (2026-10-30)

The candidate was merged with default `cfdbe16`, which includes PR #152.

- `./dev build`, `./dev test`, `python3 tools/check_source_tree.py` and
  `make test-sc64-impact-events-game` pass.
- The five product ROMs link within unchanged guards. IWRAM symbols, sizes
  and end address equal default's, and EWRAM headroom grows by 56–72 B.

**Z7.**
- Default `cfdbe16` itself no longer wins the Z7 normal-input controller: it
  reaches the 1,000,000-tick limit at frame 999,093 with 45 enemy buildings.
  This branch with `-DSC64_IMPACT_EVENTS_DISABLED` reproduces that trace byte
  for byte. With impacts enabled it also reaches the limit.
- A bisect of default's merges puts the regression at PR #161 (`6e1cb67`,
  loaded-worker depot routing). PRs #148, #157 and #158 still win at host
  tick 821,565.
- The earlier impacts-only Z7 difference on `761597d` came from the
  controller's lowest-slot home Hatchery rule, not from the impact path; see
  [the diagnosis](IMPACT-EVENTS.md#z7-controller-diagnosis).
- The Zerg lane owns the controller. Z7 is not gated here, because no current
  build of either kind qualifies a win.

**Unusable controllers.** Two other host controllers fail on default with or
without impacts:
- The canonical P7 controller fails a cursor guard; its Make rule also
  compiles `game.c` in place of the controller, because
  `P7_CAMPAIGN_CONTROLLER` is defined after the rule uses it.
- The P6 controller does not start P6 in the P7 ROM.

**ARM.** No-GDB mGBA runs of four diagnostic ROMs all match their host
oracles:
- An input-only replay of the T10 controller's held keys wins at frame 3,361,
  matching all 35 checkpoints with up to 6 impacts pending.
- Two-process normal-menu skirmish saves restore 5 (Terran) and 4 (P7)
  pending impacts.
- A P7 capacity save first refuses 12 pending impacts, then saves all 11 rows
  and restores them.
- Each restored world equals the saved world, and all continuation
  checkpoints match the first process.

Protocol, hashes and limits are in
[Pending projectile impacts](IMPACT-EVENTS.md#arm-replay-and-fresh-process-saves).
These are emulator results for a probe-entry mission and skirmishes, not
normal-menu campaign or hardware qualification.


### Z8 candidate profile (2026-10-29, in progress)

`make zerg-z8` builds the Z1–Z7 candidate plus Z8 (`SCZ8`, generation 46).
It binds the authenticated Z8 program to the live game through
`starcraft/sc64_z8_runtime_adapter.h`. The program tables come from the
private descriptor (fingerprint `f80b5f9e…`) via
`tools/sc64_z8_program_pack.py`. The ROM links at BSS `0x0203dbf0` / IWRAM
`0x03004f18`. The `zerg-z7` ROM is byte-identical to its base.

`make test-zerg-z8` passes under ASan/UBSan. It launches Z8 through the
ordinary menu and checks the trigger, owner and Nydus binding and the opening
triggers. It also checks a pause-menu save that reloads in a fresh game and
continues identically for 3,000 updates, the rejection of corrupt candidates,
and the hero Dark Templar cloak. The asset-free `make test-sc64-z8-state`
round-trips the 180-byte compact save on an authored program.

The AI backend, Transmission duration, CenterView camera snap and Nexus
kill census are authored, not native. There is no ordinary-input Z8 win, ARM
replay, SRAM continuation or timing result. See the
[audit](audits/2026-10-29-z8-playable/README.md).

### Canonical skirmish controller qualification correction (2026-10-29)

The match targets now expand the actual `TERRAN_ROM_FLAGS`, `ZERG_ROM_FLAGS`
and `PROTOSS_ROM_FLAGS` at execution, with the required companion translation
units. The earlier 54-match report above used chapter flag subsets; it does
not qualify the current canonical products. A dry-run regression checks the
ordered feature/include vectors and catches both early expansion and an older
chapter substituted for a canonical profile.

On runtime base `8d1e653`, the corrected controller passes 27 focused
construction/selection/command-witness cases under host sanitizers. All 54
map/race/opponent configurations were attempted: 51 finish (4 wins, 47 losses),
and three hit the unchanged 120,000-input-tick limit. The Terran product stalls
on imported map 1, human Protoss, three bots; Zerg and Protoss products stall on
map 1, human Terran, two bots. These are failures, not completed matches. No
AI thresholds or terminal rules were changed to produce a pass.

The Terran stall exposes a real resource-return routing defect: a Gateway
beside the Nexus makes its center route unreachable while a point inside the
existing drop-off ring is reachable. Loaded Probes keep their eight minerals
and cannot finish the return trip. A normal-input minimal reproduction fails
on this runtime; shared-engine repair and requalification remain outstanding.
Other stalled configurations still require diagnosis.

The controller now tracks pending travel separately from paid foundations,
observes completion/loss on every input tick, navigates remembered menu rows,
and follows moving workers when selecting them. It emits action witnesses
only for accepted clicks. Matrix failures are retained while the remaining
configurations continue, and stale trace files cannot satisfy a new run.
This host evidence does not establish ARM performance or hardware behavior.


### Shared worker depot-circle routing (2026-10-29)

A loaded worker could fail to route to its depot center even though its
32-pixel drop-off area was reachable. This stalled four mineral carriers in
three canonical matrix cases. Return movement and optional nearest-depot
reachability now use the same complete set of walkable cells intersecting the
existing deposit circle. The imported engine stores one multi-source field per
cached depot in the existing route pool, with a separate goal-key namespace;
ordinary point routes cannot reuse it as a connected-component or suffix hint.
The original-art path uses its existing forward-BFS storage. Cargo amounts,
drop-off distance, order state and save schema are unchanged.

An independent pixel-enumeration/BFS oracle passes 11 sanitizer configurations,
including legacy distance fields, packed fields, partial and split floods,
borrowed grids, active stride, rectangles and 128-by-128 maps. The real Blood
Bath reproduction, mineral/gas trips, a forced long detour, live-waypoint
save/load, depot-slot reuse, lift/land and nearest reachable-depot cases pass.
All eight prepared depot fixture variants pass (four products, normal and
nearest-depot paths). All 54 canonical normal-input matches finish within the
unchanged 120,000-tick limit: Terran 3 wins/15 losses, Zerg 3/15, Protoss 4/14.
The three earlier stalled cases now finish. Canonical startup/save/UI checks
also pass, including 702 skirmish startup/save combinations. The promoted
Protoss fixture had stale P6-only expectations; it now checks P7's exact
save generation 43, identity and seven-entry catalog while retaining P6 checks.
These startup/save checks do not demonstrate full campaign wins.

All four ARM links pass the unchanged guards. EWRAM headroom is 28,480 bytes
for Terran, 28,372 for Zerg, 23,828 for Protoss P7 and 200 for Showcase. IWRAM
headroom is respectively 496, 72, 960 and 56 bytes. Captured actual per-TU
commands, raw dependencies, consumed input hashes, link maps and output hashes
are retained privately. Frozen imported inputs were copied from available
files matching the recorded hashes; some historical source links had been
removed externally. The old failed single-point/eager-scan prototype was
rejected and is not included.

The unchanged production-loop headless replay of the new Terran ROM matches
the completed host trace (loss at frame 9,428) and measures 28.08 effective fps
over 9,426 advancing PLAY frames. A separate late three-AI window measures
20.36 fps over 992 advancing frames, with eight nonadvancing PLAY updates
reported separately. This fails the near-30-fps objective for that workload.
The new and old traces differ, so these are separate workload measurements,
not an isolated patch speed comparison or physical-GBA qualification.

Read-only profiling of that same three-AI replay reproduces all 9,000 timing
rows exactly. Mean frame work is 823,830 guest cycles: update-entry to audio
249,145; audio to render 64; render to next update 574,621 (includes presentation
and loop overhead). The drawing counter averages 480,890 cycles, including
206,532 for the unit/effect phase and 124,372 for background. The unit/effect
phase includes death effects, tracers and projectiles; it does not isolate sprite
compositing. The 27 route-build frames have
the largest update spikes; even zero-build frames average only 21.67 fps.
Rendering is the next sustained-cost target; route construction remains a
separate spike target. Counters overlap and must not all be added together.

### Exact source-animation clock arithmetic (2026-10-29)

The 30 Hz to 24 Hz presentation clock now computes `floor(4*n/5)` as
`n - ceil(n/5)` using one 32-bit division. The inverse computes saturated
`ceil(5*n/4)` as `n + ceil(n/4)` with a checked sum. These identities preserve
the original result for every unsigned 32-bit input, including wrap-domain
ages and the saturation boundary. Animation event timing and duration remain
unchanged. An independent original-64-bit-formula oracle passes 2,062,934
bounded, boundary and random cases; the existing VFX sampling test also passes.
The actual Terran attack-pose call site uses `__aeabi_uidiv` in place of
`__aeabi_uldivmod`; unrelated 64-bit operations remain.

All four ARM products link within the same RAM guards and have unchanged
EWRAM/IWRAM headroom. Actual per-TU compiler/link commands, dependencies,
consumed-input hashes and ROM/ELF hashes are recorded privately. The only
changed consumed source header is the clock helper; imported inputs and
ordered game feature definitions match the paired baseline.

On the identical recorded three-AI workload, the advancing-frame count remains
992 and cycles fall from 817,238,918 to 810,213,225 (0.86 percent), moving the
measured rate from 20.36 to 20.54 fps. All 9,000 tick/frame/phase observations
match. The full one-AI replay also matches all 9,488 observations and decoded
terminal entity banks, still losing at frame 9,428; its 9,426 advancing frames
fall from 5,630,905,715 to 5,627,534,060 cycles (28.08 to 28.10 fps). These are
paired mGBA guest-cycle measurements. The three-AI near-30-fps goal remains
unmet; this is a small improvement rather than a completed performance pass.

A larger transparent-edge facing-sprite experiment was rejected before
promotion. It passed synthetic and real-bank pixel oracles and all four link
guards, but the identical three-AI workload regressed to 20.31 fps. Its static
bank transparency ratio did not establish a useful runtime saving. The source
prototype and failed performance evidence remain private; the original sprite
renderer and generated data format are retained.

### Indexed sprite row alignment (2026-10-30)

The indexed compositor now checks source/destination alignment compatibility
once per row. Advancing both pointers by two preserves their modulo-four
difference, so incompatible rows cannot take the four-pixel word path.
The existing pair compositor, transparency, odd edges and clipping remain
unchanged. Independent randomized framebuffer fixtures pass with source
alignments 0..3, word/non-word variants and vertical scene clipping.

All four retained canonical ARM products pass unchanged RAM guards. EWRAM
is unchanged; IWRAM grows by eight bytes in Terran/Zerg/P7, with respective
488/64/952 bytes remaining. Showcase compiles out this optional word-path
change and its ROM and margins remain identical (200 bytes EWRAM, 56 IWRAM).
Actual ordered per-TU compiler/link argv, dependencies and output hashes are
retained privately; only common/gba.c differs in the paired consumed inputs.

Against the preceding clock-math product, the identical three-AI window
retains all 9,000 tick/frame/phase observations and 992 advancing frames.
Cycles fall from 810,213,225 to 806,838,185 (0.42 percent), about 20.54 to
20.63 fps. The full one-AI replay retains all 9,488 observations and captured
terminal state, with 9,426 advancing frames: 5,627,534,060 to 5,626,410,088
cycles (0.02 percent). These small paired emulator gains do not establish
physical-hardware or broad campaign/near-30-fps qualification.

Read-only primitive attribution reproduces every clock-baseline cycle row.
In that workload, indexed sprites consume 81.6 million cycles, including
65.2 million in source death art; packed facing sprites consume only 6.9
million. A long-lived building-remnant image accounts for most indexed
death drawing. This measured usage motivates a separate remnant-packing
candidate; static transparency counts alone did not justify the rejected
facing-span prototype.

### Packed building remnants (2026-10-30)

The authenticated death importer now also packs persistent remnant layers of
building raw IDs 106..175. This adds six image banks (110, 111, 216, 217, 335,
336), 24 frames and 291,232 phase-word bytes. Transient bursts and unsupported
actors retain their existing paths. Indexed pixels, layer timing, colors,
visibility, lifetimes and the clipped HUD-edge fallback are unchanged.

A fresh baseline generation exactly matches the retained input bank. The
candidate preserves all non-word header content byte for byte and all 780
preexisting packed declarations. Ten importer tests pass. The optional private
bank oracle checks every new frame/phase against indexed bytes and independently
composites 144 patterned framebuffer cases, including scene edges, under
ASan/UBSan. No generated bank or game assets are tracked.

All four retained canonical ARM products link within the unchanged ROM/RAM
limits, with EWRAM/IWRAM margins identical to the preceding row-alignment
candidate. Actual per-TU argv, raw dependencies, consumed input hashes, linker
maps and output hashes are retained privately. Only the expected generated
death-bank header differs in paired consumed inputs; ordered feature/include
vectors remain identical.

The identical three-AI late window retains all 9,000 tick/frame/phase rows and
992 advancing frames. Cycles fall from 806,838,185 to 779,509,400 (3.39 percent),
about 20.63 to 21.35 fps. A separate read-only pixel observer reproduces every
pinned-observer cycle row for both products and all 992 full-frame pixel hashes
match (602 distinct hashes), proving picture parity for this window. Earlier
clock-baseline attribution identified repeated building-remnant image 336 draws;
the pixel observer itself does not identify individual sprite calls.

The full one-AI replay retains all 9,488 observations and decoded terminal
entity/configuration state: 9,426 advancing frames, 5,626,410,088 to
5,623,881,694 cycles (0.045 percent), still about 28.12 fps. These are narrow
paired emulator measurements, not physical-hardware or all-campaign proof.
The late three-AI near-30-fps target remains unmet.

### Reuse source projectile motion within a draw (2026-10-30)

Source trace drawing now computes distance and flight ticks once and reuses
them for duration, projectile placement and matching flight/death children.
The 16-byte automatic record is internal to a synchronous draw. A non-NULL
record requires the same immutable profile and launch/end coordinates;
actor/init overlays and the pre-existing public wrappers request fresh values.
No saved or persistent RAM state, source timelines or pixels change.

An independent copy of the original arithmetic matches 26,896 boundary and
deterministic random cases under ASan/UBSan, including null
outputs, zero distance/speed, stationary/unknown bullet types and unsigned
32-bit ages. Full-int in-flight cases that already overflow the original signed
64-bit product are outside this equivalence claim. A separate authored moving
projectile fixture asserts all 27 original draw events across 40 ages, including
child birth positions and no extra calls after expiry. Existing source VFX
sampling, raster, runtime, alias-resolution and reverse-byte fixtures pass.

Four retained canonical ARM products pass unchanged ROM/RAM guards, with
identical margins to the remnant baseline. Actual per-TU compiler/link argv,
raw dependencies, consumed input/audio hashes and output maps/hashes are
retained. Only the two source runtime/render headers differ in consumed inputs;
ordered feature/include vectors and imported assets match.

Exact-argv recompilation with stack-usage reporting preserves every executable
object section. Compiler static trace/overlay frames shrink across all four
profiles; for example Terran draw_tracers is 888 to 608 bytes and its overlay
helper 328 to 272. The new preparation helper has a 72-byte static frame.
The inspected Terran path through preparation and nonzero 64-bit division
sums to 1,504 bytes including main, game_render and outgoing argument areas,
compared with 1,768 for the corresponding baseline overlay/division path.
These specific paths are not a whole-program stack bound; deeper sprite
helpers, startup and interrupts are not covered. The fixed reserve is unchanged.

The identical late three-AI replay retains all 9,000 tick/frame/phase rows and
992 advancing frames: 779,509,400 to 757,021,353 cycles (2.88 percent lower),
about 21.35 to 21.98 fps. A separate read-only observer reproduces every pinned
cycle row and all 992 full-frame hashes match, with 602 distinct baseline
hashes. The full one-AI replay retains all 9,488 observations and decoded
terminal state, still losing at frame 9,428; 9,426 advancing frames cost
5,623,881,694 to 5,624,163,624 cycles (0.005 percent higher), still about
28.12 fps. This is a mixed workload result, not a universal speedup.
These narrow emulator checks do not establish hardware, all-scene or near-30-fps
qualification; the full goal remains open.

### Custom-game catalog integration checkpoint (2026-10-30)

The current-main/PR147 integration adds shared native production catalogs,
selected-builder availability, paid addon and building-morph paths. Focused
four-profile sanitizer fixtures pass input-driven Factory/Machine Shop/Tank
Mode and Stargate/Scout chains, save/lifecycle and source building-art checks.
The Scourge service uses source pair cost/larva accounting, two-body supply and
slot reservation, air-only target guards and suicide damage. Native Dropship
custom-game boarding, unloading, carrier loss and candidate-only cargo/attachment
save validation are covered where the profile maps its identity. Campaign
validation predicates remain separate. Reaver production is unavailable until
its Scarab lifecycle is implemented; Siege transition and complete roster
services remain open.

The consumed Terran art bank differed from today's importer. Its historical
input was reproduced exactly; current source preparation was audited and found
to flatten modifier-10 shadows as opaque palette pixels, expanding Scout health
bounds by 42 pixels. The corrected generator defers those shadow layers and
records their metadata privately. It restores the body crop and adds the real
Factory body frame. A patterned-framebuffer oracle validates Factory, Machine
Shop and Stargate source-body mapping and field/HUD clipping across four profile
vectors; the old Terran bank fails specifically on missing Factory art. Separate
shadow rendering remains unfinished, and broader bank changes are not claimed
pixel-identical to the old bank.

Default-off SRAM tags are unchanged. The opt-in expanded identity extension
reserves bit31 of its profile-specific base tag, with a compile-time collision
assertion. Runtime fixtures cover both directions of outer/embedded tag rejection,
valid construction/addon/morph/cargo roundtrips and rejection before live-state
mutation. Old and integrated codecs produce identical sampled low-type rows
(9,000 compact and 12,000 P7 typed rows); this is not exhaustive over every
possible payload. Opt-in P7 host proof uses a separately regenerated fact table,
not the default-off canonical artifact.

An early frozen-source ARM link passes unchanged guards in all four canonical
products with EWRAM headroom 28480/28372/23828/200 bytes and IWRAM
496/56/952/56 bytes (Terran/Zerg/P7/Showcase). This is a resource preflight,
not final gameplay/performance or physical-GBA qualification. The frozen-source
controller matrices and sampled timing are recorded below; campaign completion
and fresh-process ARM SRAM qualification remain open. The
full Z7 Egg fixture now supplies its source-required Spire and explicitly
visible target cells; adult-over-Egg and Egg-fallback assertions are retained,
and an unseen-target negative case passes with combat flags on and off.


### Custom-game integration qualification after fact-table repair

The runtime checkpoint is `8a85d6d`; subsequent controller-vector and canonical
P7 Make repairs change verification tooling only. At verification commit
`97531b1`, the full default suite passes: 683 Python checks (16 optional skips),
the additional eight-test suite and host C checks. Source-boundary, doctor and
default build checks also pass. Documentation-only updates follow that tested
commit. The shared Scourge capability
is now part of the generated fact-table fingerprint. Canonical P7 was rebuilt
with table SHA-256
`ed2289496f7587ecd5758931652e6ddd1ff15c4f8184539b0301a22d1e28caf9`.
The prior table is not substituted for this qualification. The new table also
includes the current raw-194 building rectangle; its change is not described as
metadata-only. Terran, Zerg and Showcase do not consume that P7 table; their
retained compiled engine input hashes still match the checkpoint.

The actual Make native gate executes four existing runtime fixtures and five
additional fixtures per product, with production flags and interceptor companion
translation units. The P7 subset was rebuilt against the repaired table. Runtime,
button chains, save validation, Scourge and art pass. Dropship runs pass in Terran
and Zerg; P7/Showcase explicitly skip because their default identity mapping lacks
raw 11. These skips do not establish transport service coverage.

Host controllers reach valid terminal outcomes in all 54 configurations (two
maps, three human races, one through three rotated bots, in each race product):
10 wins, 44 losses, no cap failures. All 702 startup/save configurations and 27
focused construction regressions pass. The P7 18/234/9 subset was rerun with the
repaired table. These are normal-input match tests, not proof that every standard
unit or every campaign can be completed.

Input-only mGBA 0.10.5 replays use the pinned port-free backend, 64-bit guest
cycle clock, separately counted menu/nonadvancing updates, and exact ROM/ELF/trace
hashes. One-AI runs reach the same host terminal state. Three-AI runs below are
nonterminal samples of updates 8,001 through 9,000. The human race matches its
product for these six workloads.

| Product | One-AI advancing frames | One-AI guest cycles | Mean fps | Three-AI sample cycles / 1,000 frames | Mean fps |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran | 9,459 | 5,633,448,297 | 28.17 | 667,976,266 | 25.12 |
| Zerg | 8,623 | 5,082,001,791 | 28.47 | 754,769,344 | 22.23 |
| Protoss P7 | 7,183 | 4,523,842,208 | 26.64 | 645,500,178 | 25.99 |

These workloads differ from the preceding main-branch traces. Their rates are
observations, not paired optimization deltas, stable-30-fps proof, hardware
qualification or coverage of all race/AI compositions.

Additional existing prepared inputs were resolved into an isolated overlay,
without changing earlier frozen banks. The actual canonical Terran target passes
its early mission/save checks but fails to finish T5 at 200,000 ticks. Repeating
T5 on current default `1d864f4` with the same inputs yields a byte-identical trace
(SHA-256 `6553d8e2b0c2373f0fbff8e0b76733d29407a68a47a18327bee3e85ad8c27bdf`)
and the same failure. Transport-engine and Z7 runtime pass; transport-query, P3
Scourge and Z6 transport fixtures fail identical assertions on the two snapshots.
These comparisons classify observed existing failures; they do not make the
aggregate campaign gate pass.

The canonical P7 controller target previously lost its controller prerequisite
by referring to a later Make rule-template name and compiled `game.c` as its
entry point. It now names the controller source as the first prerequisite, and a
Make regression checks the source, dependencies and three companion units. The
corrected actual build links. Its bounded run reaches a cursor-guard failure,
which also reproduces on current default with the same recipe repair and that
base's compatible fact table. The subsequent controller repair is recorded below.

Private executed-argv, raw dependencies, input/output hashes, traces and logs are
retained under `.local/build/skirmish-catalog-{controller-v1,controller-v2,native-final-v1,native-final-v2,arm-preflight-v1,arm-final-v2,runtime-v2,runtime-profiles-v2}`
and `.local/build/campaign-regression-{v1,base-v1}`. Source artifacts only are
committed. No Actions, SD writes or debugger sockets were used. Full roster
services, campaign completion and near-30-fps work remain active.


### Campaign-controller and ARM SRAM follow-up (2026-10-30)

Verification commit `e2a80cb` repairs the existing T5/P7 controllers for field
controls. No production engine source or ROM flags change in this follow-up.
T5 replaces its direct menu-row assignment with D-pad navigation, maintains
home defense during tech/hero operations, and continues the Wraith plan if the
optional Dropship is unavailable. Seven explicit path-grid initialization calls
are replaced with guards that wait for the real simulation's grid. Two final
host runs win at tick 159,216 / frame 158,491 with zero faults, both heroes and
the Command Center alive, and zero enemy buildings. Their traces are identical
(SHA-256 `4ca8aeb7813eb51d3d5206e3761ecdcf6fdcf501b017b1c10c95882094b4282f`).
This uses probe entry and inherited unseen-enemy/map knowledge; it is not a
normal-menu ARM replay or a player-information-limited policy qualification.

P7 closes the field menu before moving its cursor, navigates from the remembered
row, filters unavailable/pending builders and propagates cursor failure without
sending an unintended A press. A route starts from a combat unit near the
combat group's mean, avoiding repeated orders based on a forward straggler.
Two host runs reproduce the same strict win at frame 48,939 / 49,229 calls,
within the unchanged 60,000-call bound, with all three heroes alive. The host
strict verdict checks native outcome, faults, victory conditions and timers.
A port-free normal-menu ARM replay reaches the same frame, RESULT/win and loss
clear, matching slot/owner/raw type/position/HP for all 147 live units and 101
buildings. Guest switch/action-timer internals were not captured; their direct
ARM parity is not claimed. The whole replay costs 127,596,445,835 guest cycles
including menus. A separate 997-advancing-frame window measures about 6.82 fps:
campaign performance remains far below the target despite the completion proof.

The Terran aggregate now passes T5, T6, late-world/save/repair and target checks,
then stops at T7. Independent T7, T8 and T9 controllers do not win at their
unchanged limits; T10 wins. A stale Firebat fixture expected invented radial
collateral that the engine had already removed; the replacement tests direct
invincibility under both supported flags and controlled direct damage, without
claiming the unresolved native Flame Thrower splash geometry. The target
broadphase fixture now recognizes the shared and legacy Z7 Scourge wrapper
configurations. These are verification repairs, not new weapon mechanics.

The headless transport can boot from read-only 32-KiB SRAM and export a new
image through the core API, while memory writes remain limited to the five
scheduler fields. All seven optional real-backend protocol tests pass. A
no-SRAM-option run reproduces all 9,000 original Zerg timing rows and the exact
754,769,344-cycle sample. The recorded SDK rebuild has identical file-backed
Mach-O sections; output-name/signature metadata makes its full file hash differ.

Using the locally packaged canonical candidates, all three native-race one-AI
skirmishes save/load a human foundation with its HP and construction progress
preserved. Separate ordinary-input runs save/load a paid production queue with
its ordered product and progress preserved. A fresh emulator boot with the
engine-written SRAM matches an in-place load at offsets 0, 100 and 1,000,
including full runtime actor-bank hashes. All six cases pass. Nine negative
runs reject corrupted checksums, embedded-version mismatches and outer-version
mismatches without changing HOME's actor banks or the input files. Record tags,
lengths, checksums and embedded versions are checked independently. These cases
do not establish every cargo/morph, mission, pixel or physical-hardware save path.

The final default gates pass on `e2a80cb`: source boundary (1,006 indexed files),
doctor, build, 685 Python checks (17 optional skips), additional eight-test suite
and host C checks. Private evidence is retained in `.local/build/e2a80-final-gates`,
`campaign-regression-v1`, `t5-grid-guard-v1`, `p7-ordinary-arm-v1`,
`skirmish-sram-v2` and `skirmish-sram-queue-v1`. Shared roster/campaign and
near-30-fps work remains open. No Actions, debugger sockets or SD writes were used.


### Visibility-owner short circuit (2026-10-30)

At runtime commit `014c7b8`, the common Arbiter detection helper skips sight-radius
and squared-distance work once a candidate's exact source-owner bits are already
present in the accumulated visibility mask. Existing axis culls, invalid-owner
fallbacks, detector eligibility and owner mapping remain authoritative. No global
cache, capability flag, save field or mission-active predicate is added.

A frozen copy of the preceding linear implementation lives only in
`tests/sc64_p7_owner_mask_game.c`. The prepared P7 and Showcase Make targets pass
2,000 positive randomized cases each, plus explicit human-grid, source-slot,
aliased-owner, duplicate, loaded/hidden, powered-cannon and map-boundary cases.
Showcase explicitly skips the unavailable interceptor-specific identity case.
The 24 native fixture invocations pass (22 actual passes and two intentional
Dropship identity skips). P7's 18/234/9 match/startup/construction subset passes
again with the current generated table.

The winning P7 sample uses 2,442,974,059 rather than 2,453,127,565 cycles over
1,000 updates, a 0.41 percent reduction. The whole 49,229-call winning tape uses
127,213,865,473 rather than 127,596,445,835 cycles, a 0.30 percent reduction,
with the same final win/frame and gameplay snapshot. Across 9,000 prefix updates,
unit/building/P7-state/visibility hashes and the rendered 240x160 framebuffer
hashes match. The native Protoss three-AI sample remains exactly 645,500,178
cycles. These are narrow workload results; the campaign remains near 6.8 fps,
far below the goal.

All four fresh ARM builds record executed per-TU ordered compiler arguments,
raw dependencies, consumed headers/audio, linker arguments/maps and toolchain
inputs. Their formatted ROM code/data match the qualified private candidates
exactly; the unused linear reference was removed from production code before
promotion. EWRAM/IWRAM bounds are unchanged, and the inspected cache-source-tick
static frame remains 1,280 bytes. This is not a whole-program stack bound.

Raw VRAM differences are confined to the audio arena between mode-4 pages at
the inspected checkpoints, not visible frame pages or OBJ VRAM. The initial
music pointer difference is the exact audio-blob relocation; later decoder/PCM
phase differs along with elapsed cycles and interrupt counts. Audio source/bank
hashes match and inspected stream pointers remain in bounds. Bit-identical PCM
or perceptual audio equivalence is not claimed.

The newly packaged candidates pass the six native-race foundation/production
queue fresh-process SRAM cases again. A seventh case saves the ordinary P7
winning tape at call 5,000 / frame 4,966: decoded world fields, the entire
persistent P7 state and campaign state roundtrip, then fresh/in-place loads
match those states and full actor-bank hashes at offsets 0, 100 and 1,000.
Malformed-image rejection remains covered separately. These checks do not prove
an entire resumed campaign or all cargo/morph paths.

Default gates at `014c7b8` pass: source boundary (1,007 indexed files), doctor,
build, 685 Python checks (17 optional skips), additional eight-test suite and
host C checks. The subsequent Showcase-oracle change is test-only and both
prepared oracle targets pass. Private proof lives in `.local/build/p7-owner-*`,
including `p7-owner-final-v1` for per-TU provenance and
`p7-owner-campaign-sram-final-v1` for the mission-state continuation. Full roster,
remaining campaigns and near-30-fps work remain open; no Actions or SD writes.

### T7 and T9 controller repairs (2026-10-30)

With runtime source `5ad0adc`, the repaired T7 controller wins twice at input
call 47,735 / frame 47,101 within the unchanged 60,000-call bound. Both traces
are identical, with zero faults, Kerrigan alive and the emitter delivered to
the native goal. Menu normalization now uses D-pad inputs, route planning waits
for the engine's path grid, and the escort retries the paid Dropship transfer
or ground relay when its carrier dies. The inherited risk planner reads unseen
hostile positions; this is not an information-limited playthrough.

The T9 controller wins twice at input call 45,625 / frame 44,896 within its
unchanged 500,000-call bound. Both traces are identical, with zero faults,
Kerrigan alive, no remaining Protoss buildings and 21 remaining Zerg buildings.
Its policy uses ordinary commands, visible enemies, remembered buildings and
immutable scenario placement hints. Neither controller writes actors, wallets,
mission state or the menu cursor, or forces a path-grid rebuild.

These are host probe-entry checks, not normal-menu ARM, SRAM continuation or
performance qualification. The T7 source SHA-256 is
`d2e67fc965e7031be0a516f607b73c2e2d87816b9d445bacf2da947fce7758c3`,
and the T9 source SHA-256 is
`cba1eeb7878f7e686f8be804d3874dfe3453bd9fc9dfbe881044ca4a61af2d2d`.
Executed compiler arguments, raw dependencies, consumed-input hashes and both
run logs/traces are retained in `.local/build/t7-repair-v3` and
`.local/build/t9-policy-port-v1`. T8 remains the unresolved Terran controller;
the full current aggregate has not yet passed. Runtime rules and limits are
unchanged by these controller repairs.

### Native cargo and building-morph SRAM continuation (2026-10-30)

The qualified `014c7b8` Terran and Zerg products pass three additional
fresh-process ARM continuation cases. Native host fixtures seed the initial
actors, prerequisites, resources and larva, then use the engine's boarding,
paid morph/queue and save paths to create 32-KiB SRAM. ARM loads that image,
saves through the pause menu, and compares an in-place load with a fresh HOME
load. These are fixture-seeded lifecycle checks, not ordinary player acquisition.

- Terran Dropship cargo survives load and unloads through ordinary menu inputs.
  Complete unit/building banks and decoded state match at offsets 0, 55, 100
  and 1,100 scheduler ticks, with the Marine alive and no longer loaded.
- A Zerg Lair saved at morph progress 400 completes on both continuations.
  Complete banks and decoded state match at offsets 0, 100, 1,100 and 2,200
  simulation updates, retaining larva state.
- Hive and Greater Spire morphs complete at the same offsets. A paid Drone
  queue overlaps the Hive morph, completes, and increases the human Drone
  count from four to five. Larva replenish and morph HP reaches the new maxima
  identically in both continuations.

The ARM-resaved images have two valid, independently checked record checksums
with the expected tags and payload sizes. The seed images remain immutable;
guest memory writes are limited to the existing input scheduler. Actual host
compiler arguments, raw dependencies and consumed-input hashes accompany the
seed fixtures in `.local/build/p7-owner-cargo-morph-sram-v1`, together with the
ARM state/bank comparisons. Other cargo types, malformed lifecycle combinations,
whole resumed missions and physical hardware remain outside these cases.

### T8 resource expansion and P7 count optimization (2026-10-30)

The dedicated T8 controller replaces the shared assault runner in the public
T8 Make target; T10 is unchanged. It uses ordinary lift/land, boarding, unloading,
construction, production and attack commands to sustain its economy across
three mineral fields, preserve gas collection and add a second Starport.
Two host probe-entry runs win identically at input call 193,336 / frame 185,476,
within the unchanged 200,000-call bound. Norad remains alive, hostile buildings
reach zero, the native primary objective and completion bit are set, and no
faults occur. The source SHA-256 is
`c8c7eb642f9bbf6e9a8331a5ab5a9cba6c9788a8099a17a012f22ed7580d4146`.
Both traces hash to
`5d17c5ab0be19d89b515aee1f3c4b50dfd0243580dc83f8a01ccd2ffc2a97279`.
Evidence in `.local/build/t8-policy-port-v1` includes the executed compiler
arguments, raw dependencies and 172 input hashes. The inherited policy reads
unseen enemy and mineral positions. This is neither an information-limited
playthrough nor normal-menu ARM qualification. The full aggregate is checked
separately from these individual wins.

The P7 Command census now skips type-match calls for unequal exact raw IDs
and reuses the identical Nexus match for the two applicable owner selectors.
The authenticated trigger-row check, owner/raw validation, kind-aware matching
and fail-closed behavior are unchanged. No cache, persistent storage, mission
predicate or compiler flag is added. A frozen preceding census remains only in
`tests/sc64_p7_count_snapshot_reference.c`. The P7 and Showcase public sanitizer
targets pass explicit nonzero witnesses for all eight count rows, 500 populated
valid worlds, 3,000 mixed valid/malformed cases, wrong-kind raw IDs, inactive and
mismatched contexts, null context and failed-snapshot sentinel preservation.

The complete normal-menu P7 winning tape takes 124,487,769,867 cycles versus
127,213,865,473 on the preceding owner-optimized build, a 2.14 percent reduction.
All 50 checkpoint positions/outcomes and the complete observed final gameplay
snapshot match. Across 9,000 prefix updates, unit/building/P7-state/visibility
and rendered-framebuffer hashes match. The 1,000-update measured window takes
2,394,919,369 versus 2,442,974,059 cycles, a 1.97 percent reduction. The campaign
still falls far short of 30 fps; these results do not establish audio equivalence.

All four production links retain their exact previous RAM bounds. Terran and
Zerg ROMs are byte-identical to the preceding builds; Protoss and Showcase
consume only the intended changed runtime header. Native three-AI samples in
P7 and Showcase remain cycle-identical at 645,500,178 and 693,760,156 respectively.
A new P7 save at update 5,000 / frame 4,966 roundtrips its complete actor banks
and persistent mission state, with fresh/in-place continuations equal through
1,000 updates. Executed per-TU arguments, dependencies, input hashes, maps,
replays and SRAM evidence are under `.local/build/p7-count-{opt,final}-v1` and
`.local/build/p7-count-campaign-sram-final-v1`. Canonical candidates are packaged
locally; no SD delivery or physical-hardware qualification is claimed.

The public `make test-sc64-terran-full-campaign` aggregate passes on the current
runtime and controller sources at `89067f3`, including all ten mission runners,
early and late save/world/repair checks, combat/target/contact checks, and the
234-configuration startup/save matrix. It was built in a fresh private snapshot
with held prepared inputs, actual per-TU compiler arguments and raw dependencies;
all 312 consumed inputs are hashed, and project source/Make inputs match the
committed versions. Evidence is in `.local/build/terran-controllers-final-v1`.
This clears the earlier aggregate controller failures, without turning host
probe entry into an ARM campaign-progression claim. Default source/doctor/build
and tests pass at `ebacc8a` (685 Python checks with 23 optional skips, the
additional eight-test suite and host C checks); the count change additionally
passes its two prepared-profile sanitizer targets and four ARM links. The source
boundary passes all 1,009 indexed files after adding the two new test sources.

The repaired T9 tape also wins through the normal campaign menu on the canonical
Terran ARM executable. A fresh HOME boot uses 13 ordinary launch calls before
the 45,625-call host tape; no SRAM unlock fixture or actor-state writes are used.
At scheduler tick 45,638 / frame 44,896, the guest reaches RESULT/win, primary
objective complete, completion bit 4,096, Kerrigan alive, zero trigger faults,
zero Protoss buildings and 21 Zerg buildings. All recorded host checkpoints
match. A read-only final host observer retains the original input trace and
matches the guest's 192 live units by slot, owner/source owner, runtime/raw type,
position, HP, order and target, and all 42 buildings by slot, owners, types,
position and construction state. Building HP was not captured by that observer.
The port-free replay takes 51,972,781,802 64-bit guest cycles including menus;
it is not steady-30-fps or hardware proof. The controller's scenario-placement
hints remain part of its information assumptions. The Terran ROM is byte-identical
to the current four-profile build. Evidence and exact ROM/ELF/backend identities
are retained in `.local/build/terran-late-new-arm-v1`.

### Nuclear Silo admission guard (2026-10-30)

Final enabled-content review found that a mapped Nuclear Silo could be paid
for despite missing nuclear ammunition/launch services. Commit `6203b45` keeps
its catalog row visible but unavailable. A native Terran fixture supplies the
completed prerequisites, a free add-on site and sufficient funds, then checks
that admission and purchase reject without wallet, selection or actor-bank
mutation. Positive Machine Shop and Control Tower paths still pass. All four
runtime fixture vectors pass; the other three explicitly skip the Silo-specific
fixture because the required native identities are absent.

All four subsequent ARM products retain the same RAM bounds and record their
per-TU arguments, raw dependencies, maps and input hashes. The P7 campaign prefix
matches the count-qualified build at all 9,000 state, pixel and timing rows;
its measured window remains 2,394,919,369 cycles. The Showcase three-AI prefix
matches state/pixels and its measured window remains 693,760,156 cycles, while
51 earlier timing rows differ. The earlier full P7 tape result is not a
full-tape run of these new binaries. Private proof is in
`.local/build/silo-gate-review-v1` and `.local/build/silo-gate-arm-v1`.

The canonical packages in
`.local/releases/20261030-custom-catalog-silo-candidate` match their qualified
builds outside the standard header. No SD write is claimed. Integration remains
draft while other late-tech structures with unavailable actor services are
resolved; neither a mapped identity nor a catalog row is full roster support.

### Shared Shield Battery and bounded hot-loop work (2026-10-30)

The common race-engine capability set now includes Shield Battery services.
All four canonical products use the same raw-172 construction, exact psi power,
energy regeneration, command-driven transfer, owner validation, power-loss and
dead-slot cancellation. The selected Battery shows energy and a Recharge hint.
P7 map properties and source ownership remain authenticated, without widening
its mission-active predicate or writing another mission's persistent union.

New recharge order 15 uses an unused four-bit encoding; legacy P7 order values
are unchanged. Unit shield fractions consistently use 1/1280 points. Save word
26 carries an SH12 marker; markerless non-P7 shield phases are multiplied by
four before live-state replacement, while P7 phases and non-shield regeneration
fields remain unchanged. Markerless empty Battery queues normalize inert stale
cells left by old slot reuse, preserving energy; marked malformed queues stay
invalid. Unknown markers, incompatible orders and malformed identities reject
before replacing the live actors. New-to-old binary downgrade loading is not
promised. Record sizes, actor limits and the reserved stack space are unchanged.

The eight public game/save fixtures pass under sanitizers in Terran, Zerg, P7
and Showcase. The intended legacy P7 Battery fixture passes after replacing its
obsolete immediate-build expectation with actual pending travel and arrival
payment. Native save tests cover partial energy, phase and recharge state,
malformed candidate rejection, marker migration and P5/P6 union isolation.
Imported Battery body pixels and clipping pass in all four art banks; source
construction selects the Protoss warp image/blend data. A full visual review of
the complete construction animation is not part of those art checks.

Two bounded performance changes remove redundant clearing of unreachable owner
list links and scan animation events backward for the original last eligible
frame. Actual owner-list rebuild fixtures pass 10,000 randomized updates in each
profile, including invalid-owner fallback; the frame selector matches its frozen
forward reference in 320,000 sanitizer-backed comparisons. The isolated paired
Zerg three-AI sample improves 1.07 percent before adding Battery behavior.

The combined v3 Zerg map-0/human-Zerg/three-AI workload takes 747,475,403 versus
754,769,344 cycles over 1,000 advancing frames, a net 0.966 percent reduction.
Across 8,928 advancing prefix frames, live unit fields (normalizing the old
shield fraction denominator), complete building banks, visibility, resources
and displayed pixels match. Timing-dependent hardware frame counters are
excluded. This is one busy workload and remains below 30 fps, not a steady-rate
or hardware claim.

All four v3 ARM links pass the unchanged guards with EWRAM/IWRAM margins of
28,480/488 bytes (Terran), 28,372/48 (Zerg), 23,828/952 (P7), and 200/56
(Showcase). No global service pool or clock is allocated. Each native-host-seeded
ARM save resumes an active recharge; fresh/in-place full state and actor banks
match at updates 0, 1, 2, 3, 4, 100 and 1,000, through recharge completion.
These seeds do not establish ordinary player acquisition; native fixtures
separately exercise paid construction and menu controls. P7's 9,000-call prefix
also matches the previous qualified state and pixel digests at every update.

Private evidence lives in `.local/build/shared-battery-arm-v3`,
`shared-battery-sram-v3`, `shared-battery-p7-prefix-v3`,
`battery-public-targets-v1`, `shared-battery-matrix-v1`,
`battery-save-integrated-v3`, `battery-art-review-v1`,
`zerg-visible-vfx-v1` and `battery-zerg-net-v1`. The v3 P7 normal-menu replay
subsequently completed all 49,229 calls and won at frame 48,939, taking
123,808,001,557 guest cycles including menus. All 50 milestone observations and
the observed final snapshot match the previous count candidate. Evidence is in
`shared-battery-p7-full-v3`. The native v1 matrix completed 54 matches (10 wins,
44 losses, zero harness failures), 702 startup/save configurations and 27
construction cases. The v1 campaign gates passed the full Terran host aggregate,
P7 canonical checks and legacy Battery checks; these remain distinct from full
ARM campaign progression and full roster parity.

### Shared-profile guard and Scourge effect corrections (2026-10-30)

Checking the plain unified product exposed two missing capability guards in
the preceding integration. Battery mobility validation referenced the extended
actor-facts API even when that API was absent, and shared Scourge target checks
referenced mission invincibility storage in plain race builds that do not
allocate it. Runtime and candidate-save Battery checks now share a guarded
predicate; its native shielded Protoss identities are movable. The
Scourge check retains invincibility protection wherever its storage exists.

The unified weapon fixture then found a real effect defect: the Scourge burst
mapping was enabled for P3/Z7 but omitted the shared Scourge capability. The
mapping now follows the shared mechanic. The added impact regression checks the
burst profile, shields/armor damage, attacker consumption, supply decrement and
absence of repeated damage. Existing weapon assertions were retained. A separate
plain-profile Battery fixture checks native Zealot eligibility and rejects a
forged Battery target in both runtime and candidate validation, without a skip.
Fresh `make unified` and `make test-unified` pass in the frozen v5 snapshot.
The unified ARM image leaves 3,224 EWRAM and 752 IWRAM bytes before the guards.
Earlier failed snapshots and compiler logs are retained rather than counted as
passing checks.

All four v6 canonical ARM links pass unchanged memory guards. Protoss and
Showcase ROMs are byte-identical to v3, so their v3 SRAM and P7 replay proofs
apply to the same executable bytes. Terran and Zerg changed with the effect
mapping. Their new Battery saves match complete fresh/in-place actor banks and
state at updates 0, 1, 2, 3, 4, 100 and 1,000. These are host-fixture-seeded
saves, not ordinary-acquisition qualification. The Zerg three-AI sample remains
747,475,403 cycles over 1,000 updates (22.45 effective fps); all 8,928 advancing
prefix observations match v3's unit fields, building bank, visibility, resources
and rendered pixels. This broad parity check does not substitute for the
targeted Scourge impact fixtures.

The v2 native matrix again passes all 54 terminal matches across both maps,
all human races and one to three AIs, plus 702 startup/save configurations and
27 construction cases. The four-profile Battery game/save and shared Scourge
fixtures pass all 12 cases. The public VFX profile target passes with its
designated flags. An exploratory invocation with canonical source-effect flags
was incompatible with that fixture's legacy-tracer assertions and is not used
as qualification. No production assertion was relaxed to accommodate it.

The fresh full Terran host aggregate also passes all ten controllers and its
included combat, world, repair, save and startup checks. The final default
doctor/build/test gates pass: 685 Python checks with 23 optional skips, the
additional eight-test suite and the host C checks. These snapshots precede
only a two-line Battery comment correction; no C tokens or line counts changed.
The subsequent v7 four-profile ARM rebuild consumes that final header and is
byte-identical to v6, with unchanged bounds. Default, unified and Terran gate
evidence is in `shared-battery-fallback-final-gates-v2`,
`shared-battery-unified-v5` and `shared-battery-campaign-gates-v2`.

Actual compiler vectors, raw dependencies, input hashes and maps are under
`shared-battery-unified-v5`, `shared-battery-arm-v6`, `shared-battery-arm-v7`,
`shared-battery-skirmish-v2`, `shared-battery-sram-v6`,
`shared-battery-zerg-v6` and `shared-battery-vfx-public-v6`. The four canonical
ROMs in `.local/releases/20261030-shared-battery-profile-fixes` match these
qualified products outside the standard header. No SD delivery, physical
hardware qualification or steady-30-fps result is claimed. The full roster
services and remaining campaign stages still require implementation.

### Shared Bunker service (2026-10-30)

Raw 125 now has a common garrison service across Terran, Zerg, Protoss and
Showcase, with the common capability also enabled in plain race/unified builds.
The authored eligibility/fire-class rules were checked against the existing
pinned BWAPI UnitType facts and OpenBW behavior: four cargo points, same-player
Terran one-space occupants, the occupant's own weapon plus 64 source pixels of
range, and attempted ejection on destruction. The current imported unit set has
no authentic zero-space Terran passenger. Custom live owners, rescued human
campaign identities and distinct nonhuman source owners are checked separately.

The eight frozen native game/save fixtures pass under ASan/UBSan in all four
canonical compiler vectors. They cover missing prerequisites, paid SCV travel
and construction, unfinished-building rejection, five simultaneous board orders
with four arrivals, ordinary contextual boarding/unload, an 80-pixel approach
and a diagonal route around a blocked cell, cooldown/range/immunity, tagged
source-weapon effects, Firebat air exclusion, unarmed SCV/Medic passengers,
transport-command coexistence, successful ejection, fixture-blocked failed
ejection, supply/corpse cleanup, pending cancellation and slot reuse. High-ID
building targets still take damage and save successfully without overflowing
the P7 target cache. Nine malformed candidate cases reject without replacing
the actor banks or wallet. No save schema or actor limit was expanded.

Four additional seeded actual-first-campaign fixtures pass boarding, firing,
unload and save/load while preserving mission phase/objective state. These are
host fixtures with injected actors, not ordinary-acquisition or ARM campaign
playthroughs. A separate plain unified host boarding/save/load round trip passes.
The ordinary-input skirmish matrix again completes all 54 matches (10 wins,
44 losses); every individual result matches the preceding Battery matrix.
All 702 startup/save configurations and 27 construction cases pass too.

Four final v3 ARM links pass the unchanged guards with the same EWRAM/IWRAM
margins as the preceding build: 28,480/488, 28,372/48, 23,828/952 and 200/56
bytes respectively. Fresh/in-place SRAM continuation matches full snapshots
and actor banks through updates 0, 1, 2, 3, 4, 100 and 1,000 in every product.
The native seeds contain three Marines and a Firebat, active cooldowns and a
cached target. Separate fresh-boot ARM runs select the Bunker and unload all
four passengers through ordinary A/B/directional input. No guest actor writes
or edited SRAM payloads are used. The native fixture setup remains distinct
from ordinary acquisition on hardware.

The first ARM stress comparison found an unnecessary spatial-index invalidation
on every building-slot creation. Cleanup now invalidates only actual cargo or
boarding transitions. The corrected v3 Zerg three-AI replay matches all 8,928
advancing unit/building/visibility/resource observations and actual framebuffer
hashes. The observer's pixel record includes a hardware frame counter; an
initial comparison incorrectly counted that counter as a pixel difference.
The displayed 240-by-160 image hashes match, including the first flagged frames.
The measured 1,000-update sample takes 748,598,135 cycles (22.41 effective fps),
about 0.15 percent more than the preceding 747,475,403-cycle build. This is a
small feature overhead, not a performance improvement or a 30-fps result.

One separate native-seeded four-occupant scene averages 29.26–29.61 effective
fps across the four products over 1,000 updates after 100 warmup updates.
There is no paired empty-Bunker control, so it does not isolate service cost
or establish a worst-case bound. P7's 9,000-call normal-menu prefix matches its
preceding unit/building/mission/visibility/framebuffer hashes. A read-only ARM
capture confirms the Bunker capacity/unload HUD fits and loaded infantry bodies
stay hidden; the image alone is not proof of a particular weapon's provenance.
Exact N64 Bunker overlay geometry and original order timing remain separate
fidelity work, as do whole-program stack and physical-hardware qualification.

The full current P7 normal-menu winning tape also passes all 49,229 calls,
ending at frame 48,939 with a win. All 50 milestone observations and the final
observed snapshot match the preceding Battery run. It takes 123,827,664,277
guest cycles including menus, versus 123,808,001,557 before this service
(about 0.016 percent more). This is complete tape coverage, not a steady-30-fps
or all-campaign-progression claim. The pinned runner command, result and
comparison are under `shared-bunker-p7-full-v3`.

The final frozen aggregate passes default doctor/build/test (685 Python checks,
17 optional skips, the separate eight-test suite and C checks), unified ARM
build and host tests, and all ten full-Terran host controllers with their
included checks. The plain unified link leaves 3,216 EWRAM and 600 IWRAM bytes
before the unchanged guards. Source/Make inputs stayed frozen throughout these
gates; commands, per-TU arguments, dependencies and hashes are in
`shared-bunker-final-gates-v1`. These host campaign results are not full ARM
campaign progression.

Private evidence is retained under `shared-bunker-native-v4`,
`shared-bunker-campaign-v3`, `bunker-plain-save-v2`, `shared-bunker-skirmish-v1`,
`shared-bunker-arm-v3`, `shared-bunker-sram-v3`, `shared-bunker-unload-v3`,
`shared-bunker-zerg-v3`, `shared-bunker-occupied-sample-v3`,
`shared-bunker-p7-prefix-v3`, `shared-bunker-p7-full-v3` and
`shared-bunker-visual-v3`.
Compiler arguments, raw dependencies, consumed hashes and maps are captured;
intermediate failed runs are not counted as final qualification. Canonical ROMs
are packaged locally in `.local/releases/20261030-shared-bunker-candidate`.
No SD delivery or Actions run is claimed. The full roster, remaining campaign
stages and near-30-fps goal remain incomplete.

The seeded campaign and plain unified checks were rerun against committed
runtime `b15c582`; their consumed project dependencies match exactly. Earlier
runs before the no-op index fix remain historical evidence. The four ARM
products, native game/save matrix and ordinary skirmish matrix also have
committed-source closure records. The broad default/unified/Terran snapshot
differs from the final tracked tree only in documentation; optional per-TU
dependencies match, while default build/test provenance uses the full source
snapshot rather than a per-TU capture.

Existing read-only stage latches over the same busy Zerg sample identify map
rendering as the largest measured stage (mean 302,434 cycles), followed by
orders (173,311) and HUD (72,515). Map includes background (132,256), building
(9,944) and unit/effect drawing (116,449); these are not additive independent
totals. Render latches describe the latest completed draw and may not align
one-to-one with update entries. The orders maximum is 2,174,022 cycles. Timing
rows are identical to the original v3 sample; no instrumentation build or actor
state writes were used. Distributions and provenance are retained in
`shared-bunker-stage-profile-v3`, to guide the next rendering/spike optimization.

### Shared rendering and depot-flood performance batch (2026-10-30)

The next batch keeps simulation results and rendered pixels unchanged:

- Facing examines only the signed quadrant's nine candidate directions, with
  the original lowest-index tie rule at the north-west wrap. The T5 facing
  fixture compares the actual helper with the original 32-vector search for
  all 16,785,409 coordinate pairs in `[-2048, 2048]` squared.
- The synchronous depot flood temporarily clears each enqueued walkable bit,
  avoiding packed route-field reads for visited checks. Every queued bit is
  restored before return. Seed/neighbor order, route fields and cache policy
  stay the same; custom read-only grid accessors retain the field-based path.
- Canonical race and Showcase renderers compute the static terrain key once
  per draw, using the existing caller-supplied-key API.
- Source effects reuse the image lookup already resolved by the caller when
  locating its frame. The synthetic render fixture now supplies the generated
  frame-directory contract and checks exact pointers against its old lookup,
  as well as the existing moving-projectile and child draw-event sequence.

On the pinned busy Zerg three-AI tape, updates 8001–9000 take 730,928,748
cycles versus the Bunker baseline's 748,598,135: 2.3603% fewer cycles and
22.41 to 22.95 effective fps. Both measurements use the same uninstrumented
64-bit mGBA cycle runner, including audio, rendering and presentation waits.
All 8,928 advancing gameplay-state and framebuffer SHA observations match the
baseline; only the hardware-counter prefix is excluded from pixel comparisons.
This is one nonterminal sample, not a universal or physical-hardware 30-fps
claim. The complete roster, campaign and performance goals remain open.

The frozen combined candidate links all four canonical products without
changing actor limits or RAM reserves. Remaining EWRAM/IWRAM bytes are Terran
28,480/488, Zerg 28,372/48, Protoss 23,828/952 and Showcase 200/56.
Independent exhaustive direction and real-bank frame-pointer oracles, terrain
key/scroll tests, depot BFS variants, motion-reuse and facing-clip checks are
retained locally. The final default gate additionally exposed the standalone
grid-accessor compatibility issue; its repair preserves the read-only accessor
contract instead of requiring new mutable storage.

Private provenance, executed compiler arguments, raw dependencies, consumed
hashes, maps and comparisons are in `shared-render-batch-v1` and
`shared-render-cross-profile-v1`. The rejected terrain row-mask experiment
showed no useful gain. Separate tile-clean and grouped-animation experiments
remain private and are not included in the batch's performance claim.

Final qualification of committed runtime `8d96343` passes default
doctor/build/test (685 Python tests, 17 optional skips, the separate eight-test
suite and C fixtures), unified build/test, the complete Terran host aggregate
and the actual T5 facing fixture's 16,785,409 comparisons. Both synthetic
source-effect fixtures retain their original draw/pointer assertions after
supplying the authored directory metadata; initial failing runs remain
historical evidence, not passes. Final default evidence is
`shared-render-default-gate-v3`; other broad gates and T5 facing are in
`shared-render-final-gates-v2`.

The corrected source's four canonical ARM products are in
`shared-render-cross-profile-v2`. Their game payloads equal the timed combined
candidate's payloads byte for byte, with only normal GBA boot headers and
padding differing. All 54 individual skirmish result JSONs equal the Bunker
baseline. Fresh startup/save and construction checks pass 702 and 27 cases;
four native-seeded garrisons match fresh/in-place ARM state through 1,000
updates; all 9,000 P7 prefix observations match. These are recorded in
`shared-render-skirmish-v2`, `shared-render-bunker-sram-v1` and
`shared-render-p7-prefix-v1`. The full P7 winning run described above predates
this batch; the current batch has prefix evidence, not a new full winning run.

`shared-render-proof-closure-v1` checks consumed project/input hashes,
per-TU commands and exact product links to the replay results. It additionally
verifies the audio assembler blob byte for byte inside each linked ROM.
Updated normal-Make ROMs are retained locally in
`.local/releases/20261030-render-performance-candidate`, with hashes and
header-only equivalence to those qualified products. No SD write or Actions
run occurred. No claim of complete campaigns, full roster or 30 fps is made.

### Follow-up audio, terrain and animation lookup batch (2026-10-30)

The follow-up skips an audio slot only when the live actor is dead and its
previous snapshot is already zero. Death transitions, nonzero stale snapshots,
new/reused actors, construction completion, selection, event admission and
discontinuous ticks keep their original handling. No sound is removed and no
new snapshot storage is allocated. A deterministic 10,000-step differential
matches ordered cues and complete observer snapshots in each canonical profile;
the public hook fixture also checks deaths, dead-slot clearing, reuse and
discontinuities in the default and imported profiles.

The terrain tile test derives bounds from the view already validated by a
successful cache reuse, avoiding repeated validation per tile. Failed reuse
still clears that precondition. Added edge/phase tests cover 49 camera pairs,
queries outside the view and failed reuse. Actual renderer comparisons pass
all four profiles across cold/warm cameras, fog changes, screen transitions,
resource/building mutation, save/load and restart.

Projectile rendering resolves Init, Death and Flight animation records through
one lower-bound search and an adjacent scan. The generated table must remain
strictly ordered by image and animation ID. The real renderer's generated-bank
fixture compares exact pointers with the original lookup for all 65,536 image
IDs, including absent images/phases; authored synthetic tables retain the
existing moving-projectile, child-event and alias-pointer assertions.

The same busy Zerg three-AI sample takes 714,338,530 cycles versus 730,928,748
for the preceding batch: 2.270% fewer cycles and 22.95 to 23.49 effective fps.
All 8,928 advancing gameplay-state and actual framebuffer hashes match. The
combined audio differential also matches the preceding batch. These are
bounded nonterminal observations, not complete campaign, universal 30-fps or
physical-hardware qualification. Actor counts and all RAM reserve guards stay
unchanged; the four canonical links retain their preceding memory margins.

Private measured products, commands and comparisons are in
`shared-render-followup-v1`. Four-profile ARM links, audio comparisons, actual
framebuffer coverage, terrain and animation oracles are in
`shared-render-followup-cross-v1`. Initial malformed harness setups and an
overstrict authored test assumption about other live actors in the default
fixture were corrected and are not counted as passes. The final default,
generated-bank and unified gate evidence is recorded separately under
`followup-final-gates-v1`.

Final follow-up gates pass the actual generated-bank oracle, default build/test
(685 Python tests, 17 optional skips and the separate eight-test suite), plus
unified build/test. The committed-source closure at `3e2ead3` verifies every
consumed canonical project/external input and the audio bytes embedded in each
ROM; the broad snapshot differs only in this validation document. Four
native-seeded Bunker saves match fresh/in-place ARM continuation through 1,000
updates. The current P7 9,000-call ordinary-input prefix matches the preceding
batch for actor, mission, visibility and framebuffer hashes, with no terminal
win claim. Evidence is in `shared-followup-proof-closure-v1`,
`shared-followup-bunker-sram-v1` and `shared-followup-p7-prefix-v1`.

Normal-Make packages with header-only differences from qualified products are
in `.local/releases/20261030-render-followup-candidate`. All four hashes are
recorded; delivery is local to the Mac, with no SD write or Actions run.

The preceding four-optimization batch was separately extracted onto exact
default `1d864f4`, qualified independently and merged as PR #168. Its own busy
Zerg sample improves 751,639,571 to 731,761,211 cycles (22.32 to 22.93 fps),
with 8,948 observed gameplay/framebuffer rows matching. Its 54 ordinary-match
traces match exact default, and default/unified gates and four ARM links pass.
The default full Terran aggregate still fails at the pre-existing T5 controller
cap: an exact-baseline run has the same result, byte-identical trace and 660
matching log lines. The stopped aggregate does not qualify T6–T10. That
standalone failure is not a green campaign gate and must not be conflated with
the earlier repaired-controller qualification on this integration branch.
Private records are in `default-perf-evidence-v1`; the skipped-patch first
attempt was identified and retained as baseline-only, then real candidate
qualification was performed. Merge `2b08ca3` brings the qualified default
change into this branch without changing its source tree at all.

### Bound depot searches to the requesting worker (2026-10-30)

The shared race capability now stops a depot flood once it assigns the
requesting walkable origin. Root and neighbor ordering are unchanged. Such a
field is explicitly incomplete: it can serve an already assigned origin but
cannot prove an unreachable component. Another unassigned origin rebuilds the
same slot from the roots. Blocked origins and the full-field helper still
request a complete flood, preserving the shortest-neighbor choice. All queued
walkability bits, including the unexpanded frontier, are restored before
return. Invalid depot, geometry and origin arguments return before grid writes.
No extra persistent frontier, actor storage or route pool is allocated.

`SC64_DEPOT_BOUNDED` is supplied once by `RACES_FLAGS`, so the same engine
behavior applies to the race, unified, campaign and Showcase products. A
non-race build explicitly rejects the capability; disabled legacy builds stay
supported. The depot oracle covers 18 configurations, including packed and
nibble directions, partial point floods, temporary visited bits, read-only grid
access, rectangular and 128-by-128 maps, blocked origins and cache churn.

The exact final route header (private experiment v3) reduces the busy Zerg
three-AI sample from 714,338,530 to 697,484,133 cycles: 2.3594% less work,
23.49 to 24.05 effective fps. All 8,928 advancing gameplay and framebuffer rows
match. Earlier Terran and Protoss timing experiments used v2 and are not
performance qualification of this final header. This sample remains below
30 fps and does not establish a worst-frame bound or hardware performance.

The exact v3 native comparison matches all 54 ordinary skirmish results and
passes 702 startup/save configurations. Four seeded Bunker SRAM continuations
match fresh and in-place ARM processes through 1,000 updates; the P7 9,000-call
ordinary-input prefix also matches. These do not establish new full campaign
wins. The four canonical ARM links preserve the fixed EWRAM/IWRAM guards;
remaining margins in bytes are Terran 28,480/488, Zerg 28,372/48, Protoss
23,828/952 and Showcase 200/56. Plain race and unified ARM products also link.

Final builds using the actual Make capability definition are byte-identical
to the qualified v3 canonical products. Default doctor/build/test (686 Python
tests, 17 optional skips and the separate eight-test suite), unified build/test
and the full Terran host aggregate pass in the final frozen source snapshot.
The latter qualifies host probes, not ordinary-menu ARM campaign completion.
Source, raw dependencies, actual commands, generated inputs, maps and product
hashes are retained privately under `depot-bounded-v3`,
`depot-bounded-v3-native-v1`, `depot-bounded-v3-four-arm-v1`,
`depot-bounded-v3-runtime-v1`, `depot-bounded-v3-sram-v1`,
`depot-bounded-final-arm-v2`, `depot-bounded-final-generic-v1` and
`depot-bounded-final-gates-v1`. The final closure reports no source drift;
failed capture-wrapper attempts are retained separately and are not passes.

Default PR #169 separately qualifies the preceding audio/terrain/lookup batch
on its own base, with a 23.58-fps busy sample. PR #170 separately repairs the
Terran host controllers, including their retained source-map/hidden-world
planning. Those default results are not substitutes for this integration
branch's different source and generated inputs. Full roster services, all
campaigns, N64 fidelity and near-30-fps performance remain unfinished.

### Default-branch bounded depot searches (2026-10-30)

This batch is independently qualified on default `fcb5195`; it does not
include the separate draft roster/service integration. `RACES_FLAGS` now
supplies `SC64_DEPOT_BOUNDED` across the race-engine products. A depot flood
can stop after discovering the requesting walkable origin. Partial fields
serve only discovered origins, never unreachable-component proofs; another
unassigned origin rebuilds from the roots. Full-field callers and blocked
origins still request a complete field. All queued walkability bits, including
the unexpanded frontier, are restored. No new persistent frontier or pool,
actor-limit reduction or RAM-reserve change is introduced.

Eighteen depot oracle configurations cover packed/nibble directions, partial
point fields, visited-grid borrowing, read-only grid access, blocked origins,
cache churn, rectangular maps and 128-by-128 maps. Capability tests verify
the common flag appears once and reject its use without the race engine.
Default doctor/build/test passes (678 Python tests, 16 optional skips, the
separate eight-test suite and C fixtures), as do unified build/test and the
full Terran host aggregate. The latter remains source-map/hidden-world host
probe coverage, not ordinary-menu ARM campaign completion. All 54 individual
normal-input skirmish results match the earlier exact-default evidence; 702
startup/save configurations and 27 construction cases pass.

The paired busy Zerg three-AI sample decreases from 711,493,203 to 695,201,548
guest cycles over 1,000 updates: 23.58 to 24.13 effective fps, with all 8,948
state and framebuffer observations matching. This is a nonterminal sample,
not steady-30-fps, whole-campaign or physical-hardware qualification. The
four canonical ARM links retain the fixed guards; EWRAM/IWRAM headroom in
bytes is Terran 28,480/488, Zerg 28,372/64, Protoss 23,828/952 and Showcase
200/56. Actual per-TU arguments, raw dependencies, input hashes, ELF/maps/ROMs,
comparison logs and the zero-mismatch source closure are retained privately
in `default-depot-bounded-v1`. The reused baseline runtime inputs are checked
against the current default Git tree and the independently qualified PR #169
products; an earlier mismatching PR #168 comparison is not baseline evidence
for this batch. No proprietary data is committed and no Actions run occurred.

### Reuse resolved source-projectile metadata (2026-10-30)

The shared tracer renderer now unpacks a tagged projectile and resolves its
attack profile once for offscreen culling and drawing. The packed-key wrappers
remain available; malformed keys and absent profiles still produce no effect.
The resolved values live on the stack. No simulation, damage timing, persistent
storage, actor limits or RAM reserves change.

The actual preceding renderer header and the resolved path produce the same
frame digest across 1,410,048 generated-bank cases, including headings, motion
ages, fog, clipping, mirrored poses and child overlays. The fixture reports
344,832 culls, 163,060 drawn cases and digest `403bcc76fe61a883` for both
implementations. The fixture uses a deterministic test compositor; the separate
production replay matches all 8,928 advancing gameplay and actual framebuffer
observations. The numeric capture counter is not treated as a pixel digest.

The same busy Zerg three-AI window takes 693,601,985 cycles instead of
697,484,133: 0.557% less work, 24.05 to 24.19 effective fps. This is one
nonterminal workload, not a universal 30-fps or physical-hardware result.
The public bank fixture checks both wrapper and direct resolved paths,
malformed keys and absent profiles. Private independent-oracle sources retain
the frozen preceding header; wrapper-versus-wrapper checks alone are not
claimed as an independent implementation comparison.

Default doctor/build/test (686 Python tests, 17 optional skips and the separate
eight-test suite), unified build/test and the full Terran host aggregate pass
on the exact final three-file source snapshot. Host-controller information
limits remain unchanged. All four canonical ARM links keep their preceding
memory bounds. The final capture uses the actual Make flags without injecting
an additional depot define; its ROMs are byte-identical to the previously
qualified canonical products. The timed raw Zerg image has the same game
payload as the normally packaged image, apart from the cartridge header and
unused tail padding.

Actual per-TU commands, raw dependencies, source/input hashes, maps, product
comparisons and logs are private in `vfx-resolved-tracer-v1`,
`vfx-resolved-final-gates-v1` and `vfx-resolved-final-arm-v1`. The gate closure
reports no consumed-input drift and no symlinks in its input trees. This stage
does not claim a repeated full P7 win or fresh Bunker SRAM replay; those remain
earlier-stage evidence. Full roster services, all campaigns, broader visual
fidelity and near-30-fps performance remain unfinished.

### Default-branch resolved projectile metadata (2026-10-30)

Independently qualified on default `26a2fec`, this shared rendering change
unpacks a tagged tracer and resolves its attack profile once, reusing those
values for offscreen culling and drawing. Packed-key APIs remain available;
malformed keys and absent profiles retain their no-effect behavior. Values
live on the stack, with no simulation, damage timing, persistent pool, actor
limit or RAM-reserve change. This excludes the draft roster/service work.

The frozen preceding default renderer and the new resolved path match all
1,410,048 generated-bank cases and digest `403bcc76fe61a883`, including
headings, motion ages, fog, clipping, mirrored poses and child overlays.
That fixture uses a deterministic test compositor; the separate production
Zerg replay matches all 8,948 gameplay and framebuffer observations. Its
1,000-update busy three-AI sample improves from 695,201,548 to 691,839,468
guest cycles, or 24.13 to 24.25 effective fps. This is one nonterminal
workload, not steady-30-fps, complete-campaign or hardware qualification.

Default doctor/build/test passes (678 Python tests, 16 optional skips and
the separate eight-test suite), as do unified build/test and the full Terran
host aggregate. Host controllers retain their source-map/hidden-world
information limits. Four canonical ARM builds preserve the fixed guards;
EWRAM/IWRAM headroom in bytes is Terran 28,480/488, Zerg 28,372/64, Protoss
23,828/952 and Showcase 200/56. Actual Make flags are recorded without
injected definitions. Paired consumed inputs differ only in the intended
game and renderer source; generated assets, other inputs and toolchain match
the baseline. Commands, raw dependencies, input/source hashes, maps and
comparisons are private in `default-resolved-tracer-v1`. The earlier #171
54-match, 702 startup/save and 27 construction results are baseline evidence
for unchanged gameplay paths, not new runs of this renderer-only stage.
No Actions were run and no proprietary inputs are committed.

### Source sprite birth offsets and clock domains (2026-10-30)

On terrain integration `d924f54`, source opcode 21 (SPROLUSELO) now uses the
parent image's imported LO offset at sprite birth, including facing and
mirroring. The birth sample uses absolute source-animation time even when
presentation uses time relative to weapon release. Negative relative births
and loop iterations retain that distinction. Older generated banks are
rejected at compile time; regenerate them with the updated importer.

The authenticated bank has six such events bound to five attack profiles:
Hydralisk ground/air, Drone ground and Hunter Killer ground/air. Tests cover
both headings, imported child pixels and offsets, and the source/relative
clock counterexample. The old helper fails that counterexample; the corrected
helper passes. The Drone attack inherits a pre-attack frame unavailable to
this renderer and retains its existing frame-zero fallback. Global sprite
elevation/order, general native VM semantics and exact N64 execution remain
unproven; this is not an all-projectile fidelity claim.

The exact nine source files pass the frozen clock candidate's doctor,
default build/test, unified build/test and full Terran host aggregate. On
the combined terrain/source-offset snapshot, all four canonical ARM products
pass the unchanged reserve guards with captured per-TU dependencies and the
same authenticated generated bank. Cached/fresh rendering matches 512 scenes
per profile with actual engine traces, across the five attack profiles, both
headings and ages 7/8. Separate real-bank child-overlay scenes also match
512 cases per profile and assert that the child changes pixels; the engine
trace comparison alone does not instrument the child draw independently.
All frozen source/input hashes remain unchanged after testing.

Private provenance is in `iscript-event-clock-v1` and
`terrain-clock-combined-v2`, including the negative oracle, bank/profile
composition, four ARM links and scene logs. The preceding terrain-only
timings remain measurements of that stage; no new combined fps or campaign
completion result is claimed. Source only; no Actions or hardware claim.

### Shared terrain window transfer (2026-10-30)

On integration base `81fc21b`, the terrain cache uses a specialized aligned
DMA3 window transfer for horizontal shifts. It may overwrite only edge tiles
that the caller redraws before presentation. The generic window API remains
unchanged; partial rows, unaligned shifts and narrower scene clips use it.
The shared cache serves campaign, custom-game and Showcase products without
adding persistent storage or reducing actor limits or RAM reserves.

The promoted six-file patch matches the frozen v3 source byte for byte.
Its Make-wired sanitizer fixture passes 5,292 clipped-window cases; removing
the narrow-clip fallback reproduces the regression caught during development.
Four prepared-profile scene fixtures each pass 512 camera-phase/direction
cases with fog, construction, mineral depletion and static objects on exposed
edges. The default build/test and source-boundary checks pass. The preceding
v2 runtime also passes doctor, unified build/test and the full Terran host
aggregate. V3 adds the permanent test wiring: its four canonical ARM ROMs,
ordered compiler arguments and consumed inputs match v2 exactly, so that
runtime evidence transfers by identity rather than being claimed as rerun.
All four links preserve the fixed EWRAM and IWRAM reserve guards.

Paired real ARM-emulator custom-game samples, 1,000 updates each, preserve
every observed gameplay/framebuffer row: Terran 8,926, Zerg 8,928 and Protoss
8,927. Guest cycles fall from 613,489,611 to 605,343,334 (27.35 to 27.72 fps),
693,601,985 to 682,929,112 (24.19 to 24.57 fps), and 613,726,043 to 604,206,765
(27.34 to 27.77 fps), respectively. These are busy nonterminal samples,
not full campaign wins, steady 30 fps or physical-hardware measurements.
Initial unguarded prototype measurements are superseded by these results.

Private evidence is in `terrain-transfer-final-v2` and
`terrain-transfer-final-v3`: raw dependencies, executed compiler arguments,
source/input hashes, maps, paired replays, regression and scene logs, and
`final-closure.json` record zero drift and the ROM identity checks. No Actions
were dispatched and no proprietary inputs are committed. Full roster services,
remaining campaigns and broader fidelity/performance qualification stay open.

### Default-branch terrain transfer (2026-10-30)

Independently extracted on `ae691253`, the shared terrain cache transfers aligned
horizontal windows with one bounded DMA3 operation, then redraws exposed edge
tiles. Generic windows, unaligned shifts, partial boundary rows and narrower
scene clips retain their existing path. No persistent buffers, actor limits,
sight ranges or RAM reserves change. The six executable/test files were
qualified at `dfda937`; the later validation addition changes documentation only.

Local source-boundary, doctor, default build/test, unified build/test and the
full Terran host aggregate pass. Sanitizer checks cover 5,292 clipped-window
cases and four prepared profiles with 512 cached/fresh scene comparisons each.
All four normal ARM links preserve the fixed guards; remaining EWRAM/IWRAM
bytes are Terran 28,536/328, Zerg 28,444/64, Protoss 23,900/952 and Showcase
272/56. Captured Make commands, per-TU dependencies and input hashes match the
private input copies. Public-main weapon and P7 fact banks were used; this
extraction does not include the draft roster services or source-birth offset fix.

A separate port-free mGBA probe uses actual production flags and replaces only
main. Over 1,501 play ticks after a normal-menu launch (observed mission field
8), alternating horizontal camera input produces matching selected-state and
displayed-framebuffer rolling hashes at all four checkpoints. Timed update,
audio and rendering work, including presentation waits, falls from 992,093,504
to 983,925,857 guest cycles, a 0.823 percent reduction for this workload. This
is hash-based observational evidence, not byte-for-byte state proof, steady
30 fps or physical-hardware qualification. An initial whole-VRAM hash included
audio scratch/offscreen data and is superseded by the displayed-page probe.

Evidence remains private in `terrain-public-extract-v1`. The Terran host gate
wrote four controller-trace outputs inside its independent copy; all other
75 preexisting prepared files remained identical. No shared inputs were
regenerated, no Actions were dispatched and no proprietary data is committed.


### Shared visibility count spans (2026-10-30)

Qualified on integration runtime `7871191` at private commit `01cc02bf`, this
change writes visibility and its minimap occupancy only when a cell crosses
zero sources, and updates the ninth source-count bit only at 255/256. A
bounded ROM helper applies row spans without repeated per-cell call setup.
Source sight geometry, exploration, source-slot limits, saved state and RAM
reserves are unchanged. Reset clears counts and visibility together before
rebuilding sources, preserving the count/grid invariant used by the fast path.

The exact three adopted source/test files pass independent ASan/UBSan full-grid
reference-count, visible/explored-grid and minimap oracles. Cases include
30-source ownership batches, 250 to 280 to 250 overlap, all-edge out-of-map
centers, empty spans and a 511 to 510 to 255 to zero source sequence. Default
source/doctor/build/test, unified build/test and the full Terran host aggregate
pass on the frozen candidate. Four production-vector ARM builds preserve
all existing bounds: EWRAM/IWRAM spare bytes remain Terran 28,536/328, Zerg
28,444/48, Protoss 23,900/952 and Showcase 272/56. Recorded consumed-input
hashes differ only in the intended header, with no missing dependencies.

A port-free paired T5 replay has 22 nonempty 500-tick windows through tick
11,000. The ownership-transfer sight peak drops from 3,065,139 to 2,122,219
guest cycles (30.76 percent); summed sight work drops from 346,715,658 to
341,592,769 (1.48 percent). Summed full-frame cycles change from 7,613,203,711
to 7,606,083,937, about 0.094 percent less work for this short interval.
The reported checkpoint state/fog digests match. Seven separately measured
palette-resolved 240 by 160 bitmap CRCs at ticks 7,500 through 10,500 also
match. These observations are not every-frame/full-state equivalence, a full
campaign result, general FPS improvement or physical-hardware qualification.
An empty duplicate final timing row is excluded from all aggregates.

Evidence remains private under `sight-span-private-20261030/.local/checks`
and `.local/probe`, with the earlier candidate retained separately. Four
controller-trace test outputs changed during the Terran gate; the other 190
copied inputs were unchanged, with no asset-generation recipes. Source only;
no Actions were dispatched. Battlecruiser/Yamato remains separate experimental
work, and full roster/campaign and near-30-fps goals remain unfinished.

### Default visibility span counts (2026-10-30)

This independently extracted optimization updates cell visibility/minimap
occupancy only at zero-source transitions and the ninth count bit only at
255/256. A bounded ROM row helper avoids repeated per-cell call setup.
Sight ranges, geometry, explored state, source slots, persistent storage
and RAM reserves remain unchanged. Independent full-grid/count/minimap
oracles cover ownership batches, 255/256 transitions, 511-source sequences,
out-of-map centers and empty spans.

Exact visibility commit `673d890` on default `71a134df` passes source/doctor,
default build/test, unified build/test and the full Terran host aggregate.
Current-default integration `e6f821d` on `fdb63f3` passes source/doctor/default
build, focused sanitizer oracles and four actual Make ARM captures plus normal
links. Each profile's source and input closure is verified. Spare EWRAM/IWRAM
bytes are Terran 28,536/328, Zerg 28,444/64, Protoss 23,900/952 and Showcase
272/56. The later base change contains only PR176's generated-map mineral-style
initialization and its fixture assertions. The full gates/timing were not
rerun on that integrated head; the visibility code is identical, the base
change was independently qualified and its new combined ARM links were checked.

The fresh port-free T5 comparison at `673d890` has 22 nonempty timing windows
and 10,991 gameplay ticks. Selected-state/fog checkpoint hashes and seven
palette-resolved framebuffer CRCs agree. Peak sight cost at tick 9,000 falls
from 3,062,662 to 2,122,499 guest cycles (30.70 percent). Summed sight cost is
346,957,909 to 342,107,187 (1.398 percent less); full-frame cost is
7,645,233,405 to 7,638,767,300 (0.0846 percent less). This one shortened
emulator trace does not establish general FPS, all-frame/state equality,
physical hardware behavior or full campaign completion.

Private records are in `visibility-public-v1` and `visibility-fdb-v1`. Four
private controller traces were rewritten by the host gate; other prepared
inputs were preserved. Some completed auxiliary unified/Terran test-build
images were removed during disk cleanup without a prior per-file inventory;
the four captured canonical ARM products, raw dependencies, probe products,
source/input snapshots and test logs remain. No Actions were dispatched and
no proprietary inputs are committed. The later documentation-only commit
does not change the qualified executable/test files.


### Shared integrated cartridge stage (2026-10-30)

Runtime `b3d631c` combines the shared tech-tree/unit services with merged
stock source sight, delayed projectile impacts, terrain DMA, source-birth
overlay offsets, visibility spans and real generated-map mineral styles.
The subsequent default-qualification merge changes documentation only.
The default branch separately received the terrain transfer, canonical resume
recipe repair, generated mineral sprites and visibility spans in PRs 174,
177, 176 and 178. The larger service integration remains a draft.

The integrated predecessor `7871191` passes default build/test, unified
build/test, full Terran host aggregate, four normal ARM guards, six impact
game vectors, four skirmish runtime vectors and twelve terrain/clock scenes.
A fresh 54-match ordinary-input host matrix reaches terminal results across
three products/both maps/all human races/one to three opponents (10 wins,
44 losses). The canonical resume recipe exposed an early Make expansion
defect for P7; the fixed recipes compile all three actual product vectors and
pass 72 menu-driven save/continuation cases. Those match/save runs precede
the later visibility and mineral changes and are not claimed as repeated
on the final cartridge. The visibility candidate separately passes its full
seven gates, four ARM guards and oracles, as recorded above.

All four final `b3d631c` ordinary Make products pass source/command/dependency
closure, reserve guards, 32 MiB header/payload/padding checks and package
readback. Current weapon bank `99efc112` and P7 facts `ed228949` are used;
the final mineral rendering fixtures pass on those actual inputs across
all four products. These checks deliberately do not substitute older
public-main bank qualification for the integration's inputs. EWRAM/IWRAM
spare bytes are Terran 28,536/328, Zerg 28,444/48, Protoss 23,900/952 and
Showcase 272/56. The Mac-only release is
`20261030-sight-minerals-candidate`, with verified ZIP/member hashes and a
scoped manifest. Existing KODAK delivery remains the earlier `66afaff`;
these later ROMs were not copied after the card's safe eject.

Evidence is private in `terrain-clock-main-*`, `shared-current-final-gates-v1`,
`shared-current-skirmish-v1`, `sight-span-package-v2` and
`sight-minerals-package-v1`. The shared source tracker and profile counts
remain partial: Terran has ten campaign menu entries, Zerg six and Protoss
seven. All thirty staged missions, full roster services and near-30-fps
performance are unfinished. Pending-impact save capacity is eleven rows in
P7 and ten in Zerg, with safe refusal if exceeded. Battlecruiser/Yamato
remains private/default-off pending production integration and broader
qualification. No Actions were dispatched and no proprietary assets are
committed.
## T9 ordinary-input qualification (2026-10-29)

This record closes the T9 completion item left open in the 2026-10-26 and
2026-10-28 sections, and only for what it measured. It covers a host
completion with ordinary inputs and a normal-menu ARM replay of the same
inputs in mGBA, on the commits and private inputs listed below. It does not
cover physical hardware, saving and resuming inside T9, or other Terran controllers. The T5, T7 and T8 outcomes below are historical
results on the stated runtime, not claims about current main.

The result applies only to runtime `9ff7f7c`. It does not qualify any later
runtime. To stay mergeable, this branch later merged `codex/source-release` at
`97c39fa` (as `d8985c1`) and then at `ae69125` (as `f0c238b`). That base is
57 commits past `9ff7f7c` and changes 33 files under `starcraft/` and
`common/`. Neither merge nor the later integration with `2847ab0` was re-qualified; the replay
audit below re-checks only the retained `9ff7f7c` logs. T9 needs a fresh host completion and ARM replay on any runtime
that is to be qualified.

Provenance of the qualifying run:

- **Runtime:** `codex/source-release` at
  `9ff7f7c6a8cd93937845203ca69a7ee669f77c58`, merged into this branch as
  `0c754cc44f07039d657132f88c4c7ce2504b2be8`. `git diff 9ff7f7c 0c754cc --
  starcraft common` is empty.
- **Controller, probe and driver:** commit
  `2d575f6509137cd986c04fbe03e2f48aa32a2e83`. The ROMs, trace and tape below
  were built from that tree.
- **Build-provenance capture:** commit
  `70b86da7111c9ce0e838078de4490f1ba719dbbf`. After `2d575f6` the branch adds
  only the capture tool, its test and documentation. The capture rebuilt both
  ROMs byte for byte (see "Build provenance").

### Why the earlier controller stalled

`tests/sc64_t9_campaign_run.c` stopped at its 500,000-tick limit on base
`a4ff09d` (43 Protoss buildings, one Wraith, wallet 42/14). The cause was the
controller, not the engine:

- Its ground-reachability check read the lazily built path grid before the
  engine had built it, and it used 8-connected steps where engine routes use
  four. Every mineral patch and geyser therefore looked unreachable. The
  controller ferried an SCV to the home geyser by Dropship and tried to place a
  second Command Center there, which failed. With a single long gas trip, gas
  stayed near zero and only one Wraith was ever built. The whole T9 map is one
  4-connected ground component.
- Clicking an SCV while another SCV was still selected issued the engine's
  repair order to the selected SCV. When the selected SCV was a builder, that
  cancelled its construction (a Starport stayed at 1 HP).
- The home mineral line holds 11,100 minerals and is mined out after roughly
  20,000 ticks. Idle SCVs then take the nearest patch on the map, in the Zerg
  base, and die there.
- Flyers move on eight headings, diagonal first. A Wraith attack-move from the
  base to the north-east Protoss base clips two Zerg Spore Colonies and
  auto-acquires them. Any Force 3 building death is a native defeat trigger.

### Controller policy

The rewritten controller still enters through the compile-time mission-12
probe and changes the world only through `game_update()` with ordinary keys.
It never writes actors, resources, triggers or mission state, and it does not
force phases or weaken objectives. Victory is the unchanged native trigger
(Force 2 commands no buildings). The native defeat triggers stay live:
Kerrigan's death, losing every unit, and any Force 3 building death.

The policy reads only the player's own units, buildings and wallet; static
terrain, minerals and geysers; and the immutable scenario placement table.
From the placement table it takes the 43 Protoss structure sites as briefing
targets and the 21 Zerg structure sites as no-fly zones. Hostile actors count
only while their cell is in the player's current `visible` grid. Hostile
buildings seen once are remembered at their last seen position until that
cell is visible again without them. Diagnostic output (`--verbose`,
`--final-inspect`, `--map`) may print more, but no input decision depends on
it. Tick 1 is left neutral because on the normal menu the briefing's START
frame runs that tick.

Economy: the opening SCVs walk to the home line and the ordinary idle-worker
rule assigns them patches. The controller builds a Starport, then a Refinery on
the home geyser, and trains SCVs up to 20 while that line lasts. It keeps at
least nine SCVs on minerals and puts up to ten on gas; once the home line is
gone, every SCV goes to gas. Supply Depots are built ahead of demand and a
third Starport follows. Fleet: Wraiths gather east of the mineral line. At 24,
the clump launches as a wave whose membership the controller tracks itself.
Each leg is planned on 32-pixel cells with a 192-pixel exclusion around Zerg
structures and visible Zerg actors, plus a soft cost near visible Dragoons and
remembered Photon Cannons. Waypoints are emitted at heading changes, so each
eight-direction leg is flown as planned. The final leg is an attack-move on
the nearest remembered Protoss building or uncleared briefing site.

### Host completion

```sh
make test-sc64-t9-controller   # writes build/t9-arm/t9-controller.json
./build/test-sc64-t9-campaign-run --trace PATH --verbose
```

The run wins at host tick 51,696 (frame 50,926): all 43 Force 2 buildings
destroyed, all 21 Force 3 buildings intact, Kerrigan alive, zero trigger
faults, phase 3, and the T9 completion bit set. 42 Wraiths and 13 SCVs
survive; the wallet ends at 40/5,850. Two runs produce byte-identical traces
(SHA-256 `8868402d01d8a2d66a5b80d6f2f7d873f6ac8343b8fd817e98378d2e66ed1f74`,
2,807 input transitions). The earlier runtime `a4ff09d` and the qualifying
runtime `9ff7f7c` produce the same trace byte for byte. Before tick 1 was made
neutral, the same policy won at tick 44,292.

### Normal-menu ARM replay

`tests/sc64_t9_arm_replay.c` replaces `common/main.c` in an otherwise
canonical Terran build (`TERRAN_ROM_FLAGS`). The ROM boots, taps A, RIGHT×4, A
and START through the ordinary campaign menu and briefing to mission 12, then
plays the trace's key transitions through the shipped
`game_update()`/`audio_update()`/`game_render()` loop. Observations go to the
mGBA debug log; no debugger port is opened and nothing writes game state.
`tools/sc64_t9_arm_replay.py run` starts one mGBA process with its own HOME,
config and save directory and compares every ARM observation with the host
trace's observation at the same tick.

```sh
make sc64-t9-arm-replay
python3 tools/sc64_t9_arm_replay.py run --out build/emulator/t9-arm-replay
python3 tools/sc64_t9_arm_replay.py audit build/emulator/t9-arm-replay/mgba.log
```

**Result: ARM win.** The qualifying run (hashes below) launched mission 12
from the menu at frame 1 with zero faults and played all 2,807 tape entries.
Every one of its 52 observations (launch, ticks 1,000–51,000 and the final
tick) matched the host trace exactly: frame, screen, win and loss, faults,
phase, objective flags, completion bits, Protoss and Zerg building counts,
Kerrigan, unit counts, both resources and the selection. At tick 51,696
(frame 50,926) the ARM build reports `won=1`, phase 3, objective flag 1,
`race_completed=4096`, 0/43 Protoss Force 2 buildings, 21/21 Zerg Force 3
buildings and Kerrigan alive, identical to the host. The driver reported
`ok: true` after 163 seconds of wall time.

The driver checks the whole probe log, not just its markers. A log qualifies
only with exactly the probe's line order:

1. START
2. the seven menu keys
3. LAUNCH ok
4. one CHECK/CHECK2 pair per observation
5. both timing records
6. DONE

Every line must parse completely, with every field in the probe's order.
Each line is checked against the trace and the probe's schedule:

- START must give the trace's tape length and stop tick.
- The menu keys must be the normal route.
- LAUNCH must report 13 menu frames, one mission frame, the play screen,
  mission 12 and
  `race_mode=0`.
- DONE must have consumed every tape entry.
- Each CHECK2 must carry the tick of the CHECK before it.
- Observations must fall exactly on tick 1, every 1,000 ticks and the stop
  tick.

A repeated tick and kind is rejected rather than merged, as is a periodic and
terminal pair that disagrees at the stop tick. Asset-free tests cover each of
these and the reviewed counterexamples. Those counterexamples are a `T9 LAUNCH
ok` or `T9 DONE` marker without its fields, and conflicting `CHECK2` lines
appended after DONE.

The host trace must also be a complete reference on its own, before any log
is read. Every observation must carry every compared field as an integer.
Periodic observations must fall on every 1,000 ticks through the stop tick,
once each, and every row must be an input, the start, a periodic, a terminal
or one of the controller's labelled checkpoints. Earlier, the comparison
checked only the observations and fields the trace supplied: with the host's
tick-1000 periodic record removed, an ARM `gas=999` there still passed, with
one observation compared. Asset-free tests now cover a missing periodic
record (including at the stop tick), an off-cadence or repeated one, a
missing or non-integer field, an unknown kind, and that counterexample.

`audit` re-ran these checks on 2026-10-30 against the retained evidence. The
host trace (`8868402d…`) loads: 51 periodic observations at ticks
1,000–51,000, the start, two labelled checkpoints and the tick-51,696 win, all
complete. The qualifying log (SHA-256
`82b4f4bf6e7385888fb660297749c5cfac1a4a7f020d40a8c394e5cdb03cb9bb`, 118 probe
lines) passes against it, with 52/52 observations matched and no log or
timing errors. Injecting the reviewed counterexamples into a copy of the log
makes it fail. Removing the trace's tick-1000 periodic record, or the `gas`
field of its tick-2000 one, makes the trace fail to load. That counterexample
needed an edited trace: it does not show a divergence in the recorded run.

Guest-cycle timing comes from the GBA timers (TM2/TM3 at 16,777,216 Hz).
These are emulator guest cycles, not a physical-hardware measurement. The
mission counters start on the briefing's START release, which runs tick 1, so
they cover exactly the 51,696 mission frames. The 13 boot, menu and
START-press frames before it are reported separately (16,837,365 cycles). The
driver rejects a run without exactly one well-formed record of each kind, or
whose mission record does not cover ticks 1..stop, or whose buckets do not sum
to its frames.

| Mission measure (ticks 1–51,696) | Value |
| --- | --- |
| Total cycles | 57,803,662,926 (3,445.4 guest seconds) |
| Mean per frame | 1,118,146 cycles (3.98 VBlank periods; effective 15.0 fps) |
| `game_update()` share | 23,304,360,278 cycles (40.3%) |
| Peak frame | 12,612,714 cycles (44.9 VBlank periods) |
| Frames by rounded VBlank periods (0…6, ≥7) | 0, 0, 3,348, 20,666, 14,019, 7,731, 3,239, 2,693 |

The mean is well above one 280,896-cycle VBlank period. On this build T9 runs
at roughly a quarter of the 60 Hz frame rate in its late, unit-heavy phase.
That is a performance observation, not a qualification failure; performance is
tracked separately.

Superseded runs are not part of this record's evidence, and the stricter audit
rejects each of them:
- A replay on runtime `a4ff09d` (branch commit `10fc402`) also matched 52/52,
  but its timing included the menu frames. It has no separate menu timing
  record, so its log is out of shape.
- Three earlier partial replays were stopped by wall-clock caps and a full host
  disk, so their logs end before the final observation. They had logged mGBA
  at mask 127, which records every DMA. The driver now uses decimal mask 7
  (fatal, error and warn).

### Hashes and inputs

| Item | SHA-256 |
| --- | --- |
| Canonical Terran ROM `build/StarCraft-Terran-GBA.gba` (32 MiB) | `685788ff1909fd532d4d128e18d5bd4c108b13b581c4d248672e9900783954df` |
| Its retained ELF | `5fcb78f02d5c4cce8608c13b1786283328ac8a9a4ea289bae544325f58119a18` |
| Replay ROM `build/StarCraft-Terran-T9-ARM-Replay.gba` (32 MiB) | `3a8986dcf5e71e1c4aef7d984e883904ad80cf2e912ccb8740da40e0f19d5276` |
| Its retained ELF | `0a57e591b1dab8a30426b581006d3f4b54f59b84836ad2b39da07f49a9ef4e49` |
| Host trace `build/t9-arm/t9-controller.json` | `8868402d01d8a2d66a5b80d6f2f7d873f6ac8343b8fd817e98378d2e66ed1f74` |
| Generated tape header | `3a70737c14a909f48752f287854287e96b27a3a427c6f54270315b585eb45a9a` |
| Private T9 scenario header | `e879cf500ce748efff5ede644e4d1790b5cdf9380de9fc95406ca362175cc0bd` |
| Private T9 availability header | `cf4996518f08027a487fc499c188c7855dbf9c77a77b3e9c467f28a1649ab517` |
| Private late-Terran race header | `9231da838e256a48dfd3d06f63708f5f05c9afe2326ccfeceb07e337cc35c0bf` |
| Private late-Terran sprite header | `b7a9f5dde6984567d07e46adabdb3a3bbca4dec1d72c68260bfdd4a39e7b3ae4` |
| Format-2 audio bank (advisor build) | `bd0adfd0ce83c0bb054d40c8f6bc69d91a55ca0f846f7ada56d48d98d7f72427` |

The private inputs come from the StarCraft 64 USA ROM with SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b` and the pinned `sc64-maps`
checkout at `09b57f787a247107d1de82b8c5eab2734667ea1d`. They are prepared
locally and stay out of Git, as do the trace, tape, ROMs and emulator logs.
The replay ROM links within the unchanged IWRAM guard
(`__iwram_end = 0x03004e38 < 0x03005000`) and BSS guard
(`__bss_end = 0x020369b0 < 0x0203e000`) in `common/gba.ld`.

### Build provenance

`docs/provenance/t9-9ff7f7c/` records the executed builds of both ROMs. It was
captured by `tools/sc64_t9_build_provenance.py` from the real `make` recipes
with the compiler driver wrapped, not from a dry run. For each ROM it holds:

- the driver argv and every executed `cc1`, `as` and `collect2` argv, in order
- each unit's ordered `-D` and `-I` options as `cc1` received them, with every
  `-I` ahead of the `-D` options and the driver-added `-D__USES_INITFINI__`
- each unit's raw `-MD` dependency file
- SHA-256 manifests of every consumed source, generated header, audio blob,
  linker script, library and toolchain binary

Its README gives the command and method. The rebuild matched both ROM hashes
above byte for byte. Against the retained ELFs that produced the qualifying ROMs, the
rebuilt ELFs differ in 12 bytes, all in `.strtab`. Those bytes are two local
`FILE` symbols named after the driver's random temporary objects. Every other
section, including code, data, IWRAM and DWARF, is identical. The ELF hash
therefore changes on every build, and the ROM hash identifies the build. The
sorted `make -n` flag-set hash in earlier revisions of this record could not
establish include order or the consumed inputs, and the provenance folder
replaces it.

### Other checks in this environment

All of these ran on the qualifying tree (runtime `9ff7f7c`, branch code
`2d575f6`):

- `./dev build` passes.
- `./dev test` runs 663 Python checks. The only failure is the Protoss
  sprite-roster case of `test_sc64_photon_cannon_renderer`. It cannot find the
  private `sc64_fact_tables_generated.h` in this checkout's inputs, and it
  fails the same way on unmodified base.
- `./dev sc64 build --profile terran` passes.
- `./dev sc64 test --profile terran` passes T1–T4, then stops at the T5
  controller, which loses at tick 111,945. Unmodified `a4ff09d` loses T5 at
  the same tick with the same inputs.
- Run individually past that stop:
  - T6 wins at tick 19,552.
  - The late world, save and repair tests pass.
  - T9 wins at tick 51,696.
  - T10 wins at tick 3,552.
  - T7 does not win by its 60,000-tick limit, and T8 does not win by 200,000.
    Unmodified `a4ff09d` gives identical results for both.

This branch changes neither the runtime nor the T5, T7 or T8 controllers, so
those three results are also the results of unmodified `9ff7f7c`. The
regressions are outside this change and remain open.

No physical-GBA result is claimed.

## Source walk cycles and facing in every race ROM (2026-10-29)

`RACES_FLAGS` now carries `SC64_UNIT_FACING` and the new
`SC64_UNIT_WALK_FRAMES`, so the Races and unified ROMs, every race campaign
chapter and probe, the canonical products and Showcase all draw directional
frames and play source walk poses. `tools/sc64_unit_walk_frames.py` reads each
unit image's IScript Walking loop from the verified ROM. The local bank covers
34 units in 1,260,912 pixel bytes, with all 32 headings stored (the western 15
pre-mirrored). The SCV, Probe and most flyers loop one pose in the source, so
they keep their facing frame and the stride lift. The Siege Tank hull rolls its
tread poses under a level turret.

Evidence, all on the private inputs prepared in this workspace:

- `make test-sc64-unit-walk-frames-game` passes under ASan/UBSan with the
  Terran, Zerg, P7, Showcase, Races and unified compiler vectors. Zergling,
  Marine, Zealot, Hydralisk and Drone each face east (heading 8) and then
  north-west (28) while moving, show every pose of their cycle (7, 9, 8, 7 and
  5) with `draw_unit` output equal to the expected pose frame, and return to
  their facing frame once stopped. Every stored western heading matches its
  mirrored eastern twin, and a moving Tank draws treads with an unlifted turret.
- `make walk-arm-probe` builds a no-debugger Races probe ROM
  (`tests/sc64_unit_walk_arm_probe.c`). In mGBA 0.10.5 with `-l 7` it reports
  `WALK DONE ok`. A Zergling walked 30 frames east facing heading 8, and all 7
  poses were fully present on the shown page. A move to the north-west followed
  the planner's waypoints: north first, then headings 26 to 24 toward the
  target, with the final heading matching the target direction. Once it stopped,
  six frames showed the facing frame.
- All 32 race-engine ELFs that link here keep their `__iwram_end` from base
  `8d1e653`, and BSS grows by at most 16 bytes. The tightest, Showcase, ends BSS
  at 0x0203df40. Text grows by 1.38 MB in products that already had facing and
  by 1.80 MB in those that gain it. P4 Storm also gains the packed facing
  sidecars (+9.1 MB, 21.5 MB total), and the largest ROM, Terran, is 27.9 MB.
  The P6 ROM was not built because its private sprite input is absent here.
- The unit-draw stage of a canonical Zerg build (`sc64_unit_draw_cycles`, CPU
  cycles, mean of 60 frames, 24 Zerglings walking) was measured with a scratch
  probe built with and without `-USC64_UNIT_WALK_FRAMES`. Walking east, it went
  from 99,058 to 136,353 cycles (19.8 Zerglings visible). Walking west, it went
  from 137,341 to 194,650 (24 visible). That is about 1.9k to 2.4k cycles per
  visible walking Zergling more than the packed word sprites. Standing units
  are unchanged. An earlier version that mirrored western headings at draw time
  through the ROM-resident blitter cost about 150k extra cycles walking west,
  which is why the bank stores every heading.
- `./dev build` and `./dev test` pass in a checkout without `.local`, and the
  default ROM hash is unchanged. `make test-unified` and `make test-races` pass.
  The Terran full campaign (T5 qualification, late-world Firebat splash), Zerg
  (Z1 Extractor replay) and Protoss (P6 input missing, P6/P7 save-schema
  asserts) suites fail identically on the unmodified base with these inputs.

Walk poses are presentation only and are not saved. Turn rates, independent
turret tracking and idle flyer animations are not modelled. No physical
hardware or frame-rate claim is made.
