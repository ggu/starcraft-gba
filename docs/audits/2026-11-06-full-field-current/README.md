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
replayed and checked again. Current-source link, guest and performance results
are pending; no promotion or physical-hardware claim is made here.

On `271db94a`, both fresh HOST Z7 full-tape builds reached the recorded win and
passed all 307 strict observations. All 175,455 per-call selected actor/world
and displayed indexed-frame/full-palette fingerprints matched; final journals
and replay records matched, and the frozen source/input manifest was unchanged.
The recorded tape therefore remains valid through this queue change in this
HOST comparison. These fingerprints are not complete-state proof; other
scenarios, normal links, guest continuation and cycle measurements remain open.

Four HOST custom games on this source launched through normal menus with a
Zerg human, both maps and one or three mixed AI opponents. Each advanced 4,000
calls, and all 41 sampled actor/world/route and indexed-frame/full-palette
fingerprints matched between control and candidate. The human remained idle;
these are opening/warm simulation checks, not completed or busy battles.
