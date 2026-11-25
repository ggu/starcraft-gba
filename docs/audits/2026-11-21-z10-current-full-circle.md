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

### Earlier defense and Reaver pursuit

From frame 2,359, the Drone places the Den and morphs both
existing Colonies. These structures finish by frame 3,425,
and paid Hydralisk production begins at frame 3,483.
At frame 4,983 the player has five Hydralisks, all six starting
Zerglings and both Sunkens. Fresh HOME restores this earlier
position and re-exports identical SRAM.

A flank continuation moves the combat group west and targets
the nearby Reaver at frame 6,616. The Reaver disappears by
frame 7,118. The attack continues, and by frame 8,818 the Den,
western Sunken and all miners have been lost. Two Hydralisks
remain. Prioritize extra static defenses from the earlier
economy save, then keep trained units near the mining line.

| Local receipt | SHA-256 |
| --- | --- |
| Frame-4,983 SRAM and fresh readback | `2ee81e0cde1fe4b4a506fa08130fc1150fc1ce2bf1a6c7d880bfeeff66726d99` |
| Earlier defense fresh HOME verification | `4b12d0a292df953d25a515d255b6a7c50ca15d4a9b1defa966cfffd4da5783a5` |
| Flank fresh HOME verification | `774a4f4f8ff96a082e0f39e51e29dcdb87127d487611d4f2df5ebbda7db80776` |

### Four Sunkens before the raid

An alternate opening completes two additional western
Sunkens, the original two Sunkens and the Den by frame
4,640. Hydralisks are queued and two miners remain. The
fresh HOME replay re-exports identical SRAM. In a passive
continuation to frame 7,849 the extra Sunkens and Den fall
while both original Sunkens and both miners hold. The
Reaver is at 39 HP in the frame-7,649 observation. Keep
the earlier checkpoint. In a focused continuation, the Reaver
disappears after the attack order. At frame 8,850 two miners,
the eastern Sunken, Pool and Hatchery survive with four
Hydralisks and three Zerglings. The Den is destroyed and
a hostile actor remains nearby. This supports a defensive
continuation with workers available to rebuild.

| Local receipt | SHA-256 |
| --- | --- |
| Four-Sunken SRAM and fresh readback | `8f3f8fcec358c7ab0e4bd2a364da7d6790fb5afb041f9eced6926f513a62a3a8` |
| Four-Sunken HOME verification | `dcda4c3e1182f3b3407de0d200765b1757dc91769b9dde1c96b536c8e8f93ce5` |
| Continuation through the first raid | `70cd9ffc1b5890806f011f952b0e64328001fa65a19b14288d671b3219afca44` |
| Focused Reaver continuation | `efb10364be2a9ca47938a3c9f66488449ed2e595065f957e94a06bd8ad91e4a1` |

### Rebuild after the first raid

Fresh HOME restores the focused branch at frame 8,850.
Starting construction immediately loses the Hatchery and
miners. In the successful defense branch the nearby
enemy is gone by frame 9,220 after an immediate attack
order. The Hatchery survives with 6 HP, along with the
Pool, eastern Sunken and both miners.

The replacement Den finishes at full health and paid
Drones restore four mining workers. Fresh HOME at frame
14,345 restores the Den, Hatchery, Pool, Sunken, and the
1,806/800 bank. No nearby attacker is present in that
observation. Another ordinary build order starts a
second Hatchery at (560, 1776). Fresh HOME at frame
15,812 retains construction at 714 HP, three miners,
six Hydralisks, one Hydralisk queued and the 1,601/725
bank. A later passive replay sees attackers arrive
from both sides around frame 16,112. Protect the new
Hatchery while the fighters are still together.

| Local receipt | SHA-256 |
| --- | --- |
| Recovered Den and mining SRAM | `99bd54a6b93d92d6a713cdbc3286d5c908050827d17acdafe3e427a2d4b79454` |
| Recovered position fresh HOME | `216951752de3ccc56d9100e2eff5730c455316f54680529e51fc0896054afd61` |
| Second Hatchery construction SRAM | `c4e65f00fbee815d7cba49c44320fcc1298e417aaca6dd0069b2cd345f95c77d` |
| Second Hatchery fresh HOME | `ad1960a0dae859808476ab0b8be5f4dbb6a150776840903cf48928e2a6edc2ac` |

