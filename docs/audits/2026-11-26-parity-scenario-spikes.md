# N64 parity scenario spikes S5–S8

These spikes test the scenario infrastructure the parity harness needs
on the N64 side: state files, closed-loop tape input, mission entry and
launch seeds. They extend the read-only capture frontend from the
[S1–S3 spikes](2026-11-25-parity-capture-spikes.md),
[`experiments/ares-headless/capture.cpp`](../../experiments/ares-headless/capture.cpp),
at ares `0aafd85789215e84e1e43415c07d4c88461b7899`
([build and tape reference](../../experiments/ares-headless/CAPTURE.md)).
Every run used the owned USA ROM (`e7d21880…b020`) under the JIT unless
stated, held the shared native lock and ran alone. ROM copies, cartridge
saves, state files, tapes, RAM dumps and frames stay in an ignored
private folder; this note keeps only numbers, hashes and verdicts. The
frontend never writes guest memory: every change of game state below
comes from controller input or from loading a state file written by the
core's own serializer.

| Spike | Verdict | One-line evidence |
| --- | --- | --- |
| S5 state files | PASS with one rule | A state written at step 20,000 and loaded in a fresh process matches the writer byte for byte to step 30,000, and four more fresh loads agree over shorter runs; under the JIT the save itself changes the run that takes it, so the save point belongs to the tape |
| S6 tape inputs | PASS | Cursor, camera and selection list located and checked by closed-loop selections in four scenarios; 700 of 700 presses held 3 steps registered; two text boxes dismissed closed-loop with no human order change |
| S7 mission entry | FAIL | No save on disk unlocks more than Wasteland and Backwater Station, and the menus offer no other route; the speed setting persists only through a saved game |
| S8 seeds | PASS | The same Blood Bath lobby started at the same step replays an identical full match (all 13,120 tick records, defeat at tick 12,960); across 5 seeds the Zerg and Protoss computers each open two ways; campaign launches reseed from the clock |

The host was heavily loaded during most runs (load average 10–950), so
throughput is given as main-thread CPU time.

## Frontend additions

Five tape commands were added; all are read-only with respect to guest
memory.

| Command | Purpose |
| --- | --- |
| `save NAME` | Write the whole-machine state with `root->serialize(true)`: the same 8,512,920 bytes desktop ares writes to a slot file |
| `load NAME [STEP]` | Restore a state staged with `capture.py --state-input` before the first step of a fresh process |
| `flush` | Write the cartridge save (FlashRAM) through the core, as desktop ares does on unload |
| `watch ADDR BYTES` | Log a field to `watch.csv` on every step where it changes |
| `until ADDR BYTES OP VALUE MAX MASK X Y [HOLD GAP]` | Hold (or pulse) the pad until a guest field meets a condition; a miss ends the run |

`tap tick raw K` keeps every `K`th tick's rows so a full match fits on
disk; it combines with the S4 slice's `tap tick rec` in either order. The pure helpers (condition operators, field widths, file names)
have asset-free checks in `make test-ares-capture-view`. Binaries:
`1a69a1fe6693f45d4635752afda2125def04392ec4dcac766f2c3c966556a81e`
(S5, S7, S8 and S6 scenario A) and
`d113b50e766f403ca5a102e1b8d5f2c9989347af08cbe38a4c69f245b3f1707b`
(adds the pulse form of `until`; S6 scenarios B–D). After review the
tracked source also requires saved state names to end in `.state`,
refuses a state file whose size differs from the core's state size (the
pinned serializer zero-fills reads past the end), reads back a flushed
cartridge file and compares it with FlashRAM (the pinned save call
reports success even when it cannot write), and parses `until` pulses,
`load` steps, `tap` options and guest addresses strictly (a physical
RDRAM offset or its KSEG0/KSEG1 alias, never masked into range), and
checks a state's header against the core's own before loading it. It
builds `41430947b823625e1fdfe74a8d0b9511578454cb501f43cecac29c06bd8f8902`,
which loaded the S5 state in a fresh process, matched the writer's
step-21,000 hash, wrote a state at step 21,002 identical to the one from
the earlier review builds (`ae80e99b…`), and refused a `load` after a
checkpoint and a second `load` before running any step. Smoke runs on the review builds
before it, whose refusal code is unchanged in the final build, refused a
truncated state, an oversized state, an exact-size state with a
corrupted header, an out-of-range watch address, a malformed pulse and a
non-numeric load step before running any step; loaded after `mode
interp`, a review build kept the interpreter and matched the earlier
interpreter run from the same state.
`capture.py` refuses a `load` name that does not
match a staged state exactly, and stages only state files whose digest
the manifest of the run that wrote them records, because the core trusts a
state's payload. An intermediate build,
`69fd4a42…`, wrote a state at step 21,000 with the same SHA-256
(`a3e49f6b…`) as the earlier JIT run that saved there. An earlier build, `bc2df72d…`, failed
every `load` because a load-mode `nall::serializer` reports
`size() == 0` and the state was passed through the in-process copy
path; it was used only for runs without `load` (`s7-speed`, menu
exploration), and the first S5 writer, which matches the later one.

