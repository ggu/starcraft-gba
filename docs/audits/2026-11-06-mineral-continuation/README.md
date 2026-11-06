# Depleted mineral continuation candidate

The November 6 hardware report is confirmed as Protoss10 custom gameplay.
The shared worker order loop stopped when a mineral field was exhausted.
This candidate lets an established mineral job choose another nearby field
without discarding its last cargo or restarting deliberately stopped workers.
It is not merged or delivered yet.

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

Normal guarded Terran/Z7/P10/Showcase ARM links, a baseline-failure witness,
and clean asset-free default build/test checks are queued. Do not promote
before their results are recorded. Native mineral timing, per-field queues,
new-map pacing and hardware 30-fps behavior remain separate work.
