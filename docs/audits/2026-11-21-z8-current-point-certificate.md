# Deferred point certificates on the packed Z10 build

The exact seven opt-in route flags from [PR #616](https://github.com/ggu/starcraft-gba/pull/616)
compose with the packed-building Z10 recipe on `60dd3d94`. The control
keeps the release sparse packing and the same prepared building and
actor banks. The candidate adds these flags to its frozen ARM compiler
vector.

```text
SC64_ROUTE_POINT_PREFIX_PROBE
SC64_ROUTE_POINT_COMPONENT_PROBE
SC64_ROUTE_POINT_NEAR_COMPONENT_PROBE
SC64_ROUTE_POINT_NEAR_DISTANCE_PROBE
SC64_ROUTE_POINT_SECOND_EXACT_HIT
SC64_ROUTE_POINT_COLD_CERTIFICATE
SC64_ROUTE_POINT_FAR_READ
```

Normal point movement stays on the non-monotone path. The cartridge
header, ELF payload and prepared input pins pass. The packed control
ends at IWRAM `0x03004d60` and EWRAM `0x02039b30`. The candidate ends
at `0x03004ea0` and `0x02039f60`, leaving 352 and 16,544 bytes below
its link guards. Runtime stack watermarks are a separate check.

## Ordinary-input Z8 replay

Load the same ordinary pause SRAM from blank HOME with L. The resulting
field starts at simulation frame 16,659 and tape call 17,122. Resume the
same held controls through call 33,900. Ordinary A or START button edges
acknowledge the three recorded one-page modals at calls 27,625, 27,746,
and 27,894. The actor frame stays fixed during each acknowledgment.

All 17 sampled checkpoints match unit, building, resource, fog and
walkability hashes, logical route metadata, palette, both indexed pages
and the display register. Across all 203 timed advancing calls, game
frames and logical route counters match. A separate read-only ARM
observer records 668 identical argument and return records in those
windows. The movement calls return 627 matching coordinate pairs and
24 matching declines. All 17 cached weapon-range lookups decline.
The observer reproduces the ordinary runs' timed cycle samples.

| Tape calls | Mean guest cycles, control → trial | Peak guest cycles, control → trial |
| --- | ---: | ---: |
| 26,330–26,370 | 1,555,862 → 1,499,317 | 9,677,315 → 9,299,317 |
| 31,050–31,090 | 1,647,316 → 1,627,816 | 4,751,489 → 4,749,853 |
| 33,780–33,900 | 1,758,274 → 1,581,133 | 12,056,654 → 10,909,326 |
| All 203 samples | 1,694,983 → 1,574,037 | 12,056,654 → 10,909,326 |

Across the sampled calls, mean cycles drop 7.14% and peak cycles drop
9.52%. Nearest-rank p95 rises from 3,235,449 to 3,309,269 (+2.28%).
These measurements cover selected emulated updates after fresh load.
The control enters 94 full linear fills. The trial enters three and
attempts 103 local certificates. The far-reader helper has zero visits.

Call 33,900 adds 883,950 cycles, including 883,883 in orders. Both
builds make the same six successful movement queries, all with the
same outputs. Both retain 423 logical builds across that update.
The trial enters a local certificate and a full linear fill. The
control enters zero full fills. Preserve this ordinary read in the
next bounded-fallback test. Keep the seven trial flags off in the
normal recipes while this tail and the real far-reader gate remain open.

The combined sparse and non-monotone HOST oracle checks 194 certified
shortest paths, including 45 blocked goals, and 62 declines. The sparse
boundary fixtures match canonical BFS. The separate far-reader fixture
checks 108 randomized full fields and 8,320 range scratch cases.

| Local input or receipt | SHA-256 |
| --- | --- |
| Packed Z10 control ROM | `ea3faa53ff31553e91e3888c58746161d9ac9681bffd6a45885df4e14ea32308` |
| Seven-flag trial ROM | `944e3cc7837b5154c9729bfaf86889c48ff0c0f5edc67a56acaaa942c826a4d7` |
| Retained pause SRAM | `bf99add2606817dc705ba1c03d5a15ca8f72a6133c2e8b1b73915b2606f130d4` |
| Ordinary input tape | `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb` |
| Paired comparison with direct route answers | `2051f2a93c46b6348addf297070bbb9dec67c03f8b19f11870ae6e4709b7a6d4` |
| Trial link and exact compiler vector | `7e038214a43de5fead42ff0ad0c2cc6c4171a9d5a64b4cc565cb0edd126d02ef` |
