# Playtest and N64 fidelity review — 2026-10-27

Tested main: `c3c8ac956aa8fbc69a09562d629e7c3897c7d06c` (after PR51).
Production Showcase ROM SHA-256:
`b8d32d58102f8d4494d8271a2bdb4335b4a7b943b2045ae15f39039469c2b7f0`.
This audit changes no gameplay or assets. Private captures, generated banks,
ROMs and saves remain ignored.

## Methods and practical limits

- A private ARM wrapper includes the unchanged production game and common
  renderer, with the production Showcase word-sprite/source-effect flags.
  Controller button sequences enter all eight scenes through the menu.
  The wrapper calls ordinary `game_update`, `audio_update` and `game_render`.
  It observes state and clock counters without patching world state.
  Owned mGBA processes run without GDB, an isolated save directory, and no
  access to an existing emulator session. All eight reset checks compare
  both world and framebuffer hashes against the original paused preview.
- A sanitized host replay uses the same production renderer flags, normal
  controller/menu inputs, and captures paused tick 0 plus gameplay ticks
  1, 8, 16, 20, 24, 30, 60, 120 and optional 300. Terminal and reset rows
  are logged separately. Its images are **host-renderer captures**, not
  screenshots from the ARM emulator. Native desktop capture was unavailable.
- Separate controlled host fixtures inspect construction, worker cargo,
  unit movement and Scout projectiles. Their setup can place units, assign
  health and alter a target position; they are diagnostic fixtures, not
  normal-menu gameplay proofs. Runtime combat/render paths remain in use.
- The earlier multi-Pylon stress fixture was rebuilt against current game
  code with production Showcase flags and ASan/UBSan.
