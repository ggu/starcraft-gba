# Protoss mission 6 on the packaged cartridge

The c23009f8 Protoss cartridge spends more cycles in simulation than in drawing in three sampled sections of mission 6. Even the fastest section takes 929,540 cycles per advancing update. A 30 Hz update budget is about 559,241 GBA cycles.

| Input offset on HOME tape | Advancing updates in 128 calls | Whole loop | Ground and other orders | Other simulation counter | Actor drawing |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 4,630 | 125 | 1,221,598 | 237,714 | 562,025 | 47,920 |
| 17,341 | 128 | 1,349,589 | 235,034 | 524,032 | 273,629 |
| 40,331 | 128 | 929,540 | 224,126 | 368,305 | 27,441 |

Numbers are mean guest cycles per advancing update, rounded individually. The columns show selected disjoint costs. Use the guest loop timer for the total. The debugger interval also includes the cost of reaching the next observation.

## Where to work first

- Burrow lookup consumed 96,373, 99,690 and 53,485 self cycles per sampled call respectively. Burrow release consumed another 63,277, 65,088 and 64,317 self cycles. Test changes on all three windows and the earned victory save, including a fresh reload.
- The campaign tick itself accounted for 119,035, 113,051 and 110,742 self cycles. Inspect trigger work and binding validation before changing any world-state checks. Trap updates cost 71,002 and 75,328 self cycles in the first two cuts.
- Keep the middle cut for rendering regressions. The depth-ordered actor routine alone took 201,966 self cycles per sampled call there. The early and late camera views would understate that cost.

## Capture and cross-check

Boot the packaged P10 ROM (SHA-256 `705f79cdc4b24565eb69e856181c1b28a57c81c091e097036fbe49f38d2488e8`) in mGBA, load the earned mission 5 result from HOME, and continue into mission 6 on the linked ordinary-input replay. Sample 128 consecutive calls at each offset. Aggregate the guest counters using the advancing-world denominator in `pace_budget_report` in `tools/sc64_custom_full_match.py`.

Repeat the tape with the matching ELF and emulator-side stack sampling on every eighth call. All 384 timing rows and the world fingerprint at the end of each window matched the independent unprofiled run. Stack self-cycle means use all 16 sampled calls in each window, including any call that did not advance the world. They describe hotspots and use a different denominator from the table. These are emulated guest measurements on one mission and cartridge. Hardware pacing and other missions need separate measurements.

## After the burrow optimization

The change in [PR #963](https://github.com/ggu/starcraft-gba/pull/963) raised the full mission replay from 15.55 to 16.92 updates/s in mGBA. Both runs used 52,709 measured controller calls. The candidate reached the same victory with all 59 checkpoints, all 17 dialogue pages and a byte-identical SRAM re-export.

A second experiment removed the per-update copy of the door and trap enable bits into the rendering bindings. It read the existing saved bits when drawing a prop or computing the terrain cache key. The three mean loop costs changed from 1,122,243 to 1,120,151, from 1,250,443 to 1,248,540, and from 858,142 to 856,215 cycles. The terminal world matched at each cut. At all 129 boundaries in the first window, world arrays, visibility, both bitmap pages, palette, camera and the display register matched. With under a quarter-percent gain in each window, leave this experiment local and prioritize the remaining trigger and combat costs.
