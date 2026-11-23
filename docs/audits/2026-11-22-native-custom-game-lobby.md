# Native Custom Game lobby controls

An ordinary USA N64 run in ares reached Setup Custom from reset.
Press Start, select Single-Player with A, return from the initial
tutorial briefing with B, move Right to Scenario and open Setup Custom
with A. Challenger shows Melee, two starting slots, Human Terran and
Computer Zerg. The visible rows cover GameType, Map and the two seats.

On a separate two-slot map, Down twice selected the computer row.
Analog Right changed Computer Zerg to Computer Terran. Further inputs
reached Computer Random and brought the row back to Zerg.

A fresh ares process loaded a retained ordinary Blood Bath FFA lobby.
It shows four starting slots, Human Terran, and three computer rows
set to Zerg, Terran and Protoss. Analog Right on the first computer
row changed it from Zerg to Terran. The sampled lobbies expose no
difficulty field. Other native modes and menus are outside this observation.

The regular GBA race cartridges keep their two-map selection and
independent human and computer races. Their [launch matrix](2026-11-22-command-owner-lookup.md)
exercises one to three CPU choices on each map. The native lobby
observation supplies a reference for the two-slot and four-slot menus.

| Native screenshot | Local SHA-256 |
| --- | --- |
| Fresh Challenger lobby | `0721f9f5cf4d73f6e666ecd15bada3fda734a772bafd504165a193c9ccd004f8` |
| Race changed on a two-slot computer row | `d56695068654e3c07cce37fd942deff5fe9be916a06dfe6c9501b466b0ff1f34` |
| Race changed on the first Blood Bath computer row | `120513ed05f693acffd85ec34a5919c83616d1401347b4fbbbcba400afe14f74` |

## November 23 preview on current cartridge inputs

The released Terran ROM reaches ordinary Custom Game setup without a
Melee selector. With the existing `SC64_NATIVE_MELEE_STARTS` option,
all four profiles on `37e45126` link against the same prepared inputs
with the original memory guards. Zerg leaves 32 bytes below its IWRAM
code limit.

Twenty ordinary launch, pause and fresh HOME cases pass across Terran,
Zerg, Protoss and Showcase. They cover Blood Bath Melee with one, two
and three CPUs, plus three-CPU Free for All on Blood Bath and Outpost.
Each resumes for 120 neutral updates and matches uninterrupted play.

Sixteen cross-version Free for All cases restore the recorded lobby,
game frame, seats and starts. They cover both maps and both directions
between the preview and the regular ROM. The four regular ROMs reject
the preview's Melee saves from HOME. All 20 cross-version cases
preserve the input SRAM when re-exported.

Eight ordinary launches resolve a Random human and a Random CPU with
one Closed seat. Each profile repeats its resolved races and start
order under the same inputs. All 30 campaign openings match the
regular reference after explicitly checking that the added Melee
state is zero. Four saved campaign starts (T10, Z8, P9 and P10)
load to the same state on both cartridges and preserve their SRAM.

Each neutral three-CPU Melee replay reaches RESULT between game
frames 11,523 and 11,704. Across the 11,400 fully sampled updates,
the four profiles average 25.19–27.07 emulated updates per second.
The final full 600-update windows average 16.08–19.70. A full
active win remains a separate check. A pause at update 6,000 and
load from a fresh HOME process produce the same defeat frame,
world state, both video pages, palette and exported SRAM as resuming
the pause in the original process, on all four profiles.

| Local receipt | SHA-256 |
| --- | --- |
| 20 ordinary save continuations | `cdb2c3e16eb6310553329cacc14a57586a63ac0837a213019c82abfeb6f30f28` |
| 20 cross-version load and rejection cases | `299e6290b0fe1d43bca291cbc14b93165e89f499fcfdf4cb66cd4b537d821dad` |
| Random and mixed-seat launches | `de0473b6098fd650339c7e2f27faaea7b00b9b33f877ed8824451dc133e24247` |
| Four legacy campaign save loads | `853448855dc39fdc1f62ad41950075d474dde4f99a6dba9ee7296b6cd7b7bd9e` |
| Four three-CPU Melee runs | `4750d836940d7d906147bd0df5897ca6cf7a7b0c6c2538f989393c13d6041e35` |
| Three-CPU defeats after fresh HOME load | `6838dd54bd2a5bdea0fbc087e3b08b45b8283534fa71aecba0cf71d8cc893340` |

## Instructions in late three-CPU Melee

Ordinary neutral replays on Terran and Protoss match paired
unprofiled runs at 32 cuts per product. Every cut matches game
state, route counters, both video pages, palette and elapsed guest
cycles. The instruction histograms and call stacks balance at
each sampled update.

| Mean guest cycles | Terran updates 9,001–9,016 | Terran updates 11,385–11,400 | Protoss updates 9,001–9,016 | Protoss updates 11,385–11,400 |
| --- | ---: | ---: | ---: | ---: |
| Whole loop | 689,180 | 970,140 | 759,876 | 1,353,772 |
| Orders | 342,700 | 413,254 | 359,536 | 502,125 |
| Uncached contact mask within orders | 111,785 | 91,215 | 47,225 | 53,861 |
| Rendering | 213,032 | 385,028 | 247,108 | 581,950 |
| Depth-ordered actors within rendering | 64,704 | 218,656 | 86,691 | 389,867 |
| Audio IRQ | 14,972 | 43,839 | 17,770 | 119,233 |

The Protoss late window spends 96,720 cycles on tracer drawing and
about 108,071 in the ordinary sprite blitter. Audio's first-order
decoder alone accounts for about 68,018 cycles. The nested values
overlap their enclosing stages. In these late windows both profiles
miss the 30 fps budget. These are emulator measurements.

Three bounded probes preserve the state and display samples. A
per-tile-row visibility cache in the projectile raster changes the
Protoss late mean by about 9 cycles. A zero-width audio decoder
shortcut raises that whole-loop mean by about 36 cycles. A
single-direction contact bucket bound raises the Terran means by
0.58% and 0.34%. Keep all three out of normal builds. Measure actor
and terrain work that can be reused across the two video pages, and
which contact queries recur as units move between cells.

| Local instruction report | SHA-256 |
| --- | --- |
| Terran baseline | `e58feac2b13368c56edcd90ce45590058d733d4a8933495277e13d936a346fa3` |
| Protoss baseline | `c9fb55c51e18157705f615c94e25fce99e56f14c62e60d950202d0d714c2ff97` |
| Projectile fog cache | `63a97f37c0d43edefb5c06a451d47eeab8e257b3824a3501d8ad2ed8c08da88d` |
| Zero-width audio shortcut | `5e5f894297453c646e9365750629f2133439126ede99fd851593ec52a17b4345` |
| Single-direction contact bound | `24c992c419e196db6be74229ac4372a25bea01808439f8a9352bcca9ede9697a` |

## Normal cartridge integration candidate

The four normal build recipes enable the same existing lobby mode
tested in the preview. The proposed build combines it with the
batched target index. All four memory checks pass, with the same
IWRAM endpoints as the standalone preview. The current campaign
opening checks, ordinary skirmish save continuations, cross-version
load and rejection cases, and Random seat checks reproduce the earlier
receipts. The normal Terran candidate reproduces the active Melee
victory through a fresh pause load and a third-process result readback.
