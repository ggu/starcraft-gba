# Custom-game roster status

This page records the state of the shared Terran, Zerg, and Protoss custom-game roster while the current runtime integration is being tested. The catalog is a reference for the standard StarCraft/Brood War roster; a catalog row is not proof that its unit, building, animation, ability, or production lifecycle is implemented. The custom-game path remains a reduced engine adaptation and does not run the original StarCraft 64 engine.

## What is wired now

The authored raw-ID catalog in [`sc64_skirmish_catalog.h`](../starcraft/sc64_skirmish_catalog.h) describes ordinary units, buildings, addons, building morphs, unit morphs, modes, subunits, and ammunition. The current custom-game menu reads this catalog, and the race adapter checks catalog prerequisites and producer identity before offering a trained product. Worker-built buildings, addons, and building morphs are presented as distinct menu row kinds. Product admission still requires a mapped engine unit with a normal producer; campaign-only records with producer `255` are deliberately rejected by [`sc64_skirmish_adapter.h`](../starcraft/sc64_skirmish_adapter.h). This prevents catalog coverage from silently turning a scenario actor into a functioning skirmish unit.

The product queue is still the engine's bounded queue and unit representation. A catalog row does not itself supply missing movement, combat, morph, spell, cargo, ammunition, or subunit behavior. In particular, catalog modes and morphs are not generic training products. The UI and adapter work is under test; this page does not claim full custom-game roster completion or hardware qualification.

The local integration fixture exercises Barracks and Factory construction,
Machine Shop payment and completion, per-Factory Tank admission, Tank payment
and birth, Probe-built Stargate completion and Scout production, and Zerg
building morph clocks with parallel Drone production and larva preservation.
It passed under host address/undefined-behavior sanitizers for all four normal
profile flag sets. The existing normal-input Blood Bath pylon regression also
passed for Showcase. Four normal ARM products linked within the existing RAM
guards; these checks do not establish a new frame-rate measurement.
The default build/tests and prepared-input unified build/test suite also pass.

## Opt-in identity extension

`SC64_SKIRMISH_ROSTER` appends the ordinary raw identities missing from each
profile, retaining all previous enum values. It is currently **off in normal
ROM targets**. The asset-free rule fixture checks all three races in each
profile: 40 ordinary unit/morph rows and 51 building/addon/morph rows, with
numeric stock facts checked for newly appended rows.

| Compile profile | Unit enum count | Building enum count |
| --- | ---: | ---: |
| Terran | 58 | 66 |
| Zerg | 55 | 64 |
| Protoss P7 | 58 | 65 |
| Showcase | 57 | 65 |

Run `./dev test` for the asset-free checks, or
`make test-sc64-skirmish-rules` for just the all-race identity/rule matrix.
Run `make test-sc64-skirmish-runtime` with the corresponding prepared inputs.
Identity/rule coverage does not supply a missing sprite or behavior. Before
enabling the extension in normal ROMs, qualify the remaining runtime paths,
fact-table generation, imported art, and every active save codec. Expanded
building types use the reserved high save-tag bit when this extension is enabled; default-off tags remain byte-for-byte unchanged. Old opt-in prototypes with ambiguous ordinary tags intentionally reject, and no migration is promised for those prototypes.
The compact and P7 typed building codecs have a seventh type bit in reserved
padding without increasing their row sizes; unchanged old low-type rows retain
their existing field offsets and bytes.

## Sprite and facing coverage

The four prepared normal profile flag sets inspected for this status are Terran, Zerg, Protoss P7, and Showcase. They all enable the authored race sprite adapter and unit-facing rendering, but they do not enable the same campaign actor banks. Unit sprite lookup uses an explicit raw-ID source-frame switch. If no source frame or facing frame exists, `game.c` falls through to its compact authored placeholder drawing; a fallback is not counted as sprite coverage. Facing uses a separate source-frame sidecar and mirrors the 17 source directions to cover the runtime's 32 directions ([`sc64_unit_facing.h`](../starcraft/sc64_unit_facing.h)).

