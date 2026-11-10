# Z4 shared Egg and CreateUnit retry HOST experiment

Runtime checkpoint `1cfa354d` adds an explicit HOST-only Z4 stage to a combined
Z9+Z4 qualification vector. It does not enable the standalone Z4 product, any
normal ROM flag, or a release capability. Z4 source slots use its explicit
mapping (1 to ENEMY, 2 to owner 4, 3 to SIDE), and both producer admission and
new-tag detached Eggs check the actual source/product permission. Ambiguous
transfers fail closed. The owned-ROM permission header matched SHA-256
`c9ff17c083b5f9cb699aad753da5b496931dd818f5b50d315954926632457a03`;
593 inputs were checked before and after the runs.

Z4 CreateUnit advances its source cursor before dispatch. If a free body
exists but consuming it would leave too few for a paid paired Egg and existing
queued products, the experiment persists one pending bit per source trigger in
the previously invalid high part of the Z4 fault word. The new-tag validator
requires a matching CreateUnit action, cursor and one-tick action state; old
tags reject the marker. The callback retains the original capacity fault when
the pool is already full at initial dispatch. A pending action waits if the
pool later fills; when capacity returns it retries once before that trigger
advances. Independent emitted actions still dispatch, and a terminal outcome
is held until pending creation is resolved. This is a bounded GBA capacity
policy, not a claim of native or PC fidelity: pinned OpenBW commit
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8` consumes a trigger CreateUnit
failure and completes the original Egg child when allocation of its second
child fails. N64 behavior remains unverified.

Under the shared native lock with a 4 GiB reserve, the combined HOST vector
passed actual-bank paid Zergling Egg admission, save/fresh-load and paired
birth, owner rejection and genuine old-tag fallback. Source-3 PUNI denies raw
46, but every larva product supported by this vector is permitted; the
structurally valid denied-product admission and detached-row tests therefore
use a clearly separate synthetic permission fixture and are not an
actual-bank denial claim. The unmodified `aa8f43c5` combined Z9+Z4 source
produced a genuine initial-world old-tag save, SHA-256
`dcc157d9f0dbc43f0d2e94c65a519c200a2c7b4697cf28f00fa7793db4d3162e`.

Constructed Z4 tests also passed: pair plus queued-product capacity boundaries
(including already-saturated queues); no-pair and initially full-pool controls;
retry waiting across a later full pool; exact-once creation and cursor advance;
a concurrent independent source action; terminal outcome held across
fresh-process save/load; malformed wrong-action marker atomic rejection; and a
checksum-valid old-tag marker rejection with a marker-cleared control and
genuine-old fallback. A separate current-head matrix passed Z1, actual-bank
Z2, Z3, Z8, Z9 and shared-capacity tests. The genuine-old Z3 fallback was
qualified on earlier runtime `c6fc9833`, whose Z3 runtime remains unchanged.
Inputs, private saves and executable artifacts remain ignored.

The initial `71990544` retry fixture failed compilation due to a test-main macro
collision; the first concurrent-action test at `06386962` failed because it
expected an AI side effect from a resource action. `0808adb8` failed its own
full-pool predicate assertion and independently missed a queued-plus-pair
boundary; both were corrected before the final checkpoint. These failures are
retained in the local receipts. An independent read-only review of
`1cfa354d` found no further concrete marker/order defect in the reviewed
scope.

All worlds and capacity conditions here are constructed HOST fixtures. The
standalone Z4 profile and its actual save layout, ARM links and memory/stack,
ordinary-input campaign continuation, broader concurrent/terminal schedules,
transfer semantics, original source trigger parity and Z5/Z6 remain open. No
complete campaign, hardware timing or near-30-fps claim follows.
