# Z10 worker routes immediately after a building is destroyed

The [mixed battle](2026-11-17-z10-order-tail.md) still spends 1,597,303 orders cycles at frame 16,076 with the current renderer, checked projectile retirement and near-root route fixes combined. The combined Z10 preview ROM was used as the exact control. A bounded probe (`f80e09f293712bcc367570a4f15c243763696754`) adds diagnostic events on 12 game frames and does not change regular builds. Both ARM legs reproduced 43 watched world/arena cuts, 62 framebuffer/palette cuts, 1,000 per-update route hit/build counts, and the same complete 32 KiB SRAM via ordinary title-L load and START/B save. Control and diagnostic loop totals were 967,760,116 and 973,825,345 cycles (+0.63% diagnostic overhead including code placement).

At frame 16,075 the invalidate caller resolves to `destroy_building`, with the path grid previously valid. It also clears cached worker homes. On the next update, worker 1's first depot query rebuilds the 64×64 grid. The comparison detects changed topology and five newly open cells, and the grid rebuild takes 127,084 cycles. This is a destruction-driven cache reset followed by worker demand; the event did not record the destroyed structure's slot or type.

Frame 16,076 records 15 logical route builds and 41 hits. The bounded trace retained 12 actual scalar floods (1,112 queued cells, 498,770 cycles), all while a worker checked whether it could reach a depot. The five first requests use depot slots 0, 2, 11, 3 and 1. The next seven builds are restarts of a matching partial depot field that did not reach the new origin. All 12 recorded original origins were still walkable. The trace buffer filled and dropped 82 detail records on this frame, so it cannot attribute the last three logical builds. The retained 32 worker home picks spent 868,840 inclusive cycles: 726,110 in measured reach checks and 142,730 outside them, including the rank scan and probe overhead. These are nested timers; the grid and recorded flood costs are already part of the reach measurement.

Example: workers 1, 2 and 4 request the same depot in succession from cells 197, 129 and 134. Their fills each restart from the same circle, visiting 41, 78 and 85 cells. Workers 7 and 13 make the corresponding first/restart queries for depot 2, with 165 and 200 cells; other depot queries intervene. This distinguishes repeated rebuilding of a bounded prefix from both a slow rank scan and generic point routes. At frame 16,079, a separate spike has four logical point builds, three donor adoptions and three actual point fills (676,486 cycles), including a lazy field materialized after adoption. Logical route counters alone conflate these cases.

An opt-in single-depot candidate keeps the original FIFO suffix and its packed discovered cells so a later origin can resume that exact prefix. The prototype leaves cache age, physical field selection and logical builds unchanged, drops the suffix on overflow, incompatible requests or scratch ownership changes, and always discards it across topology change or load. The independent packed-field oracle passed with 46 observed resumes and 17 overflow cases.

The paired ARM test again matched all 43 world/arena checks, 62 framebuffer/palette samples, 1,000 route deltas, and the complete 32 KiB SRAM. Only two of the 1,000 updates actually resumed a search, nine times in total. Frame 16,076 saved 15,431 orders cycles (1,597,303 to 1,581,872), but the whole-window mean rose from 967,760 to 968,033 loop cycles and the 95th percentile rose from 1,295,233 to 1,297,322. Keep the prototype out of release builds: a single retained queue cannot remove restarts that alternate among different depots.

| Local attribution evidence (SHA-256) | Digest |
| --- | --- |
| Combined control ROM | `d318b0e8cb90b309fb9f3cd3d2817957df0250edfb49a2c4030f770ecb3654c2` |
| Worker route probe ROM | `fd9094d37c4c51deb5b2729aa880f010d54b6355c33c93b4882dfc2e88420345` |
| Event ledger | `ae53332f28c5a705bea9d39c525a5b70746318d965056b0ce8b7d502cbb73150` |
| Paired attribution receipt | `696767737fdaa2db1d7bba817c22a80ffbdd854b1badc6cb974b0fc12643ab58` |
| Frontier candidate ROM | `b2c5f837d53accb5ec0431dee6aad2b7e05bade0bf5d2e6651fe357f834bc2cd` |
| Paired frontier receipt | `2bc9d881fc01e1e7922df932499088b0b82b4b0dad2375d305b92272dfd65e23` |
