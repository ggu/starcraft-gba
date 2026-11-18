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

`SC64_TERRAIN_FAST_VALIDATE` is a compile-time flag. It was introduced off by
default and in no product flag set, which is how the sections up to "Limits"
measured it; "Default-on qualification" below records turning it on for every
imported-mission build. With it off, every product compiles exactly the code it
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
At the time of these measurements the flag stayed off by default pending
qualification across campaign, custom games and Showcase on every product;
that qualification follows below. Simulation alone (mean
`game_update` 759k in B, 696k in C and 761k in S) already exceeds the
561,792-cycle budget, so this change does not reach 30 fps in those
battles.

## Default-on qualification

Before turning the flag on for every product, commit `77c1d641` (this branch
merged with source-release `b31cdd40`) was qualified by identical-input
replays of flag-off and flag-on builds of the Terran, Protoss P10, Zerg Z10 and
Showcase cartridges, and by a host differential that renders every game update.
No gated item differed anywhere.

### Builds

Each ROM was built from its own `make` compile line in a clean checkout of the
commit, and again with `-DSC64_TERRAIN_FAST_VALIDATE` added. The flag-off
rebuilds are byte-identical to `make`'s ROMs. All eight ROMs are also
byte-identical to the ones in the sections above, so the merge changed none of
these products and the earlier memory, stack and cycle figures apply unchanged.

| ROM | Flag off | Flag on |
| --- | --- | --- |
| Terran | `779dcda5432b1d5d…` | `a8437bc9251bae85…` |
| Protoss P10 | `153d80f72c5184b7…` | `04e703ad953a4fdb…` |
| Zerg Z10 | `bd9a33e05c0eb63f…` | `07928a481dcdd41d…` |
| Showcase | `4a5811473b9c323e…` | `86e083626d3641a9…` |

### Fixtures

No normal-menu tapes for these missions existed at this commit, so new ones
were made from the in-tree host mission controllers. Each controller was built
unsanitized with exactly its cartridge's flag set and a small private hook that
records the held keys of every `game_update` call and a checkpoint every 1,000
calls; the hook only reads state. T1–T5, P6, P10 and Z5 controllers enter
through the ordinary menus themselves. T6–T10 controllers start at the
mission, so their tapes were given the normal campaign-menu route in front and
shifted by the deploy call, as `tools/sc64_terran_controller_replay.py` does.
P6 and P10 used their HOME-tape builds with modal text boxes, so their tapes
carry the text acknowledgements; on Terran and Z10 the replay acknowledges
boxes outside the tape, as before. A fixed Showcase tape opens the hidden menu
and, for each of the eight scenes, rolls it, plays it, restarts it from the
pause menu or the result screen, plays it again and leaves.

Every controller tape replayed on the ARM cartridge matched all of its host
checkpoints (frame, screen, win, loss and wallet), and every mission tape ends
in the controller's win.

| Tape | Calls | Host checkpoints matched | SHA-256 |
| --- | ---: | ---: | --- |
| T1 | 10,151 | 11 | `f057da7d3bf4982d…` |
| T2 | 29,387 | 30 | `3736a6a11a1e4295…` |
| T3 | 54,007 | 55 | `28356d41fd63d248…` |
| T4 | 8,952 | 9 | `e9b505111c6777ed…` |
| T5 | 123,792 | 124 | `c1b6c9c2843c9a09…` |
| T6 (menu route added) | 23,860 | 24 | `290016a04945dc70…` |
| T7 (menu route added) | 4,200 | 5 | `9a31f9a23c07ab44…` |
| T8 (menu route added) | 143,273 | 144 | `b712f80c80e5f590…` |
| T9 (menu route added) | 47,997 | 48 | `ce6baf32f653afcd…` |
| T10 (menu route added) | 3,461 | 4 | `8c64d4644c12f59d…` |
| P10 | 47,194 | 48 | `3ef2618b78f6c28b…` |
| P6 | 101,370 | 102 | `305fba8ad3fd2b30…` |
| Z5 on the Z10 cartridge | 48,803 | 49 | `2f3ed27bc2d38b07…` |
| Showcase scenes | 11,186 | none | `265fc7322d3d7539…` |

The Z10 campaign, Z10 custom and Showcase custom workloads were re-run with the
fixtures and harness described above.

### What was compared

The replay driver is the private harness used above, extended with the menu
route, inserted pause-menu actions and a power-cycle step. Each arm starts a
fresh emulator process from the title with a blank save and presses physical
buttons only. The gate covers:

