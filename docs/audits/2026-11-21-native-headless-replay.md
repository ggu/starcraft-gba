# Fresh native headless gameplay replay

A fresh bounded run of the local ares interactive probe booted the owned USA
N64 game and repeated the retained ordinary-input Protoss custom-game tape.
The setup is Melee on Challenger, human Protoss versus computer Zerg. At the
first gameplay capture, the visible scene contains a Nexus, mineral fields and
a Probe; the HUD shows 66 minerals and 4/9 supply. The earlier reference
identifies the same setup and notes that the later selection highlighted
a mineral field. [Prior native reference](2026-11-06-protoss-native-reference/README.md)

The run completed 6,574 runner steps with exit 0, GPU ready and five 640×240
ARGB captures. The first capture at callback-observed step 6,199 matches the
retained first capture byte for byte. Captures at observed steps 6,538, 6,557
and 6,573 also match. The capture at 6,527 differs, consistent with
asynchronous frame callbacks. The receipts establish a fresh boot and visible
gameplay route. Simulation timing, live depth records, composed overlaps, and
game completion require additional observations.

The existing local `experiments/ares-headless/interactive.cpp` runner takes
`ROM OUTPUT_DIR interactive FRAMES`; its output directory must already contain
empty `saves` and `systems` directories. The retained `protoss-live-1` CSV
contains rows `begin,steps,mask,x,y,capture`: send the last five columns as
space-separated commands on standard input. Use a fresh isolated output
directory and set `DYLD_LIBRARY_PATH` to the directory containing the pinned
MoltenVK library. The retained neutral smoke first captures after 1,000 steps;
requests at one and 120 steps returned snapshot failure because no callback
frame was available. The runner powers on from reset and has no state-load or
sprite-memory inspection interface. An external observer or a separately
validated runner change is needed to inspect a saved Blood Bath state. The
existing wrapper also enforces a 5 GiB free-space reserve; this bounded manual
run used the already extracted pinned ROM read-only and wrote about 4 MiB to
isolated temporary storage. The local binary differs from the older manifest's
binary, so its own digest is recorded below.

| Receipt | SHA-256 |
| --- | --- |
| Owned USA ROM input | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| Local headless runner | `45d4dadbe233071efa6b6565bb16fa17809323609f5e27ff66417ecdadc974e5` |
| MoltenVK | `432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99` |
| Retained input command CSV | `4f54b5eeb5d34a9bdd59bba0c561560393c9205a816408cf84c967f1ccb97f3c` |
| First gameplay ARGB capture | `b1ba71e34478cd093ddb0529534ab0093ed8d6ef735bf0e8bbc3bf2536198388` |
| Final ARGB capture | `14fde37f40100cbd2b1791dcec889d640ad32885da0508a583f1bce36b78e0a1` |

The ROM, images, saves and captures are stored locally.

## Fresh Wasteland campaign opening

A second fresh reset used the ordinary `advance` schedule from the existing
headless `main.cpp`. Start pulses occur at steps 700, 1,000, 1,300 and
1,600. Down occurs at 3,900 and 4,000, then A at 4,100. The last Start
pulses occur at 5,000, 5,600, 6,200 and 6,800. Each pulse lasts ten runner
steps and is released between pulses.
The interactive runner received the equivalent bounded commands, with capture
requests at 5,851 and 7,051, and finished at step 7,200. It exited 0 with
GPU ready and two 640×240 captures. The isolated output occupied 2.3 MiB.

Both captures show the Wasteland Terran opening group and 10/18 supply. The
later capture shows 40 minerals. These observations reproduce the prior
headless opening and agree with the separate native Wasteland save-selection
record's 40 minerals and 10/18 supply. The GBA campaign documentation says it
uses Wasteland's original placements and retains the opening mineral grant.
Comparing the individual actors requires state observations for identities,
health, exact positions and simulation ticks. [Native save-selection record](2026-11-13-n64-t1-save-selection.md)

| Wasteland receipt | SHA-256 |
| --- | --- |
| Interactive input commands | `c170f2e38ed9cffa6cd0df4912929babd317b7c2484cd595a705bf4987e0fd6f` |
| Emitted command CSV | `3d9256471387180b6c3d805f7f0084d1752f4d84be7343c26c9056dcacce925f` |
| First ARGB capture | `ba0080b165a39d80b34bd0f00e74ce462bb72abb77e32f89d2355c6c1088c897` |
| Second ARGB capture | `df0da3a1853d11aa42df17addd32cd376d01886c6d6b97370622fba718d5c2b8` |

