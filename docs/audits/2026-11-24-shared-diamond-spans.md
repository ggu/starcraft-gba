# Shared diamond scanlines

The Mode 4 shape routine uses one packed word of exact half-widths for
each small diamond. It covers horizontal radii 0–15 and vertical radii
1–7, with 448 bytes of lookup data in ROM. Larger shapes use the scalar
width formula. Vertical clipping runs before the scanline loop.
Both paths keep the inclusive endpoints, leave neighboring pixels
alone and mark the same tile coverage.

The first arithmetic trial sped up an authored crowd with default
flags. Moving it from IWRAM into ARM ROM for the shape-and-clip
configuration raised the crowd's draw cost by 4.69%. The compact
lookup keeps the existing IWRAM or ROM placement choice. The IWRAM
function uses size-optimized Thumb, and its shape-and-clip object
symbol shrinks from 464 to 444 bytes.

## Authored ARM crowd

The test uses the original-art engine with an authored roster and
repeated attack-move orders. It measures the real orders and draw
paths across 120 updates on each pair. The scene starts with 76 live
actors, ends with 39, and issues 318 orders. Each build draws 37
diamonds per frame in the first ten updates.

| Authored build flags | Baseline mean draw cycles | Lookup mean draw cycles | IWRAM end delta |
| --- | ---: | ---: | ---: |
| Default | 630,571 | 608,937 | +8 bytes |
| ARM shape wrappers and scene clip | 649,845 | 625,872 | +8 bytes |
| Explicit diamond ROM placement | 669,056 | 626,783 | 0 bytes |

All 120 draws improve in each pair. Per-frame actor, building and
backbuffer hashes agree. Update time means stay close, with individual
timing differences up to 115 cycles. The draw timing excludes the
presentation wait. These asset-free links exercise the sharing and
placement options. Compiling the shared graphics object with the
retained Terran, Z10 and P10 flags reduces its IWRAM section by
20 bytes. Showcase's object section stays the same. Full race-product
linking and busy-match hardware pacing need their own checks.

## Normal original-art entry point

Both default ROMs start with A from the title screen and advance
1,200 updates. All 20 samples match actor and building state, economy,
RNG, palette, and both display pages. The whole replay takes
673,807,340 and 673,807,125 guest cycles, including the presentation
wait. The default link adds 16 IWRAM bytes and has the same BSS end.

## Shape and clipping cases

The host differential checks each pixel against the geometric
inequality on both physical pages, with seeded neighboring pixels.
It covers every lookup entry, degenerate and out-of-range radii,
large shapes, clipping against the screen and the HUD, and both
halfword alignments. Each sanitizer build checks 2,144 cases.

The ARM microbenchmark draws each shape 100 times and compares all
38,400 bytes of the work page. All ten cases match. The IWRAM
lookup saves 28–33% for the tested small fractional slopes.
Two exact ratios get slower in this configuration.

| Radii or clipping case | Baseline cycles | Lookup cycles |
| --- | ---: | ---: |
| 5 by 3 | 253,953 | 169,308 |
| 7 by 4 | 278,613 | 201,832 |
| 8 by 8 | 236,812 | 308,453 |
| 12 by 6 | 212,716 | 279,313 |
| Large, 480 by 320 | 36,502,170 | 19,531,670 |
| Fully offscreen, 5 by 3 | 184,744 | 49,444 |

The explicit-ROM pair improves in all ten cases, including the two
exact ratios. Count these shapes in real visible and offscreen
battles before attributing a whole-match FPS gain.

| Local receipt | SHA-256 |
| --- | --- |
| Default crowd baseline | `e6f93f7f03d7981891e2e42c26c2a8861529475e6778b011b4b878ba65d15f54` |
| Default crowd lookup | `41fb9b163f9790a4e39a4939dfa01cd34df748615c4f740e79eb54e24baf9de3` |
| Shape-and-clip crowd lookup | `ac06c0d18ea26a76ceefada4d4804a64c0c1829ad87983030ee7e61b4e4c01f4` |
| Diamond ROM crowd lookup | `2c6359bc196c976940ed86d274e0295de9d686fd61f2bbcf9940a53ef32b3d6f` |
| IWRAM microbenchmark | `e3bdaaecd2ca637b105ddf619f18c6f6ee9abe76d606b55651806c9a018a25b3` |
| Explicit-ROM microbenchmark | `6fbcaa7ad7e453366b974821d35012540574453d2c895eed874d951a6460480f` |
| Normal original-art title replay | `a1e7a2e0ce2f3d85a4cac2302a1cd2602691d4aea2e956d5bf00beda0f541a56` |