## S5: state files

The tape is the S1 Wasteland tape (resumed T1 save, FlashRAM
`76cf0ba8…`), hashed every 1,000 steps.

| Run | What it did | Result |
| --- | --- | --- |
| Writer | Ran 30,000 steps, `save` at step 20,000 | State 8,512,920 bytes, SHA-256 `82a8bc1795a047759e2629488423fca88d5da3dd29dbd409a32d901c4cff6adb`, 33–78 ms CPU to write, no file-size signal |
| Writer, second build | Same tape on `bc2df72d…` | Same state SHA-256 and all 30 checkpoints equal |
| Loader | Fresh process, `load` then steps 20,000–30,000 | All 10 checkpoints (21,000–30,000) equal to the writer's full 8 MiB hash; final `9bd1f912d77d59e5…` |
| Loader 2 | Fresh process from the same state, 3,000 steps, JIT | Equal to the writer at 21,000, 22,000 and 23,000 |
| Loaders 3–5 | Fresh processes: a JIT run that saves at 21,000, two interpreter runs, and the final build | The JIT ones equal the writer at 21,000; the final build's state at 21,000 equals the earlier one byte for byte |

**The save changes the JIT run that takes it.** The writer equals the
never-saved S1 run B up to step 20,000 and differs from step 21,000 on,
at every checkpoint. The tick counter still agrees at step 30,000
(29,191), and the 64 KiB blocks holding the unit table
(`0x2E0000–0x320000`) were equal at all ten checkpoints; the differing
blocks are `0x0A0000–0x220000` plus a few higher ones. The pinned core
calls `recompiler.reset()` for the CPU and the RSP inside `serialize`,
whether the state is being written or read
(`ares/n64/cpu/serialization.cpp:136`,
`ares/n64/rsp/serialization.cpp:58`), so the JIT recompiles every
block after a save, and interrupt timing moves with block boundaries
(S3 saw the same tick fall on different steps between modes). A
four-run check from the step-20,000 state isolates it:

| Mode | Save at 21,000 vs no save, checkpoints every 250 steps to 23,000 |
| --- | --- |
| JIT | Equal at 21,250 and 21,500; differ from 21,750 (blocks `0x100000–0x110000`) |
| Interpreter | Equal at all 12 checkpoints |

A JIT-written state also loaded and ran in the interpreter. The run
that loads a state starts with empty recompiler caches, exactly like the
writer after its save, which is why the two agree.

Rule: a save is an event in the tape. A capture that starts from a
state is compared with runs that save at the same step (cheap: one
serializer pass), never with a run that skipped the save. Under the
interpreter a save is neutral. This keeps the design's purpose:
labs and later checks can share one boot, and tapes start from a cached
mission state.

## S6: tape inputs

Four scenarios were used: A, the resumed Wasteland save (S1 tape);
B, a Backwater Station save made by an earlier native run (FlashRAM
`0871629f…`); C, a Blood Bath Free for All lobby (human Terran against
computer Zerg, Terran and Protoss) launched from a blank save; and D, a
fresh Wasteland launch with its two opening transmissions. B, C and D
start from state files (`t2-play`, `bb-play`, `t1fresh-play`, SHA-256
`9fe3b634…`, `5ab49e92…`, `b57c951f…`). Fields were found by diffing
full RAM dumps around controlled inputs, then checked by moving the
cursor with closed-loop `until` lines to a screen point computed from a
unit's row position and the camera, pressing A, and reading which row
the selection list holds.

