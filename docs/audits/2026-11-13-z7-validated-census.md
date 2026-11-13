# Reuse Z7 live actor validation in the trigger census

Eleven gameplay instruction samples of the retained normal Z7 replay
(calls 15,000–15,979, excluding menu call 15,623) put the masked census
at 79,678–82,724 guest cycles, averaging 81,333. Mandatory live actor
validation averages 18,407 cycles. The retained samples include region
geometry calls: the 81,333 cycles are an inclusive scope, not an
estimate of the cycles this probe saves. The samples do not record
the late two-counter mask's frequency.

The default-off `SC64_Z7_VALIDATED_CENSUS` requires the existing
prevalidated, pruned count snapshot. When only the current player's
loss check and hostile force's building check remain, live validation
collects those two counts before the snapshot would otherwise scan
both pools again. No count is published when an actor fails validation.
The snapshot still validates its source rows, source locations and
current-player binding. For other pruned masks, the census retains
the same owner, raw bounds and geometry checks and only reuses the
already-proven raw-to-live type result. The full five-counter census
retains its original path. Trigger evaluation and action order, save
layout and persisted state are unchanged.

The two HOST variants of `test-sc64-z7-validated-census` run with
ASan/UBSan and existing local generated inputs. The fixture checks all
32 masks against independent world counts, source/callback
authentication, mutation and active waits, pause/load and terminal
behavior. The new case compares both late counters to the existing
standalone census across unit and building liveness transitions, and
asserts a malformed late building leaves both output counters untouched
and produces the same actor fault. No ARM build or candidate timing
was run. A native check should profile a geometry-bearing steady
sample and a later authenticated two-counter update separately.
