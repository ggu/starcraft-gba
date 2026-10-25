# Unit specification audit — 25 October 2026

Historical audit of the revisions listed below. See the subsequent
[stock profile and sprite fixes](../2026-10-26-stock-profiles-and-art/README.md)
for implemented corrections and remaining behavior limits.

The engine does **not** yet match stock StarCraft unit behavior. Most default
HP, shields, armor and primary weapon numbers match the public reference tables.
There are definite numeric discrepancies and broader movement, damage and
weapon-lifecycle simplifications. A matching table entry is not a fidelity pass.

This is an audit, with no gameplay, balancing, asset or ROM changes.

## Scope and evidence

The asset-free C probes evaluated the actual `race_rules.h` helpers for eight
profiles, producing **319 profile/unit records, 66 enum names and 55 distinct raw
unit IDs**. These include ordinary units, heroes, critters and scenario props.
They are compile-time records, not a claim that every actor is playable in every
mission. Buildings, every spell and units absent from these profiles are outside
the exhaustive table census.

- Published default source branch: `75979a6aa850af2d093158193cab1cc5a6ff89a6`.
  Profiles: `TERRAN_FULL_FLAGS`, `Z3_FLAGS`, `P3_FLAGS`.
- Staged development tree: `fad79261ead702086631b3db1f6e2bacbcde22f2`.
  Profiles: `TERRAN_FULL_FLAGS`, `Z6_PROBE_FLAGS`, `Z7_SCOURGE_PROBE_FLAGS`,
  `P6_PROBE_FLAGS`.
- P7 controller branch: `2a6b399e5e7fa30040caf7814742edb044c8dde5`.
  Profile: `P7_CAMPAIGN_COMPOSITION_FLAGS`.

