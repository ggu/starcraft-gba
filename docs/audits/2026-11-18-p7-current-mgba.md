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

## Callback-based census experiment

A separate P7 prototype combined actor and Battery integrity checks with
the eight live Command counts. It kept the two failure paths distinct
and passed the host count and bad-state oracles. Both ARM builds kept
the same IWRAM and EWRAM bounds.

On the terrain-default P10 replay all 1,000 frame, route and display
observations and 11 sampled world, fog and rendered pages matched. The
callback-based fusion increased mean guest cycles from 1,958,744 to
1,993,740. Mean cost outside orders and sight increased by 34,850 cycles.
The callback implementation stays outside the release. Local comparison
SHA-256 `db2d5cd981c61d435d093c697bab0428e186e4edff3e7dd1bb5d41229b9c41aa`.

## Duplicate detector queries

A counting build recorded 3,977 detector-list mask queries in the timed
P7 window. Only 48 repeated the same raw unit type and exact position in
the same refresh pass. At the measured average cost per query, replacing
all 48 would save about 0.07% of the sampled interval before lookup costs.
The full tape records 406 repeats in 33,660 queries. Leave this memo out
of the runtime and prioritize the existing expensive paths with actual
repeated work.

## Opaque minimap copy

An aligned-word copy in the 24-pixel opaque minimap matches 256 host
pixel comparisons and the same P7 world and display samples. It reduces
the HUD stage from 130,051 to 127,780 mean cycles and the whole loop by
0.12%. IWRAM and EWRAM stay the same in Terran, P10 and Showcase. The
prototype stays optional. Local timing receipt SHA-256
`7d7dca2ce7694c78d045f2019f00678c294f7237f221dd2b08a660f48578e2a4`.

## Sustained combat after #572

The verified normal P10 build at `ca059860` averages 1,901,686 guest cycles
per advancing update in the same 958-update window. Mean orders cost is
893,661 cycles and mean background render cost is 228,131 cycles. Of 957
advancing updates with an adjacent camera observation, 912 moved the
camera. The renderer used a partial restore on 864 of those. The other
48 moving updates also changed the static cache key. There were three
stationary partial restores and 42 stationary full hits.

An instrumented build adds 0.06% to the mean and matches all 11 world,
fog, palette and displayed framebuffer samples, plus the complete
1,000-call simulation, route and terrain-cache timeline. Mean background
stages are 18,464 cycles for the static key and view bounds, 61,933 for
validation and restore, 78,528 for tile drawing, 24,917 for static object
redraw, and 45,208 for capturing the next cache. Stage values are guest
cycle measurements from the diagnostic ROM.

A vertical-rotation prototype keeps the existing VRAM/EWRAM cache split
and shifts the logical start row. It reads back only the planned dirty
rows after a vertical scroll. A full capture reestablishes the linear
layout after lateral moves or key changes. The cache preserves the exposed
strip requirement and rejects a stale backbuffer or incomplete dirty plan.
An asset-free sanitizer fixture checks 68 vertical passes through the
split and wrap in both directions, and a lateral restore after each pass.

| Advancing update cohort | Release mean | Vertical-rotation mean |
| --- | ---: | ---: |
| All 958 updates | 1,901,686 | 1,889,950 |
| 388 vertical partial restores | 1,883,204 | 1,851,760 |
| Background on vertical partial restores | 218,856 | 186,611 |

The p95 changes from 2,197,484 to 2,195,160 cycles and the peak changes
from 2,566,464 to 2,530,420. The paired run matches the same 11 world and
video cuts and all observed route, frame, and cache decisions. The current
cartridge saves 0.62% mean in this window with the prototype. The vertical
prototype remains local.

A second prototype rotates the cache by words and copies dirty rectangle
spans, allowing lateral and diagonal camera deltas to reuse the same
physical pixels. It matches the same 11 checkpoints and full simulation,
route and cache-decision timelines. Its mean is 1,900,419 cycles, down
0.07% from the release, and its p95 rises to 2,205,884 cycles. On 475
diagonal partial restores, background cost rises from 238,905 to 253,506
cycles. Keep this prototype local. The next rendering measurement should
separate the paint and transfer costs of diagonal redraws before changing
how much of the cache is read back.

Two collision memo alternatives were replayed as observers only. The
exact query rectangle and the normalized geometry key each found **zero**
reusable masks, as did the shipping memo. There were 36,650 eligible mask
requests over 958 advancing updates. All 1,000 call classifications and
11 world and display samples matched. Work on average collision cost should
look inside the first lookup for each mover.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Release camera and cache trace | `1a4e29b7e289417f7869275f9ff8894be785a55ac88036adbb4f147651c0d340` |
| Background diagnostic | `59e0dbdb59b20f53cfe229b9af3397d93b160ab01096d7e84535bb14fc41d1d5` |
| Vertical rotation comparison | `7889ae76d8e347a70d3f78afb6fe8d3292ea6e1f1ecc7e7b533a915f65a29712` |
| Pixel rotation comparison | `d3c115f25ad1778f6e4cfb9e8755ff90871d18b4182ec238adf566e5c2a72622` |
| Collision shadow receipt | `06d8797f03cbb20623cc8f5f27746d3668a11b293dca7cddcf2ee53a1ce62aef` |
