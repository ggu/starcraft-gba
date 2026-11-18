# P7 combat timing on the current Protoss cartridge

At release `0e536f26`, the standard Protoss P10 build and the build with
`SC64_TERRAIN_FAST_VALIDATE` used the same prepared local inputs and compiler
recipe. The alternate build added one definition to that recipe. Both ROM
hashes match the existing terrain-cache qualification binaries.

The pair used the retained ordinary-input P7 workload from the
[November 6 audit](2026-11-06-current-native-qualification/README.md). Both
started from the title, entered mission 26, and acknowledged three dialogue
pages with physical controller presses. The window covered zero-based tape
calls 8,020–9,019. Of those, 958 advanced the game. Forty-two calls stayed
within gameplay screens without advancing the simulation. The table counts
the advancing calls and includes the wait until the next input sample.

| mGBA timing | Standard | Terrain-cache preview |
| --- | ---: | ---: |
| Mean cycles per update | 2,070,368 | 2,027,375 |
| 95th-percentile cycles | 2,386,093 | 2,340,040 |
| Worst update cycles | 2,693,754 | 2,644,006 |
| Mean updates per second | 8.10 | 8.28 |
| Mean unit-order cycles | 985,150 | 985,349 |
| Mean map-render cycles | 468,976 | 423,199 |

All 11 sampled checkpoints matched, including sampled world state, mission
and input mode, visible and explored grids, and the complete displayed
framebuffer and palette. Every measured call retained the same game update,
mode, and classification. The mean total falls 2.1%. Unit-order work alone
exceeds the 561,792-cycle two-VBlank budget. The release keeps the preview
flag off. This measurement covers one P7 combat window in mGBA.

The order diagnostics reproduced all 11 world and video samples and the
same 958 advancing calls. The instrumented loop adds 2.0% to mean cycles.
On that same workload, setup averages 397,019 cycles, movement 392,902,
and target queries 84,697. Arbiter cache processing accounts for 269,588
cycles inside setup. Its nested counters attribute 118,361 cycles to aura
coverage and 116,849 to detection within the 150,145-cycle cache update.
The second diagnostic read reproduced the same 11 checkpoints again.
The largest movement sample was 1,283,089 cycles at tape call 8,347,
game frame 8,101, the only call where the counted route-build total increased.
Movement still averaged 391,975 cycles on advancing updates with a known
unchanged build count. Lazy materialization can do route work without
incrementing that count. Split route filling, lookup and local steering
costs before choosing the next movement optimization.

An opt-in aura eligibility prototype resolved the service binding once per
source update and reused each candidate's source type. The independent
HOST oracle matched across 240 worlds and 420 updates per world. The
paired ARM window matched all 11 state and display checkpoints. Mean order
cycles fell from 985,150 to 969,136 and mean full-loop cycles fell from
2,070,368 to 2,054,599 (0.76%). Keep this as a profiling experiment and
investigate the larger route and movement scopes next.

## Full campaign replay

Both current P10 ROMs also completed the [retained P7 victory
route](2026-11-13-p7-carried-normal-home.md) from HOME at game frame 55,892
after 57,678 physical input updates. All 59 named checkpoints, the 11
briefing acknowledgments and the 10 in-mission dialogue acknowledgments
matched. The three required heroes survived and both hostile Nexus
objectives cleared. The final verification checked the neutral survivor’s
source owner against the archived tape. Whole-replay timing remains unmeasured.
The paired receipt SHA-256 is
`08d80d7a6cdb49b2065c8d6495cf10f4553205ba744bea2b079ad1d5c72958d0`.

| ROM | SHA-256 |
| --- | --- |
| Standard P10 | `153d80f72c5184b745faf54f4c2edd1aa27805e427ea36aef1950a406c1ff14a` |
| P10 terrain-cache preview | `04e703ad953a4fdb8cf3f5cdfd9ebaf6485d7611af580280ba794f650ece7f69` |

## Contact and detection on the post-menu build

The instrumented 958-update sample separates local contact work from
routing. Contact and steering average 323,796 cycles per advancing
update. Full contact masks account for 183,335 of those cycles. Point
routing averages 24,099 cycles, direct routing averages 4,719, and the
one new point field at tape call 8,347 costs 907,678 cycles. The deferred
optical visibility pass averages 40,427 cycles within the 116,849-cycle
detector refresh scope. These are nested measurements.

Two changes reduce recurring work. A contact mask checks three endpoints
on each axis and applies each result to the directions sharing it. The
detector snapshot resolves metadata and source-owner masks once per
source update. It reuses the same validated detector dimensions at each
actor position. Ownership, power checks, optical sight, distance and
refresh cadence keep their existing rules.