| Race | Standard product raw IDs | Source sprite present in the prepared profiles | Source directional frames | Current qualification |
| --- | --- | --- | --- | --- |
| Terran | 0 Marine, 1 Ghost, 2 Vulture, 3 Goliath, 5 Tank Mode, 7 SCV, 8 Wraith, 9 Science Vessel, 11 Dropship, 12 Battlecruiser, 32 Firebat, 34 Medic, 58 Valkyrie | All listed except 58. The sprite switch has base entries for the other 12. | 0, 1, 2, 3, 5, 7, 8, 11, 12, 32, 34. Science Vessel 9 has a static sprite only; Valkyrie 58 has neither. | Source art presence does not imply a normal skirmish identity or complete animation. |
| Zerg | 37 Zergling, 38 Hydralisk, 39 Ultralisk, 41 Drone, 42 Overlord, 43 Mutalisk, 44 Guardian, 45 Queen, 46 Defiler, 47 Scourge, 50 Infested Terran, 62 Devourer, 103 Lurker | 37, 38, 39, 41–47, and 103 have base frames. 50 has a frame only under the P6 actor gate, present in P7 and Showcase; it is absent from the ordinary Zerg profile. 62 has no source frame. | 37, 38, 39, 41–47, 103. Raw 50 has a facing sidecar in all four prepared banks, but its sprite mapping remains P6-gated. 62 has neither. | P6-gated art for raw 50 does not make the Infested Terran a normal Zerg production product. |
| Protoss | 60 Corsair, 61 Dark Templar, 63 Dark Archon, 64 Probe, 65 Zealot, 66 Dragoon, 67 High Templar, 68 Archon, 69 Shuttle, 70 Scout, 71 Arbiter, 72 Carrier, 83 Reaver, 84 Observer | Base frames exist for 64–70, 72, 83, and 84. Raw 68 also has a static frame. Raw 71 has a sprite only with the P7 actor gate, enabled in Protoss P7 and Showcase. 60, 61, and 63 have no frame. | 64–67, 69–72, 83, 84. Archon 68 has a static sprite but no directional sidecar. 60, 61, and 63 have neither. | P7-gated sprite/actor mappings are not general custom-game support. Carrier and Arbiter still need normal producer behavior. |

The rendered actor path is visible in [`game.c`](../starcraft/game.c) near `draw_unit`: it tries the raw sprite, then the facing bank, then the placeholder path. The source sprite mappings are in the prepared profile's local `sc64_race_sprites_generated.h`; the facing mapping is in its local `sc64_unit_facing_generated.h`. Those generated headers and source assets are private local build inputs and are not included in the source release.

### Building art

Building art is selected separately by `sc64_race_building_art_sprite` in [`game.c`](../starcraft/game.c). The regular race sprite bank contains the standard Terran core set (Command Center 106, Supply Depot 109, Refinery 110, Barracks 111, Academy 112, Factory 113, Starport 114, Engineering Bay 122, Missile Turret 124, Bunker 125). The prepared Terran-late source bank adds Nuclear Silo 108, Science Facility 116, Covert Ops 117, Physics Lab 118, Machine Shop 120, Armory 123, and Control Tower 115 under profile-specific gates. Comsat Station 107 has no source building frame in these banks.

Zerg building frames cover the ordinary buildings except Nydus Canal 134. Protoss has source frames for the core buildings; the P7 building bank adds Arbiter Tribunal 170, Robotics Support Bay 171, and Shield Battery 172 in P7/Showcase configurations. The additional frame banks are conditional and do not establish that every profile can construct or operate those buildings. Static building art also does not imply construction states, addon attachment, or building morph behavior.

## Important capability gaps

- The runtime compact enum and raw-ID adapters are profile-conditional in [`race_rules.h`](../starcraft/race_rules.h). Most early campaign builds do not even allocate runtime identities for later standard products. Terran-late actors, P7 actors, and certain Zerg campaign actors are gated to their campaign profiles.
- Reaver raw 83 is explicitly unavailable from generic custom-game production until its Scarab production/ammunition service exists; a direct-damage substitute is not counted as complete support.
- Nuclear Silo raw 108 remains in the catalog but is unavailable from custom-game construction until its missile ammunition and launch lifecycle exists. A mapped building identity alone must not allow an inert paid add-on. The final integration review also identified other late-tech structures whose payoff depends on still-unavailable actor services; auditing and implementing those paths remains open before promotion.
- Native Scourge now has shared air-only suicide combat and one-larva/two-body production across the four canonical profiles.
- Native custom-game Dropships use shared boarding/unload/death and candidate-save validation in profiles that map raw 11; profiles without that identity remain unavailable. Overlord mission/upgrade restrictions are preserved.
- Several standard catalog products are still not mapped to an ordinary runtime producer, including Valkyrie, Devourer, Corsair, Dark Templar, and Dark Archon. A generated sprite, where present, does not change that. Some P7 actors such as Carrier and Arbiter have scenario identity mappings but are rejected from generic production when their runtime producer field is `255`.
- Siege Tank (Tank Mode, raw 5) is the ordinary Factory product. Siege Mode (raw 30) is a separate mode transition and should not be interpreted as a missing Tank product. The catalog's morph, mode, merge, deployed-subunit, and ammunition rows are reference categories; the generic queue does not implement those transitions.
- The standard roster still needs broad ordinary unit support, including complete special abilities, transport cargo, morph/merge operations, ammunition/subunit lifecycle, and profile-independent raw identities. Existing campaign behavior may implement selected mechanics for particular actors without supplying those mechanics generally in custom games.
- Building prerequisites and menu visibility are only part of the system. Construction, worker consumption, addon attachment, morphing in place, creep/power rules, producer queues, and save/load state need coverage for each newly enabled building path.

