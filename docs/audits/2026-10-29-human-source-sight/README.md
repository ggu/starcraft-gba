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
row-span tables from the Z7 sight-span work. A moving unit's footprint change
runs in the same IWRAM symmetric-difference loop as the 56/72 radii, with the
radius's halfwidth row and extent passed in; additions and removals use the
Z7 row spans. Radii without a table (1–3 tiles: Spider Mine, Nuke, doors and
wall traps) use the existing exact disk path. The Zerg Z6, Protoss P7 and Showcase
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
- Moving the source-radius transition into the shared IWRAM loop changes no
  footprint. With it, the host T5, T7, T8, T9 and T10 controller traces are
  byte-identical to those before it. All T4 span variants of the visibility
  fixture and both Z7 sight-span tests pass. On ARM, every probe window's
  gameplay and fog digests match the earlier revision (see below).
- `tests/sc64_t4_actor_rules.c` checks representative BWAPI values and that
  each armed Terran unit's sight covers its ground and air weapon ranges.
- Terran ARM link: BSS end is unchanged at `0x020370c0`. IWRAM end moves
  from `0x03004e38` to `0x03004ed8` (+160 bytes: the source branch of the
  span transition is inlined into `sc64_update_exploration`). This is within
  the `0x03005000` guard, with 296 bytes free. ROM text grows by 1,964 bytes.

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
This change removes that path for them.

### Paired normal-menu ARM measurement

Each workload is the private host controller tape for that mission, embedded
in a ROM built with the full Terran product command. The ROM starts at the
title screen, takes the normal campaign menu route, and feeds the tape one
held-key state per loop iteration. `arm-probe/terran_sight_probe.c` replaces
only `common/main.c`; it includes the tree's own `game.c`. Every game
function in each probe ELF has the same size as in that tree's product ELF,
and the IWRAM function set is identical. The only differences are `main`,
`gba_keys`, the probe's reporting helpers and the tape.

Each run is a fresh mGBA 0.10.5 Qt process with an isolated `HOME`, log mask
`7` and `videoSync=0`/`audioSync=0`. No GDB port is opened. All cycle counts
come from the emulated GBA timer (`gba_clock`, 16,777,216 Hz):
- *Update* is `game_update`.
- *Sight* is `sc64_sight_cycles` from that update, i.e. `update_exploration`.
- *Full* is update plus `audio_update` and `game_render`.

A tick is counted only when the screen is PLAY before and after it. Effective
fps is ticks × 16,777,216 / total full cycles. The runs are:
- *base* is `9ff7f7c`.
- *head* is `0ff389e`, this PR before the IWRAM transition.
- *opt* is this PR's final source.

Games diverge when the fog gate changes a decision, so the paired comparison
uses only the leading 500-tick windows whose gameplay digest matches between
base and the PR. That digest covers units, buildings, resources and frame,
and excludes the fog grids. Head and opt match each other in both the
gameplay digest and the full digest (which includes fog) in every window of
all three runs.

| Mission | Paired ticks | Variant | Mean update | Mean sight | Peak sight | Mean full | Sight share of full | Effective fps |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| T7 | 38,989 | base | 365,056 | 70,009 | 155,902 | 846,076 | 8.27% | 19.83 |
| | | head | 384,372 | 89,499 | 336,429 | 869,478 | 10.29% | 19.30 |
| | | opt | 346,243 | 50,803 | 234,643 | 823,047 | 6.17% | 20.38 |
| T8 | 33,487 | base | 362,564 | 37,395 | 390,846 | 810,225 | 4.62% | 20.71 |
| | | head | 346,467 | 21,090 | 295,105 | 794,885 | 2.65% | 21.11 |
| | | opt | 343,416 | 17,927 | 286,648 | 792,300 | 2.26% | 21.18 |
| T5 | 4,991 | base | 322,067 | 15,398 | 68,516 | 705,132 | 2.18% | 23.79 |
| | | head | 337,174 | 30,486 | 266,568 | 720,270 | 4.23% | 23.29 |
| | | opt | 325,360 | 18,691 | 185,093 | 710,422 | 2.63% | 23.62 |

