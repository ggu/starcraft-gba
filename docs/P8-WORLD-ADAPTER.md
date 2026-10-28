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

CreateUnitWithProperties with selector zero uses the existing default-property
create service. Nonzero selectors use the optional
`create_with_property` callback only after the adapter validates the one-based
UPRP selector against the authenticated UPUS bitmap. The callback receives the
source action, resolved owner, count, selector, location and immutable
20-byte property record. This is a dispatch contract: the adapter does not
interpret or apply override fields. A callback must implement every requested
field exactly and check its own pool capacity; missing or rejecting services
fail closed. The optional source-contract test checks the authenticated
descriptor's create selectors against UPUS without emitting property rows.
The three P8 AI IDs are passed only to a P8-specific `run_ai` callback; no
generic Z8 AI or attack-move fallback exists.

The opt-in `sc64_p8_game_bridge.h` implements the authenticated P8 Create rows
for selectors zero through three. It reuses the P8/Z8 source-owner mapping,
source-rectangle midpoint/clamp and live `spawn_unit()` / `spawn_build()`
paths. The nonzero path accepts only the three exact authenticated records,
applies their 100% HP/shield/energy overrides, keeps the resolved action owner
when the source owner override is invalid, and maps the enabled 0x10 status
value to the engine invincibility sidecar. Any record drift or unsupported
field fails before pool allocation. This is a source-derived create service;
it does not add a general-purpose UPRP interpreter.

The native evidence is the pinned generated call path: the setup loader copies
the 0x500-byte table to `0x800C7654` and normalizes 64 records of 20 bytes;
the runtime property path indexes `0x800C7640 + selector * 20` and passes the
selected row to the property application helper. The helper dispatches HP,
shield, energy, resource and hangar values from their field-valid masks and
separately applies status bits. Its 0x10 status handler sets actor state bit
0x20. In P8, only HP, shields and (for raw unit 79) energy are enabled; resource
and hangar overrides are not enabled. The owner field is invalid in all three
rows, so the native action's resolved owner is used. These findings justify
the narrowly bounded P8 decoder but do not establish other maps' property
records or a reusable general decoder.

The game has no general energy register, but `Unit.carry` can store Q8 energy
for raw Tassadar, as used by the P4/P6 storm adapters. P8's raw-79 create
initializes that field to the native 100% maximum. P8 currently has no cast or
regeneration service wired to that value; this slice claims only the
create-time state, not usable spell energy or spell behavior. P8 AI remains
separate unsupported runtime work.

Wait, Transmission and CenterView require `async_action`. It must implement
one shared timer per source owner and retain an armed marker for each action:
`PENDING` means the action started and must resume at the same cursor;
`BLOCKED` means the action did not start because that owner is already
waiting. The P8 scheduler then clears that trigger's waiting bit and lets
later immediate actions run. This adapter provides no timer storage or save
encoding. A missing callback fails closed.

PauseGame and Mute/UnmuteUnitSpeech have no service in this slice and fail
closed. In the authenticated P8 descriptor the Mute and Unmute rows are
disabled; the engine also has no shared audio gate that gives the existing
speech-mute state end-to-end effect. Conditions reject unrecognized flag bits
and comparisons. Actions reject nonzero reserved trailing bytes because the shared Z8 action shape has
no field that can preserve them. The authenticated P8 rows currently have
zero in those trailing bytes; this check prevents future data from being
silently discarded.

The default asset-free service fixture is:

```sh
make test-sc64-p8-world
```

It covers Command/Bring counts, completion and cargo, owner/force selectors,
half-open geometry, switch and elapsed conditions, MoveLocation, one-based
UPUS/property-record forwarding and created-slot visibility, resource and invincibility mutation,
presentation, opaque-AI failure, and the pending/blocked callback contract.
Its actors and properties are synthetic. When the authenticated P8 descriptor
is already prepared locally, the optional source-contract check is:

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
unit's actual engine raw type. The generic Z8 default-create service still
rejects nonzero P8 selectors; P8 uses its separate property-aware service.

With both the authenticated P8 descriptor and prepared Z7 inputs available,
`make test-sc64-p8-live-create P8_SCENARIO_DIR=/path/to/prepared/scenarios`
checks the descriptor's selector-zero Create actions against the real bounded
unit/building pools. It reaches all used nonzero selectors, checks HP, shields,
energy, owner and invincibility results, and verifies malformed property data
fails without mutating either pool. The target is optional and does not
generate its inputs. It does not test P8 mission AI, save/load, spell behavior,
or campaign victory.
