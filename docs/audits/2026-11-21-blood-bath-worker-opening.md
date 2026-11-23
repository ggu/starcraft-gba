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

### Observed retarget during approach

A fresh bounded ares run of the matched southwest Terran start
narrows SCV 69's patch change to observer steps 6,505–6,515.
At all three cuts it has order 85, phase 1, zero cargo, zero
claim and a zero work timer. SCV 71 holds patch 49 throughout,
and SCV 72 waits at that patch. This is consistent with the
alternate search during approach observed at the northwest
start. The later sampled worker has not begun service or
received cargo. The capture does not trace the exact branch
or the candidate ranking for this particular change.
At step 6,505, patch 56 is the nearest other live patch by
squared center distance from both SCV 69 and patch 49. This
sample leaves both ranking origins possible.

The observer qualified three RAM cuts, two frame copies and
unchanged input pins. Its step counter is an observer clock.

| Observer step | SCV 69 target | RAM cut SHA-256 |
| --- | ---: | --- |
| 6,505 | 49 | `34f0aa0c84aef7e012e6aebf2249b555d04e14d49932ba1bfb362588dd4ffc84` |
| 6,515 | 56 | `f097c9fe998b17ae0c01b5db74236ed74f2b626aedaab8d8d5891cb33aedc3d6` |
| 6,525 | 56 | `7a17b6b29fa9020f2adafe5296a866ea4851b53aca0893f7733e34690a99de0e` |

### Shared mineral contention prototype

`SC64_MINERAL_PATCH_CONTENTION_PROBE` checks for contention
at the mineral work radius in both worker order paths. Each
order pass reconstructs one temporary claimant per mineral
slot from eligible workers with remaining work timers. If an
older save contains multiple partial timers on one patch, the
lowest eligible unit slot works first and the other timers keep
their remaining work. A patch claimed during an update becomes
available on the next update after its worker finishes or leaves.
A worker with an invalidated depot cache can hold a claim for one
update while it discovers that no home remains.

An empty waiting worker without partial work can choose a known,
unoccupied, reachable patch. The prototype reuses the existing
depletion search's cluster radius, route check and slot ordering.
It staggers each worker's alternate search across 16 updates.
The worker waits when the search has no eligible result. Gas,
loaded depot returns and the existing opening assignments still
follow their existing rules. Claims need at most one byte per
mineral slot and use the existing saved target, timer and frame.

The authored host fixture covers simultaneous arrivals, old
duplicate timers, no alternate, hidden and unreachable patches,
and loaded return. It also saves and reloads duplicate timers in
the default profile. Run `make test-sc64-mineral-contention`
for the default and Frontier variants.
The same fixture also passes with prepared race inputs, cargo
kinds and nearest-depot selection. That run checks that the
explicit gather flag survives a contention retarget.

| Authored ARM Frontier order workload | Probe off mean cycles | Probe on mean cycles |
| --- | ---: | ---: |
| Four miners at separate patches | 47,330 | 50,878 |
| Sixteen miners at separate patches | 38,551 | 43,587 |
| Twenty-four workers at one patch, no alternative | 34,231 | 39,323 |
| Twenty-four workers at one patch, alternatives nearby | 34,212 | 74,252 |

The emulator measures the order pass over 16 updates per setup
with the game wait states. The first two setups match all sampled
worker state. The last two deliberately differ as workers wait
or retarget. Much of the last increase coincides with new route
and movement work. Use an ordinary gameplay replay before making
any frame-pacing claim. The local timing receipt SHA-256 is
`6f14d292d8991570f05be996ce860df68741779708aa1325e8e7510778345f5f`.

The authored Transit Yard three-CPU replay uses the same title,
setup and ordinary controller tape on Foundry. Both configurations
select map 0, player 0 and CPU factions 1, 2 and 0. Their state
differs by controller update 2,000. The baseline wins at game
frame 14,727. The contention build wins at game frame 12,502,
between controller updates 12,660 and 12,680. Later inputs on
the full tape start a new game in the contention build.

Through controller updates 100–12,000 the baseline averages
29.446 emulated updates per second and the contention build
averages 29.518. Gameplay diverges during this window, so the
cadences describe the two runs and leave the isolated runtime
cost to the authored order-pass fixture. The baseline receipt
SHA-256 is `a17fcbf041ccef18522ee21335482e349782751e100e0b92b020502be394d7db`.
The finer candidate victory receipt is
`3b7f303aadb28f2b98868fe6d8ee3ae3841d369873289ea7068cc6c3b8525d2b`.