- a gameplay checkpoint every 1,000 calls: mission, screen, frame, win, loss,
  wallet, faults and digests of the unit, building and mineral arrays;
- a video sample every 200 calls (50 in the Showcase scenes), and at +0, 1, 2,
  3, 5, 10, 30, 60, 125 and 250 calls after every load, restart or save: both
  Mode-4 pages, the palette and the visible and explored grids;
- every save image, the per-call frame and screen sequence, and the outcome of
  each inserted action.

### Save, load and restart

- **Fresh process (P10).** The tape saves from the pause menu at call 37,700,
  after returning unsaved UI state to its load-time form. The first process
  stops at call 37,704 and exports the save, which is identical in both arms.
  A new process with that save presses L on the title in place of the
  unpausing START, which restores play at frame 36,761, and the tape continues
  to the win. Off and on matched on 51 checkpoints, 246 video samples and both
  save images. Both arms also matched every later host checkpoint.
- **Restart and load in one process (T4, P10).** In Jacobs Installation, a
  pause save at call 4,000, a restart at 5,000 and a load of that save at 6,500
  were added, and the tape then ran on to call 8,952. In P10, a restart at
  call 39,000 and a load of the tape's own save at 40,500 were added, running
  to call 42,000. Every checkpoint and video sample matched, including those
  right after each action.
- **Showcase.** Each scene is restarted once and replayed.

### Results

Whole-update guest cycles. For each workload the window is the 1,000
consecutive in-play calls with the highest flag-off mean. That window is used
unchanged for the flag-on run.

| Workload | Window | Off mean | On mean | Off p95 | On p95 | Off worst | On worst |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| T1 | 6,236–7,235 | 604,271 | 601,742 | 700,113 | 673,271 | 23,409,462 | 23,383,391 |
| T2 | 28,387–29,386 | 818,525 | 814,283 | 903,353 | 901,533 | 2,174,610 | 2,175,916 |
| T3 | 50,725–51,724 | 1,003,631 | 1,003,289 | 1,187,466 | 1,187,007 | 1,705,594 | 1,704,618 |
| T4 | 133–1,132 | 915,354 | 897,715 | 1,413,576 | 1,378,816 | 6,861,783 | 6,852,519 |
| T5 | 24,089–25,088 | 1,171,874 | 1,167,405 | 2,051,499 | 2,051,265 | 2,211,899 | 2,215,035 |
| T6 | 11–1,010 | 1,224,579 | 1,197,532 | 1,449,679 | 1,403,016 | 9,982,137 | 9,962,053 |
| T7 | 3,201–4,200 | 1,006,733 | 981,876 | 1,880,630 | 1,881,695 | 2,823,918 | 2,810,298 |
| T8 | 98,293–99,292 | 1,291,304 | 1,288,199 | 1,970,609 | 1,959,207 | 2,390,459 | 2,390,576 |
| T9 | 35,956–36,955 | 1,296,878 | 1,268,037 | 1,571,517 | 1,551,304 | 2,010,060 | 1,980,046 |
| T10 | 15–1,014 | 999,317 | 993,146 | 1,581,294 | 1,580,824 | 9,958,487 | 9,939,294 |
| P10 | 43,948–44,947 | 2,201,471 | 2,162,243 | 2,949,901 | 2,928,778 | 3,717,640 | 3,691,930 |
| P6 | 33–1,032 | 1,912,352 | 1,885,485 | 2,088,335 | 2,048,568 | 10,176,114 | 10,156,985 |
| Z5 (Z10 cartridge) | 217–1,216 | 1,213,039 | 1,187,769 | 1,883,876 | 1,875,568 | 2,411,511 | 2,401,912 |
| Showcase scene 0 | 72–1,071 | 1,093,718 | 1,086,504 | 1,392,339 | 1,385,185 | 1,892,574 | 1,886,039 |
| Showcase scene 5 | 5,700–6,699 | 835,300 | 827,795 | 1,458,861 | 1,451,897 | 1,591,183 | 1,553,065 |

The re-runs of the Z10 campaign windows A, B and E, the Z10 custom windows C and
Q and the Showcase custom window S gave exactly the figures in "Before and
after", with the same equivalence results. The flag-on mean is lower in every
window, by 0.03% (T3) to 3.3% (custom window C). Only T1, T7, the quieter Showcase
scenes and the quiet custom window Q have a meaningful share of calls within
the 561,792-cycle budget.

