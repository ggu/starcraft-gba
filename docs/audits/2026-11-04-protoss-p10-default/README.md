# Ten-mission default Protoss ROM

`make protoss` and `./dev sc64 build --profile protoss` now build the P10 product,
`StarCraft-Protoss-P10-Transport1-GBA.gba`. This is a product choice, not a completeness
claim: the mission set grows from seven to ten, and the gaps below still stand.

## What changed

- **Build:** `verify-protoss` and `tools/verify_roms.py --protoss` point at the P10 ROM.
  `protoss-p7` keeps building and verifying the P7 ROM on its own.
- **Tests:** the old `test-protoss` suite is now `test-protoss-p7`. `test-protoss` runs that
  plus `test-sc64-protoss-p10` and `test-sc64-protoss-px`.
- **Preparation:** `./dev sc64 prepare --profile protoss` also generates the P8, P9 and P10
  scenario headers, and the profile's readiness checks require them and P10's map gate.
  Make derives the other P8 to P10 inputs from the source maps.
- **Saves:** default Protoss now uses `SCPA`, generation 47 and
  `.local/play/protoss-p10-transport1`. No save is converted, so existing P7 saves stay in
  `.local/play/protoss-p7-transport1` and load only in explicit `protoss-p7`.

No game code changed.

## Results

Run on merged base `88c17f15` plus this change. Inputs were cloned from the 2026-10-31 pinned
set into a private `.local`, with the codec-2 audio bank. ARM runs use mGBA 0.10.5, one
isolated process per run, log mask 7, no debugger. Timing covers the whole tape, including
menu and briefing frames, at 16,777,216 cycles per second.

| Mission | Win frame | ARM observations matched | Mean cycles per frame | Equivalent fps | Peak frame cycles |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1 | 24,986 | 26 of 26 | 736,602 | 22.8 | 18,789,891 |
| P2 | 34,026 | 35 of 35 | 1,259,803 | 13.3 | 27,808,297 |
| P3 | 48,961 | 50 of 50 | 1,225,343 | 13.7 | 43,788,655 |
| P4 | 136,750 | 138 of 138 | 1,313,233 | 12.8 | 28,632,254 |
| P5 | 5,056 | 6 of 6 | 1,091,505 | 15.4 | 24,156,519 |
| P6 | 99,906 | 102 of 102 | 1,413,690 | 11.9 | 41,291,404 |
| P7 | 48,923 | 51 of 51 | 2,514,927 | 6.7 | 31,740,844 |
| P8 | 96,889 | 100 of 100 | 1,793,323 | 9.4 | 26,441,979 |
| P9 | 150,969 | 155 of 155 | 2,096,826 | 8.0 | 31,740,780 |
| P10 | 72,441 | 74 of 74 | 1,834,877 | 9.1 | 39,610,494 |

Every ARM replay ended in the same win as its host run.

| Check | Result |
| --- | --- |
| P8 fresh-process save on ARM | Prefix 32 of 32, resume 69 of 69, same win |
| P9 fresh-process save on ARM | Prefix 32 of 32, resume 124 of 124, same win |
| P10 fresh-process save, host | 36 of 36, same win. Its ARM run is [#255's](../2026-11-03-p10-ordinary-input/README.md), on an earlier base |
| `make protoss`, `make protoss-p7` | Link and verify |
| P10 `__bss_end` / `__iwram_end` | `0x020394f4` / `0x03004a58`, below `0x0203e000` / `0x03005000` |
| P7 `__bss_end` / `__iwram_end` | `0x0203838c` / `0x03004a28` |
| `python3 -m unittest tests.test_sc64_dev` | Pass, 45 tests |
| Split-race controller, skirmish rules and skirmish runtime fixtures, built with P10 flags by overriding `PROTOSS_ROM_FLAGS` | Pass |
| Skirmish match matrix, built the same way | Pass, 18 configurations, all human losses, which the tool accepts. P7 flags give 17 losses and 1 win. The test controller isn't a winning strategy, so this doesn't rate skirmish balance |
| P8, P9 and P10 scenario headers regenerated into a scratch path | Byte-identical to the pinned inputs |

## Known gaps

- **Availability.** P10's availability tables aren't applied.
- **Gantrithor.** It has no attack. Draft #212 covers Carrier work.
- **Saves.** P1 to P7 have no save continuation under these flags; P8, P9 and P10 have one
  save point each.
- **Speed.** Every mission is below the 30-fps target. P7 is the slowest at about 6.7 fps
  equivalent, and single frames still take 1 to 3 seconds of GBA time.
- **Shared fixtures.** `PROTOSS_ROM_FLAGS` is still the P7 flag set, so most of
  `test-protoss-p7` doesn't test the default ROM's flags. Only the fixtures in the table above
  were run with P10's.
- **Composition.** `make test-sc64-p7-campaign-composition`, a development profile, still
  fails at its P6 step.

## Not run

- **Fresh preparation.** The new prepare step was checked through its unit tests and by
  regenerating the three headers. `./dev sc64 prepare` wasn't run end to end, because this
  worktree's inputs are cloned from the pinned set.
- **Smoke and profile tools.** `tools/sc64_split_smoke.py` and `tools/sc64_split_profile.py`
  now point at the P10 ROM, but they use the shared debugger port, so they didn't run.
- **Production ROM.** ARM results are from a replay probe that shares the candidate flags
  but has its own main loop. The production ROM was linked and verified, not replayed.
- **Full P7 suite.** `test-protoss-p7` wasn't rerun in full on this base.
- **Hardware.** Nothing was run on a physical GBA.

Each result is one route run once. It shows the mission can be won with ordinary input, not
that the route is reliable. The AI backends are authored, so native combat equivalence is
not claimed.
