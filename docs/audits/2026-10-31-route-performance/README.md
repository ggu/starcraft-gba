# Route performance investigation, 2026-10-31

This is exploratory local evidence, not a product activation or a hardware frame-rate claim. The paired guest-ARM replays use current Zerg source with the duration and profile caches and indexed weapon bank enabled. The 9,020-call private custom-main tape samples the last 1,000 calls for cycle totals and records 90 periodic actor/fog/RGB/frame checkpoints. All paired comparisons below matched all 90 checkpoints; those samples are not exhaustive state or save equivalence. The custom probe's loop counter measures update, audio and render including presentation wait. Its RAM layout is not the normal product layout.

The source `starcraft/game.c` blob was `69613c8d94b800bd7e30bb4b48a30906351a87dc`; runtime source was unchanged from integration `2b2e674`. The private tape header SHA-256 was `0d79706afb77e58c958a6dd622a56843f611f4400bfa014bdbeb7db751f6ef2b` and derived indexed bank SHA-256 was `d8f12875e21698ab82e2562238690c067c06bf7b1e65129abb58a5291640b896`. These hashes identify inputs without distributing them. Each build compiled its translation units afresh and retained local before/after input and dependency receipts, compiler commands, ELF, map and ROM; no private inputs or outputs are included here.

## Locating the cost

Skirmish AI decisions are made together every 60 simulation frames, but they occur after the measured orders stage. Staggering AI therefore does not directly explain or fix the measured orders peak, and would change decision timing.

In a separate, more heavily instrumented matched replay, the worst movement event was call 8330, frame 8260. Movement measured 1,085,003 cycles on the baseline, of which one `path_next_tile` call measured 1,015,282. The other direct wrappers recorded 2,839 cycles for one direct-path check, 6,826 for 46 traversability checks, 7,669 for 15 step checks, and 28,848 for six contact masks. The candidate showed the same qualitative result and all 90 checkpoints matched. Wrapper and clock overhead changes execution and the counters can overlap; these numbers locate a hotspot, not additive production accounting.

An earlier separate presentation probe found a worst-loop difference largely inside `gfx_end`, consistent with waiting for a safe display interval. Its instrumentation cannot establish exact attribution of the original uninstrumented run.

## Route combinations

The baseline for the first two comparisons is the indexed/cache-on replay without the tested route flags: total loop 693,086,989 cycles, total orders 167,904,237, peak loop 2,223,375 and peak orders 1,181,113. Totals cover 1,000 calls. The combined VISIT_GRID, WORD_CLEAR and L_FAST_PATH candidate measured loop 690,269,160 (−0.407%), orders 164,299,494 (−2.15%), peak loop 2,223,906 and peak orders 1,011,993 (−14.3%).

| Individual option | Loop total change | Orders total change | Peak orders | IWRAM change |
| --- | ---: | ---: | ---: | ---: |
| VISIT_GRID | +0.049% | −0.418% | 1,087,569 | −152 B |
| WORD_CLEAR | −0.359% | −1.77% | 1,094,265 | 0 B |
| L_FAST_PATH | +0.041% | +0.022% | 1,181,027 | +448 B |

All individual candidates retained identical BSS (224,892 B in the custom probe). L_FAST_PATH had no observed benefit on this tape with MONOTONE_PROBE already enabled; that does not prove it redundant for other maps or workloads. The three-option custom Zerg link used 19,960 B IWRAM (+288 B) and 224,892 B BSS, with `__iwram_end` 0x03004df8 below the unchanged 0x03005000 guard and `__bss_end` 0x02036f44 below 0x0203e000. These are custom-probe bounds, not normal-four product qualification.

The later cache-policy comparison used its own VISIT_GRID + WORD_CLEAR baseline: loop 690,269,992, orders 164,257,276, peak loop 2,223,957 and peak orders 1,011,625; custom-probe IWRAM 19,520 B, BSS 224,892 B. Relative to that baseline:

| Additional option | Loop total change | Orders total change | Peak orders | IWRAM change |
| --- | ---: | ---: | ---: | ---: |
| PARTIAL_FLOOD | −0.112% | −1.55% | 1,215,001 | +232 B |
| DECAY_FREQ | +0.081% | +0.005% | 1,011,357 | 0 B |
| Both | +0.013% | −1.51% | 1,215,871 | +248 B |

BSS was unchanged and the custom Zerg links passed their existing guards. Partial flooding raised this sample's peak orders by about 20%; decay showed no meaningful benefit here. No per-cache hit/build counters were captured in this uninstrumented comparison.

## Validation and next step

The following asset-free focused commands passed on the tested source:

```sh
make test-sc64-route-word-clear test-sc64-route-l-fast test-sc64-route-component-visit
make test-sc64-route-policies test-sc64-route-flood-split
```

The second command exercises 16 policy/split fixtures. These oracles and sampled replay parity support investigation; they do not replace current normal-product guard, behavioral, save/resume and representative workload qualification. Qualify WORD_CLEAR and VISIT_GRID, separately and together, against the actual normal products and representative maps before considering activation. The present sample does not justify L_FAST_PATH, PARTIAL_FLOOD or DECAY_FREQ activation, particularly where IWRAM headroom is tight. No Actions were dispatched.
