# Contact attribution diagnostic

Source-only, default-off diagnostic for the existing normal P7 contact-table
vector. No gameplay or timing change is authorized. The baseline and
instrumented replacement mains must be built from the same frozen inputs and
compiler argv, with only `SC64_CONTACT_ATTRIBUTION` differing. The replay
uses the retained ordinary P7 tape, dismisses the same modal boxes and records
90 state/fog/pixel checks. It is not a full per-update equivalence proof.

The cumulative counters use modular 32-bit per-update differences. Indices:
0 initial-query cycles; 1 initial queries; 2 mask-preparation cycles;
3 mask calls; 4 mask bucket traversal cycles; 5 bucket entries; 6 table-domain
calls; 7 arithmetic-fallback calls; 8 geometry cycles; 9 direction-selection
cycles; 10 direction attempts; 11 cost-qualified terrain calls; 12 terrain
cycles; 13 completed-in-preparation masks; 14 unrepresentable candidate
fallbacks; 15 reserved. Geometry is nested inside bucket traversal, and
terrain is nested inside direction selection. Timers and counter writes add
cost, so these figures are attribution, not promised savings. Per-candidate
timing can materially perturb cache/layout; report overhead against the paired
baseline. The paired result below is specific to this diagnostic and tape.

With prepared pins and the same normal P10 argv and tape, run the build script
in separate new directories for `baseline` and `diagnostic`, then the read
script for each result and `sc64_contact_attribution_verify.py BASE DIAG`.
The build script takes `ARGV_JSON OUTPUT_DIR TAPE_DIR MODE`; reader takes
`--runner`, `--rom`, `--elf`, and new `--out`. Native runs require the shared
lock, at least 3 GiB plus the 700 MiB output budget, frozen source and pin
hashes before and after, and coordination with other queued jobs. Verify both
90 checkpoints and modal/active counts before using the numbers. A semantic
candidate needs an independent scalar and lifecycle oracle, full per-update
state comparison, multiple workloads, normal-vector timing and memory guards.


## Paired P7 result

At source `25ee229b` (based on canonical `e52bd8af`), the current P10 normal
build dry run produced exactly the same ordered 227 compiler arguments as the
retained benchmark vector. Separate baseline and diagnostic replacement-main
ROMs used that vector and the same ordinary P7 input tape. All 1,958 prepared
input hashes matched both the source and COW destination before and after each
phase; the tape and argument-vector hashes also matched. The shared native
lock and disk reserve were held throughout. Both runs had 958 advancing
updates and three dismissed modal boxes. All 90 sampled unit/building, fog,
resource/frame and pixel checks matched each other and the retained reference.
This is sampled identity, not per-update or whole-mission proof.

The baseline mean was **2,200,413.73 guest cycles** per advancing full loop;
the peak was **3,918,373**. The instrumented mean was **2,338,024.67**, peak
**4,211,553**: 6.254% instrumentation overhead. These are emulator
replacement-loop results, not a physical GBA frame-rate measurement. Peak is
reported as a tail observation; this harness does not provide percentiles.

| Instrumented scope | Mean cycles/update | Mean events/update |
| --- | ---: | ---: |
| Initial collision query | 89,479 | 56.95 queries |
| Mask preparation | 34,596 | 38.26 masks |
| Mask bucket traversal | 143,180 | 763.23 entries |
| Geometry inside traversal | 18,206 | 122.66 table lookups |
| Direction selection | 90,687 | 306.09 direction attempts |
| Terrain inside selection | 34,561 | 229.40 calls |

There were zero arithmetic table fallbacks, prepared-final masks, or
unrepresentable candidate fallbacks in this window. Geometry is included in
traversal; terrain is included in selection. The invasive per-entry counters
and nested timers make these noisy inclusive scopes, not additive costs or
measured savings. The entire instrumented terrain scope is roughly 1.6% of
the baseline even if eliminated completely; a standalone terrain shortcut
is rejected as too small for the current material-performance objective.
The entire bucket scope is roughly 6.5%, also an inflated upper bound. Prior
first-query reuse regressed the early window and ordered steering fell back
for most attempts; neither should be reintroduced unchanged.

For prioritization, the separately instrumented pre-activation P7 setup
profile found roughly 457,829 cycles/update inside Arbiter processing, within
582,673 setup cycles, with 3.32% total instrumentation overhead. Those nested
historical scopes are larger than the current collision subscopes but do not
establish current attainable savings. First profile present-source due/cache,
owner, optical and geometry work before choosing an exact reuse. Pursue a
collision representation change only if a bounded independent oracle and
paired early/busy workloads justify its overhead. The same earlier P7
profile attributed 602,805 cycles/update to rendering, also inclusive and
instrumented; separate useful rendering from presentation wait before choosing
a render optimization. None of these scope ceilings can be combined or
interpreted as a demonstrated performance improvement.