The prototype stays opt-in. Qualify the opening with duplicate
valid targets, exact native alternate ranking, ordinary saves
and reloads, campaign income, and busy Custom Game timing before
enabling a shared release flag.

## Formation experiment

The [opt-in source](https://github.com/ggu/starcraft-gba/commit/8dcd0f3934ed8e7c8f2364afaa57d4ce6dc1bc9f) changes `race_skirmish_init` after imported map and seat resolution. The human Terran starts at the observed half-scale hall offsets in Blood Bath Free for All. The four workers have reachable, distinct fields and legal `ORDER_MINE` rows with zero cargo and work timers. The northwest start uses the observed settled targets. The other orientations retain their current distinct-patch order. If an expected patch or route cannot be resolved, initialization uses the existing opening. The path query follows current hall placement and grid invalidation. Verify the other seat orientations on ARM and qualify Zerg and Protoss independently before widening.

Keep this comparison's worker timer and route predicates identical across both ROMs. A fresh-start-only change is save-compatible without a schema change. `race_campaign_init` calls the skirmish initializer only in custom play. `race_load_game` instead restores map geometry, then reads the existing worker and building rows. Preserve the established `ORDER_MINE` invariant in `race_save_adapter.h`: every saved mine target must be a valid mineral slot. Native phase 0 has a null target, so the GBA experiment resolves that transient state before publication. Existing `SC64_WORKER_GATHER_PENDING` identifies explicit orders and permits ordinary manual gathering without a depot. Its semantics should stay intact.

A subsequent exact contention experiment can start with the native duplicate first targets after resolving them to valid GBA slots. Give the patch a one-worker service rule in an explicitly saved new-match mode. An empty at-contact worker with `ORDER_MINE`, the same target and a positive saved work timer owns service. An arriving worker can reassign to a reachable open patch or wait without accruing work, carrying resources or changing the wallet. Cancel, depletion, a departing worker and a missing alternate need replay checks. The workers' existing order, target and timer can represent service across save/load. A match-mode marker should distinguish this logic from existing custom saves, including saved matches on Blood Bath. The current `state[23]` parsers validate their tag bits, so such a flag needs an explicit encoding and legacy-decoder test.

## Contact follow-up

Native workers approach outside the combined collision bounds. In the retained northwest sample the Command Center's top extent is 41 source pixels, the SCV extent is 11 and separation adds 1. The returning SCV's goal at (122, 187) is 53 pixels north of the hall center. A south approach to a mineral with 15-pixel bottom extent keeps the SCV 27 source pixels south of the patch. The GBA currently has a 32-pixel center circle for depot credit and a 7-pixel center circle for minerals. Native footprint contact is a separate experiment. It needs a consistent reach predicate in both resource-work branches of `race_update_resource_worker`, matching contact goals in `sc64_route_depot_next` and `sc64_depot_cell_point`, and matching credit eligibility. Use source-space integer comparisons to account for odd source widths at half scale. Keep route eligibility and credit geometry consistent.

The native worker reach helper at `0x801401f4` expands each side by
the opposing worker extent plus one source pixel. A local fixture
projects the loaded SCV onto the nearest clear side of that rectangle.
It reaches the native north and east points in two of four captured
N64 departures. The two other north approaches end on the north edge
at different horizontal coordinates. Starting from the actual GBA
east miner, the candidate reaches (99,93) and the native arrival is
(99,102). Both points use GBA pixels. Each path credits exactly eight
minerals and the fixture repeats them through save and reload with the
departure pinned. This isolates geometry and leaves route selection open.
The observation SHA-256 is
`1edc3252b3baeeb15d34bd9dd829ab681abbf09c4996e0b5eb40eff5d8d80aa8`.

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

### Shared Terran opening trial

The shared initializer tries a four-worker row beside each Terran Command
Center, for human and computer seats and in both Custom Game types. The row
must fit the live path grid and offer a route to each worker's starting field.
The prior placement remains the fallback for a blocked row. The four initial
jobs still use the existing nearest-unused field order. The change operates
only during initialization and the save codec stays the same.

An ordinary Blood Bath Melee launch placed the human Terran southwest and
one Zerg CPU northeast. At the first update, both GBA versions had 50
minerals and assigned the same field slots to the four SCVs. The new row
sent each worker on an outbound trip before the existing 45-update gather
wait. The first eight-mineral delivery moved from update 74 to 99, or
2.470 to 3.303 emulated seconds after the launch cut. First pickups moved
from update 44 for all four to 69, 79, 92 and 103 in slot order.
The matched N64 capture places the first credit next to the 9- and 11-second
elapsed readings. The native workers share a patch in that capture. They
take turns working it and the next worker starts during the first worker's
return trip. The new GBA row still sends the workers to four separate fields.
Its warm-window bank samples range from 170–178 at second 19 and 234–258 at
second 28. Patch allocation, service and depot contact still need their own
comparisons. [Matched clock observations](2026-11-22-steady-order-cost.md#matched-blood-bath-credit-and-elapsed-time)

Four fresh HOME continuations matched the uninterrupted state. They cover a
pre-change journal and new journals with workers en route, mining and carrying
ore. Loading and re-exporting preserved each journal byte for byte. All eight
regular and alternate-start profile links retain the previous IWRAM and EWRAM
endpoints. The four-Terran launch costs about 603,200 additional guest cycles
on the launch call. The one-Terran launch adds about 194,980. Those are launch
costs, separate from ongoing gameplay pacing.

The eight sanitizer fixtures passed 234 ordinary race lineups apiece, including
seat and restart checks and first deliveries from all 16 SCVs in a four-Terran
match. The regular cartridges match the previous launch metadata for 1,134
named or closed-seat lobbies and the previous state fingerprints for all 30
campaign openings. All 48 ordinary ARM save/reload checks passed across regular
and alternate-start builds. The Outpost control's full HOST trace is byte
identical before and after the change.

The existing combined-arms HOST policy wins with the formation at call 30,541
and game frame 29,909. The variant that renders every controller frame stalls
at its 75,000-call limit, while that variant wins on the previous build. Its
attack policy stops sending the main force below 30 Marines while any enemy
units remain. A local controller comparison allows the sweep at 12 Marines
once the opponent has at most eight units. The first additional attack issues
orders to 47 actors at call 32,142. The formation build then defeats the Zerg
at call 43,386 and game frame 42,658 with zero Zerg units or buildings.

The ordinary HOME ARM replay follows that rendered-controller tape on
both the original trial and its composition with release `43e91668`.
All 221 recorded observations match across 3,406 input transitions
and 219 action checks. These cover launch configuration, action-time
frame and resources, and terminal unit and building counts. On the
current runtime the human finishes with 39 units and 17 buildings.
The 43,386-call replay averages 21.29 emulated main-loop updates per
second, including menu and presentation time. The earlier controller
tape also wins on the released normal Terran ARM cartridge at call
30,541 and frame 29,909. It matches 192 observations across 3,175
input transitions and 190 action checks, with 83 Terran units and
18 buildings remaining.

The four regular links use the same IWRAM and EWRAM as the current
control. All 1,134 ordinary named and closed-seat lobby observations
and 30 campaign opening fingerprints match. The 24 regular pause-save
and fresh HOME cases match the earlier trial, including world
fingerprints and SRAM bytes.

| Local witness | SHA-256 |
| --- | --- |
| Matched southwest economy and four continuations | `4d8c29ba1c04db3abb931a9f1cb64af5a21fc259b6905737906ec701a5fedc69` |
| One- and four-Terran launch costs | `0d208bfa9b825319dac4bf79ebf21ac181c0e1fe7227526395da435290ebf95f` |
| Regular ARM lobby matrix | `1dae2173c498b742dd3950d4893f573045ae57e11dd5bc71eb473881949e5d9e` |
| ARM 48-case fresh HOME matrix | `de684a464324f6beb0d36eee8d621aeaec7a5230901142325145660110504b5e` |
| Formation HOST victory tape | `6e7dc6f9cf8caca66719ae0ab64dee87f50cc777707b721112ec23bd097fb681` |
| Formation HOST victory with rendered controller frames | `bd48ae2d8221ddccf9e49d6674f039ab0ac49f1be45aea697a6e95e22e57c7c0` |
| Ordinary ARM victory on the original trial | `e8775789f0519ee3c251fbe784214785d95d0471129247ae6b930e9da9202696` |
| Ordinary ARM victory on the current composition | `21d20157196bf66bacb75ed21666e7ece84a78bd474338d8a0569f7805520b68` |
| Earlier controller's ordinary ARM victory | `c2d4a629aaf7126c38389c5a6153dd64c1f83d4e2e8e4531e6ab0e3d6f4a5096` |
| Current 1,134 ordinary launches | `3aa41077157c38253f02ebf0e7f573fcaf73398091fe9de6a9e299f5bd180132` |
| Current 24-case regular fresh HOME matrix | `69d3b681acdd0cc5246ee4ffaf3ac33a4ca69bfdcc7f20f20d221f7d9a6a91bf` |

### Earlier opt-in witnesses

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
