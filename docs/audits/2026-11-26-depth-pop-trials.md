# Depth heap pop trials

The Z10 control is the normal #913 runtime at `82fc1a73` with
retained cartridge SHA-256
`2d1d00751fbc9e82a012823ec54fedda54dd3a55b0cf5d1a001be2de268fe7fe`.
An existing 20-update Blood Bath instruction cut attributes about
60,411 self and 237,008 inclusive cycles per update to
`sc64_draw_depth_ordered_actors`. A fresh eight-update moving
Outpost cut matches its unprofiled cycles, camera, both bitmap pages
and palette. It attributes 41,923 self and 162,210 inclusive
cycles to the same function. The generic and packed sprite blitters
consume 37,127 and 36,766 self cycles.

The first trial follows the smaller child to a leaf before repairing
the hole upward. The second runs the ordinary sift-down in IWRAM
ARM code with the current heap count in a local. Enable the second
trial with `-DSC64_DEPTH_ARM_POP_TRIAL` in the normal Z10 ARM
compiler arguments. The flag is off by default. A sanitizer oracle
checks 2,500 interleaved insert/extract sequences, including
duplicate painter keys and inserts after earlier draws.

| Workload | Control mean | Bottom-up mean | ARM mean | ARM interval p95 versus control |
| --- | ---: | ---: | ---: | ---: |
| Retained Blood Bath, 491 advancing updates | 920,546 | 923,532 | 918,733 | 1,266,060 versus 1,265,860 |
| Late Outpost camera sweep, 291 measured intervals | 976,244 | — | 973,681 | 1,340,216 versus 1,343,073 |

The late Outpost run starts from a physical pause save at world
frame 10,872 with 147 units and 59 buildings. The controls use
R+Right+Down for 120 updates, R+Down for 51, then 120 neutral
updates. Across 292 samples the camera takes 172 positions. Up to
63 unit anchors enter the viewport. At 158 samples at least two
anchors share an 8-pixel bin. This census indicates crowded
geometry and does not count only visible opaque pixels. The
endpoint, both Mode 4 pages, palette and logical work page match
throughout. The selected display page differs at one boundary
and agrees on the next. All 291 intervals exceed two VBlanks.

In Blood Bath the seven original checkpoints, endpoint and
both bitmap pages match across all 513 captures. The bottom-up
and ARM builds differ from the control at one and three
display-selection boundaries respectively. The next boundary
matches each time. The ARM link adds 168 bytes to IWRAM, with
a 0.20% Blood Bath saving and a 0.26% Outpost saving. Keep
the trial off by default.

| Private receipt | SHA-256 |
| --- | --- |
| Blood Bath bottom-up timing | `2b05970eba274a4839fa6d31a89480fdbc81ee593b120fccef53987d72461fe9` |
| Blood Bath ARM timing | `961bcd2cc5ffac27cca494132586fdba3ed291cb6e1153e00c5ed5255c3629e8` |
| Late Outpost control | `1831e90395efbfe7ee0a2982d59f51e229fdda43f7000abd81b8682bc62ab764` |
| Late Outpost ARM | `d3e1597f83ffdadb4ae4961f1436dd3b0a48a7b0bf953925e092712e7c2e3ccf` |
| Late Outpost instruction cut | `646a22a0568a6d0d02cab5a95af9e19c1030b689d074d5a2a7a289b0861d2fd3` |
