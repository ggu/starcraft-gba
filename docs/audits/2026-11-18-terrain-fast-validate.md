# Cheaper exact terrain-cache validation (`SC64_TERRAIN_FAST_VALIDATE`)

## Why this target

A re-profile on `95a435c7` (the source-release head on 18 November) took
25 exact instruction-level samples in each of six 1,000-call windows of the
current Z10 cartridge and the Showcase cartridge. In the two dense Z10
campaign windows, all four sprite blitters together cost 119,454 (calls
19,000–19,999) and 102,541 (calls 35,000–35,999) guest cycles per sampled
update. The terrain restore/capture group cost 142,547 and 142,711, and
stayed near 140k in every busy window, including the custom battles where
few units are on screen (138,448 in the Z10 custom battle, 144,404 in
Showcase). The biggest piece of that group was not pixel copying but the
per-frame cache validation inlined into `game_render`: the culled static
object key, the per-tile clean test and the cache lookup, about 61–67k per
update, ahead of the band capture (27–35k) and the scrolled restore
(18–27k). Validation work is pure overhead when the answer is "unchanged",
so it is the best render target on current source that can be made faster
without changing a single pixel.

For this audit window B was profiled again from a fresh flag-off build
(the same 25 calls, 35,000 + 40*i). Every number above for B reproduced
exactly: whole loop 1,489,106, terrain group 142,711 (65,540 of it
inlined in `game_render`, plus `sc64_terrain_state_view` 6,807 and
`sc64_terrain_fill_now` 6,267), blitters 79,212 + 22,295 + 1,034. The other
windows' figures are from the earlier profile and were not re-measured.
With the flag on, the same samples give a terrain group of 102,143
(inlined part 33,087, `sc64_terrain_state_view` no longer called,
`sc64_terrain_fill_now` 4,965) and a whole loop of 1,445,227; capture
(34,528) and restore (22,827) are unchanged, as are the blitters, fog and
`game_update` (678,738 against 678,675).

## What the flag changes

`SC64_TERRAIN_FAST_VALIDATE` is a compile-time flag, off by default and in
no product flag set. With it off, every product compiles exactly the code it
compiled before: `unifdef -USC64_TERRAIN_FAST_VALIDATE` on the three edited
headers leaves the base text apart from blank lines and one macro alias for
`sc64_terrain_state_view` in `draw_map`, so every configuration, not only
the four rebuilt ROMs, is unchanged with the flag off. With it on, the same decisions are reached with less work:

- **Static object key** (`starcraft/sc64_static_key_fast.h`, selected from
  `sc64_static_key.h` under `SC64_STATIC_KEY_CULL`). The reference keeps one
  "drawn" field per mineral, geyser and building slot plus the payload
  fields last seen while drawn. The fast key keeps the drawn state as bit
  words (one bit per slot, plus a construction bit for drawn buildings) and
  the payload fields only. A building's reference drawn field (0, 1 or 3)
  maps one-to-one onto the pair of bits; payloads are compared and stored
  with the reference's rules, including the out-of-range reset under
  `SC64_ZP_FULL_STORAGE`. The generation advances on exactly the same
  calls. Storage is smaller (Z10 and Showcase EWRAM end falls by 568 bytes).
- **Scrolled dirty/partial test** (`sc64_terrain_cached`). The per-tile
  test splits into a column half (old-window offset, clipped rectangle
  leaving the old camera) and a row half, each computed once; the per-tile
  work is the mask compare. The reference loop stays as the fallback for a
  view wider than 17 columns, which the view geometry never produces.
- **Same-camera soft-fog test**: with soft fog and no row masks, one pass
  over the view's cells compares `mask[i]` with `sc64_terrain_now[i]`, the
  same comparison the nested loop makes.
- **Draw-pass tile answers** (`sc64_draw_map.h`). When the draw window is
  exactly the cache view that the lookup validated, tile *i* of the
  row-major draw loop is cache cell *i*, so the clean, partial and fog
  signature answers become array reads; the index is advanced before any
  `continue`. Any other window uses the per-tile functions.
