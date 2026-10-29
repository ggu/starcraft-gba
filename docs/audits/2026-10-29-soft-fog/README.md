# Soft fog prototype

The mode-4 renderer at base `2dbb45c780f1638dbe2abd3ea616918d803a1c6d`
draws unexplored cells black and replaces explored cells outside current
sight with solid blue-gray rectangles. Both use the 16x16 gameplay grid.
This prototype retains that grid and draws shaded terrain in explored cells,
with authored, world-anchored edge masks inside known cells.

## Rendering contract

Fog classes are unknown, explored and visible; visible takes precedence.
The nine classes in a cell's 3x3 neighborhood fit exactly in a 16-bit
base-three signature. Out-of-map neighbors are unknown. Unknown cells paint
opaque black without reading terrain. Known cells sample only their own
terrain; neighboring terrain, actors and occupancy never affect the masks.

Three immutable 256-byte mask tables encode radial falloff and 4x4 ordered
dither. Their phase is cell-local, hence world-anchored because cell origins
are multiples of 16. Camera movement does not change the phase. Full visible
neighborhoods use the original fast blit. Other known cells compose into one
256-byte aligned stack buffer, then blit. All paths clip explicitly to the
world band `[12,120)`, including products without `GBA_SCENE_CLIP`.

The masks are an authored cosmetic approximation. They do not reproduce a
verified native N64 fog resource, change sight radius, reveal unknown cells,
or add higher-resolution simulation visibility.

## Palette and memory ownership

After the gameplay palette's LCD curve and UI overrides are installed, a
bounded integer search constructs a 256-entry shade map toward 60% channel
brightness. Nearest-color ties select the lowest palette index; the output
cannot increase weighted `3R+6G+B` luma. Drawing uses the prepared map rather
than repeating that search. Menu fades do not rebuild it; entering gameplay
and loading through the menu reinstall the gameplay palette and shade map.

`sc64_obj_vram.h` reserves the map after the optional projectile arena and
before the terrain pixel cache. Without P7 projectiles, its range is
`[0,256)`; with the 4,816-byte arena it is `[4816,5072)`. Writes use 128
halfword stores. Host fixtures share the same scratch backing across owners.
No shade map or new pointer is stored in GBA BSS. Normal VRAM-cache products
move one cached row to EWRAM; Showcase retains its existing RAM-only cache.

Cache entries retain exact 3x3 signatures. The row-mask fast path saves the
clipped one-cell halo in its existing row arrays (at most 18x10 cells).
An unchanged halo is a complete hit. Any change triggers exact signature
comparisons, including cells affected by off-view neighbors. Scroll reuse
compares the same world signatures. An 18-bit run starting at bit seven
requires four source bytes; the regression fixture covers that case.

Fresh normal ARM product builds pass the existing memory guards:

| Product | BSS increase | BSS headroom | IWRAM |
| --- | ---: | ---: | --- |
| Terran | 392 bytes | 28,088 bytes | unchanged |
| Zerg | 392 bytes | 27,980 bytes | unchanged |
| Protoss P7 | 392 bytes | 23,436 bytes | unchanged |
| Showcase | 152 bytes | 48 bytes | unchanged |

No reserve guard, actor limit, save layout or campaign catalog entry changes.

## Local checks and open gates

The default build remains byte-identical. `./dev build` and `./dev test`
pass, including 649 Python checks (20 skips), eight separate Python checks
and C fixtures. The focused checks also pass:

* `make test-sc64-fog`: all 19,683 neighborhoods against an independent
  direct-distance raster oracle, with and without scene clipping; HUD
  sentinels, partial cells, null/poisoned unknown sources and camera phase.
* `make test-terrain-soft-fog`: synthetic and actual-raster incremental versus
  fresh rendering, packed row masks, off-view edge/corner changes, scrolling,
  fog replacement, and VRAM-cache ownership with and without projectiles.
* `make test-gba-fog-palette`: independent palette oracle, deterministic ties,
  non-brightening, actual installed-palette preparation, shared-VRAM canaries,
  owner writes and palette regeneration.

The prepared-input `make unified` build and `make test-unified` aggregate
also pass in an isolated source copy with the fog option disabled. Two
legacy fixtures first failed identically on pristine base `2dbb45c`: one
spawned targets outside sight without refreshing exploration, and one
expected an AI to ignore a hostile AI unit in a free-for-all skirmish.
The corrected fixtures explicitly check hidden rejection before revealing
targets and retain the AI's independent acquisition policy. No targeting
implementation changes were needed.

Visual previews have been inspected, including a separately labeled
renderer-only fog fixture. Native-menu host matrices with the exact product
flags and timed ARM replay qualification are pending. The candidate is not ready for release until its visual result
and measured frame cost are reviewed. Full campaign, hardware and fresh-process
SRAM qualification are not established by these rendering tests.

## Rejected preliminary storage paths

The first prepared bank audit found no modifier-10 shade map in the actual
weapon VFX input, so relying on that bank would paint explored terrain black.
The palette-derived map removes that dependency without regenerating assets.
An intermediate proposal to use the final 256 bytes of the audio arena was
also rejected: the full Z/P profile permits 400 actors and its snapshot ends
at byte 2,400, overlapping that proposed reservation. Audio storage remains
unchanged; the explicit OBJ VRAM ownership map supplies the final location.

Private inputs, generated assets, captures, ROMs, logs and failed preparation
attempts stay local. No Actions or emulator was used for the checks above.
