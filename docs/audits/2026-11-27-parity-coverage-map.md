# N64 parity coverage map (phase 1)

Date: 2026-11-27, main `8b42d7a7`. Scope: every unit, building, spell,
research and upgrade of the three races, and all 30 Episode I–III campaign
missions, set against what the N64 parity harness
([`PARITY-HARNESS.md`](../PARITY-HARNESS.md)) measures today, followed by
the smallest set of new scenarios that would cover the rest. Nothing here
was captured: the sources are the 31 goldens in the shared fixture home, the
offline layer's reports ([`PARITY-L0.md`](../PARITY-L0.md)), the field-map
launch reports ([field-map audit](2026-11-26-parity-field-map.md)) and the
missions' CHK placements read from the owned ROM. This page names unit
types and missions only; per-map counts, positions and table values stay
private.

The proposal in the last sections is for the parent session's OK. No
scenario in it has been recorded.

## How an item can be covered

| Layer | What it checks | Where |
| --- | --- | --- |
| L0 tables | Static values: cost, supply, hit points, shields, armour, build time, weapon damage, cooldown and range, speed, upgrade and tech cost and time, against the ROM's `.dat` tables | `make parity-tables` (counts in `./dev parity`, not gating) |
| L0 content | Mission launch censuses, iscript sequences, sound ids | `make parity-content` |
| Field map | N64 launch census of each mission against its CHK; player tech and upgrade arrays at launch | `tools/sc64_parity_fieldmap.py` reports (no golden) |
| Dynamic, gating | The same behaviour measured on both engines from an N64 recording | `./dev parity` goldens |

L0 tells whether the engine holds the ROM's number; only a dynamic check
tells whether the engine *does* what the number says (a build time scaled
twice, a cooldown with jitter, damage type against size, an upgrade that is
researched but never applied). The coverage below counts dynamic checks.

Legend for the "Measured today" columns:

| Code | Meaning | Golden |
| --- | --- | --- |
| M | Move lab: cruise speed, acceleration, leg time, turn cost | `lab-*-move`, `t1-move`, `t1-fields` |
| A | Attack lab, as the subject: damage per hit, cooldown, range at first hit, the subject's survival (first hit time is report only; the Hydralisk lab gates shield damage only, never hit-point damage on the Zealot) | `lab-marine-zergling`, `lab-hydralisk-zealot` |
| F | Attack lab, as the foe: damage dealt to the subject (gating for the Zergling, report only for the Zealot) | the same two labs |
| T | Train or build duration (gating) | `bb-train` |
| t | Train or build duration, report only | `bb-train` |
| S | Skirmish AI build-order first: a building's first appearance, a unit's first completion (decision plus build time, per lobby; N64 time recorded in at least one lobby) | `sk-*` |
| I | Income: gathering rate, first delivery, trip yield | `bb-income`, `sk-*` |
| R | Report-only timeline or bullet check, N64 value only (the GBA side has none) | `t3-opening` |
| — | Nothing dynamic beyond the skirmish NEVER lines (below) | |

Every unit and building type of a computer's race has a `build.first`
line in each `sk-*` lobby. Where the N64 computer never built it, the line
expects NEVER: a GBA computer that builds it early fails (early Dragoons
and Spires do today), but no time is measured. The tables do not mark
these lines; they apply to every unit and building without an S.

"Human at start" lists the missions whose CHK gives the human player the
type at launch (labels as in the field-map audit).

## What the harness measures today

| Group | Goldens | Gating checks |
| --- | --- | --- |
| T1 movement and fields | `t1-move`, `t1-fields` | group move, order timing, fog, walk cycle |
| Blood Bath economy and production | `bb-income`, `bb-train` | income, worker and early building times |
| Unit labs | 7 move (Marine, SCV, Hydralisk, Siege Tank, Overlord, Mutalisk, Vulture), 2 attack (Marine on Zergling, Hydralisk on Zealot) | per lab, above |
| Skirmish lobbies | 17 `sk-*` (1v1 in every race pair, free-for-all, seeds) | start corner, build order firsts, economy and supply curves (computers and the human), first wave, the human's defeat (other eliminations and first contact report only) |
| Mission opening | `t3-opening` | none (report only) |

Scorecard on main: KNOWN 559, PASS 827, REPORT 202, fixture set
`d895d4f65f7fcf8e` (31 golden files). 1,328 of the 1,386 gating lines
come from the `sk-*` lobbies, which dominate the count
but see only a passive human and what the computers do until the human
falls.

## Units

### Terran

