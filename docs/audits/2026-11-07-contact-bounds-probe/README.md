# Experimental contact-direction bounds and current custom baseline

The default-off `SC64_CONTACT_BOUNDS_PROBE` replaces the eight-direction world-bounds loop with four axis checks and the equivalent direction-bit groups for in-world coordinates, bounded dimensions and speeds 0–16. Malformed positions, dimensions and unusual speeds retain the scalar loop. No collision class cache, actor cap, update cadence or save data changes.

At source `df7b4946565f70a33dbb4b1447096d040afc4dab`, an independent ASan/UBSan oracle checked every coordinate in worlds 1–33 across speeds 0–16, large-map edges and malformed fallback. The actual P7 contact-cache fixture matched OFF and ON: 4,135,391 step checks, 636,214 masks, 130,320 inactive-mask checks, 71,799 mutations and zero failures in each run. Oracle output SHA-256 `d174143a3801459b45c190430eec45c42a02e90f5005be37ce4baae97d596710`; the identical OFF/ON output SHA-256 `e9ed395008b2848f1baa52657787f9fb5f38930fc3ebbad32682b0c81469b305`.

A normal P10 ON link preserved EWRAM/IWRAM margins of 19,992/2,056 bytes. The OFF control ROM was the released `70e9800f7d9803d8a70a05a417b1b7939650999800010efdd0153ebc1ade0415`; all nine translation units preprocess byte-identically with normal flags between the frozen OFF and previous verified OFF worktrees. ON ROM SHA-256 was `93ebac5fd86f0c307ada9fee2a9b82885d7e3c9de0c60c44fc97f19770117990`. GCC specialized the ON bounds symbol, so the original post-link disassembly selector failed; the successful artifact and specialized symbol were inspected without rebuilding. Corrected link/assembly receipt SHA-256 `9f0bd53af161eb7853bc11c0d390db90c1082da83e7fcb3c2948e0cd6b7ef4a1`.

The ordinary P7-on-P10 physical-button pair matched all 1,000 timeline transitions and 11 sampled selected-world/full displayed indexed framebuffer and palette observations, with 958 advancing calls, 42 nonadvancing and three modal acknowledgments. The previously qualified `70e9` control was reused only after full ROM, symbol/size, Unit/Building/compact-building layouts, frontend, observer and tape checks. Mean cycles fell from 2,265,378.15 to 2,252,160.47 (0.5835%), but the peak rose from 3,954,788 to 4,141,350 (4.7174%). This tail regression and small mean gain do not justify promotion from this window. Pair receipt SHA-256 `60cf72c3beb1d30d3dc407d6b916b954bf81005415d00fcc3ec8ca8ad3e9bb7d`.

An independent baseline on the actual normal `70e9` P10 ROM used ordinary title/menu buttons to select custom map 1, human Protoss and independently selected Terran, Zerg and Protoss AIs. Live controller slots and selected races were checked. After 4,000 neutral play updates, the scene had 41 AI units and 16 AI buildings; the passive human had four units and one building. The next 500/500 updates advanced without modal or terminal state. Mean elapsed was 623,589 cycles and maximum 2,247,514; nested means included orders 191,832, map 150,375, background 63,692 and HUD 56,316. Existing production counters were read after timing. Ten sampled complete displayed frames/palettes and selected state were retained. This is one passive-human opening, not evidence of battle, all rosters, complete games, hardware speed or 30 fps. Progress SHA-256 `8df22903061df2a6ec0390e05a24067dd4284bf5043697a69a12df26b6b3bae6`, receipt `8207a55f23acf29a9bf19b026791d492fd17f7589bcd8a46715da0c36b1863da`. Both runs used the shared native lock and 595 frozen inputs; private artifacts remain ignored. No every-frame or full-state comparison is claimed.

## Current custom opening tail

Nearest-rank elapsed percentiles across the 500 advancing updates are p50 568,588, p90 788,476, p95 815,012, p99 1,143,889 and maximum 2,247,514 cycles. Scope counters are nested; they must not be added as independent costs. Workload figures below are from the closest preceding 100-update snapshot, not an exact per-update census. AI counts exclude the four human units and one human building.

| Before frame | Elapsed | Update | Orders | Sight | Other | Map | HUD | Background | Buildings | Units draw | Total draw | Prior sample frame | AI units/buildings |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 4214 | 2247514 | 1814441 | 1663094 | 31508 | 47232 | 157066 | 49716 | 63315 | 5346 | 44641 | 231361 | 4201 | 42/16 |
| 4357 | 1954722 | 1524447 | 1390458 | 25610 | 48540 | 152019 | 50349 | 63341 | 5472 | 39345 | 227109 | 4301 | 43/17 |
| 4438 | 1684967 | 1406618 | 1242627 | 35989 | 49495 | 147375 | 50767 | 63353 | 5654 | 34524 | 222908 | 4401 | 43/18 |
| 4404 | 1413246 | 1174294 | 1009017 | 37600 | 54833 | 156236 | 57035 | 63372 | 5538 | 43461 | 238020 | 4401 | 43/18 |
| 4405 | 1395544 | 975707 | 876634 | 37559 | 48607 | 163194 | 50561 | 63397 | 5538 | 50393 | 238505 | 4401 | 43/18 |
| 4439 | 1143889 | 855890 | 704190 | 42853 | 49349 | 145105 | 117197 | 63381 | 5629 | 32227 | 287067 | 4401 | 43/18 |
| 4358 | 1123501 | 699835 | 552702 | 25898 | 48592 | 151706 | 50348 | 63366 | 5447 | 39032 | 226797 | 4301 | 43/17 |
| 4319 | 865016 | 577433 | 168248 | 49297 | 300766 | 145186 | 116706 | 63343 | 5323 | 32670 | 286628 | 4301 | 43/17 |
| 4403 | 842756 | 432194 | 286240 | 38039 | 47828 | 163607 | 50351 | 63346 | 12080 | 44320 | 238697 | 4401 | 43/18 |
| 4207 | 842666 | 506881 | 184264 | 37039 | 226370 | 157074 | 49714 | 63292 | 5254 | 38402 | 231429 | 4201 | 42/16 |

The largest update spends 1,663,094 cycles in orders while map rendering is 157,066. Several other slow updates likewise have large orders scopes; two of the ten have comparatively high other-update work instead. These counters do not include route-build or path-grid attribution, so they cannot establish a routing cause. The next useful targeted investigation is to identify the dominant orders leaves on the same reachable custom workload, with bounded per-update route-build/path-grid and movement/collision attribution and normal-control semantic checks. No new measurement or performance claim follows from this table.
