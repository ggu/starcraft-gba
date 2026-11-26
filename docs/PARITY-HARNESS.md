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
| Capture | `experiments/ares-headless/capture.py` | Runs a tape in a fresh process; `tap tick rec` writes `tap.bin` |
| Layout | `tools/parity/sc64-usa-v1.json` | Guest RAM fields, each `proven`, `candidate` or `unknown`, with evidence |
| Decode | `tools/parity_decode.py` | Reads `tap.bin` (and `extra.bin` regions) against the layout; per-tick records, unpaused clock, hash chain; fog, image, order decoders |
| Field map | `tools/sc64_parity_fieldmap.py` | Mission-launch censuses against the CHK, the `.dat` table anchor, player upgrade and tech arrays, opening timelines ([audit](audits/2026-11-26-parity-field-map.md)) |
| Golden | `tools/parity_decode.py golden` | Two agreeing captures become `$SC64_PARITY_HOME/golden/<id>.json` |
| Measure | `tools/sc64_parity_metrics.py` | One measurement per check, used for both engines |
| GBA run | `tests/sc64_parity_run.c` (`move`, `income`) | The same scenario through the injector and trace emitter |
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

The hash chain is SHA-256 over each record's tick, globals and rows in order.
A bundle is golden only when two fresh-process captures of the same tape,
binary, ROM and CPU mode give the same chain at every tick and neither
capture reported a cache-alias conflict or a bad table link.

## Layout gate

A gating check may read only `proven` fields. `Layout.unit_field(name,
gating=True)` raises `LayoutError` otherwise, and every golden observation
lists the layout fields it read. Promoting a field needs an evidence line in
the layout file: a controlled change, or a cross-check like the Blood Bath
bank test below.

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

## Adding a scenario

1. Write a tape that reaches the situation with ordinary input, turns on
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