- Native visual reference: [StarCraft 64 NTSC longplay, Protoss First Strike](https://www.youtube.com/watch?v=rkyLf7lELXE&t=303s).
  Inspected the base/construction presentation near 5:03, the selected
  Protoss building display near 7 minutes, and combat/corpses near 10:07.
  Source interpretation also uses the existing authenticated ROM-derived
  tables and pinned reference code. These are representative comparisons,
  not synchronized N64/GBA input traces or proof of pixel-identical output.
- No physical GBA/N64 test, full campaign replay, audio fidelity comparison,
  broad sprite-completeness claim or crash-resolution claim is made here.
  No GitHub Actions were enabled or dispatched.

## What passed

The ARM replay completed 2,339 controller updates across all eight scenes.
Every restart/reset/exit route passed. The three last-stand scenes lost at
simulation tick 150. Other scenes remained playable through tick 361.
The host replay produced 77 frames and passed the corresponding routes.

The inspected Siege Line frames show source corpse/remnant artwork rather
than the old pink rectangular Zergling placeholders. Source Tank effects,
Protoss shield bars, and the blue/white warp glow are present in the scoped
captures; this does not establish complete animation or sprite coverage.

The Blood Bath stress test reached 36 Pylons (35 paid build orders), 41 live
buildings and 59 live units after 5,528 updates and 623 renders, with no
sanitizer finding. It used accelerated setup/resources and is not a hardware
reproduction of the reported fuzzy-screen crash. That crash remains open.

## Performance observations

The table uses `16,777,216 / mean_full_cycles` from measured ARM update,
audio and render calls. It is CPU-cycle equivalent throughput for these
windows, not desktop wall-clock fps or a physical-GBA guarantee. Timing
excludes wrapper reporting and hash/reset checks. It is not directly
comparable with prior differently sized replay windows.

| Scene | Measured updates | Mean total cycles | Equivalent fps |
| --- | ---: | ---: | ---: |
| Carrier Approach | 360 | 2,339,727 | 7.17 |
| Siege Line | 360 | 944,503 | 17.76 |
| Terran Last Stand | 149 | 591,688 | 28.35 |
| Zerg Last Stand | 149 | 575,112 | 29.17 |
| Protoss Last Stand | 149 | 594,062 | 28.24 |
| Protoss Crossfire | 360 | 2,126,714 | 7.89 |
| Three Way Battle | 360 | 1,353,451 | 12.40 |
| Base Under Siege | 360 | 1,016,375 | 16.51 |

Heavy-scene rendering dominates the budget: Carrier Approach averages
1,913,312 draw cycles versus 426,414 update cycles; Protoss Crossfire averages
1,729,213 draw cycles versus 397,501 update cycles. Near-30-fps performance
is not achieved in these representative combat scenes.

## Prioritized discrepancies

### 1. Combat damage and visual projectile arrival disagree

**Reproduced with a controlled fixture and current runtime code.** A Scout
attacking a flying target takes its HP from 500 to 472 on the initial attack
update. The source effect is still starting its release/flight animation.
Moving the target from x=150 to x=165 after release leaves existing tracer
endpoints at x=150. This can show damage/death before an apparent hit, or
missiles arriving at empty space.

The active source renderer already uses source speed and acceleration in a
bounded distance/time prediction. It must not be described as using only the
older constant-speed linear helper. It still follows a captured straight
path and lacks native target tracking, turning, collision and bounce logic.
In this fixture the source release is tick 1, startup is 1 source tick,
flight is 14 source ticks, and impact artwork lasts 14 source ticks. The
full 158-GBA-tick tracer lifetime includes a 126-source-tick actor recovery;
the projectile-side timeline is only 38 GBA ticks. A tracer's full lifetime
can include attacker recovery and impact effects;
it is **not** a valid measurement of flight time by itself.

Entry points: `sc64_source_emit` and attack damage in `starcraft/game.c`;
`sc64_weapon_source_trace_position` in
`starcraft/sc64_weapon_source_runtime.h`; trace rendering in
`starcraft/sc64_weapon_source_vfx_render.h`.

Suggested work: introduce bounded native projectile state/timing per weapon
family with reference-based tests; preserve instant-hit weapon semantics.
Do not fix this merely by adding a universal visual delay.

### 2. Scout twin air missiles coincide

**Reproduced in the same fixture and corroborated by imported metadata.**
Raw actor 70 / air weapon 74 emits variants 0 and 1 with the same heading
and identical x0/y0/x1/y1. The source profile declares two projectiles, but
changing only the packed variant does not give them separate launch geometry.
The result reads as one overlapping missile pair.

Reproduce: have a Scout attack a flying enemy. Inspect both tracers on the
first attack update. Fix using the reference offsets/heading behavior rather
than an arbitrary cosmetic spread. Entry point: `sc64_source_emit` in
`starcraft/game.c`.

### 3. Moving units have headings but no walking cycle

**Current rendering path confirmed; targeted movement fixture provided.**
The facing renderer selects a single frame from the 17 stored directional
poses (mirrored for the other headings). It has no movement-animation clock.
The actual-update fixture moves the Zealot from x=480 to x=506 and the SCV
from x=480 to x=512 over 16 updates. Both retain facing/frame index 8 and
unchanged frame pixel hashes across five samples. Thus position and direction
change while the gait stays fixed. The word
sprite path uses equivalent direction-only metadata. Attack poses are a
separate animation path and do not provide walking.

Reproduce: move a Zealot or Zergling across open terrain at a fixed heading.
Compare leg poses over several updates with the N64 reference. Entry point:
`sc64_facing_blit` in `starcraft/sc64_unit_facing.h` and `draw_unit` in
`starcraft/game.c`. Add bounded movement/idle animation selection using
source sequences without confusing gait with movement-speed changes.

### 4. Protoss construction skips its final materialization state

**Current lifecycle code confirmed; staged Pylon finish capture provided.**
The modifier-9 destination-dependent blue glow **is implemented** in the
current renderer. Older construction-modifier audit notes describing it as
an opaque ordinary sprite are superseded.

The remaining discrepancy is lifecycle: `race_tick_buildings` calls
`race_complete_building` at the construction threshold. That immediately
clears `under_construction` and resets progress, switching to the completed
building path without a separate source finish/materialization sequence.
This conclusion is source-derived; the N64 footage was not replayed with an
identically timed Pylon build to measure the finish offset.

Reproduce: watch a Pylon at the last construction update and the next update.
Relevant code: `race_complete_building` / `race_tick_buildings` in
`starcraft/race_adapter.h` and the construction branch of
`draw_building_overlay` in `starcraft/game.c`. Preserve the distinction
between mechanical completion and presentation when implementing the finish.

### 5. The production Carrier scene still has no Interceptor attack

**Observed scene behavior and compile-scope confirmation.** Carrier Approach
is a movement preview; the three Carriers have move orders. The normal
Showcase build does not enable the optional Carrier-training/Interceptor
flight prototype merged in PR43. Scouts and Zealots fight beside the ships,
but that does not establish Carrier combat.

Entry point: scene 0 in `starcraft/sc64_showcase_adapter.h` and the optional
Carrier flags in the Makefile. Qualify combat, save behavior, memory and
performance before promoting the separate prototype into production.

### 6. Worker cargo is a simplified badge

**Captured controlled mineral/gas carrying states for all three workers.**
Cargo is drawn as a hand-authored icon beside the unit, rather than a source
cargo attachment using native offsets/poses. This improves visibility but
remains a fidelity difference. It is lower priority than missing combat and
walking behavior. See `draw_worker_cargo_badge` in `starcraft/game.c`.

## Private evidence and rerun locations

All paths below are relative to the tested checkout, under ignored output:

- `.local/build/playtest-2026-10-27/gallery.html`: local frame gallery.
- `arm/replay.c`, `arm/run.py`, `arm/result.json`, `arm/arm.log` beneath
  that directory: input-only ARM replay and results.
- `host/replay.c`, `host/compile.sh`, `host/state.csv`,
  `host/unit-positions.csv`, `host/captures/`: production-flag host replay.
- `scout/scout_probe.c`, `scout/scout-ticks.log`, `scout/*.ppm`: controlled
  projectile fixture, HP/endpoints and renders.
- `details/fidelity-fixture.c`, `details/walking-frame-state.txt`,
  `details/cargo-state.txt`, `details/*.ppm`: controlled visual fixtures.
- `stress/pylons.c`, `stress/compile.log`, `stress/run.log`: Pylon stress run.
- `manifest.json`: hashes of local source/results and matching production ROM.

The private C harnesses include the checked-out game; rebuild them whenever
that source or compiler flags change. Execute prepared compiler recipes only;
do not regenerate shared private banks just to rerun this audit.

For the next fix, prioritize source-effect render cost alongside correct
projectile arrival/target tracking, then walking cycles and construction
materialization. Keep each behavioral change reviewable in a separate PR.