Across all workloads, 862 gameplay checkpoints, 4,066 video samples and 20 save
images per arm were compared, over 815,377 tape calls per arm, and all
matched. Two informational fields differed, as before: the displayed-page bit
in some samples and the fog-tile-cache digest, whose tags are ROM addresses.

### Host differential

Every mission controller (T1–T10 with Terran flags, Z1–Z10 with Z10 flags,
P1–P10 with P10 flags) was built twice with rendering added after each
`game_update`, in the main loop's order (`audio_update`, then `game_render`),
once without the flag and once with it. So was the custom-game controller for
72 lobbies: the Terran, P10, Z10 and Showcase flag sets, both maps, each human
race and one to three CPU opponents, up to 20,000 calls each. Each update's
digest covers both framebuffer pages, the palette and the visible and explored
grids. All 102 pairs produced identical digest streams and identical input
tapes, over 2,777,905 rendered updates per arm: 449,017 Terran, 539,509 Zerg,
654,575 Protoss and 1,134,804 custom. Every rendered tape also equalled the
controller's tape without rendering. All missions end in a win except Z7, whose
controller loses at call 16,910 under the Z10 flags.

### Wiring and build matrix

`mk/terrain-fast-validate/Makefile` defines `TERRAIN_FAST_VALIDATE ?= 1` and a
value `TFV` that is ` -DSC64_TERRAIN_FAST_VALIDATE` unless the variable is 0.
The top-level Makefile appends `$(TFV)` to `SC64_FLAGS` and `BACKWATER_FLAGS`,
the roots of every imported-mission flag set, and filters it out of
`P7_TILE_FLAGS`, the one product set with `SC64_TILE_RENDERER` (the tile
renderer has its own terrain path, and the `#error` guard stays). With
`make TERRAIN_FAST_VALIDATE=0` every compile line equals the earlier one.

Of 170 ARM targets (158 `.gba` rules and 12 ELF-only probes), 164 carry the
flag. The six that do not are the P7 tile probe and five targets with no
imported-mission cache: `StarCraft-GBA.gba`, `StarCraft-GBA-Ring.gba`,
`probe.gba` and the two typed-unit benches. On `77c1d641` plus the wiring, 135
targets had the private inputs to build. For each of them:

- the `TERRAIN_FAST_VALIDATE=0` build is byte-identical to a build of the
  unmodified commit (ROM SHA-256, or the `objcopy` image for ELF-only probes);
- with the flag on, `__iwram_end` and the IWRAM symbol set (names, sizes,
  types) are unchanged, and the EWRAM end falls by 432–680 bytes, or stays
  where the product has no `SC64_STATIC_KEY_CULL` (Z3, Races, Skirmish,
  Backwater, unified).

The tightest links with the flag on are 248 bytes of IWRAM (Z6 and the
Showcase melee-starts probe) and 812 bytes of EWRAM (the Z7 steering probe).
Nine targets already exceed the IWRAM guard on the unmodified commit and still
do, by the same amount with the flag on or off: retired Z7 (64 bytes over,
`update_orders` in IWRAM), the Z7 egg preview, P4 Storm, P5 Shuttle, the P7
contact, fact, move and tile probes and the Zerg impact probe. Relinked with a
relaxed guard, their IWRAM sets are identical off and on. Eighteen targets
could not be built for missing private inputs (the five Nuke probes, the P6
ROM, the Z7 ARM replays, the T10 impact replay, `verify-campaign`, Wasteland
and the six Z9-carried Z7/Z8 ROMs).

`StarCraft64-Skirmish-GBA.gba` has no soft fog, scrolled cache, key-once or
culled key, so the flag changes only the draw-pass tile answers there
(`game_render` grows by 304 bytes; RAM ends are unchanged). No test in the
tree renders that product, so a private host harness ran 6,000 scripted
frames with `game_render` after every update: flag off and on produced
identical per-frame hashes (2,282 distinct frames, 1,234 camera positions).

