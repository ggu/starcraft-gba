# Steady target and cloak-cache costs on the merged release

The paired physical-button Protoss 10 replay starts from the retained
campaign journal and runs 9,489 updates to victory. The timing control
uses the ROM qualified in #722. The two 16-update attribution windows
reconcile instruction totals with the external guest clock.

## Arbiter cache stores

An untracked unit preserves a valid countdown for any later cloak and
clears its detector state. When the stored value is already correct,
the source tick can skip the write. Tracked units keep the existing
detector refresh and countdown path. Randomized tests cover malformed
values, cloak, aura, transport, slot reuse, and detector refresh.

| Guest cycle measurement | #722 control | Candidate |
| --- | ---: | ---: |
| Mean over the full P10 victory replay | 1,523,158 | 1,517,236 |
| Early P10 cache scope per update | 37,256 | 31,016 |
| Late P10 cache scope per update | 34,743 | 28,873 |

The full mean falls 0.39% to 11.06 emulated updates per second. The
two runs match all 81 world, raw weapon-cache bank, route, fog, HUD,
framebuffer and palette cuts, and the victory journal. A trial that
evaluated cache validity before branching on tracking lost the timing
gain. The recovered production build reproduces the faster trial's
ROM byte for byte.

All eight normal and preview images pass both memory guards at their
original endpoints. Terran and Zerg reproduce the control ROMs.
The changed Protoss and Showcase builds use the shared observer path.
The prepared Protoss and Showcase ASan/UBSan fixtures match the frozen
oracle across 48 worlds and 17,280 updates. All ten Protoss chapters
start from the ordinary campaign menu and advance past opening
dialogue. The two Protoss ROMs pass 36 Custom Game start/save/reload
cases. Showcase passes normal Free for All and preview Melee Blood
Bath continuations.

A controlled native Arbiter encounter remains open. The next N64
comparison should move an ally across the cloak boundary, then
bring a powered Cannon or Observer into detection range. Compare
target acquisition and damage with the same inputs on both engines.
Physical GBA frame pacing remains open.

## Three-AI Custom Game opening pace

The three normal race ROMs enter each map through the regular Custom
Game menu. The human uses the cartridge's race, and the three active
CPU seats are Terran, Zerg and Protoss. After 900 neutral updates, a
second 900-update window measures each advancing main-loop interval.
There are 28–29 live units at the end of each window.

| ROM | Map | Emulated updates/s | p95 guest cycles | Peak guest cycles |
| --- | --- | ---: | ---: | ---: |
| Terran | Outpost Run | 29.86 | 594,898 | 719,666 |
| Terran | Blood Bath | 29.86 | 588,761 | 662,871 |
| Zerg | Outpost Run | 29.78 | 592,674 | 1,160,534 |
| Zerg | Blood Bath | 29.86 | 629,032 | 732,478 |
| Protoss | Outpost Run | 29.76 | 615,386 | 927,305 |
| Protoss | Blood Bath | 29.86 | 589,718 | 661,891 |

The 30-update timing deadline is 561,792 guest cycles. Every opening
sample exceeds that deadline at p95, and the longest Zerg Outpost
update spans more than two such intervals. This measures neutral-input
opening pace in an emulator. Active battle and physical GBA pacing
remain qualification gaps.

## Three-AI Custom Game battle pace

The same map and seat choices run from ordinary new games on the
merged #723 runtime. The human leaves the controls neutral during
play. Each match advances 12,000 updates. Across the twenty
600-update intervals, frame 10,801 through 11,401 is the slowest
in all six cases. Every slow interval shows missing unit slots and
reduced health or shields between its two cuts.

| ROM | Map | Mean guest cycles | Emulated updates/s | End units |
| --- | --- | ---: | ---: | ---: |
| Terran | Outpost Run | 1,100,582 | 15.24 | 105 |
| Terran | Blood Bath | 908,251 | 18.47 | 121 |
| Zerg | Outpost Run | 1,255,362 | 13.36 | 117 |
| Zerg | Blood Bath | 997,132 | 16.83 | 141 |
| Protoss | Outpost Run | 1,267,466 | 13.24 | 116 |
| Protoss | Blood Bath | 1,013,490 | 16.55 | 134 |

The independent instruction attribution at Protoss Outpost frames
11,100–11,115 matches the unprofiled run in guest cycles, world,
palette, both video pages and route counters. The last cut has
134 units and 58 buildings. Full-loop mean is 1,375,049 cycles.
Orders account for 633,093 cycles and rendering for 529,808.
Nearest-target search costs 139,078 within orders, including 48,936
for candidate eligibility. Movement accounts for 140,507, and
combat for 115,178. Within rendering, depth-ordered actors account
for 320,937, with 96,034 in the tracer part draw. Inclusive samples
nest. The route build counter stays at 652 while the hit counter
moves from 18,511 to 18,545. Use this no-new-build battle cut for
repeated target eligibility and tracer admission work, alongside the
separate route-stall cut. Player-driven battle pacing and hardware
measurement remain open.

### Iterate the published target row mask

The indexed nearest-target query already maintains a
per-owner row mask. Shift the masked bits down to the
first row of the search and consume them in order. This
reads the owner mask once and stops at its last set bit.
For each visited row, walk the corresponding cell heads
from the first clipped column.
Conservative stale bits still permit an unnecessary visit.
The cell order, distance comparisons and eligibility
checks remain the same.

Each authored ARM batch runs 512 queries through the shared
search body over 144 unit and 64 building slots. Query
answer digests match with the new row loop. The timed span
includes the query input generation and digest accumulation.
IWRAM is 32 bytes smaller with the normal compiler setting
and 48 bytes smaller with its size-optimized query setting.

A separate sanitizer run compares both exact query bodies
in 262,144 calls across 256 authored worlds, including
edge rows and columns, moved units, stale positive bits,
dead buildings and both hostility paths. Every answer
matches.

| Placement span | Search radius | Normal cycles, before → after | Size-optimized cycles, before → after |
| ---: | ---: | ---: | ---: |
| 2,048 | 90 | 990,134 → 913,289 | 1,196,920 → 1,130,508 |
| 1,024 | 64 | 1,048,982 → 985,032 | 1,241,906 → 1,180,142 |
| 512 | 90 | 2,129,854 → 2,072,209 | 2,540,293 → 2,447,823 |
| 128 | 64 | 5,442,837 → 5,386,910 | 6,804,279 → 6,758,675 |

Iterating just the row bits slowed the size-optimized
query in all four batches. Walking the cell heads as part
of the same change gives savings in both compiler modes.
The authored measurements qualify the query loop only.
Full-frame campaign and Custom Game performance
measurements are still open.

| Local receipt | SHA-256 |
| --- | --- |
| Authored ARM fixture | `304b9c76936634898cc5e37e469fd98b7c9460db45c1ba04d2bcb113773792f1` |
| Normal ARM timing output | `efea963a269806a929b2011e26eb4083316bee976cfc169801d43fea8f959a3c` |
| Size-optimized ARM timing output | `6bcfabd71d31135cb3fca7f5714d8eba990190f9c1e67877aac796af9222c3f6` |
| Authored host differential runner | `ae763c1c6b53f62c2fe31bede694e7df354cfa67b50fb2e7a12f9ba9d1e1af56` |

### Walk direct corridors by grid cell

The direct path follows both axes until one reaches its
endpoint, then finishes on the other axis. Advance the
walk-grid index at each crossed boundary, including one
diagonal index advance for a simultaneous X and Y crossing.
With in-bounds endpoints this uses the existing walk grid
without a bounds check, coordinate division and grid-validity
check at each tile. Starting and finishing in one cell
still causes no grid rebuild or cell read. Out-of-bounds
coordinates use the existing checked path.

The ASan/UBSan comparison matches the full cell-read
sequence, the decision, and the grid rebuild count on
120,000 authored paths. It includes clear and blocked
terrain, four map sizes, horizontal, vertical and diagonal
trips, endpoints in the same cell and negative coordinates.
An ARM fixture with runtime map dimensions compares eight
batches of 256 paths, with matching answer digests.

| Authored batch | Existing cycles | Grid-cell walk cycles |
| --- | ---: | ---: |
| Horizontal clear | 2,006,216 | 793,903 |
| Vertical clear | 1,616,912 | 733,184 |
| Short clear | 1,225,685 | 1,122,233 |
| Mixed long clear | 3,133,377 | 1,067,770 |
| Mixed blocked terrain | 824,380 | 632,812 |
| Starts west of the map | 3,302,890 | 5,074,872 |
| Identical endpoints | 551,913 | 551,913 |
| One pixel within the same cell | 575,274 | 568,553 |

The authored and default-build IWRAM sections are each
32 bytes smaller. The out-of-map reference takes longer
from ROM. A read-only sample of the retained Outpost battle matches all
16 world, route, palette and two-page cuts. It contains
95 direct paths, all in bounds. Of those, 79 cross tiles
with both axes moving, 12 are axis-aligned and cross a tile,
and 4 begin and end in the same tile. The original direct
path scope averages 2,146 guest cycles per update in this
window. The uncached steering mask averages 54,591. The
table above measures the query, with full-frame and hardware
timing still open.

A second read-only run observes 80 updates from the
frame-46,924 Z8 HOME checkpoint. All 18 direct-path calls
are in bounds, and every sampled world, route, palette and
two-page result matches its uninstrumented continuation.

The asset-free default ROM starts through ordinary A input
in both builds and runs 1,200 updates. All 20 sampled actor,
economy, RNG, palette and two-page hashes match. Total
guest cycles are 673,806,915 and 673,807,340 respectively.
This run includes presentation pacing and provides
integration evidence for the default ROM.

| Local receipt | SHA-256 |
| --- | --- |
| Authored fixture | `dbd944ea45a12af8bf955e00b17d7c4cbbc1219c168c8a759aa0a69b2b20c595` |
| ARM timing output | `9e96d978b769074b9988a43daa5cb944b0c702142ab305798e75262a69d78a83` |
| Retained Outpost direct-path census | `a0646920b36b128fa27727a0f9cadc7cadcf5188507f240b9644678d785be0e5` |
| Z8 fresh HOME direct-path census | `9cec10842ae59e3e43eb7fb8f9992fd8690360c535325d7187e8ce7e9f547a57` |
| Asset-free default ARM pair | `4bda4c6d63c6ecaf9edfb9bbc9904b9fcc3114ee5e96cd43059577ef4eb12387` |

## Visible fights in Protoss Crossfire

The merged #724 Showcase build enters Crossfire through the normal
hidden menu and runs with neutral controls. Profiled and unprofiled
runs match all 32 full clock, world, tracer bank, route, palette and
video samples. This gives a dense on-screen workload alongside the
three-computer Outpost setup.

| Mean guest cycles per update | Frames 40–55 | Frames 95–110 |
| --- | ---: | ---: |
| Full loop | 1,462,383 | 1,380,207 |
| Game update | 486,099 | 402,542 |
| Render | 839,388 | 847,815 |
| Depth-ordered actors | 584,897 | 592,225 |
| Byte sprites inside unit drawing | 130,161 | 148,318 |
| Tracer part drawing inside actor depth | 119,725 | 109,255 |
| Overlay walking inside tracer parts | 26,982 | 5,719 |

