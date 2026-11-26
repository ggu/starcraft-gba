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
writes guest memory only through the explicit `poke` tape command, which the
owner allowed for recording on 2026-11-26 (a run with any poke is staged
evidence: its manifest records `pokes` and `n64_staged`, and goldens built
from it carry the tag `n64-staged`). It has no host-input or debugger API.
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
The shared lock serialises every session's captures and native builds. A
session may name its own lock file in `SC64_CAPTURE_LOCK` to run its captures
alongside others'; emulation does not depend on host load (S1–S3, and the
field-map pair below: a capture run in parallel gave the same per-tick hash
chain as one run alone), but check a parallel run against a solo one before
relying on it, and keep to two of your own captures at a time.

The frontend counts every byte it writes into the run directory. `capture.py`
passes `--output-budget-gib` (default 8) and `--reserve-gib` (default 5) to it
as `SC64_CAPTURE_BUDGET_BYTES` and `SC64_CAPTURE_RESERVE_BYTES`; before a
write would pass the budget or leave less than the reserve free (free space is
measured again every 16 MiB written) the run stops with exit code 10, so the
manifest's `ok` is false and the capture is incomplete (like other failure
exits, no `status.log` is written). A run that finishes records
`output_written` and `output_budget` in `status.log` (`output_written` excludes
`status.log` itself and a flushed cartridge save, which are checked before
they are written but not added to the total). Counted: every log and
data file the frontend writes (`*.csv`, `tap.bin`, `extra.bin`,
`ticks.raw`, dumps, states, frames) and the polls it writes at the end. The
check runs before every command, every 64 steps, before each tap record,
dump, state, frame and cartridge flush, and at finalization (which reserves
64 KiB for `status.log`), so the budget holds to within one command's log
lines and a flushed cartridge save (128 KiB). Outside the budget: the staged
inputs and `manifest.json` (written by `capture.py`) and the frontend's
stdout and stderr, which `capture.py` redirects into the run directory.

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
| `pulse ADDR BYTES OP VALUE STEPS MASK HOLD GAP` | Run exactly `STEPS` steps, pressing `MASK` (`HOLD` steps on, `GAP` off) only while the field, read after the previous step, meets `OP` against `VALUE`; otherwise press nothing. Never misses. Dismisses prompts that come at unknown times: `pulse 800F0C20 2 ne 0 N 2 3 60` presses Start only while a text box holds the pause halfword, so it cannot open the pause menu. Replies `PULSE step pressed value cpu_ms` |
| `poke ADDR HEX` | Write 1 to 256 guest bytes (an even number of hex digits) into RDRAM and into any data-cache line holding them, then read them back through the guest view (exit 8 if they differ). Logged in `events.csv`, counted in `status.log` (`pokes=`); refused before a `load`. For data only: it does not invalidate recompiled blocks or the instruction cache, so never poke guest code. A state saved after a poke stays staged: a capture that loads it records `n64_staged` and `n64_staged_inputs` |
| `region ADDR BYTES`, `region every K`, `region on` | Declare up to 16 guest blocks (1 MiB in all); `region on` opens `extra.bin` and fixes them. While `tap tick` runs, each block is written on every follow-up read (every `K`th tick change with `region every K`). Format in `capture_view.h`; `tools/parity_decode.py` joins the blocks to the tick's record and its hash chain |
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

| Option | Report |
| --- | --- |
| `--s1 RUN_A RUN_B` | Same-tape determinism over checkpoints |
| `--s2 RUN` | Clocks and a Marine build timer |
| `--s3 RUN` | Follow-step changes, link and alias faults, branch results |
| `--compare RUN_A RUN_B [--exclude-block HEX]` | Checkpoints at shared steps without requiring the same tape (a loaded state against the run that wrote it); says when one run ends early |
| `--watch RUN` | `watch.csv` changes per field |
| `--s8 RUN_A RUN_B` | Per-tick unit records and globals of two matches, per phase |
| `--opening RUN` | Each owner's unit types in order of first appearance |
| `--sweep RUN... [--ticks N]` | Each run's launch seed, owners by race and each computer race's opening order |

The run-pair reports (`--s1`, `--compare`, `--s8`) give no verdict for the
same directory or run id twice, a missing run id (unless `--legacy`, for
captures made before run ids), a log that does not match its manifest
digest, a loaded state with no recorded digest, or a run made with
`--unrecorded-state`. A run that did not finish cleanly makes the result
partial. `--s1` gives a PASS only for power-on tapes (no `load` or `mode`)
that end with `hash`, with matching verified end steps and a shared
checkpoint there, and never under `--legacy`. `--s1` and `--compare` also check each log
against the checkpoints its own tape schedules (`every`, `hash`, from a
`load` step), so a checkpoint missing from both runs is not passed over;
`--compare` says when a tape does not end with `hash`, when a run stopped
before its tape's last step, and when checkpoints or end steps leave
steps uncompared. `--s8` says when a run was not tapped to its end, when
the verified end steps differ, and when the tick log does not match the
frontend's `tap_reads` and `tick_changes` counters in `status.log`. The
single-run reports say when a truncated or unverified `ticks.raw` cuts
them short. `--sweep` leaves out of its totals any run whose coverage,
launch or loaded state it cannot establish (`--legacy` admits older
loaded captures with a warning), keeps totals per raw stride and flags
strided starting rosters.

`capture.py` records in each manifest where every staged state came from
(`state_input_source`: the run id and binary of the capture that wrote
it, which must have finished cleanly, not used `--unrecorded-state`, and
recorded the producers of any states it loaded itself). A state from a
capture made before run ids needs `--legacy-producer`; the manifest then
records `legacy_producer` and the reports treat the run as legacy
evidence. `--unrecorded-state`
exists for refusal tests only; it sets `unrecorded_state` and makes `ok`
false.

The pure helpers in `capture_view.h` have an asset-free test,
`make test-ares-capture-view`, which is part of `make test`.
