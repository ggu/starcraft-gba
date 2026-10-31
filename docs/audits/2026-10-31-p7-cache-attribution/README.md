# P7 shared cache attribution and classification experiments

The shared Protoss-service performance hold remains open. On the retained
normal P7 custom-game save, a private, default-off ARM diagnostic measured
47,981 mean guest cycles per update in `sc64_p7_arbiter_cache_process_due`,
within 470,361 diagnostic orders cycles. It observed zero tracked units,
detection refreshes, eligible Arbiters and optical-visibility queries across
900 samples. The cache still maintains its source clock and per-unit cache
state. These counts describe this window, not other games. Clock reads and
counters add overhead and alter code placement; the diagnostic is not a normal
build speedup measurement.

The diagnostic was built from `60734e5` with the literal normal P7 flags plus
the private counters. Across 901 active frames its full Unit and Building
arrays and generic service clock and due/initialized words matched the
retained uninstrumented ROM; nine screen/palette checkpoints also matched.
The initial saved frame was 9001, with 114 units and 48 buildings. In ten
100-frame-spaced censuses no intrinsically cloaked units or Arbiters appeared;
Overlords and static detectors were present. The counters rather than that
census establish the zero counts for the 900 sampled updates.

Two source-only experiments reused the existing transient aura byte to classify
unit identities once per cache pass. The first classified intrinsic cloak and
Arbiter identities for every live row. The second restricted cached cloak
classification to fallback raw identities and retained the direct check for
explicit raw identities. Neither changes the source timer or persisted layout.
Neither is included in this checkpoint or admitted for shared use.

| Normal ROM mean guest cycles over 900 updates | Control | First experiment | Fallback-only experiment |
| --- | ---: | ---: | ---: |
| P7 full loop | 1,054,739 | 1,047,249 | 1,059,421 |
| P7 update | 657,895 | 653,470 | 659,685 |
| P7 orders | 471,580 | 466,897 | 473,450 |
| P8 full loop | 1,678,587 | 1,682,020 | not run |
| P8 update | 1,158,897 | 1,160,365 | not run |
| P8 orders | 434,000 | 435,556 | not run |

The P7 comparisons used the same retained normal custom save and runner as the
existing hold, and the P8 comparison used the retained natural P8 SRAM and
same runner. Each tested candidate matched its control for 901 captured frames
of Unit, Building, mineral, visibility/exploration, banks, mission and service
state, and nine screen/palette checkpoints. The first experiment passed the P7
cache cadence fixture, both 80-world aura union fixtures and a focused check
for an intrinsically cloaked Observer within an Arbiter aura under ASan/UBSan.
A separate product-vector host batch overlapped a subsequent source edit and
is retained as **unqualified**, not evidence for either experiment. The
fallback-only candidate was not built for P8 after its P7 regression.

All normal results are bounded emulator measurements, include presentation
waiting in the full-loop metric, and are not hardware timing or complete
mission proofs. The first experiment's P7 gain is small relative to the older
service-disabled P7 mean of 876,869 cycles and came with a P8 regression; the
fallback-only result did not help P7. Code layout and workload mix may affect
these small differences. The public P8 parent subsequently received the
independent Pylon predicate change in `232bc0f`; these experiments were not
combined with or measured on that parent. More substantial shared work and
broader qualification remain necessary before the hold can close.
