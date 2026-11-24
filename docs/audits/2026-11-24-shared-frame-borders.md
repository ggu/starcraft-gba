# Shared Mode 4 drawing and authored crowd timing

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

Extending the panned pair to 600 updates retains matching state and
display cuts. The mean falls from 631,696 to 627,942 cycles, and p95
falls from 1,076,127 to 917,265. The number of frames above two
VBlanks drops from 147 to 142 as the initial battle settles.

The initial camera in these pairs starts south of much of the fighting.
A second 600-update panned replay centers the camera closer to the
fighting at world y 420. All sampled state and display cuts match.
Mean frame cycles fall from 672,914 to 670,564, p95 stays within
three cycles, and updates over two VBlanks fall from 196 to 176.
Mean drawing costs 476,353 versus 470,576 cycles.

The closer-baseline replay changes an average of 13.95 of the 105
battlefield tiles between successive uses of the same display page
at unchanged camera coordinates, across 82 early comparisons.
During the 36 camera-change comparisons it changes 60.44 tiles on
average. These are indexed-pixel observations covering display rows
12–119. Reuse still needs proof at the level of the original painter's
ordered contributors, and should include both camera patterns.

The sanitizer oracle compares 3,000 shapes against the prior
four-rectangle implementation, both as tile data and Mode 4
framebuffer data. The clipped cases include degenerate sizes,
overlapping halfwords and both screen pages. The existing shape and
scene-clip suites also pass. Building the common graphics object
with each of the Terran, Z10, P10 and Showcase flags leaves its
IWRAM section unchanged. The two rectangle-outline functions add
244 bytes of instruction text before linking. Race-cartridge and
physical frame-pacing qualification remain open.

## Full-width fills and glyph rows

Read-only ARM probes match the panned baseline's draw time and both
display pages at eight updates. The probes attribute an average of
99,145 cycles to rectangle fills and 42,866 to text. A fully
visible HUD fill costs 3,696 cycles for 12 rows or 11,769 for
39 rows. Clipped full-width terrain also accounts for recurring
fills in the authored workload.

The full-width row API transfers aligned Mode 4 bands with synchronous
DMA3. It uses the shared rectangle painter for host and tile-buffer
draws. The common rectangle path keeps its original code and IWRAM
footprint. The ARM pixel oracle covers 400 whole-frame comparisons
across both pages, including zero-height, empty clip, negative origin,
clipped height and bottom-edge fills.

An in-bounds 5-by-7 glyph establishes its clip and halfword alignment
once and draws two pixel pairs per row. Space advances without writing
pixels. Partial glyphs keep the existing row painter in ROM. The
sanitizer clipping test and a host pixel comparison cover both pages,
glyph parity, ASCII, clipped positions and multiline strings.

| Combined workload | Baseline → candidate mean draw cycles | Baseline → candidate mean frame cycles | Frames above two VBlanks |
| --- | ---: | ---: | ---: |
| Fixed camera, 120 updates | 455,539 → 427,229 | 886,775 → 849,332 | 120 → 119 |
| Battle in view with pan and scene clip, 600 updates | 476,353 → 438,942 | 672,914 → 660,677 | 196 → 163 |

Both comparisons match simulation digests, cameras, palette, display
control and both video pages at every update. End-to-end mean falls
4.22% and 1.82% respectively. The visible pair retains its broad p95
at 1,123,172 cycles and increases the peak by 729 cycles. Its drawing
p95 falls from 597,906 to 560,441 cycles. The isolated row and glyph
changes each reduce mean draw cost before combination.

The recorded Terran, Z10, P10 and Showcase flags pass ARM syntax
with their prepared inputs. Their graphics objects each use 76
fewer IWRAM bytes than the source baseline. This checks the shared
code placement at object scope. Full cartridge links, release
gameplay and real-device pacing remain open.

## Finished-band reuse trial

The authored trial captures ordered drawing commands and keeps a
separate history for each physical page. It compares the commands
affecting each of seven full-width battlefield bands. A changed band
is cleared and redrawn in original order. The comparison considers
both histories, so a disappearing command dirties its old band.

The 600-update visible panned pair matches both video pages, palette
and sampled state. The trial reuses 3,341 of 4,200 bands and records
up to 345 commands per update. Mean whole-frame time rises from
659,276 to 729,159 cycles, with p95 rising from 1,123,171 to 1,389,351.
Command capture alone averages 55,981 cycles outside the candidate's
draw counter. A forced post-capture fallback disagrees on 13 displayed
updates despite matching state and palette. The trial stays out of
the shared renderer. A later attempt needs a cheaper way to identify
changed contributors and a verified fallback.

## Clipped terrain rows and solid lines

An authored ARM fixture covering all horizontal phases shows that
aligned sources at phase 2 can draw 100 clipped five-row cells in
67,520 cycles versus 211,920. The retained `1f0ad66e` P10 cartridge
has no clipped-row calls in the two fixed-camera samples. Holding
Right for the same two 12-update windows produces 12 clipped-row
calls. Eight are clipped horizontally and the other four have phase
0. The camera advances four pixels at a time from phase 0 in the
measured runs. The stage census identifies no effective branch hits
for phase 2 there.

Direct halfword writes for a wholly visible solid line match the
reference on both pages across the authored 600-update panned pair.
They save 347 mean draw cycles per update. Mean whole-frame time
stays at 660,677 cycles with 163 updates above two VBlanks. Keep
both trials local and prioritize the larger movement and painting
work. Additional camera positions may exercise other row phases.

## Paired frame loops after the packed-sprite merge

Source release `fc17165b` includes the packed-sprite row change and
the current shared simulation. It is the baseline for both replay
pairs below. The same authored setup and normal frame loop drive
both builds. All 720 sampled updates match on the simulation digest,
camera, both framebuffer pages, palette and display control.

| Workload | Mean frame cycles before → after | Mean draw cycles before → after | Two-VBlank misses before → after |
| --- | ---: | ---: | ---: |
| Fixed camera, 120 updates | 652,700 → 615,256 | 284,137 → 255,060 | 70 → 62 |
| Visible pan, 600 updates | 609,143 → 603,527 | 297,605 → 260,105 | 115 → 101 |

The whole-frame means fall 5.74% and 0.92%, respectively, and
mean drawing cost falls 10.23% and 12.60%. The panned p95 stays
within four cycles. Linked IWRAM falls by 88 bytes in the fixed
fixture and 64 in the panned fixture. Linked data and BSS match
within each pair. The fixed pair's latest peak rises by 1,173
cycles. The fixture still misses the two-VBlank budget during
busy play. Race-cartridge and GBA SP timing remain open.

| Local paired receipt | SHA-256 |
| --- | --- |
| Fixed camera | `b1caa6b977985d2cef30ee691b6b599ee4489c98d49d1a3e0e8dec390a2d735d` |
| Visible pan | `e49fe7009e9644291e9ed7871b6c99ca52be355dbb113d8cc0cebe688849cc14` |

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
to replay the clipped pan. Add `--camera-y 420 --updates 600` to
reproduce the closer battle window. The command builds both ROMs from
the same authored fixture and writes per-update hashes and timing
receipts. A changed sample causes a nonzero exit. Both checkouts
build with the project's standard ARM tools and the authored
skirmish source.
