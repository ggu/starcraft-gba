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
The lifecycle, save, normal-menu and carried-mission checks below give their
own scope. A whole current-controls Z7 win and near-30-fps performance are not
established by these checks.


### Local results at runtime `bacfd64`

The runtime tree (`common/` and `starcraft/`) remains identical to that revision
through the subsequent fixture/controller changes. Default doctor/build and
full tests pass: 609 Python tests with 18 optional skips, plus C fixtures. The
source-boundary check passes on the staged source-only tree.

Both lifecycle and actual Save fixtures pass in Terran, Z6, typed P7 and derived
Z7, each checking all three player races. Coverage includes travel interruption,
Drone/Extractor recovery, self versus other-unit occupancy, resource/technology
revalidation, building slots filling after admission, successful race-specific
commit, dead slots and transactional rejection of malformed pending identities.
These authored worlds exercise the actual engine but are not hardware or
whole-campaign evidence. Save coverage here is in-process host SRAM.

Normal-menu skirmish/save/reload/continued-state checks pass all 72 cases across
Terran, Z6 and P7: both maps, every player race, 1–3 AIs and focused three-AI race
compositions. Four normal-product ARM links with actual prepared briefing and
advisor audio inputs retain the unchanged guards:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `0x0203de14` | `0x03004e20` |
| Z6 | `0x0203de78` | `0x03004f18` |
| P7 | `0x0203dd78` | `0x03004b68` |
| Showcase | `0x0203ded8` | `0x03004fa8` |

Existing race, gas placement/harvest, construction-rendering, blue warp-anchor,
Z7 Colony/morph and Egg fixtures pass after updating their synchronous
foundation assumptions. Source effects and completed-sprite assertions remain.

P7 reaches strict source victory at frame 36,488 with its three heroes alive.
Z6 initially failed because its controller checked for an Extractor and second
Hatchery immediately after placement. Waiting via ordinary neutral input for
their actual foundations restores strict victory at host tick 116,025, frame
114,898, without changing the mission conditions. The current-main `f17a184`
baseline wins at host tick 119,892.

Terran T1, T2, T3, T4, T6, T7, T8 and T10 host controllers pass. T1/T6/T8 need
the same controller-only travel/foundation distinction. T5 also initially failed after
its asynchronous construction helpers were migrated. Differential traces
showed eight SCVs on stalled old MOVE orders in both candidate and current-main
`f17a184`; baseline timing happened to recover enough workers later. Its
controller now recovers one eligible worker through ordinary individual STOP
and MOVE inputs when mining is starved, verifying actual mineral gathering.
It reaches strict victory at host tick 114,497, frame 114,061, with the primary
objective complete and no enemy buildings. The game and victory predicates
are unchanged by this controller recovery.

T9 still reaches its 500,000-tick limit, including on current-main `f17a184`;
it remains unqualified and is not claimed as a new passing campaign here.
None of these results establishes a new Z7 full win, fresh-process SRAM
continuity, physical-GBA behavior or near-30-fps performance. Those remain
separate product and full-campaign qualification work.