Cycle columns are per tick. In T7 the gameplay state of base and the PR
stays identical for the whole compared span, so the sight difference is the
radius change alone. Head's ROM-resident Z7 transition cost 19.5k extra
cycles per tick there (−2.7% fps). The shared IWRAM transition removes that
and ends below main, because main's late-Terran actors used the brute-force
disk. T8 is cheaper than main at both PR revisions. T5's paired span is short
(4,991 ticks), and opt remains 3.3k sight cycles per tick (0.7% fps) above
main there.

Whole runs, which compare head and opt on the same game:
- T5 (114,797 ticks): mean sight drops from 79,885 to 44,073 cycles per tick,
  and effective fps rises from 20.33 to 21.24.
- T7 (39,312 ticks): mean sight drops from 91,141 to 51,584, and fps rises
  from 19.27 to 20.37.
- T8 (46,807 ticks): mean sight drops from 23,443 to 18,943, and fps rises
  from 20.84 to 20.93.

Peak full-tick cost comes from update work outside sight, at 8.3–9.0M
cycles. T5 also has a single-tick sight spike of 3.06M cycles near tick
9,000 at head and at opt. It happens when many units gain full footprints at
once, and most of it is per-cell `sc64_visibility_source_change`, which runs
from ROM on main too. Main's T5 run has a 1.24M spike near tick 8,000, but it
is already a different game by then, so the two spikes are not paired.

The ARM replays do not reproduce the host controller outcomes, at any of the
three revisions. T5 stays in PLAY without a win through tick 114,806. T7 has
no result at tick 39,323. T8 shows a loss at tick 46,821, and main's T8
shows a loss at tick 46,791. Up to that point the frame counts follow the
host traces. So these numbers measure normal-menu play of each mission's
opening and middle game, not a completed mission. The host/ARM outcome gap
exists on main too and is outside this change.

### Provenance

- Toolchain: `arm-none-eabi-gcc` (GCC) 16.2.0, GNU ld (GNU Binutils)
  2.47.20260726. Emulator: mGBA 0.10.5.
- The `make -n -B build/starcraft-terran.elf` command is byte-identical in
  the base and PR trees (SHA-256
  `9bea0e3e4c3d4da980cccb0e8aa9822115aedb87c6b18449da0bec6580c73de9`). In
  order:

  ```
  arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -g -std=c11 -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Icommon -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/backwater/generated -DSC64_AUDIO_IMPORTED -I.local/sc64/audio/generated '-DSC64_AUDIO_BLOB=".local/sc64/audio/generated/sc64_audio_generated.bin"' -DSC64_RACES -DSC64_FIELD_CONTROLS -I.local/sc64/briefings/generated -I.local/sc64/interface/generated -DSC64_CONSTRUCTION_SOURCE_ART -I.local/sc64/construction/generated -DSC64_UNIFIED -DSC64_IMPORTED_SKIRMISH -I.local/sc64/skirmish/generated -DSC64_LARGE_MAPS -DSC64_T4_ENABLED -DSC64_T4_RUNTIME -DSC64_ROUTE_PACK3 -DSC64_T4_WINDOWED_SCHEDULE -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_COUNT_CACHE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS -DSC64_UNARMED_QUERY_SKIP -DSC64_T3_ENABLED -DSC64_T4_CAMPAIGN_ENTRY -DSC64_T5_ENABLED -DSC64_T5_RUNTIME -DSC64_T5_CAMPAIGN_ENTRY -DSC64_T6_ENABLED -DSC64_T6_STORAGE -DSC64_T6_RUNTIME -DSC64_LATE_TERRAN_STORAGE -DSC64_TERRAN_LATE_ACTORS -I.local/sc64/terran-late/import-stage -I.local/sc64/races/generated -DSC64_TERRAN_LATE_ENABLED -DSC64_TERRAN_LATE_RUNTIME -DSC64_T6_CAMPAIGN_ENTRY -DSC64_TERRAN_FULL_CAMPAIGN_ENTRY -DGBA_SPRITE_WORDS -DGBA_SCENE_CLIP -DSC64_HUD_BUFFER_REUSE -DSC64_TERRAIN_CACHE_VRAM -DSC64_HUD_VRAM_BANDS -DSC64_TERRAIN_ROW_MASKS -DSC64_MINIMAP_RASTER -DGBA_WINDOW_BANDS -DSC64_STATIC_DIRTY_ROWS -DSC64_ROUTE_FLOOD_SPLIT -DSC64_UNIT_WORD_SPRITES -DSC64_CONTACT_CACHE -DSC64_MINIMAP_BLIT -I.local/sc64/facing/generated -DSC64_UNIT_FACING -DSC64_TANK_SOURCE_VFX -I.local/sc64/tank-vfx/generated -DSC64_WEAPON_SOURCE_VFX -I.local/sc64/weapon-vfx/generated -DSC64_UNIT_ATTACK_POSES_ENABLED -I.local/sc64/attack-poses/generated -DSC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL -DSC64_TRACE_DURATION_CACHE -DSC64_DIRECT_FIRING_POINTS -DSC64_DEATH_SOURCE_ART -I.local/sc64/death/generated -DSC64_ROUTE_MONOTONE_PROBE common/start.s common/main.c common/gba.c common/memory.c common/audio_bank.S starcraft/game.c starcraft/sc64_interceptor_motion.c starcraft/sc64_interceptor_pool_codec.c -mcpu=arm7tdmi -mthumb -mthumb-interwork -nostdlib -Tcommon/gba.ld -Wl,--gc-sections -Wl,-Map,build/starcraft-terran.map -lgcc -o build/starcraft-terran.elf
  ```

  `arm-probe/build_probe.py` makes three substitutions: it puts the probe
  source in place of `common/main.c`, drops `starcraft/game.c` (the probe
  includes it), and appends `-DTAPE_HEADER`. The map and output paths change.
  Rerunning the committed script reproduces the measured opt T8 ROM byte for
  byte.
