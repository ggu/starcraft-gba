# Validation

This page records reproducible checks for the source snapshot. Automated host checks validate selected rules and data handling; they do not establish complete gameplay balance or physical-hardware behavior.

## Reproducible checks

For the asset-free skirmish, run:

```sh
./dev build
./dev test
```

These checks build the StarCraft ROM and run the host-side rule regressions. For the optional ROM-derived unified profile, first follow [Development](DEVELOPMENT.md#optional-unified-profile), then run `make unified test-unified`. The profile tests cover implemented mission/frontend behavior and save handling, not every possible campaign route.

To validate play in mGBA, use `./dev play starcraft` for the default game or `./dev play unified` after preparing the optional profile. Emulator results are not physical GBA results.

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

## Known limits

The engine is a reduced adaptation with a small authored default map, fixed-size entity pools, simplified combat/pathfinding and faction-specific approximations. See the [game guide](../starcraft/README.md) and [unified profile guide](SC64-UNIFIED.md) for feature limits. Physical GBA hardware testing is pending unless a dated result is added here.
