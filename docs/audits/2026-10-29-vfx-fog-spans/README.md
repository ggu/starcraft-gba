# Source VFX fog-span optimization

When a cropped source weapon frame straddles visible and hidden map tiles,
its rasterizer used to test each hidden pixel separately. Visibility is
constant within each 16-pixel map tile. Runtime `99bcb7a` advances to the next
tile boundary (or the source row end) after an in-bounds hidden-tile test.
Source columns, mirrored-row handling, visible-tile blits, destination LUTs,
world bounds and the scene clip are preserved. Out-of-map pixels keep their
previous behavior. No assets, animation events, lifetimes, palette values,
weapon values or gameplay rules change.

## Validation

The focused ASan/UBSan raster oracle passes across fog patterns, camera and
map edges, mirrors, source strengths and dark-mask dispatch. A hidden-raster
case also bounds visibility queries by tile spans. Default `./dev build` and
`./dev test` pass: 649 Python tests, 20 optional skips, eight separate profile
tests and the native fixtures. Source-boundary and whitespace checks pass.

Four full normal-product ARM links retain their existing memory ends:

| Product | IWRAM end | BSS end |
| --- | --- | --- |
| Terran | `0x03004df0` | `0x0203708c` |
| Zerg Z6 | `0x03004f40` | `0x020370f8` |
| Protoss P7 | `0x03004b70` | `0x020382b8` |
| Showcase | `0x03004fd0` | `0x0203df38` |

No reserve or guard is reduced. The change adds no persistent or derived
state, and does not change a unit or save layout. All 72 normal menu/skirmish
save continuations pass across both maps, all human races, 1–3 AIs and focused
three-AI compositions. Three actual ARM save runs restore in-place and in a
fresh isolated process, preserve the same SRAM bytes across restart, and
match host state from frame 9001 through 9901. All owned emulators terminated
and were waited on. The save harness's cache markers describe the existing
VFX duration cache, not this stateless raster change.

Actual Mode 4 image probes compare baseline main `2782067` against runtime
`99bcb7a`. Both baseline and candidate match the host's world records and
RGB555 checksums at every captured normal-menu Blood Bath checkpoint in all
three race products. The capture reads the displayed VRAM page and palette;
both page values were observed. This is exact sampled-image parity, not an
every-frame or hardware image comparison. Baseline source was archived from
the committed tree. Candidate include traces and source hashes identify the
new renderer; all proprietary prepared inputs stay external and unchanged.

## Measured scope and limits

The same probes retain frame timing outside image-capture work, excluding
two ticks after each report. Across 37,859 counted frames, total cycles fall
from 22,130,026,474 to 22,096,617,078. Weighted cycle-equivalent rates are:

| Profile | Baseline fps | Candidate fps | Slowest window, baseline → candidate |
| --- | ---: | ---: | ---: |
| Terran | 28.732 | 28.870 | 26.950 → 27.674 |
| Zerg Z6 | 28.660 | 28.660 | 28.028 → 28.028 |
| Protoss P7 | 28.722 | 28.734 | 24.667 → 24.795 |
| Aggregate | 28.702 | 28.745 | — |

The aggregate improvement is about 0.15%; most complete windows are
unchanged. The maximum measured frame remains 5,032,193 cycles. These are
fixed-camera unattended defeats against three mixed AIs, not campaign wins,
steady 30-fps coverage or measurements on a GBA SP. The slow Protoss window
still needs substantial work. This change neither enables nor qualifies a
mission; delivered scope remains T10/Z6/P7, with the accepted all-30-mission
goal unfinished.

Private evidence stays under `.local/build/vfx-fog-spans-release/`,
`vfx-fog-spans-rgb/` and `vfx-fog-spans-fresh-sram/`. All prepared-profile
builds use captured direct compiler arguments and existing absolute prepared
inputs; no imported Make recipes, extraction, Actions, GDB or SD writes were
used. No proprietary data, generated game assets, ROMs, saves, screenshots,
private harnesses or native decompilation are committed.
