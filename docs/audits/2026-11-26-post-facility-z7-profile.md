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
game-state samples matched the release. The baseline's 95th percentile is
2,878,551 cycles and its maximum is 19,998,355.

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
