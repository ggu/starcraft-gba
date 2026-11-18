# Initialize only touched rows for a point-route prefix

The retained Zerg 7 route trace records 435 prefix expansions across
477 advancing updates. A separate FIFO model matches all 435 returned
queue lengths against the captured walkability grid. The grid hashes
match at nine replay cuts. There are 284 successful expansions that
leave a prefix, including 49 blocked roots. Their visited cells and
vertical neighbor reads span 23.98 real rows on average, p95 33,
and at most 45 out of 128.
[Route trace and coverage](2026-11-19-z7-repeated-negative-route.md)

The default-off `SC64_ROUTE_POINT_LAZY_ROWS_PROBE` initializes a
nine-row band around the root on a 128 × 128 map. It extends a band by
up to eight rows before reading outside it. The same scratch plane,
four-neighbor order, FIFO batch size, packed codes and pending suffix
remain in use. An open origin outside the band is unvisited. A prefix
that declines initializes all remaining rows before the ordinary full
flood continues. A disconnected seed and the full-reader materialization
keep their existing paths.

The model for this band size initializes 8,852 padded rows across the
284 successful prefixes, compared with 36,920 before. The mean is
31.17 padded rows, p95 41, and maximum 57. The model avoids 3,817,248
bytes of initialization writes and adds 104,153 interior tests and 787
band extensions over the sample. These are work counts. ARM timing,
layout and linker headroom still need measurement.

The prefix fixture passes under AddressSanitizer and
UndefinedBehaviorSanitizer. Its focused cases exercise poisoned
scratch, both padded map edges, blocked-root seeds, two-sided
extension and full-field fallback. With the disconnected-seed and
byte-lookup options on in both builds, 386,646 route queries and
26,306 independent packed-field checks pass per build. Per-query
logical cache state and per-scenario fill counts match. The
range-copy and strict read checks pass, including all 8,320 alignment
and remainder cases. The focused prefix fixture also passes with the
disconnected-seed and byte-lookup options off.
