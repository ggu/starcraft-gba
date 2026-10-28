# Whole-visible source VFX rectangles

Runtime `d8bf5b8` submits a fully visible source frame as one contiguous
rectangle when its rows do not require reversal. The source pointer advances
by `(sy0 - top) * width` for vertical clipping; the submitted height is
`sy1 - sy0`, retaining the renderer's 12..120 scene band and the common
blitter's active clip. Existing mirror pixels are contiguous and eligible.
Rows needing the 256-byte reversal buffer and mixed-fog spans keep their
previous row paths. Modifiers 0/1 and mapped 9/10 retain the same dispatch,
palette data, transparency and source columns. No generated data changes.

The per-pixel oracle passes for fog, map/screen edges, vertical clipping,
mirrors and mapped dispatch. Focused ASan/UBSan and default build/tests pass:
649 Python tests, 20 optional skips, eight profile tests and native fixtures.
Source-boundary and whitespace checks pass. The original-art ROM remains
byte-identical. This change adds no RAM, unit fields or saved state.

All four normal ARM products retain the previous IWRAM/BSS ends:

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x0203708c` |
| Zerg Z6 | `0x03004f40` | `0x020370f8` |
| Protoss P7 | `0x03004b70` | `0x020382b8` |
| Showcase | `0x03004fd0` | `0x0203df38` |

All guards and reserves remain unchanged. The 72 ordinary menu/skirmish/save
cases pass across both maps, all human races, 1–3 AIs and focused three-AI
compositions. In all three race products, actual ARM in-place and fresh-process
SRAM restores preserve the SRAM bytes and match host continuation from frame
9001 to 9901. All owned emulator processes terminated and were waited on.
Cache markers refer to the existing duration cache; this rectangle path has
no cache.

The candidate also matches sampled actual Mode 4 RGB555 and world checkpoints
against the host in all three race products. Baseline main PR129 reuses the
already qualified `99bcb7a` pixel/timing probe and its immutable reports/ROMs;
the exact same probe template is compiled for the candidate. Image captures
are outside timing intervals, with two ticks excluded after reports. These
are sampled-frame comparisons, not an every-frame or hardware image proof.

Across the same 37,859 counted normal-menu Blood Bath frames against three
mixed AIs, cycle-equivalent rates are:

| Profile | Baseline fps | Candidate fps | Slowest window, baseline → candidate |
| --- | ---: | ---: | ---: |
| Terran | 28.870 | 28.941 | 27.674 → 28.058 |
| Zerg Z6 | 28.660 | 28.705 | 28.028 → 28.028 |
| Protoss P7 | 28.734 | 28.754 | 24.795 → 25.006 |
| Aggregate | 28.745 | 28.789 | — |

The gain is modest (about 0.15% aggregate). These fixed-camera unattended
defeats do not establish campaign completion, a frame-time ceiling, steady
30 fps or performance on the GBA SP. The accepted all-30-mission goal and
broader performance coverage remain unfinished; this change enables no
mission and leaves the delivered catalog T10/Z6/P7 unchanged.

Private reports stay under `.local/build/vfx-whole-raster-release/`,
`vfx-whole-raster-rgb/` and `vfx-whole-raster-fresh-sram/`. Prepared-profile
checks use captured direct compiler arguments and existing absolute prepared
inputs. No imported Make recipes, extraction, Actions, GDB or SD writes were
used. No assets, generated game data, ROMs, saves, images, private harnesses or
native decompilation are tracked.
