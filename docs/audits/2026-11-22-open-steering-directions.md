# Shared ground steering cost

The ground mover evaluates its local steering directions after an
actor blocks the intended step. The contact query already identifies
which of the eight offsets cannot move. Visit the remaining directions
in the same order and evaluate distance and terrain only for them.
An earlier terrain check has already prepared the path grid for this
movement call. Equal distances keep the first eligible direction.
The existing contact memo, waypoint state and blocked retry remain in
the same control flow.

The shared path is active in the normal Terran, Zerg, Protoss and
Showcase builds. All four pass the ROM and RAM guards. Static EWRAM
endpoints match the control. IWRAM ends eight bytes earlier in each
race ROM and 24 bytes earlier in Showcase.

| Paired controller replay | Mean guest cycles per call | p95 guest cycles |
| --- | ---: | ---: |
| 9,489-call P10 resume through victory | 1,550,286 → 1,535,936 | 2,134,533 → 2,119,436 |
| 3,600 advancing Z10 Blood Bath updates | 820,235 → 817,911 | 1,109,367 → 1,108,015 |

P10 resumes at frame 36,761 and wins at 46,057. All 68 world,
indexed display and fog cuts and victory SRAM match. Frame and route
counters match on every call. The mean falls 0.93% and p95 falls 0.71%.
The Blood Bath pair uses the combined Z5 detector build as control.
Its mean falls 0.28%. All 31 world and video cuts, 16 fresh HOME cuts,
per-call route counters and SRAM match. These traces measure emulated
guest cycles. Dense campaign pacing is still above the 30 fps budget.

All 54 ordinary Custom Game launches and fresh HOME continuations
pass across the three race ROMs. The updated Z10 ROM also consumes
the first 24,000 calls of the corrected Z7 physical tape and matches
all 61 named observations in that prefix.

| Local receipt | SHA-256 |
| --- | --- |
| Current P10 victory control | `aeb93c63d55a0bac711100683d7da239dfa34b4b2a24bc0f327698575fea461e` |
| P10 steering pair candidate | `c3e926c0d70257c7dc3f539585d6bcab9e0fc7e896fe6b9d94a34e1052f1214f` |
| Combined Blood Bath replay | `7bbe1bda3c207236474a24b09fde9e77974ca3e181bf73350dd53c7fd6889f66` |
| Custom Game fresh HOME matrix | `31fa35ccba191e0b299c2dc4b5e3ae8094e97ecd2367339eda288f87b44e5caa` |
| Combined Z7 named prefix | `d509b47a6381c0f148d79eeba2198b66f82c0d68dbce9478316e12b7c56e79fe` |

## Reuse the prepared walk grid

The intended-step terrain check prepares the walk grid
before blocked steering. The contact query and its memo
leave the grid alone. Read it directly for each open
direction that can improve the current score, with the
existing bounds and same-cell allowance. Keep the exact
squared distance, direction order and tie rule.

An ASan/UBSan fixture extracting both source loops
matches all 393,216 selected directions and non-origin
terrain probe sequences across three map geometries.
The fixture exercises every open mask, speed zero,
blocked cells, equal scores and map edges.

A separate authored ARM fixture compares 512 selections
per batch. The mixed batch uses the direction counts
observed in 503 steering calls across 16 retained Outpost
updates. The walk grid is valid before and after every
call. The most common counts are five open directions
(169 calls) and three (102 calls), with 29 empty masks.
All recorded world, video, palette and route cuts match
the retained replay. The timing batches use authored
terrain and the matching tested compiler settings.

| Candidate directions | Normal cycles, before → after | Size-optimized cycles, before → after |
| --- | ---: | ---: |
| None | 44,096 → 47,680 | 59,456 → 63,552 |
| One | 131,751 → 128,636 | 138,523 → 136,432 |
| Random masks | 247,896 → 219,660 | 260,704 → 243,568 |
| Seven | 342,278 → 294,765 | 358,717 → 330,934 |
| Observed mask-count mix | 228,000 → 203,592 | 240,963 → 226,616 |

The isolated empty-mask fixture costs more. The mixed
batch saves 10.7% and 6.0% respectively. This measures the
selection loop, and full-game and cartridge timing are
still open. An earlier exact linear-distance rewrite
raises the authored selection costs and stays local.

| Local receipt | SHA-256 |
| --- | --- |
| Authored ARM fixture | `d63e0295ab0bb6ff829dedd81cabc1e4d41227ab4dd8dddf02a217f138697f37` |
| ARM timing output | `c7a072a5ec2114046e3b6a73e28ba48f0ea8085a3b62625c3f179d950365c0ff` |
| Exact-source differential driver | `a28da5db3e2b862b406a7da8eed286f763436ca790d918bcc70203c446d83438` |
| Outpost steering masks and grid validity | `f75c75ed516a7e4c4de13592a82e42204f8832dad5548f93eb348730ceb41971` |
| Rejected linear-ranking timings | `c03e256e5276b342ccf6f57600be5c2bae130649479ef3817354b2d55f1d985a` |
