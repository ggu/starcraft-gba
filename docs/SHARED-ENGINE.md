# Shared engine contract

Core engine behavior must be consistent across Showcase, custom skirmish, and
campaign products. A capability must not exist only because a build is called
Showcase, or because a particular mission happens to be active. Keep mission
objectives, source content and permissions, generated asset banks, save formats,
and memory placement decisions profile-specific where they need to be. Those
choices must not silently remove a core mechanic from one game mode.

This contract follows the project rule in [`AGENTS.md`](../AGENTS.md). It also
keeps three different questions separate:

- **Engine behavior:** mechanics and presentation rules implemented by the
  shared game engine. The same mechanic should be available in every applicable
  mode and product.
- **Mission and content behavior:** objectives, triggers, actors, maps, source
  permissions and imported art. These can remain specific to a scenario or
  profile. Do not widen a mission-active check to enable an objective in a
  custom match.
- **Storage and placement:** save encodings, actor layouts, RAM/ROM placement,
  cache strategy, and generated data selection. These may vary to fit the GBA,
  but a storage constraint is unfinished work to solve or document, not a
  reason to advertise incomplete behavior as shared support.

## Initial contract

The marker guards the `SC64_RACES` engine family. It does not assert that the
separate original-art/Backwater engine has identical building-construction or
Siege Tank semantics. Compare those products before describing the contract as
parity across every engine implementation.

The compile-time flag contract starts with two mechanics. Building burn is
also unconditional in the shared race engine; its common presentation and
save rules are documented in [BUILDING-DAMAGE.md](BUILDING-DAMAGE.md):

| Capability | Shared behavior | Authored implementation |
| --- | --- | --- |
| Construction HP | New buildings start with partial health and accumulate the remaining HP/shields over construction; morph completion adjusts HP against the new maximum. | [`sc64_construction_hp.h`](../starcraft/sc64_construction_hp.h), used by the shared race adapter. |
| Siege splash | Siege Tank weapon impacts apply the shared splash tiers and damage rules to eligible nearby actors. | [`sc64_siege_splash.h`](../starcraft/sc64_siege_splash.h) and its authored source bounds table. |

`SC64_SHARED_ENGINE_FLAGS` supplies both mechanics through `RACES_FLAGS`. The
required `SC64_ENGINE_CORE_REQUIRED` marker makes `game.c` reject a race-engine
build that omits either mechanic. The old `SHOWCASE_SIEGE_FLAGS` override is
retired and Make fails if a caller still supplies it. Thus Showcase, generic
race/unified builds, and the chapter products inheriting `RACES_FLAGS` receive
the same initial mechanic set. The compile contract is in
[`sc64_engine_capabilities.h`](../starcraft/sc64_engine_capabilities.h).

The initial set is deliberately small. It establishes the rule and prevents
these two behaviors from drifting between products; it does not prove that
every historical game service, actor, or visual treatment is now shared.

Construction health changes the meaning of unfinished-building save rows.
New saves carry an explicit construction-semantics marker. Unmarked older
saves remain loadable when no live building is under construction. If an
older save contains an unfinished building or morph, loading fails before
changing the live world; the saved bytes remain intact. Start a new match,
or finish construction using the older ROM and save again before upgrading.
Guessing a migration from the old HP values would risk healing damage or
leaving completed buildings permanently below their intended health.

## Remaining engine debt

