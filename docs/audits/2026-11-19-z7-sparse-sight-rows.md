# Skip unchanged sight rows in dense Zerg 7

The ordinary dense Z7 replay advances 477 times in controller calls
47,758–48,237. The retained routing and census build spends 228,866 guest
cycles per advancing update on sight. Its snapshot stream shows 21,368
nearby transitions with an unchanged actor type. None repeats another
actor's type and both absolute endpoints in the same update.
[Routing and census timing](https://github.com/ggu/starcraft-gba/blob/347f832d/docs/audits/2026-11-19-z7-snapshot-selectors.md#compose-with-the-route-work)

Of these transitions, 21,239 move at most two pixels per axis. For that
group, the existing same-tile footprint check skips 3,768 whole moves.
The remaining packed loop would visit 270,294 rows in this replay. Only
42,382 have different intervals. These counts use the original radii
and row generator for the three observed actor types.

The opt-in uses a ROM table of changed-row masks for the eligible moves.
The lower endpoint anchors the mask so reverse moves can share a table
entry. The generator compares absolute interval offsets, including
tile-crossing moves. World clipping preserves identical intervals. The
runtime visits remaining rows in ascending order and uses the original
old and new spans for source counts, exploration and minimap changes.
Other offsets and negative coordinates use the existing path.

`make zerg-z10 LATE_ZERG_SPARSE_SIGHT=1` enables the probe after private
inputs are prepared. The option defaults off. Both config stamp
directions and the named generation rule were checked. The table adds
98,304 bytes of ROM data. The paired ARM links add 99,128 bytes in all
and have identical IWRAM and EWRAM endpoints at `0x03004e18` and
`0x02038d18`. The probe adds no writable state.

Run `python3 -B tests/test_sc64_z7_sight_spans.py` for the focused host
suite. Eight configurations passed with address and undefined behavior
sanitizers. The sparse configuration checks 51,200 radius, phase and
signed-step combinations against an independent cell oracle. Additional
fixtures cover map edges, overlapping sources and fallback cases.

## Paired ARM replay

The control and sparse ROMs use the same compiler vector, including
the routing and census probes, with only the sparse option differing.
The control's unused Z8 census path differs from the earlier archived
combined ROM. Its Z7 mean is within one cycle of that ROM on the same
tape. All three match 131 historical gameplay checkpoints, the 480-call
frame and route timeline, all 21,368 nearby visibility transitions and
nine raw cuts of actors, resources, visibility and exploration, palette,
and both bitmap pages. The normal input runner only reads those banks.

| Guest timing | Control | Sparse rows |
| --- | ---: | ---: |
| Mean sight cycles | 228,854 | 100,964 |
| Mean advancing update cycles | 2,176,393 | 2,048,123 |
| Whole-window p95 cycles | 2,873,055 | 2,727,732 |
| Whole-window peak cycles | 4,388,497 | 4,258,918 |
| Emulated updates per second | 7.71 | 8.19 |

The change reduces mean sight cost 55.88% and whole-update cost 5.89% in
this window. Including the routing and census work, the mean is 17.45%
below the shipping replay's 2,481,146 cycles. Orders still cost about
1.11 million cycles. Background rendering alone costs about 251,000.
The next background comparison should separate tile paint, cache
restore and capture on the Z7 tape before changing cache layout. Record
actual camera deltas for each cache outcome.
[Shipping replay](2026-11-14-z7-point-flood-retention.md#coexistence-with-the-shipping-small-map-frontier)

The result is emulator timing from one busy window. Keep the option
in qualification for other packed-path maps and physical GBA timing.

- Control ROM SHA-256 `76e4c16db4596a0fd580887086cbd07e0354d4470ed1f629792f41d53fb6c309`.
- Sparse ROM SHA-256 `a45e83618e6f4ad870e8a3c85629dd20e1cbaa3a99413173e5de717c5f82fcf8`.
- Control replay receipt SHA-256 `6a066bd80b1e4dcbdf30dab4357abc42121e08e98a8e672130b9d9b6a1ac4411`.
- Sparse replay receipt SHA-256 `59455e9d5bab244394fc80a6cdb1174f3ba58c8879f7c76670b0557eb6c30a00`.
- Paired summary SHA-256 `2a790f6a55acb0a4d7db8201724eb0cab5be3085bf8bd66af2f5d73fbfa443d6`.

## Fresh-process save continuation

The sparse ROM loaded the unchanged shipping Z10 SRAM saved at game
frame 185,926. Ordinary title and field inputs matched all five
remaining historical checkpoints and ended in victory at frame
194,998. All six raw world and display cuts, starting with fresh
load, match the shipping continuation. This covers one shipping
save and its route. [Shipping save provenance](https://github.com/ggu/starcraft-gba/blob/347f832d/docs/audits/2026-11-19-z7-snapshot-selectors.md#finish-z7-from-a-shipping-rom-save)

- Sparse continuation receipt SHA-256 `97755d66e51f55269ce05106a84b4da0891e1b75ed21e5ef36bd424d9685ce7e`.
