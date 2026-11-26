# N64 parity unit labs (WP6, WP8)

This round adds the tape compiler (WP6) and the first unit-class labs
(WP8) to the parity harness
([`PARITY-HARNESS.md`](../PARITY-HARNESS.md)). Every lab is captured
twice in fresh processes on the N64 (ares `0aafd857…`, owned USA ROM
`e7d21880…b020`, JIT), measured by the same code as its GBA run, and judged
by `./dev parity`. Captures, scenario files, compiled tapes and goldens stay
in the private fixture home; this note keeps numbers and verdicts only.
Each GBA-vs-N64 difference below is a GBA defect to fix (owner decision,
2026-11-26), never an accepted tolerance; the baseline holds the GBA values
only so that `./dev parity` reports them as KNOWN instead of failing.

## Tape compiler

`tools/sc64_tape_compile.py` turns a small JSON scenario into a frontend
tape: a start (state file, T1 from power-on, the Blood Bath lobby, or any
episode mission through the progress-byte guest edit), the camera there,
and semantic steps (`select`, `box`, `order move|attack|hold|stop` at a
world point, `boxes`, `wait` with or without text-box dismissal, marks).
Nothing is open-loop: every cursor and camera move is a closed-loop wait on
the world point under the cursor, approached from a fixed side, and every
press is preceded by four one-step position checks (within 12 px) and a
check that no text box is open, so a tape that drifts stops with exit 9.

One frontend command was added for it, `untilw x|y OP VALUE …`, a wait on
camera + cursor compared signed. It is needed because a Z pan moves the
camera 40 px in x or 48 px in y per change with 2–3 steps of lag, so where a
pan stops is not known when the tape is written; a wait on the world point
makes that irrelevant.

Controls found on the way (exploration capture `x1-explore`): wheel
commands act at the point under the cursor with no confirming A. C-Right
is Move and B is Attack; B over the selected unit itself does nothing. B at
a point under fog gives the unit an attack-move (order byte 14 with the
point in the order target), which the GBA lab therefore reproduces as an
attack-move.