| Unit | Human at start | Measured today | Proposed |
| --- | --- | --- | --- |
| SCV | T1 T2 T3 T6 T7 T8 T9 T10 | M T I | `tech-terran` |
| Marine | T1–T10 | M A S R | `tech-terran`, `arsenal-terran` |
| Firebat | T4 T6 T10 | S | `tech-terran`, `arsenal-terran` |
| Medic | — | S | `tech-terran`, `spell-terran` |
| Ghost | T8 | — | `tech-terran`, `arsenal-terran`, `spell-terran` |
| Vulture | T3 T7 T9 T10 | M R | `tech-terran`, `arsenal-terran` |
| Spider Mine | — | — | `arsenal-terran` |
| Siege Tank (tank) | T10 | M | `tech-terran`, `arsenal-terran` |
| Siege Tank (siege) | — | — | `arsenal-terran` (mode change time, splash) |
| Goliath | — | — | `tech-terran`, `arsenal-terran` |
| Wraith | T8 | — | `tech-terran`, `arsenal-terran` |
| Dropship | T8 T9 | — | `tech-terran` |
| Science Vessel | T7 | — | `tech-terran`, `spell-terran` |
| Battlecruiser | — | — | `tech-terran`, `arsenal-terran`, `spell-terran` (Yamato) |
| Valkyrie | — | — | `tech-terran`, `arsenal-terran` |
| Nuclear Missile | — | — | `tech-terran`, `spell-terran` |

### Zerg

