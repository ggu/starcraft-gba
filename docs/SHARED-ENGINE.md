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

The compile-time flag contract covers three mechanics. Building burn is
also unconditional in the shared race engine; its common presentation and
save rules are documented in [BUILDING-DAMAGE.md](BUILDING-DAMAGE.md):

| Capability | Shared behavior | Authored implementation |
| --- | --- | --- |
| Construction HP | New buildings start with partial health and accumulate the remaining HP/shields over construction; morph completion adjusts HP against the new maximum. | [`sc64_construction_hp.h`](../starcraft/sc64_construction_hp.h), used by the shared race adapter. |
| Siege splash | Siege Tank weapon impacts apply the shared splash tiers and damage rules to eligible nearby actors. | [`sc64_siege_splash.h`](../starcraft/sc64_siege_splash.h) and its authored source bounds table. |
| Shield Battery | Mapped Batteries provide paid construction, psi power, command-driven shield transfer, regeneration, cleanup and persisted recharge state. P7's authenticated source properties remain mission-specific. | [`sc64_battery_adapter.h`](../starcraft/sc64_battery_adapter.h), the common race adapter and candidate-only save validation. |

`SC64_SHARED_ENGINE_FLAGS` supplies these mechanics through `RACES_FLAGS`. The
required `SC64_ENGINE_CORE_REQUIRED` marker makes `game.c` reject a race-engine
build that omits any of them. The old `SHOWCASE_SIEGE_FLAGS` override is
retired and Make fails if a caller still supplies it. Thus Showcase, generic
race/unified builds, and the chapter products inheriting `RACES_FLAGS` receive
the same initial mechanic set. The compile contract is in
[`sc64_engine_capabilities.h`](../starcraft/sc64_engine_capabilities.h).

The initial set is deliberately small. It establishes the rule and prevents
these behaviors from drifting between products; it does not prove that
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
| P7 and chapter services | Battery recharge is shared for mapped actors. Observer/detection, Carrier and other later services still have P7/chapter actor and runtime dependencies. | Continue separating those services from mission-only objectives and actors. Keep campaign triggers scoped to their missions; make ordinary service behavior mode-independent where the same actor capability is supported. |
| Repair | Repair order handling is called from the T6/Terran-late runtime path; the specialized repair adapter validates Terran mechanical targets, SCV ownership, costs and persisted work state. | Keep Terran repair rules, but factor common order/state handling so the engine contract does not depend on a particular Terran chapter being active. Validate saved repair work across applicable modes. |
| Transport | The transport runtime is compiled from T5, P5 Shuttle, or Z6 transport flags. Boarding, loaded-actor filtering, and cargo persistence use that runtime state. | Provide one shared transport service for every applicable product and mode; account for actor layout and every save codec. Do not enable cargo display or a carrier identity without a working cargo lifecycle. |
| Zerg eggs and morphs | Independent egg production is gated by the Z7 egg-production feature and has dedicated actor/save validation. Other products use the older sequential Hatchery queue. | Share the lifecycle and persistence contract where independent eggs are promised, including cancel, death, completion and reload behavior. Keep mission-specific Z7 content and objectives separate. |
| Building fire and burn-down | Shared race-engine burn, death cleanup, fractional save state and source damage overlays are implemented; see [BUILDING-DAMAGE.md](BUILDING-DAMAGE.md). | Native N64 burn-handler tracing, random overlay history, source blending and alternate-pose attachment offsets remain unqualified. |
| Shield bars | Shield/HP vitals already use the shared race renderer for selected units and buildings (`game.c`, `draw_unit` and `draw_building_overlay`). There is no separate profile flag for the bars. | Preserve this shared behavior; distinguish it from asset-bank parity and from always-visible battlefield status bars. |

Other incompleteness remains in race-specific abilities, morphs, unit identity,
production, damage rules and actor lifecycle. A mode-specific adapter can be a
valid staging step, but a feature is not fully shared until its engine contract,
state ownership and required persistence work in every applicable product.

## Validation stage

[`tests/test_sc64_engine_capabilities.py`](../tests/test_sc64_engine_capabilities.py)
uses actual Make variable expansion for the normal race products, Showcase,
generic/unified and representative chapter profiles. It checks that the
required marker and the three mechanics reach each compiler vector, compiles a
small asset-free contract probe, and checks that removing a mechanic or
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

## Shared Shield Battery service

The four canonical products use the same recharge implementation for raw 172,
including their custom games and mapped campaign actors. The custom build
catalog uses normal Probe payment, travel and construction; a selected powered
Battery offers a Recharge command and shows its energy. Transfer, regeneration,
power loss and dead-slot cleanup use existing unit/building fields, without a
new global pool or service clock. This is command-driven recharge; autonomous
idle acquisition is not implemented here.

Custom games compare live player ownership. P7 retains exact source identities
and authenticated map-start properties, including its existing recharge order.
Other human campaign actors can retain rescued source identities, while enemy
source identities remain distinct. Mission objectives and persistent mission
unions are unchanged. Carrier, Observer and Arbiter services are separate work.

The shared order occupies previously unused four-bit value 15; established
order values and record sizes are unchanged. New saves mark shield phases with
`SH12` in previously unused state word 26. Markerless non-P7 unit shield phases
convert from 1/320 to 1/1280 points before live-state replacement; P7 already
used 1/1280, and non-shield regeneration fields are left intact. Legacy unused
Battery queue cells are normalized only for an empty queue. New-format malformed
queues, unknown markers and invalid recharge identities reject atomically.
Older binaries do not support the new phase representation; downgrade loading
is not promised.

`make test-sc64-shared-battery` runs the actual game and save fixtures in all
four canonical compiler vectors. The source-art fixture also checks Battery
body pixels and clipping. Prepared-profile ARM links and fresh-process SRAM
continuation are recorded separately in [`VALIDATION.md`](VALIDATION.md).

The plain race/unified build does not compile the extended actor-facts API.
Its native units use the same Battery checks through a capability-aware mobility
predicate. `make test-sc64-shared-battery-plain` verifies this configuration,
including rejection of forged building identities; it is part of
`make test-unified`. Shared Scourge target protection and impact effects likewise
follow their compiled capabilities rather than requiring a campaign flag.
