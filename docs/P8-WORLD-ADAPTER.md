# P8 typed world adapter

`starcraft/sc64_p8_world_adapter.h` connects the existing bounded P8 source
callbacks to the typed world services already used by Z8. It is a caller-owned
adapter only. P8 is not wired into `game.c`, a menu, campaign state or saves;
this code does not claim that P8 is playable or qualified.

Command and Bring reuse the Z8 pool census, source-owner/force resolution,
completion rules, cargo handling and source-pixel rectangle overlap. Switch and
ElapsedTime conditions read the caller's current switch bank and seconds
counter. The same switch bank receives P8 SetSwitch writes. MoveLocation reuses
the Z8 actor-order lookup and source-pixel recenter/clamp helper. SetResources,
SetInvincibility and non-timed presentation actions dispatch through the
existing explicit Z8 services.

The caller provides the live actor view, mutable source-pixel locations, the
256-switch bank and elapsed-seconds value. The actor view may use `actor_at`
over pool capacity so a created actor is visible to later callbacks in the
same pass. `sc64_p8_world_locations_init()` validates the compact source
descriptor and copies its initial rectangles to caller-owned mutable storage.
These values are runtime inputs; this header does not define serialization or
save compatibility.

CreateUnitWithProperties can dispatch through the existing create callback,
which receives the source action, count, owner and property selector. This is
only a dispatch path: it does not decode or apply P8's UPRP records. The
callback must implement each requested P8 property record exactly and check its
own pool capacity. The adapter does not replace a nonzero selector with
default properties. Missing or rejecting creation services fail closed. The
three P8 AI IDs are passed only to a P8-specific `run_ai` callback; no generic
Z8 AI or attack-move fallback exists.

Wait, Transmission and CenterView require `async_action`. It must implement
one shared timer per source owner and retain an armed marker for each action:
`PENDING` means the action started and must resume at the same cursor;
`BLOCKED` means the action did not start because that owner is already
waiting. The P8 scheduler then clears that trigger's waiting bit and lets
later immediate actions run. This adapter provides no timer storage or save
encoding. A missing callback fails closed.

PauseGame and Mute/UnmuteUnitSpeech have no service in this slice and fail
closed. Conditions reject unrecognized flag bits and comparisons. Actions
reject nonzero reserved trailing bytes because the shared Z8 action shape has
no field that can preserve them. The authenticated P8 rows currently have
zero in those trailing bytes; this check prevents future data from being
silently discarded.

The default asset-free service fixture is:

```sh
make test-sc64-p8-world
```

It covers Command/Bring counts, completion and cargo, owner/force selectors,
half-open geometry, switch and elapsed conditions, MoveLocation, property ID
forwarding and created-slot visibility, resource and invincibility mutation,
presentation, opaque-AI failure, and the pending/blocked callback contract.
Its actors and properties are synthetic. When the authenticated P8 descriptor
is already prepared locally, the optional location-contract check is:

```sh
make test-sc64-p8-world-source-contract P8_SCENARIO_DIR=/path/to/prepared/scenarios
```

The optional check reads the existing descriptor only; it does not pack,
extract or write scenario data. Neither check implements P8's AI routines,
decodes its UPRP records, adds repeat-safe actor reserves, defines a save
format, or proves campaign progression on hardware.

When the already-prepared Z7 runtime inputs are available, the optional
`make test-sc64-p8-game-pool-bridge` fixture checks that this adapter can read
and mutate the existing live pool through the Z8 bridge. It uses the spawned
unit's actual engine raw type. It also verifies that the existing default
property create service rejects a nonzero P8 selector without allocating an
actor; it does not test P8 UPRP application.
