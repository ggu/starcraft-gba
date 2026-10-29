# Human source sight in the Terran runtime

Since `2abc3d5` ("Respect human fog when automatically acquiring attack
targets"), a human actor only acquires a new automatic target in a currently
visible cell. In the Terran runtime, most human actors had no source sight
fact. They still used the engine's approximate radii: 72 engine pixels for
units, 56 for workers and 64 for buildings other than town halls. Several
weapons reach farther than that sight. Examples are Goliath and Kerrigan
ground range (96), Wraith and Vulture (80), and the Missile Turret (112
against its 64 sight). These actors stopped firing at enemies inside their
weapon range that the source game would see. As a result, the full Terran T5,
T7 and T8 controllers lost or stalled.

The Terran runtime (`SC64_T4_RUNTIME`) now uses the pinned BWAPI
`sightRangeTiles` value for every actor that has a public raw UnitType ID. It
uses this value whenever no more specific T4, late-Terran or Z/P fact exists.
The 228-byte table `starcraft/sc64_source_sight_tiles.h` holds numeric facts
only, from BWAPI `d727fed68558c506163048ea889131d8cbb33915`. Existing
specific facts keep priority. The T4 special-actor values equal the table
entries. As in BW, an
unfinished grounded building sees four tiles. The Terran runtime has no
in-place building morphs. The visibility snapshot stores that state as
`-(type+1)`, as Z7 already does, so removal uses the radius that was added.

Most source radii (4–11 tiles, or 64–176 engine pixels) use the existing exact
row-span tables and symmetric-difference transitions from the Z7 sight-span
work. Radii without a table (1–3 tiles: Spider Mine, Nuke, doors and wall
traps) use the existing exact disk path. The Zerg Z6, Protoss P7 and Showcase
products do not compile the T4 runtime and do not change. Z7 span builds keep
the same logic; only the guard macro is renamed.

## Validation

- The private repro (an idle Goliath with a passive enemy Marine 90 engine
  pixels away, on the T8 world) now attacks the Marine (hp 40→16) without
  diagnostic flags. At 60 pixels it attacks as before. At 120 pixels, outside
  Goliath range, it does not attack.
- Private full Terran controllers, with traces kept in a private worktree:
  T5 PASS (won at tick 114,497), T7 won at tick 39,012 and T8 won at tick
  90,710, with zero faults. These are the same T7/T8 win ticks as the
  diagnostic runs with the predicate disabled or with source sight. On main
  `9ff7f7c`, T5 loses at tick 111,945, T7 has no win by tick 60,000 and T8
  has no win by tick 200,000.
- `make -k test-sc64-terran-full-campaign`, with traces in the private
  worktree: 234 skirmish startup/save configurations, profile startup/save,
  and the T3/T4/T5/late save and resume checks pass. T1, T2, T3, T4, T5, T6
  and T10 win. T4 now wins at frame 8,911, against 8,401 on main. T9's
  500,000-tick controller trace is byte-identical to main. It stops on
  economy without a win, which is the same result as on main.
- Default `./dev build` passes. `make -k test` passes except
  `test_sc64_photon_cannon_renderer` (Protoss roster). That check also fails
  on main in this private layout, which lacks `sc64_fact_tables_generated.h`.
  `test-sc64-terran-full-targets` fails at `tests/sc64_targets.c:118`
  (an owner-4 query under full-Terran flags). It fails the same way on main.
- The asset-free incremental-visibility fixture adds source radii with and
  without span tables (128 and 48 engine pixels). It also covers a Turret and
  a Command Center during construction, completion and removal, plus random
  construction-state toggles. All visibility, explored, source-count and
  minimap state still matches full recomputation in every variant.
- `tests/sc64_t4_actor_rules.c` checks representative BWAPI values and that
  each armed Terran unit's sight covers its ground and air weapon ranges.
- Terran ARM link: BSS end is unchanged at `0x020370c0`. IWRAM end moves
  from `0x03004e38` to `0x03004ea8` (+112 bytes, because the span branch is
  inlined into `sc64_update_exploration`). This is within the `0x03005000`
  guard, with 344 bytes free. ROM text grows by 2,308 bytes.

## Cost

Larger sight circles cost more visibility work per moving human unit. Host
counts from the same controller runs (O2, no sanitizers; same inputs) give
the following per-tick averages:

| Run (ticks) | Variant | Cell updates | Disk cells | Span rows | Sight lookups |
| --- | --- | ---: | ---: | ---: | ---: |
| T5 (114,497) | main | 10.1 | 0.0 | 135.0 | 6.8 |
| | this change | 21.9 | 0 | 249.3 | 8.0 |
| T7 (39,012) | main | 132.7 | 270.0 | 104.8 | 7.0 |
| | this change | 18.0 | 0 | 182.5 | 6.2 |
| T8 (90,710) | main | 37.6 | 35.1 | 109.9 | 5.8 |
| | this change | 64.9 | 0 | 402.2 | 13.4 |

The runs on main play different games because their outcomes differ. On main,
the existing late-Terran source-sight actors use the brute-force disk path.
This change removes that path for them. Main's 56/72 span rows run from IWRAM;
the source-radius span helpers run from ROM. The ARM cycle cost of this change
was not measured. The mGBA debugger port was in use by another process during
this work. Measuring `sc64_sight_cycles` on a normal-menu replay remains
open.

## Limits

This uses the engine's existing half-scale, pixel-centered sight circles; it
does not implement BW's tile sight masks, sight upgrades, or ground-height
line of sight. It covers only the Terran product. The Zerg Z6 and Protoss P7
products have the same human acquisition gate. Their generic actors without
Z/P facts still use the engine radii. No assets, generated data, ROMs, saves
or traces are tracked.
