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
committed. The catalog and runtime do not enable T4. Door/trap collision,
teleports, rescue behavior and mission scripting remain to be implemented.

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
configurations. The integrated ARM probe uses 249,304 BSS bytes and 364 data
bytes, ends IWRAM at `0x03004e68` and BSS at `0x0203cf48`, and passes both
unchanged linker assertions. This is an ARM link/storage result, not gameplay
or frame-rate evidence. Unsupported actors and door/rescue/trigger behavior
still prevent T4 playability.

Default build/tests (80 Python checks), unified build/tests and the existing
128×96 storage fixture pass. Default and unified ROM hashes remain unchanged.

## Known limits

The engine is a reduced adaptation with a small authored default map, fixed-size entity pools, simplified combat/pathfinding and faction-specific approximations. See the [game guide](../starcraft/README.md) and [unified profile guide](SC64-UNIFIED.md) for feature limits. Physical GBA hardware testing is pending unless a dated result is added here.
