# Full-field routing: current source qualification

This source-only qualification branch is based on `271db94a`, which includes
the producer-queue cancellation change. It adds guarded replay instrumentation
and test tools only; no production runtime or default flags are changed. The
candidate explicitly enables the existing full-field clear, visited-grid and
new-cell flags for comparison, while leaving the separate monotone shortcut off.

The prior source `31a5ddf8` passed the Z7 full HOST tape, bounded 1/3-AI custom
openings on both maps, fresh guest continuation and normal Z7–Z10 memory-guard
links. Those results do not establish equivalence on this newer source: L input
can now cancel a selected producer's queue, so the original tape must be
replayed and checked again. Current-source replay, link and performance results
are pending; no promotion or physical-hardware claim is made here.
