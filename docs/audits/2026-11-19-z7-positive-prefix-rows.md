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
band extensions over the sample. These are modeled work counts.

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

## Bootable ARM replay

The lazy-row candidate adds the flag to the pinned deferred-seed link
vector and passes the regular ROM header fixup. The link strips debug
sections to fit the scratch volume. The observer uses the control's
authenticated structure layout and verifies the matching sizes of all
decoded state banks in the candidate symbol table. EWRAM occupancy is
unchanged. IWRAM grows by 184 bytes and leaves 304 bytes before the
linker's stack guard.

| Dense Z7, 477 advancing updates | Deferred seed control | Lazy rows |
| --- | ---: | ---: |
| Mean whole update, cycles | 1,822,508 | 1,784,538 |
| p95 whole update, cycles | 2,652,372 | 2,589,126 |
| Slowest update, cycles | 3,500,472 | 3,386,000 |
| Mean orders, cycles | 885,334 | 847,788 |
| Cycle-based emulated updates/s | 9.21 | 9.40 |

All 131 historical checkpoints, nine byte-exact world and display
cuts, and 480 frame and route records pass. The two runs also have
identical per-update sampled actor transition records. Mean whole
update cost falls 2.08% and order cost falls 4.24%. Keep the flag opt-in
until the other profiles and dense map shapes have been measured.

- Control replay SHA-256 `0c0555eb675867fc641a17c0fe39c0768e9a999a9ecc341d2da104e71c6daae5`.
- Candidate replay SHA-256 `23bbf236ab23c7a6f206e3a38a0e97230a1399fefd31ca5eef42365beb68d37f`.
- Candidate ROM SHA-256 `71d0baf51a7b9044664c26853d7eebbb292c282e3145987e17a7c0f8d8c70cbf`.
- Candidate ELF SHA-256 `965b9846a31527c5530e470b66ea36bc0094c61989fb4454c1b9e179e90fd70b`.

## Fresh SRAM continuation to victory

The same ARM candidate loads the authenticated late Z7 save at tape
cut 188,324 in a fresh emulator process. It matches the saved world,
reselects the same Hatchery through the ordinary input sequence, and
wins at game frame 194,998. All six raw world and display cuts match the
deferred-seed control through victory. The seed file hash is unchanged.

- Continuation receipt SHA-256 `c11cec5cfc32b3e0ccef9bcb2fdcb4f9c5d495dd7aa140f4e0ff1c19d96377df`.
- World and display receipt SHA-256 `40983ea33a164dec97d718b01509bdb4c29141df84d58ba5ec24553eee61fda4`.
