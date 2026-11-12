# Exact linear FIFO for Z10 point fields

The 96×128 Z10 cartridge can spend seven point-field fills and one stale
field materialization in a single update. The earlier rectangular trial
sampled steady intervals with little point-routing work. This trial targets
the measured call 19,531 and leaves the established 64×64 fast path alone.
The probe is opt-in; normal cartridges retain their current behavior.

For 96-column maps, copy the packed walkability bits to a padded byte grid
inside the shared route scratch. Process the same seed and neighbor order
as the stock reverse flood. Each append-only batch reserves room for four
neighbors per cell. When it runs low on room, compact only the outstanding
frontier in FIFO order. If the frontier cannot fit, the existing packed
flood starts with its destination field and collision grid untouched. The
probe packs a complete direction field before returning to the stock
cache bookkeeping. Goal keys, logical/physical residency, ages, donor
selection and what callers can read are unchanged. Other map widths use
their existing dispatch.

`make test-sc64-route-linear` passed for the control, probe and a forced
32-entry frontier overflow. Each run checks 386,646 route queries, 26,283
resident fields and 35,704 fresh donors, and the per-query cache state
streams and fill counts match. A fully open 96×128 map also forces actual
compaction with the normal queue. The small-queue fixture verifies
unchanged destination bytes before fallback, then compares every cell to
the independent stock-order reference. Both verify the retained topology
grid remains intact.

## Paired ARM replay

The prepared probe was built from [#425](https://github.com/ggu/starcraft-gba/pull/425)
with the normal Z10 options and `SC64_ROUTE_LINEAR_FLOOD_PROBE`. The
same flag is available in the shared profile options:

```sh
make CANONICAL_ROUTE_LINEAR=1 verify-zerg-z10
```

The control is the normal [#421](https://github.com/ggu/starcraft-gba/pull/421)
cartridge; the intervening preview changes do not enable native start
selection in either normal build. Both loaded the same saved mission from
HOME with ordinary buttons and advanced through call 20,000 with the same
input tape. All 19 game checkpoints, including raw unit, building and
missile digests, and all 12 sampled video, palette and visibility results
match. Neither run changed the input save. The worst update still recorded
seven route builds and 40 hits in both builds.

The complete run on the same probe then won at frame 59,997 (call
61,938), with no unsupported mission events. All 61 campaign checkpoints
and 24 video samples match the complete control, as do the nine ordinary
dialog acknowledgments. Both complete 64×64 custom runs with three AIs
also match their 76 and 77 control checkpoints through defeat; measured
mean loop time on each map is within 0.001% of the control.

| Guest cycles | Control | Probe |
| --- | ---: | ---: |
| Call 19,531 loop | 14,834,555 | 8,062,174 |
| Call 19,531 simulation | 14,205,709 | 7,432,059 |
| Calls 19,000–19,999 mean loop | 1,514,737 | 1,499,013 |
| Calls 19,000–19,999 p95 loop | 2,815,537 | 2,800,050 |
| Calls 35,000–35,999 mean loop | 1,526,879 | 1,525,630 |
| Calls 35,000–35,999 p95 loop | 2,544,045 | 2,520,844 |

The peak loop is 45.65% shorter, and the surrounding window mean is
1.04% shorter. The ROM verifier and link guards pass. The EWRAM end is
unchanged at `0x02038b00`; the IWRAM end increases 368 bytes from
`0x03004be0` to `0x03004d50`, below the `0x03005000` guard.
Control SHA-256:
`6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
Probe SHA-256:
`6935591f2067ed1d165f1f720007df1ca3756e2c1b9dccdafca25392893e36b4`.

## Cartridge links and save/load

Terran T10, Zerg Z10, Protoss P10 and Showcase all link and pass the
normal ROM verifier with the shared opt-in. Each has the same EWRAM end
as its control. The IWRAM ends remain below the `0x03005000` guard:

| Cartridge | Control IWRAM end | Probe IWRAM end | Probe ROM SHA-256 |
| --- | --- | --- | --- |
| Terran T10 | `0x03004748` | `0x030048b8` | `4bf6d9d30561f36bb6c36105d062af558d2425bce2073e84a7b2b283fce5e964` |
| Zerg Z10 | `0x03004be0` | `0x03004d50` | `6935591f2067ed1d165f1f720007df1ca3756e2c1b9dccdafca25392893e36b4` |
| Protoss P10 | `0x03004ab0` | `0x03004c18` | `d911805a13ce877d5a8a8ac8b21d13d057ed5bac143b58464bed9e7b1dcfa048` |
| Showcase | `0x03004d00` | `0x03004e68` | `90fffb07ccb0943b7407929e7e46959683b3555ac86490633be05dac124759fc` |

The physical-button SRAM checks pair the normal and opt-in cartridge for
each product. Across the four pairs they cover both maps, all three human
races, one to three AIs, and named, Random and Closed seats. Each run
plays 120 updates, saves through Pause and reloads in a fresh emulator.
The resolved races, hall positions, world fingerprints, and 60-update
continuation match within and across each pair. Each pair writes the same
32 KiB SRAM image, and fresh loading leaves those bytes unchanged. These
custom games exercise the existing 64×64 route path, while the Z10
campaign proves the new 96-column flood.

The 96×96 Protoss First Strike prefix also matches through physical call
4,000 (game frame 3,793). Nine world, cache, camera, fog and video samples
agree. Both builds recorded nine logical route builds and 1,810 hits.
Read-only breakpoints counted six actual point fills; all six use the new
linear routine on the candidate. This is a campaign prefix, not a full P1
playthrough.

The 96×96 Terran Desperate Alliance prefix matches through physical call
268 (game frame 248). An ordinary A input dismisses the opening dialog;
a selected Marine then moves toward (488, 1304) around blocked terrain.
The nine snapshots match, including the Marine at (501, 1385) at the end.
Both builds recorded three logical route builds and 62 hits. One actual
point fill occurred on physical call 203; the candidate uses the linear
routine. This is a campaign prefix, not a full T3 playthrough.

This result reduces a particular lag spike. Both cycle counts are still
above the two-VBlank budget. Keep the shared flag off by default while
qualifying it on hardware and confirming that the remaining IWRAM is
enough for the next shared engine changes.
