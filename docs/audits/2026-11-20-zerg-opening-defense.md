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
four profiles. Four planner-versus-planner matches on the Terran profile,
covering both maps with one and three opponents, end in victory or defeat
between updates 22,530 and 43,374.

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
authenticated BOLT section `00C/001`. The GBA control for this opening
comparison used a 300 HP cap.

The comparison of that HP column against all 228 rows of the PC-derived
table finds two whole-HP differences. The other differences are twelve
zero-HP entries with a native value of one Q8 unit. The second whole-HP
difference is Zealot, at 80 in the native table and 100 in the PC-derived
table. The Zealot is also the only effective shield-cap difference among
the 228 rows, with 80 native shields and 60 in the PC-derived table.
The native shield-enable and shield-cap columns start at offsets 2,472
and 2,700. Completed native Zealots reach 80 HP in both the mixed and
Protoss-only Blood Bath censuses. Qualifying a change requires checking
the profile overrides and the saved HP and shield representations.

## Sunken health and save compatibility

The N64's USA morph order captures current Q8 HP at `0x80154854`,
then adds the target cap and subtracts the previous cap at
`0x801548b0`. The helper at `0x80126e64` applies the upper bound.
Before finalizing the actor the order zeroes its remaining build time,
which bypasses the generic full-HP reset. The incomplete morph skips
construction health growth. Creep and Sunken Colonies both have
102,400 Q8 HP (400 whole HP). Two completed CPU morphs in the native
Melee capture retain the same 102,400 Q8 at VI 20,270 and 20,606.
The instruction path establishes damage retention. The retained
controller capture supplies full-health morphs.

The shared GBA rule uses 400 maximum HP for a completed Sunken in
campaigns, Custom Game and Showcase. The integer and optional Q8
morphs keep the existing current HP, including positive fractions in
the Q8 path. The campaign and skirmish morph clocks and costs are
independent of the cap. The PC-derived reference table retains its PC
values.

Older Terran and Z10 saves explicitly store the Sunken's 300 maximum.
The loader validates that saved record before raising its cap to 400.
P10 and Showcase derive the cap from the saved building type.
The old current HP is retained in all four products. An older Sunken
saved at 300/300 resumes at 300/400, and a damaged Sunken saved at
117/300 resumes at 117/400. Pending morphs keep their source type,
remaining time and current HP. The record layout stays compatible
with earlier saves. An older P10 or Showcase ROM may reject a new
save containing a Sunken above 300 current HP.

| Native USA ROM range | SHA-256 |
| --- | --- |
| Morph completion, `0x118544`, 100 bytes | `9cacb5fc881c1aff6868695fd635577effa2a57ec4f465c6e370bcd54c6f1d2e` |
| HP store, `0xeab54`, 72 bytes | `847295b032dc94fe0b3650a1bf1472f4baa0e3fb58e855d49083f348ac91ecba` |
| In-progress morph, `0x1185e8`, 88 bytes | `83784b12d693730886c93153ca0dd2fd70e64ce9741c9309a21b9e59776691b0` |
| Finish guard, `0xf864c`, 76 bytes | `18dcb0fa008d61b3634815283bc007f19c9a5f486e20a7c28f88490a6011862e` |

## Regression cases

With the prepared profile inputs, build
`build/test-sc64-skirmish-ai-progress-terran`. Run it with
`--map 1 --human t --bots z --opening-defense` and again with
`--bots ztp` for the paid opening. The same test runs with the Zerg,
Protoss and Showcase profile builds. Use `--first-wave` in the
single-opponent lobbies and `--resume-at-morph --resume-at 8640 --resume-len 600`
in the solo Blood Bath lobby for the continuation.

For health retention run `--human z --bots t --morph-hp N` with
`N` from 0 through 5 on each profile, including damage, lethal damage
and in-progress save and load. Run `--human z --bots t --sunken-legacy-hp`
to write and load explicit-cap and typed rows twice. The separate
Z7/Z8 colony tests cover the campaign marker and the Q8 lifecycle test
covers exact fractional HP on completion.

| Evidence | SHA-256 |
| --- | --- |
| Unmodified native solo census | `b3c58140a1d71fdab6e18cb67ddfc7e18c8b62f1e46b49fe004ae935f89dfb39` |
| Unmodified native mixed census | `9ab2e19766f935ad5c30c2c78cb51eda3967c0b6a2724f025ac63e092f5b4ca6` |
| Native Melee spending observation | `6e88f6e2127a1325caac69e92ef826d304037b94172c120d2e9d22d0073d6cea` |
| Terran ARM solo opening and fresh morph load | `76c8475ed55283878a50576577b8089aa47dcd7dccc7702a7bdc0b1c4ea361ba` |
| Terran ARM mixed opening | `7617e8685ef14a10684cff46df52777f5f30596c1160bd620b886ee444a5df8c` |
