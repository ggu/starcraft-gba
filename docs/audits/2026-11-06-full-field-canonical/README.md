# Full-field routing: canonical qualification

Base runtime: `31a5ddf8`, including the merged command-menu alignment. This
source-only branch adds guarded replay instrumentation and tests; it does not
change production runtime files or enable any route flags. The candidate adds
`SC64_ROUTE_WORD_CLEAR`, `SC64_ROUTE_VISIT_GRID`, and `SC64_ROUTE_NEW_CELL`
only to explicit test compile vectors. The separate monotone shortcut remains
disabled: its skipped field allocations caused a genuine fresh-game divergence.

With hash-verified private inputs, ordinary retained Z7 input started from
fresh HOST state reached the recorded win for both control and candidate. Both
passed all 307 recorded observations; 175,455 per-call selected actor/world and
displayed indexed-frame/full-palette fingerprints matched, as did final journal
and replay records. Fingerprints do not establish complete-state identity.

Four HOST custom games launched through normal menus with a Zerg human, both
maps and one or three mixed AI opponents. All advanced 4,000 calls and matched
41 selected actor/world/route-field and indexed-frame/palette fingerprints per
game. The human remained idle; these opening/warm games are not completed-match
or busy-battle proof. Frozen input and source manifests matched before/after.
Private logs remain under `build/z7-host-sram-contract/host-full-flood-traces/`
and `custom-full-flood-v2/`; no private inputs or captures are committed.

Normal Z7–Z10 link guards, fresh guest continuation and guest cycle measurements
for this exact canonical source are still being checked. No 30-fps or physical
hardware claim follows from the HOST results.
