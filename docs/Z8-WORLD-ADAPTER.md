# Z8 typed world adapter

`starcraft/sc64_z8_world_adapter.h` binds the isolated Z8 scheduler callbacks
to a typed view of live actors, buildings, force membership, and resource
wallets. It is not included by `game.c` and does not add a mission profile,
menu entry, save state, or playable claim.

The caller refreshes `Sc64Z8ActorView[]` from its real bounded engine pools
before each scheduler pass. Each element carries the authenticated source
owner/type, completion state, world-pixel center, pinned source-pixel
dimensions, and a pointer to the actual invincibility flag. The array order is
also the actor traversal order used by `MoveLocation`; the adapter does not
invent a unit/building order. The caller supplies source owner activity and
force IDs, and resource wallet pointers to the engine state.

The adapter implements bounded Command/Bring censuses, exact/current-player/
all-player/Force selectors, rectangle overlap using source-pixel actor bounds,
and MoveLocation anchor lookup. Kill census comes from an explicit callback
because the base engine has no per-source-type kill ledger. Creation is
dispatched with the resolved source owner, native count-zero-as-one rule,
property selector, and source rectangle. Invincibility changes the live flag;
SetResources reuses `sc64_resource_action_apply_one_player()`. Presentation
and timed actions are dispatched to explicit services.

`Zer8`, `Ze8T`, and `MvTe` are passed to a required AI service. This adapter
does not implement them; a missing service or failed policy returns an error
to the scheduler. Unknown types, malformed ownership, missing mutable flags,
ambiguous owner groups, unsupported resource operands, and missing callbacks
also fail closed.

Run the asset-free sanitizer fixture with:

```sh
python3 -m unittest discover -s tests -p 'test_sc64_z8_world.py' -v
```

The fixture checks owner/force resolution, completed Command counts, Bring
edge overlap, Kill callback forwarding, actor-order anchoring, property-aware
creation dispatch, live invincibility/resource mutation, presentation
forwarding, and the explicit missing-AI failure. It uses synthetic typed
views; it is not a live-game or mission qualification test. Before enabling a
profile, a caller must bind real pools, source kill accounting, creation and
presentation services, all three authored AI policies, Z8 actor support,
initialization, save/restore, and performance qualification.
