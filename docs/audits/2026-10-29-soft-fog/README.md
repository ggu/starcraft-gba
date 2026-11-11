# Soft fog: shaded explored terrain and softened boundaries

Status (2026-11-12): complete but not at the cost target, on `codex/soft-fog-finish-20261112`, which
supersedes draft [#146](https://github.com/ggu/starcraft-gba/pull/146). The
feature is on in the Terran, Zerg, Protoss and Showcase products through
`CANONICAL_FOG_FLAGS` (`-DSC64_SOFT_FOG`, defined in the top-level `Makefile`,
rules in `mk/fog/Makefile`). Its cost is stated below and is above the
1% target.

## Evidence class

The result is an **authored approximation**, not Nintendo 64 parity.

One observation from the reference N64 version supports the direction: in a
single frame at a mission start, captured in an emulator, unexplored terrain
is a flat dark purple of luminance about 11/255, and lit terrain fades into it
over a wide, smooth band with no hard tile edges. That supports softened
boundaries over 16x16 blocks. It does not establish the colors or band widths
used here, and whether the N64 draws explored-but-not-visible terrain
differently from unexplored terrain was **not** established by that frame. The
GBA unknown color stays the palette's black (not the N64's near-black purple),
the shade is a 60% brightness target and the edge widths and dither are
authored. Nothing here changes sight radius, what is explored or who can see
what.

## Before and after

The mode-4 renderer at base `2dbb45c` drew unexplored cells black and
explored-but-not-visible cells as solid blue-gray rectangles, both on the
16x16 gameplay grid. Now explored terrain is drawn through a palette-derived
shade map, and known cells get world-anchored dithered edge masks, so the
boundary between visible, explored and unknown terrain is a short gradient.
Unknown cells are still opaque black.

## Rendering contract

Fog classes are unknown, explored and visible; visible takes precedence. The
nine classes in a cell's 3x3 neighborhood fit exactly in a 16-bit base-three
signature. Out-of-map neighbors are unknown. Unknown cells paint opaque black
without reading terrain. Known cells sample only their own terrain;
neighboring terrain, actors and occupancy never affect the masks.

Three 256-bit maps per neighbor direction (768 bytes of ROM) encode radial
falloff and a 4x4 ordered dither. A cell ORs the maps of its unknown and
explored neighbors, then one pass over its pixels selects black, the shaded
terrain or the terrain. The phase is cell-local, hence world-anchored because
cell origins are multiples of 16; camera movement does not change it. Fully
visible neighborhoods use the original blit. All paths clip to the world band
`[12,120)` and to a narrower scene clip.

## Palette and OBJ VRAM ownership

After the gameplay palette's LCD curve and UI overrides are installed, a
bounded integer search builds a 256-entry shade map toward 60% channel
brightness (ties take the lowest index; the weighted `3R+6G+B` luma never
rises). Drawing reads the prepared map; menu fades do not rebuild it, entering
gameplay and loading through the menu reinstall the palette and the map. The
palette itself is not modified: the shade map is a separate table.

`starcraft/sc64_obj_vram.h` is the authority for the 16 KiB OBJ tile VRAM at
`0x06014000`. The shade map is 256 bytes placed between the optional P7
projectile arena and the terrain pixel cache:

| Owner | Without projectiles | With the 4,816-byte arena |
| --- | --- | --- |
| projectile arena | none | `[0, 4816)` |
| fog shade map | `[0, 256)` | `[4816, 5072)` |
| terrain cache rows | `[256, 16384)` (67 rows) | `[5072, 16384)` (47 rows) |

Everything else that lives in VRAM is below `0x06014000` and outside the map:
the audio arena and game-audio snapshot (from `0x06009600`), the optional
death-effect state (from `0x06013600`) and the text-box queue
(`0x06013FE0`). Overlap is prevented by construction (offsets derive from one
header; static assertions require the arena to end before the map, the map to
end before the cache and the cache to end inside the area), and by tests: the
host backs the whole area with one shared array, the terrain-cache fixtures
seed canaries in every owner's range and fail on any stray write, the palette
test checks that preparing the map touches only its 256 bytes, and the P7
layout test asserts the three ranges are disjoint. The tile renderer, which
owns all OBJ VRAM, still refuses the option at compile time.

