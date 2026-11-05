# Scarab paid AI stocking probe

`SC64_SCARAB_AI_STOCK_PROBE` is disabled by default and requires the existing
human-control, weapon and namespace probes. On a source-service tick, it tries
one queue entry for an enemy or fourth-side Reaver, then advances the existing
paid production service. It uses the same parent graph validation, capacity,
15-mineral payment and source-owner wallet binding as the human command. The
choice to fill available capacity is a port-side experimental policy; it is not
an observation of PC or N64 AI purchasing decisions. It adds no persistent
storage and does not enable Reaver production in normal products.

The component fixture exercises real Unit parents, human exclusion, invalid
control decisions, insufficient funds, queue capacity, repeated paid entries,
and child allocation without a second debit. The existing payment and build
timing evidence comes from the pinned PC proxy documented in the
[human-control audit](../2026-10-31-scarab-human-control/README.md), not an
N64 trace. This probe is not a complete shared service or gameplay admission.
Source-placed ammunition, AI policy and cadence/pause behavior, owner transfer,
capacity upgrades, terrain and active-flight persistence, impact-packet saves,
art, combined Egg/Guardian/Carrier namespaces, and product matrix qualification
remain open. The historical Scarab writer mutates authenticated candidate data;
its fence remains in place pending a candidate-local replacement.

The first `make -j1 test-scarab-unit-adapter` attempt stopped before compiling:
Make attempted to regenerate the already prepared weapon source header and
reported the pinned BOLT extractor absent in this isolated checkout. Its header
was copied from the prepared input set and verified by SHA-256. The follow-up
fixture uses `CANONICAL_LOOKUP_INPUTS=` to avoid that regeneration and does not
alter the pinned reference or change any source flags. Its result is recorded
in the handoff, not treated as a normal product build.

The second command, `make -j1 CANONICAL_LOOKUP_INPUTS= test-scarab-unit-adapter`,
passed the Terran, Zerg, Protoss and Showcase ASan/UBSan Unit adapter fixtures.
The isolated private input manifest still matched all 1,959 hashes afterward.
A separate `make -j1 CANONICAL_LOOKUP_INPUTS= 'CLANG=clang -DSC64_SCARAB_AI_STOCK_PROBE' test-scarab-human-control-terran`
failed before running: the current Terran vector enables worker cargo, which
`race_save_adapter.h` explicitly excludes from the historical Scarab draft.
The existing fence was retained. Consequently the added game-loop fixture and
AI call site are not qualified; the component tests do not prove automatic
stocking in a guest or actual update loop. Both invocations held the shared
native-build lock. No ARM link or full product matrix was run.

A subsequent rerun kept the original dependency variables and passed all four
profiles with `make -j1` and `-o` for each of the 1,959 hash-verified input
paths, followed by `test-scarab-unit-adapter`. The full log is retained in the
isolated ignored `.local/scarab-ai-stock-qualified.log`; this remains a HOST
component test rather than a product qualification.

The prepared owned Z8, Z9 and Z10 scenario descriptors contain respectively
4, 9 and 1 raw-83 UNIT records. All have a raw `hangar` field of zero and a
`valid` field of 39. Field semantics and N64 runtime initialization remain
untraced. The pinned OpenBW `4b046d5` loader reads but discards its map-unit
hangar value, so it cannot establish N64 behavior or justify an initial stock.
Its unit capacity query reports five or ten by upgrade; this does not establish
AI replenishment timing. The proposed one-entry-per-tick policy is only a
bounded experiment and is not source parity.
