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
| Latest source (`69f7ea7e`) | 5,000 | 5,000 | 26.91 | Still playing |

The latest build's six observed states match the intermediate winning run at
the same ticks, including entities, resources, campaign state and countdown.
The result-ending interval is excluded from full-run means because the exact
finish point within that interval is unknown. The intermediate build's final
assault still drops to about 19 fps. These measurements do not establish the
latest build's full-mission or physical-hardware performance; T3 stays opt-in.

The latest ARM link uses 388 bytes of data and 246,656 bytes of BSS, leaving
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

## Known limits

The engine is a reduced adaptation with a small authored default map, fixed-size entity pools, simplified combat/pathfinding and faction-specific approximations. See the [game guide](../starcraft/README.md) and [unified profile guide](SC64-UNIFIED.md) for feature limits. Physical GBA hardware testing is pending unless a dated result is added here.
