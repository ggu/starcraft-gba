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
