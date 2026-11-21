# Shared changed-row sight updates

Short unit moves can change only a few rows of their sight footprint.
The existing late-Zerg lookup identifies those rows for moves of up
to two pixels on each axis. The shared dispatcher uses the same lookup
for Terran, Protoss 10 and Showcase. It applies the row deltas in
ascending order. Larger moves use the packed
row scan. Negative centers and unsupported radii use the existing
geometry, including the map-edge clipping rules.

## Dense Protoss battle

The paired normal-input replay resumes Protoss 10 through HOME, plays
9,489 calls and wins at simulation frame 46,057. All 68 world, camera,
two-page framebuffer, palette and fog observation cuts match. The fog
checks include the visibility and exploration grids, both planes of
the sight-source counts and minimap visibility. Frame and route
counter histories match for every call. The complete victory SRAM matches.

| Active Protoss 10 gameplay cycles per update | `e03337e` control | Changed rows |
| --- | ---: | ---: |
| Mean total | 1,756,883 | 1,618,399 |
| p95 total | 2,406,054 | 2,260,441 |
| Peak total | 4,186,734 | 3,949,992 |
| Mean simulation | 1,081,920 | 943,185 |
| Mean sight | 240,493 | 98,629 |

The 800 updates with the most baseline sight work average 380,706 →
144,426 sight cycles and 2,182,432 → 1,951,713 total cycles. The complete
tape uses 15,388,866,488 guest cycles versus 16,702,898,894 for the
control, a 7.87% reduction. The active map rendering mean is 448,814
cycles on the control and 449,005 on the candidate. The overall cost
exceeds the 561,792-cycle budget. Physical pacing remains open.

The ASan/UBSan sight oracle checks the generated masks against an
independent squared-distance model and exercises the shared path on a
128×128 grid at tile and map edges. The full-grid checks cover per-cell
sight counts, explored state, and minimap visibility. The normal P10
link uses 56 more IWRAM bytes and the same EWRAM allocation as its control.

## Other cartridges

Zerg 10 already uses the changed-row calculation. Its ROM hash
matches `e03337e`. All four products link inside the original RAM
guards. The narrowest static IWRAM gap is 72 bytes in Showcase.
On the Terran three-CPU Outpost tape, all 90 world and two-page
framebuffer cuts match. The 800-update battle mean falls from 995,011
to 990,259 cycles. The p95 falls from 1,284,803 to 1,281,307.
On Showcase, the normal 3,600-update Outpost canary is about 0.19%
cheaper. The 1,200-update Siege Line sample is effectively flat.
The instrumented stack checks preserve at least 6,832 bytes in
Siege Line, 7,568 in Showcase Outpost and 6,504 in the dense Protoss
tail. Each pair has the same minimum untouched watermark.

The campaign and Showcase openings and 41 Blood Bath camera cuts
per product match. The three race ROMs pass all 54 representative
ordinary Custom Game starts and fresh HOME continuations over both
maps, every human race and one to three named CPU seats.

## Generated header order

An initial trial prepended a general generated directory. This
selected a different building-word header and shifted the Showcase
Outpost Hatchery by two pixels. The masked difference covers 1,425
pixels per page while the fog and world match. The same trial
increased Terran Outpost battle mean cycles by about 2.1%.

The build now places only the two sight lookup headers in a dedicated
directory. Compiler include traces confirm the same product-specific
building-word headers and hashes in the control and candidate
for Terran, Protoss and Showcase. Showcase Outpost matches both
indexed pages, camera, palette and fog at all four ordinary
observation cuts through update 3,600.

| Local evidence | SHA-256 |
| --- | --- |
| Dense Protoss paired replay and per-call timings | `fa81bd5f5b11c0147774a19329c1b68ae016e526e21b585cfd9f1cd54a0acd59` |
| Terran Outpost | `56f25fdeccd2d0b09547f8ce790827e115965c3ab15bad3e3a0cc0e9180316a7` |
| Showcase Outpost field and fog cuts | `2a9045ba206fbe0c2223de7babe64b09e65a6270f3b3b9645caecc5e27c4ccbd` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Camera pan | `356b3078e1ebb4c9386f0bbd2de45b4c4bac8388fc8e967a7f77a47a1b3a1d59` |
| Custom Game save matrix | `513b85611c5919fc8dda74efe56a007e12237af8a65051c3571b06b3cf47c227` |
| Include resolution | `7deaff09696403aa1ff0e4b46798d3fd651e1d9494dd80432a7d7c1941eee1ec` |
| Custom Game and Showcase stack checks | `793e0bf17fbb83a8c53a0a85966977ec5dcc7e237ba47ff30fecefb01f318205` |
| Dense Protoss stack check through victory | `e39a33fadfe2f4d59aefbfd82b525d0d79474a43218a3efb0f16f8a2e45bbd51` |

## Independent order-spike inspection

At P10 call 38,952 on `43c5d98`, a near-root route request costs
906,940 cycles before completing its field. The distance is 19 tiles
and the goal is open. A bounded trial widens that certificate's limit
from 18 to 19 tiles. It saves 832,102 cycles in that update and
matches the full tail's sampled world, pages and SRAM. A later
near-root request at call 39,774 costs 905,523 extra cycles with no
new route build. The active mean stays at 1,754,578 while p95 rises
1,173 cycles. Keep the trial opt-in and track partial field lifetime
across the later request before expanding the certificate.

The baseline cost trace SHA-256 is
`43de987341d98ddcbdb8b47171ca29e8b7be68b1a3bfba43150c6ed0845c2eca`.
The candidate's full-tail receipt SHA-256 is
`c772d8baae07c7575eca3815f1a6aa9c5b4a8d23d7e8df5237e8fd9c28ce5556`.
