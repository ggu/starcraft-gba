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
