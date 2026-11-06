# Delivered P10 death-art census

This is a read-only source and ROM census prompted by the real-device report
that death effects look very large. It is not a paired N64/GBA visual test and
does not establish that the delivered size is faithful or incorrect. No game
logic, assets or captures were changed.

The delivered Protoss 10 ROM and ELF matched the November 6 KODAK receipt:
ROM SHA-256 `206847219fc46ea92526e950e5b1f5623acf6a66360fb232eb08a9b8d956e1d6`,
ELF SHA-256 `852c6bb2966be97696d1bee6ab7621e035b2526b3cc4d49628ce78165cb49336`.
Its qualified runtime is `6ba76f8e`. The retained source-death header has
SHA-256 `09c43c6a26f3dcef6303dedf13118a432100fde05b81296c8ac788cf46dc11a6`,
matching the historical prepared-input receipt and extraction manifest. The
ROM contains source-death data and packed under-layer symbols; source-art
selection is enabled in the retained P10 compiler vector.

For the following seven unit identities and six representative building
identities, the ROM's source descriptors, layer IDs, positions and timing match
the retained manifest. All 10 referenced animation programs match their ROM
step frames, durations and loop metadata. All 18 distinct referenced images'
128 indexed frame pixel arrays (415,350 bytes) and frame geometries match the
ROM. `burst` and `total` are source-update counts; a long total can include a
remnant, not a continuing explosion.

| Actor (raw ID) | Source death layers | Burst / total | Max burst cropped width × height |
| --- | --- | --- | --- |
| Marine (0) | Collapse 239; remnant 241 | 17 / 166 | 30 × 19 |
| SCV (7) | Explosion 332 | 18 / 18 | 51 × 53 |
| Zergling (37) | Collapse 54; remnant 57 | 15 / 264 | 34 × 27 |
| Hydralisk (38) | Collapse 29; remnant 32 | 17 / 216 | 47 × 36 |
| Probe (64) | Explosion 213 | 18 / 18 | 51 × 53 |
| Zealot (65) | Collapse 151 | 14 / 14 | 28 × 34 |
| Dragoon (66) | Collapse 122; remnant 124 | 15 / 264 | 44 × 36 |
| Command Center (106), Barracks (111) | Explosion 334; remnant 336 | 28 / 723 | 116 × 91 |
| Hatchery (131) | Explosion 60; remnant 111 | 24 / 723 | 94 × 83 |
| Nexus (154), Gateway (160) | Explosion 215; remnant 217/216 | 28 / 723 | 116 × 91 |
| Pylon (156) | Explosion 215 | 28 / 28 | 116 × 91 |

The SCV and Probe explosions use distinct source image IDs but share a 64 × 64
canvas and the same maximum cropped dimensions. The Protoss building burst
uses a 126 × 100 canvas; a 116 × 91 cropped frame can cover much of the
240 × 108 battlefield. The source path blits these frames at one pixel per
stored pixel and clips to the battlefield. The 48-pixel effect extent cap is
for the procedural fallback; it does not resize valid source images. These raw
IDs have valid source entries, so an ordinary death with its mapped identity
selects source art rather than that fallback.

The delivered ROM's 17-direction idle-facing frame metadata gives live-body
cropped ranges: Marine 10–13 × 14–15, SCV 16–20 × 14–22, Zergling 10–16 ×
11–15, Hydralisk 14–19 × 19–23, Probe 12–16 × 8–14, Zealot 11–16 × 15–17,
and Dragoon 20 × 26 pixels. Moving and attacking frames can differ. In the
reviewed source, both the unit-facing importer and death importer crop and
remap source GRP pixels without an explicit resize, and both rendering paths
blit stored pixels 1:1. The fact that map coordinates use a different scale
therefore does not by itself prove that one importer omitted a body scaling
step. N64 runtime per-image scaling or presentation is not captured by this
asset census.

Next evidence should capture ordinary fresh N64 and GBA scenes for the same
Probe and SCV deaths, with body/effect bounding boxes and viewport calibration;
repeat for a Pylon or another representative building. Preserve native control
and lifecycle, and compare the real timeline and presentation before proposing
any resize. The table dimensions show why the report is plausible, but they do
not justify changing source art or effect timing on their own.
