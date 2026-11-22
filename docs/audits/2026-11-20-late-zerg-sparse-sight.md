# Skip unchanged sight rows in late Zerg

Dense Zerg 7 moves dozens of sight sources each update. Most sources
move one or two pixels per axis, which leaves much of each sight footprint
unchanged. The normal Z7–Z10 builds use a generated mask to skip rows
whose packed intervals are unchanged. Reverse movement reuses the same
mask, even across a tile boundary. The remaining rows use the existing
clipped decoder and apply the same ordered remove and add operations.
Other movement offsets, radii and coordinate ranges use the existing
paths.

The mask is ROM-only. It adds 98,304 table bytes and 99,128 linked
bytes overall in Z10. Both normal images remain 32 MiB, with
IWRAM ending at `0x03004d88` and EWRAM at `0x02037e80`.
The save format is unchanged. Set `LATE_ZERG_SPARSE_SIGHT=0` to
reproduce packed-only timings. `LATE_ZERG_PACKED_SIGHT=0` also
disables the sparse path and keeps the scalar-row reference available.
Switching either option invalidates the existing sight configuration stamp.

## Current normal Z10 comparison

The test used the normal ROM on `fe3f7773` and the same ROM recipe with
sparse sight. The stock ROM SHA-256 is
`4aaf7eb2b543a164369d23be8f601de3566400971a9b1ee12db2c6ece2d1551e`.
The sparse ROM SHA-256 is
`d03494d5d53bbe048dddceeaacc2975f6b2dbcc7e03c873096ad425427059ef8`.

The Z7 controller tape checks 131 named observations through call 48,237.
All 480 rows in the measured window match in frame, screen, advancement
and route counters. The eight raw samples match actors, resources, fog,
exploration, both indexed pages, the active page and palette.

| ARM guest cycles | Packed | Sparse | Change |
| --- | ---: | ---: | ---: |
| Dense Z7 mean sight | 228,361 | 100,936 | −55.80% |
| Dense Z7 mean full update | 2,338,639 | 2,211,346 | −5.44% |
| Dense Z7 p95 full update | 3,390,991 | 3,272,258 | −3.50% |
| Dense Z7 peak full update | 4,692,183 | 4,564,258 | −2.73% |
| Three-AI Blood Bath opening mean | 561,657 | 561,657 | 0.00% |
| Three-AI Blood Bath late mean | 817,060 | 810,183 | −0.84% |
| Three-AI Blood Bath late p95 | 1,034,634 | 1,026,790 | −0.76% |

The Z7 timing includes 477 advancing calls. Blood Bath starts through
Custom Game with a Zerg player and Zerg, Terran and Protoss opponents.
The comparison measures the first 600 calls and the 1,000 calls through
frame 17,301. All 32 sampled custom states match, including source counts,
fog, minimap, actors, resources, both pages and palette. These measurements
use emulated ARM cycles. Dense Z7 still averages about 7.6 advancing
updates per emulated second. Orders alone take about 1.37 million cycles
per advancing call in that window. Among calls with a preceding route
counter sample, the 119 updates with zero new routes still average
1.51 million full-update cycles. The 185 updates with one new route
average 2.10 million. A 30 fps update has about 559,000 cycles available.
[Packed sight release audit](2026-11-12-packed-sight-rows.md)

## Fresh SRAM

Both normal Z10 builds load the same historical Z7 SRAM from frame
185,926 through HOME and win at 194,998. All five named checkpoints and
all six raw samples, including the restored world, match across builds.
The raw samples cover actors, resources, visibility counts, fog, minimap,
both pages and palette. The archived seed SHA-256 remains
`632a64001689bbbc35ae7a2ca4cea20b93c94a1187c5ee7cdb95ffe88b05662f`.

The 128×128 Z9 run loads one historical seed in both builds at frame
6,069 and saves after 600 more updates. Visibility, source counts and
minimap changed during those updates and match across builds at the
measured cuts. The emitted 32 KiB SRAM images are byte-identical. Each
ROM loads the other's save through HOME and reproduces the next 180
updates at the sampled boundary, including all fog and display banks.

## Additional cartridge checks

All four Z7–Z10 recipes pass the ARM header and memory guards. The normal
Z10 recipe reproduces the qualified sparse ROM byte for byte. The
original-art ROM also reproduces the current release.

The two Z10 builds replay the Z8 opening through 2,320 calls. Both match
the historical named checkpoints and each other at all three gameplay
cuts and eight video cuts. The diagnostic shaded-tile cache hash
includes ROM pointer tags and varies in this comparison. Both rendered
pages, the selected page, palette and fog grids match.

The Z10 mission opening reaches 4,171 calls with a fresh HOME reload
after the pause-menu save. Both builds write the same prefix SRAM.
Seven observed gameplay states, 24 video and fog cuts and the per-call
frame and screen timeline match. These are opening checks. The late Z7
run above verifies a full continuation to victory.

## Reproduction

With prepared local inputs, build the normal ROM with `make zerg-z10`.
Use `python3 -B tests/test_sc64_z7_sight_spans.py` for the sanitizer oracle.
It checks 51,200 radius, phase and signed-step combinations. Its 64×64 and
128×128 sparse vectors compare the full visibility count, explored and
minimap grids and exercise edge clipping, fallback moves and overlapping
sources. The original packed table generator still emits byte-identical
output.

| Local receipt | SHA-256 |
| --- | --- |
| Dense Z7 comparison | `3ead17b4aad63f9af9a2203889d259026e26b846a064fec0afd85d1a6022d46d` |
| Custom stock samples | `6adad78a1bb92146036abd636a3fd31b41f0dee9cbb13281e7f19596a2914646` |
| Custom sparse samples | `a1710a3b4c8ada182cc35e08cbe952001f2b236a00d42b256d1053f77c831beb` |
| Z7 stock fresh-SRAM raw samples | `585f52f530dce54ef979fa20d62ac679b780e89e8f2ff7ada127153468ea1080` |
| Z7 sparse fresh-SRAM raw samples | `60e11392f5b05b9d19111834b3089c3d1681ba45ff03c6b670a3dd5d058ebcda` |
| Z9 cross-load | `705e944ae1056ba854f9d4b6fd2b5c0bc86040516362a036729d8257e9d6756b` |
| Z8 opening sparse replay | `a3e57d7fec8e53d34678e5f5159fbfc3797b2939efe516ef11a9af2de30104d6` |
| Z10 opening sparse replay and reload | `7b8ac331ef608d3376e66064ef905862507cdfc4038dc5ae73cd9f9b85eed86e` |
| Z7–Z10 ARM links | `6d644205ffe9ffbee59f95f4d9c6cbc4b509dc7229a2f453b95504083acf7504` |
