# Busy-combat cycle profile and first exact reductions

Date: 2026-11-10. Base `3d0d7165`; changes on `codex/busy-combat-perf-20261110`.

Every number here is guest CPU cycles on the mGBA core, not a hardware frame
rate. One "update" is one main-loop iteration: input, `game_update`,
`game_render`, present. Two video frames, the 30 fps budget, are 561,792
cycles.

## Method

A private runner derived from `tools/sc64_headless_replay.c` executes one
instruction at a time over a window and adds each instruction's exact cycle
cost to the address it ran at, with a shadow call stack for inclusive cost
per function. It writes nothing to the guest. Per-update elapsed cycles,
`frame`, `screen` and sampled world state are identical with and without it,
and the per-function totals add up to the elapsed cycles exactly. The tool,
tapes and ROMs stay private.

Four ordinary-input workloads, each a pinned 1,000-update window:

- **Campaign battle:** the retained Protoss mission 7 tape, updates
  8,020–9,019 (177 units, 112 buildings, three Arbiters), on the P10 ROM.
- **Custom battles:** one match per ROM (Terran, Zerg Z9, Protoss P10) on
  Blood Bath, a scripted ordinary-input Zerg human against Protoss, Terran
  and Zerg opponents. The tape is the HOST controller's own input; the ARM
  replay reaches the same result screen, frame and unit counts, and two ARM
  runs give byte-identical cycle logs. The window is the 1,000 consecutive
  updates with the highest mean cost (about 140 units, 52 buildings).

## What the base profile showed

| Window | Mean | p95 | Max |
| --- | ---: | ---: | ---: |
| Campaign battle (P10) | 2,263,620 | 2,808,842 | 4,143,640 |
| Custom, Terran | 1,742,065 | 3,932,628 | 6,741,069 |
| Custom, Zerg | 2,692,062 | 4,213,528 | 6,988,072 |
| Custom, Protoss | 2,219,033 | 4,494,244 | 8,145,803 |

- **Custom battles: route floods.** `sc64_route_next` and its children were
  33–34% of all cycles, and the slowest 5% of updates were slow almost
  entirely because of them. A blocked unit asks for a route field toward its
  own destination cell. The cache held 14 fields sized for a 128-by-128 map
  on 64-by-64 maps, so about 70% of requests rebuilt a field with a full-map
  flood of about 2,740 cells (about 390,000 cycles).
- **Campaign battle: Arbiter bookkeeping.** `sc64_p7_arbiter_cache_source_tick`
  was 18.8% (335,000 cycles of its own code on four of five updates) and
  movement collision 15%. Route work was under 1%.
- **Present wait.** `gfx_end` polled for VBlank for 64,000–91,000 cycles per
  update while nothing else ran.
- **Placement cursor.** On frames where a building was being placed, the
  outline scan cost about 300,000 cycles and the site check ran up to four
  times.
- **Weapon tracers in the Zerg ROM.** `draw_tracers` was 267,000 cycles
  (16%). The Zerg flag chain compiled neither the trace-duration cache nor
  the off-screen cull that the Terran and Protoss ROMs have, so every
  tracer's duration was recomputed every frame.
- Cartridge wait states are already the fast setting, and the game never
  halts the CPU, so there was nothing to reclaim there.

## Changes

None alters a route answer, which route fields the game sees as cached, a
drawn pixel in the compared frames, a rule value, a save format or an actor
cap.

1. **Spare route bank used for evicted fields** (`SC64_ROUTE_ACTIVE_STRIDE`,
   in every canonical ROM). The logical cache keeps the fixed layout's slot
   count and bookkeeping (14 in these products). The unchanged 86,016-byte
   bank is partitioned at the active map's packed field size, up to 56
   fields on a 64-by-64 map (48 where range work reserves its two planes);
   the fields not owned by a logical slot hold the bytes of evicted complete
   point fields as donors. A miss that would flood a goal whose donor exists
   swaps field pointers instead of flooding. Maps where the bank holds no
   more fields than the logical cache, 128-by-128 among them, keep the fixed
   layout.
2. **Route fields survive structure changes** (`SC64_ROUTE_KEEP_FIELDS`). A
   structure change still resets every piece of logical bookkeeping, but a
   complete point field's bytes stay as a stale donor. A stale donor may
   answer only a single-cell next-step read, and only after a check on that
   read: the stored path from the origin to its root is still walkable, the
   seed set is unchanged, and no cell opened since the flood could lie on a
   path as short as the stored one. Every other reader floods first.
3. **Page flip from the VBlank interrupt** (`GBA_ASYNC_PRESENT` through
   `SC64_PRESENT_FLAGS`). `gfx_end` queues the flip and the sampled-audio
   interrupt performs it, at most once per two hardware frames. Host builds
   and ROMs without that interrupt keep the polled gate.
