# Protoss skirmish against three computer players

The Protoss mission 6 burrow change in release `4c63a258` also runs through shared unit queries in custom games. Two long skirmishes on the P10 cartridge showed about 0.15–0.16% lower mean loop cost than `c23009f8` in the sampled windows. Orders accounted for 59% of total guest loop time on Outpost and 62% on Blood Bath. Each of the 512 measured intervals exceeded two GBA VBlanks.

| P10, human Protoss | Units / buildings at end | Old loop cycles | New loop cycles | New updates/s | Orders cycles | Actor drawing cycles |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Outpost | 121 / 57 | 1,004,944 | 1,003,420 | 16.71 | 590,859 | 86,155 |
| Blood Bath | 135 / 49 | 895,574 | 894,100 | 18.76 | 551,660 | 49,216 |

Cycle columns are means per advancing update, using the guest counters. Updates/s uses the full debugger interval at the nominal GBA clock. These windows measure emulated gameplay with this camera and army composition.

## Reproduce the windows

1. Boot each P10 ROM with its matching ELF in the physical mGBA input harness. From HOME, open Custom Game, select human Protoss, and assign Terran, Zerg and Protoss to the three CPU seats in that order. Use the standard game type on both maps.
2. Give neutral input to world frame 10,880. Hold R + Right + Down for 120 updates, R + Down for 51, then neutral for one. Measure the next 256 neutral updates. Both runs finish at world frame 11,308.
3. Sample the unit, building, mineral and both fog buffers, both bitmap pages, palette, display register and camera at the initial boundary and every fourth update. All 65 samples per map matched across releases. Save from the pause menu and load from HOME in a fresh emulator. Each map produced byte-identical SRAM across releases and on re-export, with matching loaded worlds.

## Where the time goes

An independent run using emulator-side stack sampling every eighth update reproduced every timing row, sampled state and save from the new release. Self cycles averaged over 32 sampled updates per map were:

| Function | Outpost | Blood Bath |
| --- | ---: | ---: |
| Nearest target search | 93,069 | 37,237 |
| Unit order loop, excluding callees | 82,668 | 84,270 |
| Uncached contact mask | 39,266 | 36,086 |
| Movement step clearance | 31,655 | 34,650 |
| Arbiter cloak and detector cache source tick | 29,918 | 31,336 |

For target-search changes, preserve candidate ordering at ties, egg fallback, attack eligibility, and changes to occupancy during movement. Use Blood Bath as a second measurement: the same search accounts for less than half as many self cycles there. Movement clearance and contact tests deserve a separate paired measurement. The simulation and presentation still exceed the budget for sustained 30 fps.

## Target search trials

On the same P10 inputs and `4c63a258` control, clipping a query to the fixed target grid bounds saved 0.17% of whole-loop cycles on Outpost and 0.12% on Blood Bath. Adding a 256-pixel sector occupancy table to reject absent owners saved 0.47% and 0.04%, respectively, relative to that same control. Both experiments matched the unit, building, mineral and fog hashes, both bitmap pages, palette, display selection and camera across 65 cuts per map. The freshly reloaded worlds and SRAM matched on both maps. Keep these experiments local and prioritize movement and contact checks alongside target acquisition.

The prepared scalar comparison revealed an independent test issue. At frame 174, three host profiles picked slot 13 in the historical reference. Slot 13 contained an Egg. The release correctly preferred an eligible building. The reference selected the fallback rule only for the older Z7-specific Egg capability. The updated reference applies that same rule to the shared Egg actor while retaining its independent target traversal.
