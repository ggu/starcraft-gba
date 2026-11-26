# Bunker target acquisition in a busy three-AI game

On packaged c96a88ca, four loaded Bunker units make acquisition calls in
each of 384 profiled Blood Bath updates. The stock routine spends 80,424
self cycles per update. The ordinary indexed nearest-enemy reader spends
8,332 self cycles and building its index takes 25,774 inclusive cycles.
On Outpost, Bunker acquisition is outside the measured hot path.

Cold Bunker acquisition now scans live unit slots and live building slots
in separate typed loops. Both retain the original inclusive enclosing
axis bound before checking distance and the existing weapon and target
predicates. Slot order and the strict better-distance check preserve the
first eligible unified slot on ties. The cached target path, which
includes the existing save slot limit, continues to run before a search.

A spatial-index probe saved more cycles. A pending construction can add
a building during the order pass after that index was assembled. The
typed scan reads live slots directly, including actors created during the
current update.

| Saved-game window | Inputs | Stock mean | Typed scan mean |
| --- | ---: | ---: | ---: |
| Z10 Blood Bath, three mixed CPUs | 1,024 | 785,282 | 764,122 |
| Z10 Outpost, three mixed CPUs | 1,024 | 822,188 | 822,188 |
| P10 Blood Bath, three mixed CPUs | 512 | 814,904 | 814,904 |
| P10 campaign P4 | 512 | 575,150 | 575,150 |
| Terran campaign T5 | 512 | 564,935 | 564,935 |
| Showcase campaign P4 | 512 | 635,837 | 635,837 |

The Z10 Blood Bath replay saves 2.69% in mGBA. Its p95 falls from
1,025,724 to 1,002,812 guest cycles and its maximum from 1,219,745 to
1,197,198. All 518 sampled world states, actor arrays, fog, both indexed
bitmap pages and palette match. The selected-page register differed
at input 896 in the Blood Bath window. Both indexed pages held the same
pixels at that cut and the selectors match again at 897. All six pause
saves match after fresh HOME reload. Timings use guest cycles and the ordinary
physical-input path.

The ASan/UBSan Bunker engine test exercises a cold acquisition with two
units and a building at the same distance and verifies that only the
lowest unified slot receives damage. The existing extended-range,
invincibility, cooldown, building damage and save checks still pass.
The Terran, Z10 and P10 hard guards retain 128, 432 and 408 IWRAM bytes
beyond the required stack reserves.


| Normal product | ROM SHA-256 |
| --- | --- |
| Terran | `50b8530210f32698dfdb038cdca8e443aeddd9b9c20b9ea12882f7127cc400cf` |
| Z10 | `003efc93f25bff19ce62a1e230eb0242535f5d2df7ff712d37f159b1c2619948` |
| P10 | `9143f0ace34b31472677391cbc1b0b9fca86b2c4fdbb883e040caf1d1350806a` |
| Showcase | `eaa7e991a4b4e276882826a60257dc429139d8f575e8af2e81e2c8df798d278f` |