## Completion criteria

Treat a standard roster entry as playable only after the engine has a profile-independent identity, correct producer or worker flow, cost and prerequisites, completion and queue behavior, movement/combat or construction behavior as applicable, visibility and sprite behavior, and save/load coverage. Morphs, merges, addons, ammunition, and subunits need their own lifecycle tests. Unsupported entries should remain visibly unavailable until that behavior exists; removing them from the catalog would conceal unfinished work rather than complete the roster.

The fact values in the catalog were checked against the locally prepared pinned BWAPI UnitType metadata and the Blizzard StarCraft Compendium; the project records source provenance in [`SC64-RACES.md`](SC64-RACES.md#references-and-validation). The authored catalog is not copied BWAPI or Blizzard implementation code.

## Current integration qualification

The current-main integration preserves pending/active builder availability before
selected-worker preference. Sanitized input fixtures exercise paid
Barracks → Factory → Machine Shop → Tank Mode and Probe → Stargate → Scout
chains in all four canonical profiles, with explicit synthetic starting funds
and Protoss prerequisites. The production catalog does not yet implement the
Tank Mode/Siege Mode transition or Siege research.

Native save fixtures cover concurrent foundations, addon association, producer
queues, Lair/Hive/Greater Spire morphs with larva/supply preservation and atomic
malformed-row rejection. Default-off profile tags remain unchanged; feature-on
tags reserve the high bit and preserve the profile-specific base identity.
The expanded identity extension remains off in normal targets.

The prior consumed Terran sprite bank was stale and omitted Factory. Current
source preparation includes it. The importer now defers modifier-10 Init shadows
instead of incorrectly flattening them into opaque pixels and expanding body
selection/health bounds. Deferred shadow image IDs and offsets remain in the
private manifest. Separate source shadow rendering is unfinished; body-only
art must not be called complete original-engine visual fidelity.

Frozen-source host qualification now covers 54 normal-input matches across the
three race products, both maps, all human races and one to three rotated AIs:
10 wins and 44 losses, with no nonterminal failures at the unchanged 120,000-tick
cap. Startup/save checks cover 702 configurations, and 27 focused construction
regressions pass. These controllers exercise ordinary opening production and
combat, not every newly exposed product. The separate input fixtures cover the
Factory/Machine Shop/Tank and Stargate/Scout chains.

After adding the shared Scourge gate to the fact-table fingerprint, the P7
product, six native fixtures, 18 matches, 234 startup/save configurations and
nine focused regressions were requalified against the regenerated default-off
table. Native Dropship checks intentionally skip P7/Showcase where raw 11 has
no identity; that is not transport coverage for those profiles.

All four ARM links retain the existing memory guards. Input-only emulator
replays match host terminal state for one native-race, one-AI match in each race
product. Mean advancing-play rates are 28.17 fps Terran, 28.47 Zerg and 26.64
Protoss. Separate 1,000-update late three-AI samples measure 25.12, 22.23 and
25.99 fps respectively. These are distinct workloads, not paired speedup
measurements or a steady-30-fps result. The current full Terran host aggregate
passes all ten mission controllers, startup/save integration, the 234-case
skirmish startup/save matrix, and the associated combat, world and repair
checks. Individual repaired T7/T8/T9 controllers also win twice deterministically;
their probe entry and policy information limits are documented in
[`VALIDATION.md`](VALIDATION.md).

Fresh-process ARM SRAM checks cover native-race human foundations and paid
queues, a P7 mission prefix, and fixture-seeded Dropship unloading and Zerg
building morphs with a paid Drone queue. Whole-program stack bounds, remaining
campaign/normal-menu replay coverage, broader visual checks and physical
hardware qualification remain open. The full standard roster and all 30
campaign stages remain unfinished.
