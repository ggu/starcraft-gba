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
list visits; 17 optical-building list visits. Aura and update/reset are within
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