| Field | Address (KSEG0) | Evidence |
| --- | --- | --- |
| Cursor x, y | u16 `0x800AFDCC`, `0x800AFDCE` (copy at `0x800AFE18`) | Screen pixels, 640 × 480, centre (320, 240) after a load. 118 closed-loop moves met their targets in A–D |
| Camera x, y | s32 `0x800A2EE8`, `0x800A2EEC` | World pixels of the screen's top-left corner, moving in 32-pixel steps; −32 at the map's top-left edge. Scrolled and returned in x and y in all four scenarios |
| World point under the cursor | camera + cursor | The selected row lay 6–9 pixels from it in all 8 single selections that hit (A 3, B 2, C 1, D 2); one more, at a mining SCV in C, missed because the SCV had moved during the 100-step approach |
| Selection list | 12 × u32 at `0x800C0C00`; identical copy at `0x800EFC60` | Row pointers from slot 0, ordered by x; held the exact row picked by each single selection, and 6 (B), 3 (C) and 10 (D) rows after a box |
| Per-owner selection | `0x800C0A08` for owner 1 | Same rows in pick order when the human is owner 1; empty in Blood Bath, where the human was owner 2 and its rows appeared 48 bytes later at slots 6–8, so the per-owner layout is not settled |

`0x800EF420` also held the first selected row but kept only one, and
`0x8016C3C8` and `0x8036C400`, heap words that tracked the pause menu
in one run each, are not fixed addresses.

**Controls learned on the way.** The stick's response is not linear: at
16,000 the cursor moves about 5.4 pixels per step, at 10,000 about 1.9
and at 6,000 about 0.4. The camera scrolls while the cursor sits in the
screen-edge zone and keeps scrolling after the stick is released, so a
tape must move the cursor off the edge after every scroll. Z with the
stick pans the camera and Z + A selects like A. The first T1
transmission names the multi-select controls: hold A and drag a box, or
use R. The box form worked in B, C and D. R + A did not add a unit in
C, where the target SCV had moved. Every pad bit S6 needed (Z, D-pad
Up and Left included) was already in the 14-bit mask from S1–S3, so no
new button mapping was required.

**Press registration.** Presses alternated Down and Up with release
gaps cycling 3–7 steps, so each registered press moves a menu cursor by
exactly one item.

| Context | Hold | Presses | Registered | Latency (steps) |
| --- | --- | --- | --- | --- |
| Mission list (row byte `0x800D138F`), A | 3 | 200 | 200 | 2 |
| Pause menu in a fresh Wasteland game, D | 3 | 500 | 500 | 3–4 |
| Same | 2 | 100 | 100 | 3–4 |
| Same | 1 | 100 | 60 | — |

The tick counter advanced 1,644 ticks during the pause-menu test, so
the game loop was running throughout. A press held 3 steps was never
missed; a 1-step press is lost 40% of the time.

**Text boxes, closed loop.** From the fresh Wasteland launch (two
Starts after the briefing), the tape waited with `until` for the pause
halfword `0x800F0C20` to become 1, then pulsed Start (3 steps on,
20 off) until it returned to 0. Both transmissions closed this way
(opened at steps 5,926 and 6,404, closed 52 steps later each). A single
3-step Start 35 steps after the box opened was ignored, so a box
accepts dismissal only after a delay, which is why the tape pulses. Over
the dismissal tick and the five ticks after it, no row owned by the
human changed its order byte (`+0x4D`) or order target (`+0x5C`), and
the selection list did not change; only computer and neutral rows
changed, as they do on any tick.

**Verdict: PASS.** Cursor, camera and selection are located and
validated by controlled moves in four scenarios, a 3-step press was
never missed in 700 presses, and a tape dismissed two text boxes with
no order change. The design's tape compiler (WP6) can be built on
`until` over these fields; the pause menu index is a heap field and
needs locating per run or a fixed proxy.

## S7: mission entry