- **Fog signature fill** (`sc64_terrain_fill_now`). A 256-entry table
  gives the triples of two adjacent columns per lookup; the rows are summed
  through byte pointers (little-endian on ARM and on the host). Not used
  under `SC64_SOFT_FOG_SHADE_ONLY`.

`#error` guards reject the flag with `SC64_TILE_RENDERER`, without
`SC64_IMPORTED_MISSION`, and (for the fast key) without
`SC64_STATIC_KEY_CULL`.

## Exactness review

Each fast path was read against the code it replaces. No case was found
where the two can differ:

- The key's drawn bits and the reference's drawn fields start at zero, change
  on the same calls and, together with the identically handled payload
  fields, determine `changed`. Both sets of statics persist across mission
  restart and load in the same way; `generation` is a plain counter in both.
  Visibility is the same test as `currently_visible`.
- The split scroll test computes `dirty = edge | !same` and
  `part = edge & same` for tiles inside the old window and `1, 1` outside
  it, which is the reference expression term by term.
- The indexed draw answers are taken only when the lookup set
  `reused_base` and the draw window equals the view bounds that
  `sc64_terrain_tile_clean` derives; the snapshotted state
  (`reused_base`, `part_active`, the `now` window) is not written by
  anything the tile loop calls.
- The pair table is checked entry by entry against the single-column table
  in the host test. An odd last pair writes one unused triple inside the
  18-byte row; reads stop at the view width.

Product shapes checked by flag set: soft fog with scroll and no row masks
(Z10, Showcase), soft fog with row masks (Terran, P7–P10, Z6), hard fog
with a per-call key (Z7, Z8; P3 also has no key-once), hard fog with row
masks (Z4). The differential test has a variant for each shape except that
hard-fog scroll is tested with the key-once API only; the key API is
independent of the paths that changed.

## Reproduction

Inputs: a worktree with the private Z10 and Showcase inputs prepared under
`.local/` (here cloned from an earlier session's `.local` with `cp -Rc`).
Build each ROM from its own `arm-none-eabi-gcc` line as `make` prints it
(`make -n ... verify-zerg verify-showcase`, with `Z8_DESCRIPTOR` pointing at
the Z9 inputs), adding `-DSC64_TERRAIN_FAST_VALIDATE` for the candidate,
then `objcopy` and `tools/fix_rom.py` as the Makefile does. Without the flag
the rebuild is byte-identical to `make`'s ROM.

Fixtures (all private, not in the tree):

- **Z10 campaign tape.** `tests/sc64_z10_campaign_run.c` built unsanitized
  with the Z10 canonical flags; it wins at frame 59,997 (61,938 calls). Its
  digest matches the held-key tape recorded in
  `2026-11-13-z10-exact-home.md`.
- **Custom battles.** `tests/sc64_skirmish_campaign_run.c` built from each
  ROM's compile line, arguments `1 2 3` (Blood Bath, human Protoss, three
  CPU seats, free-for-all).

The replay driver is private local tooling built on the in-tree
`tools/sc64_headless_replay.c` (pinned mGBA 0.10.5 core) and the tape
validators in `tools/`. It starts each workload from the title screen with
blank SRAM, presses physical buttons only, acknowledges text boxes with
physical presses, never writes guest memory, and ends with a START/B/START
pause save. Per tape call it records the mGBA global-time delta between
successive input samples (the whole main-loop iteration, audio interrupts
and the present wait included). Windows: Z10 campaign calls 19,000–19,999
(A) and 35,000–35,999 (B), the Z10 custom battle 10,800–11,799 (C, busiest
1,000 calls) and 200–1,199 (Q, quiet opening), and the Showcase battle
10,800–11,799 (S). p95 is nearest rank. "Within budget" counts calls at or
below 561,792 cycles, two 280,896-cycle video frames (30 fps).

## Before and after

Fresh runs of the flag-off and flag-on ROMs (this audit), whole-update guest
cycles:

