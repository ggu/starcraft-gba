# Shared rectangle borders and authored crowd timing

The shared rectangle outline now clips its side columns together. Each
visible side still uses a halfword read, mask and write to preserve the
neighboring pixel. Both horizontal edges use the existing rectangle fill.
The side pass lives in ROM and keeps the existing tile-buffer color and
dirty-mark handling.

## Paired ARM replays

The authored skirmish fixture initializes 76 actors across four owners
before the first input sample. It then runs the normal input, orders,
draw and display loop for 120 updates. The baseline uses source revision
`35781d0f`. In the second pair the actual D-pad input moves the camera
from x 392 to 428, then back. The pair also uses the ARM shape wrappers
and scene clipping.

| Workload | Mean draw cycles before → after | Mean frame cycles before → after | p95 frame cycles before → after |
| --- | ---: | ---: | ---: |
| Fixed camera | 455,539 → 450,048 | 886,775 → 877,413 | 1,123,171 → 1,079,518 |
| Panning with scene clip | 461,389 → 455,733 | 903,192 → 886,806 | 1,123,172 → 1,123,147 |

Mean frame cycles fall 1.06% and 1.81%. The sampled maxima rise by
104 and 22 cycles, respectively. All frames exceed the two-VBlank
budget of 561,792 cycles in both pairs. Every sampled actor, building,
mineral, tracer, vision, AI, RNG and camera digest matches, as do both
Mode 4 pages, the palette and display control. These are emulator
measurements from an authored skirmish fixture.

The sanitizer oracle compares 3,000 shapes against the prior
four-rectangle implementation, both as tile data and Mode 4
framebuffer data. The clipped cases include degenerate sizes,
overlapping halfwords and both screen pages. The existing shape and
scene-clip suites also pass. Building the common graphics object
with each of the Terran, Z10, P10 and Showcase flags leaves its
IWRAM section unchanged. The two rectangle-outline functions add
244 bytes of instruction text before linking. Race-cartridge and
physical frame-pacing qualification remain open.

## Reproduce the comparison

Use a headless mGBA runner built from the current
`tools/sc64_headless_replay.c` with the matching library headers.
The [runner build instructions](../research/sc64-headless-skirmish-replay.md)
show the compiler and library setup. The current runner reads both
Mode 4 pages, the palette and display control in physical-input mode.

```sh
python3 tools/sc64_authored_busy_arm.py \
  --baseline /path/to/baseline-checkout \
  --candidate . \
  --runner /path/to/sc64-headless-replay \
  --output build/crowd-comparison
```

Use a fresh output directory for each pair. Add `--arm-scene-clip --pan`
to replay the clipped pan. The command builds both ROMs from the
same authored fixture and writes per-update hashes and timing
receipts. A changed sample causes a nonzero exit. Both checkouts
build with the project's standard ARM tools and the authored
skirmish source.
