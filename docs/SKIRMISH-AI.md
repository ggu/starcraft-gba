# Custom-game computer opponents

Custom games in the canonical race products (Terran, Zerg, Protoss P7) run
their computer opponents through `race_tick_ai` in
[`race_adapter.h`](../starcraft/race_adapter.h). That function now calls the
planner in [`race_skirmish_ai.h`](../starcraft/race_skirmish_ai.h). The
original-art skirmish planner in `skirmish_ai.h` is a different, older mode and
is unchanged.

The planner is an authored approximation. It is not a decoded StarCraft 64 AI
script, and its build orders, wave sizes and timings are not claims about the
original game.

## What the opponents do

Each opponent plans about every 1.6 seconds (48 updates). Opponents plan on
different updates, so no update runs more than one plan.

**Economy and recovery**

- Idle workers return to the least-loaded mineral patch near their nearest
  owned depot. Terran SCVs that finish construction, Probes after warp-in,
  scouts and pulled defenders all return this way.
- Workers are trained up to two per local mineral patch, plus three per gas
  structure and one builder, within the owner's unit share.
- Gas structures get up to three workers each once the owner has nine workers.
  A large gas surplus moves workers back to minerals.
- Supply is built ahead of need, with a margin that grows with the number of
  producers. Zerg queue Overlords from larva. Up to two supply builds run at
  once after the first few.
- Pending construction is paid on arrival, so the planner reserves the price of
  every walking builder, and the price of the next affordable tech step, before
  training army units.
- A builder that is killed on the way costs nothing, and the structure is
  planned again. A builder still walking after about 64 seconds is released.
- A Terran foundation whose SCV died gets the nearest SCV, which resumes the
  existing construction clock.
- If every depot is lost, the next depot is rebuilt at the owner's start
  location, or at a free start location.
- When the home mineral field drops below 3,000, the owner builds a depot at
  the nearest free start location that still has minerals. Zerg also build
  extra Hatcheries at home as larva sources.
- Structures are placed on rotating lattice rings around the main depot. Placement
  keeps worker lanes to minerals and gas clear, reserves Terran addon spots,
  and keeps a vertical lane open through each producer's fixed unit exit. A
  Factory boxed in by its Machine Shop and neighbors otherwise traps its
  products.

**Race development**

The ordered plans in `race_ai_terran_plan`, `race_ai_zerg_plan` and
`race_ai_protoss_plan` use stock raw IDs. Admission, prerequisites, power,
creep, producer identity and attached addons all come from the same catalog
checks as the human command menu. Costs and build times come from the engine
rule tables.

| Race | Structures | Army |
| --- | --- | --- |
| Terran | Barracks ×3, Refinery ×2, Factory ×2 with Machine Shops, Academy, Engineering Bay, Missile Turrets, Armory, Starport, Command Center | Marines and Firebats; Siege Tanks (Tank Mode) from Factories with a Machine Shop, otherwise Vultures; Goliaths after the Armory, favored against air; Wraiths |
| Zerg | Spawning Pool, up to five Hatcheries, Extractor ×2, Hydralisk Den, Lair (morph), Evolution Chamber, Spire, Creep Colonies morphed into Spore and Sunken Colonies | Zerglings, Hydralisks (favored against air or after eight Zerglings), Mutalisks, Scourge against air |
| Protoss | Gateway ×4, Assimilator ×2, Cybernetics Core, Forge, Photon Cannons, Stargate, Nexus | Zealots and Dragoons (Dragoons favored against air), Scouts |

Addons and building morphs go through `race_skirmish_start_record`. The human
command row uses the same function. It pays from the owner's own bank and
starts the normal construction clock.

A morph to a lower maximum (Creep Colony 400 to Sunken Colony 300) keeps the
Creep Colony type, maximum and hit points while it is incomplete, with
`raw_type` naming the target. It switches when the morph completes and applies
the Brood War difference then: final hit points are hit points at completion
minus 100, clamped to 1..300. Saves derive the maximum from the type, so the
morph's full hit points survive save and reload in every product codec
(`race_construction_deferred_morph`). Previously the structure took the
Sunken type at once and held more hit points than its maximum, which every
save validator rejected.

The planner picks an addon's parent before it reserves anything. The parent
must be ready for the record, its addon footprint must be placeable with a
free building slot, and the addon must not seal the producer's unit exit
(path-grid flood with the predicted footprint). When no parent qualifies, the
step is skipped and holds no money.

**Military**

This stage keeps the existing military rule: once an opponent has four armed
units, every armed unit attack-moves to the hostile structure nearest its
start location. Orders are spread over a 5×3 block at 16-pixel pitch, and
each unit's first route search is delayed by up to 11 updates using the
engine's own retry wait. Idle units still return fire through the engine's
ordinary idle engagement.

## Knowledge and cheat rules

- **No resource or production cheats.** Every structure, addon, morph and unit
  is paid from the owner's bank through the shared services and takes its
  normal build time. The planner never creates actors or changes progress.
- **Full knowledge of hostile actors.** The engine keeps exploration only for
  the human player. Opponents therefore read hostile positions, air and army
  counts, and defenses directly. This differs from StarCraft's fog of war and is an authored
  simplification.
