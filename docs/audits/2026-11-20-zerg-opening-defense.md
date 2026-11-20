# Zerg opening defenses on Blood Bath

Two unmodified runs of the owned USA Nintendo 64 ROM use Free for All,
Blood Bath and a passive Terran human. One run has a single Zerg
opponent. The other has Zerg, Terran and Protoss opponents. The values
below are the first samples where each event is visible. They bound
the events at the cadence of the census. The Sunken rows are native
type 146 with their completed bit set. [Native capture method](2026-11-19-n64-skirmish-ai.md)

| Observation | Native solo, game frame | Native mixed, game frame |
| --- | ---: | ---: |
| First Creep Colony foundation | 3,725 | 4,229 |
| Completed Spawning Pool | 4,407 | 4,445 |
| First completed Sunken | 4,881 | 4,933 |
| Four completed Sunkens | 6,115 | 6,001 |

The native runs put down Creep Colonies while the Pool was building.
Each first Sunken reused a Creep Colony position and completed after
the Pool. A separate controller-driven Blood Bath Melee capture follows
two individual Drones through both morphs. Each Drone costs 75 minerals
when it becomes a Creep Colony. The same structure spends another
50 minerals when it starts the Sunken morph.

The GBA planner previously deferred ground defenses until update 18,000.
The shared planner now requests four Sunkens in the opening, after its
initial Pool, expansion and gas steps. The first request becomes eligible
at update 4,656 using the earliest native colony sample and the source
clock. Paid foundation placement and paid morphing use the ordinary
worker and construction services. The planner checks the cost of the
actual action and only holds it when appropriate. A pending Creep Colony
needs no extra reservation while it builds. Early foundation searches use
one planner pass out of five, so other structures still get a turn at
the existing placement budget when nearby sites are blocked.

## Current GBA results

The control source is `fe370f29`. The organic host replay uses the same
Blood Bath lobbies and leaves the human idle. The spending audit observes
four Creep foundation purchases and four Sunken morph purchases by the
first four completions in each lobby, including real Drone movement.
Mineral accounting and supply checks pass.

| Observation, GBA updates | Native solo at 5/4 | Native mixed at 5/4 | GBA solo | GBA mixed |
| --- | ---: | ---: | ---: | ---: |
| First Creep Colony foundation | 4,656 | 5,286 | 5,125 | 5,113 |
| First completed Sunken | 6,101 | 6,166 | 5,894 | 5,894 |
| Four completed Sunkens | 7,643 | 7,501 | 8,294 | 8,294 |

The planner reaches the first defense close to the observed native
timing. Its fourth Sunken remains about 650 to 800 updates later. The
GBA opening has already completed its Pool at update 4,188, so its first
Creep foundation comes after the Pool. Placement, the full tech order and
army composition remain specific to the port.

With one Zerg opponent, the first offensive still leaves at update
17,232. The measured Terran-profile wave sends 30 units with 40 doubled
army supply under the existing all-in rule. The control sends 32 with 52.
In 72 host tests across the four profiles, both maps and all human and
opponent races, each single-opponent first wave still respects its gate.
The host tests also rejoin an organic in-progress morph from SRAM on all
four profiles.

The four regular ARM builds pass the ROM header, SRAM and RAM checks
with the same linked RAM endpoints as the control. All four start the
solo match through the normal controls and show four completed Sunkens
by the update-8,305 sample. The normal Terran ROM also reaches four by
update 8,305 in the mixed lobby. The stock Terran ROM has no completed
Sunkens in either opening through update 9,000. In the normal Terran
solo match, pausing and saving during the first AI morph at update 5,521
reproduces the loaded state and the next 600 updates in a fresh emulator.

In the same sampled windows before the defense is eligible, the stock
and candidate Terran builds match the structure and wallet samples and
average roughly 561,000 cycles per emulated update. On the solo route the
late average is also about 561,000. From update 8,353 to 8,977 the
mixed route goes from 675,949 to 712,632 cycles per update as the AI
workloads diverge. This is a 5.4% cost on that paired input window.
Physical GBA timing remains open.

The native HP table and live samples give a completed Sunken 400 HP.
The value is raw unit 146 in the Q8 HP column at offset 3,156 of
authenticated BOLT section `00C/001`. The current GBA profile uses 300.
Keep the HP and save-boundary check separate from this opening comparison
so the timing and combat effects can be measured independently.

The comparison of that HP column against all 228 rows of the PC-derived
table finds two whole-HP differences. The other differences are twelve
zero-HP entries with a native value of one Q8 unit. The second whole-HP
difference is Zealot, at 80 in the native table and 100 in the PC-derived
table. Completed native Zealots reach 80 HP in both the mixed and
Protoss-only Blood Bath censuses. Qualifying a change requires checking
the actual profile overrides and the saved HP representations.

## Regression cases

With the prepared profile inputs, build
`build/test-sc64-skirmish-ai-progress-terran`. Run it with
`--map 1 --human t --bots z --opening-defense` and again with
`--bots ztp` for the paid opening. The same test runs with the Zerg,
Protoss and Showcase profile builds. Use `--first-wave` in the
single-opponent lobbies and `--resume-at-morph --resume-at 8640 --resume-len 600`
in the solo Blood Bath lobby for the continuation.

| Evidence | SHA-256 |
| --- | --- |
| Unmodified native solo census | `b3c58140a1d71fdab6e18cb67ddfc7e18c8b62f1e46b49fe004ae935f89dfb39` |
| Unmodified native mixed census | `9ab2e19766f935ad5c30c2c78cb51eda3967c0b6a2724f025ac63e092f5b4ca6` |
| Native Melee spending observation | `6e88f6e2127a1325caac69e92ef826d304037b94172c120d2e9d22d0073d6cea` |
| Terran ARM solo opening and fresh morph load | `76c8475ed55283878a50576577b8089aa47dcd7dccc7702a7bdc0b1c4ea361ba` |
| Terran ARM mixed opening | `7617e8685ef14a10684cff46df52777f5f30596c1160bd620b886ee444a5df8c` |
