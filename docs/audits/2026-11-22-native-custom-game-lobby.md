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

This sample covers named human races and short save continuations.
Keep the feature in the preview while qualifying the Random seat,
busy matches, campaign saves and complete Melee results on the same
source.

| Local receipt | SHA-256 |
| --- | --- |
| 20 ordinary save continuations | `cdb2c3e16eb6310553329cacc14a57586a63ac0837a213019c82abfeb6f30f28` |
| 20 cross-version load and rejection cases | `299e6290b0fe1d43bca291cbc14b93165e89f499fcfdf4cb66cd4b537d821dad` |
