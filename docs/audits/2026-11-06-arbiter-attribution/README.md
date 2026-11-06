# Arbiter setup attribution

Default-off measurement only. The same current P10 compiler vector and ordinary
P7 tape are used in separate baseline and diagnostic replacement-main ROMs;
the diagnostic adds `SC64_DIAGNOSTIC_SETUP` and `SC64_ARBITER_ATTRIBUTION`.
No gameplay changes, native result, physical frame-rate claim, or speedup is
asserted by this source. All counters are cumulative u32 and are differenced
modulo 2^32 per update. Timer/counter costs may heavily perturb the code.

Output indices: 0 total setup Arbiter cycles; 1 reset cycles; 2 aura cycles;
3 cache update cycles; 4 detection cycles; 5 optical-unit cycles; 6
optical-building cycles; 7 optical refreshes; 8 due/initial cache ticks;
9 aura index or scalar candidate visits; 10 aura candidates entering the
rectangle-hoisted geometry path; 11 dead-row zero-check cycles; 12 zero-check
calls; 13 zero rows; 14 dead rows and 15 live rows scanned; 16 optical-unit
list visits; 17 optical-building list visits; 18 tick-local eight-entry cell LRU hits; 19 four-entry prefix hits; 20 eight-entry misses. The census resets each source tick and does not cache or alter a gameplay result. Aura and update/reset are within
total; detection is within update, optical loops within detection, and zero
checks within update. Scope values are nested inclusive measurements, not
independent savings. Zero checks are individually timed, so overhead may be
large. Instrumented candidate/zero counters are not in normal builds.

Build with `tools/sc64_arbiter_attribution_build.py ARGV_JSON OUTPUT_DIR
TAPE_DIR MODE` where MODE is `baseline` or `diagnostic`; read each new private
result with `tools/sc64_arbiter_attribution_read.py` and compare them using
`tools/sc64_arbiter_attribution_verify.py BASE DIAG`. Require all 90
state/fog/pixel samples, active and modal counts to agree with each other and
the retained reference. Verify ordered current compiler vector, source, tape,
all COW pins and disk reserve before and after each native phase. Lock and
coordinate the bounded native run. Attribute only after stating overhead;
90 samples are not full-state per-update equivalence, and peak is not a p95.

## Results and optical reuse decision

With source `4603c788`, current P10's ordered 227-element normal compiler
vector was verified against the retained argv. Baseline and instrumented
replacement-main runs had 958 advancing updates, three modal acknowledgements,
and 90 identical sampled state/fog/pixel checks, also equal to the retained
reference. All 1,958 COW input pins and the tape/vector hashes matched before
and after each phase under the shared lock. Baseline mean and peak were
2,200,285.96 and 3,918,412 guest cycles; diagnostic mean and peak were
2,236,258.03 and 3,931,016, an overhead of 1.635%. This is a single P7 tape,
not physical hardware or whole-mission timing.

Instrumented per-update inclusive scopes were 488,617 cycles for Arbiter
setup, including 156,015 for aura and 331,506 for cache update. Cache update
includes 196,131 for detection, itself including 87,956 for optical unit
and 35,545 for optical building loops. There were 766 due ticks, 205.49 aura
candidate visits and 123.14 candidate geometry entries per advancing update,
4.12 optical refreshes, 732.82 optical unit and 465.56 building list visits.
Zero-row checks cost 11,367 instrumented cycles per update. These scopes are
nested and counter/timer overhead is included; they are neither additive nor
estimates of obtainable gains. Peaks are not percentile distributions.

A subsequent diagnostic at `0ffe76e3` counted an eight-entry tick-local LRU
of validated target optical cells, with its first four entries modeling a
four-entry LRU. It did not cache or change game results. The paired run again
matched 958 active updates, three modals and all 90 reference checks; pins
and vector matched throughout. Baseline mean/peak were 2,200,341.76/3,918,371
cycles and instrumented mean/peak were 2,228,356.07/3,929,992, overhead
1.273%. Only 409 of 3,947 refreshes (10.36%) hit either capacity; 3,538
missed. The optical loops together were about 119,011 instrumented cycles per
update, an inclusive gross ceiling, so a tick-local optical-result cache is
rejected for this workload. A miss-heavy cache adds lookup and stack cost;
no material full-loop gain is established. Future work should first find an
exact recurring reuse with demonstrated frequency across early and busy scenes
before implementing a candidate; do not repeat the regressed detector-first
or ordered-steering trials.