- Compile inputs: every file read by the five compiled C units (from
  `gcc -M` with the same flags), plus the assembler sources, linker script
  and audio blob.
  - The 32 private inputs are identical in all three trees (aggregate SHA-256
    `08fa415cf9b9692042816a89d2998045f3675fc26a07b97ee62c511c38fa8e2a`).
  - Tracked inputs: base has 129 (aggregate
    `36b582c758e8726f0385432505da699c407f8e5d1929d797c4a11f428d968caf`).
    Head has 131 (`450c68c9fecf3b46cdf7ff65056511cab91c93cbfb9a0e61f03e0fcd4f63b69f`).
    Opt has 131 (`c86019c171d325dd21d1dbcabe58d62f322b856acac56ba96ce46220e4efb694`).
- Product ELF SHA-256:
  - Base: `85c57a4fc44bf65f37f778565438625c9bb14961f1922161b4e0c676099e7e1d`
  - Head: `e99387f73a887bf1e1a7a7d5e61adac2617ec8ab0f263278609851bd8a5a44f7`
  - Opt: `966d75b1e2e8ba5dc0268aab0456d2de63607a7baa76e7de56d9ac1c80d64f00`
    (ROM `a7d32eea96add268f4a26ed8972d1eb4ca1fad6315bf722c25a955ae79a7d7f6`)
- Probe source SHA-256:
  `f5e3529756cb2c6e4969ee085adbbd586c70171019533cf27d666b9aee190058`.
- Private traces (inputs to `arm-probe/make_tape.py`, not tracked):
  - T5: `46dc71da43e7b9ef9e7dc2197566b6155e4d6c30ec10491a7ee8b1520d85c787`
  - T7: `1e0cdd6bc253154c3df6f2a97f1e4a48000c8d534e047764dcebf1c9777b31e3`
  - T8: `eba761631c9c7e97c4cc3d6245b73235adfb2b9fc4b6a1d89c01282d72a3ce17`

  These are this change's host controller traces. With opt, the host
  controllers reproduce all three byte for byte.
- Probe ROM SHA-256 (base / head / opt):
  - T5: `6cfa3dce…` / `e7615c28…` / `7661371d…`
  - T7: `9992e41a…` / `2401eb16…` / `e3ac7dd5…`
  - T8: `9a63ae08…` / `a638dbc1…` / `b506cc9f…`

No physical GBA result is claimed. These are emulated-timer measurements of
one controller tape per mission.

## Limits

This uses the engine's existing half-scale, pixel-centered sight circles; it
does not implement BW's tile sight masks, sight upgrades, or ground-height
line of sight. It covers only the Terran product. The Zerg Z6 and Protoss P7
products have the same human acquisition gate. Their generic actors without
Z/P facts still use the engine radii. No assets, generated data, ROMs, saves
or traces are tracked.
