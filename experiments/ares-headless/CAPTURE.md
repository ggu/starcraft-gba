# Parity capture spike frontend

`capture.cpp` is a separate experimental frontend for the parity-harness
spikes S1–S3. It boots the owned USA ROM in the pinned ares core, runs a
controller tape and observes guest memory read-only. It hashes the full
8 MiB cache-aware RAM view at fixed step checkpoints, taps live unit rows and
clock words whenever the per-tick counter changes, and can compare a tick
under the JIT with the same tick replayed in the interpreter from an
in-process state. It never writes guest memory and has no host-input,
debugger or state-file API. Results are in
[`docs/audits/2026-11-25-parity-capture-spikes.md`](../../docs/audits/2026-11-25-parity-capture-spikes.md).

The step, wall-time, snapshot, frame, input-event and output-size limits of
the older frontends are not present. `capture.py` instead holds the shared
native lock, requires 5 GiB free, checks the ROM, MoltenVK and ares source
pins, copies the ROM (and an optional 128 KiB cartridge save) into a new run
directory, and records every input and output hash in `manifest.json`. Run
directories hold ROM-derived data: keep them in an ignored private folder.

## Build

Use a clean clone of ares at `0aafd85789215e84e1e43415c07d4c88461b7899` in a
durable directory (not `/private/tmp`), and copy `capture.cpp`,
`capture_view.h` and `inject.cmake` beside it. The core keeps the
paraLLEl-RDP include folders private, while `n64/n64.hpp` needs them, so they
are passed through `CMAKE_CXX_FLAGS`:

```sh
R=$PWD/ares/ares/n64/vulkan/parallel-rdp
cmake -S ares -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DARES_SKIP_DEPS=ON -DARES_CORES=n64 -DARES_BUILD_OPTIONAL_TARGETS=OFF \
  -DARES_ENABLE_CHD=OFF -DCMAKE_PROJECT_INCLUDE=$PWD/frontend/inject.cmake \
  -DSC64_PROBE_SOURCE=$PWD/frontend/capture.cpp \
  -DMoltenVK_LIBRARY=/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib \
  -DCMAKE_CXX_FLAGS="-I$R/parallel-rdp -I$R/volk -I$R/vulkan -I$R/util -I$R/vulkan-headers/include"
ninja -C build sc64-ares-headless
```

Tools used: CMake 4.4.4, Ninja 1.13.2, Apple clang 21.0.0, macOS arm64.
The configure step writes the four generated `resource.cpp`/`resource.hpp`
files into the clone; their hashes are recorded in each manifest. Copy the
binary to a read-only path named by its hash before a capture pair, so a
rebuild cannot change one run of the pair.

## Run

```sh
python3 capture.py --binary BIN --source ARES_CLONE --rom ROM \
  --tape TAPE --output NEW_PRIVATE_DIR [--flash-input SAVE] [--mode jit|interp]
```

A tape is one command per line:

| Command | Effect |
| --- | --- |
| `STEPS MASK X Y` | Run `STEPS` frontend steps (one `root->run()` each) with the pad held |
| `every N` | Hash the full 8 MiB view every `N` steps (`checkpoints.csv`, `regions.csv`) |
| `hash` | Hash the full view now |
| `tap off\|tick\|step [raw] [rec]` | Read rows and clocks on each tick change and the step after it, or every step; `raw` also writes live rows to `ticks.raw`; `rec` writes `tap.bin` (clocks, the four bank arrays and RNG words on every read, live rows on the follow-up read; format in `capture_view.h`, decoded by `tools/parity_decode.py`) |
| `verify N branch\|same [full]` | Every `N` tick changes, replay the next tick in the other CPU mode (`branch`) or round-trip the state in the same mode (`same`) and compare |
| `mode jit\|interp` | Switch CPU mode through an in-process state round trip |
| `rawcheck` | Check the direct RDRAM copy against the core's own debugger read for all 8 MiB |
| `peek ADDR N`, `dump NAME`, `shot NAME`, `quit` | Diagnostics |
| `# ...` | Comment; parity tapes mark the command step with `# mark command` |

Pad mask bits: A 1, Start 2, Down 4, C-Left 8, D-pad Right 16, R 32,
C-Right 64, C-Up 128, C-Down 256, B 512, Z 1024, L 2048, D-pad Up 4096,
D-pad Left 8192. A fifth column from older tapes is ignored.

`capture_report.py` summarises run directories without copying guest bytes:
`--s1 RUN_A RUN_B` compares checkpoints, `--s2 RUN` checks the clocks and a
Marine build timer, `--s3 RUN` counts follow-step changes, link and alias
faults and the branch results.

The pure helpers in `capture_view.h` have an asset-free test,
`make test-ares-capture-view`, which is part of `make test`.