**What the menus offer.** The single-player menu has three tabs:
Episode, Scenario and Load Saved. The Episode tab lists only rows the
save has unlocked, plus the next one greyed. On the best save on disk
(the native T1 victory, FlashRAM `0beffd78…`) it shows Boot Camp,
Officer Training, 1 Wasteland, 2 Backwater Station and a greyed
3 Desperate Alliance. The six progress bytes at `0x800D13C4` read
`04 01 01 01 01 01` after that save loads (step 1,038), so the Terran
episode has four open rows and the others none beyond their first
marker. No control changed the episode: R, L, Z, all four C buttons
and every D-pad direction were tried on the Episode tab; D-pad Left and
Right move between tabs and Up wraps the row. Options → Cheats lists
eight masked entries that cannot be selected, so the cheat menu is no
route either.

**Row mapping.** The mission-list row byte `0x800D138F` follows the
cursor (0, 1, 2, 3). Row 2 launches Wasteland (T1): every fresh
campaign run reached it with Down, Down, A, and the briefing frame names
it. Row 3 is Backwater Station (T2) by its highlighted label. T10, Z8
and P10 cannot be reached or seen from any save on disk, so their rows
(11, 19 and 31 in a private note made with state edits) remain
unconfirmed.

**Speed setting.** A fresh boot reads selector 4 at step 1,038 with a
blank save as well as with saved progress, so 4 is the default.
Changing Game Speed in a mission (4 → 6) did not touch the cartridge
save: a `flush` after the change gave the input's SHA-256 again. After
Options → Load / Save → Save This Game, the save changed
(`7d5dce07…`) and a fresh boot from it read selector 6 at step 1,038
and kept 6 in the loaded mission. The speed therefore persists in
flash, but only through a saved game, and it applies from boot to every
later launch. A tape must set and save the speed it wants, or start
from a save known to hold it.

**Verdict: FAIL.** No all-missions save exists on disk, and producing
one without a guest edit means winning 28 more missions by input. The
design's fallback applies: play through to produce the save (the owner
earlier named Codex for N64 play), or use the progress-byte edit at
`0x800D13C4` if the owner allows guest edits (owner decision 3). Until
then only Wasteland, Backwater Station and Blood Bath are reachable
headless by ordinary input.

## S8: seeds

**Seed words.** Watched on every step, the launch writes the seed to
`0x800EFF20`, sets the generator state `0x800EFF28` to it and counts
draws in `0x800EFF24`. The state follows
`state = state × 0x015A4E35 + 1`: 9 draws from seed 94,247 and 12 from
84,421 reproduce the logged states exactly. The seed is milliseconds
since power-on at launch. Two Wasteland launches 14 steps apart got
94,247 and 94,481 (234 ms, 14 × 16.7 ms), and the Blood Bath seeds below
step by 16.7 ms per step of delay. 84,421 is above 65,535, so the seed is
not cut to 16 bits. The fixed-seed flag `0x800AFBAC` stayed 0 in every
launch (2 campaign, 6 Blood Bath), and no launch took the fixed-seed
branch.

**Campaign.** Both Wasteland launches reseeded from the clock at the
second Start (briefing room to mission), and their draw counts differed
from tick 17 on. Campaign captures therefore depend on the launch step
as much as skirmish ones do.

**Same lobby, same Start step.** Each match loads the menu state (step
3,900), takes the Blood Bath route, waits a fixed number of steps,
presses Start and runs 45,000 steps with the tap on every tick (raw rows
every 25th tick) and a full-RAM hash every 2,000 steps. Two such runs
(`s8-k0`, `s8-k0b`) both launched with seed 84,421. Each recorded 13,120
tick changes through tick 13,119. All 13,120 change-step records and all
12,967 follow-up records match (unit-row hashes, and the banks and RNG
words), with the same coverage in both runs. At tick 0 the unit table
was not yet readable, so that one read per phase compares only the
banks and RNG words. The other 153 ticks had no
follow-up read, because the next step was already another tick change. The two runs predate the run ids
that #934 added to `capture.py`, so their independence rests on separate
run directories and processes (`capture_report.py --s8 --legacy`). All 24 full 8 MiB checkpoints and the raw row files are
byte-identical. The result words `0x800B936C` and `0x8010DBE0` changed
to `0x20000` at tick 12,960 (step 49,497) in both, when the passive
human was defeated; the final hash is
`3fd7784cf4cb3895c8e6fb68c6a2ca0d238966a5ae3732349c50902cae7ee69f`.
Each match took about 270 s of main-thread CPU (about 185 steps/s with
the tap).