### Relocate workers and restore mineral income

A direct interception during the next raid loses both
Hatcheries and the remaining ground force. Moving three
Drones individually toward (600, 1400) leaves two alive at
frame 19,479, after the old buildings have fallen. One
starts a Hatchery at (560, 1360). Generic construction and
fresh HOME preserve the completed 1,250-HP hall at frame
22,008. Hostile units are still concentrated near the old
base in that observation.

A paid Overlord restores supply access. Three paid Drones
reach the local mineral line and raise the bank from 1,227
to 1,275. Fresh HOME at frame 24,934 restores the full-health
Hatchery, three miners, a fourth idle Drone, the Overlord,
and 725 gas. A passive continuation sees the next Reavers
damage the new hall by frame 25,834 and destroy it by 26,434.
Preserve the earlier recovery branch and begin the defense
while the new hall is still healthy.

| Local receipt | SHA-256 |
| --- | --- |
| Completed relocated Hatchery SRAM | `6b81b016cbc3bafc4a8628c15c5f2b922f3403c98f4b7cdad8d4d8ef0801d5c3` |
| Relocated Hatchery fresh HOME | `1a98e3251b17b6247bd3a5249a43afd39290d441e548498f8965117d47e1a43a` |
| Relocated mining checkpoint SRAM | `8427260cbcb3679fc950cde346ccdb5175778a7d4fab847bea2eb02a80092f10` |
| Relocated mining fresh HOME | `afb9589f1ad22e74f3cbbb9867b25e8963d6cff371fbad0ae34ed413ca220599` |

### Second relocation and worker recovery

Two Sunkens weaken the next raid, which still destroys the
first relocated Hatchery. A surviving Drone starts a
second relocated Hatchery at (240, 1040). Fresh HOME at
frame 31,944 restores the completed 1,250-HP Hatchery,
a 462-HP Spawning Pool, one miner and a bank of 539 minerals
and 725 gas. The independent readback re-exports the same SRAM.

From this checkpoint a paid Overlord frees enough supply
to train three more Drones. Frame 34,571 fresh HOME retains
four Drones, the Overlord, the full-health Hatchery, the
Pool at 462 HP and a bank of 801 minerals and 725 gas.
A passive continuation sees hostile units arrive around
frame 35,300. A subsequent colony defense misses the morph
and the attackers kill the colony and workers by frame
36,004. Preserve the earlier economy and issue the defense
from a position that can complete its morph before arrival.

| Local receipt | SHA-256 |
| --- | --- |
| Second relocated Hatchery SRAM and fresh HOME | `29f835ea78a9c8e4c633f00543fe1a56f8dfb4a349a01d8716bccb01077b5bba` / `bdcb0f1ff2d09aaef2af76e577c3d8aadc03885b1204459da4bc06edc078fe7d` |
| Frame 34,571 economy input | `00f468c48da9b3a1140034b3b6d8ecd5f6e59b99228a711636c593a4933a7d42` |
| Frame 34,571 SRAM and fresh HOME | `cf6e16e7dafbc46e7a1f26f031d2c8f0edaf9202af7d726274cbbf9554c2d9f9` / `00d6d4d1391c77116d2abd5a41fc17d69aa6a1b1ea49a0935b8ca92aa49b77ff` |

### Hydralisks before the raid

In a separate branch from frame 31,944, ordinary controls
place a Den at (320, 1120). It completes by frame 32,909.
The Hatchery produces an Overlord and queues three
Hydralisks for 225 minerals and 75 gas. Fresh HOME at
34,723 restores the full-health Den, three 80-HP Hydralisks,
the full-health Hatchery and the Pool at 462 HP. The two
SRAM exports match. An attack-move toward (400, 1650)
loses the group by frame 35,690. The checkpoint verifies
production before the attack and provides an earlier
mobile-defense starting point.

| Local receipt | SHA-256 |
| --- | --- |
| Den and Hydralisk production trace | `e1c478e09985587793a3a6dfd8e7fc512fd95ec09e2417342956a8ff040223de` |
| Frame 34,723 SRAM and fresh HOME | `16ab2580cd5d70016b4887b0aaac0d716882ec802b3eeea7a8ef16dc03c0f0c9` / `2483c0ec0597d5b2864dedf1174b3820ea856ebb53b17191b02853d3b652a8d9` |