| Window | Build | Mean (x budget) | p95 (x budget) | Worst (x budget) | Within budget |
| --- | --- | ---: | ---: | ---: | ---: |
| A | off | 1,463,041 (2.60x) | 2,056,547 (3.66x) | 7,070,210 (12.59x) | 2 |
| A | on | 1,427,589 (2.54x) | 2,045,157 (3.64x) | 7,055,279 (12.56x) | 3 |
| B | off | 1,508,276 (2.68x) | 1,882,057 (3.35x) | 3,419,475 (6.09x) | 5 |
| B | on | 1,467,720 (2.61x) | 1,878,421 (3.34x) | 3,396,101 (6.05x) | 3 |
| C | off | 1,191,925 (2.12x) | 1,539,564 (2.74x) | 2,292,013 (4.08x) | 0 |
| C | on | 1,152,547 (2.05x) | 1,492,168 (2.66x) | 2,234,824 (3.98x) | 0 |
| Q | off | 562,088 (1.00x) | 589,608 (1.05x) | 905,301 (1.61x) | 516 |
| Q | on | 562,088 (1.00x) | 589,510 (1.05x) | 904,304 (1.61x) | 522 |
| S | off | 1,259,724 (2.24x) | 1,578,073 (2.81x) | 2,505,603 (4.46x) | 0 |
| S | on | 1,221,735 (2.17x) | 1,548,790 (2.76x) | 2,452,517 (4.37x) | 0 |

Mean savings: A 35,452 (2.4%), B 40,556 (2.7%), C 39,378 (3.3%),
S 37,989 (3.0%). Q sits on the two-frame pace, so its clock cannot move;
its map stage counter falls from 135,233 to 127,449. Game-update means are
unchanged within ±350 cycles (B 759,396 off, 759,161 on). The fit counts
move by a few calls either way and none of the busy windows comes close to
the budget.

## Memory and stack

| Link | `__iwram_end` off / on | EWRAM `__bss_end` off / on |
| --- | --- | --- |
| Z10 | 0x03004d70 / 0x03004d70 | 0x02038b18 / 0x020388e0 |
| Showcase | 0x03004ed0 / 0x03004ed0 | 0x02039c34 / 0x020399fc |
| Terran | 0x030048c8 / 0x030048c8 | 0x02039c70 / 0x020399c8 |
| Protoss P10 | 0x03004c80 / 0x03004c80 | 0x0203c0f8 / 0x0203bec0 |

All eight link inside the guards (`__iwram_end < 0x03005000`,
`__bss_end < 0x0203e000`). The IWRAM symbol set (names, sizes, types) is
identical with the flag on for Z10 (76 symbols) and Showcase (66); only the
order of ARM-to-Thumb veneers inside the same region changes. In ROM,
`sc64_terrain_fill_now`'s frame grows from 380 to 388 bytes (an 11-row
array of column pairs replaces the triple array) and `game_render`'s from 284 to
308 bytes.

Peak stack was measured by single-stepping every instruction of each whole
replay in a copy of the profiling runner and keeping the lowest SP per CPU
mode. Every call's clock, frame and screen equalled the unprofiled run.
The user stack starts at 0x03007e00.

| Lowest system-mode SP | Off | On |
| --- | --- | --- |
| Window B | 0x03007718 (1,768 bytes) | 0x03007718 (1,768) |
| Window C | 0x03007884 (1,404) | 0x0300786c (1,428) |
| Window S | 0x0300753c (2,244) | 0x0300753c (2,244) |
| Whole Z10 campaign replay | 0x030072e0 (2,848) | 0x030072e0 (2,848) |
| Whole Showcase replay | 0x030072d0 (2,864) | 0x030072d0 (2,864) |

The candidate is 24 bytes deeper in window C, the `game_render` frame
growth; the whole-replay peaks, reached outside the windows, are unchanged
and leave 9,584 (Z10) and 9,216 (Showcase) bytes above `__iwram_end`. The
interrupt stack low point (0x03007e78) is unchanged.

## Equivalence

