# Background work on the dense Zerg 7 replay

Use `SC64_BACKGROUND_PROFILE` in a prepared imported Mode 4 build to
read the five background timers and the cache result from a physical
input observer. `SC64_BACKGROUND_PROFILE_COUNTS` adds per-map-draw
counters for the paint paths and static art. Read the counters at the
same input boundary as `camx` and `camy`. The default build omits both
flags. The measured builds carry the same routing, census and sparse
sight probes from the preceding experiment. [Sparse sight replay](2026-11-19-z7-sparse-sight-rows.md)

Across controller calls 47,758–48,237, all builds match 131 historical
gameplay checkpoints and the 480 frame and route records. The measured
477 advancing calls include 450 partial restores, 18 misses and nine
full hits. The original cache counters independently agree at all 476
adjacent advancing cuts. The 18 misses each follow a changed static key.
Of the 476 classified motion cuts, 408 move the camera on both axes, 54
only vertically, and 14 stay still. Most diagonal steps move four
pixels on both axes.

| Background stage | Mean guest cycles |
| --- | ---: |
| Static key and setup | 16,787 |
| Cache lookup and restore | 59,611 |
| Tile selection and paint | 110,308 |
| Static objects | 25,566 |
| Capture | 38,751 |
| Full background pass | 251,092 |

Tile paint includes 4,130 direct visible blits, 668 black fills, 6,441
shaded lookups and 1,477 mixed-fog paints. Of the shaded lookups, 3,520
miss the direct-mapped 16-slot cache. Some map paths also paint an
overlay and can count more than one call per tile. The static pass
issues 55 mineral art calls and five geyser art calls in the window.

The instrumented ROM averages 2,048,229 cycles per advancing update
versus 2,048,123 for the uninstrumented control. The two bitmap pages,
palette, raw actors, resources and visibility match at all nine cuts.
At call 48,057, the presentation bit in `DISPCNT` selects the other
page. The profile is a CPU attribution probe. Presentation timing
qualification stays with uninstrumented builds.

With both profiling flags disabled, the rebuilt Z10 ROM is byte-identical
to the preceding sparse-sight ROM. `make test-sc64-fog test-terrain-soft-fog
test-gba-fog-palette` passed under address and undefined behavior sanitizers.

## Shaded cache capacity

The same instrumented vector with 32 shaded slots reduces misses from
3,520 to 2,208 while keeping 6,441 lookups and all paint path totals. It
matches all nine raw actor, visibility, palette and display cuts against
the uninstrumented control, including both bitmap pages and the selected
page. The 480-input timeline, 131 historical checkpoints and nearby
visibility transitions also match.

| Measured change | 16 slots | 32 slots |
| --- | ---: | ---: |
| Shaded cache misses | 3,520 | 2,208 |
| Tile paint cycles | 110,308 | 99,218 |
| Background cycles | 251,092 | 239,970 |
| Whole-update cycles | 2,048,229 | 2,037,189 |

The 32-slot build uses 4,160 additional EWRAM bytes and passes the
existing linker guards. The 0.54% full-update improvement is limited
to this instrumented window. Keep the 16-slot Zerg default while
testing better use of its existing space. Other normal race products
already default to 32 slots. Record actual shaded tile IDs in order
to evaluate alternate indexing and associativity before changing the
cache. A private Z7-only shaded bank would require 301,312 bytes of
raw ROM space. It would need a full equality check against the
installed gameplay palette's 256-byte shade map and a fallback for
other palettes or tile sources.

## Packed mineral drawing

The packed build enables only `SC64_STATIC_MINERAL_WORDS` on the same
uninstrumented vector. Both versions match all sampled world and
display state, including the selected page, and the prepared wrapper
matches 400 full pages in the host fixture. Background cost falls from
250,963 to 250,549 cycles per advancing call. Mean whole-update cost
remains at 2.048 million cycles. The packed build uses another 23,624
ROM bytes. The option stays disabled by default. The historical
Protoss-input Blood Bath menu spike still needs its exact tape and a
separate presentation timing comparison. [Earlier mineral audit](2026-11-13-static-mineral-word-draw.md#normal-cartridge-qualification)

- Instrumented ROM SHA-256 `7edcbd59f96a862c419a171320655f150eb9f3378e82cac907a7717235abd76d`.
- Instrumented replay SHA-256 `39c33efb543a999dc16eb9cf3820d4cceece670721a1c6ec908f5195e6a1a406`.
- Background summary SHA-256 `b05143b3ead9d4cdd5c0fa720c89c37f2c86b44caefc3732f87be00f5bd75e6a`.
- Packed mineral ROM SHA-256 `7d0a94abecd70636cba0db2be683da89ccbeaf0b9fbcbbcc2a87d164f493b926`.
- Packed mineral replay SHA-256 `6eda684c5ead01070d70cd5d9f56151d356da8fd699cc9e25e5ebb193eba623b`.
- Cache32 ROM SHA-256 `488d916d6e67c02c981a3944e3aa525e50f41f8e292faf3583acf6782e8833f3`.
- Cache32 replay SHA-256 `b832ea8054a7d4d01b47a29b177d1414fb1c101582e2e666e53aec37a43ddc9f`.
