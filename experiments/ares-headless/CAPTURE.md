# Parity capture spike frontend

`capture.cpp` is a separate experimental frontend for the parity-harness
spikes S1–S8. It boots the owned USA ROM in the pinned ares core, runs a
controller tape and observes guest memory read-only. It hashes the full
8 MiB cache-aware RAM view at fixed step checkpoints, taps live unit rows and
clock words whenever the per-tick counter changes, logs chosen fields on
every step, can hold a pad input until a guest field meets a condition,
and can compare a tick under the JIT with the same tick replayed in the
interpreter from an in-process state. It writes whole-machine state files
with the core's own serializer and loads one only before the first step of
a fresh process, and it can write the cartridge save through the core. It
never writes guest memory directly and has no host-input or debugger API.
Results are in
[`docs/audits/2026-11-25-parity-capture-spikes.md`](../../docs/audits/2026-11-25-parity-capture-spikes.md)
(S1–S3) and
[`docs/audits/2026-11-26-parity-scenario-spikes.md`](../../docs/audits/2026-11-26-parity-scenario-spikes.md)
(S5–S8).

The step, wall-time, snapshot, frame, input-event and output-size limits of
the older frontends are not present. `capture.py` instead holds the shared
native lock, requires 5 GiB free, checks the ROM, MoltenVK and ares source
pins, copies the ROM (and an optional 128 KiB cartridge save and any state
files) into a new run directory, and records every input and output hash in
`manifest.json`, including state files the tape wrote. Run
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
  --tape TAPE --output NEW_PRIVATE_DIR [--flash-input SAVE] \
  [--state-input STATE ...] [--mode jit|interp]
```

A tape is one command per line:

| Command | Effect |
| --- | --- |
| `STEPS MASK X Y` | Run `STEPS` frontend steps (one `root->run()` each) with the pad held |
| `every N` | Hash the full 8 MiB view every `N` steps (`checkpoints.csv`, `regions.csv`) |
| `hash` | Hash the full view now |
| `tap off\|tick\|step [raw [K]] [rec]` | Read rows and clocks on each tick change and the step after it, or every step; `raw` also writes live rows to `ticks.raw`, for every `K`th tick change only when a number `K` follows `raw`; `rec` writes `tap.bin` (clocks, the four bank arrays and RNG words on every read, live rows on the follow-up read; format in `capture_view.h`, decoded by `tools/parity_decode.py`). Options may come in either order |
| `watch ADDR BYTES`, `watch off` | Log a 1, 2 or 4-byte field to `watch.csv` on every step where it changes (up to 32 fields); `off` clears them |
| `until ADDR BYTES OP VALUE MAX MASK X Y [HOLD GAP]` | Hold the pad until the field, read after each step, meets `OP` (`eq`, `ne`, `ge`, `lt`, `changed`, `set`, `clear`) against hex `VALUE`, for at most `MAX` steps. With `HOLD GAP` the buttons are pulsed (pressed `HOLD` steps, released `GAP` steps) while the axes stay held. Replies `UNTIL step steps value cpu_ms`; a miss replies `UNTILMISS` and ends the run with exit code 9 |
| `save NAME.state` | Write the whole-machine state (the bytes desktop ares writes to a slot file) to `NAME.state` in the run directory. Under the JIT this changes the rest of the run, because the pinned core resets its recompilers on serialize: a run compared with one that loads the state must save at the same step |
| `load NAME [STEP]` | Before the first step only: restore `input/NAME` (staged with `--state-input`) and number later steps from `STEP`. The file must have this core's state size and header. The run keeps its current CPU mode (the startup mode, or one set by an earlier `mode` line). `capture.py` stages only states recorded in the `manifest.json` beside them (`states_sha256` of the run that wrote them), because the core trusts a state's payload |
| `flush` | Write the cartridge save through the core to `saves/Nintendo 64/game.flash` |
| `verify N branch\|same [full]` | Every `N` tick changes, replay the next tick in the other CPU mode (`branch`) or round-trip the state in the same mode (`same`) and compare |
| `mode jit\|interp` | Switch CPU mode through an in-process state round trip |
| `rawcheck` | Check the direct RDRAM copy against the core's own debugger read for all 8 MiB |
| `peek ADDR N`, `dump NAME`, `shot NAME`, `quit` | Diagnostics |
| `# ...` | Comment; parity tapes mark the command step with `# mark command` |

Pad mask bits: A 1, Start 2, Down 4, C-Left 8, D-pad Right 16, R 32,
C-Right 64, C-Up 128, C-Down 256, B 512, Z 1024, L 2048, D-pad Up 4096,
D-pad Left 8192. A fifth column from older tapes is ignored.

Names given to `save`, `load`, `dump` and `shot` are one plain component
of at most 64 characters (`A–Z`, `a–z`, `0–9`, `.`, `_`, `-`, not starting
with `.`) for `save` and `load`, and a saved state's name ends in
`.state`; existing files are never overwritten.
A state loaded in a fresh process continues byte for byte the run that
wrote it (S5). `load` is refused after the first step or once a tap,
watch, checkpoint interval or branch check is set, so every loaded run
starts the same way. Addresses are KSEG0 or physical hex; the frontend
reads physical offsets.

`capture_report.py` summarises run directories without copying guest bytes:
`--s1 RUN_A RUN_B` compares checkpoints, `--s2 RUN` checks the clocks and a
Marine build timer, `--s3 RUN` counts follow-step changes, link and alias
faults and the branch results, `--compare RUN_A RUN_B [--exclude-block HEX]`
compares checkpoints at shared steps without requiring the same tape (a
loaded state against an uninterrupted run), `--watch RUN` summarises
`watch.csv`, `--s8 RUN_A RUN_B [--legacy]` compares per-tick unit records
and globals of two matches (it needs both run ids unless `--legacy` is
given, and reports ticks without a follow-up read), `--opening RUN` lists each owner's unit types in order of
first appearance, and `--sweep RUN... [--ticks N]` gives each run's launch
seed, owners by race and each computer race's opening order.

The pure helpers in `capture_view.h` have an asset-free test,
`make test-ares-capture-view`, which is part of `make test`.