Both builds replayed identical input. Every gated item matched: 43, 16
and 14 gameplay checkpoints (mission, screen, frame, won, lost, minerals,
gas, faults and digests of the raw unit, building and mineral arrays),
27, 18 and 9 video samples (both Mode-4 pages, palette, visible and
explored grids), tape, text-box and physical update counts, and the final
32 KiB SRAM. Two informational fields differ in the Z10 campaign: the
displayed-page bit, which depends on when the queued flip lands, and the
digest of the shaded fog-tile cache. That cache stores ROM addresses of
terrain art as tags and indexes slots by address; the candidate's ROM data
sits 0x7a0 bytes higher (`sc64_tile_pixels` at 0x09d0a5e0 instead of
0x09d09e40), so tags and slot choice differ while the drawn pixels do not.

A second run of each ROM produced byte-identical per-call logs and the same
results as the first, so the runs are deterministic.

## Terran and Protoss P10

Terran and P10 were built flag off and on from their own `make` compile
lines. No campaign replay fixture for them was available, so each product
replayed a new custom-battle tape (the same controller with that ROM's
compile line, arguments `1 2 3`; the human loses at frame 21,164 on Terran
and 21,216 on P10) for 12,000 calls. Both builds of each product matched all
15 checkpoints, 9 video samples, the final SRAM and the update counts.

| Calls 11,000–11,999 | Off mean | On mean | Off p95 | On p95 |
| --- | ---: | ---: | ---: | ---: |
| Terran custom | 1,088,179 | 1,056,396 | 1,381,654 | 1,344,290 |
| P10 custom | 1,355,357 | 1,320,042 | 1,730,136 | 1,687,328 |

Terran and P10 use row masks, so their same-camera test is the existing
row-mask path; the scrolled test, draw-pass answers, fog fill and key are
the fast ones. Retired Z7, Z8 and Z9 and Protoss P7–P9 were not built or
replayed for this audit.

## Host tests

- `make test-terrain-fast-validate test-terrain-fast-validate-cache` passes.
  The differential test compiles `tests/sc64_terrain_fast_validate_side.c`
  twice, flag off and on, as separate objects, and checks 30,000 random
  frames in each of 11 flag shapes: moving, jumping, edge and out-of-range
  cameras; scrolls of 1–8, 16 and 32 pixels in both axes; fog reveals and
  resets; mineral, building, geyser and doodad changes; mission and map-size
  changes, including maps 12 and 15 columns wide. Every frame both sides
  must agree on the key generation, hit/miss, dirty and partial cells, every
  draw-pass answer for the draw window and a random second window, the fog
  signatures and cache metadata. The cache variant rebuilds the existing
  terrain cache, scroll, key-once, soft-fog and dirty-capture oracles with
  the flag on. From clean outputs the pair takes 66 s (22 sanitized
  compiles, 11 runs each); with binaries built, 39 s. Both are in the
  default `test` target, so `./dev test` is about 40–65 s longer.
- With the flag added to their own compile lines, ten game-including render
  tests pass: `test-sc64-protoss-canonical-frame`,
  `test-sc64-protoss-canonical-hud`, `test-sc64-p3-static-key`,
  `test-sc64-z7-frame-coverage`, `test-sc64-static-dirty-rows-t4`,
  `test-sc64-static-dirty-rows-photon`, `test-sc64-terran-full-frame`,
  `test-sc64-terran-full-hud`, `test-sc64-z7-canonical-frame` and
  `test-sc64-zerg-canonical-hud`.
- `./dev build` passes. `python3 tools/check_source_tree.py` passes with
  the new files and this audit staged.
- `./dev test` fails in
  `test_sc64_photon_cannon_renderer` for the Terran and Protoss profiles
  ("missing sprite: unit raw 85"); `make test` stops at `test-starcraft`,
  so later targets did not run. The same test fails the same way on an
  export of unmodified `95a435c7` given APFS clones of the same private
  inputs, so the failure predates this change and comes from the prepared
  inputs, not from the flag. `./dev test` re-prepares private data; its
  rewrites left every file under `.local/sc64`, `.local/build` and
  `.local/walk-words-managed` byte-identical to the inputs the baseline was
  built from, and flag-off Z10 and Showcase rebuilt after it still match the
  baseline digests.

