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
| GBA run | `tests/sc64_parity_run.c` (`move`, `income`, `lab-t1`, `lab-bb`, `lab-mission`, `match`) | The same scenario through the injector and trace emitter |
| Skirmish | `tools/sc64_parity_skirmish.py` | Blood Bath lobbies: frame streams of both engines, per-player measurements, seed envelopes |
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
| KNOWN | Outside the band, in the baseline, not further out than recorded (half a band of slack; for a skirmish seed envelope, half the seed's own band). A scenario run at several frame offsets: outside the band at some offset, and not wholly further out than the baseline; with no baseline line yet, inside the band at another offset | no |
| REGRESSION | Outside the band and not in the baseline, or further out than the baseline. At several frame offsets: every offset outside the band (no baseline line), or every offset further out than every recorded offset | yes |
| IMPROVED | In the baseline, now inside the band (at every frame offset) | notice |
| STALE | Baseline line with no observation | notice |
| STALE-GOLDEN | The golden was decoded against other definitions of the fields it reads | yes (REPORT with `--allow-stale`) |
| REPORT | Non-gating observation, or a table-differ count | no |
| SKIP | Private inputs absent | no |

`./dev parity --accept` rewrites `tools/parity/baseline.json`: every gating
observation outside its band becomes a known line holding the GBA value
(for a scenario run at several frame offsets, the value at each offset in
`gba_offsets`, offset 0 first and also in `gba`).

A marker or sentinel (an absent event, an off-target fight, a skirmish
NEVER or ENDED) is an outcome, not a number, and numbers are compared only
with numbers. A skirmish NEVER for a timing has a time, "not before the
measurement's reach" (the N64 run's last frame, or a finite band's upper
edge), and compares as the frame after it. At several frame offsets:

- A new finite time where the baseline recorded another outcome at that
  offset (a defeat where the human survived) is judged on its own. It is a
  REGRESSION when it lies beyond the band and every recorded value by more
  than the slack plus the GBA's own frame noise for checks of its kind
  (`FRAME_NOISE`, calibrated once: the widest spread over offsets 0–4 on
  any line of the family, 1,352 frames for build-order firsts, 516 for
  first waves, 508 for defeats; fixed, so accepting a line elsewhere never
  changes it), whatever the other offsets kept. A defeat
  between the band and "never" is nearer the N64 than surviving, so it is
  not worse.
- NEVER where a time was recorded only bounds the time from below, so it is
  judged with the whole spread: every offset's time (NEVER as that bound)
  further out than every recorded time. A wave at the very end of the run
  that slips past it at one offset is noise.
- Numbers alone (of offsets that recorded a number) are also compared
  spread against spread, so an offset that kept NEVER does not excuse
  numbers that all moved further out. A timing whose recorded outcome
  itself differed between offsets (a time at some, NEVER at others) is as
  unstable as the GBA gets, so its numbers must also clear the family's
  frame noise. A number
  inside the band at any offset keeps the comparison KNOWN: the noise
  still reaches the N64's value (only a newly finite timing is judged on
  its own).
- A line whose outcomes share none with the baseline's and have no time
  (ENDED, a marker) is a REGRESSION at any distance, when every offset is
  outside the band.
- A NEVER is a lower bound on the time: the frame after the run's reach,
  or after that GBA run's own last measured frame when its trace ended
  first. It is as far out as that bound lies past the band, and no
  distance at all while the bound lies below the band's upper edge, so an
  early end is gated by the result check, not by every timing it cut
  short.
- Only offsets the baseline recorded are evidence for the per-offset check:
  a single run's line (no `gba_offsets`) speaks for offset 0 alone.
- A seed whose own band is NEVER, in an envelope other seeds made finite,
  keeps the timing tolerance (±2 frames) as its slack.

Against a band that is itself an outcome (the
N64 never built the type), every GBA time is the same wrong outcome, so a
time that moves is no regression; a line with no baseline line still fails
while it is outside.

### Frame offsets

