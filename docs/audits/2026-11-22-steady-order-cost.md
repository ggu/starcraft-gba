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
The first damaging action and any earlier world divergence need
finer sampling. Earlier complete P8 save wins remain qualified on
their pinned older ROMs.

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