## Memory (linker ends, fog off to on)

IWRAM end is unchanged in all four ROMs; no code was added there. All four pass
the linker guards (`__iwram_end < 0x03005000`, `__bss_end < 0x0203e000`) and
`verify_roms`.

| Product | `__iwram_end` | `__bss_end` base | `__bss_end` soft fog | change | EWRAM headroom left |
| --- | --- | --- | --- | ---: | ---: |
| Terran | `030044a8` | `0203748c` | `020399e8` | +9,564 | 17,944 |
| Zerg Z9 (16 slots) | `03004850` | `0203b1ac` | `0203c5e0` | +4,148 | 6,688 |
| Protoss P10 | `03004818` | `0203978c` | `0203bcf0` | +9,572 | 8,976 |
| Showcase | `03004a68` | `020372d0` | `0203982c` | +9,308 | 18,388 |

About 8.2 KB of the added EWRAM (4.1 KB on Zerg) is the shaded-tile cache (256
bytes per slot plus tags; `FOG_TILE_SLOTS` in the top-level `Makefile`, a power
of two, default 32; `FOG_TILE_SLOTS_ZERG`, default 16, because Egg activation
and paid Scarabs need Zerg's EWRAM); the rest is the per-frame signature
array (306 bytes), the fog fingerprint (88), the wider per-cell state (153
more), the partial-redraw flags (153) and, where the terrain cache shares VRAM
with the projectile arena, one cache row moved from VRAM to EWRAM (240).
On Zerg, 32 slots measured +3.08% (headroom 2,528 bytes) and 16 slots +3.30%
(1,421,950 against 1,376,574); the frames and the ARM page and palette hashes
are identical in both, only cost differs. VRAM use is
unchanged by this pass (the 256-byte shade map and the ranges above).

## Cost (ARM, pinned busiest 1,000 updates of the three-opponent custom battle)

Mean cycles per update on the mGBA core, base and fog-on ROMs built from the
same tree (default branch `22015eb2`; base has `CANONICAL_FOG_FLAGS=` empty),
replayed over the same ticks. The Protoss opponent changed with the Carrier
merge, so the Protoss window no longer matches the original pin
(`same_workload=False` against it); fog-off and fog-on were instead compared
with each other: identical tick, frame, screen and unit and building counts at
every row. Terran and Zerg still match the original pin.

| Product | loop mean, base | merged draft (older base) | first optimization | **this pass** | shade-only variant | simulation (`upd`) |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| Terran | 1,121,240 | +35.0% | +4.4% | 1,158,191 (**+3.3%**) | +2.5% | unchanged |
| Zerg | 1,376,574 | +31.8% | +4.1% | 1,421,950 (**+3.3%**, 16 slots) | +2.3% | unchanged |
| Protoss | 1,489,501 | +30.7% | +4.3% | 1,535,765 (**+3.1%**) | +2.0% | unchanged |

This is above the 1.5% aim, and shade-only (`-DSC64_SOFT_FOG_SHADE_ONLY`:
explored terrain shaded, no edge masks, kept as a measurement option) is also
above it. What this pass did, in cycles per update on Terran:

* Signature array: skipped unless the window or a fog bit changed (fingerprint
  of the halo row words), which cut it from +11.7k to +7.7k. It is not gone
  because the fingerprint itself reads two bit runs for each of ten rows every
  frame.
* Shaded-tile cache: a cell that is explored with no unknown neighbor is a
  whole shaded tile, so it is blitted like a visible one from a cache of shaded
  tiles, not recomposed per pixel.
* Cells clipped by the world band use a word-store row blit instead of the
  halfword generic blit.

The remaining floor is the work of shading pixels in ROM-resident Thumb code
(no IWRAM to spend) in frames where the camera scrolls (73% of updates here):
Terran after this pass is signature fingerprint +7.7k, strip dispatch and
decoding +6.8k, shaded-tile misses +5.6k, composing exposed edge strips +5.1k,
other blits +1.8k, plus 1.3k elsewhere. A fully visible tile is one word-store
blit; a shaded one needs a 256-entry lookup per pixel the first time it is
seen, and edge pixels need a per-pixel choice among black, shaded and plain.
Going further needs a change flag from the writer of the visibility grids
(so the fingerprint can be skipped) and a larger shaded-tile cache, which is
EWRAM Zerg does not have.

## Pixel evidence (host, private frames under `.local/`, never committed)

`tests/test_sc64_fog_frames.py` builds each race product twice (with and
without the feature), runs both custom maps and the first campaign mission
with five camera positions each, and compares the frames. The explored grid is
widened to a fixed shape after 40 updates so each frame mixes unknown,
explored-but-hidden and visible cells (injected state, not natural play); the
simulation is identical in both builds, and 600 further updates with a render
after each leave an identical state digest. Over 45 frames (a 240x108 world band is
25,920 pixels):

* Changed pixels: 330,550 in total (7,346 per frame on average, 0 to 16,058).
  296,235 are in explored-not-visible cells (94.9% of the 312,136 pixels of
  such cells; the rest map to themselves), and 34,315 are in visible cells
  that touch a non-visible neighbor.
* Unknown cells: 210,528 pixels, none changed, so they remain opaque black
  (apart from cursor and sprites drawn identically by both builds).
* Visible interior: 522,184 pixels of visible cells whose eight neighbors are
  all visible, none changed (bit-identical).

## Determinism and palette on ARM

With the feature on, each product was replayed twice in separate processes
from a fresh title screen through the pinned window (updates 10,887 to 11,886
Terran, 10,879 to 11,878 Zerg, 10,847 to 11,846 Protoss), hashing both mode-4
pages (`0x06000000`, `0x0600A000`) and the palette (`0x05000000`, 512 bytes)
every 10 updates, 100 samples each. All three products give identical hashes
in both runs at every sample. The palette is also identical to the base ROM's
at every sample: this branch changes no palette entry (the shade map is a
separate table in OBJ VRAM). The pages differ from the base ROM's in 72, 70
and 72 of the 100 samples (the rest show no fog boundary), as intended.

## Simulation identity

The custom AI harness (`tools/sc64_skirmish_ai_matrix.py --profiles
terran,zerg,protoss`, 330 cases, unsanitized `-O2` builds with and without the
feature) passes 334 of 334 for both (the matrix grew with the Carrier merge), and every
case's complete stdout (3.83 MB) is byte-identical. Two controllers per race (Terran T7 and T8, Zerg
Z8 and Z9, Protoss P8 and P10, 40,000 ticks each) give byte-identical
stdout and traces. These harnesses never call the renderer, so they show
that the flag changes no simulation code path; the frame test above covers
rendering's effect on state.

