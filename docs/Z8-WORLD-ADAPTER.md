# Z8 typed world adapter

`starcraft/sc64_z8_world_adapter.h` binds the isolated Z8 scheduler callbacks
to a typed view of live actors, buildings, force membership, and resource
wallets. `game.c` includes an opt-in pool-reader helper, but no build profile
enables it and nothing initializes the scheduler or adds a mission profile,
menu entry, save state, or playable claim.

The caller can expose the real bounded engine pools as an ordered
`Sc64Z8ActorView[]`, or provide `actor_at` to read each actual unit/building
row on demand without a parallel snapshot. Each view carries the authenticated
source owner/type, completion state, world-pixel center, pinned source-pixel
dimensions, and a pointer to the actual invincibility flag. The enumeration
order is also the actor traversal order used by `MoveLocation`; the adapter
does not invent a unit/building order. The caller supplies source owner
activity and force IDs, and resource wallet pointers to the engine state.
With `actor_at`, enumerate pool capacity including dead slots and read the
current slot each time. This lets a later trigger in the same pass see an
actor created by an earlier action without allocating a full actor snapshot.
The opt-in game helper does this over `u[]` and `b[]`, preserves each row's
`raw_owner`, resolves source-backed raw type bounds, and points invincibility
at `invincible_units[]` or `invincible_buildings[]`. Its build gate requires
the existing P6/P7 actor facts and Nydus support used by Z8's raw type census.
Do not bind it to packed
`Unit.flags`. Unsupported live actor facts fail the query instead of
substituting approximate dimensions.
The game view marks a live Z7 Egg incomplete until its slot changes to the
paid product type. If transport support is compiled, cargo remains in the
global Command census and is excluded from location-based Bring checks, as in
the existing world-provider behavior. P7 Interceptor hangar state is not
treated as unit incompletion by this bridge.
SetResources can target either a live wallet or independent 32-bit
source-owner mineral/gas values. The game helper binds the latter so source
slots that share a combat owner do not share their trigger resource pools.

The adapter implements bounded Command/Bring censuses, exact/current-player/
all-player/Force selectors, rectangle overlap using source-pixel actor bounds,
and MoveLocation anchor lookup. Kill census comes from an explicit callback
because the base engine has no per-source-type kill ledger. Creation is
dispatched with the resolved source owner, native count-zero-as-one rule,
property selector, and source rectangle. Invincibility changes the live flag;
SetResources reuses `sc64_resource_action_apply_one_player()`. Presentation
and timed actions are dispatched to explicit services.

The opt-in game bridge also provides a bounded default-property CreateUnit
service for raw 74 (Hero Dark Templar) and raw 101 (Special Map Revealer).
It accepts only the descriptor's opcode 11/default-property shape, one unit
per action, and a valid source owner mapping. It uses the native location
midpoint, inclusive footprint edge clamp, and half-scale world coordinates;
it does not grid-snap or run building-only placement collision preflight.
An initialized ordinary native First Strike capture reads bit 0 clear for raw
74 (`0x18410000`) and raw 101 (`0x00000004`) at `0x800C99DC + 4*type`, while
control building raw 154 is set (`0xC4001001`). Snapshot provenance and the
limits of this single-session evidence are recorded in
[`research/z8-native-create-placement.md`](research/z8-native-create-placement.md).
Raw 101 remains a hidden actor; the bridge keeps its exact `raw_owner` and
configured live owner and does not project an enemy source to SIDE to grant
human vision. Enemy AI fog awareness remains unimplemented. This service is
not initialized by a mission profile. The generic action dispatcher still
rejects ambiguous multi-owner Force selectors, so this work does not claim the
authored Force-targeted actions can yet execute through the Z8 scheduler.
The game binder stores its private context in the optional `create_context`
slot, leaving the shared `context` available to AI, presentation, kill-count,
and timed callbacks. If `create_context` is null, existing callers continue to
use the shared context for creation.

`Zer8`, `Ze8T`, and `MvTe` are passed to a required AI service. This adapter
does not implement them; a missing service or failed policy returns an error
to the scheduler. Unknown types, malformed ownership, missing mutable flags,
ambiguous owner groups, unsupported resource operands, and missing callbacks
also fail closed.

Run the asset-free sanitizer fixture with:

```sh
python3 -m unittest discover -s tests -p 'test_sc64_z8_world.py' -v
python3 -m unittest discover -s tests -p 'test_sc64_z8_triggers.py' -v
```

The fixture checks owner/force resolution, completed Command counts, Bring
edge overlap, Kill callback forwarding, actor-order anchoring, property-aware
creation dispatch, live invincibility/resource mutation, presentation
forwarding, and the explicit missing-AI failure. It uses synthetic typed
views; it is not a live-game or mission qualification test. If
`SC64_Z8_DESCRIPTOR` points to the authenticated private descriptor, the world
tests also compare the native Z8 actor census with the bridge's supported raw
unit/building kinds. Without that variable, only the descriptor-dependent
check is skipped.

With the repository's existing private Z7 generated headers available, run
the host sanitizer fixture against the actual game pools with:

```sh
SC64_AUDIO=0 SC64_CONSTRUCTION_ART=0 make test-sc64-z8-game-bridge
```

This test compiles `game.c` and spawns rows into its real `u[]` and `b[]`
pools. It checks every authenticated Z8 actor/building raw kind and bounds,
dead pool slots, neutral owner 11 handling, opcode 43 mutation of the actual
invincibility arrays without changing packed unit flags, distinct source
mineral/gas slots, and Egg-to-product completion. Cargo census assertions run
when `SC64_TRANSPORT_RUNTIME` is part of the build. It also checks default
CreateUnit placement for raw 74 and hidden raw 101, preserved source/live
owners, overlap acceptance for the clear-bit unit path, edge clamping,
unsupported property/type/count rejection, and unit-pool exhaustion. It is a
host fixture with synthetic pool rows. It also binds a distinct creation
context and invokes AI, presentation, kill-count, and timed callbacks to verify
their shared context remains intact. This is not a native mission run or
campaign qualification.

Neither test enables the Z8 profile, scheduler, menu entry, save state, or
mission. Before enabling a profile, a caller must bind source kill accounting,
creation and presentation services, all three authored AI policies, all Z8
actor support, initialization, save/restore, and performance qualification.
