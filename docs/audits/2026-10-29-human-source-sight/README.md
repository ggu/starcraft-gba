# Human source sight in the Terran runtime

Since `2abc3d5` ("Respect human fog when automatically acquiring attack
targets"), a human actor only acquires a new automatic target in a currently
visible cell. In the Terran runtime, most human actors had no source sight
fact. They still used the engine's approximate radii: 72 engine pixels for
units, 56 for workers and 64 for buildings other than town halls. Several
weapons reach farther than that sight. Examples are Goliath and Kerrigan
ground range (96), Wraith and Vulture (80), and the Missile Turret (112
against its 64 sight). These actors ignore enemies inside their weapon range
that the source game would see.

On `9ff7f7c` this made the full Terran T5, T7 and T8 host controllers lose or
stall. The default branch has since repaired those controllers to respect
fog (`a79b760`), and on `97c39fa` they win without this change. This change
is therefore a source-fidelity change: the actors see their source range
again, and the same controllers win sooner (see Validation).

The Terran runtime (`SC64_T4_RUNTIME`) now uses the pinned BWAPI
`sightRangeTiles` value for every actor that has a public raw UnitType ID. It
uses this value whenever no more specific T4, late-Terran or Z/P fact exists.
The 228-byte table `starcraft/sc64_source_sight_tiles.h` holds numeric facts
only, from BWAPI `d727fed68558c506163048ea889131d8cbb33915`. Existing
specific facts keep priority. The T4 special-actor values equal the table
entries. As in BW, an unfinished grounded building sees four tiles. The
Terran runtime has no in-place building morphs. The visibility snapshot
stores that state as `-(type+1)`, as Z7 already does, so removal uses the
radius that was added.

Most source radii (4–11 tiles, or 64–176 engine pixels) use the existing exact
row-span tables from the Z7 sight-span work. A moving unit's footprint change
runs in the same IWRAM symmetric-difference loop as the 56/72 radii, with the
radius's halfwidth row and extent passed in. Additions and removals use the
Z7 row spans. Radii without a table (1–3 tiles: Spider Mine, Nuke, doors and
wall traps) use the existing exact disk path. The Zerg Z6, Protoss P7 and
Showcase products do not compile the T4 runtime and do not change. Z7 span
builds keep the same logic; only the guard macro is renamed.

## Validation

Current-default results are for the merge of `97c39fa` into this branch
(`f5b7b7a` and its successor), against `97c39fa` itself. Private inputs for
both trees come from one frozen copy (see Provenance).

- **Host controllers.** Each build plays its own controller (traces kept in
  private worktrees, zero faults):

  | Mission | `97c39fa` | This change |
  | --- | --- | --- |
  | T5 | PASS, won at tick 159,216 | PASS, won at tick 112,855 |
  | T7 | won at tick 47,735 | won at tick 43,641 |
  | T8 | won at tick 193,336 | won at tick 158,784 |

- **Full Terran suite.** `make -k test-sc64-terran-full-campaign` passes
  completely:
  - The 234 skirmish startup/save configurations and every save/resume
    check pass.
  - All ten missions win: T4 at frame 8,911, T5 at tick 112,855, T7 at tick
    43,641, T8 at tick 158,784, T9 at tick 40,045 and T10 at tick 3,432.
- **Default checks.** `./dev build` passes. `make -k test` passes except
  `test_sc64_photon_cannon_renderer` (Protoss roster), which also fails on
  `97c39fa`. This private layout lacks `sc64_fact_tables_generated.h`.
- **Repro.** The private repro is an idle Goliath with a passive enemy Marine
  90 engine pixels away, on the T8 world. The Goliath attacks the Marine
  (hp 40→16) without diagnostic flags. At 60 pixels it attacks as before; at
  120 pixels, outside Goliath range, it does not attack.
- **Visibility fixture.** The asset-free incremental-visibility fixture adds
  source radii with and without span tables (128 and 48 engine pixels). It
  covers a Turret and a Command Center through construction, completion and
  removal, plus random construction-state toggles. All visibility, explored,
  source-count and minimap state matches full recomputation in every
  variant, including the T4 span variants that exercise the shared IWRAM
  loop. Both Z7 sight-span tests pass.
- **Actor rules.** `tests/sc64_t4_actor_rules.c` checks representative BWAPI
  values, and that each armed Terran unit's sight covers its ground and air
  weapon ranges.
- **IWRAM loop equivalence.** Moving the source-radius transition into the
  shared IWRAM loop changes no footprint. Before the merge, host T5, T7, T8,
  T9 and T10 traces were byte-identical with and without it, and every ARM
  probe window's gameplay and fog digests matched.
- **Memory guards in the merged Terran ARM link:**

  | | `97c39fa` | This change | Guard |
  | --- | --- | --- | --- |
  | IWRAM end | `0x03004e18` | `0x03004eb8` (+160 B) | `< 0x03005000` (328 B free) |
  | BSS end (EWRAM) | `0x020370c0` | `0x020370c0` | `< 0x0203e000` |
  | ROM text | 26,535,528 B | 26,537,492 B (+1,964 B) | |

  The extra IWRAM is the source branch of the span transition, inlined into
  `sc64_update_exploration`.

## Paired normal-menu ARM measurement

### Method

Each workload is a private host controller tape, embedded in a ROM built from
that tree's executed full Terran product command. The ROM starts at the title
screen, takes the normal campaign menu route, and feeds the tape one held-key
state per loop iteration.

`arm-probe/build_probe.py` builds each probe as follows:
1. It removes the product ELF and rebuilds it with a normal `make`.
2. It takes the one compile/link command that Make echoes and runs, and
   compiles each translation unit separately with the same ordered flags.
   `terran_sight_probe.c` takes the place of `common/main.c`.
3. It makes the state the probe reads global in `game.o`
   (`objcopy --globalize-symbol`).
4. It links with the product's link options, in their original order.

`starcraft/game.c` is therefore compiled as its own unit with the product's
flags. In every probe ELF, every function from the game units has the same
size as in the product ELF, and the IWRAM function set is identical. The
address-normalized instruction streams of all 37 named IWRAM functions,
`game_update`, `game_render` and `sc64_update_exploration` are identical too.
The only differences are `main`, `gba_keys`, `demo_replay_checkpoint`, the
probe's reporting helpers and the tape.

Each run is a fresh mGBA 0.10.5 Qt process with an isolated `HOME`, log mask
`7` and `videoSync=0`/`audioSync=0`. No GDB port is opened. All cycle counts
come from the emulated GBA timer (`gba_clock`, 16,777,216 Hz):
- *Update* is `game_update`.
- *Sight* is `sc64_sight_cycles` from that update, i.e. `update_exploration`.
- *Full* is update plus `audio_update` and `game_render`.

A tick is counted only when the screen is PLAY before and after it. Effective
fps is ticks × 16,777,216 / total full cycles. The probe stops at the first
result screen.

Games diverge when the fog gate changes a decision. The paired comparison
therefore uses only the leading 500-tick windows whose gameplay digest
matches between the two builds. That digest is the raw unit and building
arrays (the Terran records have no padding), resources and frame, with no
fog. Cycle columns are per tick.

### Current default (`97c39fa` vs this change)

Two tape sets were used:
- **Earlier tapes:** the pre-merge controller traces of this change.
- **Current tapes:** the `97c39fa` controller traces.

Both builds replay the same tape in each pair.

| Tape | Mission | Paired ticks | Build | Mean update | Mean sight | Peak sight | Mean full | Sight / full | Effective fps |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| earlier | T5 | 4,991 | `97c39fa` | 327,698 | 15,396 | 68,539 | 703,809 | 2.19% | 23.84 |
| | | | this change | 331,001 | 18,694 | 185,090 | 708,706 | 2.64% | 23.67 |
| earlier | T7 | 38,989 | `97c39fa` | 361,662 | 58,265 | 162,995 | 993,309 | 5.87% | 16.89 |
| | | | this change | 346,546 | 43,067 | 248,472 | 974,681 | 4.42% | 17.21 |
| earlier | T8 | 33,487 | `97c39fa` | 372,171 | 37,432 | 390,908 | 934,531 | 4.01% | 17.95 |
| | | | this change | 352,981 | 17,947 | 286,248 | 917,353 | 1.96% | 18.29 |
| current | T5 | 4,991 | `97c39fa` | 327,698 | 15,396 | 68,539 | 703,809 | 2.19% | 23.84 |
| | | | this change | 331,001 | 18,694 | 185,090 | 708,706 | 2.64% | 23.67 |
| current | T7 | 5,489 | `97c39fa` | 370,985 | 66,972 | 113,408 | 728,729 | 9.19% | 23.02 |
| | | | this change | 352,223 | 47,937 | 135,995 | 686,514 | 6.98% | 24.44 |
| current | T8 | 27,987 | `97c39fa` | 387,859 | 32,093 | 440,514 | 886,153 | 3.62% | 18.93 |
| | | | this change | 404,858 | 50,007 | 647,220 | 899,309 | 5.56% | 18.66 |

Both T5 tapes give the same first 5,000 ticks.

The cost depends on the workload:
- **T7:** cheaper with both tapes (+1.9% and +6.2% fps). On `97c39fa`, the
  late-Terran actors that already had source sight use the brute-force disk.
- **T8:** cheaper with the earlier tape (+1.9% fps). With the current tape,
  which moves many more human units, sight work rises from 32.1k to 50.0k
  cycles per tick, and effective fps falls by 1.4% (18.93 → 18.66). Peak
  sight rises from 441k to 647k cycles.
- **T5:** 3.3k sight cycles per tick (0.7% fps) above `97c39fa` over its short
  paired span.

Peak full-tick cost comes from update work outside sight in every run.

Full-mission runs, each build on its own controller tape, give these figures.
They are not paired, because the games differ.

| Mission | Build | Ticks | Mean sight | Peak sight | Sight / full | Effective fps |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| T5 | `97c39fa` | 159,215 | 44,281 | 1,245,735 | 4.89% | 18.53 |
| | this change | 112,854 | 67,636 | 3,064,051 | 7.24% | 17.96 |
| T7 | `97c39fa` | 47,734 | 72,797 | 152,491 | 9.59% | 22.10 |
| | this change | 43,640 | 60,146 | 235,705 | 7.80% | 21.74 |
| T8 | `97c39fa` | 193,335 | 24,189 | 440,514 | 2.68% | 18.60 |
| | this change | 158,783 | 37,096 | 647,220 | 4.10% | 18.56 |

T5 has a single-tick sight spike of 3.06M cycles when about 30 units
transfer to the player near tick 9,000. Most of it is per-cell
`sc64_visibility_source_change`, which runs from ROM on `97c39fa` too, where
the same event costs 1.25M cycles.

### Earlier measurement (`9ff7f7c`)

Before the merge, the same method was used, with the probe including
`game.c` in its own unit. The paired windows compared three builds:
`9ff7f7c`, this change at `0ff389e` (ROM-resident Z7 transition), and this
change with the shared IWRAM transition. Cycle columns are per tick:

| Mission | Paired ticks | Sight (`9ff7f7c` / `0ff389e` / IWRAM) | Effective fps |
| --- | ---: | --- | --- |
| T7 | 38,989 | 70.0k / 89.5k / 50.8k | 19.83 / 19.30 / 20.38 |
| T8 | 33,487 | 37.4k / 21.1k / 17.9k | 20.71 / 21.11 / 21.18 |
| T5 | 4,991 | 15.4k / 30.5k / 18.7k | 23.79 / 23.29 / 23.62 |

The ROM-resident transition cost T7 19.5k extra sight cycles per tick, which
is why the transition moved into the shared IWRAM loop.

### Host and ARM mission outcomes

This is separate from the cost measurement.
- **Before the merge:** the ARM replays of this change's host controller tapes
  did not reach the host outcomes, on `9ff7f7c` or on this change. T8 showed
  a loss at about tick 46,800, and T5 and T7 reached no result.
- **In the merged layout:** each build's own host controller tape does reach
  a won result screen in the normal-menu ARM replay, at the host trace's
  final frame:
  - `97c39fa`: T5 at frame 158,491, T7 at 47,101 and T8 at 185,476.
  - This change: T5 at frame 112,381, T7 at 43,021 and T8 at 153,556.

These replays are emulator evidence of three controller tapes reaching their
host results. They are not a campaign-completion, fresh-process SRAM or
hardware qualification, and the earlier gap was not investigated here.

## Provenance

- **Toolchain:** `arm-none-eabi-gcc` (GCC) 16.2.0, GNU ld (GNU Binutils)
  2.47.20260726. Emulator: mGBA 0.10.5.
- **Recorded builds.** The product builds were recorded with a normal `make`
  after removing the ELF (never `make -B`). Each record holds:
  - the executed argv;
  - its ordered `-D`/`-I` vector (85 entries);
  - a per-translation-unit dependency pass (`gcc -M` with the same argv);
  - the hash of every dependency and `.incbin` blob.

  The records are private (they name local paths).
- **Command.** The executed argv is identical for `97c39fa` and this change
  (SHA-256 of the NUL-joined argv
  `be71dbd4a75c5fe4ab12fb402cb7be81888cbce6e87822df7f4d56a6a915e127`). In
  order:

  ```
  arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -g -std=c11 -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Icommon -DSC64_IMPORTED_MISSION -DSC64_HAS_TERRAIN_ART -DSC64_HAS_SPRITES -Istarcraft -I.local/sc64/backwater/generated -DSC64_AUDIO_IMPORTED -I.local/sc64/audio/generated '-DSC64_AUDIO_BLOB=".local/sc64/audio/generated/sc64_audio_generated.bin"' -DSC64_RACES -DSC64_DEPOT_BOUNDED -DSC64_ENGINE_CORE_REQUIRED -DSC64_CONSTRUCTION_HP -DSC64_SIEGE_SPLASH -DSC64_FIELD_CONTROLS -I.local/sc64/briefings/generated -I.local/sc64/interface/generated -I.local/sc64/command-icons/generated -DSC64_HUD_ICONS -DSC64_CONSTRUCTION_SOURCE_ART -I.local/sc64/construction/generated -DSC64_BUILDING_FIRE_ART -I.local/sc64/building-fire/generated -DSC64_ORDER_MARKER_ART -I.local/sc64/order-marker/generated -DSC64_UNIFIED -DSC64_IMPORTED_SKIRMISH -I.local/sc64/skirmish/generated -DSC64_LARGE_MAPS -DSC64_T4_ENABLED -DSC64_T4_RUNTIME -DSC64_ROUTE_PACK3 -DSC64_T4_WINDOWED_SCHEDULE -DSC64_CAMPAIGN_RACE=0 -DSC64_T4_SCENARIO_RUNTIME -DSC64_T4_COUNT_CACHE -DSC64_NO_FRAME_CLEAR -DSC64_TERRAIN_SCROLL -DSC64_STATIC_KEY_CULL -DSC64_VISIBILITY_SPANS -DSC64_UNARMED_QUERY_SKIP -DSC64_T3_ENABLED -DSC64_T4_CAMPAIGN_ENTRY -DSC64_T5_ENABLED -DSC64_T5_RUNTIME -DSC64_T5_CAMPAIGN_ENTRY -DSC64_T6_ENABLED -DSC64_T6_STORAGE -DSC64_T6_RUNTIME -DSC64_LATE_TERRAN_STORAGE -DSC64_TERRAN_LATE_ACTORS -I.local/sc64/terran-late/import-stage -I.local/sc64/races/generated -DSC64_TERRAN_LATE_ENABLED -DSC64_TERRAN_LATE_RUNTIME -DSC64_T6_CAMPAIGN_ENTRY -DSC64_TERRAN_FULL_CAMPAIGN_ENTRY -DGBA_SPRITE_WORDS -DGBA_SCENE_CLIP -DSC64_HUD_BUFFER_REUSE -DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_CACHE_VRAM -DSC64_HUD_VRAM_BANDS -DSC64_TERRAIN_ROW_MASKS -DSC64_MINIMAP_RASTER -DGBA_WINDOW_BANDS -DSC64_STATIC_DIRTY_ROWS -DSC64_ROUTE_FLOOD_SPLIT -DSC64_UNIT_WORD_SPRITES -DSC64_CONTACT_CACHE -DSC64_MINIMAP_BLIT -I.local/sc64/facing/generated -DSC64_UNIT_FACING -DSC64_TANK_SOURCE_VFX -I.local/sc64/tank-vfx/generated -DSC64_WEAPON_SOURCE_VFX -I.local/sc64/weapon-vfx/generated -DSC64_UNIT_ATTACK_POSES_ENABLED -I.local/sc64/attack-poses/generated -DSC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL -DSC64_TRACE_DURATION_CACHE -DSC64_DIRECT_FIRING_POINTS -DSC64_DEATH_SOURCE_ART -I.local/sc64/death/generated -DSC64_ROUTE_MONOTONE_PROBE common/start.s common/main.c common/gba.c common/memory.c common/audio_bank.S starcraft/game.c starcraft/sc64_interceptor_motion.c starcraft/sc64_interceptor_pool_codec.c -mcpu=arm7tdmi -mthumb -mthumb-interwork -nostdlib -Tcommon/gba.ld -Wl,--gc-sections -Wl,-Map,build/starcraft-terran.map -lgcc -o build/starcraft-terran.elf
  ```

- **Translation units** (8, in order): `common/start.s`, `common/main.c`,
  `common/gba.c`, `common/memory.c`, `common/audio_bank.S`,
  `starcraft/game.c`, `starcraft/sc64_interceptor_motion.c` and
  `starcraft/sc64_interceptor_pool_codec.c`.
- **Inputs.** The recorded inputs of the two trees differ only in this
  change's four files: `race_rules.h`, `sc64_source_sight_tiles.h`,
  `sc64_visibility.h` and `sc64_z7_visibility_spans.h`.
  - Private inputs: 34, identical in both trees (aggregate SHA-256
    `351deb411d1d1d3fdcb1f0697d1bb7ca6ad0528aef512fb862b38b241b75fe7a`).
    The building-fire and order-marker packs that `97c39fa` needs are
    private copies of the shared headers, frozen for both builds.
  - Tracked inputs: `97c39fa` has 143
    (`63e4ceb015c72c3d7a5713270f663b8e2e17f0da05a4abd8203e923332c68ae9`).
    This change has 145
    (`7f200da176f3b1da364978cc8aef4c3965a0c4d31a0f5f794358f692fa55abea`).
- **Product loadable image** (`objcopy -O binary` of the ELF). It is
  reproducible across rebuilds; the ELF itself is not, because its debug
  sections name temporary files.
  - `97c39fa`: `d337cdedd4af501b72f8aa77ec416e2fa3384a15f323f7bbc61f825d25b9f681`
  - This change: `fdde43dc89bf0770c6c0c5621e364bbe8031487c27854dc42997aa745bc82627`
- **Probe source** SHA-256:
  `86a1f9e08da44e0294eb5dbecbbee6679d3acde4729b5eaca0aa2a442c550f6c`.
- **Controller traces** (private, not tracked; inputs to
  `arm-probe/make_tape.py`):
  - Earlier tapes (this change before the merge): T5 `46dc71da…`, T7
    `1e0cdd6b…`, T8 `eba76163…`.
  - Current tapes (`97c39fa`): T5 `4ca8aeb7…`, T7 `52454366…`, T8
    `5d17c5ab…`.
  - This change's merged controller traces: T5 `51d3750e…`, T7 `aa7b9e8b…`,
    T8 `992416dd…`.
- **Probe ROM SHA-256**, `97c39fa` / this change:
  - Earlier tapes: T5 `199a4d1d…` / `391c559e…`, T7 `04f2411d…` /
    `96f8cc06…`, T8 `8a8ef1fa…` / `c9abc0a4…`.
  - Current tapes: T5 `0494f7cd…` / `d89080d7…`, T7 `bdd39c3a…` /
    `9dfce55d…`, T8 `58068d30…` / `9a3c55c7…`.
  - This change's own tapes: T5 `0b8906bc…`, T7 `ff1a1f35…`, T8
    `facd9c7b…`.

No physical GBA result is claimed. These are emulated-timer measurements of
one or two controller tapes per mission.

## Limits

This uses the engine's existing half-scale, pixel-centered sight circles; it
does not implement BW's tile sight masks, sight upgrades, or ground-height
line of sight. It covers only the Terran product. The Zerg Z6 and Protoss P7
products have the same human acquisition gate. Their generic actors without
Z/P facts still use the engine radii. No assets, generated data, ROMs, saves
or traces are tracked.