The GBA's skirmish outcomes are chaotic in its own frame phase: with the
same seed and corners, advancing the engine's frame counter by one or two
frames right after the launch moves a computer's supply cap at 8,000 frames
by 16 and its first wave by up to 500 frames (`sk-pvt-w0`, `sk-zvz-w0`).
A single run therefore judged some lines more tightly than the game's own
noise: a run one to four frames out of phase flipped 18–113 of the 1,328
gating `sk-*` lines to REGRESSION (51–113 against the baseline then on
main), as an engine change that shifted timing by a frame would. Each `sk-*` scenario now runs at
frame offsets 0, 1 and 2 (`FRAME_OFFSETS`; the runner's `--frame-offset K`
advances the frame counter K frames right after the launch, whose release
update takes step 1, and leaves the step count, and so the source-frame
clock, unchanged) and is judged on the spread: REGRESSION only when the whole spread moves further out than the
whole recorded spread, PASS or IMPROVED only when every offset is inside
the band. In the same test (offsets 0–4, every window of three
consecutive offsets judged against a baseline accepted at 0–2), no line
flipped to REGRESSION; three noise lines read IMPROVED (every offset inside
the band, where one recorded offset was outside). The price: a shift of
every offset by no more than the line's own spread reads KNOWN. Comparing
the ranked offsets instead (each offset's gap against the same rank of the
baseline's) would catch that, but flipped 0–3 lines of pure frame noise in
the same test. An ENDED checkpoint at some offsets, where the baseline had
numbers, is noise while the other offsets share the baseline's outcome:
the match's end is gated by its own `result.*` line, which compares a new
defeat by its time. Baseline lines must hold one finite value per
configured offset; a single run's line (no `gba_offsets`) stays valid.
`--explain` and `--json` list each offset's command, CPU time, measurements
and first curve divergences under `offset_runs`.
Offsets are GBA-side only; the N64 goldens are unchanged. `--jobs` (one
per CPU by default) runs the GBA runs and their measurement in worker
processes.
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
| `sk-*` (17 lobbies) | Lobby state → race rows → Start after a delay; no further input, to the end of the match | the human race's profile, `--scenario match`: the same races and the N64 seed, no command | start corners; each computer's build order, first wave; gathered minerals, supply, workers at 1,000–8,000 frames; minerals per frame by worker count (report); each player's elimination |

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

## Skirmish lobbies

The `sk-*` scenarios (WP11, with the WP9 economy curves) each name one N64
Blood Bath Free for All lobby with a passive human: `sk-<human>v<computer>`
for a 1v1 (two computer rows Closed), `sk-ffa` for the human Terran against
the default Zerg, Terran and Protoss rows, and `-w<N>` for a Start press
`N` frontend steps late, which moves the seed. The table of lobbies is
`LOBBIES` in `tools/sc64_parity_skirmish.py`; the measured gaps are in
[`audits/2026-11-26-parity-skirmish.md`](audits/2026-11-26-parity-skirmish.md).

Each tape loads one lobby state (the Scenario tab with Free For All and
Blood Bath already chosen), sets the race rows with presses held 10 steps,
waits, presses Start, records every tick and stops 600 steps after the
result word (`0x800B936C`, one byte per owner) changes: a whole 1v1, or a
free-for-all up to its first defeat. `watch` lines log the result and
supply words, which tap.bin does not carry, and the seed (which tap.bin
does carry; the golden reads it from there); `parity_decode.py` requires both captures' `watch.csv` to be
identical as well as their hash chains. In a whole match about 3% of ticks
lose their follow-up read, because the next step changes the tick again;
the decoder's clock still counts such a tick from its change read (see the
skipped-tick rule above). A checkpoint reads the first sampled frame at or
after it, recorded in the golden as `frame` for the GBA side. A run directory may keep its tap as
`tap.bin.zst` (about 30x smaller); the decoder checks the manifest digest of
the decompressed bytes.