## Fixture provenance

The 12 November Z10 prefix save, its controller tape and runner were kept in
`/private/tmp` and are gone. The Z10 tape used here was regenerated: its
SHA-256 matches the held-key tape of `2026-11-13-z10-exact-home.md`, but it
replays from the title with a blank save, so windows A and B share the
12 November call numbers, not its game state. The custom and Showcase
tapes are new replacements; their scripted player's camera stays near its
own base, so the AI battle is mostly simulated off screen and those windows
stress simulation and terrain upkeep more than unit drawing. The same
fixtures were used for both builds.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Tested source change (`git diff HEAD` on `95a435c7`, before this audit) | `8b719dc847c850dbccdffd4f62beb8079887608b535f1b726b520dd1aa8b56ea` |
| Z10 ROM, flag off (equals the 18 November baseline) | `bd9a33e05c0eb63f3d283b8f64ccaab2eea49270da11cd148a992989501073cd` |
| Z10 ROM, flag on | `07928a481dcdd41d7b14998fbc6d48e6fe16b15eb31ec1076bdbd2a470eb82e0` |
| Showcase ROM, flag off (equals the baseline) | `4a5811473b9c323e3a776f4261ad67a948c5636dafe1f973789f21286752b984` |
| Showcase ROM, flag on | `86e083626d3641a902ff2f4acd60ea6b54ed6c9f7fd46bfb23d08335e0a9745d` |
| Terran ROM, off / on | `779dcda5432b1d5d128d085a49eb26a77dee66756e6664c645abaa51d66fc60a` / `a8437bc9251bae854f1c05cc036b719262968ca97cd00005eae1e3688594397f` |
| P10 ROM, off / on | `153d80f72c5184b745faf54f4c2edd1aa27805e427ea36aef1950a406c1ff14a` / `04e703ad953a4fdb8cf3f5cdfd9ebaf6485d7611af580280ba794f650ece7f69` |
| Z10 campaign tape | `bea076602f09b1c8496b8c3dfd5d49640be3a8352946763178f0ef40a8329310` |
| Z10 custom tape | `0c65c7b16b855f1b7ffa3153e4ae72d8ed6a48513c570dba805f0b778e56be51` |
| Showcase custom tape | `be92a3b4a623c03ca8da48cf2b349b62cccd9e60e562daaaeba408d8472fd5c1` |
| Terran / P10 custom tapes | `01d0307c011ef0587d1ff0932ecc0931778bafb729ef6263b217f062498a71e5` / `d8228c1f9c4579d92049b1b7644724de732c987bd060145143457cc59e09b16b` |
| Headless runner | `ff2c5a8b1f6764ed399d8a1b59df6ffde135ae2cd01a6d67500684b8429d23b7` |
| Final SRAM, Z10 campaign (both builds) | `c792f8ee5ebc9e7911e77333df722ba7c20f99a520bf87a8c5bcb044a40c8015` |
| Final SRAM, Z10 custom (both builds) | `ddec522d8f3039d833d554426b47223f897a2ae1cf9b71e2a5616ab6e85a069b` |
| Final SRAM, Showcase (both builds) | `4c68267636e435a5ca9993c540e68c8f690ea2de28758264fa726e5ba30ecc25` |
| Z10/Showcase comparison report | `bb057ec31109f2256501860beb679888a2fd29e72595bd7b8777df86bd19d308` |
| Terran/P10 comparison report | `3507c235c1eaa47956ae4c844d67761880a3b521fa1c4edeb7bba619a92f7679` |

## Limits

These are emulator guest cycles only; no hardware measurement was made.
The flag stays off by default until it is qualified across campaign,
custom games and Showcase on every product. Simulation alone (mean
`game_update` 759k in B, 696k in C and 761k in S) already exceeds the
561,792-cycle budget, so this change does not reach 30 fps in those
battles.
