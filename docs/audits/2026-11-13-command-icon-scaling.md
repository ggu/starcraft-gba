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

## Normal cartridge qualification

On the normal Z10 cartridge playing Z7 from HOME, the paired physical
mGBA replay matched all 39 named checkpoints through the test window
and 67 sampled states. The selected and previous frame pages, palette,
fog, exploration, HUD state and raw world fingerprints all match.
Four samples have the command menu open. Their mean whole-loop cost
drops from 3,109,827 to 2,396,722 guest cycles (22.93%); the mean scoped
HUD cost drops from 2,629,969 to 1,798,627 cycles (31.61%). Call 15,623
falls from 3,285,923 to 2,523,781 cycles. The remaining 63 samples are
effectively flat, averaging 1,317,485 and 1,318,076 cycles. A separate
signed-division cost remains outside the icon change. These are
emulated update costs over a bounded window, not GBA hardware FPS.

Both full Z10 three-CPU custom tapes reproduce their results and all
76 Outpost and 77 Blood Bath checkpoints; the measured 1,000-update
means differ by under 0.2%. The paired physical custom save/load and
60-update fresh-process continuation match on all four product ROMs,
including unchanged SRAM hashes and the resolved seats. All four
32 MiB links pass their unchanged RAM guards and ROM verification.
The asset-free default ROM and source boundary also pass.

The control was the previous qualified normal Z10 product; the source
changes between that control and this candidate include opt-in paths
left off for these runs. The exact candidate runtime is `cbba1c5`.
Paired Z7 receipt SHA-256 values:

- Control: `479ecdf90f34d1c603332c133cb15d283058d29645a4b5e8a7f1ef901802f268`.
- Candidate: `d38326cb92a5d48161c4e01eb0f43b717d6efce2a991f43feb3d00fa5fecadac`.
