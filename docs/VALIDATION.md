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
signals remain fidelity gaps.

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