On the GBA the `match` runner scenario launches the same lobby and sets the
host launch clock to the N64 seed, so the native start draw puts every
player in the same corner (`start.corner`, gated). Players are joined by
role and race: the human is the only player whose bank never falls (the
GBA side is told), and lobbies never repeat a computer race. The same code
(`measure`) reads both engines' normalised frames: build-order firsts
(buildings when placed, units when complete), minerals gathered, supply
used and provided, complete workers at fixed source frames, minerals per
frame at each worker count, the first wave (three units with a weapon
within 12 tiles of the human's town hall) and each player's elimination.
A time not reached within the N64 run reads `999999` (NEVER) on either
engine, so it matches only NEVER; a GBA checkpoint after its match ended
reads `-1`, which no band admits.

Bands are the N64 value widened by quantisation only (times ±2 frames,
gathered minerals ±one trip per worker, counts exact). After the goldens of
a lobby group exist, `python3 tools/sc64_parity_skirmish.py envelope`
unions each band over the group's seeds, so the seed envelope is the
tolerance exactly where the N64 itself varies; each seed's own band stays
in the golden (`seed_lo`, `seed_hi`) for later re-runs. The start corner
stays exact per seed. NEVER is never the top of a range: when some seeds
never reached a time, the band covers the others and the observation lists
NEVER under `also_ok`, which the judge treats as inside the band. Goldens
decoded with different layouts are not merged.

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
| Attack | damage per hit, cooldown (frames between the subject's hits), centre distance at the first hit, the target's damage on the subject, frames from the first hit taken to death. A hit counts only on the configured foe's unit id and only with a verified single attacker: the subject is the only living human row (or GBA building of the human) within 288 N64 px of the victim, on the sample that shows the drop and on the one before it. A row that hatches, changes owner or is reused by a new unit (the subject's row included; on the GBA also a birth record in the slot) counts as a new unit from that frame. A lifetime that ends in place (a hatch, an owner change) is no kill, and a subject that changes in place leaves survival unmeasured; on the GBA, where a unit can die and its slot be reused inside one engine step, a birth in an occupied slot or any in-place change of another row counts as a removal, which is a kill. No cooldown interval spans a kill of any enemy (units born during the fight included), a destroyed GBA enemy building (seen in the frames or in the trace's removal events; neutral and human removals do not count), or a drop on another enemy or GBA enemy building (shields, a weapon hit in the trace's `D` records, or more hit points than a burning Terran building loses) within reach of the subject, and no interval joins two hits in one frame; the first hit's range and time stand only when no earlier foe drop or kill was unresolved, and the range only when the first hit's frame holds one hit. Damage taken gates only when the same single enemy, a unit of the foe's id on both samples, was the only enemy within 288 px of the subject around every hit. When the GBA fought (any foe drop or kill; a drop or kill of another enemy unit or GBA enemy building within reach of the subject, a kill being a row that reaches 0 hit points or a trace removal at 0, with the actor's last position, so a unit born and killed inside one source frame counts; or any hit on the subject), or the observation ended early (the subject changed in place or was removed with hit points left, the run stopped before its requested steps, or the subject was not found), but a check has no value, that check gets none (REGRESSION, "not measured"), never the "absent" marker; only survival reads absent then, when the subject did not die and the observation ran its course. When the only fighting was with other enemies in reach of the subject (the foe and the subject untouched), the missing checks read the "off-target" marker instead, which, like "absent", inherits KNOWN only from the same marker in the baseline. An off-target drop reads off-target only when the subject is the single human attacker in reach of the victim on both samples (a GBA row counts as human by its cartridge owner or its live owner, so a rescued unit is a rival); one another human unit could have dealt leaves the checks unmeasured. The GBA trace's accepted-hit records (`DA`: what each hit applied; a hit the engine rejected, on an invincible, burrowed or loaded target, writes none) count on their own, whatever the sampled hit points show: on the subject a hit taken (the survival clock starts at the earliest confirmed hit), on the foe a fight with it, on another enemy or enemy building in reach an off-target fight. A hit's target is the occupant at the hit's engine step (a birth, removal, owner or type change in between names the next sample's; one in the same step leaves it unresolved). Each record names its source's live owner: a drop whose records all come from another owner (an enemy's hit on an enemy) is no fight of the subject's. On the GBA the exact checks also hold every accepted, non-lethal hit the subject dealt to the foe and took from a unit of the foe's type, so a wrong hit no sample shows still fails them. A row or building with no cartridge owner whose engine owner is the trace's `neutral` reads as the cartridge's neutral (11). No cooldown interval touches an unresolved drop at either end. Several weapon-hit records on one unit in one frame show as one drop, so the exact damage of that kind (hit points, shields, or damage taken) is unmeasured, and so is the first hit's range when it is the first hit; a GBA shield drop short of its single hit record by no more than one frame gap of regeneration (7/256 a frame, whole points) is no exact value either, and leaves the shield damage unmeasured. A GBA removal record counts for the unit last sampled in that slot before it (no birth in between, the same owner and type), and a vanished GBA subject is dead only when the trace confirms it (its matched removal at 0 or below, or, with no matched removal, a birth in its slot); otherwise it is lost and survival is unmeasured. Burn excuses only a completed Terran building's death at exactly 0 that its burn could explain since its last row, with no weapon hit on it in between. The trace records no same-type slot reuse within one update, so a unit replaced in that way is not told apart | exact (every hit); ±1 frame; ±8 px; exact; ±4 frames |
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
only checks that something is selected. A `sc64-tape-scenario/1` file
with a `select` lacking `row`, or a `box` lacking `row` and `count`,
therefore compiles only with `legacy_selection`; every scenario recorded
before the rule already sets it. Mark names must be unique, in the
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
   `./dev parity`, and `--accept` the known gaps. For a skirmish seed, add
   the lobby to `LOBBIES`, write its golden, then run
   `tools/sc64_parity_skirmish.py envelope` before accepting.

