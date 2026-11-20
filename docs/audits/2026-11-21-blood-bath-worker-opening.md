# Blood Bath worker opening and depot contact

The matched passive opening uses northwest Terran at source (128, 240) and southwest Zerg at (288, 1808). The USA N64 Terran starts with 50 minerals and four SCVs south of the Command Center. The SCVs start at (80, 296), (104, 296), (128, 296) and (152, 296). All four have order 85, phase 0, an empty resource target and zero cargo and work timer. Terran offsets relative to its Command Center are the same at all four Blood Bath starts. The southwest Drones also start at the Hatchery, at (248, 1856), (312, 1856), (336, 1856) and (352, 1826). The available formation samples cover four Terran placements and three Zerg placements. Protoss formation needs its own capture.

The GBA opening picks four distinct mineral fields by distance to the hall, places a worker at each patch plus (4, 4), and assigns the mining order in `race_skirmish_init` in `starcraft/race_adapter.h`. The matched unmodified and 102-update wait builds both place the northwest SCVs at (84, 44), (20, 44), (36, 28) and (100, 28) on their first ordinary update, with patch IDs 0, 4, 8 and 12. Half-scale native start positions are (40, 148), (52, 148), (64, 148) and (76, 148). The imported terrain flags mark the measured Terran and Zerg formation cells walkable. Live collision and routing still need fixture coverage.

## Native target resolution

Order 85 resolves a missing target in native code at `0x80132ab8`. It calls `0x80148b60` with the worker sprite's position when the remembered position is empty. It stores the target, gives the worker a movement goal, and enters phase 1. At approach, `0x80132bb4` reads the target mineral's active-gather byte through `0x80131fd0`. A busy field can invoke the alternate search at `0x80132bf8` and replace the target. The matched neutral replay reproduces the change during the first trip.

| Native update | Observation |
| --- | --- |
| 0 | Four order-85 SCVs with no mineral target. Wallet 50. |
| 5 | The SCVs target source patch centers (32, 80), (160, 80), (160, 80) and (320, 176), corresponding to packed GBA slots 4, 0, 0 and 16. |
| 63 | SCVs 61 and 64 mine. SCVs 62 and 63 still approach the same patch. |
| 78 | SCV 62 mines the shared patch. SCV 63 approaches a different patch at (192, 48), GBA slot 12. |
| 109 | All four mine in phase 5 at distinct patches, in slot order 4, 0, 12 and 16. |
| 172 / 188 / 203 | Terran wallet 50 / 58 / 74. The first delivery falls in the interval (172, 188]. |

The 102-update GBA wait probe already shows 82 minerals at update 151. The nominal 5:4 update conversion puts the observed native first-delivery interval at (215, 235] GBA updates. This measures the opening lead. A separate warm-window measurement is needed to assign the remaining income difference to travel or deposit geometry.

## Formation experiment

