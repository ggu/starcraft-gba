# N64 parity: Blood Bath skirmish goldens (WP11, WP9 curves)

This audit records the first skirmish goldens of the parity harness: whole
Blood Bath matches against a passive human, captured on the N64 and judged on
the GBA host engine with the same measurement code. The harness pieces are
described in [`PARITY-HARNESS.md`](../PARITY-HARNESS.md) (section *Skirmish
lobbies*). Every capture, tape, golden file and frame stays in the private
fixture home; this note keeps derived numbers only. Per the owner's decision
of 2026-11-26, the GBA follows N64 ROM values, so every difference below is a
GBA defect, not an accepted gap.

| Criterion | Result |
| --- | --- |
| True 1v1 lobbies | All 9 human/computer race pairs, one seed each; three seeds for ZvT, TvZ and TvP |
| Free-for-all | Human Terran against computer Zerg, Terran and Protoss, two seeds |
| Goldens | 17, each from two fresh-process captures with identical hash chains and identical `watch.csv` |
| Start corners | The GBA, given the N64 seed, put every player of all 17 lobbies in the N64 corner (38 of 38 `start.corner` checks pass) |
| Gated checks | 524 differ (baseline lines, GBA values only), 804 pass; 178 report-only. Before the N64 mining time and rule tables (#986, #989) 564 differed; see *Re-run on the N64 mining time and rule tables* |
| Wrong-variant test | The same lobbies launched on the GBA with a seed 333 ms off fail 3–17 gated checks each, `start.corner` among them; the right seed fails none |
| `./dev parity` | Exit 0 against the baseline |

The largest defects: every GBA economy still gathers faster than the N64's
(at 1,000–2,000 frames the passive human 1.2–1.5x, computers 1.3–1.7x, up to 2.7x
against the rushing Zerg; before #986 it was about 2x); each computer race
follows a different build order; the N64 Zerg computer has a 9-pool rush
that the GBA never plays; and in 12 of the 16 N64 matches that ended in the
human's defeat, the GBA human was still standing when the N64 match ended.

## Lobbies and how they were reached

All captures used the pinned frontend `41430947…` (the #935 build, which
has `tap tick rec`, `watch`, `until`, `save` and `load`), the JIT, the owned
USA ROM and no cartridge save. One boot (`lobby-base`) pressed through the
title, opened the Scenario tab, set Game Type to Free For All and Map to
Blood Bath, and saved a state at step 5,120 with the cursor on the Map row.
Every lobby tape loads that state, then:

- moves to the human row and presses Right to cycle the race (Terran,
  Protoss, Random, Zerg);
- sets each of the three computer rows (default Zerg, Terran, Protoss;
  Right cycles Zerg, Terran, Protoss, Random, Closed). A 1v1 keeps the first
  row and closes the other two; the free-for-all keeps the defaults;
- waits 60 plus `N` steps, presses Start, records every tick, and stops 600
  steps after the result word (one byte per player) changes, which is the
  human's defeat in every 1v1 and the first defeat in a free-for-all.

Presses are held 10 steps with 50–60 step gaps. The lobby frame was checked
in the exploration capture (`Human Zerg / Computer Terran / Closed /
Closed`). Each capture held the shared native lock, which serialises all
captures on the machine, so none ran beside another capture; each pair is
two fresh processes with identical hash chains and identical `watch.csv`.

## Start corners: the seed decides, and the GBA reproduces it

The launch writes the seed (milliseconds since power-on) to `0x800EFF20`,
and Blood Bath draws owner rows to start markers from it. The GBA's native
Blood Bath launch (`SC64_NATIVE_MELEE_STARTS`) reconstructs the same draw
from its own launch clock. The parity runner's new `match` scenario sets the
host clock to the N64 seed before pressing launch. In all 17 lobbies every player's starting town hall on the GBA is in the same
quadrant as on the N64 (38 of 38 `start.corner` checks), across 14 distinct
seeds from 91,828 to 102,523 ms. The `rng.seed` field is `proven` in the
layout file on this evidence and S8's.

The exploration capture (human Zerg against Terran, seed 100,837) shows the
mechanism exactly: the human's Hatchery at N64 (288, 1808) and the Terran
Command Center at (128, 240); the GBA run with that seed put them at GBA
(144, 904) and (64, 120), the same points halved.

## Supply on the cartridge

tap.bin carries the four bank arrays but no supply. Three full-RAM dumps of
the exploration match (steps 6,610, 9,610 and 15,610) located it by
searching for the supply values implied by the decoded unit table:

| Words | Address | Dump values (human Zerg owner 1, Terran owner 2) |
| --- | --- | --- |
| Zerg available, used, max | `0x800B3AC0`, `0x800B3AF0`, `0x800B3B20` | 18, 8, 400 at all three dumps (Hatchery and Overlord; four Drones) |
| Terran available, used, max | `0x800B3B50`, `0x800B3B80`, `0x800B3BB0` | 20/8, 20/14, 36/24 (Command Center, then a completed Depot; 4, then 6 SCVs + 1 in training, then 10 SCVs + 1 SCV and 1 Marine in training) |
| Protoss | `0x800B3BE0`, `0x800B3C10`, `0x800B3C40` | 0, 0, 400 (no Protoss player) |

The layout is PC StarCraft's: per race, three `u32[12]` arrays in doubled
units, races in Zerg, Terran, Protoss order, `0x90` bytes apart. A unit in
training already counts as used (status bit 31 clear in its row), so the
cartridge reserves supply when training starts. The same Terran words also
appeared at `0x8010AF20` and `0x8010AF50`. `available` and `used` are `proven` in the layout file on this
evidence; `max` stays `candidate` (it never changed). The tapes log all 24
words of owners 0–3 with `watch` lines.

## Clock: lost follow-up reads

A tick's record is the read one step after the tick counter changes (S3).
In a whole match the tick sometimes changes on two consecutive steps, and
then the earlier tick has no follow-up read: 0 to 430 ticks per match
(median 171, up to 3% of a match's ticks). The Zerg-human lobbies lost
almost none (0–1), the others 93–430. Every lost tick was isolated (never
two in a row), each had its change read, and both captures of a pair lost
the same ticks. The S4 decoder counted source frames by records, which
would have run the clock up to 3% slow by the end of a match. The decoder
merged from the unit labs (#984) counts such a skipped tick from its change
read when it ran unpaused, and accepts a pair only when every skip is
explained that way and both runs keep the same clock; all 17 pairs pass that
rule. Checkpoints read the first sampled frame at or after the
checkpoint, and the golden records that frame for the GBA side.

The result screen pauses the game: after the human's result byte changes,
every tick is paused, so a capture ends a few unpaused frames after the
defeat.

## The result word

`0x800B936C` (copy at `0x8010DBE0`) holds one result byte per owner 0–3:
read as a big-endian word, owner 0 is the most significant byte. A byte is
0 until that owner is defeated and then 2: `0x02000000` when the human was
owner 0, `0x00020000` for owner 1 (as in S8), `0x00000002` for owner 3. In
`sk-ffa-w37` the first byte to change was the Protoss computer's, which ended
that capture before the human fell. The human's byte changed within a few
frames of its last building either reaching 0 HP or leaving the table: in
`sk-tvt-w0` the Command Center sat at 0 HP from frame 10,073 or earlier, the
byte changed at 10,076 and the row left at 10,082; in `sk-tvz-w0` the row
left at 3,999 and the byte changed at 4,000. Dying N64
buildings keep their rows for a few frames, so a building census measures
sprite cleanup on the N64 and immediate removal on the GBA. The gated result
check (`result.human_defeated`) therefore compares each engine's own
declaration (the human's N64 byte, the GBA `lost` flag); the building census
(`result.eliminated`) is reported only.

## Measurements

One function (`measure` in `tools/sc64_parity_skirmish.py`) reads a
normalised frame stream from either engine. The N64 stream comes from
tap.bin rows and banks plus the watched supply and result words; the GBA
stream from the parity trace's unit and building rows (engine types mapped to
cartridge ids by the runner), its bank and supply rows, and the runner's new
`--gather` file. The GBA engine keeps no cumulative gathered total, so a
host-only hook (`SC64_TEST_GATHER_OBSERVER` in `add_owner_resources`, whose
only callers are the two worker delivery sites) counts deliveries; it is an
`#error` outside host builds, and the shipped ROMs are byte-identical with
and without it (below). GBA used supply adds the supply of a queue head
that is training, as the cartridge counts it; waiting orders and a head
stalled at the cap hold none (`2026-11-25-waiting-training-supply.md`).

| Check | Definition | Band |
| --- | --- | --- |
| `start.corner` | Quadrant of the starting town hall | exact, never widened |
| `build.first.<Type>` | First frame a computer has the type: a building when placed, a unit when complete. Every type of the computer's race that is not in its starting roster has a line; a type the N64 never made is expected NEVER, so a GBA-only type fails | ±2 frames |
| `econ.gathered@T` | Minerals gathered since launch at T = 1,000, 2,000, 4,000, 6,000, 8,000 | ±one trip per worker, never below 0 |
| `supply.used@T`, `supply.cap@T` | Doubled supply used and provided (capped at 400) | exact |
| `workers@T` | Complete workers | exact |
| `attack.first_wave` | First frame three of a computer's armed units stand within 12 tiles of the human's town hall | ±2 frames |
| `result.human_defeated` | First frame the game has declared the human defeated | ±2 frames |
| `econ.rate_k<n>` | Minerals per frame while a player had n workers (at least 300 frames) | report |
| `attack.first_contact`, `attack.supply_at_wave`, `result.eliminated` | One armed unit in range; supply at the wave; no building row left | report |

A time not reached within the N64 run reads NEVER (999,999) on both engines;
a GBA checkpoint after its match ended reads −1. Both are outcomes, not
numbers: the judge counts a change to or from either as a regression,
whatever the baseline says. Unit rows at 0 HP are left out of worker counts
and waves on both engines (the N64 keeps a dying unit's row for a few
frames; the HP field, proven by the field map's damage checks, also reads
each type's full HP as its highest value over a match here, and the human's
last town hall fell 1,500, 590, 138, 0 before its row left). The GBA side is measured over
the N64 run's frames only. Where a lobby was captured
with several seeds, `envelope` widens each band to the union over the seeds,
so the envelope is the tolerance exactly where the N64 varied; a timing some
seeds never reached admits NEVER (`also_ok`) without stretching the numeric
band, but only from a seed whose run lasted past the time another seed saw
it. Goldens of one group must share a layout and differ in seed, and each
golden's tape must equal `lobby_tape()` for its lobby id byte for byte (all
17 captured tapes do), which pins the rows and the launch delay.

## Results

Times are source frames from the launch (N64 unpaused ticks; GBA steps x
4/5). "never" is NEVER: not reached within the N64 run. Seeds are
milliseconds since power-on at launch.

| Lobby | Seed | N64 human defeated | GBA human defeated | N64 lost follow-ups |
| --- | ---: | ---: | ---: | ---: |
| `sk-tvt-w0` | 97,835 | 10,076 | never | 298 |
| `sk-tvz-w0` | 96,834 | 4,000 | never | 129 |
| `sk-tvz-w37` | 97,451 | 15,253 | 14,640 | 430 |
| `sk-tvz-w101` | 98,518 | 17,857 | 14,678 | 171 |
| `sk-tvp-w0` | 98,835 | 9,084 | never | 291 |
| `sk-tvp-w37` | 99,453 | 9,177 | never | 224 |
| `sk-tvp-w101` | 100,521 | 9,022 | never | 250 |
| `sk-zvt-w0` | 100,837 | 10,262 | never | 1 |
| `sk-zvt-w37` | 101,455 | 10,572 | never | 0 |
| `sk-zvt-w101` | 102,523 | 9,921 | never | 1 |
| `sk-zvz-w0` | 99,837 | 17,609 | 14,453 | 0 |
| `sk-zvp-w0` | 101,839 | 9,084 | never | 1 |
| `sk-pvt-w0` | 98,835 | 10,138 | never | 283 |
| `sk-pvz-w0` | 97,835 | 16,679 | 14,542 | 362 |
| `sk-pvp-w0` | 99,837 | 8,929 | never | 252 |
| `sk-ffa-w0` | 91,828 | 3,566 | never | 93 |
| `sk-ffa-w37` | 92,446 | never (Protoss computer defeated at 4,775) | never | 158 |

`sk-tvp-w0` and `sk-zvp-w0` both ended at tick 9,085 on the N64. The match
rows confirm it: the human's last building fell to 0 HP by tick 9,081 in
both.

### Build orders

First frame of each type (buildings when placed, units when complete), N64
range over the captured seeds against the GBA range:

| Computer | Type | N64 | GBA |
| --- | --- | ---: | ---: |
| Terran (7 matches) | Barracks | 1,292–1,504 | 1,676–1,705 |
| | Supply Depot | 1,796–2,114 | 844–950 |
| | Refinery | 2,834–3,172 | 2,264–2,390 |
| | Marine | 2,855–3,118 | 3,239–3,290 |
| | Academy | 4,772–5,159 | 3,557–3,674 |
| | Medic | 6,432–6,939 | never |
| | Bunker | 7,059–7,397 | 7,070–7,501 |
| | Firebat | 7,376–7,741 | 5,197–5,312 |
| | Factory | 9,035–9,667 | 7,582–7,604, or never |
| Protoss (7 matches) | Pylon | 1,247–1,518 (2,811 once) | 656–714 |
| | Gateway | 1,992–2,291 | 1,794–1,849 |
| | Shield Battery | 3,866–4,145 | 4,144–4,445 |
| | Zealot | 3,879–4,193 | 3,312–3,364 |
| | Assimilator | 5,949–6,313 | 4,195–4,490 |
| | Cybernetics Core | never | 4,194–4,896 (all but `sk-ffa-w0`) |
| | Dragoon | never | 6,397–7,012 (every 1v1) |
| | Stargate | never | 5,698–5,943 (every 1v1) |
| | Fleet Beacon, Scout | never | Fleet Beacon in two, Scouts in two (8,610–8,648) |
| Zerg, late pool (4 matches) | Spawning Pool | 3,178–3,243 | 1,388–1,428 |
| | Extractor | 3,509–3,597 | 2,452–2,538 |
| | Creep Colony | 3,840–3,902 | 4,089–4,131 |
| | Sunken Colony | 4,870–4,999 | 4,454 |
| | Hydralisk Den | 6,345–6,468 | 5,840–5,959 |
| | Zergling | 6,809–6,954 | 3,223 |
| | Lair | 7,547–7,643 | 7,104–7,641 |
| | Hydralisk | 7,716–7,850 | 7,639–8,215 |
| | Lurker | 11,034–11,207 | never |
| | Queen's Nest | 14,805–17,459 | never |
| | Spire | never | 8,869–9,328 (all four) |
| Zerg, 9-pool rush (3 matches) | Spawning Pool | 738–857 | 1,428–1,449 |
| | Zergling | 2,495–2,530 | 3,223 |
| | First wave | 2,952–3,174 (human or the Protoss computer) | never |

The N64 Zerg computer plays two openings: a 9-pool rush (`sk-tvz-w0`, both
free-for-alls, where it hit the human once and the Protoss computer once)
and a late pool into Hydralisks and Lurkers (the other four). The seed picks
it (S8 found the same two modes); the start corner did not decide it, since
the rush happened from corners 1 and 2 and the late pool from corners 1 and
3. The TvZ envelope therefore spans both modes and is wide on purpose.

### Economy, supply and workers

| Player | Minerals gathered at 2,000 frames, N64 | GBA | GBA/N64 |
| --- | ---: | ---: | ---: |
| Passive human, 4 workers (17 matches) | 280–360 | 408–440 | 1.2–1.5 |
| Terran computer | 392–456 | 608–672 | 1.4–1.6 |
| Protoss computer | 408–528 | 632–672 | 1.3–1.6 |
| Zerg computer, late pool | 488–520 | 704–736 | 1.4–1.5 |
| Zerg computer, 9-pool rush | 264–304 | 704–712 | 2.3–2.7 |

Minerals per frame by worker count (WP9 curves, report only) show a similar
ratio: with 4 workers the passive human gathers 0.154–0.185 per frame on the
N64 and 0.213–0.229 on the GBA; a computer with 8 workers 0.238–0.344
against 0.339–0.417. The S4 Blood Bath measure (first delivery 105 against
154 frames, rate 0.212 against 0.176) is the same defect. Late in the match
the gap narrows for the late-pool Zerg (1.1–1.2x at 6,000–8,000 frames) and
stays at 1.5–1.8x for the Terran and Protoss computers, which have more
workers than the N64's.

Workers at 2,000 frames: N64 Terran computers 8 and Protoss 7–9, GBA 9–10
and 9. The GBA Zerg computer has 8 Drones at 1,000 frames (N64 7 in the
late-pool games, 3 when rushing), 9 at 2,000 and 4,000 (N64 9 and 12) and
10–11 at 6,000 (N64 15), with fewer Overlords (supply provided 38 against
52 at 6,000). Doubled supply used at 2,000 frames: N64 Terran 16–18, GBA
20–22; Protoss 14–20 against 20; Zerg 8–18 against 20. Supply provided runs
ahead on the GBA for Terran and Protoss (an earlier Depot or Pylon).

### First wave

| Computer | N64 | GBA |
| --- | ---: | ---: |
| Terran (1v1) | 9,492–10,051 | 10,084–10,309, or never within the N64 match |
| Protoss | 8,464–8,741 | 8,847–9,043 |
| Zerg, late pool | 14,906–17,537 | 14,040–14,320 |

## Re-run on the N64 mining time and rule tables

The goldens were first judged against the engine before #986 (workers hold
at the minerals for the N64 mining time of 102 updates; construction frames
hold for iscript waits) and #989 (the N64 rule-table values, among them a
150-mineral Spawning Pool, a 200-mineral Academy, a 600-frame Dragoon and
an 80/80 Zealot). Both change skirmish play, so all 17 scenarios were run
again on the merged engine. The N64 side did not change: re-decoding all 34
captures with the merged decoder into a private home gives goldens identical
to the shared ones, and none is stale.

| Family | Lines before | Now pass | Narrowed | Unchanged | Widened | New | Lines now |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `econ.gathered@T` | 166 | 6 | 160 | 0 | 0 | 0 | 160 |
| `build.first.<Type>` | 153 | 9 | 80 | 29 | 35 | 13 | 157 |
| `supply.used@T` | 77 | 22 | 21 | 19 | 15 | 11 | 66 |
| `supply.cap@T` | 64 | 15 | 7 | 37 | 5 | 0 | 49 |
| `workers@T` | 78 | 16 | 11 | 34 | 17 | 4 | 66 |
| `attack.first_wave` | 13 | 0 | 3 | 3 | 7 | 0 | 13 |
| `result.human_defeated` | 13 | 0 | 0 | 11 | 2 | 0 | 13 |
| All | 564 | 68 | 282 | 133 | 81 | 28 | 524 |

"Narrowed" and "widened" compare the distance from the N64 band (a value
that moved to the same distance on the other side counts as unchanged); the two
attack lines that changed outcome count by which side of the band they
moved to (`sk-tvt-w0`'s Terran now attacks at 10,084, closer than never;
`sk-zvt-w0`'s no longer attacks within the N64 run). Every mineral total
moved toward the N64, as expected of the mining time, and the human's
supply and worker lines already passed.

Reverting one change at a time on the merged tree splits the movement:
#986 alone fixes 67 lines and accounts for 92 of the 95 regressions; #989
alone fixes none and moves 22 lines out (16 regressions), mainly the first
Dragoon, now 150 frames earlier with the 600-frame N64 build time, and the
late-pool Zerg's economy and attack after the cheaper Pool. Each widened or
new line was checked against its golden before the baseline accepted it.
None is a measurement change (the goldens and the measurement code are
unchanged): every one is a GBA behaviour the N64 captures contradict. The
109 widened or new lines, by defect:

- **Protoss tech (defects 3 and 6), 24 lines:** with slower income the GBA
  Protoss places its Assimilator earlier (5 lines) and builds a Cybernetics
  Core (4) and Dragoons (3) sooner, and adds a Stargate in all five 1v1s
  (5), a Fleet Beacon in two (2) and Scouts in two (2). In `sk-ffa-w37`,
  where the N64 Protoss falls at 4,775 before building any of them, the GBA
  Protoss now builds an Assimilator, a Cybernetics Core and a Shield Battery
  (3). The N64 Protoss builds none of the Core,
  Dragoon, Stargate, Fleet Beacon or Scout within the run. It also has more
  supply used at 4,000 frames once (1 line) and more Probes and supply at
  6,000–8,000 (14).
- **Terran opening (defect 2), 17 lines:** the Depot-first order makes the
  Barracks and first Marine wait longer for 150 minerals (12), and the
  Bunker and Factory move a few frames further out (5). The GBA Terran also
  has more SCVs and supply after 6,000 frames (13 lines).
- **Zerg Drones and Overlords (defects 4 and 5), 31 lines:** one Drone more
  and more supply used at 1,000–4,000 frames (17), fewer Drones and
  Overlords at 6,000 (7), the Creep Colony later (4) and the Pool 3–40
  frames further out (3).
- **Timings (defects 8 and 9), 9 lines:** Protoss and Terran waves 10–80
  frames later than before (and `sk-zvt-w0`'s Terran no longer attacks
  within the run), the late-pool Zerg's 60–90 frames earlier, and two human
  defeats 35–60 frames earlier.

## Defects

Each line is a GBA defect against the N64 ROM, with its numbers above. All
are baseline lines (`tools/parity/baseline.json`, GBA values).

1. **Mining rate faster (all races, all players).** The passive human
   gathers 1.2–1.5x the N64's at every checkpoint, and 1.2–1.4x per frame
   with 4 workers. The computers gather 1.3–1.7x at 1,000 and 2,000 frames,
   up to 2.7x against the rushing Zerg, which mines with fewer Drones. At
   6,000–8,000 frames the late-pool Zerg's gap narrows to 1.1–1.2x, while the Terran and
   Protoss computers, with more workers, stay at 1.5–1.8x. The S4 first
   delivery comes at 105 against 154 frames. Before the N64 mining time
   (#986) the ratio at 2,000 frames was 1.8–2.6x, and up to 4.1x against
   the rushing Zerg.
2. **Terran build order.** The GBA builds the Supply Depot first (844–950)
   and the Barracks second (1,676–1,705); the N64 builds the Barracks first
   (1,292–1,504) and the Depot at 1,796–2,114. With slower income the GBA
   Barracks now comes 200–410 frames and the first Marine 160–420 frames
   after the N64's (both later than before #986). The GBA's Academy and
   Firebats come 1,100–1,600 and 2,100–2,500 frames early; it never trains a
   Medic (N64 6,432–6,939).
3. **Protoss build order.** The GBA places its first Pylon about 2x earlier
   (656–714 against 1,247–1,518), its Gateway 190–490, its first Zealot
   550–860 and its Assimilator 1,750–2,120 frames early, and
   builds a Cybernetics Core, Dragoons and a Stargate that the N64 computer
   never builds before the human falls (two matches also reach a Fleet
   Beacon and two train Scouts).
4. **Zerg openings.** The GBA has no 9-pool rush: its Pool always comes at
   1,388–1,449 (N64: 738–857 when rushing, 3,178–3,243 otherwise). In the
   late-pool games the GBA reaches most tech steps early (Pool 1,750–1,860,
   Extractor 970–1,130, Hydralisk Den 390–630 frames), its Creep Colony
   190–290 frames late, never makes a Lurker or Queen's Nest (the engine has no
   Lurker), and builds a Spire instead.
5. **Zerg Drones and Overlords.** The GBA Zerg computer has one Drone more
   than the N64's late-pool Zerg at 1,000 frames, then stops at 9 through
   4,000 frames and has 10–11 at 6,000 (N64 12 and 15), with fewer
   Overlords at 6,000 (supply provided 38 against 52).
6. **GBA-only types.** Within the N64 run the GBA builds types the N64
   computer never makes: Cybernetics Core, Dragoons and Stargates (all five
   1v1s against Protoss; the Core also in `sk-ffa-w37`), Fleet Beacons and
   Scouts (two matches each), Spire (four Zerg games), Machine Shop, Vulture
   and Starport (Terran), and early Creep Colonies and Extractors against
   the rush openings.
7. **Supply.** Used and provided supply at the checkpoints follow the build
   orders; the passive human's `supply.cap` passes at all 78 checkpoints.
8. **Attack timing.** The GBA Terran attacks 260–550 frames later or not
   within the N64 match, the Protoss 290–450 frames later, the late-pool
   Zerg 600–3,200 frames earlier.
9. **Outcome.** In 12 of the 16 N64 matches that ended in the human's
   defeat, the GBA human survived to the end of the N64 run; in the other 4
   (late-pool Zerg) the GBA defeat came 600–3,200 frames early.

## Coverage

Covered: all nine 1v1 race pairs (one seed each), three seeds each for ZvT,
TvZ and TvP (one per computer race), and the three-computer free-for-all
with two seeds. The 1v1s and the free-for-all use the Free For All game
type, which the GBA uses by default.

Not covered, and why:

- **More seeds** for the six other 1v1 pairs and the free-for-all: every N64
  capture on the machine shares one lock, and the 34 captures here took
  about 2.3 hours of lock time beside the other sessions' captures. Their
  bands are therefore exact (no envelope).
- **Random races, Melee game type, two-computer lobbies, other maps:** not
  captured; the GBA runner supports Free for All on Blood Bath only.
- **Human races in the free-for-all** other than Terran (the default lobby).
- **The rest of a free-for-all after its first result:** the tape stops
  when any player's result byte changes, so `sk-ffa-w37` ends when the
  Protoss computer falls. A tape that waits for the human's own byte needs
  the owner before launch, which the seed decides.
- **Gas:** the gas arrays stay `candidate`; gas gathered is decoded but not
  gated.
- **Computer eliminations:** the shipped GBA builds declare no computer
  defeat (`SC64_NATIVE_SKIRMISH_ELIMINATION` is a preview flag), so only the
  building census is reported.

## Layout file changes

The layout gains the supply arrays, the per-owner result bytes and a `seed`
entry for the launch seed (the first word of the `rng` block, which keeps its
recorded definition), and adds this audit's evidence to the HP field. The
per-observation field digests (`layout_fields_sha256`) cover the new kinds
(`supply.*`, `result.word`, `rng.seed`), so a later edit to those definitions
marks the skirmish goldens stale, and an unrelated edit does not.

## Cost and checks

| Item | Value |
| --- | --- |
| N64 captures | 34 (17 pairs), 18,222–66,570 frontend steps, 78–500 s wall each under load; about 300 MB of tap.bin per whole match before compression |
| Private data kept | 516 MB of run directories (taps as `tap.bin.zst`, ROM and lobby state hard-linked to the master copies), 17 golden files |
| GBA side | One match runs in 6–8 s of CPU (`--no-route-checks`, render on); the 17 skirmish scenarios take about 2 minutes of `./dev parity` |
| Shipped ROMs | `StarCraft-GBA`, Terran, Zerg Z10, Protoss P10 and Showcase Transport1 ROMs are byte-identical with and without this change's `game.c` |

Every capture held the shared native lock, so none ran beside another
capture (the lock serialises all captures on the machine). The pairs
reproduced bit for bit regardless of the host load around them.

The exploration capture (human Zerg against Terran, seed 100,837, with RAM
dumps) and the 17 pairs are kept under the main checkout's
`.local/parity-harness/skirmish`.
