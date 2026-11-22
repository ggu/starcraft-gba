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
battle as a second workload. The historical mode-0 tile path falls
back to Mode 4 for Custom Game and gave flat P7 timing. Its imported
audio and RAM limits remain relevant. See [renderer validation](../VALIDATION.md#p7-mode-0-tile-renderer).

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
