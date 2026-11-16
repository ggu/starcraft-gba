# Equal-footprint rejection in shared sight spans

Sixteen ordinary P10 instruction windows average 1,760,018 cycles.
Exploration takes 178,306, including 101,218 in the exact shared sight
row helper. Two sampled trigger sweeps were kept out of these averages.
The row share grows from 1.40% early to 5.45% in the middle and 9.15%
late. This suggests targeting row calculation rather than the whole
exploration update.

The isolated `dd48157` probe (`SC64_SHARED_SIGHT_PHASE_REJECT`, outside
the release tree) reuses the Z7 phase
classes. Nonnegative positions in the same center tile can skip the
whole movement transition only when the exact unbounded footprints
match. Clipping preserves equality. Type changes, tile crossings,
unsupported radii and legacy spans retain their original path. The
classifier runs within the existing ROM sight lookup call.

A read-only replay of the normal P10 ROM sampled the old and new
visibility snapshots at the same calls as the instruction windows.
The radii are taken from the P10 vector's own actor sight facts.

| Sample calls | Nearby same-type moves | Same center tile | Equal exact footprint | Scalar row calls avoided |
| --- | ---: | ---: | ---: | ---: |
| 5,200–5,645 (six) | 34 | 31 | 4 | 144 |
| 24,400–24,845 (six) | 168 | 149 | 16 | 544 |
| 42,300–42,745 (six) | 292 | 266 | 29 | 986 |
| All 18 samples | 494 | 446 | 49 | 1,674 |

Across the 16 ordinary samples, 48 of 453 moves qualify. Scaling each
original row scope by that update's skip share gives a rough gross
opportunity of 0.61% of loop cycles before classifier cost; this is an
estimate, not measured candidate timing. No cartridge was linked.

The independent squared-distance phase-class oracle and the asset-free
ASan/UBSan full-grid oracle pass, including a shared path variant.
Paired actual-vector ARM `game.c` objects, without a cartridge link,
retain 13,468 IWRAM bytes on P10 and 12,324 on Terran; Showcase changes
13,776 to 13,768. These are object sizes, not final linker guard values.

The larger [packed row technique](2026-11-12-packed-sight-rows.md)
reduced complete Z10 loops by 2.6–3.1% in measured windows. P10 has
2,255,396 unpadded ROM bytes left and the Z10 version added 51,616,
so ROM capacity is not the limiting factor. P10 would need to remove
about 35% of its measured row cost for a 2% mean loop reduction across
these samples (22% late, 37% in the middle). P10's scalar path already
runs in IWRAM, unlike the late Zerg comparison, so a bounded paired
measurement is needed to establish a gain. Keep the phase-only probe
off.
