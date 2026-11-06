# Experimental interactive callback bridge

This separate experimental prototype keeps a fresh emulator process for bounded ordinary controller chunks. It has only the limited callback qualifications documented in the audit; it is not independently matched to a console. Configure a separate isolated build using `interactive.cpp` as `SC64_PROBE_SOURCE`, and review both files before testing. The existing scripted frontend is unchanged.

`interactive.py` checks the pinned source, owned ROM and exact MoltenVK library, holds the common native lock, requires 5 GiB free and bounds the child at 180 seconds. It copies the ROM and isolates the output/saves. The child accepts one ASCII line per command: `steps mask x y capture`, with 1–1000 steps per command, 14000 total, 30 seconds per line and 60 seconds cumulative input wait. Mask bits: A=1, Start=2, Down=4, C-Left=8, D-pad Right=16, R=32, C-Right=64, C-Up=128, C-Down=256; axes are signed -32767…32767. Capture 1 requests one callback copy, capture 0 advances without storing a frame. Responses are READY, OK, NOFRAME or FRAME; frame observations are asynchronous and are not exact game-update timestamps. It retains command, input and callback records, at most 32 images and 64 MiB of copied pixels. EOF or timeout ends the session; no savestate, guest memory or host event API is used.

A neutral smoke and fresh Protoss custom runs exercised this bridge; see `docs/audits/2026-11-06-protoss-native-reference/README.md`. The 30-second command and 60-second cumulative idle caps require a responsive visual supervisor. An outer 180-second kill is the hard bound if the core blocks inside a chunk. Continue to inspect output, timeout behavior, fresh save isolation and input logs for each run. No audio is retained.

The callback command bits are independent: D-pad Right (16) does not press C-Right (64), which is used by the observed Probe Move command. The asset-free mapping check can be compiled and run without ROM or emulator inputs:

```sh
c++ -std=c++17 -Wall -Wextra -Werror tests/sc64-ares-input/mapping.cpp -o /tmp/sc64-ares-input-mapping
/tmp/sc64-ares-input-mapping
```