## Checks

* `make test-sc64-fog`: all 19,683 neighborhoods against an independent
  direct-distance raster oracle, with and without scene clipping, for aligned
  and odd-address terrain; HUD sentinels, partial cells, null/poisoned unknown
  sources and camera phase.
* `make test-terrain-soft-fog`: synthetic and real-raster incremental versus
  fresh rendering (including strip-only redraws), packed row masks, off-view
  edge and corner changes, scrolling, fog replacement, a direct check of the
  per-frame signature array on every map edge for byte and packed grids, and
  OBJ VRAM canaries with and without projectiles.
* `make test-gba-fog-palette`: independent palette oracle, deterministic ties,
  non-brightening, installed-palette preparation, shared-VRAM canaries and
  palette regeneration.
* `tests/test_sc64_fog_frames.py`: the pixel properties above; skips without
  the private inputs.

## Open gates

The cost target is not met (see above). Native previews and a hardware check
of the dither on a real LCD have not been done, nor has full campaign, SRAM or
hardware qualification. The edge masks and the 60% shade are authored; a
paired N64 observation of explored-but-not-visible terrain would settle
whether they should differ from unexplored terrain.

## Rejected storage paths

The first prepared bank audit found no modifier-10 shade map in the actual
weapon VFX input, so relying on that bank would paint explored terrain black;
the palette-derived map removes that dependency. A proposal to use the final
256 bytes of the audio arena was rejected: the full Z/P profile permits 400
actors and its snapshot ends at byte 2,400, overlapping that reservation.