4. **Placement outline from row bitmasks**, and the site verdict computed
   once per draw.
5. **Per-profile and per-tracer weapon trace values cached**, and the
   existing off-screen cull enabled for the Zerg chain.

### A first version that was not exact

The first form of change 1 raised the logical slot count itself to 56. It
measured slightly better on two ROMs, and three sampled matches showed the
same state as the base, but a full comparison found 16 of 54 custom lobbies
ending in a different state. Weapon-range movement
(`sc64_range_cached_point_step`) reads a route field only if one is cached,
so which fields are resident changes the simulation. The donor design above
replaced it. The same property means any test controller that queries routes
through the cache perturbs the game it is playing.

## Results

Custom windows, mean cycles per update for the whole loop (same pinned tape
and window; the sampled game state matches the base in every run):

| ROM | Base | Now | Change |
| --- | ---: | ---: | ---: |
| Terran | 1,742,065 | 1,120,887 | −35.7% |
| Zerg | 2,692,062 | 1,377,035 | −48.8% |
| Protoss | 2,219,033 | 1,482,554 | −33.2% |

| ROM | p95 base → now | Max base → now |
| --- | --- | --- |
| Terran | 3,932,628 → 1,598,086 | 6,741,069 → 2,492,161 |
| Zerg | 4,213,528 → 1,877,375 | 6,988,072 → 4,383,774 |
| Protoss | 4,494,244 → 1,990,135 | 8,145,803 → 4,462,920 |

`game_update` alone: 1,219,213 → 702,395 (Terran), 1,862,028 → 837,215
(Zerg), 1,597,674 → 961,147 (Protoss). Updates whose `game_update` exceeds
3,000,000 cycles: 70 → 0, 110 → 2, 127 → 4.

Campaign battle: mean 2,263,620 → 2,163,846 (−4.4%), p95 2,808,842 →
2,503,522, max 4,143,640 → 3,773,540. Its map is 128 by 128, so the route
changes do not apply there; the gain is the page flip and placement cursor.

Memory, from the ELFs (limits `0x03005000` and `0x0203e000`):

| ROM | `__iwram_end` base → now | `__bss_end` base → now |
| --- | --- | --- |
| Terran | `03004480` → `030044a8` | `02036f2c` → `02037484` |
| Zerg Z9 | `03004800` → `03004850` | `0203ac4c` → `0203b1ac` |
| Protoss P10 | `030047f8` → `03004818` | `020391f0` → `02039758` |

That is 32–80 bytes of IWRAM and 1,368–1,384 bytes of EWRAM per ROM. The
hot flood did not grow; the IWRAM bytes are register-allocation effects in
`is_traversable` and `move_toward`.

## Exactness evidence

- **Route changes, whole game, HOST.** The ordinary-input custom-match
  controller, built from each ROM's flag vector at the base and with the
  route changes, gives byte-identical per-update traces and result lines in
  all 18 lobbies of each of the three products (2 maps × 3 human races × 1–3
  opponents). Four mission controllers (Z5, Z7, P4, P7) give identical
  output. The 330-case custom AI matrix passes.
- **Route changes, unit level.** `make test-sc64-route-keep-fields` issues
  321,445 queries per build on five map shapes and five terrains with
  block, open, mixed, revert and burst grid changes. Every answer equals a
  from-scratch tie-ordered flood, and the logical cache state hash-equals
  the fixed layout's after every query. Nine deliberately wrong variants of
  the validity check and donor handling were each caught.
- **Other changes, whole game, HOST.** With the route option removed, the
  earlier candidate's custom AI harness output was byte-identical to the
  base on the cases compared, and all 29 mission controllers compared gave
  identical output on base and candidate. Frames rendered every 90 updates
  matched wherever the simulation matched.
- **Tracers, ARM.** Both mode-4 pages and the palette, hashed after every
  update of each custom window and every tenth update of the 500 before it,
  are identical before and after the tracer change: 3,150 frames, 0
  mismatches. `make test-sc64-weapon-source-ticks-bank` compares the cached
  values with the earlier code over all 66 profiles (686,256 cases).
- **Placement outline.** `make test-sc64-building-placement-vfx` compares
  it with the earlier scan on 6,000 random sprites.
- **ARM windows.** `frame`, `screen` and the sampled unit and building
  counts match the base at every sample in all four windows.
- Save formats are untouched; the 30 save and fresh-load continuation cases
  of the AI matrix pass.

Not shown: per-update state identity on ARM (only sampled), frames outside
the four windows, and anything on hardware.

## What remains

