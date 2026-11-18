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

| ROM | SHA-256 |
| --- | --- |
| Standard P10 | `153d80f72c5184b745faf54f4c2edd1aa27805e427ea36aef1950a406c1ff14a` |
| P10 terrain-cache preview | `04e703ad953a4fdb8cf3f5cdfd9ebaf6485d7611af580280ba794f650ece7f69` |
