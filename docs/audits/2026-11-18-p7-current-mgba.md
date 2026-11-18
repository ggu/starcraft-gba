# P7 combat timing on the current Protoss cartridge

At release `0e536f26`, the standard Protoss P10 build and the build with
`SC64_TERRAIN_FAST_VALIDATE` used the same prepared local inputs and compiler
recipe. The alternate build added one definition to that recipe. Both ROM
hashes match the existing terrain-cache qualification binaries.

The pair used the retained ordinary-input P7 workload from the
[November 6 audit](2026-11-06-current-native-qualification/README.md). Both
started from the title, entered mission 26, and acknowledged three dialogue
pages with physical controller presses. The window covered zero-based tape
calls 8,020–9,019. Of those, 958 advanced the game. Forty-two calls stayed
within gameplay screens without advancing the simulation. The table counts
the advancing calls and includes the wait until the next input sample.

| mGBA timing | Standard | Terrain-cache preview |
| --- | ---: | ---: |
| Mean cycles per update | 2,070,368 | 2,027,375 |
| 95th-percentile cycles | 2,386,093 | 2,340,040 |
| Worst update cycles | 2,693,754 | 2,644,006 |
| Mean updates per second | 8.10 | 8.28 |
| Mean unit-order cycles | 985,150 | 985,349 |
| Mean map-render cycles | 468,976 | 423,199 |

All 11 sampled checkpoints matched, including sampled world state, mission
and input mode, visible and explored grids, and the complete displayed
framebuffer and palette. Every measured call retained the same game update,
mode, and classification. The mean total falls 2.1%. Unit-order work alone
exceeds the 561,792-cycle two-VBlank budget. The release keeps the preview
flag off. This measurement covers one P7 combat window in mGBA.

The order diagnostics reproduced all 11 world and video samples and the
same 958 advancing calls. The instrumented loop adds 2.0% to mean cycles.
On that same workload, setup averages 397,019 cycles, movement 392,902,
and target queries 84,697. Arbiter cache processing accounts for 269,588
cycles inside setup. Its nested counters attribute 118,361 cycles to aura
coverage and 116,849 to detection within the 150,145-cycle cache update.
The second diagnostic read reproduced the same 11 checkpoints again.
The largest movement sample was 1,283,089 cycles at tape call 8,347,
game frame 8,101, the only call where the counted route-build total increased.
Movement still averaged 391,975 cycles on advancing updates with a known
unchanged build count. Lazy materialization can do route work without
incrementing that count. Split route filling, lookup and local steering
costs before choosing the next movement optimization.

An opt-in aura eligibility prototype resolved the service binding once per
source update and reused each candidate's source type. The independent
HOST oracle matched across 240 worlds and 420 updates per world. The
paired ARM window matched all 11 state and display checkpoints. Mean order
cycles fell from 985,150 to 969,136 and mean full-loop cycles fell from
2,070,368 to 2,054,599 (0.76%). Keep this as a profiling experiment and
investigate the larger route and movement scopes next.

## Full campaign replay

Both current P10 ROMs also completed the [retained P7 victory
route](2026-11-13-p7-carried-normal-home.md) from HOME at game frame 55,892
after 57,678 physical input updates. All 59 named checkpoints, the 11
briefing acknowledgments and the 10 in-mission dialogue acknowledgments
matched. The three required heroes survived and both hostile Nexus
objectives cleared. The final verification checked the neutral survivor’s
source owner against the archived tape. Whole-replay timing remains unmeasured.
The paired receipt SHA-256 is
`08d80d7a6cdb49b2065c8d6495cf10f4553205ba744bea2b079ad1d5c72958d0`.

| ROM | SHA-256 |
| --- | --- |
| Standard P10 | `153d80f72c5184b745faf54f4c2edd1aa27805e427ea36aef1950a406c1ff14a` |
| P10 terrain-cache preview | `04e703ad953a4fdb8cf3f5cdfd9ebaf6485d7611af580280ba794f650ece7f69` |
