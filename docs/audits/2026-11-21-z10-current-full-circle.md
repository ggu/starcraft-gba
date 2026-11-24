# Full Circle on the current Z10 cartridge

The normal Z10 cartridge built from `efa386da5c9fc85462a7d7830a12a5f25e17ac32` completed Full Circle (mission 21) on the retained ordinary-input controller route. The public source release `5516dffad0e39604e7ecdeb20944c67284d41c47` adds documentation to that source.

The HOME run made an ordinary pause save at frame 2,526. A separate fresh HOME instance loaded that save and continued the same retained button tape, substituting the normal HOME load input for the pause exit. It won at frame 50,193 with mission completion bit 21 and objective flag 1. All 20 retained checkpoints matched, the scheduler fault count stayed at zero, and the frame advanced within the expected cadence throughout. Ordinary inputs dismissed nine dialogue pages. The run used 52,186 tape calls and 49,540 physical calls after the save cut, including dialogue inputs.

A further fresh HOME instance loaded the resulting victory journal and reproduced the terminal observation and campaign state. Those observations held through eight neutral updates, and the SRAM re-export reproduced the victory journal byte for byte. The save's second journal slot was generation 2 and its checksum validated.

The emulator measured 70,347,482,196 guest cycles across 49,540 updates, about 11.81 main-loop updates per second including presentation waits and UI. The near-30-fps and physical-device checks remain open.

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Z10 ROM | `ad2709458c154142fb776a096d42a444baffe98455832bc0e647a377633bb80a` |
| Matching ELF | `b2e33f6bcb0f4107dcfe0d17722ddfdd397af82cdde81908f42ab75ffc177c39` |
| Controller tape | `83c30ae24a7bbde3dfbf4adf83a461a3706d06f6ac4a4b9743ddb9bb022dbf73` |
| Physical runner | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Pause journal | `eb51bb2516245903843264fe0282a9875eaec19102fa828ada43be1e5603b7f0` |
| Victory journal and frozen re-export | `d39a88a088d7acdd2d388468ab9cedf2371b3d1ed21318e4f5519f1c5f175268` |
| Continuation receipt | `2d9c4e3874f68c65123a16b5f5121d229621e02326c4b005f57dd7a1db821c13` |
| Fresh result readback receipt | `c7398e5f51e92f4529975c962e3d53495721dce47e205553824206c1034b9baf` |

The prepared cartridge, controller, save, runner and detailed receipts are local. The first continuation process stopped after call 30,000. The successful continuation restarted from the validated HOME save.

## All three current finales

Ordinary controller tapes also completed the finales on the exact T10 and
P10 cartridges listed below. Each replay saved through pause, resumed in a
fresh HOME process, and loaded the resulting victory journal in a third
process. The final SRAM re-export matched the original victory bytes in
both cases.

| Cartridge | Pause frame | Victory frame | Controller calls | Receipt SHA-256 |
| --- | ---: | ---: | ---: | --- |
| Terran T10 | 2,276 | 3,361 | 3,446 | `8976377e5b3ce40ed673745ef77077826ab131091d1b3f06c6f262b07fb6f8dd` |
| Protoss P10 | 36,761 | 46,129 | 47,266 | `b2fe252005e631d376112ee6e6c05740d9b51544321d5e29ed97fe88e1e255d2` |

P10 matched 20 retained checkpoints, including six after fresh load,
and held the victory state through eight neutral updates. The local
runner copy used a 2 GiB free-space threshold with its existing 16 MiB
per-file cap. Cartridge and replay assertions were unchanged.

## Current three-cartridge Custom Game matrix

Fresh mGBA processes used ordinary buttons to configure and launch 18
matches per cartridge. The matrix covers Outpost and Blood Bath, every
human race, and one, two or three named CPU races. The one-CPU cases use
the middle seat. The two-CPU cases leave it closed. The three-CPU cases
rotate the three different races.

All 54 matches played 120 updates, saved through pause, and matched
after a fresh HOME load and another 120 updates. The saved and restored
lobby, seats, resolved bases, and gameplay fingerprints matched their
uninterrupted controls. All 54 loads read the original 32 KiB SRAM
without changing its bytes.

A separate 1,134-case launch run checked all 63 nonempty assignments of
closed, Terran, Zerg and Protoss to the three physical CPU seats, on both
maps and for each human race on each cartridge. All requested seats and
bases resolved as expected. The exhaustive runs stop at gameplay entry.
Their receipt SHA-256 is
`9a578686472720ad9f45a8c389680a66133e061187898c7d62e81b0ae186ae89`.

| Cartridge | Game code | ROM SHA-256 |
| --- | --- | --- |
| Terran T10 | SCT8 | `10b1837cb63c3922aa725ab4970998b5be595d131c339fa31724a3a42383f368` |
| Zerg Z10 | SCZA | `ad2709458c154142fb776a096d42a444baffe98455832bc0e647a377633bb80a` |
| Protoss P10 | SCPA | `37c78b32d10c114e5478fff6db2afc06889d0853183a4e75034a2388b6fe6b80` |

The [retained campaign guides](../RACE-CAMPAIGNS.md) describe the
separate ten-entry campaign menus. The local playtest folder
`StarCraft-GBA-2026-11-21-5516dffa` holds these three exact files.
Each header, game code, SRAM signature, length and checksum verified
after the copy to the Mac. Its checksum-file SHA-256 is
`c0a1743e0cc6037b53e53e34ca97697474909e5c62dd1584fd948fe739db6a9b`.
The 54-case receipt SHA-256 is
`faf8b3f524bc9c512d8bf2932d865284f9f5ed7c3e31e2292fd0365416f91103`.
SD readback and physical performance remain open.

## November 24 cartridge opening and first raid

The [Z9 victory continuation](2026-11-13-z9-normal-physical.md#road-assault-crystal-return-and-victory)
enters Full Circle from RESULT and fresh-loads it at frame 11.
The next ordinary pause save at frame 597 holds a Spawning Pool
under construction. Fresh HOME restores the save byte for byte.

At frame 5,667 the Pool, Hydralisk Den and two Sunken Colonies
are complete. Three paid Drones are mining, a second Hatchery
is under construction, and the bank holds 1,142 minerals and
950 gas. Fresh HOME restores the world and re-exports identical
SRAM. This is the retained pre-raid position.

A continuation queues three paid Hydralisks at frame 5,732.
The first raid is still active at frame 7,532. All six starting
Zerglings have been lost. Three Hydralisks and two Overlords
remain. The main Hatchery has 1,152 HP, the eastern Sunken
has 28 HP and the western Sunken is at full health. All
three miners survive. The old winning route used a much
larger defense. Build additional Colonies and start Hydralisk
production earlier, then intercept the Reaver before it
reaches the mining line.

| Local receipt | SHA-256 |
| --- | --- |
| Frame-597 SRAM and fresh readback | `1c8352db71b2caca0dcc92b0a0974aabb40ce95a7e9d223f28678d63a2da3752` |
| Paid economy and defense inputs | `3c3f8b7c5f9023a4be8550d3db42c2ecc25155a3d61d9ade05a559a3f89905fb` |
| Frame-5,667 SRAM and fresh readback | `751e3044e6807b417a8e7a27bd098df16727e1f1c4ed592a0c8659be259e3be8` |
| Raid continuation and fresh HOME | `ada84b29a54a7f16a41af8ace928d2c07baad2b78455e06f98b25ef7864126ef` |
| Frame-7,532 SRAM and fresh readback | `5a47068922ab1fc120ee2b53d569ea86ddafaa5e48dfcaa72dba21a61f88f8a2` |
