# Mode 4 terrain cache padding

The shared Mode 4 renderer uses ten rows of video memory after the second
framebuffer for terrain and fog. The cache writes 2,400 bytes starting at
`0x06013600`. The text-box queue starts at `0x06013fe0`, 128 bytes past
the cache. The optional VRAM death pool owns the range in its build, so
that configuration continues to keep the ten rows in EWRAM. Each
transfer uses aligned words. OBJ tile VRAM still holds the first band.

## Busy Blood Bath

In the exact `29f74d85` Z10 cartridge, a fixed-camera inspection of the 61
EWRAM rows found no uniform row at 31 display cuts. Ignoring the viewport
border, the displayed pixels differed from terrain in a mean 32.81 rows
over a mean 36.29-row band. This is a displayed-frame sample. A selective
restore would need exact damage tracking for the page being reused,
including draw order, missed frames and camera changes.

The paired test runs 3,600 updates of the same ordinary three-CPU Blood
Bath save. The candidate keeps the full transfer and moves ten rows to
faster memory.

| Z10 guest cycles per update | Control | VRAM padding |
| --- | ---: | ---: |
| Mean | 842,472 | 840,021 |
| p95 | 1,134,841 | 1,132,275 |
| Peak | 3,714,007 | 3,711,437 |

Both indexed framebuffer pages, the palette, the world fingerprint and
route counters match at all 31 cuts and 16 paired fresh-load cuts. The
SRAM bytes match. Full-run route counter histories match. The 30 fps budget
for this measurement is 561,792 cycles per update. Physical timing remains
open.

| Receipt | SHA-256 |
| --- | --- |
| Displayed-row inspection | `1b3f0da0b539e2c763489fc8b0f78839a68c7a2b14412c6452dc099af3781a8d` |
| Busy Blood Bath comparison | `791b69c4091eea8b48303a409c6c733798a9c7a2e063295b2e76dcc74f1a9e3a` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Custom Game camera pan | `356b3078e1ebb4c9386f0bbd2de45b4c4bac8388fc8e967a7f77a47a1b3a1d59` |
| Custom Game HOME and SRAM matrix | `871a353cd3df3204d1289ef25a20434fa0cff3311c27d42f8e17d03044e0cbe7` |
| Protoss 10 late campaign and result | `a3b415f7fe44e399e585ebaa502d84152bc627c3c3293856cd1b4ffc02cc52f3` |
| Terran depth and selection fixture | `58eb6b98d06f1b27a129712d193b70d8a7029a1fcab87ac7d217810bb77acde4` |
| Showcase depth and death fixture | `0970530ec39164667c815967133cbeff8bcaa476d13bb03205b15bbc72fd0dca` |

## Shared build and renderer checks

All four ARM links pass the original RAM guards. EWRAM use falls by
2,400 bytes each. IWRAM endpoints are identical to the paired control.

| Product | EWRAM end | IWRAM end |
| --- | --- | --- |
| Terran | `0x0203a6dc` | `0x03004b40` |
| Z10 | `0x02039a68` | `0x03004ea8` |
| P10 | `0x0203b32c` | `0x03004f68` |
| Showcase | `0x02038d60` | `0x03004fc0` |

The three ordinary Campaign openings and the first hidden Showcase scene
match in world state, both indexed framebuffer pages, both HUD bands and
palette at frames 61 and 121. Each product also passes 512 host scene
comparisons against a fresh render, with construction, mineral depletion,
fog changes and both scroll directions across all camera phases.

In an ordinary-input Blood Bath pan, all 41 cuts match for each of
the four products. The check includes the world, camera, active page,
both indexed pages, palette, HUD bands and the text-box queue region.
Across the three race ROMs, all 54 normal Custom Game launches and
fresh SRAM continuations pass over both maps, all three human races
and one to three independently chosen CPU seats.

The retained Protoss 10 controller tape resumes at world update
36,761 and reaches a victory at update 46,057. All 68 comparisons and
the complete result SRAM match. Terran HOST depth and foreground
selection checks pass, including the Tank hull and turret on both
pages and the SCV on a foundation. Showcase passes five fallback death
overlap cases on both pages and the Siege Line source-art death case.

The asset-free dirty capture fixture crosses both storage boundaries and
restores to the other page. It exercises the ordinary three-band cache
and the two-band cache when the optional death-effect pool owns the
padding. The separate pool test covers its depth and color fields.