The early and late overlay scopes vary even with similar call counts.
The retained stack records contain 446 and 594 tracer-part calls
and 148 and 137 overlay walks, respectively. Neither window records
a call to the ground-trail depth helper. Reusing ground-trail picks
would need a workload that actually exercises that helper.
A separate read-only census matches the 32 unprofiled cuts and maps
all 507 direct unit byte-sprite calls to attack-pose source arrays.
Those calls use 56 distinct frame pointers. The full bank has 1,651
arrays and 1.23 MB of indexed pixels. Four phase copies with masks
would need at least 9.84 MB, above the available ROM space. Qualify
a bounded packed subset and retain the exact byte-sprite fallback.
Retain the ordinary Outpost
battle as a second workload.

A local trial packs 85 frames from one source bank shared by actor
IDs 37, 38 and 53. A linear pointer lookup raises the mean in both
windows. An ordered lookup lowers the early window from 1,462,383
to 1,447,442 cycles and the late window from 1,380,207 to 1,363,039.
All 32 recorded cuts match. It adds 286,156 linked ROM bytes and
preserves both RAM endpoints. The experiment verifies the pointer
order for that link. Keep the branch local while converting the
lookup to direct pose metadata and qualifying a sustained battle.
 The historical mode-0 tile path falls
back to Mode 4 for Custom Game and gave flat P7 timing. Its imported
audio and RAM limits remain relevant. See [renderer validation](../VALIDATION.md#p7-mode-0-tile-renderer).

## Generated attack-pose sidecar

The optional source generator takes actor names and emits sidecar
metadata from the same resolved pose records as the byte-sprite
bank. At runtime, the renderer uses the resolved actor, mode,
heading and tick to select the packed frame, then checks frame
identity. A generated content identifier rejects stale bank pairs.
An empty option preserves the existing generator output and draw
path. Switching the option invalidates the build configuration.

With the Hydralisk bank selected, the paired Showcase Crossfire run
averages 1,421,295 to 1,399,615 cycles per update in the two 16-update
windows. All 32 world, video, palette, route and tracer bank samples
match. The eight normal and preview links pass their original memory
guards. Each gains 292,332–292,732 linked ROM bytes, with unchanged
BSS and data sizes. A prepared fallback without the word-sprite
capability also links. The 36 focused source generator, facing and
weapon tests pass, including bank mismatch and boundary cases.

The documented Showcase Make target also builds with the option
disabled, enabled and disabled again. The final disabled ROM matches
the first one byte for byte. The enabled build adds 292,532 text bytes
with unchanged data and BSS, and the two products match all 32
Crossfire checkpoints. The pooled guest cycle mean falls by 1.48%.

Over 900 advancing updates of the full Crossfire scene, the
mean falls from 946,292 to 937,866 cycles (0.89%). All 30 checkpoints
match the world, both video pages, palette, route and tracer bank.
The scene is still in play at the last sample.

The broader replays show the workload dependence. All six
12,000-update three-AI Custom Game pairs match 120 world, two-page,
palette and route cuts. Mean cycles are within 0.01% in each pair.
The eight packed-actor cartridge variants pass all 48 ordinary
Custom Game startup, save and fresh-process load cases across both
maps and the three independently configured CPU seat counts. Each
case runs 120 updates before saving and 120 after loading.
The 9,489-update P10 continuation matches all 81 cuts and victory
SRAM, with means of 1,506,238 and 1,506,323 cycles. Z1 reaches the
same victory at frame 50,669 with 50 matching periodic checkpoints.
Over the 50,856 recorded inputs, mean cycles fall from 698,858 to
698,346 (0.07%). The actor subset remains an opt-in build option.
Choose further actor banks using sustained visible battle measurements
and the remaining ROM space.

The same frame subset can use two alignment phases of 16-bit pairs.
It stores 127,540 pixel and mask bytes, versus 283,608 for four
word phases. The byte and pair renderers agree in the clip/no-clip
ASan/UBSan oracle and at all 32 Crossfire samples. The pair loop in
ROM raises the early mean to 1,507,434 and the late mean to
1,426,478. Putting its inner loop in IWRAM lowers them to 1,460,249
and 1,375,805. The latter adds 72 IWRAM bytes in Showcase and trails
the four-phase path. The normal Z10 link has 40 bytes of IWRAM guard
margin. Keep the pair variants local.

## Aligned reads in the shared byte-sprite compositor

A local trial reads two indexed pixels in one halfword when the
source pointer is aligned. Unaligned pairs use byte reads. Mode 4
still writes only aligned halfwords or words, and transparent pixels
retain the corresponding destination lanes. The original-art
framebuffer oracle passes with both word-copy configurations under
ASan and UBSan.

The ordinary Showcase Crossfire control and the trial match both
video pages, the palette, world, tracer bank and route at all 32
short-window cuts. Removing indexed inner-loop address arithmetic
alone saves 0.64% pooled. Also using aligned halfword reads saves
2.24% pooled and 1.53% over 900 advancing updates. All 30 cuts
in the longer fight match.

The eight normal and preview links reproduce the measured pilot
ROMs byte for byte from the tracked source candidate. Each passes
its RAM guard. Showcase gains 72 bytes of IWRAM margin and normal
Z10 has 16 bytes of margin. All six 12,000-update Custom Game pairs
match 120 state and video samples. Outpost means increase
0.33–0.42%, and Blood Bath means fall 0.04–0.05%. The P10 fresh
save tail wins with all 81 state, video and route cuts and the
victory journal matching. Its mean rises 0.018%.

Keep the trial out of the regular cartridge builds. A broader
renderer change should retain the visible-battle improvement while
reducing cost under both common skirmish workloads.

## Target column and crowded pose indices

Nearest-target search now uses the published per-owner column
summary to skip cells in unused columns. Occupied columns and
entity chains retain their previous visit order. A stale set bit may
still cause a cell read. The isolated change lowers targeting from
136,688 to 131,331 guest cycles in the 16-update Protoss Outpost
battle sample and saves 0.16% in the 9,489-update P10 victory tail.

The painter builds a tracer index when eight or more admitted units
have active cooldowns. It includes live tagged projectile-only traces
and preserves ascending tracer-slot priority for equal ages. Unit
identity, launch coordinates and pose validity still decide whether a
trace supplies a frame. Sparse scenes and draws outside the indexed
painter use the direct scan. An unconditional index increased mean
cost in every 12,000-update Custom Game replay by 0.21–0.40%.

The combined runtime saves 1.19% over 900 advancing Crossfire
updates, with 30 matching world, video, palette, route and tracer
cuts. The P10 fresh save tail saves 0.16% with 81 matching cuts and
the same victory journal. All six 12,000-update Custom Game pairs
match all 120 non-timing samples. Five pairs use 0.004–0.134% fewer
cycles and the sixth uses 0.003% more. All 48 ordinary Custom Game
startup, pause save and fresh HOME continuation cases pass across
eight normal and preview products.

All 30 missions launch through the ordinary campaign menus on the
three combined normal cartridges. After acknowledging any initial
field dialogue, each advances into gameplay. The 30 retained
launch checkpoints match the facility release.

All eight links pass the static RAM guards. The painter's stack frame
grows by 224 bytes on Terran and Z10 and 280 bytes on P10 and
Showcase. Observed system stack margins over the static IWRAM end are
6,776 bytes in sampled Crossfire updates and 6,312 bytes in five Z10
busy-save updates. These samples leave worst-case stack use open.
The prepared opt-in packed pose bank also links. With the option
enabled, 32 checkpoints across 960 Crossfire updates match the
disabled build, with a further 0.93% cycle saving in that run.

## Mode 4 page update coverage

On the merged targeting and pose runtime, the physical-button
observer reads both indexed display pages, the palette and DISPCNT
after each advancing update. Each 16-update window below has one
page flip per update and a stable palette. The page selected in
DISPCNT already contains its new image at the preceding sample.
The other page contains the newly composited world for the next
flip, so the comparison follows that physical address across the
update. A tile in this census is a 16×16 screen-aligned region of
the 240×108 battlefield, with a shorter last row.

| Workload | Camera changes | Mean changed work-page tiles | Mean tiles with an overlay over terrain |
| --- | ---: | ---: | ---: |
| Crossfire, frames 40–55 | 0 | 35.94 | 80.94 |
| Crossfire, frames 95–110 | 0 | 35.13 | 77.75 |
| Crossfire, frames 600–615 | 0 | 8.25 | 55.00 |
| Three-CPU Outpost, frames 11,100–11,115 | 0 | 16.00 | 73.75 |
| P10, calls 39,760–39,775 | 13 | 96.81 | 60.38 |
| P10, calls 44,000–44,015 | 13 | 50.13 | 49.75 |

All counts are out of 105 tiles. The overlay column compares the
new work page with the bare terrain and fog band assembled from
its VRAM and EWRAM cache segments. The cache is valid and its
camera matches the new frame at every sample. Camera movement
changes all 105 work-page tiles in the first P10 window on each
of its 13 movement updates.

Terrain-only restoration can skip a smaller set of tiles than
the visible frame change suggests, because substantial stable
art remains over the terrain. Prioritize reuse of composited
regions on a stationary camera, with an exact test for unchanged
contributors and bounded storage for both pages. Retain moving
camera cases in the benchmark. The census measures pixel
equality. Render-cycle savings remain to be measured.

Counting full-width work-page changes in the same fixed-camera
windows gives an average of 37 changed rows and 6.31 changed
eight-row bands in Outpost. Early and middle Crossfire average
79.25 and 76.25 changed rows, or 10.56 and 10.13 bands out of 14.
These are lower bounds on the rows that a conservative damage
tracker would revisit. The row census receipt SHA-256 is
`161e90bd4a097528006b93f3e17b24b8bd8b456c7bbe24d74c19467d513128c0`.

The old work page differs from bare terrain in every row in the
late Outpost cut and all three Crossfire cuts.
Terrain stays stable during all 64 updates. Skipping only clean
full-width rows would save no terrain transfer in these samples.
An exact reuse trial will need smaller regions or retained
composited bands. The terrain-restore census receipt SHA-256 is
`9c40cfe4704a984474011c86f8fa34e8b0ada5fc9cf5e7bc1778ef85c9573d0e`.

## Detached attack sprite draw records

Crossfire spends about 27,600 guest cycles per update resolving
the position and source age of detached attack sprites. The painter
resolves an admitted sprite once for ordering and again for drawing.
A local trial reuses the draw record stored at admission. The first
variant keeps up to eight records, and a second indexes the full
bounded set by tracer slot. Both preserve the existing heap keys.

| Replay | Control mean cycles | Slot-indexed trial |
| --- | ---: | ---: |
| Crossfire, 900 updates | 935,055 | 929,124 |
| Protoss Outpost, 12,000 updates | 760,046 | 760,160 |
| Protoss Blood Bath, 12,000 updates | 689,425 | 689,504 |
| P10 fresh HOME victory, 9,489 updates | 1,503,782 | 1,503,862 |

The slot-indexed trial matches all 151 sampled cuts, including both
display pages, the world, and each workload's retained route and
palette state. The campaign victory journal matches byte for byte.
The eight-record variant has about the same Crossfire gain and
slightly higher costs in both Custom Games. Keep both trials local.

A separate census of 507 direct attack-pose byte-sprite calls in
Crossfire finds 140,501 transparent edge pixels out of 267,934
source pixels. That includes 2,539 entirely transparent rows. The
census uses unclipped source rectangles, so it bounds the available
work rather than measuring a cycle saving. The 56 distinct poses
would use 2,708 bytes of two-byte row bounds before lookup overhead.
Measure an equivalent Outpost sample before choosing a sidecar.

| Local receipt | SHA-256 |
| --- | --- |
| Slot-indexed Crossfire trial | `b94c5cfabe7a73025c0f8e01648747fc0c623f475ecdb7b2980386d9f6175979` |
| Slot-indexed Custom Game trial | `6d87accdeec50ec16eaceca21acd5b543b8bb1ec5deb6813bcf2e2ef073a484b` |
| Slot-indexed P10 trial | `0031a3876a0b9787bf9913bc4bacdd2da06c206ceb3d89fc8a888e30c36ba033` |

### Transparent edge and word-run trials

Four retained late P10 Melee cuts at calls 11,385–11,388
match the world, route, palette and both displayed pages.
The read-only census covers 84 byte draws, 1,410 visible
rows and 23,736 visible source pixels. After clipping and
the odd first pixel, 594 rows have matching source and
destination word alignment. A leading and trailing aligned
zero scan can omit 2,356 visible pixels. It removes at least
two words in 126 rows and never removes a whole visible row.
In the existing aligned pair loop, 276 additional zero words
follow a blank quad and can be skipped across the four cuts.
These are opportunity counts on the retained `1f0ad66e` ROM.

The authored ARM benchmarks compare a per-row edge scan,
a run scan only after an aligned zero word, and a variant
that also scans source-aligned words after a blank scalar
pair. All eight cases for each tested build match a scalar
oracle across the full framebuffer. The measured cycles
include the call and loop around 40 repeated draws per case.

| Authored sprite case | Baseline cycles per draw | Trial cycles per draw |
| --- | ---: | ---: |
| Edge scan, sparse 32×24 | 18,136 | 12,867 |
| Edge scan, dense 32×24 | 16,856 | 20,011 |
| Aligned run, sparse 32×24 | 7,745 | 7,271 |
| Aligned run, blank 48×24 | 9,601 | 6,359 |
| Aligned run, dense 32×24 | 9,793 | 9,839 |
| Mixed run, sparse 32×24 at misaligned destination | 13,761 | 10,002 |
| Mixed run, dense 32×24 at misaligned destination | 12,385 | 12,986 |
| Mixed run, right clip | 10,001 | 10,890 |

The initial edge trial uses `-Os`; the two run trials use
`-O2` and the same word and scene-clip capabilities. The
gain depends on existing alignment and long transparent
spans. Keep the three scans local given the dense and clipped
regressions. Measure a certified row-bound compositor on a
mixed visible workload before adding draw-time scanning.

| Local receipt | SHA-256 |
| --- | --- |
| Edge scan fixture | `bd14fec8242bac7e6999f8411829b87fd3e55ae585105dd9d5647cc9d9a914e6` |
| Aligned zero-word run fixture | `dda33f09302607cf627cf5ef8104941b8c04ec12b43a9c3bd3f677be3a49ecac` |
| Mixed run with misaligned destinations | `0e5be3c3c2ac1494deed16d1414052f9c008228c8c2bcce26e2b6ffc478a0cfb` |

### Precomputed row bounds

An optional compositor reads the first and last opaque source
column for each row. It skips empty rows and reads the phase
words only between the clipped endpoints. The packed frame
layout stays the same. A local converter emits the bounds
only when explicitly requested and returns them only for a
matching raster. Authored host fixtures passed 2,372
differential cases with scene clipping and again without it.
The ARM trial compared every pixel on the destination page
against a scalar oracle for all eight authored fixtures in
all three builds.

| Authored case, cycles per draw | Existing packed IWRAM | Bounded ROM | Bounded IWRAM |
| --- | ---: | ---: | ---: |
| Sparse 32×24 | 4,870 | 9,206 | 3,285 |
| Blank 48×24 | 6,006 | 1,942 | 693 |
| Dense 32×24 | 5,598 | 18,886 | 6,693 |
| Two transparent columns at each edge | 5,214 | 15,862 | 5,493 |
| Sparse 31×25, odd destination | 5,074 | 9,719 | 3,466 |
| Sparse left clip | 4,056 | 9,208 | 3,287 |
| Sparse right clip | 3,518 | 8,198 | 2,885 |
| Sparse vertical HUD clip | 2,548 | 4,868 | 1,763 |

The IWRAM trial uses 608 additional bytes of that section.
The ROM placement costs more on every tested nonempty
fixture. The IWRAM placement saves 18–33% on the sparse
cases, costs 20% on dense art and costs 5% when only two
columns are transparent at each edge. Keep both paths local.
Before spending IWRAM on a second compositor, price a
whole-frame crop on a mixed scene, where transparent edges
are often too narrow to skip an aligned word.

| Local receipt | SHA-256 |
| --- | --- |
| Authored ARM runner | `f6361053fe49b323f475802d942ab0928cbee18ef300f3fe408c5f0fe5677385` |
| ARM timing results | `7ceaac2e92c4db13c29bffd6bf9d81c413f7cb526a84fc28d0791eab6b9311d8` |

### Whole-frame transparent crop

This trial removes the outer rectangle of transparent
pixels before phase packing. The draw adds the crop's
left and top to the original destination and uses the
existing packed compositor. All 11 authored cases in
both ARM builds match the scalar oracle across the full
destination page. The mixed fixture includes dense and
blank controls. The draw wrapper reads a frame pointer
in both builds and loads two signed offsets in the crop
build. Empty images use their original frame.

| Authored case, cycles per draw | Original | Cropped |
| --- | ---: | ---: |
| Sparse 32×24 | 4,905 | 2,463 |
| Dense 32×24 | 5,633 | 5,663 |
| Narrow transparent edge | 5,249 | 4,463 |
| Sparse 31×25, odd destination | 5,109 | 2,599 |
| Sparse right clip | 3,553 | 2,063 |
| Small 12×14 | 1,699 | 1,207 |
| Dense 12×14 | 1,853 | 1,883 |

The two offset loads and additions cost 30 cycles in the
dense controls. IWRAM section size remains 5,680 bytes in
both fixtures. The measured gains cover authored draws only.

The read-only census of all 16 retained late P10 Melee cuts
(11,385–11,400) checks the widest and tallest opaque extent
in each recorded call. All 319 packed draws use tight
rectangles already, for zero removable groups across
182,468 groups counted against all four phases. One of 321
byte draws has five removable pixel slots out of 160,965
in the sample. Keep the crop prototype local. If a future
bank has measurable whole-frame margins, preserve frame
identity, apply offsets after mirroring and drawing
placement, and align culling with the rendered pixels.

| Local receipt | SHA-256 |
| --- | --- |
| Crop fixture with runtime offset loads | `2cc7dad7146efea171ffb67fd5722f4c55254e1fcea3b20cfa68e411104ebb67` |
| ARM timing results | `5f9e32c37499c07a60ab8fe2343f1217eca70542efc9c2e9e5aed581edcb2eea` |
| Retained P10 Melee source-call evidence | `7807d46bc665b7073f7736994622eacb71c5cb0228a9f170a37701b7fc1c6970` |
| Crop census | `760de111b9dfc950b6c5c662ab6e9bde00a15c0b8aaa4296a7621ec322bdc222` |

## Shared heading for projectiles and unit facing

Projectile creation and delayed-impact launch now use the same
integer direction lookup as unit facing. A double shot reuses the
heading of its two common endpoints. The previous projectile
calculation tested all 32 rounded direction vectors. An independent
dot-product oracle matches 16,785,409 map-coordinate pairs and
140,256 additional samples on and beside direction boundaries.
Those samples extend to deltas of 65,535 and include the wraparound
tie between northwest and north.

Across six 12,000-update three-CPU Custom Games, the shared lookup
uses 0.0166–0.0224% fewer guest cycles. Crossfire uses 0.067% fewer
cycles over 900 updates. The P10 fresh HOME replay saves 0.037%
over 9,489 updates to victory. All 231 sampled world and video
cuts and the P10 victory journal match. The four normal and four
preview images pass the existing static RAM guards. The default
asset-free ROM reproduces byte for byte.

All 30 campaign entries launch and advance after opening dialogue,
with the same 30 recorded world checkpoints. The three normal race
cartridges pass 1,134 ordinary Custom Game launches. Each launch
checks the human race, map, computer seats and town hall locations
for every nonempty arrangement of the three named or closed seats.
Forty-eight Custom Games across the eight images pass ordinary pause
saves and fresh HOME continuation on both maps. The prepared
weapon and projectile depth fixture passes under ASan and UBSan.

The first full local run found three failures in the prepared
building roster fixture. They reproduce on the preceding release.
The fixture called the vitals overlay by itself while expecting a
ring from the world-depth painter. It now exercises the painter and
composes the expected ring, body and vitals in that order. The
corrected renderer module passes with the current prepared inputs.

| Local receipt | SHA-256 |
| --- | --- |
| Independent direction differential | `88e2b1f065c7ac0332c915dcd3ab5b9aaaf16de557b2acdc475e8a88b0009295` |
| Six Custom Games | `b88b62332e1287175a75e5bea04c4376fdbf9e2520ab0f1561471dc172b73d19` |
| P10 fresh HOME victory | `8494bbede980bf2cd86fcf209b315637c9cc7a631ed8968d5c078098485b9135` |
| Crossfire | `24bed3a24f0deebc76a1215fbdade40775c3c0b7029514091c23f1921b384333` |
| Campaign launches | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Exhaustive named-seat launch matrix | `f92dff99fd9671eea66174eec2ef2b9f50b97df1422929df8089aa388b8640b6` |
| Pause save and fresh HOME matrix | `5325dd211df92503fc5e8762154600c99e57bb77698263e396b60506e3269f82` |

## Source weapon profile index

On the preceding P10 ARM link, the cached timing lookup uses two
software divisions to check alignment and index a 52-byte profile.
The shared lookup bounds the address by the array and divides a
record offset by four, then multiplies by the exact inverse of 13
modulo 2^32. The result is an index only when it lies inside the
bank. A different record layout uses the ordinary checked division.
The packed cache, profile values and caller interfaces stay in use.

The ASan/UBSan fixture checks every byte offset in the 66-record
array, the one-past address and copied profiles. Its timing and
motion differential passes 686,256 cases. All eight normal and
preview ARM links retain their preceding IWRAM and EWRAM endpoints.
The default ROM matches its preceding bytes.

| Replay | Control mean cycles | Index mean cycles |
| --- | ---: | ---: |
| Crossfire, 900 updates | 934,432 | 926,310 |
| P10 fresh HOME victory, 9,489 updates | 1,503,232 | 1,502,844 |
| Protoss Outpost, 12,000 updates | 759,876 | 759,603 |

All six ordinary three-CPU replays use 0.029–0.041% fewer cycles.
All 231 world and video cuts match, along with the full P10 victory
journal. Crossfire saves 0.87%. The P10 continuation still averages
about 11.16 emulated updates per second.

All 30 campaign launch checkpoints match. All 1,134 normal Custom
Game launches verify the requested map, human race, named/closed
CPU seats and hall locations. The 48 ordinary pause-save and fresh
HOME cases pass across the normal and preview products. The full
local `./dev test` run completes with 1,177 Python tests, nine skips
and passing native sanitizer fixtures, including the renderer module
with the corrected world-depth building check.

| Local receipt | SHA-256 |
| --- | --- |
| Six Custom Games | `f088b70e067f3c8a41cc100c5c13f3bec42954b2dd61256256e5427f5b1ca6b6` |
| P10 fresh HOME victory | `b4987e76dbe7c5b43766a8939b41a8cfc29c5f3d00759b62dc569bc17a4f6817` |
| Crossfire | `ac9491edfd8a82afacb71c97a1f58f657e3d210789ec49373503af7667982a37` |
| Campaign launches | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Exhaustive named-seat launch matrix | `9e8fa637a948f4e66ffb70d69b99607665d2d3ead3fff5cc87a4672becc80311` |
| Pause save and fresh HOME matrix | `71c6395cd239cf8b4b8491d3d86d74d9c8f305c0eb8ea72886912154f6c82972` |

## Visibility coordinate lookup

The prior P10 link spends about 27,235 cycles per busy Outpost update
in `currently_visible`. Its Thumb code uses signed-division correction
on each axis and saves registers to the stack. The shared lookup now
checks both signs together and shifts nonnegative coordinates directly.
Coordinates from -15 to -1 still map to cell zero. Coordinates of -16
or lower are outside the grid. The common linked path uses no stack.

The byte and packed grids each match the earlier C calculation in
1,578,123 ASan/UBSan comparisons. The fixture covers offsets around zero
and both far edges, empty dimensions, non-power-of-two widths and
integer extremes. The original-art skirmish fixture covers the first and
last cells and the adjacent out-of-bounds cases. All four usual ARM
builds have the same IWRAM and EWRAM link endpoints as the control.

| Paired replay | Mean guest cycles saved |
| --- | ---: |
| Terran Outpost, 12,000 updates | 0.77% |
| Terran Blood Bath, 12,000 updates | 0.56% |
| Zerg Outpost, 12,000 updates | 0.81% |
| Zerg Blood Bath, 12,000 updates | 0.63% |
| Protoss Outpost, 12,000 updates | 0.87% |
| Protoss Blood Bath, 12,000 updates | 0.63% |
| Showcase Crossfire, 900 updates | 0.47% |
| P10 fresh HOME victory, 9,489 updates | 0.73% |

All 231 replay cuts match world state, both framebuffer pages, palette
and route counters. The P10 continuation also matches its fog grids,
weapon and HUD fingerprints and final SRAM. Its mean moves from
1,502,844 to 1,491,932 cycles per update, about 11.25 emulated updates
per second. The 30 campaign opening fingerprints and all 24 regular
pause-save and fresh HOME cases also match, including saved SRAM bytes.
All 1,134 normal Custom Game launches match the earlier map, human race,
named or closed CPU seats and hall positions.

| Local receipt | SHA-256 |
| --- | --- |
| Coordinate comparison fixture | `42efec20460807a6979d86fd55a518a5d3f97bb905d8e68ae3b97460be250b75` |
| Six Custom Games | `dbc98b62ef8a7de0132ed090ca96b89c978c80cb8f8dca562ed751c7b0456da2` |
| P10 fresh HOME victory | `c28c76cf409873d428df3fffd61f7ee4df4451e6c361b6a7d302507a3bce7b09` |
| Showcase Crossfire | `7086a496074cfdea19df7cb7d88d6338581611e52c430ff4c387f0c2dbfbab4f` |
| Campaign openings | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Regular pause save and fresh HOME matrix | `f3d10f2b9f5e7354fee2e0a879df60a676f4fd8b73f796e73ca139994aed7350` |
| Ordinary Custom Game launch matrix | `0955050b8a1dd809200f174a728a01c689deddbfb88d0b347a27c9a7c67b5d34` |

On the retained Outpost frame 11,100, a read-only breakpoint census
records no entries into the attacker-overlay branch of tracer drawing.
The same cut matches world, both video pages, palette and route counters.
An early exit for empty actor overlays cannot save cycles in this cut.
Measure the projectile-overlay branch and wider fights before proposing
a tracer shortcut. The one-frame receipt SHA-256 is
`17be680573df0bde2e69793ffbfa0372f25bea1765d31bf0bca2d4e209d03e24`.

## Partially fogged effect raster

The local tile-band trial queries each column once per 16-pixel world
row and reuses the visibility mask for its scanlines. It clips to map
bounds and preserves each row's blit order for flipped and mapped
sprites. The sanitizer pixel oracle passes a 256-pixel raster that
straddles 17 tiles, all 16 tile phases, and viewport and map edges.

Twelve authored ARM batches use the shared Mode 4 blitters on ordinary,
strength-mapped and destination-mapped pixels. Each batch draws 24
effects. Both versions produce the same framebuffer digests.

| Visibility case | Cycle change across the three batches |
| --- | ---: |
| Whole raster visible | 0.94–3.00% more |
| Whole raster hidden | 65.03–91.96% fewer |
| Mixed visible and hidden tiles | 27.84–39.05% fewer |
| Map edge | 54.91–86.20% fewer |

Separating the fallback into a function brings fully visible changes
within about 2%, with stack use split across caller and callee. These
are fixture measurements.

In a read-only replay of the retained three-CPU Outpost battle,
all 105 raster calls across frames 11,100–11,107 are rejected by the
existing viewport bounds. In Crossfire, the 498 raster calls across
frames 40–55 and 95–110 all pass the whole-raster visibility check.
Both replays match the retained world, both framebuffer pages, palette
and route samples. The 105 viewport rejections consume 25,565 guest
cycles from raster entry to return, about 3,196 per update. That leaves
little headroom inside the final check. Neither workload exercises
partial fog. Keep the trial local.

A nested trace of the same eight Outpost updates attributes all 105
rejected rasters to 82 of 426 tracer calls. Those 82 calls spend
511,770 guest cycles from tracer entry to their first raster,
about 63,971 per update. All 426 tracer calls together cost 787,611
cycles. The preparation total includes timeline and overlay work
the painter may still need. Identify work that an earlier bound
can safely skip, and qualify unchanged depth ordering and trail picks
in a visible fight.

| Local receipt | SHA-256 |
| --- | --- |
| Authored ARM fixture | `1aba289823f13756238e8cdd9096c2d6732f4953123d044c8730a89da7a1b95a` |
| ARM timing and framebuffer results | `457397a6ac76a5394611fc82d4b8c8512d9f66982e2ba36bafda50d503f6de1b` |
| Separate fallback ARM results | `b61bede9e95f61e803d4ef659ff932b0992b8acf31f81683be1dbb2237638123` |
| Outpost raster census | `d0d360dc2a3da44aabbf82823dc2cd967bc2df075cfad0b0a42a1d7cc30200db` |
| Outpost raster entry-to-return timing | `8c42be687db0ef0b77890733bcc1da1ba4525bd5328717c123ad15528904056d` |
| Outpost nested tracer timing | `f294995a856fe7b7807377ef5c7d6937645fd484aefc2008b26389e2435ef713` |
| Crossfire raster census | `ba93b109c4b900c94c13aa64557dfb0148bce73091acc26e8d2926f827752e8b` |

### Viewport bounds for captured FOLLOW effects

The same Outpost cuts trace all 105 rejected rasters to projectile-only
FOLLOW draws. Both anchors were outside the viewport in every case.
The earlier bounds check excluded FOLLOW, so these draws reached the
renderer. With the retained camera, endpoints, launch projection and
66-by-61-pixel envelope radii, all 105 would reject before animation and
raster preparation. The read-only replay matches the eight retained
world, framebuffer, palette and route samples.

An ordinary-input camera holdout pans to (520, 790) before the same
eight cuts. All 105 FOLLOW raster rectangles intersect the viewport
there, and none satisfy the proposed rejection. The world and route
samples match the neutral-camera run. This checks the candidate
geometry against the retained cartridge's draws.

The shared renderer now uses that proof for projectile-only FOLLOW
draws with signed-16-bit geometry and no trail-pick output. The current
projection samples the fixed segment from launch to captured target,
including child births. The existing frame and child-offset bounds
cover that segment. Calls that collect trail picks still use the
existing path to produce painter keys. The sanitizer fixture exercises
every clip edge, a partially visible frame with an offscreen anchor,
a segment crossing the viewport, combined parts and invalid geometry.
An authored differential checks 65,536 trajectories over 18 sampled
ages each. All 40,845 rejected trajectories produce zero blits,
including the synthetic child offset case.

Four authored ARM cases run 64 draws each with the actual Mode 4
blitters. Both builds produce identical framebuffer hashes.

| Draw case | Baseline cycles | Candidate cycles | Candidate culls |
| --- | ---: | ---: | ---: |
| Offscreen FOLLOW | 289,753 | 11,792 | 64 |
| FOLLOW crossing with both anchors offscreen | 334,633 | 341,673 | 0 |
| Visible FOLLOW | 309,345 | 309,345 | 0 |
| Visible FLY | 309,345 | 309,345 | 0 |

The crossing case exercises the expanded bounds and draws the same
two pixels. The far-off case finishes with the original blank page.
The mixed fixture uses 82 far-off draws out of 426, plus visible
horizontal, reversed and vertical crossings. Both optimization
levels reproduce eight changed pixels and the same full framebuffer.
The speed-optimized build uses 2,225,418 versus 1,929,054 cycles
(13.32% fewer). The size-optimized build uses 2,320,682 versus
1,981,509 cycles (14.61% fewer).

Measure current cartridge frame pacing in sustained battles with
both camera positions before using these figures for FPS claims.

| Local receipt | SHA-256 |
| --- | --- |
| Outpost trajectory and call-site replay | `a7b0ccfd5924d5532996fd2617fc5875e5e7114581b5bfca6c6e0b0b7346464f` |
| Outpost expanded-bounds replay | `a153287f5b1bab6cd66816882bd871de952f0da0bdaad7605349661f958b7337` |
| Outpost panned-camera holdout | `13eb1378441c2b5db88b293f8395922102e15e5bf1b14424064db644a47e062b` |
| Authored sanitizer differential | `5df6921e9d8f33c890120803459214758eac60be24710d39e8acc3012836db98` |
| Authored ARM fixture | `78a0d1de6a2f45becac0dc801429ae3b6f45f843ba693fda14eefc50538e2103` |
| ARM timings and framebuffer checks | `9467874673ce94a1e895ee6342abe61e102386f7123241df5d812d000395f831` |
| Authored mixed ARM fixture | `99d105fa7a9374df1346a7f47dc33b6fa1da5a34c18074f8e15b8681a53509c1` |
| Mixed ARM timing, speed-optimized | `79e1028622e952f99501695fc028f1fdf0c5a307c03d4a9b9b56f46eb99f100d` |
| Mixed ARM timing, size-optimized | `09f45e87190d75a774cdd5dafbf6b9ee8fd27104f3a8ce92203ff004266850c7` |

## Outpost resource sprite costs

The ordinary late Outpost repeat matches the retained world,
two-page framebuffer, palette and route observations across 16
updates. Its five recurring painter calls to the byte sprite
compositor resolve to four mineral sprites and one geyser. The
mineral calls account for about 59,514 guest cycles per update and
the geyser for 9,750. These are measured call costs, and the
[earlier optional packed mineral path](2026-11-13-static-mineral-word-draw.md)
gained about 695 cycles per update in its 1,000-update Outpost trial.
The local census receipt SHA-256 is
`b2f13dc9e64ba200cfb8db0190f6dd355f02271b93a5e6ef4cc3b56cb38c4331`.

In the aligned word path used by all three regular race cartridges,
the compositor now uses the loaded pixels for both opaque and mixed
groups. Each byte with a zero source pixel keeps its destination
pixel. The zero-byte test removes false matches for a source byte of
one following a borrow from a lower byte. The host clip fixture
covers this case, palette indices on both sides of bit seven and a
clipped row. A separate sanitizer oracle passed 431,697 comparisons.

| Paired ARM workload | Mean guest cycles saved |
| --- | ---: |
| Late Protoss Outpost, 16 updates | 0.47% |
| Terran Outpost, 12,000 updates | 0.41% |
| Terran Blood Bath, 12,000 updates | 0.21% |
| Zerg Outpost, 12,000 updates | 0.42% |
| Zerg Blood Bath, 12,000 updates | 0.23% |
| Protoss Outpost, 12,000 updates | 0.53% |
| Protoss Blood Bath, 12,000 updates | 0.26% |
| P10 fresh HOME victory, 9,489 updates | 0.12% |

The late Outpost window moves from 1,340,828 to 1,334,492 cycles
per update. Across 16 late Outpost cuts and 120 long skirmish
cuts, world state, both framebuffer pages, palette and route
counters match. The P10 victory run matches all 81 cuts and
final SRAM. It averages 1,490,148 cycles per update, or 11.26
emulated updates per second. All 30 campaign openings and 24
regular pause-save and fresh HOME continuations match, including
the saved SRAM bytes.

Each regular race build adds 16 bytes to IWRAM and uses the same
EWRAM as its control. Z10 ends at `0x03004fd8`, leaving 40 bytes
to the linker guard. Both tested Z10 busy windows record the same
minimum system stack pointer of `0x03006870` before and after the
change. The default source build and ordinary Showcase ROMs are
byte-identical to their controls. Enabling the existing aligned word
path in Showcase adds 0.19–0.36% cycles across three 16-update
Crossfire windows despite matching all sampled frames. Keep the
current build flags.

A separate bottom-up depth-heap trial matches all 64 late Outpost
and Crossfire cuts. It adds 0.30% cycles in Outpost and 0.32–0.49%
in the two busy Crossfire windows. Keep the incremental heap.

| Local receipt | SHA-256 |
| --- | --- |
| Late Outpost | `6fae47ec8c69727468a01f694aae349cfd5da4dde30eec01730eb0d4da465ca0` |
| Six Custom Games | `006a3c74eca0f111d58d5077c9dbfa1d545330ce338ab9bf0efd00fcbaac6e3e` |
| P10 fresh HOME victory | `471e69e4bb28a9398fada4e9ccdff1769d76ec9c075043ef22fad020462f576f` |
| Campaign openings | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Regular pause-save and fresh HOME matrix | `4ed2681e57b97484b59063126a3c2792d2a06f1725cc5049a434a94493bc925c` |
| Z10 stack probe | `876c0ef38c7772e6fbc89bcafe582e37edebaf2e40ce0c134cfec715e8024dea` |
| Showcase aligned word trial | `ff4d7840e8296a80799c2bc305c82804e66cfd45ec1e38abbd26f83002db34a6` |
| Depth-heap trial | `cd642403f5f4d390847692415e392983807de3b4748936f952b40007e7087e2f` |

## Opaque minimap raster copy

The minimap copies a 24×24 opaque raster to x=197. At that destination
phase, aligned source words can supply the interior of two consecutive
destination words. The shared compositor preserves both neighboring
pixels on each row and uses the existing pixel pairs for other
alignments and horizontally clipped draws. The sanitizer fixture
checks zero-valued pixels, row clipping, and both horizontal edges.

| Paired ARM workload | Mean guest cycles saved |
| --- | ---: |
| Late Protoss Outpost, 16 updates | 0.53% |
| Terran Outpost, 12,000 updates | 0.44% |
| Terran Blood Bath, 12,000 updates | 0.37% |
| Zerg Outpost, 12,000 updates | 0.44% |
| Zerg Blood Bath, 12,000 updates | 0.41% |
| Protoss Outpost, 12,000 updates | 0.56% |
| Protoss Blood Bath, 12,000 updates | 0.46% |
| Showcase Crossfire, 900 updates | 0.47% |
| P10 fresh HOME victory, 9,489 updates | 0.38% |

All 64 short Outpost and Crossfire cuts, 120 long-match cuts, 30
sustained Crossfire cuts and 81 P10 observations match their controls
in world state, both indexed framebuffer pages, palette and route
counters. P10 victory SRAM, all 30 campaign openings and 24
pause-save and fresh HOME cases also match. All four production links
retain their IWRAM and EWRAM endpoints. The default ROM's SHA-256
is unchanged. The P10 victory replay averages 1,484,463 guest cycles
or 11.30 emulated updates per second.

| Local receipt | SHA-256 |
| --- | --- |
| Short Outpost and Crossfire cuts | `37c1608cf8d4cd9ec090d64b2f939614b74900c3b4cc95727ce05b9ad3818d75` |
| Six Custom Games | `baa68e3319065264cd90a9c12b884b8450831e0c68bbb6174c8c9f8f4f48928c` |
| Sustained Showcase Crossfire | `172e28f0232f2acb585e83ce7d38f6cc716e2e080bc4d5e8d28ed291bc54ad57` |
| P10 fresh HOME victory | `23a834f176690f6991ff050c93be5eda8440f39b03dbfb3472c6092386de21a7` |
| Campaign openings | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Regular pause-save and fresh HOME matrix | `bb3384b1407224e1fbbefa07a00c0454a60f798efbd6409d0394cf3a3d8cfdf3` |

## Automatic gathering in two Custom Game openings

A fresh native Melee setup selects Challenger, human Terran and
computer Zerg. After Start, no in-map controller commands are sent.
The callbacks at runner steps 6,009, 6,509 and 7,009 show 98, 122
and 138 minerals, with 4/10 supply and moving SCVs. The retained
ROM, executable, MoltenVK and tape hashes match before and after the
run. The direct ares build launches from an unpacked source tree;
the standard wrapper's pinned Git revision check remains unavailable
for that tree. Callback steps are asynchronous to game updates.

The regular GBA Terran ROM launches Outpost with one Zerg computer
through its Custom Game menu. All four SCVs have mining orders
before any in-map inputs. Minerals rise from 50 to 450 over 1,000
neutral updates, with 4/10 supply and eight minerals in sampled
worker cargo. The observed openings share automatic gathering. The
maps, game type and timing units differ, so the relative mining
rate remains open.

The follow-up ordinary Blood Bath Melee runs select a Terran human,
one Zerg CPU and two closed seats on both engines. Native captures
show four starting SCVs, 50 minerals and 4/10 supply. After neutral
input, the first visible credit is eight minerals between observer
steps 6,309 and 6,409. The GBA preview first credits eight minerals at
neutral update 74. The visible native start is southwest and the
GBA first-seat run starts southeast. The observer increments are
asynchronous to native game updates. Matching the base position and
measuring the native gameplay clock remain prerequisites for a rate
comparison.

In a further GBA sweep, pre-launch waits of three, 12 and 13 updates
produce a southwest human base and, respectively, a northwest,
northeast or southeast Zerg base. These launches use ordinary menu
input. All three credit the first eight minerals at neutral update
74. The native human corner now has a GBA match. The native Zerg
corner and gameplay clock still need the separate RAM observation.

The native observer's cache-aware memory views match before and after
switching from the recompiler to the interpreter at runner step 4,200.
The isolated interactive driver waited for the transition response
while the observer waited for the next command. The observer now
flushes that response, as it does for its other command replies.
Available disk space delayed the first run of the corrected observer.

### Matched Blood Bath credit and elapsed time

The corrected native observer completes the same neutral tape. Its
snapshot beside HUD callback step 6,307 has elapsed-seconds value 9
and the frame shows 50 minerals. The snapshot beside callback step
6,407 has value 11 and the frame shows 58 minerals. The RAM cuts
occur at steps 6,309 and 6,409. The game clock is the big-endian
word at `0x800b9368`, identified in the earlier pause intervention.
Video callbacks and RAM captures are asynchronous, so the paired
cuts bracket the observed credit rather than identify its update.

The native Command Center is at (288, 1808) and the Hatchery at
(1760, 240). The regular GBA Melee preview launches through the
same map and race choices, with buildings at (144, 904) and
(880, 120), matching both native anchors at half scale. Its wallet
stays at 50 through neutral update 73 and reaches 58 at update 74,
after 2.470 seconds measured by the emulated GBA main-loop clock.
It has 114 minerals at 9.027 seconds and 122 at 11.034 seconds.

A fresh N64 session repeats the qualified Blood Bath input
through runner step 6,409. Its three RAM captures at steps
6,000, 6,309 and 6,409 and the video requested at steps
4,500 and 5,600 match the retained run byte for byte. The
controller tape also matches. The observer verifies the
hybrid transition and completes within its 64 MiB output cap.
This reproduces the existing first-credit bracket. The
exact native game update remains unknown.
The local input receipt SHA-256 is
`7eb5d4fea7d80b864a41711ddc944a1974170960dd794fc8195c2cb8928b0c77`.

A further native replay with the same hall anchors shows 66 minerals
beside elapsed-seconds value 19 and 90 beside value 28. The GBA
wallet for elapsed second 19 ranges from 186 to 194, and its wallet
is 266 throughout elapsed second 28. The observed warm-window
gains are 24 minerals on native and 72–80 in the corresponding GBA
clock brackets. The clocks were sampled independently. Separating
formation, travel, patch contention and depot contact remains next.

The retained native actor cuts show all four human SCVs targeting the
same mineral patch at elapsed second 3. One SCV is mining and three
are waiting. At second 9 one carries ore to the Command Center, one
mines and two wait. By second 19 the workers have spread to three
patches. This uses the human Command Center and owner from the
matched-corner lobby to identify the four live workers.

On the pinned GBA Terran Melee preview, the starting four SCVs are
already in their first work cycle on four different patches after
one update. No sampled update across elapsed seconds 0, 3, 9, 11,
19 or 28 has two SCVs with an active timer for the same patch.
That makes opening assignment and travel a separate difference from
the service timer. Check formation and first target choice in a
fresh native start before fitting a mining wait to the wallet.
The scoped comparison receipt SHA-256 is
`c5e68db3124a9f016ec2a2c1f0b801625c37818b3e5e6ac1bb4f48e3e8b851b2`.

A fresh ares boot follows the same ordinary Blood Bath input to
runner step 6,000. The read-only cut at elapsed second 0 shows all
four human SCVs near the Command Center, moving toward patch 49
with zero cargo and zero work timers. At second 1, each still has
the same target and has moved toward it. At second 3, one worker
holds the service claim and three wait with zero timers. The final
RDRAM cut matches the previous step-6,000 dump byte for byte. This
narrows the opening gap to initial placement, patch choice and
travel as well as service timing. The sampled cuts do not establish
the precise update at which the first worker starts service.

A fresh ares continuation of the opening narrows the service boundary
to runner steps 5,941–5,955. At step 5,940, all four SCVs still
target patch 49 without a claim or timer. By 5,955 one worker has the
claim and a timer of 74. At 5,970 it is still the only worker with a
claim, its timer reads 70, and the other three are waiting. The native
elapsed-seconds field reads 2 at all three cuts. Runner steps include
presentation cadence, so they are insufficient to derive an exact
simulation update or mining delay.

### Mineral targets spread between native seconds 12 and 15

A new bounded ares run continues the authenticated one-Zerg
Blood Bath opening without in-map input. At runner step 6,500
the native clock reads 12 seconds. Three workers still target
patch 49 and the fourth is carrying eight minerals back. At
step 6,650, second 15, SCV 69 claims patch 56. SCV 71 claims
patch 49 and another two workers still target it. At step
6,800, second 18, SCV 69 still holds patch 56 while SCV 72
holds patch 49 and SCV 71 carries eight minerals back.

This brackets the first observed change from patch 49 to 56
between steps 6,500 and 6,650. The player hall and initial
worker identities match the original comparison. The three
RAM and two video captures fit the observer's 64 MiB cap.
Input pins match before and after the run. The exact retarget
update and the condition that initiates it remain open.

| Local receipt | SHA-256 |
| --- | --- |
| Controller and observer tape | `dcbe9bc3e4f27a424c63054e7e1b0abe3e41b32f36a8d301f102670b44194271` |
| Step 6,500 | `4536053a302175e447877cc9c2e040e403e742cd7bd2f0ee094c5fbb0f996092` |
| Step 6,650 | `2722ffdf670313889b5df31d1b98ace167e70be2022fdb2d8f764d3a9c736457` |
| Step 6,800 | `81544fa4a0ae73f7f067bd5376d0356aad9389e8652c014172c2ef8cf0395a9b` |

The normal Terran ROM at `e09a574b` reaches the Custom Game setup
but has no Blood Bath Melee option. The earlier matched GBA opening
used the separately configured Melee preview. Qualify that mode on
the regular release profiles before using it as a retail-ROM parity
baseline.

The same match can launch on the November 23 current-source preview.
The human and CPU hall coordinates agree with the previous matched
corners. All four GBA SCVs have different patch assignments in the
first update, whereas all four native SCVs approach patch 49. At
elapsed second 19 the GBA wallet is 170–178, rising to 234–258 at
second 28. The native samples read 66 and 90. These are brackets on
each emulator's own clock. They separate opening assignments from
the amount of service time.

| Local receipt | SHA-256 |
| --- | --- |
| Fresh ares capture | `2444078b4f6be7fd5c45f774b34f974800e1eb0bc4e6f9e17b332d82a3827` |
| Three worker-state cuts | `64b46adf2d8384873cccc4091c580f25f1a8a9982ec6b92c6bcb954617b59eed` |
| Service-boundary ares capture | `b18f352f2e8943eef033f57db091a8c9f4e47ff4df780804e1fbfb748201c4ba` |
| Service-boundary worker observations | `938de5dff9f51ce22fa5288cf15e546417c05765565827bc60a72dcd70988754` |
| Current GBA Melee preview and native matched corners | `5e19d18c1661d85ecbf239a5078477c6c339cef5faf9fb1ee7d8920fc4f99590` |

| Local receipt | SHA-256 |
| --- | --- |
| Native input and binary pins | `548aa5ea47118533bf1fcdf7b680afe50aa127862e56dcf2e245cd9ed4328579` |
| Native commands | `144ab8c390815ba035e466ce4de93c7c4f6d973e1067cf1d7324f62013dfc581` |
| Earlier native RAM cut | `e565b0ff5a7492e12160119c7b8355f3839fffbe46855a6a9659016425040981` |
| Later native RAM cut | `542c61a39b1e97cbdaddb611cd0064a5181fc505bec99f8db6efcb62596130c7` |
| Matching GBA corners and guest clock | `42f96f2d1862b5767dca602bae3ed91f9781ef4dee2ee429ff4dd8fb9bd3ee46` |
| Native warm-window commands | `ff9b85523d8551992ca346f5a93c20955b39fc037853d9eb1bab0ae9b96cc3ad` |
| GBA warm-window replay | `990cff94a1db57dc67d2dafdd15f4a42ffa18951cb23e50ad446c4d04d49e762` |

### Packaged Terran Melee mineral opening

The packaged `7f7eab41` Terran 10 cartridge repeats the matched
Blood Bath start through ordinary Custom Game controls. The human
Terran Command Center and the Zerg CPU Hatchery match the retained
ares opening's corners, and both start with four human SCVs and
50 minerals. Automatic orders and eight-mineral credits are present
on both engines. At GBA elapsed seconds 0, 3, 9, 11, 19 and 28,
every observed mineral patch has at most one human worker with
an active timer and empty cargo. The retained native elapsed
second-3 cut shows four workers on the same mineral target,
with one mining and three waiting.

| Elapsed second on each emulator | Native observed minerals | Packaged Terran minerals |
| --- | ---: | ---: |
| 9 | 50 | 98–106 |
| 11 | 58 | 122 |
| 19 | 66 | 170–178 |
| 28 | 90 | 234–258 |

The native values are HUD observations from the retained ares
run. The GBA bounds sample every advancing update in each reported
second of the GBA guest clock. The independent clocks and native
callback timing limit the comparison to these opening brackets.
First patch assignment, travel and service each need separate
measurement before tuning the mineral clock. The packaged
ROM, matching ELF, observer and prior native snapshots were
pinned. The paired replay receipt SHA-256 is
`21750770c960ece1f6287870369f32f6cedf7b86965f471b30af20361f36a7a1`.

A fresh native ares run narrows first claim to observer steps
5,944–5,949. At step 5,943, all four human workers have zero claims
and zero work timers. At 5,949, one has a claim and its timer is
still zero. At 5,955, the same worker's timer is 74 and the entire
snapshot matches the earlier qualified cut byte for byte. The
observer reached the requested RAM cuts with the pinned inputs.
Its final check requires a copied display frame, so the initial run
exits nonzero. A second fresh run copies one frame in scratch and
passes the observer's exit checks. All three RAM cuts match the
initial run byte for byte. These cuts show that the claim can
precede the positive timer. The observer steps bracket the transition
on a different clock from simulation updates.

| Local receipt | SHA-256 |
| --- | --- |
| First three RAM cuts | `fd2efc7a7b45a6ec15f94e47d40e5921f6e1efe62aa2134aff12b7c840c35da6` |
| Qualified repeat ares run | `db375866fed03099398018aa1f05f3982691232f484c71f76b1d81e1c345667d` |

### Paired mineral assignments through elapsed second 18

A fresh ordinary-menu run of the packaged Terran cartridge
samples the same matched start through GBA guest seconds 0, 12,
15 and 18. All four GBA SCVs retain their four distinct initial
assignments on every sampled update. Every patch has at most one
working SCV with a positive timer and empty cargo. The GBA wallet
is 122–130 at second 12, 146–154 at second 15, and 170 at
second 18.

In the pinned N64 cuts, three SCVs still target patch 49 at
second 12 while the fourth returns cargo. At second 15 one SCV
targets patch 56. These are independent emulator-clock windows
with a continuous GBA sample and three N64 snapshots. They
show an assignment difference before any clock adjustment.
The native retarget rule and service cadence still need
separate qualification. The paired receipt SHA-256 is
`91ef12addc5949747d4b5280c1f35dbcaa3ba5b579ec1bdbe09dc9aa479d458f`.

## Protoss campaign continuation and replay controls

The exact routing release wins P5 after an ordinary pause save at
frame 2,700 and fresh HOME load. Seven checkpoints, 12 briefing pages
and five field pages match. Tassadar survives at (1,704, 1,771),
and the mission wins at frame 5,057 with its completion bit set and
zero trigger faults.

The regenerated historical P6 controller tape still matches its
published hashes. A paired input run on the historical and current
ARM ROMs differs on call 39, with the cursor beside overlapping
units. The older proximity picker selects the Zealot at (107, 84).
The shared foreground picker selects Tassadar at (108, 98). The
following attack-move reaches different units, which explains the
first retained checkpoint mismatch at call 943.

A P6 replay controller repair selects units through ordinary cursor
and button input, then checks the single selected unit before issuing
its next order. The regenerated host tape wins at frame 92,986.
The facility P10 ARM ROM wins on the same tape, using a fresh HOME
load after saving at frame 64,800. All 105 retained checkpoints match,
including 32 after loading, and SRAM is unchanged by the load.
Zeratul and Tassadar survive, the completion bit is set, and the
replay records zero trigger faults and unsupported actions.

## Carried P8 replay refresh

The regenerated host route wins P8 at frame 105,433. An ordinary
input run on the facility P10 cartridge matches the first four
named checkpoints. At the next checkpoint, frame 11,690, Fenix
has 240 HP in both runs. The recorded host shield value is 240 and
the ROM reads 76. The game frame matches and no trigger fault is
reported. The replay stops before its planned pause save.

The first host build omits the facility's canonical defense and
target flags, and it auto-acknowledges field dialogue. A follow-up
uses the facility source with those flags and records ordinary
authored-page input. It reaches the same checkpoint in the same
physical call and reproduces the 240 versus 76 shield difference.
Adding the facility collision-bucket and shared-sight flags produces
the same host controller log and checkpoint trace byte for byte.

Fenix's first observed shield loss occurs at physical call 10,271, game frame
10,031. The ROM Reaver at (653, 108) fires at Fenix at (528, 81),
reducing his shields from 240 to 140. The host Reaver at (669, 120)
fires toward (558, 182), leaving Fenix's shields at 240. At call
10,245 Fenix's position matches in both runs. The hostile Reaver and
three other enemy actors already have different positions. Earlier
complete P8 save wins remain qualified on their pinned older ROMs.

A paired actor sweep matches all four watched enemies through call
10,000. At call 10,163, frame 9,926, a Zealot's shields drop by 20
on the ROM while its host row stays at full shields. The ROM already
has a live friendly Dragoon trace aimed at that Zealot. The same
Dragoon stands idle in the host run before the later Reaver shot.
At the A press immediately after call 10,111, both runs have the
cursor at (567, 184) with the field menu closed. The ROM has Dragoon
slot 127 selected. The host has seven other slots selected. The
same physical command therefore changes different units. Earlier
selection checkpoints locate the split at the A press at input call
10,075. With the cursor at (467, 220), the host selects slot 112 and
the ROM selects slot 127. The next A expands the host selection to
a group while the ROM keeps slot 127.

All eight candidates occupy (468, 220) at the first split, with
the same standing facing. The update-only host camera stays at
(0, 0). The cartridge camera is at (344, 166). The click is outside
the host viewport, so the host picks by proximity and the cartridge
uses the foreground painter order. Rendering after each update in
the normal HOME controller gives the next click the painted camera
and records the authored field-dialogue inputs in the tape.

On the current P10 image from the weapon profile index release,
the regenerated tape wins after an ordinary pause save and separate
fresh HOME load. It reaches victory at frame 104,113 after 106,011
controller updates. All 40 named checks match, including 27 after
loading. The physical pause journal contains frame 30,949 and its
bytes remain identical through the continuation. Fenix has full
health and shields, the Hyperion survives, and the objective flag
is set with zero trigger faults.

The tracked HOST recipe reproduces every input and checkpoint of
the full replay. The tracked observer also passes a 12,000-update
physical prefix with matching world and key-edge checks, and
independently verifies both opening dialogue pages. The full
victory receipt uses the earlier observer that follows the same
recorded dialogue inputs and checks key-edge frames.

| Local receipt | SHA-256 |
| --- | --- |
| First shield loss and paired positions | `6b058071cd381a708a1c4055291b7ecf05837607e075fd0e4b5106af78293322` |
| Aligned host actor samples | `33d91b6e31b95d234c4b65fc33d26e8f2c735a8ac9f42648d11f4ce88285d107` |
| First tracked hostile shield difference | `17ac9dcec3142fd04a4d7ee339f1aadf2e2eb5e657ed3021b981ed827beaaf71` |
| Dragoon command selection bracket | `edf31bc12c13825883e9ea2986ff08ed12a73065e7fc2a25721fa315cf69aea4` |
| First point-selection split | `5a377e3adb3ef87054d832409bc4f675b794bfd74bff0deed6fe557d205b07d5` |
| Paired camera and actor geometry | `a77e9a7e8993f835ed4f984d0788583630307dbaa642def434a36e2aa45844e7` |
| Tracked HOST tape | `9b4a77eb57054e89f831bb744c943a6bb0f4bbf4de6c40079d512292d08a62e4` |
| Tracked physical observer prefix | `e0d1fd4c61d4f7e29ac742eeb5e3c48f62c7371680847ab689f55bf1e5cfbb49` |
| Current P10 fresh HOME victory | `da1a360279e72be1f69e2d99f19d09b9a2c772ca05d4bf13e12b0283c3b90b35` |
| Current P10 pause journal | `e074c498d189521570812bf9052b0254f9777d4d2690ebb1149316d71ee0d79a` |

## Selection and detector query candidate

The [combined draft](https://github.com/ggu/starcraft-gba/pull/724)
checks selection before the other live worker and hint flags. The
cached detector resolves its source slot only when a cloaked target
requires the existing detector lookup. The legacy observer retains
its normalized optical slot. Both changes leave game state layout
and command dispatch intact.

The 9,489-update P10 replay to victory falls from 1,517,236 to
1,506,386 guest cycles per update (0.72%). All 81 world, cache,
route, fog and display cuts and the full victory journal match.
The six 12,000-update Custom Game replay pairs match 120 semantic
world, route, palette and video cuts. Mean guest cost falls 0.17–0.25%
across the six replays and 0.26–0.50% in their slowest 600-update
intervals. All eight normal and preview RAM endpoints match the
control. Forty-eight ordinary start/save/reload cases also pass across
both variants, all four products, both maps and one to three CPUs.
The full default suite passes on runtime `6f9679de`, including 1,176
Python tests with 35 skips and the native sanitizer checks.

## Wasteland selection and Terran 7 continuation

Two isolated runs of the authenticated USA N64 ROM use the same
ordinary input sequence to launch Wasteland. The callback at runner
step 8,999 is byte-identical in both. One branch holds R for ten
runner steps beginning at 9,000, then releases it. The input log
contains five pressed polls. At callback-observed step 9,999, five
Marines have green selection indicators and the HUD offers Hold/Move,
Stop and Attack. The neutral branch shows no selection. Both read
40 minerals and 10/18 supply. These callback steps are asynchronous
with native game updates.

On the exact merged Terran GBA ROM, ordinary A acknowledgements
clear opening transmissions in both branches. A clean R press and
release selects the same five Marine slots, leaving the five SCVs
unselected. The selection and different command band persist at
frame 1,102, 1,000 advancing updates after the release cut. Neutral
input in the control leaves selection empty. Both show 40 minerals
and 10/18 supply at the before-input, release and later cuts. This
compares the observed selection behavior across systems. Their
runner steps and input dispatch timing are measured independently.

Terran 7 on the same ROM wins after an ordinary pause and a fresh
HOME load. All 15 checkpoints match the pinned input tape. The save
is made at frame 2,289 and victory occurs at frame 3,931. The
victory journal survives another fresh readback. The draft runtime
repeats all 15 checks and produces byte-identical pause, victory
and readback SRAM.

## Wasteland camera and fog

A fresh native Wasteland launch holds the analog stick to the right
from runner step 9,000. At callback step 9,049 the viewport box has
moved from approximately x=32–72 to x=64–103 in the minimap, with
room before the right boundary at x=163. A green scroll indicator
appears by the right edge of the 640×240 capture, whose content is
horizontally doubled. The step 9,099 cut has reached the right
boundary, and the main view shows unexplored ground in black. The
root-run steps schedule inputs and capture callbacks. Game update
timing is asynchronous to those steps.

A separate ordinary Terran GBA run holds D-pad Right after the
visible opening dialogue. The camera x coordinate moves from 0 to
100 at 50 advancing updates and to 300 at 150 updates. The cursor
remains near x=120 on the 240-pixel screen until the viewport
clamps at the end of the map. At 150 advances, 25,171 of 25,920
scene pixels use the dark fog palette index, up from 3,782 at the
opening. The neutral control stays at 3,782. The neutral and pan
branches match world, visible and explored hashes at each cut.
The comparison confirms panning into darkness without revealing
it and records the difference in cursor placement during the pan.

## Bounding the other steady costs

The combat audio budget rejects five of 50 attack and death sound
attempts in 48 Crossfire updates. The late Outpost and P10 replay
samples record no calls to the effect emitter in 16 and 18 updates
respectively. All 82 observed updates match the retained world state,
framebuffer pages, palette and route counters. Skipping the visibility
and actor lookup once the budget is full has little opportunity in
these cuts. Keep the existing event order and focus on larger costs.
The audio census receipt SHA-256 is
`be2fcb108db8afa9d5de25652adab719ab57be685eb1588ade9b8d0fc6c714e2`.

Earlier read-only target sampling on the same release observed 409
queries and 1,899 owner-cell visits across three P10 updates. Of those
visits, 1,765 were empty. A row bitmap pruned the empty columns but its
upkeep erased the gain. Sparse clearing reduced the overhead and still
raised mean busy Blood Bath cost from 811,651 to 812,742 cycles. Keep
the stock index and use a broader movement workload for the next trial.

A separate read-only contact census covers five exact P10 updates.
A uniform radius bound for each verified flight layer proves that
33 of 701 scanned buckets cannot intersect any of the eight probe
positions. Those buckets contain 76 of the 2,182 visited candidate
nodes. All 134 modeled live queries have a valid layer bound. This is
an upper limit on the nodes removed by this uniform-radius geometric
test in the sampled workload, before paying for the bound itself.
The broader movement and collision traversal remains the priority.

### Collision bucket footprints

Eight read-only P10 cuts on the opaque minimap release model 175
uncached contact queries and 2,484 candidate visits. The existing
actor-specific axis test rejects 1,850 of 2,309 same-layer candidates.
There are no opposite-layer visits in these cuts. Within each update,
673 visits repeat a bucket whose occupant records still match.

Bounding the occupied points inside each bucket can exclude 416
visits with the verified layer radius, or 1,062 using the largest
known radius in each bucket. These counts model the scan before
its full-mask early exit and exclude maintenance cost. The trial
stores local extrema on the current bucket head, widens them
after moves and rebuilds them for each orders pass. An asset-free
sanitizer oracle passes 10,000 lifecycle mutations and compares
30,797 eight-direction masks to the existing point queries.

The trial exceeds the P10 IWRAM guard at `0x03005138`. Moving
bucket movement bookkeeping to ROM links with 480 additional
EWRAM bytes. It matches all 16 late Outpost cuts in world state,
both display pages, palette and route counters, but raises mean
cycles from 1,327,411 to 1,384,514. Keep this trial local.

| Local receipt | SHA-256 |
| --- | --- |
| Occupied bounds census | `6bc8ad020d02d4ba7c5deebd10231c31699ade418e25c5bae9b46bbdc40bd67d` |
| Late Outpost pair | `97e3e2ca5dd5df38d7fb8e5ae99d410f86edcae3474c85a685c27d5d547839f5` |

### Collision table layout and lookup count

The authored geometry table covers 210 radius and speed
planes, each with 65×65 offsets. An offset outside the
strict radius-plus-speed axis bounds blocks no steps.
Removing those zero cells gives a 333,410-byte table
including plane offsets. Padding cropped rows to powers of
two simplifies row addressing and uses 463,016 bytes.
Grouping by speed allows one stride and origin lookup per
scan and uses 561,950 bytes. Using a 64×64 core and a
129-byte edge for the only plane that needs it uses
860,289 bytes.

The authored ARM batches each read 512 eligible table
entries. The answer digests match in every batch. The
values below use mixed radius and speed
queries, with each baseline compiled against the same
test loop as its candidate.

| Layout | Bytes including directory | Existing cycles | Trial cycles |
| --- | ---: | ---: | ---: |
| Existing layout | 887,250 | — | — |
| Cropped square | 333,410 | 23,078 | 28,205 |
| Power-of-two cropped row stride | 463,016 | 23,078 | 26,669 |
| Speed-major, groups of eight | 561,950 | 21,160 | 21,491 |
| Speed-major, groups of 32 | 561,950 | 20,680 | 20,387 |
| 64×64 core and separate edge | 860,289 | 23,590 | 24,117 |

In a read-only study of Outpost frames 11,100, 11,105,
11,110 and 11,115, the returned ROM masks match the
modeled traversal on all 129 observed scans. World, both
video pages, palette and route counters match the retained
replay. The 1,511 modeled node visits produce 309 eligible
geometry lookups, with one to six per scan and no repeated
geometry tuple within a scan. A bucket can be skipped if
its whole cell lies outside the directions that can still
be blocked under the non-increasing-distance rule. That
test can skip 12 of the 453 occupied visits and 56 nodes
before paying for its own arithmetic. These bounds apply
to the sampled replay and include the existing full-mask
early exit.

The 32-entry benchmark's grouping benefit has no measured
counterpart in these four cuts. Keep the layouts local and
look for reductions before the geometry lookup.

| Local receipt | SHA-256 |
| --- | --- |
| Cropped and padded authored timings | `4f68da8f056bf8f79e8373156151260e0ae206d4757fb622c76de67ee13bcacc` |
| Grouped-speed authored timings | `2f16480d19388ae3ae2bd2bcc1a33710632e61b131d2c6f4b2e5531f48fa9b91` |
| 64-wide stride authored timings | `8bb53ecb083c6b5e658a92db4bf05451a3fdc03f2da2a70c5f1e3dfc236dd602` |
| Observed-mask Outpost census | `80b842424e9269210dd782ee43a7229921b5b645fa04fc994bc1b557f782c75d` |

Hoisting the collision mask table's speed offset removes two
redundant axis checks after the radius gate. The tighter lookup
matches both 16-update P10 windows and all 81 victory-tail cuts.
Full-tail mean changes from 1,506,238 to 1,506,445 cycles. Keep this
table-addressing trial local and focus on the repeated traversal.

An isolated audio probe suppresses volatile snapshot writes whose
value would be unchanged. It matches world, video, palette and
route counters at the 32 attribution cuts. The early observer scope
rises from 37,420 to 40,964 guest cycles and the late scope rises
from 41,537 to 45,064. The conditional scan stays out. Future audio
work needs to reduce the duplicated actor traversal itself while
preserving ordered event admission and slot reuse.

A point-route trial skips the already discovered predecessor while
keeping seed handling and the order of the other neighbors. The
route sanitizer suites match the independent BFS and their compaction,
parking and resumption cases. The canonical Z10 link exceeds its
IWRAM reserve. Placing the loop in ROM in both the control and trial
allows a paired 3,600-update Blood Bath comparison. Across 387 route
builds, all 31 sampled state and video cuts and the exported SRAM
match. Mean cycles are 575,794.18 and 575,794.06. Keep the trial local.
The archived late Z7 battle still lacks a current paired measurement.

The later Protoss Outpost contact window permits a stricter search
bound. Once a non-egg target is eligible, cells whose nearest point
is strictly farther can be skipped. The diagnostic finds 7,088 of
15,824 live node visits in those cells across 16 updates. The short
trial saves 2,656 cycles per update with matching cuts. The longer
P10 replay costs 0.60% more and the canonical Z10 link exceeds the
IWRAM guard. Keep the current target search.

Using an existing timing cache for trace startup calls leaves the
same 16-update mean within 47 guest cycles. An independent 128-byte
animation-index cache cuts the short battle mean 0.29%. Its full
9,489-update P10 replay saves 0.02% and matches all 81 cuts and the
final SRAM. Keep both trials local. Use an active battle to measure
which repeated overlay walks can be elided before adding storage.

| Local receipt | SHA-256 |
| --- | --- |
| Control P10 replay | `2fc3cde69e257772fc454bbf2a2c1c13658589414757af09f3def32563045bc2` |
| Candidate P10 replay | `c85b93c6530472558c54ec47bc07c3c61f3d56eb0f095e72a00747fb3fe44503` |
| Candidate instruction attribution | `c371ce2b4116bd83627ceb9f7b60e23dbb98f062f7dd280ae866108f1169dc7b` |
| Contact census | `c9ecf168bd2d1896b97ae87b9ff7318bf0029f4b7bc5d77c7c9a511da6eb9644` |
| Prepared host comparisons | `85fb61a1f5e7c15023bb464ec61f1ff08e91d4b45d5e88ced00d95420638192b` |
| Protoss chapter starts | `b4d9c22777ae2dba696a5a03e78f23931e2c7b74499e8a9e84cb42796fcb8849` |
| Normal Protoss Custom Game matrix | `b5c41de469d331b5adaa3bb1821afa750105b89acf1c72987e7cd702be67703b` |
| Preview Protoss Custom Game matrix | `076ee6a8e277f2970dde40bab96837224908bd1bda380a0a139da6254bc1c4b2` |
| Audio snapshot write trial | `d172e631be8bfa2bcee1a2b9408a873e06849d0d0e67a6cce75876baf1a9ec86` |
| Three-AI Custom Game opening pace | `d3ca4076caf0274ad6723017b1eea3ae1eedb72db2c316c7e68cdce9edfb61e0` |
| Six later Custom Game replays | `d639899cc4ef2f59be65aebed7473a56244170d7311306c31196b8c6f1d54a24` |
| Three-AI Outpost attribution | `8288f35449fffd6f5da6442fb2f99f63c7922ad5f18c7937d7bd5ccf801624a9` |
| Both matched Outpost cut files | `ae47fa6bed92fc8d3399c15fc0b5c8cb2d6988f45decf6b2c86242fc8afa7e4b` |
| Native Wasteland before R, both branches | `8adb1b58ce96670e72c3b676fb51b8c641c25548a1d02f069139c1dcb0981c00` |
| Native Wasteland selection capture | `01ad7c9ffcd1cf69c101a85963bf95b3ceec8cab6a51b71c29501f17fd96bd43` |
| Native Wasteland neutral capture | `159d64db1ec8765d869f9492ececd6f1d5b6f349b2a29ba10c13f62035b4cd86` |
| Current Terran Wasteland selection and neutral pair | `658041c8810e7c79eb83dc922dc740ba9efa01b482b82e85cf17490808e56ef7` |
| Current Terran 7 fresh-SRAM victory | `e5843d9a9a1d8683dbfd46f230aa9232b70e686a964fa916cd320dc16ffcc551` |
| Point predecessor ROM-placement control | `c7ce279873e286c66d5350a2c3ef162dc90625a2525c083797fcd5dd234c9176` |
| Point predecessor trial | `fe3c3b42273aeda085567964ac667700dda3ce446f8f25e90b8677942848cbdc` |
| Combined Protoss 10 victory replay | `2762fc00d29327d87fdee95389f0359c0169541e09b58397afd8ebbe2d6d91ca` |
| Matched six-game control with video and world cuts | `1e90c488704968e50765b9323a94cd36a334b30bd7fb5d1d39ed698d09837e39` |
| Matched six-game combined trial | `0c0da0e5b1ba92ae4ea74bf2bebe1d2cab1eb0833b526ce03291d6c2d18178be` |
| Candidate 48-case cartridge save/load matrix | `7710d185af7a0bfeeafcf15d89d17e8c7544c939ef32ef372beb08f94ce48df1` |
| Draft Terran 7 fresh-SRAM and victory readback | `414b31ad903b4950e26f52c92bc1a4369dc7f113fae4479ed617aa16c61ffc25` |
| Native pan controller polls | `eee02f3618135729d384e64d5360b61572e32e4a33f5fe57b995b89665b87c86` |
| Native pan callback windows | `7984d3e7a587debad852c996819f3b2640bd168f12bee41ba23deb1b0e9a6dc9` |
| Native pan before the viewport reaches the edge | `d28762bce485fd7271c6d06344a82c54419847895486d40e16d10d730801079a` |
| GBA pan and neutral comparison | `4d3ee8b3865cb37d2d21abd0fdf752b3173696e9f344cc8fbf0310b0f8e97f5d` |
| Target-bound live node census | `446d790031d75a50d915fc9a1e439f43cee93c8576d3a40988c761f62f328f36` |
| Target-bound full P10 replay | `6adc414f046423faf50b672d3e5cd14d86fb13fe3f40f7c44c4bfaeb3658c8f3` |
| Indexed animation cache short battle | `37908f901f19d4081e8bba9a1c38f9658fec6785cdb67ee3b0840a4f500c441b` |
| Indexed animation cache full P10 replay | `e07765af34580f7e451d64150d591ae8439d23a786c4c12a5b5a1a959ce1a855` |
| Visible Crossfire instruction attribution | `0b001e9a886f937fa8b553f2a85fea6cb83107f7762ae304116c4ece51951545` |
| Matched profiled and unprofiled Crossfire cuts | `50928c356289710d94fa8c52fbf83080ba87468224ce79eecfb8779070e63954` |
| Crossfire attack-pose call census | `948d1c7e09a856e875f7b8e5f8689c9a4c553578eb3a4567763cb1d0074127b5` |
| Selectively packed attack-pose trial | `9ba22c1ff4633172913521b98da94dc16954bb72ffc76237926026208c16e7cf` |
| Eight packed-pose normal and preview link comparisons | `8db8ffae0bb20adcb6300317b9bae851a38742eb61058ad27a91e9d964e375e2` |
| Documented Showcase target, packed poses off | `e8726e5f828f8067e6f676ed7ff9398885f68a1c3a648458e369d4f32e8d9fce` |
| Documented Showcase target, packed poses on | `9685250f05a786e39fdeab95cdc836ff544fcfc535641338d64776f774e52431` |
| Sustained Crossfire control | `77703a54ac7c7f098ae4574414f8ccbd8291a7d0a1b4d8bd135a4724a3e7659b` |
| Sustained Crossfire with packed poses | `326942ba0facfbab4e50246570209565f7dece4247f2d0e110fc7d2393f6998d` |
| Packed-pose 48-case ordinary Custom Game matrix | `395252f6b13bf27abef890bc131d30f24ac40237f96f1fce454a9abe73885514` |
| Packed-pose six Custom Game pair candidate | `761eb3f8eb98e42cc9bc25ec6be4e80764a01d3b18744a20c09556248c6119a0` |
| Packed-pose P10 victory | `347d19a29195330e6c5ef33eb5169097af4b144127bade4c0ef273b7a38d3d44` |
| Packed-pose Z1 victory | `47174756fc67adc21274df4558995723a3e41011c3419ee9094feb60ea4b0a75` |
| Compact pair compositor in ROM | `734fda9e01a765f303d22b3f204e2e95efb5f5e89252aa2d3fecea7158d0260a` |
| Compact pair compositor with IWRAM inner loop | `f56aed0a7a3fa12fab3675e62a42edebd029ae957029518ec2497075c3ba97b4` |
| Shared byte compositor, 32 Crossfire cuts | `826089da58fe0bc2ac43c9500adc5c2a60445de8d9a6306b617d55408eace57f` |
| Shared byte compositor, 900 Crossfire updates | `906978a6180de3d72e07ac6f131a724c3dbd155c734534e3fcf0c14dc4c3bcf2` |
| Shared byte compositor, six Custom Game replays | `5cb807685223cc20a737c5b64c49686ef6f0416327e5da09187243d03825549f` |
| Shared byte compositor, current P10 fresh save victory | `e67cb892b874b383e11c915661bc24ae209c6f29b44702e50c158105e60a8a6d` |
| Target-column-only 16-update Outpost candidate | `ce1619d53c2536b84e052a3eb330f9391a2a9e52d07cc6ece7d050e495dec350` |
| Combined indices, sustained Crossfire | `254ed62ad2ad629968f5e579169dc591e75fc5bb26ac3d9b7fecd7c361fc25b5` |
| Combined indices, six Custom Game replays | `a9e551497e4cce09b6f4254111a6dc6bec05e8af7255fed115d689ac7c3f15ec` |
| Combined indices, P10 victory continuation | `c207a602a7034dd202789d6ee14be793eff33fd7716bb96295a750a36f724d40` |
| Combined indices, 48-probe Custom Game matrix | `a57acb6f8e1e80cd470ac1a16ace2db118c078b4f79081bd17901f5c9e066e6a` |
| Combined indices, all 30 campaign launches | `20cd9a6a4f0c3650ad5d1a529dfe1b72c73a765481cfec6102fee2a5e1a38f7c` |
| Combined indices, stack observer | `6b11f0b451904a1665dbcf6a95b688a10c2d83684996f7eb4fb838b8b7027e33` |
| Mode 4 page and overlay coverage, Crossfire | `ceec52e00a69424d262cc67b58a80eed28b1198f58104bc1b43f5c93d6d41400` |
| Mode 4 page and overlay coverage, three-CPU Outpost | `edd555b039873e21e7ea49142e567325046a2ca758ce6daf9b39db038f8afbb0` |
| Mode 4 page and overlay coverage, P10 | `ab58f706404b5936c65c848ab860de63aa37ff8301f6c3fd04976a61d072876f` |
| Collision table addressing, P10 victory continuation | `85167f6a4b7092138f5896abd31e17715c30d024c0e234880a2173d0d3827ac5` |
| Native Challenger opening tape | `341ea961382f8e368a90b7e084a63afe8267da23c1fc59dd26ed37183e6bfa9f` |
| Native Challenger final observed opening frame | `1f802e921166a464433def5394c2f64b6585d504c1e7f7d9754754722ae76fc1` |
| Current GBA Outpost gathering continuation | `b0c29b94177124741107388941c4f4f7c930bc0c957a2f7714e86c56af0218b8` |
| Native Blood Bath one-CPU opening tape | `ed076af8ed5c8d8906f75d49fa8791ce3abe1b62a247072e132de0cd8e5805dc` |
| Native Blood Bath one-CPU opening frames index | `28324258b6de0f8ef51b63d545bb2aaccb08d1db64ad261de64e05e3915182e3` |
| GBA Blood Bath one-CPU opening | `9557929dff3d115dc2accece76081ee18bd46cb9c20c126a3591c440459aa034` |
| GBA Blood Bath southwest start, three Zerg corners | `7ba669e5788c64207a9f290c883e79dd4d51b2dd8d2e33a621613afa6aba8ac7` |
| Current P5 fresh save victory | `9e23597c5b2d0e3198cf7b3138c4df93da07cec51d6bfc85849d78e29a56c88c` |
| P6 historical and current pick comparison | `5422d9d79a5d2295bac6237931be73a52cfb4f5ec88125e20107827a6a69a8af` |
| P6 ordinary ARM fresh save victory | `6f701acab49cb1c597100e03977c395eb74899bb769f0ce06fe11997bdd89348` |
| Carried P8 first facility-ROM replay divergence | `485e866d70da87e7f67e479329824ad2c4915c7d63d231d88093224f6f142c81` |
| Carried P8 facility flags and authored dialogue replay divergence | `734d1f5da3fcc85b2334b7dbfe1288ae1fe603249e76fe77d589c50b7aff756b` |