An independent local ares run on November 22 reproduced both ARGB hashes
from the same opening commands. It completed 7,200 steps and reported
GPU ready. A separate physical-button replay of the current Terran baseline
and route-compaction candidate matched all four sampled GBA world cuts. Both
recorded 10/18 supply at frames 0, 1, 29 and 30, and the 40-mineral grant at
frame 30. This confirms the two observed values across platforms. The native
grant timing, Raynor loss and the zero-actor defeat boundary remain open. The
paired GBA receipt SHA-256 is
`327919258a563a00e1fb8aff65bab6dddaa5de9be9b681ff6f53051735637e23`.

## Blood Bath state and render-row observation

A separate local observer restored an ordinary Blood Bath step-zero state and
read native RDRAM before advancing. Its initial dump has the 900-row,
312-byte unit-table marker at `0x802d2370`. The first row is at `0x802d2380`.
The unit sprite pointer at row offset `0x0c` and the native 36-byte sprite pool
at `0x80207c70` agree on the following records. Sprite layer is at `+0x0d`,
flags at `+0x0c`, and the renderer's anchor is at `+0x14/+0x16`.

| Unit slot / raw type | Sprite ID | Sprite anchor | Layer / flags | Painter key |
| --- | ---: | --- | --- | --- |
| 60 / 106, Command Center | 2139 | (128, 240) | 4 / `0x00` | `0x203c085b` |
| 61 / 7, SCV | 2138 | (80, 296) | 4 / `0x04` | `0x204a085a` |
| 62 / 7, SCV | 2137 | (104, 296) | 4 / `0x04` | `0x204a0859` |
| 63 / 7, SCV | 2136 | (128, 296) | 4 / `0x04` | `0x204a0858` |
| 64 / 7, SCV | 2135 | (152, 296) | 4 / `0x04` | `0x204a0857` |

All five are owner 2. The Command Center sprite pointer is `0x8021a93c`.
The four SCV pointers are `0x8021a918`, `0x8021a8f4`, `0x8021a8d0`, and
`0x8021a8ac`. The row-head entry at `0x80112cbc` points to the Command
Center; the entry at `0x80112cc4` points to sprite 2135, whose next links
visit 2136, 2137, and 2138. All four workers sort after the Command Center
because their native sprite Y is greater. The GBA ground queue applies the
same Y precedence across units and buildings. The N64 orders the four
equal-Y workers by sprite ID. Their screen overlap and last-image retirement
remain open harness cases. [Exact-tie replay cases](2026-11-21-fallback-world-depth.md#replay-cases)

The observer then ran 1,000 neutral runner steps and dumped RDRAM again.
The native game-update counter advanced from 0 to 271 during those 1,000
runner steps. At the second cut SCV 61 has order 87, phase 5,
unit and sprite anchor (58, 107), sprite 2138 and key `0x201ac85a`. Its
unit target pointer identifies a raw-177 mineral at (32, 80), sprite 2191,
key `0x2014088f`. The worker sorts after the mineral by native Y. The row
headed at `0x80112cac` contains worker IDs 2136, 2137 and 2138, and the
row headed at `0x80112ca8` contains mineral 2191. Their respective chains
end normally. A composed framebuffer mask and per-update target trace
would test overlap and reacquisition timing.

The observer is a local state-load and debugger-read modification of the
interactive probe, with a hard-coded owned step-zero save. It reported 1,000
completed neutral steps, 1,000 callbacks, GPU ready and no pressed A or
Start polls. The callback capture was observed at runner step 999; the final
RDRAM dump is taken after the run. Repeating this exact cut requires the
local prototype with state-load and memory-read support and the retained
inputs. Use a bounded `1000 0 0 0 1` interactive chunk and isolated
existing `saves` and `systems` directories.

| Local input or receipt | SHA-256 |
| --- | --- |
| Restored step-zero state | `ed9225feb4caa29de200475af24c435c85acddabdc2c9e352857bc8732f30fcd` |
| Observer source / executable | `db963c9583e38c21402d25004e8cfc185d3686b7d7f156c0953dba4cc48e3523` / `56bf45b372373cd504cf78ca246ffd1076a0164ba6853b68598ae581eee21251` |
| Restored RDRAM dump | `fa5e1c5f5d9bb107bc5524297169c623f0ab08764074d321b594ebf3a562e71f` |
| Final RDRAM dump | `280033561d72c5c4315312b2a32306ce6a94c1d98fc624514cdeacf5f444948e` |
| Commands / status | `3ebbcbf2715feaccec07242937ad34f6fd7850c642939a16ce41933e1a911899` / `461c9a789ab0a5e34cac361680beeebb390b7b45040a35c5b6a048639c0b9f9c` |

Record target pointer, order and phase on each observed game update in a
separate controlled neutral continuation before estimating target-selection or
reacquisition cadence. Isolate any combat acquisition case from mining's own
service and target-selection rules.