The [opt-in source](https://github.com/ggu/starcraft-gba/commit/8dcd0f3934ed8e7c8f2364afaa57d4ce6dc1bc9f) changes `race_skirmish_init` after imported map and seat resolution. The human Terran starts at the observed half-scale hall offsets in Blood Bath Free for All. The four workers have reachable, distinct fields and legal `ORDER_MINE` rows with zero cargo and work timers. The northwest start uses the observed settled targets. The other orientations retain their current distinct-patch order. If an expected patch or route cannot be resolved, initialization uses the existing opening. The path query follows current hall placement and grid invalidation. Verify the other seat orientations on ARM and qualify Zerg and Protoss independently before widening.

Keep this comparison's worker timer and route predicates identical across both ROMs. A fresh-start-only change is save-compatible without a schema change. `race_campaign_init` calls the skirmish initializer only in custom play. `race_load_game` instead restores map geometry, then reads the existing worker and building rows. Preserve the established `ORDER_MINE` invariant in `race_save_adapter.h`: every saved mine target must be a valid mineral slot. Native phase 0 has a null target, so the GBA experiment resolves that transient state before publication. Existing `SC64_WORKER_GATHER_PENDING` identifies explicit orders and permits ordinary manual gathering without a depot. Its semantics should stay intact.

A subsequent exact contention experiment can start with the native duplicate first targets after resolving them to valid GBA slots. Give the patch a one-worker service rule in an explicitly saved new-match mode. An empty at-contact worker with `ORDER_MINE`, the same target and a positive saved work timer owns service. An arriving worker can reassign to a reachable open patch or wait without accruing work, carrying resources or changing the wallet. Cancel, depletion, a departing worker and a missing alternate need replay checks. The workers' existing order, target and timer can represent service across save/load. A match-mode marker should distinguish this logic from existing custom saves, including saved matches on Blood Bath. The current `state[23]` parsers validate their tag bits, so such a flag needs an explicit encoding and legacy-decoder test.

## Contact follow-up

Native workers approach outside the combined collision bounds. In the retained northwest sample the Command Center's top extent is 41 source pixels, the SCV extent is 11 and separation adds 1. The returning SCV's goal at (122, 187) is 53 pixels north of the hall center. A south approach to a mineral with 15-pixel bottom extent keeps the SCV 27 source pixels south of the patch. The GBA currently has a 32-pixel center circle for depot credit and a 7-pixel center circle for minerals. Native footprint contact is a separate experiment. It needs a consistent reach predicate in both resource-work branches of `race_update_resource_worker`, matching contact goals in `sc64_route_depot_next` and `sc64_depot_cell_point`, and matching credit eligibility. Use source-space integer comparisons to account for odd source widths at half scale. Keep route eligibility and credit geometry consistent.

## Qualification

Use the existing physical Custom Game driver in `tools/sc64_skirmish_sram_continuation.py`. Launch Blood Bath with Terran and Zerg in opponent row three. Confirm base IDs [0, 3], bank 50, four workers, their formation and distinct initial positions. Hold neutral and observe each worker's target, work timer and cargo through a first paid trip. For the contention experiment, require the stock duplicate target before approach and an actual reassignment or wait before its timer starts. Capture the first eight-mineral credit and compare the GBA before/after at matched nominal update brackets.

Save during outbound travel, during work, and while carrying ore. Fresh HOME L must restore the saved state and agree with its uninterrupted path through a deposit. Load an original Blood Bath save on both ROMs and compare its continuation. Retain the same opening and continuation on the compact map and a short Z1 or T1 campaign checkpoint. Before promotion, run the ordinary custom matrix on the normal Terran, Z10, P10 and Showcase products, plus the affected AI opening ledgers and the existing worker-cycle and pending-gather save fixtures in `tests/sc64-worker-cargo/Makefile`.

### Measured Terran formation trial

The opt-in `SC64_NATIVE_BB_TERRAN_OPENING_PROBE` stages the formation
on the normal Terran source vector at `0034ebd4`. At the ordinary
Custom Game launch the four worker positions and settled northwest
target slots match the half-scale observation. The trial keeps the
existing 45-update gather wait.

| Matched northwest start | Stock Terran | Formation trial |
| --- | ---: | ---: |
| First mineral pickup, GBA update | 45 | 98 |
| First eight-mineral credit, GBA update | 75 | 141 |
| First credit per worker, in spawn order | 75, 82, 82, 89 | 142, 152, 169, 141 |

The earliest credit moves 66 updates toward the native timing. The
native nominal GBA-scale bracket remains (215, 235]. Measure starting
contact and patch contention before changing the gather timer on the
normal ROM. The trial uses distinct settled targets throughout its
first trip.

The HOST fixture passes 234 ordinary menu configurations and checks
the formation, valid targets and reload at all four Terran starts.
In ARM gameplay, ordinary Start/B saves at frame 30 from the original
and formation builds both reload on the trial through frame 340. Each
matches 13 uninterrupted world and worker samples and all observed
mineral credits. The IWRAM and EWRAM link endpoints are unchanged.
Keep the flag off by default while broadening the ordinary replay.

| Local witness | SHA-256 |
| --- | --- |
| N64 USA ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| Matched step-zero native snapshot | `ed9225feb4caa29de200475af24c435c85acddabdc2c9e352857bc8732f30fcd` |
| Neutral native worker handshake observations | `8ae068ed4c0994e09be558a4a77efae362f12b06ce70444045033e8736eada52` |
| Startup formation receipt | `c58714121aa59e2ba4ed7d6ae1057054bd4016f6e2af53eeeefb4e02d623b812` |
| Matched patch and depot geometry receipt | `15e90307e77d5ab3f6ac0e752b09dd8cb8aff0d244bb7e8be5d4f13b65dc3d66` |
| Native order-85 missing-target block, ROM `0xf6750`, 156 bytes | `78c2fb295dc9fed1de8a3b285864ef4ec2c7244df8a3ea9c5ab54f73e82b3017` |
| Native order-85 busy-field branch, ROM `0xf68a4`, 144 bytes | `922a7c84a5c2a7f509660d2dc3fc86598e713feb4cd7eed0980d164d0476938e` |
| Formation trial and both ARM fresh continuations | `f784df21abdafb830c394cb63150ee7a4cf870040de2ebfabdb656e23e058e98` |