The standard P10 control is release `82421976`. Each candidate used the
same 1,000-call physical-button P7 window. All four replays matched the
11 sampled worlds, complete indexed framebuffers and palettes, and all
1,000 frame, screen, mode and advance classifications. Each advanced
the simulation 958 times.

| Build | Mean cycles | 95th percentile | Worst cycles |
| --- | ---: | ---: | ---: |
| Standard post-menu P10 | 2,073,302 | 2,388,348 | 2,689,507 |
| Contact world bounds | 2,055,422 | 2,370,649 | 2,677,732 |
| Detector snapshot | 2,029,382 | 2,325,930 | 2,693,637 |
| Both changes | 2,011,436 | 2,307,879 | 2,681,419 |

Together the changes reduce mean cost by 2.98%, from 8.09 to 8.34
updates per second in this window. The combined normal Terran, Z10, P10
and Showcase cartridges linked within the existing IWRAM and EWRAM guards.
P10's detector source-tick function uses 3,144 bytes in its own stack
frame, up from 1,888. This is a compiled frame size. Hardware stack
high-water and the 30 fps gameplay target remain open.

The detector differential matched the reference for 240 worlds of 420
updates on P10 and 80 worlds of 420 updates on each of the plain P7,
rectangle-enabled P7 and Showcase configurations. The contact fixture
compared all eight direction bits with a scalar reference at 4,860
rectangular-map cases, including off-map origins and nonpositive steps.
The exact-shared contact and memo tests also passed.

An earlier memo-miss shortcut reduced mean cost by about 0.55% against
the pre-menu control. It remains a separate experiment. Its result does
not enter the combined comparison.

| Paired record | SHA-256 |
| --- | --- |
| Contact world bounds | `11c501152303972ceeed8edf96083100414edf08fc7046613c074a439a52b508` |
| Detector snapshot | `23107fda9ea033dd2352689d1a716dfbe5ecdb0f5f84bd051b23762f8cabc9fa` |
| Contact and detector combined | `be44765f3f4c9f10c79fcc8559aef765602c379146df6b931b9825ca62764489` |
| Earlier contact memo miss | `4c932b5c80c258faad5c7f710152b70777dfb270ac1f02c3970ea15f6f4168ea` |

The combined P10 build replayed the complete P7 victory from HOME at
game frame 55,892 after 57,678 physical updates. All 59 named gameplay
checkpoints, 11 briefing pages and 10 field acknowledgments matched
the verified standard replay above. Paired receipt SHA-256
`deedf718800b168810f9e3f4a25e6774da1998b15becf84b5648031cdebcab3c`.

All four combined cartridges passed a separate fresh SRAM continuation
after 3,000 updates of ordinary custom play, with 900 more updates
matching uninterrupted play. The four lobbies covered both maps, all
human races, one to three CPUs, and a sparse two-CPU lineup. The clean
source checkout passed `./dev test` with 1,161 Python cases, 35 skips,
and the compiled and auxiliary checks.

## Reuse the collision candidate load

The packed collision path already reads a candidate's aligned pair of
signed coordinates for the coincident-position test. The next candidate
build reuses those two halfwords when calculating the mask offset.

The same physical P7 window on release `ca822ea7` passed all 11
world, framebuffer and palette checks and all 1,000 checked update and
route observations. Mean cost moved from 2,011,436 to 2,002,131 cycles
per advancing update, a 0.46% reduction. The 95th percentile moved from
2,307,879 to 2,298,965, and the peak from 2,681,419 to 2,664,696.
The P10 sanitizer oracle matched 636,690 active and 130,616 inactive
contact masks. All four standard ARM products passed link guards and
their three-CPU Blood Bath menu smoke. Each 32 KiB SRAM export matched
its release counterpart byte-for-byte and loaded in a fresh emulator.

A separate trial moved the collision table's speed-plane calculation out
of the candidate scan. Its mean fell by 0.018% and its peak rose, so the
release skips that change.

| Record | SHA-256 |
| --- | --- |
| Paired P7 coordinate reuse | `a22e24caf680d768d894cd8f45b8e552631e94176799e9d3047c85f4d92b764d` |
| Four-profile ARM save comparison | `578799c6f4f246cfa1cd8fae8182eb94283f95a5edfb52cc523238a715f2683f` |
| Separate speed-plane trial | `0b57e84083cad0eb627919ac2d1ceba601711260f54d79eb6b3b5e391d77b7a1` |