| Area | Current boundary | Work still needed |
| --- | --- | --- |
| Separate original-art engine | The required marker is specific to `SC64_RACES`; the original-art/Backwater path does not compile this race-engine contract. | Decide and document which common mechanics apply to that independent engine, then share applicable behavior instead of treating a race-engine flag as universal coverage. |
| Facing, weapon effects, death art | The four canonical Terran, Zerg, P7, and Showcase products use the prepared facing, weapon-VFX/attack-pose, and death-art banks. Generic `RACES_FLAGS`/`UNIFIED_FLAGS` and several early chapter profiles do not use that same presentation set. Z4/P5/P6 carry some of it, but do not match the canonical four. | Make presentation behavior consistent where the same source art is available. Keep missing source assets visible as a content limitation; do not count placeholder rendering as source-art parity. Track asset inputs and product memory separately from the mechanic contract. |
| Ranged approach routing | Canonical products include direct firing-point routing; generic race/unified and some chapter builds do not. It also depends on the packed route configuration. | Share the applicable navigation behavior across products while preserving route-field capacity and measuring worst-case GBA time for each storage profile. Do not make flag presence alone the completion criterion. |
| P7 and chapter services | The Protoss unit services are a shared layer (see below). P6 keeps its own cloak and detection adapter for its Dark Templar and Zeratul, and P1–P5 bind none of the services. Computer players do not train Interceptors, use Arbiters or recharge at Batteries. | Fold P6's cloak adapter into the shared layer and requalify P6. Teach the AI to use the services where the source AI does. |
| Repair | Repair order handling is called from the T6/Terran-late runtime path; the specialized repair adapter validates Terran mechanical targets, SCV ownership, costs and persisted work state. | Keep Terran repair rules, but factor common order/state handling so the engine contract does not depend on a particular Terran chapter being active. Validate saved repair work across applicable modes. |
| Transport | The transport runtime is compiled from T5, P5 Shuttle, or Z6 transport flags. Boarding, loaded-actor filtering, and cargo persistence use that runtime state. | Provide one shared transport service for every applicable product and mode; account for actor layout and every save codec. Do not enable cargo display or a carrier identity without a working cargo lifecycle. |
| Zerg eggs and morphs | Independent egg production is gated by the Z7 egg-production feature and has dedicated actor/save validation. Other products use the older sequential Hatchery queue. | Share the lifecycle and persistence contract where independent eggs are promised, including cancel, death, completion and reload behavior. Keep mission-specific Z7 content and objectives separate. |
| Building fire and burn-down | Shared race-engine burn, death cleanup, fractional save state and source damage overlays are implemented; see [BUILDING-DAMAGE.md](BUILDING-DAMAGE.md). | Native N64 burn-handler tracing, random overlay history, source blending and alternate-pose attachment offsets remain unqualified. |
| Shield bars | Shield/HP vitals already use the shared race renderer for selected units and buildings (`game.c`, `draw_unit` and `draw_building_overlay`). There is no separate profile flag for the bars. | Preserve this shared behavior; distinguish it from asset-bank parity and from always-visible battlefield status bars. |

Other incompleteness remains in race-specific abilities, morphs, unit identity,
production, damage rules and actor lifecycle. A mode-specific adapter can be a
valid staging step, but a feature is not fully shared until its engine contract,
state ownership and required persistence work in every applicable product.

## Shared Protoss unit services

Carrier Interceptor training, flight and projectiles; Arbiter cloak;
intrinsic cloak (Observer, Dark Templar, Zeratul); detection (Observer,
powered Photon Cannon, Overlord, completed Spore Colony); and Shield Battery
energy and recharge are engine behavior of those actors. They live in
[`sc64_protoss_services.h`](../starcraft/sc64_protoss_services.h) and the P7
service adapters, and they run wherever a mode supplies a binding:

| Mode | Owner identity | Saved service clock | Placed-actor data |
| --- | --- | --- | --- |
| Custom games and Showcase scenes | Slot 7 is the human side; slots 1–4 follow the other live owners. | Race save state word 26, tagged; older custom saves load with an empty clock and empty cache words. | None; a placed Battery keeps the engine's starting energy. |
| P7 | The mission's source (CHK) slots. | P7's three reserved bytes. | P7's two authenticated Battery rows. |
| P8, P9 candidates | Each mission's source slots, from its canonical descriptor. | A clock in each mission's payload. | P8's Battery rows, authenticated from its CHK; P9 has none. |
| Other campaign missions | No binding. | — | — |

Mission objectives, triggers, map authentication and source-owner tables stay
with each mission's adapter; no mission-active check is widened. A served
actor's identity uses its source type, falling back to its engine type when
it has none, as elsewhere in the engine. Load preflight validates a candidate
under its own mode and mission. Passive shield regeneration on the native
shield clock stays with the missions that adopted it; it is not part of the
Battery service.

The canonical Protoss, P8 and P9 candidates and Showcase compile the full set;
the Showcase now includes Carrier training and flight, and its scene-0
Carriers start with four Interceptors aboard. That leaves the Showcase 8 bytes
of IWRAM and 256 bytes of EWRAM under the guards, the tightest margins of any
product. The Terran and Zerg products do not carry these Protoss actor
identities. Canonical Protoss's image changes for the generic binding (8
bytes of EWRAM, no IWRAM, same save size); its campaign controller traces
do not.

