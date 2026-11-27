# N64 parity enemy labs: weapons against mission enemies (phase 2, second part)

Date: 2026-11-27, on top of the tech-tree labs branch (`claude/parity-tech-labs`).
The production spike found that the cartridge has no attack on the player's own units.
B over one of the player's own units gives an attack-move (order 14) and does no damage
([tech-tree labs](2026-11-27-parity-tech-labs.md)). The arsenal labs of the
[coverage map](2026-11-27-parity-coverage-map.md) therefore take the fallback the map
names: enemy targets in campaign missions. This round adds nine of them.

Each GBA-vs-N64 difference below is a GBA defect to fix (owner decision, 2026-11-26).
The baseline holds the GBA values only so that `./dev parity` reports them as KNOWN.
Captures, scenario files, tapes and goldens stay in the private fixture home; this page
keeps the method, the verdicts and the measured numbers.

## Method

Each lab reuses the attack measurement, the `lab-mission` GBA runner and the judge of the
[unit-class labs](2026-11-26-parity-unit-labs.md). Only the lab specs are new. Each lab
was built in four steps:

1. **Lab state.** A capture launches the mission with the progress-byte guest edit,
   dismisses the opening text boxes for 2,500 steps, and saves the state, as for the
   Z2, T3 and T10 labs. Labels are `n64-staged`.
2. **Exploration.** One capture per candidate selects one human unit and presses B at an
   enemy's own position. B at a fogged point is an attack-move, so the subject engages
   whatever it meets on the way. Each exploration took about 20 seconds; the first fight
   it showed decided whether the pair became a lab.
3. **Golden.** Two fresh captures of the same compiled tape, with identical hash chains,
   become the golden.
4. **GBA run.** The runner launches the mission and gives the same attack-move at the
   halved point, at the same source-frame offset. Terran builds have no targeted attack,
   so an attack-move is also the only order every GBA profile replays alike.

The human player is the one owner of a unit type that only the human fields at the lab's
first frame. In Z7, Zerg against Zerg, every type the human fields also has an enemy
owner, so the spec names the map's human slot instead (`human_owner`). That slot must own a unit of the subject's type.

### What the CHK scan could not show

The coverage map's private scan found about 35 enemies that stand alone at launch (no
other enemy within 288 px). Most of them could not be reached:

- **Guarded routes.** A subject sent toward a lone enemy usually met other enemies on
  the way. In T1, T4, T5, T6, Z5 and Z7, the CHK placements do not show that the route
  is guarded.
- **Reacting defenders.** Base defenders move when an enemy wanders near them (a second
  T3 Marine, the P3 Scouts). That breaks the lab's rule of exactly one moving subject.
- **Unselectable subjects.** In T7 the subject could not be selected: the tape stopped
  with exit 9 twice. A computer Firebat stood 49 px from it.
- **Cloaked targets.** Observers were left out: they cannot be targeted without
  detection.

Clean first fights against one enemy, or one small group, became the labs below. Their
targets are not always the enemy the subject was sent at.

### Comparable starts

The GBA lab starts at the mission launch. The N64 lab starts after the opening
transmissions, about 640–700 ticks of the game clock (which also runs while a text box
is open). A target that moved or was born during the opening is therefore not
comparable. Every kept lab's victims stand exactly on their CHK placement at the N64
lab's first frame.

Rejected for this reason or others:

| Candidate | Why rejected |
| --- | --- |
| P2 Dragoon on Overlord | The Overlord drifts (order 159). It was 541 px from its placement when the lab began |
| P3 Dragoon on Zergling | The Zergling it fought was born after the lab state |
| P8 Dragoon on Zealot | The N64 subject moved about 90 px before its command, so the GBA finds no unit at its launch point |
| Z1 Zergling on Marine | The GBA Zergling dies after one hit. Its cooldown has no GBA value, and the harness cannot accept a missing value |
| P1 Zealot on Zerglings | The N64 golden gates the cooldown and first-hit range: its blades land 2 frames apart (gaps 2, 21, 2, 2, 18 …), 8 each. The GBA applies both blades in one frame, so neither check gets a GBA value, and a missing value cannot be accepted into the baseline. This is a GBA defect; the lab waits for a measurement that times a pair of hits in one frame as one attack |

## Labs

| Golden | Mission | Subject → target | Weapon class covered |
| --- | --- | --- | --- |
| `lab-hydralisk-dragoon` | Z2 | Hydralisk → Dragoon | explosive, on shields |
| `lab-mutalisk-dragoon` | Z2 | Mutalisk → Dragoon | air-to-ground glave, on shields |
| `lab-marine-marine` | T5 | Marine → enemy Marine | normal on small |
| `lab-mutalisk-hydralisk` | Z7 | Mutalisk → Hydralisk | normal on medium |
| `lab-marine-hydralisk` | T3 | Marine → Hydralisk | normal on medium; explosive on small (damage taken) |
| `lab-vulture-hydralisk` | T3 | Vulture → Hydralisk | concussive on medium (half damage) |
| `lab-guardian-zealot` | Z6 | Guardian → Zealot pair | long-range acid spore, on shields |
| `lab-zergling-goliath` | Z1 | Zergling → Goliath | melee on large, armour (5 − 1) |
| `lab-dragoon-zergling-pack` | P1 | Dragoon → three Zerglings | explosive on small (half damage) |

