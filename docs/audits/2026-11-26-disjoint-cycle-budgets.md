# Disjoint custom-game cycle budgets

Baseline (before indexed weapon lookup) recorded 2026-11-26, 04:01–04:02 Hong Kong time (2026-11-25 UTC), using the existing ordinary-input Zerg cartridge replay receipts. The six 512-call windows resume saved battles; the table averages only world-advancing updates. They cover both maps and all three AI races, but the human and cartridge are Zerg throughout. They are not idle-game measurements, cross-cartridge coverage, or hardware measurements.

At 16,777,216 Hz, nominal 30 fps allows 559,241 cycles per update; two 280,896-cycle display fields allow 561,792 cycles (about 29.86 Hz). Each stage is an absolute upper bound on what eliminating that entire stage could save, not an achievable optimization estimate. Orders, other simulation, background, actors, HUD and remaining render/loop are disjoint. The residual subtracts the inclusive update and draw timers from the observed loop interval; those boundaries can differ. audio_update and present wait are not independently timed, and game_audio_observe is inside draw.

| Map | AI | Active / calls | Total | Orders | Other simulation | Background | Actors | HUD | Render/loop residual |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Outpost | P | 456 / 512 | 716,012 | 174,985 | 120,391 | 137,416 | 145,101 | 73,950 | 64,167 |
| Outpost | PZ | 508 / 512 | 863,084 | 227,936 | 172,749 | 154,579 | 144,077 | 100,034 | 63,709 |
| Outpost | TZP | 494 / 512 | 1,409,253 | 569,775 | 252,550 | 123,162 | 270,189 | 125,465 | 68,111 |
| Blood Bath | P | 503 / 512 | 639,792 | 122,272 | 111,794 | 128,148 | 145,823 | 64,049 | 67,707 |
| Blood Bath | TZ | 496 / 512 | 1,001,530 | 300,498 | 169,018 | 127,499 | 253,610 | 93,912 | 56,992 |
| Blood Bath | ZZZ | 496 / 512 | 1,441,347 | 534,304 | 212,923 | 93,581 | 408,242 | 129,628 | 62,669 |

The busiest mean exceeds the nominal budget by 882,106 cycles. Even removing all orders or all actor drawing independently cannot cover that deficit; their combined ceiling is 942,545 cycles. This makes paired simulation scaling and actor rendering plausible areas for investigation, not a validated plan. Establish equivalent gameplay, save/reload, both framebuffer pages, and total cycle benefit across products and longer idle and combat windows before enabling changes.

All six receipts use the same ROM SHA-256 `1573dc017a4d85e456afc060cda322dbde5a29353428fc529e142280fb22d4f3`, ELF `2d16ecce9a610d42c7ecb0e74727e7605809944aad16341a14a0559480b944c6`, and runner `050e5e7dcab772798780a11d1c6c3b4b84135e4b681aa11e08f666738ebaa9bf`. Per-case inputs follow (SHA-256; no assets included). Local receipts were `/private/tmp/sc64-z10-lookup-qualification/<case>-release/pacing.json`.

| Case | Save SHA-256 | Trace SHA-256 |
| --- | --- | --- |
| 0-p | `b812d7307b68b4c577c8aa00bdd5200c91b81d5d650c7b9394737859c5d40e76` | `e62f2c592c9c411894eb5a298dc56ab8c1ef43f082599f3ac36202ebb50ed0a2` |
| 0-pz | `774547d7ed7822641d6ddbad16a7c8982b1542f50ff38792ad54d025fde30a06` | `1dbae613e49011bd4f728c00ee829dbaa4dc6798207cd9c8db6e680f78bb1391` |
| 0-tzp | `fe14a3ae08ad063fc5046af20d2a20c2b091bb75c079c236921b5fa1fb95541e` | `6bd708f5b772d0c692a5e9b6f793c8c5bb184118c9b583ab058c9cb9674e0691` |
| 1-p | `cfbd87abba2817478396c6fd2a8c213d0fe85884b01ae7b1b938609195cfcd1e` | `7c4c482fcce665649f46ab34f29ebd85141d648032050612fc0e4d86ed9b5fac` |
| 1-tz | `042bdb9115c2e288ac632922be6690d2b1aceb75cdb28acf5e2a101a932b3c27` | `017a33869f5e9a091f33cf85e348e891f3e38569fa260a4f76373f98b57d2704` |
| 1-zzz | `1d5334b685103822f79d6d2310950876c5506345d89935941c7459c60ecfac93` | `5438324244ff1cfbd9fc724a912973674e65132e24ee332d42f0940eb24a6363` |

## Current Zerg cartridge with indexed weapon lookup