| Unit | Human at start | Measured today | Proposed |
| --- | --- | --- | --- |
| Larva, Egg | (created after launch) | — | `tech-zerg` (larva spawn interval, egg time) |
| Drone | Z1–Z4 Z6–Z10 | I | `tech-zerg` |
| Zergling | Z1 Z4 Z5 Z8 Z9 Z10 | F S | `tech-zerg`, `arsenal-zerg` |
| Overlord | Z1–Z4 Z6–Z10 | M | `tech-zerg` |
| Hydralisk | Z2–Z4 Z6–Z10 | M A S R | `tech-zerg`, `arsenal-zerg` |
| Lurker | — | S | `tech-zerg`, `arsenal-zerg` |
| Mutalisk | Z2 Z3 Z6 Z7 | M | `tech-zerg`, `arsenal-zerg` |
| Guardian | Z6 | — | `tech-zerg`, `arsenal-zerg` |
| Devourer | — | — | `tech-zerg`, `arsenal-zerg` |
| Scourge | — | — | `tech-zerg`, `arsenal-zerg` |
| Queen | Z4 | — | `tech-zerg`, `spell-zerg` |
| Broodling | — | — | `spell-zerg` (Spawn Broodlings) |
| Infested Terran | — | — | `tech-zerg` (after infesting the computer's Command Center), `arsenal-zerg` |
| Defiler | — | — | `tech-zerg`, `spell-zerg` |
| Ultralisk | Z8 Z10 | — | `tech-zerg`, `arsenal-zerg` |

### Protoss

| Unit | Human at start | Measured today | Proposed |
| --- | --- | --- | --- |
| Probe | P2 P3 P5 P7–P10 | t I | `tech-protoss` |
| Zealot | P1–P9 | F S | `tech-protoss`, `arsenal-protoss` |
| Dragoon | P1–P5 P7–P9 | — | `tech-protoss`, `arsenal-protoss` |
| High Templar | P4 | — | `tech-protoss`, `spell-protoss` |
| Dark Templar | (hero only: P10) | — | `tech-protoss`, `arsenal-protoss` |
| Archon | — | — | `tech-protoss` (merge time), `arsenal-protoss` |
| Dark Archon | — | — | `tech-protoss`, `spell-protoss` |
| Reaver, Scarab | — | — | `tech-protoss`, `arsenal-protoss` |
| Shuttle | P5 | — | `tech-protoss` |
| Observer | P7 | — | `tech-protoss` |
| Scout | P3 P7 | — | `tech-protoss`, `arsenal-protoss` |
| Corsair | — | — | `tech-protoss`, `arsenal-protoss`, `spell-protoss` |
| Carrier, Interceptor | P8 | — | `tech-protoss`, `arsenal-protoss` |
| Arbiter | P9 | — | `tech-protoss`, `arsenal-protoss`, `spell-protoss` |

### L0 status of the units

Refreshed on `8b42d7a7` (`tools/sc64_parity_tables.py`, private report):
86 gameplay cells differ, down from 130 on 2026-11-26. For the units above:

The differ has two layers: the raw-id stock tables (speed, weapons,
sight) and the release race rules (cost, supply, hit points, shields,
armour, build time).

- No release race-rule row (reported absent): Valkyrie, Corsair, Dark
  Templar, Dark Archon, Devourer, Lurker (and its egg), Larva, Broodling,
  Cocoon and the Nuclear Missile. A dynamic check of an actor with no
  engine counterpart reads absent on the GBA, a defect to list, never a
  skip. A missing row is not always a missing mechanic: Larvae exist in
  custom games as a timed count per town hall, so `tech-zerg` can still
  measure the spawn interval. Some of these still have stock rows
  (Valkyrie and Dark Archon appear below for speed).
- Stock cells that differ: top speed of SCV, Drone, Probe, Vulture,
  Dropship, Valkyrie, High Templar, Archon and Dark Archon (the 22 speed
  cells held back because they break campaign controllers). Race rules:
  the ground weapon of the Infested Terran.
- The other cells compared for the units above match the ROM, so a dynamic
  gap on those cells is behaviour, not data. Cells the differ does not
  compare are not covered by L0.

### Heroes and campaign-only units

Infested Kerrigan (human in Z5, Z6, Z8), the Hero Dark Templar (human in
P10), Raynor, Kerrigan, Tassadar, Zeratul, Fenix, Hunter Killer and the
named ships (allied or neutral rows in 15 missions) have no dynamic check.
They are covered by the mission openings below where they act, and by the
L0 rows. No dedicated hero lab is proposed: their weapons are table rows
already compared offline, and the behaviour classes they share are covered
by the race arsenals.

## Buildings

| Race | Building | Human at start | Measured today | Proposed |
| --- | --- | --- | --- | --- |
| Terran | Command Center | T1–T3 T7–T10 | — | `tech-terran` (lift-off and landing too) |
| | Supply Depot | T1–T3 T7 T9 T10 | T S | `tech-terran` |
| | Refinery | T7 | S | `tech-terran`, I (gas) |
| | Barracks | T2 T3 T7–T9 | T S | `tech-terran` |
| | Engineering Bay | T3 T7 | — | `tech-terran` |
| | Bunker | T3 T7 | S | `tech-terran`, `arsenal-terran` (loaded) |
| | Academy | T3 | S | `tech-terran` |
| | Missile Turret | T3 T7 | — | `tech-terran`, `arsenal-terran` |
| | Factory | T7–T9 | S | `tech-terran` |
| | Starport | T7 T8 | — | `tech-terran` |
| | Armory | — | — | `tech-terran` |
| | Science Facility | T7 T8 | — | `tech-terran` |
| | Comsat Station, Nuclear Silo, Machine Shop, Control Tower, Covert Ops, Physics Lab (add-ons) | Control Tower: T7 | — | `tech-terran` |
| Zerg | Hatchery | Z1–Z4 Z6 Z8–Z10 | — | `tech-zerg` |
| | Lair, Hive (morphs) | — | S (Lair) | `tech-zerg` |
| | Creep Colony | Z1 Z2 Z4 Z6 Z8 Z10 | S | `tech-zerg` |
| | Sunken Colony, Spore Colony (morphs) | Z3 Z4 Z6 Z9 | S (Sunken) | `tech-zerg`, `arsenal-zerg` |
| | Extractor | Z4 | S | `tech-zerg` |
| | Spawning Pool | Z2 Z3 Z6 Z9 | S | `tech-zerg` |
| | Evolution Chamber | Z9 | S | `tech-zerg` |
| | Hydralisk Den | Z3 Z6 | S | `tech-zerg` |
| | Spire, Greater Spire (morph) | — | — | `tech-zerg` |
| | Queen's Nest | — | S | `tech-zerg` |
| | Nydus Canal | Z8 | — | `tech-zerg` (exit build, transit) |
| | Ultralisk Cavern, Defiler Mound | — | — | `tech-zerg` |
| | Infested Command Center | — | — | `spell-zerg` (Infestation) |
| Protoss | Nexus | P2 P3 P5 P7 P8 P10 | — | `tech-protoss` |
| | Pylon | P2 P3 P5 P7 P8 P10 | t S | `tech-protoss` |
| | Assimilator | P7 | S | `tech-protoss` |
| | Gateway | P5 P7 | S | `tech-protoss` |
| | Forge | P7 P8 P10 | — | `tech-protoss` |
| | Photon Cannon | P7 P8 P10 | — | `tech-protoss`, `arsenal-protoss` |
| | Cybernetics Core | P7 | — | `tech-protoss` |
| | Shield Battery | — | S | `tech-protoss`, `spell-protoss` (recharge) |
| | Robotics Facility, Robotics Support Bay, Observatory | — | — | `tech-protoss` |
| | Stargate, Fleet Beacon | Stargate: P7 | — | `tech-protoss` |
| | Citadel of Adun, Templar Archives, Arbiter Tribunal | Citadel: P7 | — | `tech-protoss` |

Installation doors and traps (T4, Z5, P6), the Cerebrates, the Overmind,
the Ion Cannon, the Stasis Cell, beacons and powerups are mission content:
the L0 censuses place them exactly in all 30 missions, and the mission
openings below see what they do in the first minutes.

## Spells and abilities

No spell, ability, research or upgrade has a gating dynamic check today.
The only dynamic number is `research.duration` for the T3 computer's
Burrowing, report only and N64 side only. The L0 tech table compares five
rows (Yamato Gun, Personnel Cloaking, Consume, Ensnare, Psionic Storm) on
the engine constants it reads; none differs in the refreshed report.
Another 38 tech rows have no engine row, and the remaining ROM rows
carry no price to compare.

The "GBA" column comes from reading the engine's code on `8b42d7a7`:

- **yes**: implemented.
- **partial**: only placed or scripted units have it, or the feature has
  no command.
- **no**: absent.

Profiles: T is the Terran release, Z10 the Zerg release, P10 the Protoss
release. "Custom" is a Blood Bath or custom game on those profiles.

| Spell or ability | Caster | N64: human at start | GBA | Proposed |
| --- | --- | --- | --- | --- |
| Stim Packs | Marine, Firebat | T1–T10 (research needed) | no | `spell-terran` |
| Spider Mines | Vulture | — (placed mines in T5) | partial: T5's placed mines only | `arsenal-terran`; `lab-t5-mines` |
| Siege Mode | Siege Tank | T10 | partial: placed Siege Tanks, no mode change | `tech-terran` (mode time), `arsenal-terran` |
| Scanner Sweep | Comsat Station | — | no | `spell-terran` |
| Defensive Matrix, Irradiate, EMP Shockwave | Science Vessel | T7 | no | `spell-terran` |
| Lockdown, Personnel Cloaking, Nuclear Strike | Ghost | T8 | no; the nuke exists behind a probe flag only | `spell-terran` |
| Cloaking Field | Wraith | T8 | no | `spell-terran` |
| Yamato Gun | Battlecruiser | — | yes (T, P10; not Z10) | `spell-terran` |
| Heal, Restoration, Optical Flare | Medic | — | no (the Medic trains but has no orders) | `spell-terran` |
| Lift-off, landing | Terran buildings | T7–T10 | partial: the late Terran campaign (T7–T10); the Science Facility under its own flag | `tech-terran` |
| Repair | SCV | T1 T2 T3 T6–T10 | partial: T6 buildings, and the late Terran campaign (T7–T10) | `lab-t10-repair` |
| Bunker load | Bunker | T3 T7 | yes | `arsenal-terran` |
| Transport (load, unload, capacity) | Dropship, Shuttle, Overlord | T8 T9, P5, Z1–Z4 Z6–Z10 | yes (Overlord after Ventral Sacs) | `tech-*` |
| Burrow | Zerg ground units | Z1–Z10 (research needed) | partial: placed burrowed units in P6 and T6 | `spell-zerg` |
| Infestation | Queen | Z4 | no | `spell-zerg` |
| Parasite, Ensnare, Spawn Broodlings | Queen | Z4 | no (Ensnare rules exist but are never enabled) | `spell-zerg` |
| Dark Swarm, Plague, Consume | Defiler | — | Consume: Infested Kerrigan in Z5 only | `spell-zerg`; `lab-z5-kerrigan` |
| Lurker Aspect | Hydralisk | — | no | `tech-zerg` |
| Lair, Hive, Greater Spire, Sunken, Spore morphs | Zerg buildings | Z1–Z10 | yes in custom games; in the campaign only Lair and the colonies (their own paths) | `tech-zerg` |
| Guardian, Devourer morphs | Mutalisk | Z2 Z3 Z6 Z7 | Guardian: host tests only; Devourer: no | `tech-zerg` |
| Nydus Canal | Nydus Canal | Z8 | partial: transit through Z8's placed pairs | `lab-z8-nydus` |
| Psionic Storm | High Templar | P4 | yes, but campaign only: P4 Templar and Tassadar, P6 Tassadar, Z6 Kerrigan | `spell-protoss`; `lab-p4-storm` |
| Hallucination | High Templar | P4 | partial: one scripted copy in Z6 | `spell-protoss` |
| Archon Warp, Dark Archon Meld | High and Dark Templar | P4 | no (the Archon is a placed actor) | `tech-protoss` |
| Recall, Stasis Field | Arbiter | P9 | no | `spell-protoss` |
| Cloaking aura | Arbiter | P9 | yes (passive, P7 onward) | `spell-protoss` (detection by an Observer) |
| Mind Control, Feedback, Maelstrom | Dark Archon | — | no | `spell-protoss` |
| Disruption Web | Corsair | — | no | `spell-protoss` |
| Shield Battery recharge | Shield Battery | — | yes | `spell-protoss` |
| Interceptor build | Carrier | P8 | yes (P10; T and Z10 through the Carrier release) | `tech-protoss` |
| Scarab build | Reaver | — | yes (P10 only) | `tech-protoss` |

Three things follow for the proposal:

1. **Most spells are absent on the GBA.** A spell lab on a Blood Bath
   start measures the N64 once; on the GBA it reads absent. That is a
   listed defect with a golden ready, not a passing check.
2. **Most of the spells the GBA does have belong to campaign heroes.**
   Yamato, Shield Battery recharge, the Arbiter's aura and the production
   abilities work in custom games. Psionic Storm, Consume, Personnel
   Cloaking and Hallucination work only through Tassadar, Infested
   Kerrigan or one mission's Templar. A Blood Bath High Templar on the
   GBA has no Storm.
3. **So implemented spells need mission labs.** For those spells, the
   comparison that gates now has to run in the mission whose units have
   them. The six `lab-<mission>-*` scenarios in phase 3 do that.

## Research and upgrades

L0 compares 11 weapon and armour lines and differs on every one, in a way
no table edit fixes: the engine has one generic weapons line and one armour
line per race, two levels, a flat price and a fixed time. The ROM has one
line per family, three levels, and a base cost plus a factor per level.
The Protoss lines and Singularity Charge are compared and equal. 34 other
upgrades have no engine row.

| Race | Upgrades | N64 | GBA | Proposed |
| --- | --- | --- | --- | --- |
| Terran | Infantry Weapons, Infantry Armor, Vehicle Weapons, Vehicle Plating, Ship Weapons, Ship Plating | three levels per line | one generic weapons line and one generic armour line, two levels, flat 100/100 | `research-terran` (cost and time per level; damage change on the arsenal target) |
| | U-238 Shells, Ion Thrusters, Charon Boosters, Titan, Moebius, Apollo, Colossus, Caduceus Reactors, Ocular Implants | one level | Colossus Reactor (Battlecruiser energy) only | `research-terran` (cost, time, effect probe) |
| | Stim Packs, Lockdown, EMP, Spider Mines, Siege Mode, Irradiate, Yamato, Cloaking Field, Personnel Cloaking, Restoration, Optical Flare | research | Yamato research only | `research-terran` |
| Zerg | Melee, Missile, Flyer Attacks, Carapace, Flyer Carapace | three levels per line | generic line, two levels | `research-zerg` |
| | Metabolic Boost, Adrenal Glands, Muscular Augments, Grooved Spines, Pneumatized Carapace, Antennae, Ventral Sacs, Anabolic Synthesis, Chitinous Plating, Gamete Meiosis, Metasynaptic Node | one level | Ventral Sacs only | `research-zerg`; P1 opening (the N64 computer upgrades Pneumatized Carapace in the first minutes) |
| | Burrowing, Spawn Broodlings, Plague, Consume, Ensnare, Lurker Aspect | research | no | `research-zerg`; T3 and P1 openings (the N64 computers research Burrowing in the first minutes) |
| Protoss | Ground Weapons, Ground Armor, Air Weapons, Air Armor, Plasma Shields | three levels per line | yes in P10 custom games (Protoss human only); generic line elsewhere | `research-protoss` |
| | Singularity Charge | one level | yes in P10 custom games | `research-protoss` |
| | Leg Enhancements, Scarab Damage, Reaver Capacity, Gravitic Drive, Sensor Array, Gravitic Boosters, Khaydarin Amulet, Apial Sensors, Gravitic Thrusters, Carrier Capacity, Khaydarin Core, Argus Jewel, Argus Talisman | one level | no | `research-protoss` |
| | Psionic Storm, Hallucination, Recall, Stasis Field, Disruption Web, Mind Control, Maelstrom | research | no | `research-protoss` |

No computer player on the GBA ever researches or upgrades, while the N64
computers do so in the mission openings. The `op-*` scenarios gate that
directly, through each computer's first research.

## Missions

Every mission already has an exact L0 launch census (CHK placements against
the GBA world at launch, all 30) and an N64 launch capture (field map: 27
exact against the CHK; T10, Z5 and Z10 differ by a few unexplained
positions, and Z5 also lacks its Data Disk row). The player tech and upgrade arrays equal the ROM's in all 30
launches (N64 RAM against the CHK's effective settings). Apart from the
T1 movement checks and the unit labs, no mission has a gating check of what
happens after the launch, and no passive opening gates.

| Mission | Name | Human starts with | Today after launch | Labs hosted | Proposed |
| --- | --- | --- | --- | --- | --- |
| T1 | Wasteland | Marines, SCVs, Command Center | `t1-move`, `t1-fields` | Marine move, SCV move, Marine on Zergling | `op-t1` |
| T2 | Backwater Station | Marines, SCV, Barracks | — | | `op-t2` |
| T3 | Desperate Alliance | Marines, Vultures, SCVs, base with Bunkers | `t3-opening` (report only) | Vulture move | `op-t3` (makes `t3-opening` gate) |
| T4 | The Jacobs Installation | Marines, Firebats | — | | `op-t4` |
| T5 | Revolution | Marines | — | | `op-t5` |
| T6 | Norad II | Marines, Firebat, SCVs | — | | `op-t6` |
| T7 | The Trump Card | base to Science Facility, Science Vessel, Vultures | — | | `op-t7` |
| T8 | The Big Push | Ghosts, Wraiths, Dropships, base | — | | `op-t8` |
| T9 | New Gettysburg | Marines, Vultures, Dropship, base | — | | `op-t9` |
| T10 | The Hammer Falls | Siege Tanks, Marines, Firebats, Vulture | — | Siege Tank move | `op-t10` |
| Z1 | Among the Ruins | Zerglings, Drones, Hatchery | — | | `op-z1` |
| Z2 | Egression | Hydralisks, Mutalisks, Hatcheries | timeline report (no golden) | Hydralisk move, Mutalisk move, Hydralisk on Zealot | `op-z2` |
| Z3 | The New Dominion | Hydralisks, Mutalisks, base | — | | `op-z3` |
| Z4 | Agent of the Swarm | Queens, Hydralisks, Zerglings, base | — | | `op-z4` |
| Z5 | The Amerigo | Infested Kerrigan, Zerglings | — | | `op-z5` |
| Z6 | The Dark Templar | Guardians, Hydralisks, Mutalisks, base | — | | `op-z6` |
| Z7 | The Culling | Hydralisks, Mutalisks, Drones | — | | `op-z7` |
| Z8 | Eye for an Eye | Ultralisks, Hydralisks, Nydus Canals | — | | `op-z8` |
| Z9 | The Invasion of Aiur | Hydralisks, Zerglings, base | — | | `op-z9` |
| Z10 | Full Circle | Ultralisk, Hydralisks, Zerglings, Hatchery | — | | `op-z10` |
| P1 | First Strike | Zealots, Dragoons | timeline report (no golden) | | `op-p1` |
| P2 | Into the Flames | Zealots, Dragoons, Nexus | — | | `op-p2` |
| P3 | Higher Ground | Scouts, Zealots, Dragoons, Nexus | — | | `op-p3` |
| P4 | The Hunt for Tassadar | High Templar, Zealots, Dragoons | — | | `op-p4` |
| P5 | Choosing Sides | Shuttle, Zealots, Dragoons, Nexus | — | | `op-p5` |
| P6 | Into the Darkness | Zealots | — | | `op-p6` |
| P7 | Homeland | base to Stargate, Scout, Observer | — | | `op-p7` |
| P8 | The Trial of Tassadar | Carrier, Dragoons, Nexus | — | | `op-p8` |
| P9 | Shadow Hunters | Arbiter, Zealots, Dragoons | — | | `op-p9` |
| P10 | Eye of the Storm | Hero Dark Templar, Nexus, Photon Cannons | — | | `op-p10` |

## Proposed scenarios

The design rests on one observation: a mission start hands the human
player only 26 of the 40 trainable unit types (none of the Brood War
units), and rarely next to an enemy alone enough for the attack labs'
single-attacker rule (a private scan of the CHK placements). Building the tech
tree in Blood Bath gives every type, in one place, with a measured train
time on the way. So the bulk of phases 2 and 3 is four chained segments per
race from one Blood Bath start, and phase 4 is one passive opening per
mission.

### Spike first: production input (no golden)

One exploration capture, as S5–S8 were, before any phase 2 recording:

| Question | Pass |
| --- | --- |
| Build, train, add-on, morph, research and cast through the N64 command wheel | Each is confirmed by its own evidence in a recorded run (order byte; queue, build or research timer; bank debit, which for a building comes when the worker places it, not at the command; energy or effect for a cast); `tools/sc64_tape_compile.py` gets `build`, `train`, `research`, `cast` ops with the same closed-loop checks as `order` |
| Attack on an own unit (force attack) | B over another own unit gives an attack order on it, and hits land; if not, the arsenals fall back to enemy foes (below) |
| Bank edit | `poke` on the human's minerals and gas (bank block `0x800B1D44`) holds, and the decoder attributes later drops to spending; promotes `banks.gas` |
| Computer opponent | Which lobby keeps the computer out of the human's base for the length of a segment (its first wave in the `sk-*` goldens; else the computer's bank set to zero by `poke` each segment, mirrored by the GBA runner) |
| Fields | `energy` (+0x9E) promoted; per-unit spell timers (Stim, Lockdown, Irradiate, Plague, Ensnare, Stasis, Maelstrom, Defensive Matrix, Parasite, cloak, burrow) located in the row with controlled changes |

### Phase 2: production and arsenal (6 scenarios, 12 captures)

| Id | Start | Tape | Measures (both engines) |
| --- | --- | --- | --- |
| `tech-terran`, `tech-zerg`, `tech-protoss` | Blood Bath 1v1, human of that race, bank edited (n64-staged) | Builds every building, add-on and morph once in dependency order, does the research a product needs first (Lurker Aspect before the Lurker; Zerg: Infestation of the Terran computer's Command Center, brought below half hit points first, before the Infested Terran), and trains or morphs every unit once, with the quantities a product consumes (two High Templar for an Archon, two Dark Templar for a Dark Archon, a Mutalisk per Guardian or Devourer, a Hydralisk per Lurker); Siege mode; loads and unloads one transport of each race; lifts and lands one Terran building | `train.duration`, `build.duration` and `morph.duration` per type (±2 frames); cost (the bank debit, exact: at the queue for units, at placement for buildings, with command-to-placement latency as its own check); supply used and provided (exact); Larva spawn interval; Interceptor and Scarab build; mode change, load, unload and lift-off times; transport capacity |
| `arsenal-terran`, `arsenal-zerg`, `arsenal-protoss` | The `tech-*` end state (state file) | Each combat unit, and each attacking building (loaded Bunker, Missile Turret, Sunken, Spore, Photon Cannon), hits one target of each size class it can hit (small, medium, large; ground and air; shields), one attacker at a time. Ordered unit weapons hit own targets where the race has the class; automatic weapons (loaded Bunker, Spider Mine, turrets, colonies, Photon Cannon) and the missing classes (Terran flyers are all large) use the lobby computer's units, picked by race for the classes needed | The attack-lab checks (damage per hit on hit points and on shields separately, cooldown, range at first hit, first hit) against every size and damage type; secondary targets by weapon behaviour from the ROM tables: splash (Siege Tank, Firebat, Lurker, Reaver, Archon, Valkyrie, Corsair, Infested Terran, Spider Mine), Mutalisk bounce, the Devourer's acid spores as a debuff checked apart from damage; Spider Mine trigger range; Interceptor launch |

GBA side: a new `tech` runner scenario that replays the N64's commands
through the injector (`sc64_pi_build` and `sc64_pi_train` at the N64
command frames and halved sites) with the same bank edit. Two pieces are
new test code, not game code. First, `sc64_pi_attack` today attacks only a
hostile target (its fallback is an attack-move), so the arsenal needs a
test-only own-target attack path for ordered unit weapons (GBA Bunkers
and Spider Mines reject same-owner targets, so those only fire on the
computer's units). Second, attacking buildings are observed firing on
their own, not ordered. The attack measurement then accepts a human-owned
foe named by its row.

Targets and single-use attackers. Each arsenal names a target setup per
weapon. Where a 1v1 lobby lacks one (shields for Zerg weapons in the Zerg
against Terran lobby that Infestation needs), a short branch from a
lobby against the missing race supplies it. Scourge, Infested Terran and
Spider Mine die on impact. For them the checks are impact damage, trigger
or acquisition range and time to impact, not a cooldown.

A type the engine cannot build is reported absent per check, never
skipped, and must not stop the chain. Today `sc64_pi_train` and
`sc64_pi_build` count a refusal as a failure and the runner exits nonzero.
The `tech` runner instead does three things:

- It checks absence per profile before the run: no identity, or no
  production path (the Guardian morph exists only in host tests). Such a
  type counts as a verified absence, the actions that depend on it read
  absent too, and the independent ones go on.
- A supported command waits until its GBA prerequisites are ready, never
  earlier than its N64 frame. The wait is reported as its own delay, and
  each duration is still timed from the command's actual start.
- Any other refusal stays an error.

If the spike finds no force attack on own units, the arsenals become enemy
labs instead, chosen from the enemies the private CHK scan found alone
at launch, plus a Blood Bath lobby against a computer of the target race for the Brood War
units. That set is larger (about 20 labs) and covers fewer size classes.

### Phase 3: spells and upgrades (12 scenarios, 24 captures)

| Id | Start | Tape | Measures |
| --- | --- | --- | --- |
| `research-terran`, `research-zerg`, `research-protoss` | The `tech-*` end state | Researches every tech and every upgrade level that `tech-*` did not already buy (a first purchase is measured in the segment that makes it), then a short probe after each effect: one hit on the arsenal target after each weapon and armour level, one move leg after each speed upgrade, one attack at range after each range upgrade, energy after each energy upgrade, capacity after Reaver and Carrier capacity | `research.duration`, `research.cost`, `upgrade.duration` and `upgrade.cost` per level (exact cost, ±2 frames); the effect as the change of the probe's own check |
| `spell-terran`, `spell-zerg`, `spell-protoss` | The `research-*` end state | Each caster casts each spell once on own targets at known distances (Mind Control, Parasite and the EMP shield drain on a computer unit of the lobby's other race) | energy cost (exact), effect duration (±2 frames), damage per tick or total, radius as the farthest target affected, side effects (Consume energy gain, Recall arrival, Broodling count, Nuke delay) |

Mission labs for the abilities the GBA implements only in campaign
contexts. Each one gates now, where the Blood Bath segments read absent on
the GBA:

| Id | Start | Measures |
| --- | --- | --- |
| `lab-p4-storm` | P4 after its transmissions (n64-staged entry) | the human High Templar's Psionic Storm: energy cost, pulse damage and count, radius by targets at known distances |
| `lab-z5-kerrigan` | Z5 | Infested Kerrigan's Personnel Cloaking (activation cost, drain per frame) and Consume (energy gained, range) |
| `lab-z6-storm` | Z6 | Kerrigan's Psionic Storm, as in P4 |
| `lab-z8-nydus` | Z8 | transit time through a placed Nydus pair, group transit order |
| `lab-t5-mines` | T5 | a Marine walking into the computer's placed Spider Mines: trigger range, arming, damage and splash on the subject |
| `lab-t10-repair` | T10 | an SCV repairing a Siege Tank whose hit points were set by a guest edit on the N64 and by the runner on the GBA (the same value): hit points per frame and bank cost |

Lobby races for the cross-race spells: Terran against Protoss (EMP on
shields), Protoss against Zerg (Mind Control, Maelstrom, Feedback), Zerg
against Terran (Parasite, Spawn Broodlings, Infestation needs a damaged
Command Center: the human's own Terran target is impossible, so the
computer's).

### Phase 4: mission openings (30 scenarios, 60 captures)

`op-<mission>` for every mission. `op-t3` replaces `t3-opening` with a
fresh pair. `t3-opening` is marked `tick_skips_ok` (T3 loses some
follow-up reads), and `tools/parity_decode.py` refuses gating observations
for such a scenario. Busy missions therefore first need a sampling rule
that gates with skipped ticks, or a tape that taps them reliably,
qualified on one busy opening before the matrix. Each opening is the
mission launch tape, text boxes dismissed with `pulse`, no other input,
for the first 5,400 source frames (about five minutes at selector 4).
Measured on both engines with one shared measurement (the T3 timeline code,
generalised):

- the human's bank and gathered minerals at 1-minute checkpoints, first
  delivery;
- each computer's first new unit and building per type, its first spending
  and first research. Production is counted per actor lifetime (a birth or
  morph of a row), not as a count rising above its launch total. That is
  the rule of today's `first_rises`, which misses a replacement for a lost
  unit. Scripted spawns and ownership changes are told apart from
  production;
- first contact (first enemy row within sight of a human row), first damage
  to a human unit, first attack wave (three weapon units within 12 tiles of
  the human's start), and the human's losses by checkpoint;
- trigger effects with a time: text boxes (pause halfword), reinforcements
  (human-owned births with trigger evidence: not Larvae, not production,
  ambiguous births left unclassified). Victory and defeat stay out of the
  gating checks until a campaign result watch is qualified on one known win
  and one known loss: the result word proven today is the skirmish one, and
  it is read through `watch`, outside `tap.bin`.

GBA side: the census runner per mission profile. The field-map audit lists
the trace gaps that make today's T3/Z2/P1 comparison partial (raw unit
types in the Zerg and Protoss traces, computer wallets, projectiles); those
are emitter work in `tests/sc64_parity_trace.h`, not game code. A mission
whose GBA computer is inert reads REGRESSION and is listed as a defect, as
the field-map audit already does for T3, Z2 and P1.

### What the proposal covers

| Item kind | Total | Dynamic today | After phases 2–4 |
| --- | ---: | ---: | ---: |
| Trainable units (three races) | 40 | 14 with a measured event; 4 in an attack lab; the rest only skirmish NEVER lines | 40: train time, every weapon |
| Buildings, add-ons, building morphs | 51 | 18 with a measured event (15 of them AI timing only); the rest only skirmish NEVER lines | 51: build time |
| Spells and abilities | 44 | 0 | 44 (17 exist on the GBA in some form; the rest read absent) |
| Researches (spell and ability techs) | 24 | 1 (report only) | 24 |
| Upgrades (each line once) | 50 | 0 | 50 (typed lines exist on the GBA only for Protoss in P10 custom games; the generic two-level line elsewhere) |
| Campaign missions after launch | 30 | 4 host a lab (T1, T3, T10, Z2); no gating opening | 30 openings |

Captures: 1 spike + 96 golden captures, at most two at a time. At the
measured 135 steps/s (S3) and about 3.4 steps per tick, a five-minute
opening is about 2.5 minutes of capture and a tech segment about 8; with two
lanes, phase 4 is about 1.3 hours of capture and phases 2–3 about 1.5 hours.

`./dev parity` cost: each new scenario adds one GBA host run, or three if
it has to run at three frame offsets because it turns out as noisy as the
skirmish lobbies. The opening runs are about as long as one `sk-*` match;
the estimate is +15–25 s on the current ~75 s with the worker pool, to be
measured and reported per phase.

### Not proposed

- Played campaign wins (the design's T1, Z8, P8 wins): one run is a
  witness, not a band, and the controllers already gate them.
- Episodes IV–VI: the harness enters only the 30 Episode I–III missions.
- Per-hero labs: heroes share weapon classes with the arsenals and their
  values with L0.
- More skirmish seeds: the 17 lobbies already bound the AI's opening; the
  gaps they show are early production and mining, which the `tech-*`
  segments now measure directly.

## Questions for the parent

1. Is the bank edit (n64-staged) acceptable for the `tech-*` chains? It is a
   guest edit of the same kind as the progress byte. Without it the human
   must first mine every cost, which multiplies each run's length and puts
   the computer's attack waves inside it.
2. If the spike finds no force attack on own units, should phase 2 use the
   enemy-foe fallback (about 20 mission labs), or the computer of a
   Blood Bath lobby as the target?
3. Five minutes per opening, or a mission-specific horizon (the first
   attack wave, where it comes later)?
4. Record the N64 side of every spell and upgrade now, although most read
   absent on the GBA and would enter the baseline as KNOWN defects (the
   recommendation: one capture covers both, and the golden is ready when
   the engine gains the feature), or only the 17 abilities and the upgrade
   lines the GBA has?
