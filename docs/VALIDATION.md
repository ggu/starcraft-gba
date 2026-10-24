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
The Z3/P3 full candidate replays and optimized Z4 fresh-save checks are
additional qualification work, not implied by those prefix comparisons.

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