The reference is the clean, pinned BWAPI revision
`d727fed68558c506163048ea889131d8cbb33915`:
[UnitType.cpp](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/UnitType.cpp),
[WeaponType.cpp](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/BWAPILIB/Source/WeaponType.cpp),
[UnitType.h](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/UnitType.h),
[WeaponType.h](https://github.com/bwapi/bwapi/blob/d727fed68558c506163048ea889131d8cbb33915/bwapi/include/BWAPI/WeaponType.h).
Behavioral comparisons also consulted the pinned
[OpenBW bwgame.h](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h)
for cooldown modifiers, movement fields and fractional HP; no implementation was
copied from that reference. These describe stock **PC** behavior. Native Nintendo 64 table and scheduler
equivalence has not been established. Source scenario overrides can also change
defaults. No N64 binaries or assets are included in these artifacts.

The runtime source review uses `fad7926`, particularly `starcraft/game.c`,
`race_rules.h`, `race_combat.h`, the T5 actor adapter and P3/P6/P7 adapters.
PR5 head `d085310` changes HUD/render code, with no changes to the audited
movement/combat helpers. The later P7 controller commit adds qualification work,
not different common unit stats. No emulator or hardware timing was measured
for this audit. The 30 Hz clock is an authored pacing target, not a measured or
guaranteed rate.

## Default durability

| Field | Matching canonical profile/unit records | Qualification |
|---|---:|---|
| Maximum HP | 314/314 | Default initialization, before scenario HP percentages |
| Maximum shields | 314/314 | Default totals only; regeneration and shield armor are separate mechanics |
| Base armor | 308/314 | Six repeated records are the same legacy Raynor Marine compatibility identity, with 0 instead of 3 armor |

Five additional records have a noncanonical or disabled raw-ID round trip and
are retained in the comparison rather than counted as canonical matches. Two are the
legacy Terran `U_RAYNOR` alias; three are profile-disabled Dropship import
mappings. A round trip alone is not proof of actual spawn identity: earlier
campaign loaders can select the legacy Raynor alias directly.

The legacy Raynor record needs explicit cleanup. Non-T4 profiles map it as raw20
(Raynor Marine) with 200 HP, 0 armor and no weapon. Early Terran compatibility
loading also uses `U_RAYNOR` for raw19 (Raynor Vulture), retaining 200 HP/0 armor/
no weapon instead of the stock Vulture hero's 300 HP/3 armor/30 damage. Active
T5 uses a separate `U_RAYNOR_VULTURE` with the correct default durability and
weapon; expanded Z/P profiles use `U_ZP_RAYNOR_VULTURE`. This is an old adapter
approximation, not a reason to change every Raynor identity together.

## Confirmed combat discrepancies

Damage below is base damage before shields, armor, size and upgrades. Cooldowns
and ranges in this table use the source table's frame/pixel units.

| Unit / property | Engine | Pinned stock reference | Effect |
|---|---:|---:|---|
| Siege Tank, siege damage | 50 | 70 | Lower primary damage |
| Goliath, air attack potential | 10 once | 10 × weapon factor 2 | Missing missile multiplier |
| Scout, air attack potential | 14 once | 14 × weapon factor 2 | Missing missile multiplier |
| Drone, cooldown | 15 | 22 | Faster base attack; converted engine interval 19 vs 28 updates |
| Probe, cooldown | 15 | 22 | Same issue |
| Fenix Zealot, cooldown | 15 | 22 | Same issue; its 20 × 2 blade damage does match |
| Hunter Killer, ground/air range | 128 | 160 | Shorter reach |
| Drone / Probe, range | 15 | 32 | Shorter reach |
| SCV, range | 15 | 10 | Longer reach |
| Siege Tank, minimum range | No generic check | 64 | Generic firing path can attack inside the stock dead zone |

Goliath/Scout have `maxAirHits=1` in UnitType and `damageFactor=2` in their weapon
tables. Those are distinct fields. The engine ignores the factor; this audit
does not infer exact per-missile armor resolution from the summary multiplier.

Firebat needs mechanic work rather than a blind “multiply by three” fix. Stock
UnitType reports a **maximum** of three ground hits. The engine applies one
primary 8-damage hit and an authored radial 3-HP splash when a unit-target hit
reaches HP. Splash affects alive, unloaded other-owner units within 12 runtime
pixels, including the surviving primary unit, and bypasses the usual shield/armor
path. Outside later mission-specific guards it can also affect non-hostile owner
slots. Building-target hits do not enter this splash loop. That does not reproduce flame geometry or establish a
guaranteed 24 damage to every target.

## Speed, cadence and scaling

The common movement kernel gives most movable actors **one pixel per update**.
Zerglings, Firebats, legacy Raynor, ordinary Vultures, the Z/P Raynor Vulture and
T5 Dropships get two. T5's distinct Raynor Vulture still gets one. Special actor
flags can prevent motion; mines and optional Interceptor projectiles have
separate lifecycle/motion handling.

At half spatial scale and an assumed stock 24-frame/second versus authored
30-update/second basis, expected speed is `topSpeed × 0.5 × 24/30`. This is a
normalization assumption, **not proof of native N64 speed or measured GBA time**.
BWAPI itself describes topSpeed as approximate for some units.

| Unit | Current cardinal pixels/update | Expected on that basis |
|---|---:|---:|
| Marine | 1 | 1.600 |
| Firebat | 2 | 1.600 |
| SCV / Drone / Probe | 1 | 1.968 |
| Zergling | 2 | 2.196 |
| Hydralisk | 1 | 1.464 |
| Dragoon / Scout | 1 | 2.000 |
| Mutalisk / Wraith / Queen / Scourge | 1 | 2.668 |
| Overlord | 1 | 0.332 |
| Vulture | 2 | 2.560 |
| Guardian / Battlecruiser | 1 | 1.000 |

Even the last row matches only the nominal cardinal rate. The common kernel
has no source acceleration, heading, turning or momentum. It steps X and Y
independently, allowing faster diagonal travel on clear paths. Collision,
pathfinding, upgrades and performance further affect observed travel times.

Ordinary attack cooldowns become `ceil(sourceCooldown × 5/4)` updates and
ranges become `ceil(sourceRange / 2)`, with an authored +18 pixels for building
targets. These conversions explain many differences in numeric units. They do
not explain the wrong source-basis values above. Common firing uses a fixed
cooldown without full attack-animation timing, source cooldown variation or
generic Stim/attack-speed upgrade effects. Simulation updates are tied to the
render loop; failing the performance budget also slows real elapsed gameplay.

## Broader behavior that table matches do not cover

- Normal ranged attacks apply damage immediately when creating their visual
  tracer. This common path has no general projectile travel/impact simulation.
  Optional P7 moving projectiles are a separate system.
- Most weapons do not use the complete damage-type × target-size matrix.
  T4 Ghost/T5 hero and mine helpers cover narrow cases. Tank/Hydralisk/Scout
  explosive and Vulture/Firebat concussive matchups therefore diverge even when
  primary damage matches. Integer HP also differs from source fractional HP.
- Generic player weapon research adds +1 per level to each hit regardless of
  source weapon upgrade family/bonus. Tank expects +3, siege cannon +5, and
  many other weapons differ. T5 hero helpers and Scourge have explicit exceptions;
  specialized adapters must be reviewed separately. Armor research is similarly
  simplified, and complete shield armor/status/upgrade behavior is not present.
- Siege splash and Mutalisk bounce are not provided by the generic attack path.
  Reaver direct 100 damage is an adaptation of a Scarab attack, with no generic
  stock ammo/travel/splash lifecycle. Spider Mines have a special T5 handler, so
  their generic zero weapon is not evidence of zero detonation damage.
- Scourge is working through the P3 bypass in P3/P6/P7 and through a distinct
  Z7 combat gate. It remains unarmed in the audited Z3/Terran profiles. Reading
  `can_attack_air()` alone would incorrectly label P3/P6/P7 Scourge unarmed.
- Zero direct Carrier damage matches its source weapon table but says nothing
  about its child Interceptors. P7 Archon splash/Observer/Battery/Arbiter features
  are gated adapters; stale capability comments are not sufficient evidence
  that a feature is absent or complete.
- Import records retain placement HP percent but drop generic UNIT shield and
  energy percentages. Generic UNIS/custom-unit-stat application was not found.
  Actual per-map impact was not qualified here; this is a support limitation,
  not proof that a particular mission starts units with the wrong values.

## Artifacts and reproduction

- [Per-profile, per-unit comparison](unit-specs.json): every probed actor,
  durability, raw identity, ground/air weapon values, hit counts, cooldowns,
  ranges, nominal movement and rule cost/time/supply values.
- [Manifest](unit-specs-manifest.json): source commits, full profile define sets,
  reference-table hashes, counts and comparison digest.

Comparison fields named `generic_*` or `shared_motion_*` describe that reviewed common
path. `engine_class_eligible` is weapon-class eligibility, including the reviewed
Scourge exception; actual target eligibility also depends on life, ownership,
loading, immunity, visibility and mission adapters. Special adapters and ineligible
target classes must not be interpreted as ordinary attacks. Cost/time/supply columns are retained for review, without
automatically treating hero/mode-switch or Zerg paired-production policies as
incorrect stock prices. Neither a `base_table_match_only` result nor a matching
HP total qualifies all abilities, collision, target acquisition or mission AI.

The JSON stores a `columns` array and a `rows` array in the same column order.
CSV is an optional local export for spreadsheets; it is not added to the public
source index.

The public helpers need Python 3.9+, Make and Clang, with no game assets:

```sh
python3 tools/sc64_unit_spec_reference.py --reference /path/to/clean/pinned/bwapi --output .local/unit-specs/reference.json
python3 tools/sc64_unit_spec_engine.py /path/to/engine TERRAN_FULL_FLAGS .local/unit-specs/probes/terran
python3 tools/sc64_unit_spec_engine.py /path/to/engine Z3_FLAGS .local/unit-specs/probes/zerg
python3 tools/sc64_unit_spec_engine.py /path/to/engine P3_FLAGS .local/unit-specs/probes/protoss
python3 tools/sc64_unit_spec_compare.py --reference-json .local/unit-specs/reference.json --probes-dir .local/unit-specs/probes --output .local/unit-specs/report --csv-output .local/unit-specs/unit-specs.csv
```

For the exact dated matrix, use the manifest's engine commits and profile
variables. The P7 profile is defined on its controller branch. Optional extra
defines can be passed as `--extra=-DSC64_EXAMPLE` to the engine probe. Runtime
exception logic in the comparison tool is specific to this reviewed source and
must be rechecked for newer revisions. The tool does not claim automatic
discovery of all runtime behavior.

Validation performed: extracted 234 reference unit rows and 132 weapon rows;
compiled and ran all eight C rule probes; checked raw-ID round trips; reviewed
constructors, attack selection, damage, movement and profile exceptions; and
independently checked the highlighted discrepancies. Full constructor execution,
all spell mechanics, native N64 defaults and hardware wall-clock parity remain
unqualified. No gameplay fix is included in this audit.

Recommended fix order: correct the definite primary numeric errors and legacy
identity boundaries; introduce per-unit fixed-point motion with a declared clock
basis; then qualify damage classes/upgrades and special weapon lifecycles with
deterministic combat fixtures and hardware timing checks.
