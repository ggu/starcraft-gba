# Dense Z7 movement after the wide facility search

The Zerg ROM from release `c96a88ca` (SHA-256
`1dab164dbc409a63ec6db61ed2b3c93de165007a47444f674159ec367c7a776c`)
replayed the archived Z7 battle from input 136,697. Two mGBA profiling
passes sampled every 67th input at offsets 1 and 34. Each of the 147
profiled update costs matched the ordinary runner at the same input.
Direct function costs sum to all 237,656,037 cycles in the sample.

| Function | Direct share of sampled cycles |
| --- | ---: |
| Linear movement-field expansion | 9.05% |
| Game renderer, excluding its callees | 6.96% |
| Contact blocking mask | 5.94% |
| Wide facility FIFO expansion | 5.11% |
| Packing directions into the route field | 4.80% |
| Nearest-target search | 4.44% |

The ordinary 4,901-input window provides the comparison below. All trial
game-state samples matched the release. Across all inputs, the nearest-rank 95th percentile is
2,877,393 cycles and the maximum is 20,000,839.

| Direction packing trial | Whole-window cycles | Change vs release |
| --- | ---: | ---: |
| Release, existing Thumb code in ROM | 7,730,499,579 | — |
| ARM code in ROM | 7,832,249,057 | +1.32% |
| ARM with a fast path for unreachable blocks | 8,087,744,307 | +4.62% |
| Compact Thumb loop in IWRAM | 7,799,450,308 | +0.89% |
| Original Thumb packer in IWRAM in place of the projectile drawing helper | 7,675,018,140 | −0.72% |

The last trial leaves 152 bytes above the required stack margin, down from
432. Moving projectile drawing out of fast memory also affects battles with
different unit mixes. Keep the release memory layout while qualifying changes
with a larger benefit on ordinary campaign or three-CPU custom play. The
full-window figures include time spent waiting for presentation.

## Bunker release comparison

On the Zerg ROM from #978 (SHA-256
`003efc93f25bff19ce62a1e230eb0242535f5d2df7ff712d37f159b1c2619948`),
the same save and controller tape reproduced every one of the 4,901
debugger intervals and both captured stage timers from the preceding build.
All 72 sampled gameplay and video records matched, including both indexed
pages, the displayed page and the palette. The battle still averages 10.64
input updates per emulated second. Three intervals fit 559,241 cycles.

### Where the 30 Hz budget goes

Of the 4,901 controller inputs, 4,866 advanced the world. That gives **10.56
simulation updates per second** including time spent in the other 35 inputs.
The cartridge loop timer and its nested counters reconciled for every advancing
update. Mean cost was 1,580,966 cycles, compared with a nominal 559,241-cycle
budget. The simulation alone exceeded the entire budget on every advancing
update.

| Stage | Mean cycles per advancing update |
| --- | ---: |
| Orders | 879,273 |
| Sight | 70,047 |
| Other simulation, including remaining update time | 207,654 |
| Map background | 184,786 |
| Actors | 55,245 |
| HUD | 101,539 |
| Remaining render and loop | 82,422 |

At the longest debugger interval, input 136,791, the route-build counter
increased by 28 and orders took 19,100,900 cycles. That spike is reproducible,
but most time accumulates elsewhere. Among the 4,865 advancing inputs with an
observed preceding counter, 1,973 built two to seven routes and averaged
1,273,900 order cycles. Another 1,703 built one route and averaged 709,615.
Even the 1,185 inputs with zero new builds averaged 435,642 order cycles and
1,133,956 total loop cycles. Prioritize the repeated one-to-seven-build updates
and identify the remaining cost of movement and contacts on zero-build inputs.

A separate instruction profile on the #978 ROM reproduced the guest cycle
counts at inputs 136,789 through 136,792. At the peak (136,791), 26 calls
to the wide cached fill performed 23 full linear floods. The floods spent
12,004,803 direct cycles expanding and 2,573,049 packing the completed
directions. On each of the two previous inputs there was one full flood.
The subsequent input had none, but still took 1,117,259 cycles, including
153,058 in uncached contact masks. Trace the destinations and changes to
the obstacle grid before choosing a cache or algorithm change.