P1's allied Protoss rows stay more than 1,200 px from the Dragoon's fight, so none of
their hits can be credited to it.

## Results

`./dev parity` on these nine goldens: 18 PASS, 18 KNOWN, 17 REPORT. Times are source
frames, distances N64 px centre to centre.

| Lab | Damage per hit | Cooldown | First-hit range | Survival | Damage taken |
| --- | --- | --- | --- | --- | --- |
| Hydralisk → Dragoon | shields 10 PASS | 15 PASS | **N64 104.6, GBA 126.5** | **N64 91, GBA 152** | 15 (report) |
| Mutalisk → Dragoon | shields 9 PASS | **N64 32, GBA 30** | **N64 114, GBA 92** | **N64 420, GBA 332** | not measured on the GBA (report) |
| Marine → Marine | 6 PASS | 15 PASS | **N64 143.3, GBA 126.5** | **N64 92, GBA 252** | not measured on the GBA (report) |
| Mutalisk → Hydralisk | 9 PASS | N64 31, GBA 30 PASS | **N64 28.3, GBA 92** | **N64 217, GBA absent** | not measured on the GBA (report) |
| Marine → Hydralisk | 6 PASS | 15 PASS | **N64 102.1, GBA 126.1** | N64 103, GBA 106 PASS | 5 PASS |
| Vulture → Hydralisk | 10 PASS | N64 30.5, GBA 30 PASS | **N64 107.8, GBA 127.1** | **N64 89, GBA 121** | not measured on the GBA (report) |
| Guardian → Zealot | shields 20 PASS | (one N64 hit) | **N64 244.6, GBA 256** | **N64 124, GBA absent** | not measured on the GBA (report) |
| Zergling → Goliath | 4 PASS | N64 9, GBA 8 PASS | **N64 34.5, GBA 14** | N64 48, GBA 45 PASS | not measured on the GBA (report) |
| Dragoon → Zerglings | 10 PASS | **N64 27.5, GBA 30** | **N64 49.2, GBA 127.3** | **N64 329, GBA absent** | not measured on the GBA (report) |

The first-hit time is report-only, as in every attack lab (it includes the walk and the
N64 input lag), so it is left out of the table. Every gating damage value matches the
N64 exactly. These include the explosive and concussive size multipliers (10 for the Dragoon
and 10 for the Vulture, both half damage) and the Goliath's 1 point of armour.

### GBA defects (accepted as KNOWN)

1. **First-hit range (9 lines).** On the N64 the target usually closes in before the
   first shot. In seven of the nine fights it had moved 17–109 px toward the subject by
   then. The GBA subject fires at full weapon range: 126–127 px for the 128 px weapons
   (Marine, Hydralisk, Vulture, Dragoon), 256 px for the Guardian and 92 px for the
   Mutalisk. GBA melee hits land at 14 px centre distance against the N64's 34.5.
   Range is measured centre to centre on both engines.
2. **Survival (7 lines).** The N64 fights end differently: other enemies join (a second
   Dragoon in Z2; hits of two sizes in Z7), and the targets come out to fight. On the GBA
   the subject lives longer (Hydralisk, Marine, Vulture), dies sooner (Mutalisk on
   Dragoon), or does not die in the run (`absent`: Mutalisk on Hydralisk, Guardian,
   Dragoon on Zerglings).
3. **Cooldown with no variation (2 lines).** GBA cooldowns are fixed, 30 for both the
   Mutalisk and the Dragoon. The N64's short gaps vary: Mutalisk 28–32 (the Dragoon
   fight also has gaps of 63 and 90 frames; medians 32 against
   the Dragoon and 31 against the Hydralisk, which passes) and Dragoon 24–32 (median
   27.5).

### Wall time

Each lab adds one GBA run of about 0.4 s of one core to `./dev parity`, in the worker
pool. On this machine under load, run back to back: 97 s for the private home with these
nine goldens and `tech-terran`, 89 s for the shared home.

## Next

- **Weapons with no clean fight yet.** Explorations for the Firebat (T4), Ghost and
  Wraith (T8), Scout (P3), Ultralisk (Z8) and Arbiter (P9) met other enemies first or
  never fought. The Siege Tank (T10) and the Carrier (P8) were not tried. No mission
  starts the human with a Goliath. Routing a subject round a guarded route first, with a
  move to a staging point, needs a two-order attack measurement. That is small work in
  `lab_attack_n64` and `lab_attack_gba`.
- **Zealot.** The paired-blade timing needs a measurement that times a pair of hits in
  one frame as one attack.
- **Brood War units** (Medic, Valkyrie, Lurker, Devourer, Dark Archon, Corsair) are not
  fielded by the human at any mission start. A Blood Bath lobby against a computer of
  the target race, as the coverage map proposes, still covers them.
