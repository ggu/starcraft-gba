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
  structure and one builder, within the owner's unit share. Once no mineral
  field anywhere on the map has ore, no more workers are trained.
- While fewer than four workers remain (counting queued ones) and ore is
  left, army production leaves the price of one worker in the bank, so an
  owner that survives a raid on its mineral line can always mine again.
- Gas structures get up to three workers each once the owner has nine workers.
  A large gas surplus moves workers back to minerals, and so does falling
  below eight workers: the last worker never stays on gas.
- Supply is built ahead of need, with a margin that grows with the number of
  producers. Zerg queue Overlords from larva. Up to two supply builds run at
  once after the first few.
- Pending construction is paid on arrival, so the planner reserves the price of
  every walking builder, and the price of the next affordable tech step, before
  training army units. A gas reservation only holds back purchases that need
  gas; workers, supply and gas-free units are still bought. With no ore left
  on the map nothing is reserved for tech, town halls or supply: the bank is
  final and goes to units.
- A Terran or Zerg producer whose chosen unit needs gas the owner lacks trains
  the gas-free line unit instead (Marine, Vulture or Zergling) once 400
  minerals are free. Protoss Gateways already fell back from Dragoons to
  Zealots. With no ore left, the line unit is taken whenever the choice
  cannot be paid for.
- A builder that is killed on the way costs nothing, and the structure is
  planned again. A builder still walking after about 64 seconds is released.
- A Terran foundation whose SCV died gets the nearest SCV, which resumes the
  existing construction clock.
- If every depot is lost, the next depot is rebuilt at the owner's start
  location, or at a free start location. This comes before supply: with no
  depot nothing is mined, and a Protoss owner otherwise spent the Nexus
  price on Pylons to replace the supply the Nexus had provided.
- When the home mineral field drops below 3,000, the owner builds a depot at
  the nearest free start location that still has minerals. Zerg also build
  extra Hatcheries at home as larva sources.
- Structures are placed on rotating lattice rings around the main depot. Placement
  keeps worker lanes to minerals and gas clear, reserves Terran addon spots,
  and keeps a vertical lane open through each producer's fixed unit exit. A
  Factory boxed in by its Machine Shop and neighbors otherwise traps its
  products.
- Protoss structures that need psi power get a second, smaller search when
  the ring around the Nexus has no powered site: the half of an inner ring
  around one of the owner's completed home Pylons that faces the Nexus (the
  map center for Photon Cannons). The Pylon and the radius change from pass
  to pass, and both searches share the same budget of eight expensive
  placement checks.
- A Nexus with nine or more workers and no completed or pending Pylon within
  256 pixels gets a Pylon even when supply is not short. Pylons left at a
  lost expansion otherwise keep the supply cap up while nothing can be
  placed at the remaining base.

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
| Protoss | Gateway ×4, Assimilator ×2, Cybernetics Core, Forge, Photon Cannons, Stargate, Fleet Beacon (Protoss ROM only, see below), Nexus | Zealots and Dragoons (Dragoons favored against air), Scouts, and in the Protoss ROM Carriers |

### Carriers (Protoss ROM)

The Protoss ROM compiles the Carrier service, so a Protoss opponent uses the
same paid products the human does. Other ROMs compile none of it and their
opponents keep the plan above (their bytes are unchanged).

- **Fleet Beacon.** A plan step, after the second Assimilator, once the bank
  holds 600 minerals (`RACE_AI_IF_RICH`) and the Stargate stands. The step
  reserves its price like any other unaffordable tech step.
- **Carrier.** A due Carrier replaces the Scout at an idle Stargate when a
  completed Fleet Beacon stands, fewer than four Carriers exist or are queued,
  the ground line (Zealots and Dragoons) numbers at least four per Carrier
  counting the new one, and the owner's unit share has room for the Carrier
  and its four Interceptors (children occupy unit slots, and the owner's
  share, 56 units at most, binds them). It goes through `race_queue_unit`
  (350 minerals, 250 gas, 12 supply), so supply, power and the actor pool
  apply. The ratio and cap are the whole policy: Gateways keep producing, and
  the Carrier is never taken from money the line needs, except that once the
  gas is in hand, an idle powered Stargate and free supply stand ready, the
  Carrier's price is reserved so the Gateways cannot spend the last minerals
  first. Without gas or supply nothing is reserved.
