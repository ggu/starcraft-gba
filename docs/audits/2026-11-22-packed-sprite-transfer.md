# Packed sprite transfer on ARM

The Mode 4 packed sprite path reads each color word and opacity mask
together from ROM. It skips transparent groups, writes solid groups
directly and reads the destination for mixed groups. The four-pixel
phase uses the low bits of
the anchor, including for negative X. Each of the three race builds and
Showcase uses the same reader.

## Busy Custom Game measurements

The paired Z10 replay runs the same 3,600 updates of a three-CPU Blood
Bath checkpoint on the `66f9bc3d` release and the candidate. It matches
world state, both indexed framebuffer pages, HUD bands and palette at
all 31 main cuts and 16 fresh continuation cuts. Route counters match
throughout, and the SRAM files match byte for byte.

| Blood Bath guest cycles per update | Control | Candidate |
| --- | ---: | ---: |
| Mean | 840,021 | 832,982 |
| p95 | 1,132,275 | 1,123,557 |
| Peak | 3,711,437 | 3,702,179 |

On the same 33 instruction-profile calls, eight middle-window updates
average 34,849 → 27,950 cycles inside the packed blitter (−19.8%),
at 5.5 calls per update in both builds. The shared world painter
averages 190,429 → 183,524 cycles over those updates. The original
byte sprite writer still costs about 21,300 cycles across two calls
per update in this window. Measure its call sites separately from
weapon animation and the terrain copy.

The complete three-CPU Terran Outpost tape matches at 90 cuts, with
the same terminal world and both indexed pages. The 800-update busy
window mean falls from 1,000,950 to 994,929 cycles. The p95 falls from
1,294,005 to 1,287,559 cycles. The 30 fps budget is 561,792 cycles per
update. Physical GBA frame pacing remains unmeasured.

## Page, clip and build checks

`make test-gfx-sprite test-gfx-sprite-packed` passes with AddressSanitizer
and UndefinedBehaviorSanitizer. A standalone asset-free ARM fixture
also compares the scalar and packed blits over 576 generated scenes.
It covers all four X phases, three mask classes, width extremes and
clipping at all four edges. It uses both framebuffer pages and compares
every indexed pixel, including the HUD. There were zero mismatches.

Terran, Z10, P10 and Showcase all link inside the existing RAM limits.
The packed blitter saves 64 bytes of static IWRAM per profile. The
EWRAM endpoints match the control. The race-ROM campaign openings
and first Showcase scene match both framebuffer pages, HUD, palette
and state. The hidden Siege Line scene matches at 90 cuts through
450 advancing updates. An ordinary Blood Bath camera pan gives 41 matching cuts
per product. All 54 tested Custom Game launch and fresh HOME
continuation cases pass across the race ROMs, maps, human races,
and one to three named CPU seats.

The resumed P10 controller tape matches 68 cuts and SRAM through
victory. Its 9,489 timed calls use 16,681,029,665 guest cycles, down
from 16,750,465,853.

| Receipt | SHA-256 |
| --- | --- |
| Baseline instruction replay | `5e81dfb1af3b30a3f794528dcb23f1ee2c549310c19d31d5821158ac6739fe82` |
| Candidate instruction replay | `35327a8609a03fef47c8e1eab467fc95365c9b53d1f187ce2eff40ecdf186b1d` |
| Blood Bath | `edcc707b307506422bbc4280843e27c05ba6990031e48c64307d6d77c6c21dbc` |
| Terran Outpost | `675d1fb488609ac0ef9037478d892916ddc960b5c91fa0bc5b012b4aa154b107` |
| ARM clip and mask fixture | `772652579b107e9d947833b93361f8e846585868d141925b8aec33f389817cd3` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Showcase Siege Line | `afcaaab522a83752cc23745b7cd358f1c78e9421c2489d8f3f4e2e09ae766f79` |
| Custom Game camera pan | `356b3078e1ebb4c9386f0bbd2de45b4c4bac8388fc8e967a7f77a47a1b3a1d59` |
| Custom Game HOME and SRAM matrix | `62833f88072d3c325a64712fc18316ab9cab258949169e82391fe8a0fb6465d2` |
| Protoss 10 continued victory | `4da39366f7c545c095c0c56d98bfd2a410f235a366dd17c58b9a03a1b813565b` |
