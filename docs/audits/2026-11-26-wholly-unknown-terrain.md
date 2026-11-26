# Whole-viewport unknown terrain

The clipped scrolling Mode 4 renderer can fill the 240 × 108 terrain band with
one DMA transfer when no visible or explored tile intersects it. It uses the
same current fog bitmaps as the ordinary draw. A visible or explored cell at
even the last pixel of the viewport routes through the ordinary terrain cache.
The cached previous view stays available when the camera returns. World-depth
actors, effects, and HUD follow the ordinary path. Hardware tile rendering is
outside this code path.

## Paired cartridge measurements

The controls were rebuilt from release `51c7f18f` and matched the four packaged
ROMs byte for byte. The candidate changed only the unknown-view path. The
shared default build reproduced each measured candidate ROM byte for byte.
Headless mGBA drove normal physical inputs. Cycle intervals are between guest
input samples, including frame pacing. They are not hardware measurements.

| Cartridge and window | Calls | Mean cycles, control → candidate | p95 cycles, control → candidate | Max cycles, control → candidate |
| --- | ---: | ---: | ---: | ---: |
| Z10, late Outpost camera sweep out and back | 492 | 968,464 → 899,177 | 1,309,296 → 1,241,194 | 1,959,213 → 1,959,615 |
| Z10, stationary late Blood Bath | 256 | 867,176 → 868,179 | 1,326,333 → 1,327,701 | 1,610,771 → 1,612,270 |
| T10, fresh Outpost with three CPUs | 320 | 564,428 → 564,428 | 600,413 → 597,247 | 1,412,371 → 1,411,542 |
| P10, fresh Outpost with three CPUs | 320 | 562,673 → 561,794 | 609,184 → 601,413 | 706,792 → 705,511 |
| Showcase, fresh Outpost with three CPUs | 320 | 565,307 → 565,307 | 604,410 → 601,899 | 1,539,097 → 1,539,904 |
| T10, earned T5 assault journal | 160 | 577,648 → 577,649 | 681,968 → 681,355 | 761,456 → 760,934 |
| Z10, opening Z8 from an earned Z7 victory | 160 | 710,452 → 710,612 | 842,923 → 842,981 | 1,702,821 → 1,703,303 |
| P10, linked P10 opening from an earned P9 victory | 256 | 912,328 → 791,905 | 1,044,057 → 917,437 | 1,144,973 → 1,041,271 |

The Z10 Outpost sweep included 342 scrolled and 150 stationary calls. Mean
background time on those groups fell from 153,729 to 62,945 cycles and from
49,851 to 28,791 cycles respectively. All 492 full-loop cuts still exceeded
the 30 fps budget. The late Blood Bath camera stayed fixed with 59 viewport
fog changes. Its 0.12% mean increase measures the cost when the renderer
cannot take the shortcut.

The T5 holdout included 103 views with explored memory beyond current
visibility and no wholly unknown views. The P10 replay matched the published
launch and first progress checkpoints, ending at world frame 2241. All 256
measured P10 cuts had wholly unknown terrain. Their mean background work fell
from 143,448 to 22,464 cycles and every full loop still exceeded the 30 fps
budget.

Both raw indexed bitmap pages, palette, draw-page selection, camera, fog and
final world state matched in all eight paired windows. The display page bit
at the input breakpoint differed at seven cuts. At five cuts both indexed
pages already matched one another, so the displayed bytes were identical.
In the Blood Bath and fresh P10 Outpost pairs the other two sampled boundaries
showed adjacent completed frames and converged on the next input. The Mode 4
presenter can consume its queued flip from the VBlank interrupt independently
of the input breakpoint.

## Bounds and build checks

`make test-terrain-dark-view` runs the asset-free ASan/UBSan test. It covers the
256 x/y offsets inside a tile, alternating pages, both map bounds, partial
tiles, one currently visible or previously explored last tile, an explored
neighbor just outside the view, changed clip, a 15-column map, smaller maps,
and lookup of the prior cache after returning from wholly unknown terrain.

The three hot-IWRAM stack guards pass with identical linked hot bytes. Spare
space in the guard is 128 B for T10, 368 B for Z10 and 380 B for P10. The four
normal ROM verifiers accept the candidate's 32 MiB headers and memory
reserves. Use `SC64_TERRAIN_DARK_VIEW=0` in a fresh build for the previous
terrain path.