- **Free-for-all.** Every computer player is hostile to every other player,
  following the existing `owners_hostile` rules.

## Reference basis

| Area | Status |
| --- | --- |
| Unit and building costs, build times, supply, prerequisites, producers, addons | Engine rule tables and the raw-ID catalog. The catalog is checked against pinned BWAPI UnitType data and the Blizzard compendium ([catalog notes](SC64_SKIRMISH_ROSTER_STATUS.md)). |
| Script inventory | The private SC64 AI catalog (BOLT leaf `012/000`, see [SC64-AI-CATALOG.md](SC64-AI-CATALOG.md)) lists the melee custom scripts `TMCu`, `ZMCu`, `PMCu` and the Brood War `TMCx`, `ZMCx`, `PMCx`. This shows that SC64 ships per-race melee programs. Their bytecode semantics are unaudited, so nothing here is decoded from them. |
| Plan shape | Authored. The plan follows the general structure that public AIScript documentation (PyMS) gives for stock melee scripts: build orders, defense builds, attack preparation and waves. No program content is copied. |
| Recovery and placement | Authored. |

## Missing engine capabilities

The planner uses only products that work in the canonical products. The
capability audit (`build/test-sc64-skirmish-ai-progress-<profile> --audit`)
prints how each catalog row maps in a profile. Rows the planner skips, and
why:

| Missing capability | Effect on opponents |
| --- | --- |
| Nearest-depot cargo return (`SC64_NEAREST_RESOURCE_DEPOT`) is off in canonical products. Cargo goes to the owner's first completed depot. | An expansion mines inefficiently until the main depot is gone. Expansions are taken only when the home field runs low, or as recovery depots. |
| Zerg larva products are queued serially per Hatchery in these products. Parallel eggs exist only in the Z7 runtime. | Zerg production capacity comes from extra Hatcheries. Queues are capped at two items. |
| Per-owner weapon and armor upgrades: unit levels apply only to the human. | No Forge, Engineering Bay, Evolution Chamber or Armory research. Those buildings are built only for defenses and unit prerequisites. |
| Medic healing, Queen spells, High Templar, Science Vessel, Defiler and Dark Templar products | Not trained. They have no working ordinary combat role in custom games. |
| Reaver (Scarab ammunition), Carrier and Interceptors, Arbiter, Observer, Shuttle and Ultralisk ordinary production (producer `255` or explicitly rejected) | Not trained. Robotics, Templar, Fleet Beacon and Ultralisk Cavern paths are omitted. |
| Unit morphs: Lurker, Guardian, Devourer, Archon | Not available. The Hive and Greater Spire morphs have no payoff and are skipped. |
| Siege Mode transition and Siege research | Tanks fight in Tank Mode only. |
| Bunker loading by computer players | Opponents do not build Bunkers. |
| Comsat Station, Nuclear Silo, Covert Ops, Physics Lab, Valkyrie, Corsair | Unmapped or unavailable in the catalog adapter. |
| Per-owner fog of war | Opponents have full knowledge of hostile actors (see above). |

## Budget and saves

- **No new saved state.** Everything is derived from units, buildings,
  minerals, banks and `frame`. The only reused field is a walking builder's
  `work_timer`, which every existing unit codec already stores. It counts the
  owner's planning passes, up to 40, while the builder walks. A saved match
  therefore resumes with the same decisions.
- **Bounded work.** Each plan does a fixed number of linear scans over the
  unit, building and mineral pools. Supply totals come from the same census
  scan rather than separate scans; the queue service still checks admission.
  A plan runs at most one placement search. It examines one of six lattice
  rings, rotating by plan, and sends at most eight candidates to the
  expensive placement services.
- **Unit share.** Each opponent stops training at `(MAX_UNITS - 16) / (bots +
  1)` units, counting queued products, and never above 56. Buildings have a
  matching share. This leaves the human at least the same share as each
  opponent and bounds combat and route work.
- The planner adds no RAM. Its functions are `noinline` and are placed in ROM.

## Validation

`tests/sc64_skirmish_ai_progress.c` builds once per canonical race product
(`make build/test-sc64-skirmish-ai-progress-{terran,zerg,protoss}`). It
launches a match through the normal Custom Game menu with ordinary inputs and
a passive human, then:

- `--audit` prints the catalog capability matrix for the profile.
- `--map M --human R --bots RRR --ticks N --every K` writes JSON snapshots of
  each owner's bank, workers (mining, gas, idle, building), supply, army and
  air counts, and counts of each unit and structure raw ID.
- `--recovery S` runs scenario S: 0, a walking builder is killed; 1, a
  constructing SCV is killed; 2, a foundation is destroyed; 3, the main depot
  is destroyed. Each loss is injected through the engine's `hit_target` death
  path. The scenario passes only after the opponent recovers through its
  normal paid services.
- `--resume-at T --resume-len L` saves through the pause menu at tick T and
  continues for L ticks. It then reloads the SRAM into a reinitialized game,
  continues for L ticks, and requires identical world digests.

`tools/sc64_skirmish_ai_matrix.py` runs the harness across products, maps,
human races and opponent mixes. It validates every record strictly, failing on
missing, duplicate, malformed or out-of-order records.