[`tests/sc64_protoss_services_game.c`](../tests/sc64_protoss_services_game.c)
(`make test-sc64-protoss-services`) runs the actual engine on the canonical
Protoss, Showcase and P8 compile vectors. It checks the bindings, that an
unbound campaign mission runs none of the services, Carrier training, a
stocked hangar and Interceptor launch against an enemy, Cannon detection of
an Observer, Overlord detection of Zeratul, the Arbiter aura, Battery siting
for every owner, recharge, and a custom-game save and reload of the clock,
Battery energy and cache words, including rejected and older saves. On the
Showcase vector it also checks that the scene-0 Carriers launch.

## Validation stage

[`tests/test_sc64_engine_capabilities.py`](../tests/test_sc64_engine_capabilities.py)
uses actual Make variable expansion for the normal race products, Showcase,
generic/unified and representative chapter profiles. It checks that the
required marker and both mechanics reach each compiler vector, compiles a
small asset-free contract probe, and checks that removing either mechanic or
providing only a partial shared override fails. It also checks the retired
Showcase-only override and that optional construction source art is not
required for the engine capability contract.

The five configuration tests passed against 13 actual Make compiler vectors.
They run in the normal `./dev test` aggregate and can also be run with
`make test-sc64-engine-capabilities`.

[`tests/sc64_shared_mechanics_game.c`](../tests/sc64_shared_mechanics_game.c)
uses the actual engine and production flags, without locally enabling the two
mechanics. `make test-sc64-shared-mechanics` passed under AddressSanitizer and
UndefinedBehaviorSanitizer for Terran, Zerg, P7, Showcase, generic race, and
unified profiles. Each profile exercises campaign and custom modes. The
fixtures check construction HP/shield progression and preservation of damage
through completion for all three races, worker release/consumption rules, and
Siege primary damage, friendly and hostile collateral, air exclusion, and
invincibility flags. Save fixtures exercise the real serializer/loader in
legacy and packed codecs: completed legacy worlds load, legacy unfinished
foundations/morphs and unknown markers fail without replacing actors,
buildings or resources, and marked damaged foundations preserve HP/shields
through reload and completion. These are host SRAM checks, not fresh-process
emulator qualification. They do not establish every interruption/morph
lifecycle, mission completion, or visual parity.

The default build and asset-free suite passed locally (654 Python tests, 20
skipped, plus the separate eight-test suite and C checks). A prepared unified
build and its engine/campaign/menu checks passed. The four normal Terran,
Zerg, P7, and Showcase ARM images linked at 32 MiB with unchanged RAM and
stack-reserve guards. Their remaining EWRAM/IWRAM margins were respectively
28,480/536, 28,372/96, 23,828/960, and 200/56 bytes. Rare SCV construction work
was outlined into ROM to preserve the strict IWRAM reserve. Compiler vectors,
source and generated-input hashes, linker maps, and local logs were retained
outside the source tree.

These results qualify the initial mechanics and build configuration, not
global feature parity or a hardware release. Emulated timing, fresh-process
SRAM qualification, complete campaign playthroughs, and physical GBA testing
remain separate checks. The remaining engine services above still need to be
shared and validated.

## Automatic builder ownership

A new automatic construction request must choose an eligible worker before
preferring the current selection. Pending and active constructors, transported
workers, dead actors, and workers of another owner/type are not candidates.
The same predicate protects the common race build entry point and AI worker
selection; arriving pending workers still use their existing arrival checks.
This prevents a second auto-build request from stealing an SCV from its first
foundation. Explicit Move/Stop cancellation remains available. Probes become
available when warp construction releases them; consumed Drones do not.

`make test-shared-builders` checks the actual production compiler vectors.
The fixtures include two simultaneous paid Terran builds, busy-worker
rejection without order/wallet mutation, and the existing all-race travel,
arrival, cancellation, payment and AI checks. `make test-build-worker-default`
covers the original-art auto-builder selection and is in the default suite.
The separate opt-in roster adapter must use the same predicate before it can
be described as fixed; its integration is tracked by PR #147.