| Check | Result |
| --- | --- |
| Approach precision | In the 19 lab captures made before the final compiler revision, all 276 `untilw` approach legs met their targets and all 53 pre-press position checks passed (largest miss 5 px on one axis); the three labs recaptured with the final revision passed theirs too |
| T1 five-Marine move, regenerated from a scenario file | Same cruise and spread as the hand-written tape, arrivals one frame later ([below](#wp6-done-condition)) |
| Determinism | Every lab pair has identical per-tick hash chains (the goldens require it) |
| Reproducibility | Recompiling each lab's scenario file with the tracked compiler gives its captured tape line for line (bar the header's scenario digest) |
| Parallel load | `lab-marine-move-3`, run while three other captures ran (one of this session's, two of the field-map session's), has the same 497-record chain as the two runs made alone under the shared native lock; the two `t1-move-compiled` runs, made at the same time in separate lanes, share one 620-record chain |

## Labs and goldens

Captures for missions other than T1 start from states written by runs that
open every episode row with the owner-approved guest edit (`poke
0x800D13C4 0C0A0A`, the field-map session's frontend command). Those
goldens are labelled `n64-staged`. The T1 and Blood Bath labs use ordinary
input only (`n64-ram`).

| Golden | Class | Start | Subject | Provenance |
| --- | --- | --- | --- | --- |
| `lab-marine-move` | ground, small | T1 after both transmissions | Marine | n64-ram |
| `lab-scv-move` | worker | T1 | SCV | n64-ram |
| `lab-hydralisk-move` | ground, medium | Z2 | Hydralisk | n64-staged |
| `lab-tank-move` | ground, large | T10 | Siege Tank (tank mode) | n64-staged |
| `lab-overlord-move` | air | Blood Bath, human Zerg | Overlord | n64-ram |
| `lab-mutalisk-move` | air | Z2 | Mutalisk | n64-staged |
| `lab-vulture-move` | hover | T3 | Vulture | n64-staged |
| `lab-marine-zergling` | weapon, death | T1 | Marine against one Zergling | n64-ram |
| `lab-hydralisk-zealot` | weapon, shields | Z2 | Hydralisk against a Zealot | n64-staged |
| `bb-train` | train and build | S4 Blood Bath captures (passive) | computer workers and buildings | n64-ram |

Each move lab orders the subject out (195–335 px of path) and, once it has
settled, back past its start, so the second leg starts with a reversal.
The GBA run joins the same unit by its halved N64 position (or, in Blood
Bath, where start corners are shuffled, as the only human unit of its id
with the orders given as offsets) and gives the same orders at the same
source-frame offsets: the point each order used is the world point under
the N64 cursor at the press, read from the capture's verified `watch.csv`
(a single unit ends exactly there except when another unit holds the spot).

## Results

Times are source frames, distances N64 pixels. Bands are the N64 value
with the tolerance in [`PARITY-HARNESS.md`](../PARITY-HARNESS.md#unit-class-labs).
"absent" means the event never happened in the GBA run; the baseline holds
it as a marker that a later measured value can never inherit (and the
reverse), so a GBA change that makes the event happen is judged afresh.

### Movement

| Subject | Cruise N64 / GBA | Acceleration | First leg | Turn cost | Start latency (report) |
| --- | --- | --- | --- | --- | --- |
| Marine | 3.92 / 4.03 PASS | 3 / 4 PASS | 85 / 85 PASS | **3 / 0** | 7 / 1 |
| SCV | 4.30 / 4.85 PASS | **17 / 3** | **115 / 80** | 1 / 0 PASS | 5 / 1 |
| Hydralisk | 3.64 / 3.61 PASS | 3 / 3 PASS | **83 / 79** | −1 / 0 PASS | 7 / 1 |
| Siege Tank | 3.84 / 4.00 PASS | 3 / 3 PASS | **56 / 40** | 0 / 0 PASS | 7 / 1 |
| Overlord | 0.825 / 0.839 PASS | 3 / 3 PASS | **239 / 232** | **−3 / 0** | 8 / 3 |
| Mutalisk | 6.22 / 6.54 PASS | **20 / 3** | **57 / 38** | −1 / 0 PASS | 7 / 1 |
| Vulture | 5.76 / 5.85 PASS | **14 / 3** | **49 / 42** | 2 / 0 PASS | 5 / 1 |

The N64 SCV, Mutalisk and Vulture ramp up over 14–20 frames (SCV per-frame
steps from its first movement: 1, 0, 1.4, 1.4, 1, 2.2, 1.4, 2.2, … 4.2, 5),
and the SCV also slows over about 14 frames before it stops; on the GBA
every subject reaches full speed in its first frame and stops dead. The Siege Tank and Overlord legs differ by path, not
speed: the N64 tank's path is 195 px against the GBA's 165 for the same
end point, and the N64 Overlord's 210 px against 197 for a straight 198 px
offset (it curves as it turns). The N64 start latency includes the
pad-to-game input lag, so it is reported, not judged.

### Weapons, shields and death

| Check | N64 | GBA | Verdict |
| --- | --- | --- | --- |
| Marine damage on Zergling | 6 (5 of 5 hits) | absent | KNOWN |
| Marine cooldown | 16 (gaps 13, 17, 17, 15) | absent | KNOWN |
| Marine range at first hit (centre distance) | 79.2 | absent | KNOWN |
| Zergling damage on Marine | 5 (7 of 7 hits, one Zergling in reach) | absent | KNOWN |
| Marine survival from first hit taken to 0 HP | 58 | absent | KNOWN |
| Hydralisk shield damage on Zealot | 10 (HP untouched) | absent | KNOWN |
| Hydralisk cooldown | 15 (the two hits credited to it; 8 more had an idle Hydralisk 273 px from the Zealot) | absent | KNOWN |
| Damage the Hydralisk took | 8 (10 of 10 hits, Zealot blades) | absent | REPORT (the Zealot's Pylon is also in reach) |
| Hydralisk survival | 112 | absent | KNOWN |

A hit that brings a unit to 0 is left out of the hit lists on both
engines: the N64 row then shows only the points that were left, and the GBA
row is removed in that step. Survival still counts from the first hit
taken, the killing one included. A shield drop that may have reached 0
between samples (no more left than one tick of regeneration per elapsed
frame) times a hit but gives no exact damage. Damage taken gates only when
every hit had the same size, since N64 hits carry no source.

Both attack labs are single-attacker setups (re-recorded after #984, see
the follow-up section). The T1 Marine attack-moves round a ridge to
(873,1350), 262 px short of a Zergling pair whose second Zergling stays
more than 288 px away. On the N64 the near Zergling comes out to meet it:
the Marine's first shot lands at 79 px, and the Zergling kills it 58 frames
after its first bite. The GBA Marine walks the same route, passes 168 px
from the pair and stops at the point; neither Zergling moves and no shot
is fired. The Z2 Hydralisk attack-moves to (1002,1100), south-west of a
Zealot and away from that Zealot's Photon Cannon. On the N64 the Zealot
comes out, the Hydralisk opens fire at 128 px, and the Zealot kills it 112
frames after its first hit. On the GBA the Hydralisk reaches the point, 245
px from the Zealot, and nothing happens.

N64 rows outlive their hit points: the Marine's row stays 16 frames after
its HP reaches 0. A GBA row goes in the step of the killing hit, so the
killing hit leaves no HP record there; victims count as killed when their
points reach 0 or their row is gone.

### Train and build times (Blood Bath computers, passive)

| Type | N64 timer → completion | GBA | Verdict |
| --- | --- | --- | --- |
| SCV | 300 → 301 (10 completions) | 300.0 (16) | PASS |
| Supply Depot | 600 → 602 | 599.2 (3) | KNOWN |
| Barracks | 1,200 → 1,202 | 1,199.2 (2) | KNOWN |
| Probe | 300 → 301 (6) | no GBA Protoss computer in this lobby | REPORT |
| Pylon | 449 → 520 | no GBA Protoss computer | REPORT |
| Marine | 360 (incomplete in the window) | 360.0 | not judged |

## Layout changes

Two unit fields were promoted on lab evidence (`tools/parity/sc64-usa-v1.json`):

- `hp` (+0x08, 8.8): 40, 35, 60 and 80 × 256 for Marine, Zergling, SCV and
  Vulture at rest; the T1 Marine lost exactly 5 × 256 per Zergling hit,
  eight hits from 40 to 0, while the Zergling it shot lost exactly 6 × 256
  per hit.
- `shields` (+0x60, 8.8): 80, 80 and 100 × 256 for Zealot, Dragoon and Scout;
  the Zealot lost exactly 10 × 256 shield per Hydralisk hit with its HP
  unchanged, and regained 7/256 per tick. Other races' rows hold a constant
  100 × 256, so the field is read for Protoss rows only.

The decoder now counts a tick that began and ended inside one frontend
step (a skipped follow-up read) in the unpaused clock when its change-step
read shows it ran unpaused. A golden still needs both runs to skip the same
ticks, and every skipped tick must have its change-step read. Three labs
have such skips (Vulture 2 ticks, Hydralisk move 1, Hydralisk–Zealot 3);
those frames are simply not sampled.

## Defects

Every row is a GBA difference from the N64 with the numbers above; each is
a KNOWN line in `tools/parity/baseline.json`.

1. **No acceleration or braking for SCV, Vulture and Mutalisk.** N64
   frames to 90% of cruise 17, 14 and 20; GBA 3. First legs 115/80, 49/42
   and 57/38.
2. **Ground routes shorter than the N64's.** Siege Tank path 195 vs 165 px,
   leg 56 vs 40; Hydralisk leg 83 vs 79 at the same cruise.
3. **Overlord flight path and turn.** N64 path 210 px for a 198 px offset,
   leg 239 vs 232; reversal starts 3 frames sooner than the first move on
   the N64 (turn cost −3), 0 on the GBA.
4. **Computer guards do not come out to a nearby enemy.** On the N64 a
   guarding Zergling (T1) and Zealot (Z2) walk out to a human unit that
   stops 245–262 px away, and the fight follows (Marine: 6 per hit, cooldown
   16, first hit at 79 px, killed after 58; Hydralisk: 10 shield damage per
   hit, cooldown 15, killed after 112). On the GBA neither guard moves, even
   with the Marine passing at 168 px, so neither lab has a GBA hit.
5. **No turn delay on reversal.** A Marine Move reversed back along its
   path starts 3 frames later than its first move on the N64 (turn cost 3)
   and 0 on the GBA. Every GBA subject shows 0; the N64 ranges from −3 to 3
   (defect 3 is the Overlord's).
6. **Construction ends 2–3 frames early.** Supply Depot 599.2 vs 602,
   Barracks 1,199.2 vs 1,202 (N64 timers 600 and 1,200 complete 2 frames
   after reaching 0).

The N64 Marine's gaps vary (13–17), where the GBA's were fixed at 15 (16
when a frame boundary falls between) in the #984 recording, which suggests
a randomised cooldown on the cartridge. One lab cannot confirm it.

## Coverage

Movement classes: every class has an N64 golden (table above). Weapons:
two weapons on the subject side (Gauss Rifle, Needle Spines) and two on the
target side (Zergling claws, Zealot blades); armor and size scaling is
observed only as "shields take the full 10 of an explosive weapon". Train
and build: SCV, Supply Depot, Barracks judged; Probe and Pylon N64 only.

Not covered, and why:

- **The other units.** Firebat, Medic, Ghost, Goliath, Wraith, Dropship,
  Science Vessel, Battlecruiser, Valkyrie, Zergling (move), Drone,
  Ultralisk, Guardian, Queen, Defiler, Scourge, Infested Terran, Probe
  (move), Zealot (move), Dragoon, High Templar, Dark Templar, Archon,
  Reaver, Shuttle, Observer, Scout, Carrier, Arbiter and the heroes have no
  lab yet. The brief asked for one capture per movement class first; each
  further unit is one scenario file and two captures (~1 min each) from a
  mission state. The states already written cover T3 (Vultures, Marines,
  SCVs; Zerg Hydralisks, Overlords, Mutalisks as targets), Z2 (Hydralisks,
  Mutalisks, Overlords, Drones; Protoss Zealots, Dragoons, Scouts as
  targets) and T10 (Siege Tanks, Vulture, Firebats, Marines, SCVs).
- **Staged units.** No CreateUnit row was staged: the progress-byte edit
  reached every unit class needed, so the trigger-list edit at
  `0x800CD214` was not attempted.
- **Projectile travel time.** Needs the bullet pool or a fire event on the
  N64 (the cooldown byte is unmapped); the field-map session maps
  projectiles.
- **Size-scaled hit-point damage** (explosive on small, concussive on
  large). The Hydralisk never got through the Zealot's shields; a lab with
  a longer fight or a shieldless target is the next step.
- **Zealot damage as a gating check.** Every Z2 Zealot stands next to a
  Pylon or Photon Cannon, so the hits the Hydralisk takes never have a
  single enemy in reach and stay report-only.
- **Sound timing (WP13).** The capture frontend does not record audio and
  no N64 sound-request field is located. The GBA trace's `A` records are
  ready for the comparison once one is.
- **Build and train times of Protoss and Zerg on the GBA side.** The GBA
  Blood Bath runner has one Terran computer.

## WP6 done condition

The design's done condition is the T1 five-Marine tape regenerated from a
scenario file and replaying to the same hash chain. The compiled tape
(`t1-move-compiled`: boot, both transmissions, box select, C-Right at the
S4 target) was captured twice:

| Measure (`t1-move` code) | Compiled tape | S4 hand-written tape (golden) |
| --- | --- | --- |
| Cruise speed | 3.9049 | 3.9049 |
| Group spread | 29.4387 | 29.4387 |
| First / last arrival | 81 / 82 | 80 / 81 |
| Start latency (report) | 7 | 8 |

Both compiled captures give the same 620-record hash chain. The chain is
not the hand tape's: the compiled tape enters the mission at another step,
and campaign launches reseed from the clock (S8), so the two are different
games that measure the same. The one-frame arrival shift is inside the
±2-frame band.

## Reproducing

Private home: `.local/parity-harness/labs` in the main checkout
(`scenarios/`, `tapes/`, `runs/`, `regen.sh` to rewrite the goldens). Two
frontend builds made the captures: `capture-ce3c72e8` (this work's `untilw`
alone; T1 move labs, Blood Bath, the compiled T1 move) and `capture-8b81284d`
(the field-map frontend's `poke`, `pulse` and `region` plus `untilw`; the
staged missions and every lab that dismisses text boxes). Both sources are
pinned in the private home. The three labs re-recorded after #984 were made
with `capture-d8175bff`. This branch's `capture.cpp` (the field-map
commands, their later output budget, and `untilw`) builds `d8175bff`; the
emulation code the captures ran is the same. `./dev parity` needs only the
goldens.

The fixture home is shared by every worktree, and each golden records the
layout fields its measurement read. After this branch was rebased onto the
field-map PR, the nine `lab-*` goldens, `bb-train`, `t1-move` and `bb-income` were
re-decoded there against the merged layout; `./dev parity` then exits 0
with 24 PASS, 34 KNOWN, 24 REPORT and 17 SKIP (skirmish goldens this branch
has no measurement code for), and no STALE-GOLDEN, REGRESSION or IMPROVED
line.

## Follow-up (after #984)

Rules tightened after review, with their effect on these goldens:

- A hit counts for the subject only with a verified single attacker (no other living human row
  within 288 N64 px of the victim). In the Hydralisk–Zealot lab two of the three N64 hits had
  other Hydralisks in reach, so one hit is credited and the cooldown check is gone (shield
  damage 10, range and survival remain).
- Damage taken gates only when exactly one enemy row was within 288 px of the subject at every
  hit. Both Zerglings were in reach of the T1 Marine, so its damage taken (5) is now report-only.
- Each press is checked against the subject's order byte. The Marine lab's return press gave an
  attack-move (C-Right with the cursor over the other Marines), and the GBA replayed that leg as
  an attack-move.

The three labs these rules affected were then re-recorded (two fresh captures each, identical
hash chains, goldens written to a private `SC64_PARITY_HOME` until this lands):

| Lab | Change to the tape | Verdict change |
| --- | --- | --- |
| `lab-marine-move` | Return press at (320,188) on open ground: both presses are now Move | turn cost N64 2 → 3, GBA 0: PASS → KNOWN (defect 5) |
| `lab-marine-zergling` | Marine row 10 attack-moves to (873,1350): one Zergling in reach | damage taken gates again (5, KNOWN); damage, cooldown and range KNOWN (absent on the GBA, defect 4); survival KNOWN (absent) |
| `lab-hydralisk-zealot` | Hydralisk row 37 attack-moves to (1002,1100): away from the Zealot's Photon Cannon, idle Hydralisks out of reach at the credited hits | cooldown gates again (15, KNOWN, absent on the GBA); shield damage and range stay KNOWN; survival 66 → absent, still KNOWN (defect 4) |

Both new tapes give the selected row (`row`) and no longer need `legacy_selection`. Attempts
that did not give a single attacker: the lone T1 Zergling at (67,1143) (the Marine's route
round the ridge meets the pair first) and Hydralisk points (1000,1200) and (1000,1250)
(no fight, or one hit before the Cannon killed it).

