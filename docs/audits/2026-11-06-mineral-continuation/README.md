# Depleted mineral continuation candidate

The November 6 hardware report is confirmed as Protoss10 custom gameplay.
The shared worker order loop stopped when a mineral field was exhausted.
This candidate lets an established mineral job choose another nearby field
without discarding its last cargo or restarting deliberately stopped workers.
It is under review and has not been delivered yet.

## Behavior and bounded scope

The selection is centered on the former field, within 192 GBA pixels
(12 source tiles at half scale). Human workers consider only visible or
previously explored minerals; AI fog knowledge remains the engine's existing
model. Fields must have resources and pass the existing movement-route
predicate. Equal-distance choices use the first pool slot. No new persistent
fields, storage pools, imported art or save format are introduced.

The helper is shared by the cargo/depot-aware order loop and the older
inline worker path. A loaded worker deposits first. An explicit pending trip
whose exact target disappears retains the existing cancellation/cargo policy;
it is not silently converted into a different job. STOP and idle orders do
not acquire a job. If no eligible field is found, the worker becomes idle.

The source reference is [OpenBW bwgame.h, pinned revision
4b046d5f65302b10cb0a745f0fecd37ec85b20a8](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h),
`order_MoveToMinerals` (lines 4287–4345). Native terrain elevation, the
8-tile free-field preference and per-field gather queues are not retained
here. The candidate is bounded continuation, not exact native allocation.

## Checks so far

The actual P10 production feature vector passed the existing worker-gather
fixture plus new focused ASan/UBSan cases for all three worker races, final
cargo deposit before retarget, subsequent pickup, deliberate Stop, unknown
explicit cargo, explored fields in fog, discovery/radius exclusions and an
unreachable closer field with a reachable alternative. These are authored
scenes invoking the real order handler, not normal-button campaign wins or
physical-device performance evidence. The 187 private header/blob inputs
rehash unchanged from the preceding P10 cycle diagnostic.

On the combined source `5dcc16c8` rebased onto #308, actual P10 depletion,
real-travel and advisor sample-request tests pass with the delivered bank
(header `73754a6165...`, blob `bd0adfd0ce...`). Clean source-boundary, doctor,
default build and full default test pass (307.16 seconds). All six normal
Terran/Z7/P10/Showcase/Z9/Z10 ARM vectors link within unchanged guards; these
are links, not dynamic-stack or performance proof. The smallest IWRAM guard
headroom is 120 bytes (Z7), and smallest EWRAM headroom is 796 bytes
(Z9/Z10). RAM guards and actor limits were not weakened.

The old `271db94` game.c baseline compiles but fails the new retarget
assertion after returning its final cargo. The earlier 9606485 full default
suite also passed (456.87 seconds); its wrapper then stopped at a conservative
restore guard because the tests created new output directories. Those outputs
were preserved and the original private directories restored without overwrite.
The combined rerun completed and restored normally. The 187 original include
hashes and the separately copied advisor pair remain unchanged; that include
receipt is not a claim to enumerate every compiler dependency.

Native mineral timing, per-field queues, pending exact-target continuation,
new-map pacing and hardware 30-fps behavior remain separate work.
