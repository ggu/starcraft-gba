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
baseline. No fresh result is claimed here.

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
