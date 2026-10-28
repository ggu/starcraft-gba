# Render caches in spare VRAM and on the second page

Status: enabled in the canonical Terran, Zerg (Z1-Z4 and Z1-Z6), Protoss
(P4 Storm, P5, P6 and P7) ROM flag sets through `CANONICAL_CACHE_FLAGS` and
`CANONICAL_MINIMAP_FLAGS`. Every option is default-off in the source; probes
and host fixtures that do not use those flag sets are unchanged. This is a
rendering and memory change only: no actor, order, save or trigger state is
touched, and the host pixel oracle matches the uncached reference frame for
frame. It is not a 30 fps or hardware claim.

## What changed

Four independent options, each exact by construction:

- `SC64_TERRAIN_CACHE_VRAM` keeps the first 68 rows of the saved terrain band
  in the OBJ tile VRAM at `0x06014000`, which mode-4 products never use
  because objects stay off. Only the remaining 40 rows stay in EWRAM. The
  band is written solely by 32-bit DMA (`gfx_read_band`), so VRAM's halfword
  store rule is respected, and the restore, scrolled restore and capture
  become two transfers over the same screen rows. VRAM-to-VRAM DMA moves a
  word in about half the cycles of EWRAM-to-VRAM.
- `SC64_HUD_VRAM_BANDS` drops the EWRAM copies of the top and bottom HUD
  bands in the reusable-band profiles. Each page keeps a content version per
  band and the clear count it was drawn at; a stale page copies the band
  from the page on screen (`gfx_copy_band_from_front`) when that page holds
  the current version and has not been cleared since, and redraws it
  otherwise. Pause panels and any minimap mark outside the 24x24 map
  invalidate the current page's bottom band exactly where the previous code
  marked it dirty.
- `SC64_MINIMAP_RASTER` maintains the 24x24 minimap colors at every
  visibility or exploration counter transition (cold, non-inlined helpers so
  the IWRAM callers do not grow). Drawing copies the raster opaquely with
  `gfx_blit_opaque` instead of recomposing 576 cells and blitting them with
  black as transparent. The black panel and its edge move into the cached
  bottom band, drawn after every HUD text so a long training line still ends
  under the panel as before; the opaque raster copy then replaces every pixel
  of the map area, including dots left on that page two frames earlier.
- `SC64_TERRAIN_ROW_MASKS` (packed-grid profiles) compares the fog state of a
  cached view row as one visible and one explored bit run read straight from
  the bit grids. Rows that match skip the per-tile loop; a complete hit
  returns without touching the per-tile dirty flags, which nothing reads
  after a hit. Rows that differ fall back to the exact per-tile comparison.

## Evidence

Host oracle (`tests/sc64_z3_frame_coverage.c` compiled with the full
canonical ROM flag sets): all 22 comparisons are pixel-identical for the
Zerg Z1-Z6, Protoss P7 and Terran profiles, covering cold and warm caches,
camera positions, fog, overlays, pause, transitions, save/load and restart
over poisoned framebuffer pages. The Zerg promoted-profile, split-run,
contact-cache, HUD-cache, minimap and terrain-cache fixtures pass.

ARM (mGBA, `tools/sc64_split_profile.py`, Zerg Z1-Z6 ROM, Blood Bath, Zerg
human, three opponents, 3,000 neutral ticks, static camera). Values are the
last tick of each 1,000-tick interval; the baseline is the same source with
the four options off. Simulation state at every checkpoint is identical.

| Tick | Update | Draw | Background | HUD | Unit draw |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 119,848 → 119,957 | 213,691 → 170,591 | 79,613 → 56,673 | 66,873 → 46,348 | 17,113 → 17,017 |
| 1000 | 123,687 → 124,693 | 231,252 → 188,100 | 79,836 → 56,916 | 62,629 → 48,303 | 37,979 → 31,639 |
| 2000 | 127,089 → 127,738 | 219,475 → 176,337 | 79,871 → 56,949 | 64,340 → 49,718 | 18,111 → 18,014 |
| 3000 | 136,721 → 136,801 | 242,158 → 198,781 | 86,038 → 56,977 | 65,613 → 45,148 | 38,933 → 38,839 |

Drawing a frame costs about 43,000 fewer cycles, 18-19 percent of the draw
stage and 7.7 percent of the 561,792-cycle 30 Hz budget. This opening already
ran at 29.0-29.5 fps because its frames fit in two hardware frames; the saving
matters in crowded scenes, where every frame still pays the same band restore
and minimap cost. A diagnostic build of the baseline attributed the
background stage to the band restore (52,400 cycles), the static-object key
(18,600) and the fog-mask check (9,700), and the minimap to the panel fill
(7,900), the raster recomposition and blit (22,600), building dots (5,200-
6,600), unit dots (9,500-11,000) and the camera frame (1,400). The key and
the dot loops are unchanged.

Memory, from the linked ELFs:

| Profile | BSS end before | BSS end after | IWRAM end before | IWRAM end after |
| --- | --- | --- | --- | --- |
| Zerg Z1-Z6 | 0x0203df70 | 0x02037198 | 0x03004ea8 | 0x03004e78 |
| Protoss P7 | 0x0203de70 | 0x02037090 | 0x03004b38 | 0x03004b00 |

EWRAM use falls by 28,120 bytes in each: the 16,320-byte VRAM terrain rows,
the 12,480 bytes of HUD band copies and 80 bytes of row masks, less the
576-byte minimap raster. The canonical Terran link ends BSS at `0x020371ac`
and IWRAM at `0x03004e80`. The new blit helpers run from ROM because the
canonical profiles had under 400 bytes of IWRAM reserve left; the Zerg and
Protoss IWRAM ends still fall by 48 and 56 bytes.

## Limits

The OBJ tile area is taken by `SC64_TILE_RENDERER`, so that probe cannot
enable `SC64_TERRAIN_CACHE_VRAM` and the header refuses the combination. The
terrain and HUD options need `SC64_NO_FRAME_CLEAR` with reusable bands to
matter; elsewhere they compile to the existing paths. Default-profile and
legacy Z3/P3 builds keep their previous flags and remain byte-identical.
