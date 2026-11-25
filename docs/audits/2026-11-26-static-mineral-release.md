# Packed minerals on corrected Custom Game cartridges

The three static mineral frames use the existing packed sprite compositor by
default on the four race and Showcase cartridges. The generated word bank is
private, derived from the locally prepared Backwater sprite header at build
time; neither that header nor ROM data is committed. Set
`SC64_STATIC_MINERAL_WORDS=0` to build the byte-sprite control. The build
configuration fingerprints both the path and SHA-256 of the source sprite
header, so an older-timestamp replacement invalidates the derived bank.
The default, asset-free game does not depend on that private bank.

## Paired emulator measurements

The paired builds use source `c0ea7a36` from the gas work (#900), with
`SC64_SOURCE_GAS` enabled in both and only the mineral setting changed.
This evidence does not activate or include the gas work in this change. Each
pair loads the same retained three-computer save and runs 2,048 updates with
no buttons, then saves through the pause menu. Cycle counts are guest time in
mGBA, not hardware timing. Outpost and Blood Bath use their separate saved
worlds. Measurements include the full update and presentation waits.

| Cartridge | Map | Mean cycles, byte → packed | Change | Maximum single update, byte → packed | Pause-open cycles, byte → packed |
| --- | --- | ---: | ---: | ---: | ---: |
| Terran | Outpost | 774,959 → 760,490 | −1.867% | 2,185,017 → 2,170,730 | 1,853,933 → 1,711,780 |
| Terran | Blood Bath | 696,705 → 687,506 | −1.320% | 5,892,776 → 5,883,689 | 1,701,073 → 1,721,458 |
| Zerg Z10 | Outpost | 858,903 → 844,427 | −1.685% | 2,790,729 → 2,776,702 | 1,656,180 → 1,808,202 |
| Zerg Z10 | Blood Bath | 850,220 → 844,689 | −0.651% | 1,860,625 → 1,855,270 | 1,871,123 → 1,682,659 |
| Protoss P10 | Outpost | 942,453 → 927,963 | −1.537% | 2,863,263 → 2,849,520 | 1,861,764 → 1,761,800 |
| Protoss P10 | Blood Bath | 868,183 → 858,706 | −1.092% | 1,691,132 → 1,681,948 | 1,710,469 → 1,737,402 |
| Showcase | Outpost | 1,000,759 → 986,276 | −1.447% | 2,621,838 → 2,607,636 | 2,077,712 → 2,244,991 |
| Showcase | Blood Bath | 908,272 → 898,801 | −1.043% | 10,377,305 → 10,369,220 | 1,787,642 → 1,800,849 |

All 64 ordered samples per run match the world fingerprint, raw units and
buildings, minerals, gas depletion, both stored framebuffer pages and palette.
The resulting pause SRAM is byte-identical. Display selection also matches
except once in Protoss Blood Bath: at update 480 the byte build displays page
0 and the packed build page 1. The two stored pages and world match at that
boundary, but the displayed images differ. Every update from 451 through 550
was sampled in a second pass; update 480 was the only display-selection
difference. This is a presentation qualification gap, not exact displayed-frame
parity. Menu opening also costs more in four rows above; these short samples do
not explain the timing or bound the tails. Hardware pacing remains open.

ROM images remain 32 MiB. Across the four paired gas-enabled builds the linked
text/read-only total increases 23,656–23,664 bytes; `.data`, `.bss` and IWRAM
section sizes are unchanged. The four default product links also pass from a
prepared clean build directory. The prepared mineral wrapper test checks 400
full framebuffer pages across alignments, clipping and fallback; synthetic
packing and general packed-compositor sanitizer tests pass.

A read-only census of eight Showcase Outpost updates found 48 raw sprite calls:
32 mineral draws, eight geysers and eight other sprites. None of the 1,552
visible rows was fully transparent; 18,384 of 75,200 pixels were transparent
at row edges. This supports reusing the existing packed mineral bank rather
than adding a second metadata format. The measurements cover one retained
scene per map and do not establish a general performance bound.
