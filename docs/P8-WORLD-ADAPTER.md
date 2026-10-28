# P8 typed world adapter

`starcraft/sc64_p8_world_adapter.h` connects the existing bounded P8 source
callbacks to the typed world services already used by Z8. It is a caller-owned
adapter only. P8 is not wired into `game.c`, a menu, campaign state or saves;
this code does not claim that P8 is playable or qualified.

## Actor and capacity preflight

`sc64_p8_preflight.h` checks a prepared P8 map's dimensions, bounded actor
rows, coordinates and owners, together with the one-shot property-create
plan and the unit, building, mineral and geyser pool limits. It requires
explicit actor and property resolvers and returns an error for unresolved
actors, invalid property selectors, unsupported create triggers or exhausted
capacity. This is a census helper; a future runtime caller must authenticate
the supplied map before using it. It neither spawns actors nor enables P8.
The prepared-descriptor fixture also checks that each create-bearing trigger
has exactly one eligible active source owner; counting each action once is
specific to this P8 plan, not a general multi-owner capacity calculation.

The P8-only bindings recognize source raw 168 as the Stasis Cell and raw 28 as
a distinct Hero Hyperion actor. The raw28 adapter uses native N64 HP, armor,
weapon and shared-art evidence, and raw28's own pinned BWAPI dimensions and
pixel bounds; it reuses the raw12 sprite descriptor only because the verified
N64 raw12/raw28 source-art chains are identical. This clears the prepared
map's actor census and the fixture checks the real result, including the
remaining property and capacity validations. See
[`P8-HYPERION-FACTS.md`](P8-HYPERION-FACTS.md) for fact provenance and limits.

Passing this census does not activate P8 or establish complete combat support.
P8 still has no runtime/menu/campaign/save wiring; native AI, full mission
capacity, renderer and backend behavior remain separate work. Raw28 already
has its own row in the shared stock combat table, and the fixture confirms its
weapon selectors resolve to 23/24. This verifies lookup identity, not full
mission combat behavior or P8 runtime readiness.

With private inputs already prepared, the optional check is:

```sh
make test-sc64-p8-campaign-preflight P8_PRIVATE_ROOT=/path/to/prepared/checkout
```

The fixture runs under ASan/UBSan and includes separate insufficient-capacity
cases for all four pools, plus invalid row, property, create-trigger and
create-actor cases. The default asset-free suite also checks the P8 data
fact-table configuration. No catalog, runtime-loop or save-format activation
is included in this step.

## World services

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
waiting. The P8 scheduler then clears that trigger's waiting bit and retries
the action without the resume marker next pass. It continues with later
triggers in source order; later actions in that same trigger remain behind the
blocked cursor. `sc64_p8_timed_service.h` supplies shared per-owner timer state,
the per-action resume contract, and a presentation callback for
Transmission/CenterView. The caller ticks it once per selected game frame and
owns timer persistence; the P8 pass cadence remains caller-selected.

PauseGame returns immediately after the native handler applies its pause
effect; following actions and later triggers may still run in that pass. The
handler sets a source-pause marker and a bit on the current trigger instance.
The native top-level scheduler skips trigger time and passes for an ordinary
user pause, but continues elapsed-time and wait-timer updates and trigger
passes when the source-pause marker is set. The trigger scheduler's action
table is based at runtime address `0x800C7564`; at action-list completion it
dispatches entry 6 at runtime address `0x800C757C`. In the pinned ROM
(SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`), the table word is at file
offset `0xC817C` and contains `0x8008086C`, the native UnpauseGame handler.
The ROM file offset is not the same as the runtime address. That tail callback
clears the marked trigger bit and source marker. A pending Wait keeps its
trigger from reaching this tail, so its
source pause remains active while the shared timer advances. The timed service
tracks the source marker and per-owner/per-trigger bit, and requires caller
callbacks for both the actual PauseGame side effect and the native tail
UnpauseGame effect. Its game-loop binding, pass cadence and persistence are
still caller-owned. P8 has active PauseGame rows but no explicit UnpauseGame
row. Pause and tail callbacks return applied, ignored, or error. Ignored
represents native global/owner eligibility branches: the source action is
complete, but pause state does not change. An ignored tail preserves both the
source marker and that trigger instance's pause bit. Mute/UnmuteUnitSpeech
remain unsupported:
the authenticated rows are disabled, and the engine has no shared audio gate
that gives speech-mute state end-to-end effect. Conditions reject
unrecognized flag bits and comparisons. Actions reject nonzero reserved
trailing bytes because the shared Z8 action shape has no field that can
preserve them. The authenticated P8 rows currently have zero in those trailing
bytes; this check prevents future data from being silently discarded.

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
