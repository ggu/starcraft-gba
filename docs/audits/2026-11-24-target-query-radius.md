# Indexed target query bounds

The shared indexed nearest-target query clips its pixel bounds to the
world before converting them to cells. Dividing those nonnegative
coordinates yields the same inclusive edge cells, including a final
partial cell. The dispatch already checks the search position and
requires a valid live index.

After reading a live candidate, the query computes its squared distance
once and rejects points on or outside the circle. A point strictly
inside has both axis distances smaller than the radius. The prior
axis checks only removed points that the initial ordinary and egg
distance limits already rejected. The order of candidate visits,
ordinary slot tie-breaks, egg fallback and policy calls stays the same.
The scalar fallback still handles queries outside the indexed path.

## Authored comparisons

The ARM fixture uses the extracted production query and distance
helper, real actor record layouts, and synthetic coordinates and
eligibility. Each case rebuilds the owner-list index before a batch
of 32, 64 or 128 queries. All 36 query hashes match and the measured
index rebuild cycles agree. Query time improves 0.55–4.69%. Exact
cycle counts for the 128-query batches are below.

| Placement span and range | Compact buildings, eggs | Terran standard buildings, eggs | Compact buildings, egg rule off |
| --- | ---: | ---: | ---: |
| 2,048 / 90 | 231,136 → 227,342 | 303,087 → 300,365 | 228,787 → 223,618 |
| 512 / 90 | 520,573 → 504,455 | 657,676 → 645,575 | 504,498 → 493,772 |
| 128 / 64 | 1,381,767 → 1,318,900 | 1,846,619 → 1,797,606 | 1,260,608 → 1,221,391 |
| 1,024 / 64 | 246,289 → 242,635 | 312,574 → 310,829 | 244,162 → 240,368 |

The standalone IWRAM query symbol shrinks by 96, 80 and 116 bytes,
respectively. The Terran fixture uses the size-optimized query
attribute and the standard 100-byte building record. The compact
fixtures use the default speed optimization. The earlier simplified
record layout regressed under the Terran size attribute, which is why
the timing here uses the actual layouts. These comparisons exercise
the query and rebuild in isolation. Full race-product links, battle
replays and on-hardware frame pacing require their own checks.

The host sanitizer differential compares 126,004 answers and policy
call sequences against the prior function. It covers map sizes from
1 to 2,048 pixels, both hostility paths, strict radius boundaries,
equal-distance slot ties and the egg fallback.

| Local receipt | SHA-256 |
| --- | --- |
| Compact buildings with eggs | `d5d86ae486bb7325c604af7db2e61babd555462074894a14c87adcbbd10d7d52` |
| Terran standard buildings with eggs | `7be7f4d453b26ed25e36a5142e359595184d275fb10a588fd1149c23e96d8089` |
| Compact buildings without the egg rule | `8f3f79f92d80933b6a2681f11ad33b7e845271581c539e60162f3697e4a50ffd` |
| Generated host differential source | `8e98ba7b9fd69487c208cd61ab2622b6d92ffd20f4802e7857f0d0e43e63d1a0` |
