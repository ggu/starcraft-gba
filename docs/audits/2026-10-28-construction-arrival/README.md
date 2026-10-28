# Construction travel and foundation admission

The previous race engine paid for a structure and inserted its live foundation
when the build order was issued. Non-Terran foundations had progress `-1` until
the builder arrived. Interrupting or killing a travelling Drone could leave a
permanent Extractor that neither completed nor allowed a replacement. A current
Z7 controller run exposed that state while its gas economy stalled.

Construction now has two persistent stages:

1. `ORDER_BUILD_PENDING` stores the requested building type in `Unit.target`
   and its site in `tx/ty`. The worker travels without paying or creating an
   actor, footprint, construction effect or trigger-visible foundation.
2. At the existing 36-pixel approach ring, the engine rechecks the worker,
   mission product restrictions, technology, power/creep, site, free slot and
   current resources. A successful allocation commits the price once and
   binds the ordinary `ORDER_BUILD` to the foundation. The Drone is consumed,
   the Probe returns idle, or the SCV begins work. Failed admission returns
   idle with no charge or foundation.

The arrival placement check ignores the builder itself, while retaining other
unit and building occupancy checks. Two intents may target one site; the first
successful foundation prevents the second from committing. Intent does not
reserve invisible terrain or fabricate a source actor. Bot macro planning has
an explicit intent query and budgets its own pending prices before training;
completed technology queries still require real completed buildings.

## Persistent representation

Order 14 was unused by the current profiles. Compile-time guards keep it below
16 and outside the established profile order range. Every existing unit codec
already stores four order bits and has room for the building type in `target`.
No Unit, packed row, SRAM payload or bank layout grows. Pending orders are
validated as worker/building/race identities and bounded destinations before
loading any live state. Resource or placement availability is checked when the
worker arrives, since it can legitimately change before a pause/save.

The save tag remains unchanged. Existing readers reject living order-14 rows
rather than interpreting the type as a building slot. New readers preserve the
ordinary legacy build-order path, including a still-associated travelling
builder and an existing progress-`-1` foundation. This change does not invent a
refund or reconstruct a builder for orphan foundations already present in an
old save. Dead slots retain their existing ignored-state behavior.

## Behavioral reference and limits

The pinned [OpenBW construction orders](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h)
check arrival, placement and resources before payment and foundation creation:
`order_PlaceBuilding` (lines 4312–4361), `order_DroneBuild` (6145–6188) and
`order_PlaceProtossBuilding` (6603 onward). This implementation was authored
from the observed behavior; reference code was not copied.

The inspected N64 creation/accounting call chain does not establish its
construction-order timing. This is an OpenBW-supported lifecycle correction,
not a claim of exact N64 order execution. The compact engine retains its
existing approach ring, separate worker/building pools, construction clocks,
footprints and visual adaptation. Exact Zerg same-actor morphing, construction
cancellation/refunds, Terran resume commands and detailed collision timing
remain separate work.

## Qualification

All verification uses local compilers and already prepared private inputs.
Imported Make recipes, asset generation, GitHub Actions and hardware writes
are not part of this audit. Prepared headers, ROMs and captures stay ignored.

The initial runtime revision `bacfd64` links canonical Terran, Z6, P7 and
Showcase with imported advisor audio and briefing data under the unchanged
BSS/IWRAM guards. These are link checks, not campaign or frame-rate evidence.
Additional lifecycle, save, normal-menu and carried-mission results must be
recorded before promoting this branch. In particular, a whole current-controls
Z7 win and near-30-fps performance are not established by these checks.
