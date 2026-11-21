# Busy Blood Bath orders and world-render profile

This replay starts from a normal Z10 pause save for Blood Bath with a
Zerg human and independently named Zerg, Terran and Protoss CPUs.
The released ROM at `3049b64b` resumes through the normal HOME path at
frame 17,305 and runs 3,600 neutral controller updates. At the 31 sampled
cuts there are 59–89 live units and 40–49 buildings. Combat effects
are active at the start. Two independent emulator backends match all
31 world digests and the external cycle count for every sampled call.

The in-game counter averages 924,956 guest cycles per update. Its
nearest-rank p95 is 1,215,190 and its peak is 4,124,647. The nominal
30-fps budget is 561,792 cycles per update. These are ARM guest cycle
measurements under emulation. Physical frame pacing remains open.

## Instruction attribution

The observer samples eight quantile positions in each of four
cycle-cost bands and the single worst update. Instruction-stack cycle
attribution sums to the measured clock delta for all 33 calls.

| Group | Mean external guest cycles |
| --- | ---: |
| Lower band | 734,570 |
| Middle band | 900,263 |
| Upper band | 1,077,987 |
| Tail band | 1,493,535 |
| Worst call | 4,125,140 |

The worst call is update 2,055 at simulation frame 19,360. Its measured
orders stack consumes about 2.74 million cycles. The 32 depot reachability
calls consume about 1.40 million inclusive, including about 120,000 for
their path-grid work. Separate dense point floods consume about 870,000.
The middle-band world renderer averages 397,505 cycles, with 172,218 in
the shared depth painter and 58,666 in tracer drawing. The two
`gfx_blit_band` calls restore 47 terrain rows from object VRAM and 61
from EWRAM. A restore optimization needs to account for old dynamic
pixels on each page and camera, fog and HUD invalidation.

A separate read-only breakpoint observer examines 50 selected updates
and finds 20 completed dense point fills. Independent breadth-first
scans from the observed seeds confirm 2,777–2,801 cells per fill.
Every fill reaches all 64 rows. Four fills on the worst call each
reach 2,795 cells. A short-component packing optimization has little
opportunity in these cuts.

## Dense point-transfer trial

A temporary compile-only trial picks the existing aligned packed
writer and linear lookup for the dense 64×64 point path. The
sanitizer route oracle passes 321,445 queries with identical traces.
On ARM, all 3,600 route-counter pairs, 31 world and indexed-video
cuts, 16 fresh-continuation checks and the pause journal match.

| In-game cycle counter | Stock | Trial | Change |
| --- | ---: | ---: | ---: |
| Mean | 924,956 | 925,127 | +0.02% |
| Nearest-rank p95 | 1,215,190 | 1,220,981 | +0.48% |
| Peak | 4,124,647 | 4,130,989 | +0.15% |

The trial adds 64 bytes of IWRAM, so the release keeps the existing
writer. Next profile depot requests by cached, partial, complete
reachable and complete unreachable results. Measure repeated
complete failures for the same depot and topology before trying reuse.
Preserve cache publication, route counters and journal continuation
in the comparison.

| Local evidence | SHA-256 |
| --- | --- |
| Ordinary ROM | `0308f56731154b1eda4068f11557184f24d9e37a69658ae68b4810d691356a68` |
| Pause save | `5c57a8047a95d883b617ea51672a07e19d4bbb0835d0eb1e26eaccc96602212c` |
| Instruction-attribution validation | `697c82f6b421272280a1aa8fa5749128117027774efb1daf0cce88bbf738de59` |
| Independent timing validation | `97f01c23aeb03e01899d2f3b229310c6d5417093920dfd40ad8f02e65f9d5d44` |
| Dense route observation | `0f3685fcb945aa8fbbb823ae9bcd7fb31e2eae9eff71a7283c8d97a3a040f192` |
| Paired transfer-trial comparison | `2628365ba6a0b22876b90b62ea9317d438ae54b2596847a4eb4bfad20e3a57be` |
