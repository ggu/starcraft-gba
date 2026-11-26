# N64 parity harness: captures, goldens and `./dev parity`

The parity harness captures StarCraft 64 behaviour once from the N64 and
then judges this engine against it with host-only runs. This page covers the
pieces from capture to verdict. The GBA trace emitter and injector are in
[`PARITY-TRACE.md`](PARITY-TRACE.md); the capture frontend is in
[`experiments/ares-headless/CAPTURE.md`](../experiments/ares-headless/CAPTURE.md);
the first measured results are in
[`audits/2026-11-26-parity-vertical-slice.md`](audits/2026-11-26-parity-vertical-slice.md).

Captures, tapes and golden files are ROM-derived and stay private, under one
fixture home shared by every worktree: `$SC64_PARITY_HOME`, by default the
main checkout's `.local/parity`. Only code, the layout file and the baseline
(GBA values only) are tracked.

## Pipeline

| Stage | File | What it does |
| --- | --- | --- |
| Tape | `tools/sc64_tape_compile.py` | Compiles a scenario description into a closed-loop tape ([below](#tape-compiler)) |
| Capture | `experiments/ares-headless/capture.py` | Runs a tape in a fresh process; `tap tick rec` writes `tap.bin` |
| Layout | `tools/parity/sc64-usa-v1.json` | Guest RAM fields, each `proven`, `candidate` or `unknown`, with evidence |
| Decode | `tools/parity_decode.py` | Reads `tap.bin` (and `extra.bin` regions) against the layout; per-tick records, unpaused clock, hash chain; fog, image, order decoders |
| Field map | `tools/sc64_parity_fieldmap.py` | Mission-launch censuses against the CHK, the `.dat` table anchor, player upgrade and tech arrays, opening timelines ([audit](audits/2026-11-26-parity-field-map.md)) |
| Golden | `tools/parity_decode.py golden` | Two agreeing captures become `$SC64_PARITY_HOME/golden/<id>.json` |
| Measure | `tools/sc64_parity_metrics.py` | One measurement per check, used for both engines |
| GBA run | `tests/sc64_parity_run.c` (`move`, `income`, `lab-t1`, `lab-bb`, `lab-mission`) | The same scenario through the injector and trace emitter |
| Judge | `tools/sc64_parity.py` | Verdicts against the band and the baseline; scorecard |
| Loop | `./dev parity` (`make parity`) | Table differ plus every golden scenario, in parallel |

## Records and time

`tap.bin` holds one record per tapped read: clocks, the four 12-player bank
arrays and the RNG words on every read, and the live unit rows on the read one
step after the tick counter changes. A tick's record is that follow-up read
(the change-step read is torn about 1.3% of the time, S3).

N64 time is unpaused ticks: a tick counts when its change-step read shows the
pause halfword clear, numbered from the first tick with a unit table. GBA time
is simulation steps × 4/5. One unpaused N64 tick is taken as one source frame
at any speed selector (the selector paces wall time, not ticks).

A tick that begins and ends inside one frontend step leaves a change-step
read but no follow-up record; it still counts in the unpaused clock when that
read shows it unpaused, and its frame is simply not sampled.

The hash chain is SHA-256 over each record's tick, globals and rows in order.
A bundle is golden only when two fresh-process captures of the same tape,
binary, ROM and CPU mode give the same chain at every tick and neither
capture reported a cache-alias conflict or a bad table link. Both must
report the same read faults, and every interior tick without a follow-up
record must be a skipped tick with its change-step read (the first tick
after the tap starts and the last tick may lack one).

## Layout gate

A gating check may read only `proven` fields. `Layout.unit_field(name,
gating=True)` raises `LayoutError` otherwise, and every golden observation
lists the layout fields it read. Promoting a field needs an evidence line in
the layout file: a controlled change, or a cross-check like the Blood Bath
bank test below.

Each observation also stores `layout_reads` (its listed fields plus every
field the measurement read) and `layout_fields_sha256`: a digest of their
definitions (offsets, types, addresses, names; not their status or evidence)
plus the decoder's fixed parts (unit table locator, bank block, record rule).
A layout edit that leaves those unchanged (a new field, a promotion, a new
evidence line) keeps the golden gating. When they change, or the layout no
longer proves a field a gating check reads, the observation is
`STALE-GOLDEN` and `./dev parity` fails until the golden is regenerated from
the same runs with `tools/parity_decode.py golden`; `--allow-stale` reports
it instead, for recording work only. Regeneration refuses a layout that moves
a field the frontend records at a fixed address (`TAP_CONTRACT`): that needs
new captures. Goldens written before per-field digests compare the whole
layout file.

## Golden format

```json
{"check": "income.rate", "subject": "human workers x4", "value": 0.0,
 "lo": 0.0, "hi": 0.0, "tolerance": "...", "gating": true,
 "n_runs": 2, "provenance": "n64-ram", "layout_fields": ["banks.minerals_gathered", "..."]}
```

The golden file also holds the scenario's GBA parameters (`gba`), N64 curves
for first-divergence reports (`curves`), and the bundle's identity: runs,
hash chain, seed words, binary, tape, ROM and mode.

## Verdicts

| Verdict | Meaning | Fails? |
| --- | --- | --- |
| PASS | Inside the N64 band | no |
| KNOWN | Outside the band, in the baseline, not further out than recorded (half a band of slack) | no |
| REGRESSION | Outside the band and not in the baseline, or further out than the baseline | yes |
| IMPROVED | In the baseline, now inside the band | notice |
| STALE | Baseline line with no observation | notice |
| STALE-GOLDEN | The golden was decoded against other definitions of the fields it reads | yes (REPORT with `--allow-stale`) |
| REPORT | Non-gating observation, or a table-differ count | no |
| SKIP | Private inputs absent | no |

`./dev parity --accept` rewrites `tools/parity/baseline.json`: every gating
observation outside its band becomes a known line holding the GBA value.
`--explain ID` prints one scenario's full measurements; `--keep DIR` keeps the
GBA traces.

The scorecard ends with the fixture-set hash (SHA-256 over the golden files
judged). With no golden files the scenarios skip; if everything skipped while
`.local/sc64` exists, the run exits 1.

## Scenarios

| Id | N64 tape | GBA run | Checks |
| --- | --- | --- | --- |
| `t1-move` | Fresh boot to Wasteland, box-select the five Marines, C-Right (Move) then A on open ground | `terran`, `--scenario move`: the five Marines joined by type, owner and halved position get one group order to the target | cruise speed, first and last arrival, group spread; command latency (report only) |
| `bb-income` | Scenario → Blood Bath, human Zerg, computer Terran, start; no further input | `zerg-z10`, `--scenario income`: Blood Bath, human Zerg against one Terran computer, no command | minerals gathered per frame over 2,000 frames, first delivery, minerals per delivery |
| `t1-fields` | The campaign launch tape for T1 (below) recording the fog, image, sprite and bullet regions, then the t1-move box select and Move, then Stop, Hold, a second Move and an attack-move | `terran`, `--scenario move`, as t1-move | the t1-move checks on the first Move; order timing (command to Move, to the first and last Marine back in an idle order); fog (first newly visible tile, first dropped tile, visible and explored counts before the Stop); the Marine walk cycle (N64: the image's iscript Walking loop; GBA: stride phases) |

The human player is resolved per scenario: T1's Marine owner; in Blood Bath
the only owner of Drones (owner numbers and start corners are shuffled per
launch). Both engines start skirmish workers mining by themselves, so the
income window opens at the first in-mission frame. The move target is the
N64 movers' final centroid, halved; cursor RAM is not located yet.

Tolerances, from the S4 data:

| Quantity | Band |
| --- | --- |
| Intervals (arrival, first delivery, latency) | N64 value ±2 source frames |
| Cruise speed (8-frame chords, middle half of the move) | N64 per-unit range ±10% |
| Group spread (RMS about the centroid at rest) | N64 value ±4 N64 px |
| Income rate | ±(workers × 8 minerals) over the window |
| Minerals per delivery | exact |

Times are measured from the last state before the command on both sides:
on the N64 the last follow-up read taken at or before the tape's
`# mark command`, on the GBA the step before the injected command, which
must own its source frame (`gba_boundary_ok`).

## Mission entry

Every Episode I–III mission launches from a fresh boot with ordinary menu
input once the six progress bytes at `0x800D13C4` (unlocked rows per episode)
are set after the save loads: the launch tape pokes `0C 0A 0A` at step
1,700, rotates the main-menu campaign carousel with the stick (once for
Zerg, twice for Protoss), presses Start (and Start again on the
episode-order warning), picks the row (Terran rows 2–11 are T1–T10 after
the two tutorials; Zerg and Protoss rows 0–9), presses A, pulses Start until
the map mode halfword `0x800A5AB4` reads 3, and dismisses text boxes with
`pulse`. Such captures are `n64-staged`. The private generator and the
30 launch results are described in the
[field-map audit](audits/2026-11-26-parity-field-map.md).

## Unit-class labs

Each N64 press must show in the subject's order byte (`+0x4D`) within 12
ticks as Move (6) or AttackMove (14); the GBA replays that order at the
point the press was given, and each leg must settle within 32 N64 px of it.
A press whose last preceding tick has no follow-up read (it ran inside one
frontend step), with an unread tick between it and the first changed order,
or whose subject row vanishes or changes type or owner before then, is
refused: the first order the press gave was not observed. An attack golden
with no gating check is refused when written and fails when scored.
Each lab gives one human subject timed orders: on the N64 from a compiled
tape that loads a lab state (saved once, after the mission's opening
transmissions), on the GBA through the injector at the same source-frame
offsets. The subject and the human player are found by cartridge unit id,
never by position, so tracked code holds no placements. The runner's
`lab-*` scenarios join the subject by its halved N64 position (`--subject`)
or, where start positions are shuffled (Blood Bath), as the only human unit
of its id (`--subject-raw`) with orders given as offsets. Results and
coverage are in
[`audits/2026-11-26-parity-unit-labs.md`](audits/2026-11-26-parity-unit-labs.md).

| Kind | Measures (both engines, `tools/sc64_parity_metrics.py`) | Band |
| --- | --- | --- |
| Move, out and back | cruise speed (8-frame chords, middle half), acceleration (frames to a chord at 90% of cruise; the chord is 4 frames, or long enough to span two GBA position steps at the N64 cruise speed, as for the Overlord's 5. A lengthened chord's cutoff is at most the N64 cruise distance rounded down to whole GBA steps, so the Overlord's is 4 px, about 95% of its 5-frame cruise distance. The N64 side records chord and floor in the golden (`accel_chord`, `accel_floor`) for the GBA side to use. A mover slower than about 1 px a frame resolves acceleration to about one GBA step, 3-4 frames), first-leg duration, turn cost (second leg's start latency minus the first's), start latency (report only: the N64 figure includes input lag) | ±10% speed; ±2 frames |
| Attack | damage per hit, cooldown (frames between the subject's hits), centre distance at the first hit, the target's damage on the subject, frames from the first hit taken to death. A hit counts only on the configured foe's unit id and only with a verified single attacker: the subject is the only living human row (or GBA building of the human) within 288 N64 px of the victim, on the sample that shows the drop and on the one before it. A row that hatches, changes owner or is reused by a new unit (the subject's row included; on the GBA also a birth record in the slot) counts as a new unit from that frame. A lifetime that ends in place (a hatch, an owner change) is no kill, and a subject that changes in place leaves survival unmeasured; on the GBA, where a unit can die and its slot be reused inside one engine step, a birth in an occupied slot or any in-place change of another row counts as a removal, which is a kill. No cooldown interval spans a kill of any enemy (units born during the fight included), a destroyed GBA building (seen in the frames or in the trace's removal events), or a drop on another enemy within reach of the subject; the first hit's range and time stand only when no earlier foe drop or kill was unresolved. Damage taken gates only when the same single enemy, a unit of the foe's id on both samples, was the only enemy within 288 px of the subject around every hit. When the GBA fought (any foe drop or kill, or any hit on the subject) but a check has no value, that check gets none (REGRESSION, "not measured"), never the "absent" marker; only survival reads absent then, when the subject did not die | exact (every hit); ±1 frame; ±8 px; exact; ±4 frames |
| Train and build (passive) | Blood Bath computers: N64 rows born with the status bit clear until it sets; GBA queue progress 1 to the unit's birth, and a building's birth to its completion | ±2 frames |

Hit points are the 8.8 field at `+0x08`, divided by 256. A capture whose
tape writes guest memory (`poke`) is labelled `n64-staged`.

## Tape compiler

`tools/sc64_tape_compile.py SCENARIO.json -o TAPE` turns a scenario file
into a tape. Scenario files and compiled tapes hold cartridge coordinates,
so both stay in the private fixture home; the compiler and its tests
(`tests/test_sc64_tape_compile.py`) are tracked and asset-free.

```json
{"format": "sc64-tape-scenario/1", "id": "example-move",
 "start": {"load": "lab.state", "step": 5000}, "camera": [0, 0], "map": [2048, 2048],
 "program": [{"op": "guard"}, {"op": "tap"}, {"op": "select", "at": [300, 200], "row": "0x802E2340"},
  {"op": "order", "kind": "move", "at": [500, 300], "mark": "command"}, {"op": "wait", "steps": 450},
  {"op": "order", "kind": "move", "at": [320, 210], "mark": "turn"}, {"op": "wait", "steps": 450}]}
```

| Op | Tape |
| --- | --- |
| `start` | `{"load": NAME.state, "step": N}`, `{"boot": "t1"}`, `{"boot": {"lobby": "blood-bath", "human": R, "computer": R}}` or `{"boot": {"episode": R, "mission": N}}` (the progress-byte guest edit; staged) |
| `select`, `cursor` | Cursor onto a world point (panning with Z first when needed); `select` then presses A and checks the selection list (below) |
| `box` | A held while the cursor drags to the second corner |
| `order` | `move` (C-Right), `attack` (B), `hold` (C-Left), `stop` (C-Down) at the point under the cursor; `"mark": NAME` puts `# mark NAME` right before the issuing press |
| `boxes N` | Wait for each text box and pulse Start until the pause halfword clears |
| `wait`, `"dismiss": true` | Idle steps; with `dismiss`, Start is pulsed only while a box is open (`pulse`) |
| `guard`, `tap`, `save`, `shot`, `peek`, `until`, `hold`, `press`, `poke` | One frontend line each, validated |

`select` needs `"row"`, the KSEG0 address of the subject's unit row (table
address + 312 × index), and then requires the selection list to hold
exactly that row; `box` needs `"count"` (exactly that many units selected)
or `"row"` (the first one), and nothing selected before the drag, so a
failed drag never leaves a stale group that passes. Scenarios written before this rule set
`"legacy_selection": true`, which keeps their recorded tapes unchanged and
only checks that something is selected. Mark names must be unique, in the
compiler and in hand-written tapes. `poke`
(and the episode boot that uses it) and `wait` with `dismiss` (`pulse`)
compile only when the scenario lists them in `"frontend_commands"`, an
explicit opt-in: `poke` writes guest memory, and captures made with
either before both reached the tracked frontend used the field-map build.

World points are reached with `untilw` lines from a fixed side (fast to a
point short of the target, then slowly onto it), so the direction never
depends on where the cursor or camera was. Pans aim the centred cursor at
the point; their coarse 40/48-px steps and 2–3 steps of lag only change how
far the cursor then travels. Before any press, four one-step `untilw`
checks require the world point under the cursor to be within 12 px of the
target, and a `guard` requires no open text box, so a tape that drifts
stops with exit 9 instead of ordering the wrong unit.

## Adding a scenario

1. Write a scenario file and compile it with `tools/sc64_tape_compile.py`
   (or write a tape by hand) that reaches the situation, turns on
   `tap tick rec`, and marks the command step with `# mark command`. A
   tape may start from a state file (`load`) and wait on guest fields
   (`until`); marks then resolve through the run's `commands.csv`, so they
   land on the steps actually executed.
2. Capture it twice with `capture.py` (fresh processes, same binary).
3. Add measurement code to `SCENARIOS` in `tools/sc64_parity_metrics.py`: an
   N64 side that resolves the human player, measures, and returns
   observations plus the GBA runner parameters; a GBA side that measures the
   same numbers from a trace.
4. Add the GBA scenario to `tests/sc64_parity_run.c` if no existing one fits.
5. `python3 tools/parity_decode.py golden --scenario ID RUN_A RUN_B`, then
   `./dev parity`, and `--accept` the known gaps.