- **Interceptors.** Each pass the planner orders one Interceptor for every
  Carrier of the owner, through `race_p7_carrier_training_queue_owner`: the
  25-mineral price comes from the owner's own bank, and the service enforces
  its queue limit, the four-slot hangar and the actor pool. Money reserved for
  a structure or another purchase is respected.
- **Combat.** A Carrier counts as an armed, air-capable army unit. It takes
  part in attack and defend orders; with an enemy within eight tiles the
  flight service stops it and launches Interceptors (`sc64_p7_interceptor_flight.h`).
  Hostile Carriers count as ground and air threats.
- **No state.** The planner keeps nothing for any of this: it reads Carriers,
  Interceptors, the Fleet Beacon and the banks from actor storage each pass.
  Work is two more linear scans per pass.

The matrix checks it with `match-carrier` cases (four lobbies, Protoss profile):
besides every match criterion, including the spending audit, which now reads
paid Interceptor orders from the Carrier's queue count, a Protoss owner must
make a Carrier, order Interceptors, have them in flight and have its own shots
in the projectile pool.

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

- Units gather at a rally point a third of the way from the base toward the
  map center.
- No offensive starts before six minutes. After that, an attack starts when
  army supply reaches a threshold. The threshold starts at 12 Marines' worth
  and grows slowly with match time and with hostile static defenses. An owner
  that has used its whole unit share, an owner whose minerals are exhausted,
  and any owner after 25 minutes are all in: they attack with whatever army
  they have, even one below the normal threshold (for example three
  Marines).
- An attack targets the nearest hostile structure to the rally point, with a
  25% preference for depots. When no hostile structure or ground unit remains
  within 64 pixels of the target (the reach at which the engine's attack-move
  acquires a target), the attacking group picks the next nearest structure.
  Aircraft do not hold an objective, so a ground army never waits under one.
- On an exhausted map, an owner with no army, no ore left anywhere and less
  than the price of its cheapest army unit sends its workers to attack after
  30 minutes, so the match can end. While any field has ore the workers keep
  mining it, however far away.
- A group order is spread over a 5×3 block at 16-pixel pitch, and each
  unit's first route search is delayed by up to 11 updates using the
  engine's own retry wait, so a large order does not run every search on one
  update. The first unit takes the ordered point itself, and a slot on
  blocked terrain falls back to that point.
- Reinforcements leave home in groups of at least six Marines' worth (12 in
  the engine's doubled supply); an owner that is all in sends them at once.
  If too few attackers remain, they regroup at the rally point.
- An armed hostile non-worker with a ground weapon within 320 pixels of a
  depot is a threat. A flying one counts only while the home army has a unit
  that can shoot air. Units at home defend. Attacking units are recalled
  only when the home army is outnumbered. If the army is outnumbered near
  the main depot, up to six mining workers also defend, then return to
  mining through recovery.
- One worker visits the nearest other start location at about one minute.

## Knowledge and cheat rules

- **No resource or production cheats.** Every structure, addon, morph and unit
  is paid from the owner's bank through the shared services and takes its
  normal build time. The planner never creates actors or changes progress.
- **Full knowledge of hostile actors.** The engine keeps exploration only for
  the human player. Opponents therefore read hostile positions, air and army
  counts, and defenses directly. The early scout is cosmetic and does not feed
  the planner. This differs from StarCraft's fog of war and is an authored
  simplification.
- **Free-for-all.** Every computer player is hostile to every other player,
  following the existing `owners_hostile` rules.

## Reference basis