The matching indexed-lookup ROM is SHA-256 `84bb873c12a5e09a7ad115f94835a1d17528765e7d8106960375dccf7ccfd2e0`; the same save and trace hashes above apply. The windows were recorded at 04:01–04:02 Hong Kong time on 2026-11-26. The Outpost PZ and TZP windows each contain one advancing sample where update plus draw exceeds the measured loop interval by 5,514 and 2,711 cycles respectively. Those samples are excluded below; the `pace` command retains the raw result and marks the optional disjoint breakdown unavailable with offending ticks. The discrepancy needs investigation before treating those two full-window decompositions as reliable, or interpreting the residual as a precise presentation or audio measurement.

| Map | AI | Reconciled active / calls | Total | Orders | Other simulation | Background | Actors | HUD | Render/loop residual |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Outpost | P | 456 / 512 | 714,366 | 175,027 | 120,289 | 137,371 | 144,151 | 73,657 | 63,870 |
| Outpost | PZ | 507 / 512 | 861,148 | 228,039 | 172,616 | 154,710 | 143,434 | 98,555 | 63,794 |
| Outpost | TZP | 493 / 512 | 1,406,503 | 569,878 | 252,476 | 123,674 | 268,399 | 124,298 | 67,777 |
| Blood Bath | P | 503 / 512 | 635,981 | 122,483 | 111,801 | 128,018 | 142,951 | 63,047 | 67,681 |
| Blood Bath | TZ | 496 / 512 | 992,541 | 300,583 | 169,108 | 127,792 | 245,026 | 93,100 | 56,932 |
| Blood Bath | ZZZ | 496 / 512 | 1,414,546 | 534,127 | 213,113 | 93,394 | 383,110 | 128,230 | 62,572 |

Current local receipts: `/private/tmp/sc64-z10-lookup-qualification/<case>-indexed/pacing.json`.

## Earlier Terran and Protoss windows

Four earlier 256-call, three-AI battle windows recorded 2026-11-26 03:43 Hong Kong time (2026-11-25 UTC) use the recorded Terran and Protoss products. These are different saves, traces, windows and cartridge revisions from the Zerg table; the rows describe individual scenarios, not controlled race comparisons. All active samples satisfy the same nested-counter checks.

| Human / map | AI | Active / calls | Total | Orders | Other simulation | Background | Actors | HUD | Render/loop residual |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Protoss / Outpost | PTZ | 247 / 256 | 1,205,900 | 479,926 | 216,144 | 88,206 | 268,634 | 92,237 | 60,753 |
| Protoss / Blood Bath | TZP | 240 / 256 | 1,090,326 | 435,053 | 200,237 | 140,783 | 141,837 | 101,788 | 70,628 |
| Terran / Outpost | ZPT | 249 / 256 | 1,151,306 | 395,400 | 164,496 | 158,449 | 297,786 | 86,492 | 48,683 |
| Terran / Blood Bath | ZPT | 250 / 256 | 997,868 | 343,577 | 168,911 | 131,705 | 215,509 | 88,383 | 49,783 |

Local receipts: `/private/tmp/sc64-release-815f-pacing/paced-<case>/pacing.json`. Input SHA-256 values are below; each receipt also records its runner hash and per-update data. No raw inputs are included.

| Case | ROM | ELF | Save | Trace |
| --- | --- | --- | --- | --- |
| protoss-0-ptz | `19d1cfb354796fa7656080d29a60ff0fde5c62b8d58f25eeba7481cf1b88f81e` | `7ce86ddee0d5bffab611efa88c55eefb0010d74f9b3c69f01ae1438a1481e7d8` | `6759dce2a6dde9b23fde86dbee4755f125aa4d3eab01ec9cef1d9935fce5fb78` | `42ea51caa73faed29258cdb9427388a662e87487b9926dca9e0b061444939e69` |
| protoss-1-tzp | `19d1cfb354796fa7656080d29a60ff0fde5c62b8d58f25eeba7481cf1b88f81e` | `7ce86ddee0d5bffab611efa88c55eefb0010d74f9b3c69f01ae1438a1481e7d8` | `7ed2ae0c7b340f46be4e3f55f87d78926c1b369822327e933cb40ff0c20e0a16` | `480271d09d10155232cfbffbce42a26a207911fae677f50344a723a4820145f0` |
| terran-0-zpt | `dc63f1e629bd5b4ccc416bca27493984a6da59220cace72ab2401c4af4c71f68` | `d57ecd53567170834ea3a88981e9f27b825fa616186e7dda5af54be20bf527a6` | `3158a004e1155afca7fbfdf494ae5c7e16f23aaeac3a8c06bc96bc8f0cae0caf` | `8e3800a8c4af4700d1ff87cc4a97ceb481731f98bf025ad5ff25dcc5434fb178` |
| terran-1-zpt | `dc63f1e629bd5b4ccc416bca27493984a6da59220cace72ab2401c4af4c71f68` | `d57ecd53567170834ea3a88981e9f27b825fa616186e7dda5af54be20bf527a6` | `b62b6641ac52529da15eb30b830019980783c9c8c4cb7fd6a0871d24f99f323f` | `ab0f18323b802910d383819e203baa9c5050834a3e0aef2d19e2c7b870ce7990` |
