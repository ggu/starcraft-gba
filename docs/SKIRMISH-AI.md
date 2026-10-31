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

**Opening structures**

This stage keeps the existing opening: Terran build Barracks ×2 and a
Refinery; Zerg build a Spawning Pool, an Extractor and a Hydralisk Den, plus
up to two extra Hatcheries as larva sources; Protoss build Gateway ×2, an
Assimilator and a Cybernetics Core. Each race also builds a depot at a free
start location when its home field runs low. Opponents train Marines;
Zerglings, then Hydralisks; and Zealots, then a Zealot/Dragoon mix.

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
| Opening order | Authored, preserving the previous opponent's structure list. |
| Recovery and placement | Authored. |

## Missing engine capabilities

| Missing capability | Effect on opponents |
| --- | --- |
| Nearest-depot cargo return (`SC64_NEAREST_RESOURCE_DEPOT`) is off in canonical products. Cargo goes to the owner's first completed depot. | An expansion mines inefficiently until the main depot is gone. Expansions are taken only when the home field runs low, or as recovery depots. |
| Zerg larva products are queued serially per Hatchery in these products. Parallel eggs exist only in the Z7 runtime. | Zerg production capacity comes from extra Hatcheries. Queues are capped at two items. |
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

- `--builder S` follows a walking builder: 0, saved and reloaded mid-walk,
  it arrives on the same frame; 1, jammed in place, it is released after 40
  passes and its structure is planned again; 2, the same with a save at
  pass 20.
- `--workers S` checks idle opponent workers, since the shared update no
  longer sends idle workers to minerals: 0, a worker stopped with the
  engine's STOP returns to a local field within two passes; 1, the human's
  stopped worker stays idle; 2, a worker whose field runs out moves to
  another; 3, an SCV that finishes a structure returns to work.

`tools/sc64_skirmish_ai_matrix.py` runs the harness across products, maps,
human races and opponent mixes. It validates every record strictly, failing on
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
