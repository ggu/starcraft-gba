# P4 contact rescue on the normal P10 cartridge

The measurement starts from HOME on release 062fdcac. L restores the P3
victory journal and A enters P4. The physical P4 trace reaches victory at
frame 84,220, authored call 85,200. The linked entry covers all 13
in-mission dialogue pages. Launch briefing pages are outside this route.

| Guest measurement | Calls 17,276–17,403 | Calls 40,128–40,255 |
| --- | ---: | ---: |
| Stock mean full-loop cycles | 769,858 | 1,009,833 |
| Stock mean order cycles | 261,683 | 323,748 |
| Stock full ground route builds | 0 | 0 |
| Stock ground route hits | 58 | 73 |
| Stock mean inclusive point route lookup | 765 | 870 |
| Stock mean inclusive nearest target lookup | 48,446 | 74,298 |
| Stock mean P4 rescue scan | 36,449 | 47,818 |

A rescue update collects eligible SIDE and RESCUABLE unit slots in slot
order. A 64-pixel band mask rules out distant witnesses before the exact
32-pixel distance check. Raw actors 101 and 150 remain ineligible to
witness. Each new eligible SIDE unit enters the ordered witness list
immediately, so it can rescue a later unit or any building in the same
update. The original transfer hooks and source-pause behavior remain in
effect.

| Linked P4 measurement | Release | Grouped rescue |
| --- | ---: | ---: |
| Early 128-call mean | 769,858 | 747,503 |
| Early 128-call p95 | 852,875 | 830,749 |
| Early 128-call maximum | 907,221 | 881,740 |
| Late 128-call mean | 1,009,833 | 984,503 |
| Late 128-call p95 | 1,047,331 | 1,022,651 |
| Late 128-call maximum | 1,150,427 | 1,125,457 |
| Full continuation guest cycles | 93,096,513,046 | 90,619,447,960 |

The 2.66% reduction on the full continuation is an mGBA measurement.
All 352 common samples match for gameplay, actor arrays, both indexed
pages, the palette and camera. The selected-page register differed at
call 7,285 while both indexed buffers matched. The 241 per-input ownership
observations around the rescue match exactly. Slots 28 and 137 transfer
on call 4,330, slot 26 on 4,331 and slot 24 on 4,332. The candidate matches
all three authored input phases on each of the 13 in-mission dialogue
pages. Both fresh HOME loads export the same victory SRAM, SHA-256
`414162bb7ed8218ddf4abe6e4f1bb520ee4c21c0240d053614cf31e319a2d969`.

The host regression checks that a rescue can witness another later unit
and a building in the same update, that an earlier unit has to wait until
the next update, and that the excluded actor types remain ineligible. An
ASan/UBSan differential trial also matched the stock rescue routine over
1,200 generated worlds including boundary positions and source pause.

The independent base-range lookup trial saved 0.48% over the full route.
The lookup remains experimental. Larger steady order costs in these
windows are target acquisition and rebuilding the target index.

## Normal products

The normal P10 build reproduces the tested candidate ROM byte for byte.
The hard IWRAM check retains the 6,000-byte bound, the required 512-byte
reserve and 408 spare bytes. The shared P4 controller also compiles in
Showcase. A paired physical run enters through its ordinary campaign menu
and advances to call 5,460, frame 5,413, through the first rescue and
dialogue. Its 16 samples match gameplay, actors, both bitmap pages,
selected page and palette. The fresh active-game SRAM is identical.
The 5.08-billion-cycle opening drops to 4.41 billion guest cycles.
Terran and Z10 compile without this P4 controller.

| Changed product | SHA-256 |
| --- | --- |
| Protoss P10 | `a86f81611a7807565c32a1ea81ef662a66beb74f95ded71af2e316bdd9fe7cc5` |
| Showcase | `3b6039154a97f56e13c0a38ce32e354c7e4b97e2c85a7a9894169f7deafb4f01` |
