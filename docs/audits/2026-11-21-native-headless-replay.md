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

A separate native run appends 1,800 neutral steps and still observes
active Wasteland at 40 minerals and 10/18 supply. The older observer
requires its MoltenVK path in `DYLD_LIBRARY_PATH`. An isolated control
reproduces snapshot failure without that path. The extended run reports
GPU ready after 9,000 steps and captures one frame. It has no Raynor
loss or mission outcome observation.

A fresh November 23 ares replay extends the retained ordinary Wasteland
opening to 11,000 runner steps. It exits successfully with GPU ready.
The callbacks at steps 8,999, 9,999 and 10,999 produce three captures.
The last retains the opening group, 40 minerals and 10/18 supply. The
callback indices are runner steps. Native combat and mission-outcome
qualification remain open.

| Extended run receipt | SHA-256 |
| --- | --- |
| Input commands | `c8157dd05cd2a6d7d23ce98716316310ef5e42661eb950b591cad8df98b364cf` |
| Runner status | `35065905c90be56be3d2268def90bb60cfe0b3eabdc3cee5040d0887bbf82793` |
| Final ARGB capture | `a2f42c21b78c639ec361dc12b2bbc4e1f0bd20ca332dab8f9b9cbb7cbdf9d5d5` |

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

### Later Blood Bath worker orders

A fresh emulator continuation loads the retained ordinary AI-match
checkpoint with F3 and takes two F2 snapshots in an isolated config
and save directory. Gameplay controls stay neutral. The two snapshots
record the following native worker slot positions and order IDs.

| Native update | Owner-2 SCV slot 66 | Owner-2 SCV slot 67 |
| ---: | --- | --- |
| 19,444 | (233, 189), order 90, Command Center 65 | (299, 148), order 87, mineral 16 with claim |
| 19,868 | (276, 203), order 86, mineral 16 with claim | (299, 82), order 85, mineral 7 |
| 21,089 | (198, 187), order 90, Command Center 65 | (202, 208), order 85, mineral 14 |

Both worker slots have 60 HP in each observation. The current target
pointer at offset `0x5c` in the [native worker record](2026-11-21-native-mineral-service-queue.md)
resolves to an active actor in all six observations. A mineral
claim is a matching nonzero resource pointer at offset `0xcc`.
The sparse order samples and unattributed resource bank leave
the duration of each mineral service phase and the exact deposit
cadence unresolved.

| Local receipt | SHA-256 |
| --- | --- |
| Native update 19,444 seed state | `4db6e4cf342abd2af5b3409d6ad3b9f9298b1a12e529d5484bde09eed7eab7be` |
| Native update 19,868 snapshot | `f1ad28386562bc1da1ff518d88ef4fa4571dce872e2b5ca462443ad1a8ba1805` |
| Native update 21,089 snapshot | `64064866a7220e78d743fe3c034fd0f87d74a302f2bdca892dc7ccc4ea656d25` |

### Mineral pickup and claim release

Eighteen further neutral snapshots from the same seed,
spaced five to seven native updates apart, observe the
owner-2 mineral cycle at patch 16. At update 19,492,
SCV 67 has a live claim, no cargo, order 87 and phase 5.
The patch reserve is 388. At 19,497, it has eight cargo,
targets its completed Command Center in order 90, phase 1,
and has relinquished its claim. The patch reserve is 380
and its active flag is clear. That brackets the pickup,
debit and claim release within five updates.

SCV 66 targets the same patch without a claim at update
19,517, then holds the claim in order 87, phase 5 by 19,523.
SCV 67's eight cargo clears between updates 19,542 and
19,548. The resource bank is unattributed, so this bounds
cargo clear without resolving the exact bank credit.
The local observation log stores a hash for each snapshot.
The emulator overwrote the intermediate snapshots and
retained the final state.

| Local receipt | SHA-256 |
| --- | --- |
| Eighteen observations | `3a82517571f5c3b18cb9df36e64ec79bdbae84799d69ee25a29f316e7280b9ae` |
| Final native state | `3afd037e2b66a3b893cb558b3f00eba73f6e794f911915ba7c3374fd2fc492f0` |