**Five seeds.** The same lobby was started after 0, 7, 19, 41 and 97
extra steps and run to tick 3,300. Opening order = unit types other than
the starting ones, by first appearance at a 25-tick sampling:

| Seed | Passive human owner | Terran computer | Zerg computer | Protoss computer |
| --- | --- | --- | --- | --- |
| 84,421 | 1 | Barracks 1,299, Depot 1,924, Marine 2,624, Refinery 2,924 | Egg 149, Pool 3,174 | Pylon 1,424, Gateway 2,249 |
| 84,538 | 0 | 1,299, 1,924, 2,624, 2,924 | 149, 3,174 | 1,499, 2,324 |
| 84,739 | 3 | 1,449, 1,974, 2,649, 3,049 | **Pool 874, Egg 1,199, Zergling 2,524** | 1,249, 2,074, **Zealot 3,249** |
| 85,106 | 0 | 1,499, 1,949, 2,724, 2,974 | 149, 3,149 | 1,499, 2,299 |
| 86,040 | 2 | 1,424, 1,974, 2,624, 2,999 | 199, 3,299 | 1,449, 2,249 |

Raw rows were kept every 25th tick, so the starting roster is the tick-24
sample. One row appeared between tick 1 and tick 24 in all five runs
(83 → 84 live). A unit type first made in that window would count as a
starting type; the openings compared here begin at tick 149.

Every seed reassigned owners and start positions. The Terran computer
kept one order, shifted by up to 200 ticks. With seed 84,739 the Zerg
computer built a Spawning Pool at tick 874, before morphing any larva
(first Egg at 1,199), and had
Zerglings by tick 2,524, against a late Pool in the other four, and
the Protoss computer had a Zealot by tick 3,249. The seed therefore
selects the opening, directly or through the start positions it draws;
these runs cannot tell the two apart.

**Verdict: PASS.** A pinned launch step reproduces a full Blood Bath
match exactly; seeds are read back from RAM; at least two of the three
computer races have more than one opening across five seeds; and
campaign missions reseed from the clock. Skirmish fixtures must
therefore record the seed and either pin it or treat the result as a
union of openings, as the design's fallback describes.

## Consequences for the design

- State files work as cached starting points: one boot, many labs.
  Treat each save as a tape event under the JIT. Golden runs that start
  from a state compare only with runs from the same state, or that save
  at the same step.
- The tape compiler (WP6) can be closed-loop over plain `until` lines on
  the cursor, camera, selection and pause fields. It needs no control
  socket for these scenarios. Use presses of at least 3 steps, pulse to
  dismiss text boxes, and move the cursor off the screen edge after every
  scroll.
- Mission entry is the blocker for 27 of 30 missions. It needs a played
  save or an owner decision on the progress-byte edit. Speed must be set
  and saved, or checked at step 1,038, before a mission is captured at a
  chosen selector.
- Seeds are a function of the launch step. A skirmish golden names its
  seed, and an envelope over seeds is multimodal for Zerg and Protoss.
  Campaign goldens also depend on the launch step.

## Still unproven

- S5 covered one mostly idle Wasteland stretch (10,000 steps) and short
  runs from it. Longer or busier stretches after a load were not
  compared, and states were not loaded across binaries other than the
  four builds named here.
- The save-perturbed and never-saved JIT runs kept equal unit-table
  blocks for 10,000 steps, but this does not show that game state
  survives a save in general.
- The per-owner selection layout (`0x800C0A08` and its neighbours), R
  as an add-to-selection button and selection of moving units at a
  computed point were not settled. The pause-menu cursor is a heap word
  with no fixed address.
- Text boxes were dismissed with Start only, in one mission. Whether A
  also works, and the minimum delay before a box accepts dismissal, were
  not measured.
- Only Wasteland, Backwater Station and Blood Bath were reachable. The
  row mapping for T10, Z8 and P10 and the effect of a saved speed on
  campaign trigger timing are open.
- The sweep covers five seeds, one lobby (Free for All, Terran against
  Zerg, Terran and Protoss) and openings to tick 3,300. First-attack
  timing, other lobbies and whether position or seed drives the Zerg
  choice are open. Only one seed was replayed to the end of a match.