Host suites with the flag on by default: `./dev build`, `./dev sc64 test
--profile terran`, `test-terran`, `test-zerg`, the Showcase suites, the render
group (canonical render, T4/Z6/Z7/P6 frame coverage, static dirty rows,
spatial HUD, fog, the terrain-cache family) and `test-sc64-p7-tile` pass. The
failures below were each reproduced with `TERRAIN_FAST_VALIDATE=0` and on the
unmodified commit, so none comes from the flag: two cases of
`test_sc64_photon_cannon_renderer` in `./dev test`; `test-protoss` (missing P6
sprite input, and `test-sc64-protoss-canonical-p7-save` aborting at
`sc64_save_candidate_fixture.h:73`); `test-sc64-p3-scourge`
(`sc64_p3_scourge.c:82`); `test-unified-engine` and `test-unified-factions`;
and `test-skirmish-match`.

### Re-check on the merge base

Source-release moved to `ca822ea7` during qualification, with changes to
shared SC64 play, the race-build menu flow and Skirmish, so its ROMs are no
longer the ones replayed above. The flag's own files are unchanged there. On
`ca822ea7` plus the wiring commit:

- All 135 targets build and link with the flag on; none fails a guard that it
  passed before.
- Terran, Z10, P10, Showcase and Skirmish built with
  `TERRAIN_FAST_VALIDATE=0` are byte-identical to builds of unmodified
  `ca822ea7`. With the flag on, IWRAM ends and symbol sets are unchanged.
- The existing tapes still replay without divergence. Flag off and on matched
  on every gated item: Z10 campaign (43 checkpoints, 27 video samples, final
  save), Z10 custom (16, 18), Showcase custom (14, 9), Terran custom (15, 9)
  and P10 custom (15, 9).
- The asset-free differential test, the cache oracles, the render group, the
  Skirmish render harness and `./dev build` pass.

| Window on `ca822ea7` | Off mean | On mean | Off p95 | On p95 | Off worst | On worst |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Z10 campaign 19,000–19,999 | 1,497,825 | 1,461,275 | 2,055,465 | 2,047,054 | 7,108,092 | 7,095,180 |
| Z10 campaign 35,000–35,999 | 1,549,476 | 1,508,980 | 1,899,064 | 1,883,422 | 3,455,932 | 3,429,779 |
| Z10 custom battle | 1,222,650 | 1,183,129 | 1,567,557 | 1,522,231 | 2,320,156 | 2,270,375 |
| Z10 quiet scene | 562,088 | 562,088 | 589,627 | 589,639 | 918,566 | 911,293 |
| Showcase custom battle | 1,252,645 | 1,214,593 | 1,577,355 | 1,540,505 | 2,504,366 | 2,457,735 |
| Terran custom 11,000–11,999 | 1,081,496 | 1,049,732 | 1,374,849 | 1,333,813 | 2,204,243 | 2,154,737 |
| P10 custom 11,000–11,999 | 1,350,636 | 1,315,296 | 1,724,879 | 1,681,511 | 2,694,586 | 2,653,789 |

| ROM on `ca822ea7` | Flag off (equals unmodified) | Flag on (default) |
| --- | --- | --- |
| Terran | `b66a5d34603b58cc…` | `4346f3083be75b3d…` |
| Zerg Z10 | `cd638f0330749574…` | `17096c15560c0cb2…` |
| Protoss P10 | `5a0811e957a77d1f…` | `9cd4f73288da1354…` |
| Showcase | `d9f0fca14b1c292f…` | `39c3d4f5b5563ab9…` |
| Skirmish | `7c98bdae941c64be…` | `7b4ec71364100e1b…` |

The campaign-mission replays, save/load and restart replays and the host
differential above were not repeated on `ca822ea7`.

### Remaining limits

- Only these four cartridges were replayed on the emulator with the flag.
  Every other target was built and linked (see "Wiring and build matrix")
  but not replayed. The default original-art ROM does not use this cache and
  does not carry the flag.
- On the ARM cartridge, Zerg is covered by Z5 and Z10, and Protoss by P6 and
  P10. The other sixteen Zerg and Protoss missions are covered only by the host
  differential.
- No fresh-process load was replayed on the Z10 or Terran cartridges.
- These are emulator guest cycles; no hardware measurement was made.

| Evidence | SHA-256 |
| --- | --- |
| Headless runner | `ff2c5a8b1f6764ed399d8a1b59df6ffde135ae2cd01a6d67500684b8429d23b7` |
| P10 save image at the power cycle (both builds) | `db0a7afa244e97912d0e06fdf3b80600b02750e03ab34b4b9511af378b7f8569` |
| P10 final save after the fresh-process load (both builds) | `c69ff8edddd990e94f3f9418e5b2198cb47e649a555afb36aca5d6f54de58cbf` |
