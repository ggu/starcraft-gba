# Fresh native headless gameplay replay

A fresh bounded run of the local ares interactive probe booted the owned USA
N64 game and repeated the retained ordinary-input Protoss custom-game tape.
The setup is Melee on Challenger, human Protoss versus computer Zerg. At the
first gameplay capture, the visible scene contains a Nexus, mineral fields and
a Probe; the HUD shows 66 minerals and 4/9 supply. The earlier reference
identifies the same setup and notes that the attempted later Probe selection
selected a mineral field instead. This is not a campaign mission or a Blood
Bath depth-overlap case. [Prior native reference](2026-11-06-protoss-native-reference/README.md)

The run completed 6,574 runner steps with exit 0, GPU ready and five 640×240
ARGB captures. The first capture at callback-observed step 6,199 matches the
retained first capture byte for byte. Captures at observed steps 6,538, 6,557
and 6,573 also match; the capture at 6,527 differs, so frame callbacks should
not be assumed synchronous. Runner steps are not calibrated game frames. The
replay verifies this bounded boot and visible gameplay route, not native
simulation timing, sprite identities, elevation, painter tie order, a composed
depth overlap, or game completion. No audio was retained.

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
| Input command CSV and emitted command log | `4f54b5eeb5d34a9bdd59bba0c561560393c9205a816408cf84c967f1ccb97f3c` |
| First gameplay ARGB capture | `b1ba71e34478cd093ddb0529534ab0093ed8d6ef735bf0e8bbc3bf2536198388` |
| Final ARGB capture | `14fde37f40100cbd2b1791dcec889d640ad32885da0508a583f1bce36b78e0a1` |

ROM, game images, saves and captures remain local and are not included here.
