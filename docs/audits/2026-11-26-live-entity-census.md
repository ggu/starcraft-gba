# Live-entity census: render-side trial

A design panel on November 25 proposed replacing the full-table unit and
building scans with a roster of live slots. The trial below is the
smallest step it recommended. It failed its kill criterion, so nothing
from it is merged. The code is on the local branch
`claude/live-census-render-experiment` (commit `e3773ee3`) for
reference. All figures are mGBA guest cycles; nothing was measured on
hardware.

## What was tried

When a frame starts to draw, `census_fill()` scans both tables once and
records the live slots in ascending order, as a bitmap and as a byte
roster. Nothing spawns while a frame draws, so the roster stays exact
until the frame ends. Four consumers iterated it instead of every slot:

- `game_audio_observe`, which still visits a slot that has a remembered
  snapshot, so a death cue is not lost;
- the `draw_minimap` dot loops;
- `field_ui_refresh`;
- `sc64_hud_changed`.

Each consumer kept its own `alive` and class tests.

## Result

Paired runs from the same save, 300 updates, base `d496f1a2` against
the trial. Every pair was bit-identical: world state, unit and building
rows, both pages, palette and final pause save.

| Save | Base | Trial | Change |
| --- | ---: | ---: | ---: |
| Terran Outpost | 772,619 | 782,717 | +10,098 |
| Terran Blood Bath | 869,018 | 884,896 | +15,878 |
| Terran opening | 562,731 | 562,671 | -60 |
| Zerg Outpost | 1,058,372 | 1,070,131 | +11,759 |
| Protoss Outpost | 973,092 | 987,172 | +14,080 |

The kill criterion was a saving of at least 10k on Terran Outpost. The
trial instead cost 10.1k. In the opening the trial's drawing was 18k
cheaper, but the opening already runs at the VBlank pace, so the frame
mean does not move.

A per-function profile of Terran Outpost (32 updates) shows why.
`census_fill` costs 10,329 cycles per update: about 36 cycles per slot
from ROM Thumb, the same as a rejected slot in the loops it replaces.
The four consumers saved about 0.9k between them: `game_render` self
time fell 716 and the field UI hint 214, while `sc64_hud_changed` rose
420. A fill that rode inside the audio loop, as the panel's skeptic
suggested, would avoid the separate pass. It is still capped by the
other three consumers' dead-slot share, 3.3k + 3.1k + 2.8k = 9.2k on
Terran Outpost, before roster overhead, so it cannot reach 10k either.

## Notes for a later attempt

- A census only pays when one fill serves many scans, or when the
  bitmap is maintained at the spawn sites without refills. The update
  side has about 14 qualifying scans, but each also needs hooks at every
  whole-row write (loads, decoders, journal restores).
- `sc64_unit_buckets_begin`, `sc64_update_exploration` and
  `race_p7_carrier_training_end_frame` need dead rows: a bucket cell, the
  previous visibility, and dead Interceptor and Carrier rows by type.
  A live-only set is not enough for them.
- `tests/game_audio_run.c` writes audio snapshot words directly for
  slots the observer never saw, and the tile renderer rewrites the VRAM
  that holds them. An observer that tracks "slots with a snapshot" has
  to allow for both.
- An ARM fill in IWRAM would cost far less per slot, but the Zerg
  cartridge has 48 bytes of IWRAM left.