| Area | Status |
| --- | --- |
| Unit and building costs, build times, supply, prerequisites, producers, addons | Engine rule tables and the raw-ID catalog. The catalog is checked against pinned BWAPI UnitType data and the Blizzard compendium ([catalog notes](SC64_SKIRMISH_ROSTER_STATUS.md)). |
| Script inventory | The private SC64 AI catalog (BOLT leaf `012/000`, see [SC64-AI-CATALOG.md](SC64-AI-CATALOG.md)) lists the melee custom scripts `TMCu`, `ZMCu`, `PMCu` and the Brood War `TMCx`, `ZMCx`, `PMCx`. This shows that SC64 ships per-race melee programs. Their bytecode semantics are unaudited, so nothing here is decoded from them. |
| Plan shape | Authored. The plan follows the general structure that public AIScript documentation (PyMS) gives for stock melee scripts: build orders, defense builds, attack preparation and waves. No program content is copied. |
| Wave sizes, timings, target choice, recovery, worker defense, scouting | Authored. |

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
| Reaver (Scarab ammunition), Arbiter, Observer, Shuttle and Ultralisk ordinary production (producer `255` or explicitly rejected) | Not trained. Robotics, Templar and Ultralisk Cavern paths are omitted. |
| Carrier and Interceptors in the Terran, Zerg and Showcase ROMs | Not compiled there, so a Protoss opponent in those ROMs never trains Carriers (see [Carriers in the other ROMs](CARRIER-PROJECTILES.md#carriers-in-the-other-roms)). The Protoss ROM does train them, above. |
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
  rings, rotating by plan, plus nine points around one Pylon for a Protoss
  structure that found no powered site, and sends at most eight candidates
  in total to the expensive placement services.
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
- `--threat-census` injects a small synthetic policy fixture: two overlapping
  depots, one or two distinct non-worker attackers, a nearer armed worker,
  and the exact threat-range boundary. It checks both target selection and
  whether miners are pulled. Each attacker counts once across owned depots;
  workers remain outside the documented defense-threat policy.
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

- `--builder S` follows a walking builder: 0, saved and reloaded mid-walk,
  it arrives on the same frame; 1, jammed in place, it is released after 40
  passes and its structure is planned again; 2, the same with a save at
  pass 20.
- `--workers S` checks idle opponent workers, since the shared update no
  longer sends idle workers to minerals: 0, a worker stopped with the
  engine's STOP returns to a local field within two passes; 1, the human's
  stopped worker stays idle; 2, a worker whose field runs out moves to
  another; 3, an SCV that finishes a structure returns to work.

- `--mirror` is a fixture, not ordinary play: the planner also drives the
  human owner, on an update no opponent uses, so a match runs to the engine's
  own victory or defeat. A passive human loses at about six and a half
  minutes, before any mid or late game. Every default run also records the
  ore left on the map, whether each owner can still afford a unit, the first
  update each kind of unit and structure was made and completed, and a
  spending audit: every update, each owner's bank change plus the rule-table
  prices of what it created (structures, addons, morph starts and queued
  units, read from actor storage) must be between zero and the mineral cargo
  its workers delivered. Reserved supply must not exceed the cap when a
  supply-using unit is queued.
- `--audit-control S` hands the passive human one unpaid structure (0) or
  queued worker (1). The audit must flag exactly that update.
- `--repower S`: a Protoss opponent that has expanded loses its army, its
  first Nexus and every Pylon near the surviving Nexus, with supply left
  over. It must start a Pylon near the new home through the paid service.
- `--air-pin S`: an armed flyer hovers over the base of an opponent that is
  all in with three units. With no unit that can shoot air (0) the army must
  still leave and attack; with one (1) it must answer the flyer.
- `--no-ore S`: every mineral field runs dry. The opponent must queue no more
  workers and spend its bank on army units.

`tools/sc64_skirmish_ai_matrix.py` runs the harness across products, maps,
human races and opponent mixes. Its `match` cases run about 18 mirrored
lobbies per product (both maps, each human race, one to three opponents,
same-race and mixed) to a two-hour limit and require: the engine's victory
or defeat; nothing made before its prerequisites, checked against the
engine's own catalog rows; a clean spending audit; development by every
owner; fewer than 8 consecutive minute snapshots with two or more idle
workers while ore remains; and fewer than 15 with 1,500 or more minerals
unless the unit share is used. A match that reaches the limit fails unless
no owner can ever act again (no army, no worker with ore to mine, and no
affordable unit). That is reported as `draw: exhausted` and passes only in
the one `match-draw` case that expects it. `--profiles` and `--kinds`
select a subset. It validates every record strictly, failing on
missing, duplicate, malformed or out-of-order records. A save case whose match
ends before the save point is reported as `no save` with its reason and never
counts as a save.

`tools/sc64_skirmish_sram_continuation.py` checks the same continuation on the
unchanged ARM product in two processes of the pinned port-free mGBA backend
(`tools/sc64_headless_replay.c`). Process 1 starts a custom game from an empty
save with menu inputs, plays with a passive human, saves through the pause
menu and runs the uninterrupted reference. Process 2 loads the exported 32 KiB
SRAM with L on HOME. The restored world must equal the saved world, and the
continuation must equal the reference, compared by a fingerprint of frame,
banks, mineral fields and every live unit's and building's semantic fields.
`--tamper-sram` corrupts the record as a negative control, which must fail.
