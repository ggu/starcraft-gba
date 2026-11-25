# Source tracers in a fully fogged viewport

On the `9519d9ff` release in panned Protoss Outpost, all 264 sampled
raster calls intersect the scene clip, enter the fog fallback, and
paint no pixels. The visible grid has zero intersecting tiles at each
call. The updated bitmap drawer checks the scene once per draw, when
the first eligible tagged tracer reaches the painter. If no scene tile
is visible, it skips source painting after the existing slot and
duration preparation. The next draw queries the current view again.
The authored fallback, actor drawing and effects with separate draw
paths keep their existing handling.

| 96 advancing updates per pair | Control mean cycles | Updated mean cycles | Change |
| --- | ---: | ---: | ---: |
| Protoss 10, panned Outpost | 1,358,491 | 1,148,250 | −15.48% |
| Zerg 10, panned Outpost | 1,160,976 | 950,643 | −18.12% |
| Protoss 10, fixed Outpost | 1,227,892 | 1,227,236 | −0.05% |
| Zerg 9, opening technology | 836,779 | 836,803 | +0.003% |
| Zerg 9, road assault | 1,558,321 | 1,558,890 | +0.037% |

World, camera, both framebuffer pages, palette, object rows, tracer
rows and duration caches match in all five pairs. Pause journals and
fresh 60-update continuations match. One fixed P10 frame and one Z9
frame select a different display page. Every sampled advancing update
still exceeds the 561,792-cycle frame budget. The indexed animation
span experiment saves 0.91% in the panned P10 control and is excluded
from the updated build.

A separate 434-update P10 sweep enters fog and returns to the original
visible position, from 128 visible tiles to zero and back to 128.
Every update matches the control state, camera, viewport visibility,
both pages, palette, actor rows and tracer/cache rows. The 120-update
Terran Outpost pairs save 0.06% with matching world, pages and saves.
Three Showcase scenes match world and both pages. Scene 7 selects a
different page at five sampled boundaries. All 54 Custom Game opening
and fresh continuation cases pass on the three updated race ROMs.

An ASan/UBSan rectangle comparison passed 480,000 cases each for packed
and byte grids, including partial edge tiles, arbitrary map widths
and negative-camera fallback. All four ARM links pass their existing
memory guards. IWRAM ends are unchanged. EWRAM BSS grows by eight
aligned bytes in Terran and Protoss and stays the same in Zerg and
Showcase. Setting `SC64_DISABLE_EMPTY_TRACER_VIEW` in the otherwise
unchanged Protoss compile regenerates the control ROM byte for byte.

The first four Terran missions, Protoss 1 and 5, and Zerg 2 and 8 win
from HOME on the updated ROMs. Their sampled campaign states and
fresh result readbacks match earlier successful runs. Terran 1–4
and Protoss 5 also reproduce their fresh pause loads. Zerg 2 uses
the route that wins at frame 10,209 and checks the final and loaded
states. Zerg 8 matches all 36 checkpoints and wins at frame 86,969.
Its separate pause-route divergence from the prior release remains
open. Zerg 10 repeats its late save through victory at frame 63,523,
with the same input observations and identical fresh result readback.
The all-mission replay on one release trio and physical handheld
timing remain open.

| Local receipt | SHA-256 |
| --- | --- |
| P10 raster exit and visible-grid censuses | `ad6aab299358ba24b22d93dfafbba3bb80c5b5fa2bb874441803a4b62025ff7c` / `345a441484bf3fddd2837add2f8bced2935c9e462bce83e326e3788ad33d7852` |
| P10 panned control / updated frame and save samples | `380297631c0965ce0c4be00f0059791dbc9672f9e6f994a7629a77e1eeb1da74` / `df0b163919e4641b9889e8e9b5ce9f6a47a730a81c515326680a43bb2f24e330` |
| Z10 panned control / updated frame and save samples | `227f60f800eca61959ec556ac768ed6a63f1ddabe7752671ebe2dc1754aa8844` / `ca3117f4fff47fcd1ed15eb3f9c2900a9b7e59d5800d86e3a423957537223004` |
| P10 paired camera sweep | `3743cc6b32e17c3f7007cef11d79fb8b76e94cfd68d656dea953c7f8e8f4e9f3` |
| Terran and Showcase pairs | `ae49721946f5d647a8900e59911d2c66bb3d012219cfbe0b59a29e659e9fe391` / `2ccf4b3d49448a55967baf303d74d11bfa74cf723ac8b9ae3e5870c582641b5e` |
| Custom Game opening and fresh-load matrix | `01231672dcd2c024cff1e959b7718cd8c504a082a0f739c5f46eea3da494722b` |
| Terran 1 full run and Zerg 10 late win | `26c304ac8ca038b042c1191c2f4af6dad48c52bb7b2b5a2da52d4cabd0b12c08` / `9e1d9486d993f9276b40946d8d0a7de0488820b136b1d6ba0f3e6c21ea2fc2c0` |
| Terran 2, 3 and 4 full runs | `c88c9c0867e8245596832bc37e5b457381baae2d230021342c1747fec2209f27` / `5ecc24e8d777cec49b569eb27141b95b0f86cbffdd46dc7e2f8bffefd0dac422` / `4cf0873db2dd7e384c6b7ffb9803bd931103dd0d06385a0bbbd2cf17edb7f1e8` |
| Protoss 1 and 5 full runs | `89aa078b92c14a49b0adc9af02daf2c0d684f2928f0783b6855eae92ffad7700` / `dc7ac582d52e5852048015f4f15e7deba707a8ffc6a2441ba44649d53cf4b13e` |
| Zerg 2 full run | `ef72a9573654cab10a68d3d3f2f0423be7a65e664e9d0f2fe89636de3e527fb6` |
| Zerg 8 full run and fresh readback | `5d82f90fe1721e3e908e8ce2ca888db0040cee57894deee5166017507242ed0d` / `503a69301237f4d326964f74d76ffb0c53479b0b53428f2f7e4a3fd5778e8ed9` |
