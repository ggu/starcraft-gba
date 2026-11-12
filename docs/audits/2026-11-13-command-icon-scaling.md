# Command icon scaling

The instruction profile of normal Z10 playing Z7 measured 3,286,416
guest cycles at call 15,623. The open command menu accounts for
2,164,120 of those cycles, including 1,070,811 in `ui_row_icon`.
Unsigned division alone accounts for 804,985 cycles in the update.
The source icon renderer divided for every sample in both the opacity
test and the draw.

The shared renderer now computes source column and row offsets once
for the imported icons that fit their 20-pixel limit, using 80 bytes of
stack storage and no persistent cache. An integer step
and remainder produce the same floor division at each coordinate.
The opacity test stops once it finds the four required opaque samples,
even when those samples are clipped. The draw still sends the same
nonzero samples to the same pixel compositor in the same order.
Larger valid frames retain the scalar path, and missing or sparse
icons still fall back to the caller's procedural art.

`make test-sc64-icons` runs 9,216 asset-free sanitizer comparisons.
The independent scalar sampler checks the return value, ordered
draw calls and the entire clipped page. Cases include all source shapes
from 1×1 to 20×20, downscaling and no scaling, an offscreen four-pixel
icon, invalid atlas ranges and oversized frames.

A paired ARM guest microbenchmark uses two 64 KiB synthetic cartridges
with the same shared pixel compositor. Four visible or partly clipped
icons (20×20, 20×16, 18×18 and 14×20, in 18-pixel slots) draw alongside
a sparse icon that requests fallback. On five updates each, the return
mask and all 38,400 indexed page bytes match. The timed icon batch
falls from 865,570 to 150,286 guest cycles (82.6%); the linker's IWRAM
and EWRAM end addresses are unchanged in these fixtures.

These are icon-only measurements. The normal Z7 command-menu window
and paired 64×64 custom render samples still need to confirm the
whole-game effect. The profile also found a separate signed-division
cost outside this change.
