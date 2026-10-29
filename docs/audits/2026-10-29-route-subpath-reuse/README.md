# Reuse exact cached route suffixes

Complete imported-race reverse BFS fields can answer some nearby new goals without another full-map flood. The query follows at most two cached fields, for at most 128 gradient steps each. It accepts a field only if the requested walkable goal is on the origin-to-root path. All other queries retain ordinary exact-goal lookup and construction. No additional persistent storage is used.

FIFO discovery in left, right, up, down order chooses the lexicographically first shortest root-to-origin path. A suffix of that path is also the lexicographically first shortest path from its own root: a shorter or earlier suffix would contradict the original choice. Reversing parent pointers therefore preserves the exact first movement step, including ties. Blocked endpoints, blocked cached roots and partial fields are excluded. An existing exact-goal field takes precedence. Cache ages advance once per request, including decay boundaries; recency wrap uses the ordinary invalidation path.

## Private ARM comparison

A frozen 55,000-tick Z7 replay compares default-branch tree `8ef4cad70797d1fea5824170d17b324fdd9d5023` (merge `a4ff09dcb42aa29f2c451b443fabcb99adf56746`) with only this routing optimization enabled. Both builds retain the same aggregate diagnostic flags and harness. All 137 state traces, three world checkpoints and three Mode 4 RGB checkpoints matched the same ASan/UBSan host reference; frame 54,096 and 5,627 input events matched. All per-window stage accounting checks passed, and source/dependency pins remained unchanged before and after execution.

| Tick interval | Valid samples | Total cycle change | Full floods before / after |
| --- | ---: | ---: | ---: |
| 1–25,000 | 24,153 | −1.089% | 450 / 291 |
| 25,001–50,000 | 24,765 | +0.032% | 42 / 40 |
| 50,001–55,000 | 4,901 | −4.081% | 1,794 / 1,489 |

Across all valid samples, cycles fell 1.003%. The maximum measured update fell from 44,965,711 to 43,097,903 cycles (4.154%). Total full floods fell from 2,286 to 1,820. The unchanged RAM guards passed: this diagnostic candidate ended at BSS `0x0203da24` and IWRAM `0x03004eb8`; the helper remained in ROM.

Baseline ROM SHA256: `c547a6a74cd1ea9283c252bdf68a6f9118e0dda10df3d8a90db81da0ca64adc7`. Candidate ROM SHA256: `a48936c07b90958fc373a1673e615905a01b4e1db9eeecb2902d9796caebeeab`. ROMs, inputs, generated data, captures and private harnesses are not distributed.

This is a bounded diagnostic comparison, not a full-mission or hardware frame-rate qualification. It does not enable another campaign mission, establish steady 30 fps, or remove the remaining movement and rendering bottlenecks. The small middle-window regression is retained above.

## Corrected source provenance

The original timing image `f3050c0c…90b5a464` used an earlier private helper revision because its harness retained an absolute include to that source directory. Its route header SHA256 was `b498ab288ff10674d6ec7f10d003f0ccdc152d9894d1bcb142ffb7dc142c08da`, rather than the merged header `3c7906f94e0ae219fb358fea10f8e6e745e458daeffb87f4cc47812f5f6fd7ac`. Pinning both files did not detect that source-selection error. Those earlier timings are not exact measurements of the final merged placement.

The table above supersedes that run with a fresh rendered replay of the exact merged runtime (`f0a26676752d2efb4b30fc57d6404a3600b93864`, tree `90b83ee767287ea5e8777319938ba916cace4017`). Actual per-translation-unit dependency files are compared with their manifests, and every authored common/starcraft dependency must resolve within that variant's source directory. Compiler arguments match the original comparison after only source/output namespace rebinding. All 137 traces, three world digests and three displayed pixel digests pass, with unchanged pre/post pins and zero accounting errors. The pre-change baseline's own dependency namespace was also verified. The four normal ROM builds already used the correct committed source; they are unaffected by this correction.

A separate same-source experiment forced the contact geometry fallback out of line. It reduced the main mask function from 964 to 700 bytes and its local allocation from 132 to 100 bytes, but increased fallback-path peak stack across these frames from 168 to 208 bytes. The 55k replay still matched, while cycles increased 0.100% overall (0.135% and 0.182% in the middle and late intervals). That annotation was rejected and is not enabled in the source.

## Source and product checks

`./dev build` passed. `./dev test` passed all C fixtures and 649 Python tests (20 skipped for optional inputs), plus the separate eight-test group. The focused ASan/UBSan subpath fixture covers deterministic equal-distance ties, 328 seeded reuse cases per profile, nibble/packed fields, byte/bit grids, active-pool changes and canaries, blocked endpoints, unreachable and bounded misses, exact-root precedence, eviction/reset and decay/wrap accounting. Partial mode has a compile check and retains the existing behavior tests. The older LRU capacity fixture now spaces independent roots beyond the four-cell reuse neighborhood while retaining its exact counter and BFS assertions. The source boundary check passed with 944 indexed text files.

All four normal prepared profiles rebuilt within the unchanged RAM guards: Terran, Zerg, Protoss P7 and Showcase. The final helper keeps fallback dispatch in ROM; Showcase ends at IWRAM `0x03004f68` (152 bytes below its guard) and BSS `0x0203df38` (200 bytes below its guard). These builds do not constitute new full-campaign, fresh-process SRAM or hardware qualification. Campaign availability remains T10/Z6/P7. No CI workflow was dispatched or SD card written.