- **These battles are still two to three times the 30 fps budget.**
- **Spikes when a structure is placed or destroyed** are smaller but not
  gone: the worst updates are still about 4.4 million cycles in the Zerg and
  Protoss windows. A stored route that crosses a changed cell, an origin
  that was unreachable, and every worker's depot route still rebuild, and
  the walkability grid itself is rebuilt in full (about 230,000 cycles).
- **Arbiter cloak bookkeeping** still costs about 426,000 cycles per update
  in the campaign battle, which these changes barely touch.
- The rest is spread thin: target index rebuild and nearest-target queries
  (about 100,000), worker depot lookups (about 100,000), collision masks
  (50,000–65,000), terrain band restore and unit drawing.
- No hardware measurement. mGBA does not model the cartridge prefetch
  buffer exactly, so ROM-code cost on a console can differ.

## Round two: the worst updates (2026-11-12)

Base `22015eb2` for the profile, `bdc7b795` (soft fog on) for the final
measurement. Same pinned windows, regenerated against the then-current
opponents (`same_workload` true for every run). Instruction-level profile of
the five slowest updates of each custom window; "incl" is inclusive cycles.

| Window | Slowest updates (cycles) | What dominates them |
| --- | --- | --- |
| Zerg | 4.38M, 3.75M, 3.07M, 3.00M, 3.00M | Point route floods (`sc64_route_flood` with `sc64_route_put_new`, about 360k to 450k per flood of roughly 2,700 cells) are 35% to 60% of each update except the first. The first (4.38M, a structure change) is depot routing 2.08M, of which the depot-slot scan `race_resource_depot_valid_for` 619k (7,251 calls, 85 cycles each, every worker scanning all 160 slots because the owner lists are invalid after the change), depot route floods 1.15M (including `sc64_depot_cell_point` 130k and `build_path_grid` 290k), plus 884k of point floods. The second (3.75M) is depot floods again (903k) and three point floods. |
| Protoss | 4.62M, 3.25M, 2.94M, 2.93M, 2.88M | The first is depot routing 1.51M (`build_path_grid` 286k, `sc64_depot_cell_point` 161k, depot floods) and point floods 1.1M. The rest are point floods (1.1M to 1.5M of each). |
| Terran | 2.49M (max) | Three point route fills, 1.35M (54%) of the update, of which the flood loop 1.09M. |

The top self-cost functions of the mean are not concentrated: `game_render`
8% to 11%, `sc64_route_flood` 3% to 4%, `sc64_contact_blocked_mask_uncached`
4%, `sc64_target_nearest` 3%, `sc64_route_monotone_probe` 3% to 4% (Terran,
Protoss). No single exact change in these was found that takes 3% off a mean.

Changes (exact: 54 of 54 custom lobbies byte-identical per product, HOST):

- `race_nearest_resource_depot` rejects dead and foreign slots before calling
  `race_resource_depot_valid_for` (the first two tests of that function).
- `sc64_depot_cell_point` uses 32-bit arithmetic for map-sized arguments
  (535 cycles per call before; `tests/sc64_depot_cell_point_diff.c`, 120M
  cases against the original).
- The walkability grid is updated incrementally after a structure change
  (`starcraft/path_grid_build.h`; `tests/sc64_path_grid_incremental.c`; the
  full rebuild remains the fallback). The rebuild went from about 290k to
  about 125k cycles in the worst updates.

| Window | Mean per update | Slowest update |
| --- | ---: | ---: |
| Terran | 1,157,945 to 1,156,120 (-0.16%) | 2,569,569 to 2,569,662 |
| Zerg | 1,424,326 to 1,420,239 (-0.29%) | 4,476,226 to 3,803,179 (-15.0%) |
| Protoss | 1,529,227 to 1,526,177 (-0.20%) | 4,635,774 to 4,393,100 (-5.2%) |

Updates over 3,000,000 cycles are unchanged (0, 5, 2). The mean barely moves
because the gain is limited to the updates that follow a structure change.
After the change the slowest Zerg updates (3.80M, 3.76M) are depot floods and
point floods; the follow-on spike ten updates after a structure change (every
worker's depot field restarts as a farther worker asks) and the point floods
remain. Skipping a flood exactly needs a persisted flood frontier per field
(RAM) or a changed route cache; both were rejected here. A faster flood loop
was estimated at 1.5% to 2% of the mean and was not attempted.

Memory against `bdc7b795`: IWRAM end unchanged on all three ROMs; EWRAM end
+608 bytes Terran, +984 bytes Protoss and Zerg (the Zerg ROM keeps 5,704 of
its 6,688 bytes of headroom). Not shown: hardware, P7 campaign window, other
frames.

