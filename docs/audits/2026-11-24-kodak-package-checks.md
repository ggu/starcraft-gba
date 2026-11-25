# November 24 KODAK package checks

The three race ROMs and Showcase in
`Games/Development/StarCraft-2026-11-24` are the existing
`1f0ad66e` package. The installed files match the build manifests and
local replay images by SHA-256. The separate original-art preview is
outside these cartridge checks. All tests use a headless mGBA runner
with emulated buttons and observational guest reads. Save tests use
isolated local SRAM.

## Custom Game launch matrix

Each race cartridge launches both maps with each of the three player
races. For every one of those six choices, each of the three CPU rows
is independently set to Terran, Zerg, Protoss or Closed. There are 63
lineups with at least one CPU. The test verifies the gameplay screen,
map, player race, active CPU rows, chosen CPU races and distinct start
slots in the launched match.

| Cartridge | One CPU | Two CPUs | Three CPUs | Total |
| --- | ---: | ---: | ---: | ---: |
| Terran 10 | 54 | 162 | 162 | 378 |
| Zerg 10 | 54 | 162 | 162 | 378 |
| Protoss 10 | 54 | 162 | 162 | 378 |
| Total | 162 | 486 | 486 | 1,134 |

These are explicit faction choices in ordinary Free for All lobbies.

## Fresh N64 lobby replays

Two ordinary button tapes ran from reset on the USA N64 ROM in ares
v148. Challenger Melee showed two slots with Terran and one Zerg
CPU. Blood Bath Melee showed four slots with a Terran player and
Zerg, Terran and Protoss CPUs. The selected framebuffer from each
run matches its previously inspected capture byte for byte. The
[earlier lobby audit](2026-11-22-native-custom-game-lobby.md) also
observed changes to the race on individual computer rows.

The two-slot Challenger observation applies to that N64 map. The
GBA launch matrix exercises up to three CPUs on both GBA maps.

### Last open CPU row

Two more N64 reset replays keep the first two Blood Bath CPU rows
Closed and highlight the third row at Random. Right changes it
directly to Zerg in both Melee and Free for All. Start then opens the
field. The three witness frames per mode show the edit and play.

The exact Terran 10, Zerg 10 and Protoss 10 images repeat that
sequence in both Blood Bath modes. Right moves from Random to Zerg
with the other rows still Closed. Launch starts with the third row
as the only CPU and the world advances. Outpost Free for All on all
three images permits an all-Closed lobby and keeps it on the setup
screen on A at Launch. Reopening the third row as Zerg allows play.
The Outpost result covers the authored GBA map.

A fresh ares replay of the Melee route finishes 6,310
steps. The three setup captures match the retained hashes,
including the two sides of the Right press. The launch
capture differs from the prior run. The current contact
candidates independently repeat the lobby route on all
three GBA race ROMs. Each keeps CPU mask 4 and resolves
the opponent to Zerg, then advances from frame 1 to 9.
The lobby check covers this transition and the launch.

| Local receipt | SHA-256 |
| --- | --- |
| Native replay status and captured frames | `baaebfa6aa69b582ae8de6cbabfc0f66e8f5d93bf1cec9319d8ee98c20607709` / `d52874e6555e515d5acfde4cc0956b675c565a17ca019ddbeb88cac6637c15af` |
| Current three-cartridge ordinary lobby | `40c23b07f415d8101f329d5d1183922ed65171226621cd87b92fb56f105682fb` |

Separate reset replays extend the same sequence in the reverse
direction. The N64 first shows Random, then D-pad Right changes the
last open row to Zerg, and D-pad Left returns it to Random while the
first two rows remain Closed. Start then launches play. This holds
in both Blood Bath Melee and Free for All. The N64 observer uses an
isolated D-pad Left binding and reproduces the prior Right witness
in the same run. The cuts show the configured lobby and launch.

All three GBA race builds repeat Zerg to Random with D-pad Left in
both Blood Bath modes. Each launch has only the third row active;
it resolves to a legal race and advances from frame 1 to frame 9.
The randomly resolved faction is allowed to differ across launches.

| N64 mode | Random | Right to Zerg | Left to Random | Gameplay |
| --- | ---: | ---: | ---: | ---: |
| Melee | 5,700 | 5,750 | 5,800 | 6,360 |
| Free for All | 5,750 | 5,800 | 5,850 | 6,410 |

## Campaign, Showcase and fresh saves

All 30 campaign entries open from their race cartridge. The test
selects the displayed mission, starts from its briefing, acknowledges
opening messages where necessary and observes a live world advancing
at least two updates. The eight Showcase selections open paused and
advance after pressing A.

The Terran chapters below win on the exact installed cartridge
image. Each replay starts from HOME, continues a mid-mission pause
save in a fresh process and reloads its victory in a third process.
The final SRAM readback matches the victory save byte for byte.

| Terran chapter | Saved game frame | Victory frame | Matching gameplay checkpoints | Emulated updates/s |
| --- | ---: | ---: | ---: | ---: |
| 1 | 6,214 | 10,120 | 12 | 29.39 |
| 2 | 18,204 | 29,366 | 31 | 28.56 |
| 3 | 26,993 | 54,000 | 56 | 28.74 |
| 4 | 4,968 | 8,911 | 10 | 24.02 |
| 6 | 19,466 | 23,536 | 42 | 16.34 |
| 7 | 2,289 | 3,931 | 12 | 22.09 |
| 8 | 2,568 | 139,216 | 268 | 21.75 |
| 9 | 35,363 | 47,266 | 51 | 17.12 |
| 10 | 2,276 | 3,361 | 6 | 17.83 |

The timing includes briefing, pauses and presentation, and comes
from the GBA guest clock.

The [Terran chapter 5 victory](#terran-5-victory-and-fresh-home)
adds the last Terran chapter through a staged controller replay. The
[Zerg chapter 9 victory](2026-11-13-z9-normal-physical.md#road-assault-crystal-return-and-victory)
delivers the crystal through a cleared road and restores RESULT from HOME.
Victories with fresh SRAM readback on the installed package now cover
27 of 30 chapters, including all ten Terran, all ten Protoss, Zerg 1
through 6 and Zerg 9.

### Protoss and Zerg victories

P1 and P2 win from HOME on the exact installed Protoss image. P1
wins at frame 15,305 with 16 matching observations. P2 wins at
frame 33,698 with 36 matching observations. Its victory appears
five input calls before the tape ends. The remaining input is a
neutral release, and the victory fields stay stable through the
final checkpoint. Both victories reload in fresh HOME processes
with the correct mission and completion bit. The re-exported SRAM
matches the victory journal byte for byte.

P4 wins at frame 86,150 after 87,100 tape calls. All 88
recorded observations match. The replay advances seven briefing
pages and thirteen gameplay pages. Raynor and Tassadar match the
recorded owner and position at both rescue and extraction. A second
run saves at frame 986 and restores the field and trigger state in
a fresh HOME process. That continuation matches all 88 observations
and reaches the same win. A third process restores the terminal world
and victory state, then re-exports byte-identical SRAM.

P3, P5 and P6 win from HOME on the exact installed Protoss image.
Each resumes the pause save from a fresh HOME and restores the
terminal world from a third process. Each victory journal re-exports
byte for byte.

| Protoss chapter | Gameplay calls | Saved game frame | Victory frame | Matching checkpoints |
| --- | ---: | ---: | ---: | ---: |
| 3 | 48,168 | 20,000 | 47,661 | 49 |
| 5 | 5,172 | 2,700 | 5,057 | 7 |
| 6 | 94,430 | 64,800 | 92,986 | 105 |

P7 also wins from HOME after 57,636 gameplay calls. The replay
checks the frame at each retained input edge, saves at frame 8,845
and reaches the frame-55,892 victory after a fresh HOME load.
A third process loads the victory and re-exports identical SRAM.

P8 wins at frame 104,113 after 106,011 physical calls. All 40
observations match, and the fresh HOME continuation restores the
frame-30,949 pause save. A second continuation from that same pause
journal exports a victory journal. A fresh HOME process restores
the terminal mission and world and re-exports byte-identical SRAM.
Both journal slots validate and the pause bank stays identical.

The manifest also pins both retained
[Protoss finale receipts](2026-11-13-p9-normal-home.md).
P9 wins at frame 272,857 after 281,447 calls and P10 wins at frame
93,377 after 94,852 calls. Both restore the mid-mission save and
the victory in fresh HOME processes.

Z2 wins on the exact Zerg image at game frame 10,209. The retained
older route stopped with the Chrysalis carrier idle. The new
ordinary-input route selects the carrying Drone and moves it to
the Warp Point. It reproduces the win from blank SRAM in 10,500
physical calls. A separate run saves at frame 8,663, reloads via
fresh HOME and reaches the same victory. The uninterrupted win
also reloads through HOME and re-exports identical victory SRAM.

Z1 has a winning receipt with a fresh save continuation on the
earlier November 23 Melee build. Its input tape is missing from
the retained local path. The [installed Z10 victory](#zerg-1-victory-replay)
uses a different ordinary controller continuation from the early
pause journal.

The installed Z10 loads both earlier Z1 journals when pressing
L from fresh HOME. The paused journal restores frame 984 with a
matching world fingerprint, 55 units and 18 buildings, then advances
to frame 992 after release. The victory journal restores the frozen
frame 50,669, the completion bit and matching objective flags, army,
resources and living Chrysalis. Both export SRAM byte for byte.
This qualifies loading the earlier save and result on the exact
current image. The new run below separately reaches victory by
advancing ordinary play from that early pause.

An ordinary-button continuation from that pause queues a Drone
at frame 1,476 by paying 50 minerals. Three Drones are present by
frame 1,856, including the one already queued in the input save.
A Drone travels to the selected geyser and starts the paid
Extractor, completed by frame 2,864. A remaining Drone assigned
to the Extractor returns gas by frame 3,468. A fresh process uses
the recorded inputs to reproduce the terminal world fingerprint,
60 units, 19 buildings, and player resources of 218 minerals and
8 gas. This is a continuation from the earlier pause journal.

The first Den placement is refused with the in-game "No gas"
message. After the bank reaches 56 gas, the same build command
sends a Drone to the snapped site. The build pays 100 minerals
and 50 gas on arrival and consumes the Drone. Construction is
visible by frame 5,271 and complete at frame 6,071.

An independent run replays 5,418 physical updates from the earlier
pause to the completed Den, with the terminal world fingerprint
matching and the Chrysalis alive. It pauses and saves on the
installed Z10 at frame 6,071. A fresh process restores the same
world, exports byte-identical SRAM and advances to frame 6,079.

A longer continuation restores enough gas to train a Hydralisk,
but the first order is refused at the supply limit. The Hatchery
then pays 100 minerals for an Overlord. It hatches by frame 8,034
and restores capacity from 2 to 18. The Den is destroyed before
then, and the next Hydralisk request is refused for a missing
prerequisite. The validated opening needs the Den defended through
supply recovery to advance to Hydralisk production.

### Zerg 1 defended opening and economy

An earlier Overlord purchase at frame 3,560 keeps supply available
for the first Hydralisk. One branch queues that Hydralisk at frame
6,112 for 75 minerals and 25 gas. It hatches by frame 6,712 while
the Den is still standing. The player then loses both the unit and
the Den in an unsupported first attack.

A separate branch selects all twelve Zerglings and issues Attack
Move at frame 5,977, shortly after the Den completes. Ten Zerglings
hold near the approach. A paid Hydralisk hatches by frame 7,160 with
the Hatchery at 1,250 health and the Den at 814. An independent
6,521-update replay reaches the same world and living Chrysalis.
Saving from pause exports the same journal bytes, and a fresh HOME
load restores the world and re-exports those bytes.

The next branch reaches four Hydralisks by frame 11,491, after all
ten defending Zerglings have died. An ordinary attack order clears
the immediate pressure on the Den. The gas Drone dies in that fight.
The Hatchery buys two replacement Drones, both assigned to minerals.
The save at frame 13,270 has three mining Drones, four Hydralisks,
176 minerals and 90 gas. Hatchery and Den health are 1,250 and 142.
A fresh fixed-input replay from the frame-12,180 journal produces
byte-identical SRAM. Loading it from another fresh HOME restores
the same world and living Chrysalis. The following 120 neutral
updates retain all four Hydralisks and three miners. Minerals rise
to 200 while Den health and the gas bank hold steady.

The mission is still active with 42 enemy units and 14 enemy
buildings at the saved checkpoint. The damaged Den and interrupted
gas income remain the next constraints on an assault.

### Zerg 1 Extractor defense and renewed gas delivery

The frame-13,270 checkpoint had already lost its Extractor. An
ordinary rebuild request is accepted at frame 13,370. A Drone dies
and there is still no Extractor at frame 14,470. In another branch,
moving four Hydralisks toward that approach loses all four within
600 updates. A separate long continuation reaches frame 38,494 with
32 enemy units and 14 enemy buildings. Its Hatchery is down to 370
health, and the mission remains active.

The stronger branch starts earlier, from the frame-7,160 save.
An immediate Attack Move intercepts an air attacker. At frame 8,749,
the Extractor still has full health, the Den remains at 814 health,
and gas has increased from 37 to 85. At frame 12,222, a second
attack order diverts attackers from the Extractor. The fight costs
two Hydralisks, three Zerglings and the gas worker. New production
restores four Hydralisks by frame 14,801, with the Extractor at 123
health and 40 enemy units remaining.

An ordinary A command assigns a remaining Drone to the Extractor.
It delivers gas by frame 15,348, increasing the bank from 15 to 23.
A previously queued fifth Hydralisk has also hatched. A fixed-input
replay from the frame-7,160 save matches all three terminal worlds
and their respective SRAM bytes. A fresh process restores the last
save and the living Chrysalis, and after 120 more updates all five
Hydralisks and both workers remain alive. The Hatchery, Den and
Extractor remain at 1,250, 814 and 123 health. Gas stays at 23 in
that final neutral window.

Starting from that save, 1,200 neutral updates increase gas from
23 to 55. The gas worker, five Hydralisks, Den and Extractor survive.
One more paid Hydralisk hatches by frame 17,345, bringing the force
to six. One forward Hydralisk is down to 36 health. The Extractor
has 123 health and the chapter remains active with 39 enemy units
and 14 enemy buildings. An independent controller replay matches
the terminal world and SRAM bytes. A fresh process restores the
save and advances to frame 17,465 with all six Hydralisks, the gas
worker and the Extractor still alive.

Conservative defense and paid production reach ten Hydralisks,
four Drones and two Overlords by frame 23,161. Two follow-on
attacks destroy three enemy buildings and three enemy units from
that checkpoint. At frame 26,301 all ten Hydralisks are alive,
gas is 210, and the Hatchery, Den and Extractor still have 1,250,
814 and 123 health. The remaining 33 enemy units and 11 buildings
are active. In a separate push toward the deeper base, six
Hydralisks die. The preserved save precedes that attack.

An independent ordinary-input replay reproduces all three pause
checkpoints, their world fingerprints and their SRAM bytes from
the frame-17,345 journal. A fresh emulator loads the last save
with identical SRAM and advances another 120 updates. The living
Chrysalis and all ten Hydralisks remain present. Victory is open.

### Zerg 1 base raids and reinforcement

The continuation from frame 26,301 responds to a Wraith near the
Den and a Firebat near the Extractor. It loses the gas worker,
intercepts the Firebat and assigns a replacement Drone. Gas rises
from 233 to 249 and the Den stabilizes at 744 health. By frame
32,759, paid production reaches 14 Hydralisks. In a neutral branch
from there, the next raid destroys the Extractor and kills nine
Hydralisks. An immediate Attack Move intercepts the raid in the
preserved branch. Twelve Hydralisks and all four Drones survive,
with the Extractor at 93 health and 28 enemy units remaining.

Production reaches 16 Hydralisks with one queued by frame 37,754.
The base retains the Extractor and four Drones, with 2,028 minerals,
248 gas and 40/50 supply. Eleven enemy buildings remain. An
independent controller replay matches the five checkpoint worlds
and their SRAM bytes. A fresh load advances 120 updates with all
16 Hydralisks and the Chrysalis alive. In separate longer branches,
idling loses the Extractor and a preemptive regroup loses nine
Hydralisks. The later successful sequence responds to this raid.

### Zerg 1 victory requirements

Targeting the incoming Marine's actual path preserves the army.
In the subsequent assault, Hydralisks reduce 11 enemy buildings to
five by frame 42,816 and to zero by frame 45,652. Thirteen Hydralisks
and the Chrysalis survive. The Pool, Den and all Drones are gone.
An independent replay matches the pause journal. A fresh load
advances another 1,200 updates with zero enemy buildings and the
mission still active.

A separate branch returns to the base and rebuilds the Pool. The
Den and two Drones survive, while all Hydralisks are lost. An
Overlord hatches after being queued at 4/2 supply, allowing paid
Hydralisk production. The following raid destroys the new Pool.
The southwest evacuation reaches frame 43,412 with a living Den,
ten Hydralisks and three full-health Drones. Five hostile buildings
remain and the Pool needs rebuilding. The later push from this
checkpoint clears every hostile building by frame 46,554. The Den
survives and a raid kills the Drones before the Pool is rebuilt,
leaving the mission active. Both southwest journals reload with
identical SRAM in a fresh process and advance ordinary play.

In-memory inspection of the owned USA N64 ROM's Zerg 1 victory
trigger finds three simultaneous command-count conditions. Player 6
has zero buildings, and the current player has at least one
Spawning Pool and at least one Hydralisk Den. The victory row uses
current counts. Initial building records in the standard building
ID range belong to Player 4 and Player 6, with two and fourteen,
respectively. This verifies the numerical conditions in the ROM.
Native engine execution, ownership changes and trigger polling
timing remain open.

The retained N64 Z1 evidence covers the title and
briefing after a progress edit. A native combat comparison still
needs an ordinary mission unlock and capture.

### Zerg 1 victory replay

From the southwest pause, ordinary orders rebuild the Pool by
frame 45,340. The Den and Hatchery are standing and four enemy
buildings remain. The final attack intercepts the Goliath at the
base and clears the last four structures. At frame 49,270 the
result shows victory with ten Hydralisks, the Chrysalis at 800
health, a 387-health Pool, and a 744-health Den. The Z1 completion
bit is set and the unsupported-event counter is zero.

One independent composed replay starts at the earlier-release
frame-984 pause and executes 49,383 controller updates across 17
segments. Each segment opens in a new process, loads the previous
segment's SRAM through HOME, and matches the expected world and
output SRAM. A new process loads the final victory and re-exports
identical SRAM. Returning to HOME opens the Z2 briefing through
ordinary menu input. The missing opening tape to frame 984 and
native combat comparison remain open.

### Zerg 3 early economy

A new run from blank SRAM opens Z3 through HOME and deploys from
its briefing. A paid Drone hatches, starts collecting minerals,
then builds an Extractor. Two starting workers take mineral and
gas orders. By frame 4,520 the resource banks hold 138 minerals
and 90 gas. The 250-health Chrysalis survives along with the
Hatchery, Pool, Den and completed Extractor. An independent replay
matches all four journals. A fresh HOME process restores the
latest journal byte for byte and advances another 120 updates
with both workers still assigned.

Continuing from that save with ordinary input raises ten
Hydralisks and four Mutalisks by frame 11,342. Five Drones
continue mining or gathering gas. By frame 12,156 the first
hostile turret is down, leaving 45 hostile units and 25 hostile
buildings. Nine Hydralisks and three Mutalisks survive.
The Hatchery, Pool, Den, Extractor and 250-health Chrysalis
remain intact. An independent replay matches both new saves
byte for byte. A fresh HOME load restores the latter save
and the same field state. A 120-update neutral continuation
loses one more Hydralisk, so the next attempt needs immediate
movement or reinforcement. Victory remains open.

A separate branch from the verified frame-11,342 save trains
20 Hydralisks and retains three Mutalisks by frame 23,023,
with five Drones, all key buildings and the 250-health Chrysalis
intact. A short attack on the nearest turret drops the hostile
building count from 26 to 25 by frame 23,837. Nineteen Hydralisks
and two Mutalisks survive. Both ordinary-input segments replay
to matching field fingerprints and byte-identical saves, and a
fresh HOME load restores the assault checkpoint. The earlier
staging point keeps more forces for a longer offensive.

From the earlier staging point, a paid Lair and Spire complete
by frame 27,291. Reassigning one Drone to the Extractor restores
gas income, and a newly paid Mutalisk hatches by frame 29,174.
A second gas Drone and a replacement mineral Drone restore
the workforce to five. With two gas orders active and no gas
spending, the bank grows by 80 gas over 1,500 updates. A second
paid Mutalisk hatches by frame 31,682. A fresh HOME load at that
checkpoint restores 20 Hydralisks, five Mutalisks, five Drones
and the intact Chrysalis and tech buildings. Independent replays
match the three saved field states and journals. The air-tech
branch has 26 hostile buildings remaining.

The frame-31,682 save also continues through a third gas worker
and seven more Mutalisks. At frame 44,345 the force has 19
Hydralisks and 12 Mutalisks. An ordinary Move order concentrates
both groups before the attacks. The two turret strikes remove
two hostile buildings while all 31 combat units survive.
The last save at frame 48,118 retains the full-health Chrysalis,
key tech buildings, and six Drones split evenly between minerals
and gas. There are 33 hostile units and 24 hostile buildings.

An independent replay matches the six stage fingerprints and
SRAM outputs byte for byte. A fresh HOME process restores frame
48,118 and the same army. One Hydralisk dies after another 120
neutral updates, so the next attack needs an immediate ground
order before sending the Mutalisks ahead. A separate attempt
shows the line ahead has siege tanks and costs nine Hydralisks
on a direct advance.

Starting before the ground push gives time to hatch 18
Mutalisks. Two A taps on their stack select the air units,
then a Move order gathers them clear of the Hydralisks.
With the ground force near the base, the air group destroys
both front turrets, an exposed tank and an additional hostile
building. The frame-55,138 save retains 18 Hydralisks, 18
Mutalisks, six Drones and the intact Chrysalis and tech.
It has 34 hostile units and 23 hostile buildings remaining.
The six stages replay to their retained field and SRAM hashes.
A fresh HOME load restores that save byte for byte and keeps
all 36 combat units alive for a further 120 neutral updates.

Seven independent tape segments continue the verified frame-55,138
save from 23 to nine hostile buildings by frame 61,132, with a
fresh reload of the same field. The air force has 14 Mutalisks,
the ground force has 16 Hydralisks, and the Chrysalis still has
250 HP. An approaching Marine threatens the Chrysalis shortly
after this checkpoint. A nearby Hydralisk intercepts it through
ordinary input. The Chrysalis survives the attack at 170 HP.
From that defended state, the air force clears four additional
buildings with the combat force intact.

Saving immediately after an intervening Command Center attack
returns the save-failed notice at frame 64,486. A read-only
breakpoint trace reaches the save adapter's pending-impact
limit. The check receives 12 impacts and the serializer
admits eight. The previous SRAM journal remains loadable.
A later ordinary save succeeds after another building falls.
The recovered frame-64,819 journal fresh-loads with three
hostile buildings, 16 Hydralisks, 14 Mutalisks, six Drones and
a 170-HP Chrysalis. A separate ground selection clears the
approach while a Hydralisk stays near the Chrysalis. The
frame-66,829 save retains the full army with 14 hostile units
and three hostile buildings. Three independent input segments
return the same save bytes and field fingerprints.

The final two input segments move the ground force into the
remaining Marines and send the Mutalisks to support it. Victory
appears at frame 68,639 with two hostile units and two hostile
buildings remaining. The Chrysalis survives at 170 HP. The
result sets the primary objective and Z3 completion bit and
records zero unsupported actions. Six Hydralisks, 14 Mutalisks
and six Drones remain alive. An independent replay reproduces
the victory and the 32 KiB autosave byte for byte. Fresh HOME
load restores the same result. Ordinary menu input then opens
the Z4 briefing and deploys the next mission.

### Zerg 4 victory and Zerg 5 deployment

Ordinary input from the Z3 victory opens and deploys Z4 on the exact
installed Z10 image. Drones build the Pool, gather minerals and gas,
and complete the Lair and Spire. The Chrysalis hatches at frame 18,111.
Kerrigan has 400 HP and the mission reports no unsupported actions.

The frame-43,789 pause save restores 20 paid Mutalisks, five Drones,
and the Lair, Pool, Spire and Extractor in a fresh HOME process. A
direct advance loses most of the air force. The east-edge flank takes
18 Mutalisks to the Command Center. Ordinary attack orders destroy it,
and the objective sequence reaches victory at frame 47,951 with
Kerrigan still at 400 HP.

An independent replay repeats all ten ordinary-input segments from
the Z3 victory, matching each world fingerprint and SRAM export.
Fresh HOME Load restores the frozen victory with completion mask
`0xc000` and re-exports the same 32 KiB. Returning to HOME and pressing
A, Right, A and Start with releases opens the Z5 briefing and deploys
the first field frame.

### Zerg 5 victory and Zerg 6 deployment

The independent Z5 replay starts from that Z4 victory and uses
24,349 controller updates across 14 fresh-HOME segments. Every
segment matches the retained world fingerprint and 32 KiB SRAM.
Recalling the Zerglings clears Kerrigan's route at the early gates.
With cloak active she reaches gate 25 by frame 5,599. She withdraws
to recover to 400 HP and at least 225 energy before the final approach.

The terminal Bring condition fires at frame 17,286. The later source
pause advances on an ordinary A press, and the mission wins at
frame 18,974 with Kerrigan at 241 HP and zero unsupported actions.
Fresh HOME Load restores the result, completion mask `0x1c000`, and
the byte-identical SRAM. From the result, B, A, Down, A and Start
with releases opens Z6 and deploys the first field frame. On a
separate continuation, the starting Hatchery pays for and finishes
a Lair by frame 1,981 and an Extractor by frame 3,005. Assigning a
Drone brings the first gas delivery by frame 3,519. A fresh load
restores the bank at 158 gas, both buildings, and Kerrigan at full
health. All four opening stages independently match their world and
SRAM. A further continuation pays 200 minerals and 200 gas for
Ventral Sacs. The research is complete by frame 10,163. Ordinary
selection and targeting then board the full-health Kerrigan into an
Overlord by frame 11,111. Fresh HOME restores both the completed
research and boarded state with a byte-identical SRAM export. Separate
controller replays reproduce each field and journal.

### Zerg 6 ferry and mainland assault

The loaded Overlord reaches the island and unloads Kerrigan through
the ordinary command menu by frame 14,950. She has 400 HP and the
carrier has 200 HP. A fresh HOME load restores both units and the
completed transport research with identical SRAM.

With Kerrigan on the island, ordinary production brings the
mainland force to 20 Hydralisks. The first concentrated strike
destroys five hostile buildings. Fourteen Hydralisks survive at
the frame-34,450 checkpoint. A follow-up destroys two more
structures and leaves three Hydralisks. The fresh frame-35,103
state has eight Force 2 buildings in region 4, and 67 Force 2 units
and 19 buildings in region 5. Five independent replay stages from
the boarded save match every world fingerprint and SRAM export.

Another continuation from frame 34,450 sends 12 paid reinforcements
to the existing force. The fresh frame-55,229 checkpoint retains 25
Hydralisks near region 4 and Kerrigan at full health. Eight Force 2
structures remain in region 4 at that checkpoint. The next
concentrated attack brings region 4 down to one Photon Cannon and
no Force 2 units. Fresh HOME restores that frame-57,403 checkpoint
with two surviving Hydralisks, Kerrigan at 400 HP and identical SRAM.
Region 5 retains 67 Force 2 units and 19 structures. All three
reinforcement stages independently match their world and journal.

Four more paid Hydralisks join the surviving air units and destroy
the final Photon Cannon. The fresh frame-63,540 save has zero Force 2
units or buildings in region 4, and Kerrigan still has 400 HP. The
region 5 census remains at 67 Force 2 units and 19 buildings. Its
clearance and the victory journal remain open.
An independent replay matches both final controller segments and their
SRAM journals.

#### Region 5 Storm attacks and evacuation

After region 4 clears, paid production stages 21 Hydralisks. The
surviving advance reduces region 5 from 19 structures to 14.
The Overlord ferries Kerrigan back to the cleared mainland, where
she cloaks and casts Psionic Storm. Three pairs of paid casts across
three sorties reduce region 5 from 67 living units to 30 by frame
109,548, including reinforcements during the last recharge. Kerrigan
retains 400 HP through the attacks.

The Overlord then evacuates Kerrigan to the island at full health.
At frame 113,206 the fresh save has 32 hostile units and 14 buildings
in region 5. Protected production rebuilds the force to 30 paid
Hydralisks by frame 121,460. That save reloads through HOME with
Kerrigan at 400 HP, region 4 still clear, and 37 hostile units and
14 buildings in region 5. A separate controller replay from frame
63,540 matches the world fingerprint and SRAM bytes of all 12
checkpoints. The terminal journal also exports byte for byte after
an independent HOME load.

A further ferry and two paid Storms reduce the region 5 army
from 37 to 24. Two reinforcements arrive during the evacuation.
The frame-130,610 journal restores Kerrigan on the island at
400 HP, all 30 Hydralisks, and 26 hostile units and 14 buildings
in region 5. Six more independent controller stages match their
worlds and SRAM. The final HOME load also exports identical SRAM.

Another ferry, Storm pair and evacuation reduce region 5 to
17 hostile units and 14 buildings by frame 147,057, with Kerrigan
at 400 HP on the island and 30 Hydralisks ready. All seven
independent controller stages reproduce their world and SRAM.
Fresh HOME also exports the final journal byte for byte.

A separate push from frame 147,057 takes region 5 to nine units
and 12 buildings but leaves only nine Hydralisks. Returning to
the 30-Hydralisk checkpoint and sending Kerrigan on one more
sortie reduces the defenders to nine units with the force
intact. The subsequent Hydralisk advance destroys all 14
structures and the last two Probes. Kerrigan has 400 HP and does
not join the ground assault. The ten-stage sortie and attack
replay matches each world and SRAM.

#### Zerg 6 victory and Zerg 7 deployment

The cleared regions satisfy the Force 2 conditions, but the
island position at (959,1020) stays inside the smaller location
used by the next trigger. With Kerrigan alive and unloaded,
ordinary movement takes her to (1025,1004), inside the island
and past that smaller rectangle. The authored sequence completes
at frame 177,044. The first terminal screen shows victory with
Kerrigan at 400 HP and the Z6 completion bit set. Two independent
controller segments reproduce the region clearance, the final
world and the victory journal.

Fresh HOME Load restores the victory and exports identical SRAM.
Ordinary buttons reach the Zerg 7 briefing and deploy mission 18
at frame 1 with zero unsupported-mission faults. This brings the
installed package to 25 of 30 verified wins.

### Zerg 7 guarded opening and production

Starting with the independently verified Z6 victory journal, ordinary
HOME inputs deploy Z7 and save at frame 11. In one branch, a paid
Hatchery at (1120,1680) finishes with 161 HP under attack. A separate
branch places the same Hatchery, then selects the opening 12 fighters
with R and orders them to defend near (1080,1688). The Hatchery
finishes at 1,250 HP by frame 2,696.

Training more Drones raises the live worker count to six by frame
4,405. The full-health Hatchery and its eight Hydralisks and four
Mutalisks survive. One Drone collects minerals and the bank reads
372 minerals and 200 gas. Three independent ordinary-input stages
match the world fingerprint and SRAM. Fresh HOME restores the
worker roster and bank with byte-identical SRAM.

The next paid construction and morph sequence completes a Spawning
Pool, Lair and Hydralisk Den. The fresh frame-9,640 checkpoint
retains eight Hydralisks, four Mutalisks and five Drones. The Lair
has 1,800 HP, the Den 850 and the Pool 750. The bank is 242 minerals
and 50 gas. All three technology stages also match their world and
SRAM in an independent replay. A nearby gas supply is the next
milestone for sustained production and the approach to the objective.

From that journal, a paid Creep Colony completes and morphs into
a Sunken Colony at (1008,1632). A separate air order intercepts
the first hostile Mutalisk. At frame 10,885 the Lair has 1,792 HP
and the Sunken 400. Eight Hydralisks, three Mutalisks and four
Drones remain, with 197 minerals and 50 gas. All three ordinary
input stages independently reproduce the checkpoint fingerprints
and exact SRAM. Fresh HOME confirms the final roster and bank.
The longer defense against the Guardian raid still needs an
economy that can keep producing units through the attack.

An alternate branch builds an early Evolution Chamber and places
a Spore Colony behind the frontline at (1176,1632). The eastern
ground rush and northern Guardian attack leave the Spore at 12 HP,
the Lair at 926 HP and one surviving Drone by frame 12,316. This
branch has yet to sustain mining or deliver gas through the raid.

In a continuation from frame 10,885, the three Mutalisks fly
north toward the approaching Guardians. Two watched Guardian
slots disappear from the local scan. A separate read-only
fresh HOME census at frame 12,926 finds those slots reused by
Zerglings and three other live Guardians. The Lair and Den
remain at 1,155 and 78 HP. The ground force destroys the Pool,
and the bank reaches 261 minerals with gas at 50. The saved
defense establishes an air interception under ground pressure.
Pair the north interception with a ground defense before
committing to the Extractor.

| Local receipt | SHA-256 |
| --- | --- |
| Northern interception | `c75a0a80763b2b6336dfb5360096199a4d179d82d62267ad341defda7aa10112` |
| Fresh HOME global census and identical SRAM | `8fad131038f4ad5f2a2f701cc0f6866730ce5ea8495b634c8505e9b80e2f3904` |

A combined air and ground branch from frame 10,885 selects all
eight Hydralisks and issues a distant eastern attack order.
The cursor travel and repeated retargeting leave time for the
ground attack to reach the base. At frame 11,764, five
Hydralisks remain and the Extractor has 58 HP. All Hydralisks
and Drones fall by frame 13,451, and only three Mutalisks
survive after the Pool and Lair fall. The original frame-10,885
save remains the continuation point. Test a closer defensive
position before sending the Mutalisks north. Receipt SHA-256
`bf195f978d97a4f4d61d1e2cc20598903ca602967ec7ecd0fd0e0678af24cb63`.

A separate branch from frame 6,410 builds two full-health
Sunkens on the western side. Fresh HOME at frame 9,891
matches the export and retains the starting eight Hydralisks
and four Mutalisks. A paid Den completes in the follow-up.
The closer attack order at (1008, 1640) preserves that mobile
force through frame 12,430 while the Hatchery, Pool and Den
fall. Both Sunkens remain at full health. A second hold at
(1144, 1624) also loses the base. Correct the defensive
placement against the actual attacker route before relying
on the two-Sunken opening.

| Local receipt | SHA-256 |
| --- | --- |
| Two-Sunken checkpoint SRAM and fresh readback | `7c7cd992ba56528086db8be96ef176702c3e74f5a4d8d9e0cc51d795a1409802` |
| Completed Den and first close hold | `c887d92734fc97a789178eec0eb1092f3bef299f207f40a806418b1737f299f9` |
| Second close hold | `dc905a5ba02f37334a18cc1a3bb342e62f2e901cc5cf551b9a5daa3b9e85309c` |

#### Intercept the northern and eastern approaches

From the two-Sunken save, issuing an eastern attack-move
to (1240, 1672) leaves the Hatchery at 818 HP. Fresh HOME
at frame 11,526 retains both Sunkens, six Hydralisks, four
Mutalisks and three Drones. Continuing the attack-move
leaves the Hatchery exposed to a separate approach. A
closer direct intercept at (1130, 1640) extends the hold.
Fresh HOME at frame 12,492 retains the Hatchery at 730 HP
and ten friendly units. With the same order held, the
Hatchery falls at frame 13,184. The earlier November 13
victory used a different sequence of ordinary inputs.
These routes cannot isolate a change in engine behavior.

| Local receipt | SHA-256 |
| --- | --- |
| Eastern attack-move SRAM and fresh HOME | `b38dad35b64d8443b59efb3865303196d0fd49009212d573ea000129448e7ef8` |
| Closer intercept inputs | `7f612c63e5b5098ec9190a5b970097473eebd3e2a57d927627e4f4ef848b4732` |
| Closer intercept SRAM and fresh HOME | `023e457d3c81b98928ac611cbe2bd1c44ced7a2446f1938b2dcfd3568ca41b5e` / `51de3267559c8f325254de06ba7d679fdd46baa319509807797794e37d01906e` |

#### Sunken coverage on three approaches

The alternate frame-8,851 opening grows a second Sunken
on the east at (1240, 1696), paired with the western
Sunken at (1008, 1632). Fresh HOME at frame 9,886
restores both at 400 HP and the Hatchery at 1,250 HP.
The eastern colony falls by frame 12,086 and the
Hatchery by 13,386. Moving all defenders east leaves
the west exposed and ends the hold by frame 12,394.

A follow-up builds and morphs a third Sunken north at
(1192, 1616). Fresh HOME at frame 10,892 preserves
it at 278 HP, the two earlier Sunkens at 400 HP each
and the Hatchery at 1,250 HP. The three-Sunken hold
without further orders loses the Hatchery by 13,292.

Two further fresh HOME loads reproduce the same frame-10,892
world fingerprint and byte-identical SRAM. A coordinated order
sends all 12 available fighters to (1350, 1500) by frame 10,977.
At frame 13,177 the final friendly building has fallen, all 54
enemy buildings remain, and the GBA mission stays active with
16 friendly mobile units. Z7's campaign defeat predicate counts
both buildings and mobile units. The equivalent native N64 state
has yet to be observed. The readback and attack-input receipts have
SHA-256 `a563b60e44bb0030fe7637a3852cebe888bc6261527519b11771f33119e403d1`
and `c609f292712c3fd1d02c1f154f593935a7adc05e9963bc6dce32fccf531b9e1c`.
Both branches retain 200 gas with no active gas source.
Split the mobile defense and qualify mining through the
raid before committing to further static defenses.

| Local receipt | SHA-256 |
| --- | --- |
| East and west Sunken SRAM and inputs | `bbd8e26eeaae481a8d646495c2281d4457623d4aaef27f0b70283e066074868c` / `1f60e3cfe69ffe95022346372ef3a08a0c11e1445b2b326480dc3d66fc9794e3` |
| Three Sunken SRAM and fresh HOME hold | `bb10d558b61d96eb0fa668dcc482a9fe016a03b2cf64b5175942449ebba7fdda` / `cfbc2a19f2fefb0a84cd9787b20cbec405bcc2fee2afa2bcdf77151127c40632` |

### Pending impacts can reject an ordinary Protoss pause save

The current P10 ROM loads the Outpost three-CPU journal
at frame 11,001. After a normal camera pan and neutral
updates, a B save from the pause screen at frame 11,334
returns failure in the pending-impact guard. The queue has
nine events and the record admits eight. The original
journal stays byte-identical and fresh HOME falls back to
frame 11,001. Saves at frames 11,332, 11,333 and 11,335
have eight events and succeed. Paired P10 ROMs built with
both collision settings are byte-identical and reproduce
the same condition.

The exact current Z10 Carrier-profile build rejects the
[proposed padded codec](https://github.com/ggu/starcraft-gba/pull/818)
at its compatibility guard. The codec also excludes P10's
typed building rows and research journal. Its synthetic
32-impact qualification does not establish either product
path. P10's typed building rows expose 100 padding bytes,
with 12 occupied by the research levels. Those rows alone
cannot contain 192 event bytes. Treat layout, journal
authentication and recovery of the last valid slot together.
Preserve active projectiles, research, Yamato and transport
state when extending the relevant record formats.

| Local receipt | SHA-256 |
| --- | --- |
| Exact P10 pause return, queue count and unchanged SRAM | `7286a0975b84e83922fc3ad61f2ca190bc0f31c5d68c0e48f82363bc6be88b4c` |
| Prior and adjacent journal fresh HOME plus 60 updates | `f0267b6fe1256b2b5dedd9185d4c5658f4acc2f71918d2d34c1c70cbea6cd8bb` |

### Wasteland opening on N64 and KODAK

A new ares v148 reset uses the retained Wasteland opening and
ordinary R press. Both framebuffer captures match the retained
run. The N64 HUD shows 40 minerals and 10/18 supply, and the
second capture shows the five Marines selected.

The exact KODAK image starts at 10/18 supply and grants 40
minerals between game frames 29 and 30. In separate fresh
launches, R selects the five Marines and leaves the five SCVs
unselected. The five Marines are still selected 1,000 updates
later. All units stay unselected in the neutral control. The two
platforms use independent clocks.

A second isolated ares v148 run starts with fresh save storage and
continues the ordinary Wasteland launch with A and analog movement.
It reaches 11,420 emulator steps and retains five frames. The last
frame shows the displaced camera over fog and the opening 40 minerals
and 10/18 supply. This run confirms the controller and capture path
for longer tests. Native campaign progression remains unverified.

A fresh ordinary-input ares run verifies that R selects the five
starting Marines. After a short analog cursor move and C-Right,
the group changes position. A subsequent command moves the group
into the trees. The native HUD identifies the C-button movement
controls. These captures cover selection and movement. An enemy
engagement and mission completion remain open.

The ordinary pause menu saves Wasteland to the first empty slot.
The original interactive bridge closed without writing FlashRAM.
With the desktop ares save sequence added before unload, the same
input writes a 128 KiB cartridge save. A fresh bridge process
copies it into isolated save storage and opens Load Saved with two
D-pad Right presses. `Terr01 01:53` identifies Wasteland. A confirms
the slot and the field restores with the Marines by the trees,
40 minerals and 10/18 supply. The original flash and the new run's
export match byte for byte. The replay uses ordinary controller
input and cartridge FlashRAM.

From the first slot, two ordinary C-Right moves advance the
Marines southeast across the road toward a water crossing. The
pause menu saves the position to the second slot. A fresh process
selects `Terr01 02:41` and restores the crossing with 40 minerals
and 10/18 supply. The 128 KiB FlashRAM round trip matches byte for
byte and the first slot is still listed. A later ordinary run
moves the five Marines along the eastern shore to its southern
edge. Continuing south finds Jim Raynor and displays the advisor's
new survival objective. This supplies a native counterpart to the
[GBA rescue checkpoint](2026-11-13-t1-t2-normal-physical.md#november-20-t1-fresh-sram-and-result-autosave).
The first slot now stores `Terr01 03:47`. A fresh process reloads
the field after the encounter with byte-identical FlashRAM and
still lists the earlier second slot. A native mission win remains open.

From that encounter, ordinary movement takes Raynor south to
the Command Center, Supply Depot and mineral field. Raynor
survives an enemy engagement with 270 HP. The first slot saves
this position and a fresh process restores it with byte-identical
128 KiB FlashRAM. The native Goals screen lists a Barracks,
additional Marines and Raynor's survival. The bank still has
40 minerals and five SCVs remain near the northwest start.
Mining and production are the next verified-play checkpoints.

Finer cursor placement selects a controllable 60-health SCV.
The native unit panel shows its Build, Repair and Move choices.
C-Right moves that SCV south from the northwest opening group
to the rocky pass. The ordinary menu writes a first-slot save,
and a fresh native process restores the SCV there with a
byte-identical FlashRAM export. The field still shows 40 minerals
and 10/18 supply. The next checkpoint is an explicit mining
order at the base followed by a verified increase in minerals.

A fresh continuation reselects the same SCV and issues another
C-Right Move to the south. The original field HUD shows Build,
Repair, Move and the ground-context B Attack label. The bank
still has 40 minerals. The next guarded replay positions the cursor
over a mineral patch while the SCV is selected. The native prompt
switches to **B Harvest**. Pressing A selects the patch. The bank
still reads 40 after that input.

#### Wasteland mineral collection

An isolated ares v148 replay uses the same starting FlashRAM.
The scratch bridge exposes the ordinary B button alongside the
existing mapped controls. The receipts pin the compiled binary
and its actual source separately from the wrapper's tracked
source entries. With the SCV selected and the cursor on the
patch, the HUD shows **B Harvest** and 40 minerals at step 9,890.
A B press and neutral updates raise the bank to 48 by step 10,900,
64 by 11,900 and 72 by 12,900. These three HUD captures precede
a later snapshot failure in that observer. A separate guarded run
repeats the first delivery, writes the first save slot through the
ordinary menu, and exits successfully with a changed FlashRAM hash.
The frame after saving still shows 48 minerals. A fresh ares
process selects the updated first slot, `Terr01 08:31`. It advances
800 neutral steps after confirming the load and shows 64 minerals
at step 5,120. The exported FlashRAM is byte-identical to the file
created by the save run.

A later native checkpoint saves at 120 minerals. Reloading it
and letting the same worker continue brings the bank to 152.
The explored base and mineral line remain visible. Barracks
placement and paid production are still open in the native
replay.

On the exact KODAK Terran ROM, A on an unrevealed patch destination
issues Move. The selected SCV reaches the location, idles, and
keeps the bank at 40 without collecting by proximity. Contextual A
on a visible neighboring patch at (272,472) issues Mine by frame
1,661. At frame 1,861 the SCV is carrying eight minerals. The worker
dies on its return path before delivery, and the bank stays at 40
through frame 2,061.

A separate ordinary-input Wasteland playthrough on the same installed
ROM reaches victory at frame 10,120. Its frame-6,214 pause journal
matches the retained win replay byte for byte. A fresh branch from
that earlier journal observes two 60-HP SCVs carrying eight minerals
each with the bank at 439. After 60 neutral updates both SCVs still
have 60 HP, their cargo is empty, and the bank is 455. A regular
pause save at frame 6,274 loads through HOME with the same frame,
bank and five worker records. The fresh export is byte-identical.
The bank rises to 463 after 60 more neutral updates.

#### Wasteland Barracks construction

The N64 SCV opens a three-by-three Build grid with C-Left. A left
stick input highlights the Barracks at 150 minerals. The selected
SCV can place a green footprint near the mineral line. After A
confirms the site, the bank drops from 160 to 10 and a foundation
appears by emulator step 8,535. The frame at 11,035 shows additional
building sections and an active progress bar. The run exits with
the original FlashRAM intact. A second ordinary run saves the
active construction into the first slot and exits successfully with
a changed FlashRAM export. The final frame still shows the foundation
and its SCV at 10 minerals. A fresh first-slot load restores that
construction and advances with neutral inputs. By step 8,120,
the Barracks is complete and the advisor calls for ten Marines.
The dialogue holds the field through the step-9,120 capture.
The fresh FlashRAM export matches the construction
checkpoint byte for byte.

Continuing the native advisor sequence with A activates the
Marine counter. The next field capture shows five of ten
Marines with 10 minerals. A fresh attempt reselects the same
60-HP SCV near the completed Barracks and moves the cursor onto
the minerals. The field shows B Harvest. Pressing B at emulator
step 9,680 raises the bank from 10 to 26 by step 10,680 and to
34 by step 11,680.

A later native run opens the completed Barracks menu with 66
minerals. The Marine option shows a price of 50. A starts
production, and the next capture shows the active progress bar
and 11/18 supply. Mining continues through the payment, with
24 minerals at the first post-order capture. By emulator step
9,035 in this run, the objective counter reads six of ten
Marines. Full native victory and the next mission unlock
remain open.

A fresh process loads the first slot at six Marines. Mining
continues to 56 minerals. The ordinary Barracks menu again shows
50 for a Marine. A queues the second paid Marine, dropping the bank
from 56 to 6 and raising supply to 12/18. The new Marine finishes
with the counter at seven of ten. The FlashRAM export matches the
six-Marine input image byte for byte for this unsaved continuation.

The seven-Marine position is saved to the first slot and loaded
in a fresh process. The export stays byte-identical. Mining
raises the bank to 54. A third 50-mineral order leaves four
minerals and queues at 13/18 supply. The resulting field capture
shows eight of ten Marines. This continuation ends before a
new save.

The eight-Marine state is then saved and reloads in a fresh
process. Mining funds the ninth Marine, and a new first-slot
save holds nine Marines and 74 minerals. After another fresh
load, the tenth paid Marine produces the native victory
screen. Accepting the result changes FlashRAM and transitions
to Backwater Station, including the briefing. A separate native
process loads the resulting FlashRAM and lists Backwater Station
as available while Desperate Alliance remains locked. Its
FlashRAM export matches byte for byte. Selecting Backwater
Station through the ordinary menu reaches briefing page six
with the two mission objectives. Advancing past the final
briefing opens the playable map and Raynor's opening dialogue
in a fresh-process replay. The FlashRAM export still matches
the post-victory journal.

A separate fresh native replay from the nine-Marine save reproduces
the final victory frame byte for byte. Thirteen of fifteen captured
frames agree across the two runs. This replay ends on the result
screen with the input and output FlashRAM identical. The local
manifest has SHA-256 `9ff9b0386480d3b6f8376375066139798bfa6b04ba830e746ae6fc4349735296`.

A read-only replay of the full GBA Wasteland input tape observes
the matching 150-mineral charge, from 463 to 313, when the
Barracks first appears at frame 6,367. It completes with 1,000 HP
by frame 7,866. Five ordinary orders queue Marines at 50 minerals
each. The first completes at frame 8,319 and the fifth at 10,119.
The replay still wins at frame 10,120. These checkpoints follow
each platform's ordinary controls and use separate clock units.

### Backwater Station opening

The N64 replay opens Backwater Station through the campaign menu
after Wasteland victory. The playable field shows 200 minerals,
200 gas and 5/26 supply. The opening roster visible at the base
is four Marines, Raynor's Vulture and an SCV, with a Command
Center, a Barracks and two Supply Depots. The SCV gathers without
a new order from the player. Minerals rise to 216, then 232.
The ordinary Save Screen writes a first-slot checkpoint. A fresh
process loads that slot and resumes play with 248 minerals, 200
gas and 5/26 supply. Its FlashRAM export is byte-identical.

On the installed Terran ROM, ordinary controller inputs open
the same mission and the read-only observer sees those six units
and four buildings, 200 minerals, 200 gas and 6/26 supply. The
human units report idle orders. In a separate replay, after
600 neutral updates the SCV is still idle and the bank is
still 200/200. Its N64 counterpart is seen beside the mineral
patch and then on the way back to the Command Center. The
controller tape has neutral steps and A taps between these cuts.

The authenticated USA cartridge's unit table has 2 half-units
of required supply for Marine (raw 0) and SCV (raw 7), and
0 for both Raynor units (raw 19 and 20). The human slot starts
with four Marines and an SCV, for 5 supply. The CHK places
Raynor in a separate slot. The opening counter alone leaves
his transfer timing open. The source rule for the GBA's
Raynor alias charges two half-units.

The Backwater CHK contains 15 triggers and zero Order or
Run AI Script actions. The starting SCV is a preplaced unit.
This constrains the worker-order comparison to the opening
engine behavior. The N64 render snapshots establish gathering
and a changing bank, while the exact internal order and the
initial mineral target require a separate trace.
The [OpenBW units.dat loader](https://github.com/OpenBW/openbw/blob/master/data_loading.h#L1433-L1443)
identifies the supply columns in the cartridge table.

A further controller-only load sees minerals rise from 248
to 256 while gas and 5/26 supply hold. An A press aimed at
the vehicle yields no visible selection panel or ring. That
capture leaves its ownership and the hit test unresolved.

#### Backwater Marine production

A separate fresh N64 process loads the first slot and uses
the stick and A to select the Barracks. Its panel shows
1,000 HP. C-Right opens Build and the ordinary stick
selection highlights a Marine for 50 minerals and one
supply. A confirms the order. The next cut has a queued
portrait and progress bar, 206 minerals from the prior
256, and 6/26 supply from 5/26. With neutral input, the
queue clears and another Marine is visible beside the
Barracks. Mining continues. The input and output FlashRAM
match byte for byte.

A separate selection attempt snaps the cursor to Raynor's
Vulture, then A produces no unit panel. That distinguishes
cursor snap from a confirmed player selection in this position.

On the exact KODAK Terran ROM, ordinary opening controls
select the preplaced Barracks and queue one Marine.
Minerals fall from 200 to 150. The queue contains one
active unit while current supply remains at 6/26. The
Marine completes during neutral updates and current supply
rises to 7/26. The observations cover the paid production
path on both systems. N64 displays the extra supply at
queue time. These GBA observations read actual used supply.

| Local receipt | SHA-256 |
| --- | --- |
| N64 opening | `fdbebbcc391be9fc9ce15bde03896f2b540c1504c1d9164b1079d2a84b963f62` |
| N64 first-slot save | `bbb280c07d827e96135ee6ee17b3268bec83d20a20681e18662a4e40b08306f6` |
| N64 fresh-process reload | `2c9d3141c995bf315e72a9e500aa91601b60c507af72631d02d2f6c6b07ebab4` |
| GBA opening and neutral updates | `c23236098c12c013370634bb91fda62357c34a4273d9420be5d62243e3904cd6` |
| GBA independent opening and 600 neutral updates | `4fa9651e09dccc7efc4770cdd8a633823e7c7863722ed1c6f55218aeefbc099f` |
| N64 unit supply table inspection | `18cf32940064e50f2a6872388622061cb6360a04f27ea5e0183d0d2edad89751` |
| N64 Backwater starting roster and action counts | `7ac41e6b4fab6762aca295fed9b07792ae277aa329b72d1d716e755524328f2b` |
| N64 follow-up harvesting and selection probe | `161e81ca5e295a135ee7a21855f6a4e9181db425ff0e7248921b9777b85d37a5` |
| N64 Backwater Marine payment, queue and completion | `23e8f1bd1493d4b378b20afaa32dd775d4bd1d1279e0f6eb13d1f6cbc21740f0` |
| N64 field cursor snap and selection attempt | `7fe3baa1973ccc388c2970e53c80bec68c7d10ede87f3cff671001fde6af3a8a` |
| Exact-KODAK Backwater Marine payment, queue and completion | `29fe8339b66cf3f39d56ec0f0b399ca6213a26b5ad4d44a420bc38095f8b513a` |

A further native controller attempt from the first-slot save
leaves the Marines stationary after A and the attempted move.
No unit panel appears. This run completes with unchanged
FlashRAM and does not establish hero acquisition. Its manifest
has SHA-256 `45c854ba51cc6c54a74eaa4c6a55ff4327a1d13b8167d658b48985f15d21a670`.

### Terran chapter 5 input divergence

The old paid-air route matches all checked observations through
call 17,000. Select opens pause at call 20,401 with no selection.
The following R chord restarts at call 20,403 from game frame
20,073. A diagnostic replay resumes each unintended pause with
ordinary Start input. It reaches call 22,000 at the expected game
frame 21,668, with the expected 18 friendly buildings. It has 27
friendly units versus the retained 35, and 200 minerals versus
512. Continuing the same pause-corrected inputs reaches RESULT
at call 57,747, game frame 56,506, with Raynor dead and Kerrigan
alive. The runtime records defeat and zero trigger faults.
The current cartridge still needs a winning route.

An ordinary move order after input call 22,000 takes Kerrigan to the
northeast edge. She reaches it by game frame 24,775 and stays at
full health through frame 129,658 in the neutral continuation.
The mission is still active with all 21 hostile buildings and
four friendly buildings remaining.

Applying the old suffix after that retreat recalls Kerrigan through
the global combat-group selection and later orders. That variant
loses at frame 108,946 with all 21 hostile buildings still standing.

A fresh retreat test uses an ordinary L-drag box to select two
nearby Marines. A ground order moves both with formation
offsets. Raynor retains his idle target, and Kerrigan retains
her retreat target and idles on arrival. This verifies local
command of an assault group with both heroes excluded.

The explicit Move test selects Raynor and uses B, Up and A to
choose Move from the command menu. Placing the cursor at (1379,203)
sets his movement order and destination. He remains around
(1319,203–205) at 45 HP for 2,000 updates. Kerrigan reaches (1528,7)
at 250 HP. This establishes command acceptance and isolates the
remaining route problem to reaching that waypoint. A previous
contextual A click on nearby ground selected another actor.

The retained route puts Raynor at the stalled location by input
call 20,000. A new branch at call 18,000 verifies selection of his
slot and issues Move toward (1448,398). He moves from (1352,290)
to (1456,383) by 200 updates and stays there through update 500.
He keeps 45 HP and Kerrigan keeps 250 HP. The test establishes an
earlier route that clears the original position. This waypoint is
short of its target, and the mission outcome is still open.

An earlier branch at call 12,000 redirects Raynor from (945,397)
to a staging area near (856,398). He reaches it within 100 updates
of the Move order and keeps 294 HP through game frame 19,160. Kerrigan
follows her prior order and falls to 23 HP. A later attempt selects
her verified slot at frame 13,229 while she still has 250 HP. That
attempt ends before issuing an order because the menu sequence does
not enter Move mode.

A second early branch selects Kerrigan at frame 13,229 and uses an
ordinary A click on empty ground at (702,538). This replaces her
old attack order with Move. At frame 19,288 Raynor remains at
(856,398) with 294 HP and Kerrigan is at (702,527) with 250 HP. They
are the two remaining player units. The mission is still active
and this branch has yet to produce a victory.

One direct approach moves Raynor alone toward the rebel base.
At frame 12,814 his position is (1348,257), inside the base-entry
rectangle, and he has 118 of his earlier 294 HP. He dies by
frame 13,306. The mission's recruitment trigger checks for
Kerrigan's raw identity, so this branch only tests the approach.

An earlier branch at input call 8,000 preserves seven Marines and both
heroes at full health at frame 10,093. A long group order to (200,700)
stalls through 1,000 updates. Ordinary L-box selection and three shorter
ground orders to (110,590), (180,540) and (260,500) advance the escort.
At frame 13,603 six Marines and both heroes are between (229,496)
and (255,516). One Marine stalls at (190,534). All seven Marines
retain 40 HP, Raynor 300 HP and Kerrigan 250 HP. The bank stays
at 300 minerals and 300 gas. Independent HOME reload restores the
frame, every observed friendly actor field and the exact SRAM bytes.
Recruitment and victory are open.

Another branch from frame 13,603 sends the selected eight-unit
escort north to (250,450). The independent frame-14,692 HOME
reload restores their positions around the waypoint, all seven
Marines at 40 HP, Raynor at 300 HP and Kerrigan at 250 HP. One
Marine remains at (190,534). The bank retains 300 minerals and
300 gas, and the SRAM export is byte-identical. Two alternative
longer moves stall or expose units to a hostile detour. The
northern waypoint is the start of a five-order continuation through
(215,400), (185,330), (230,280), (280,232) and (344,200). At frame
16,671 six Marines have reached the final position with Raynor
at 300 HP and Kerrigan at 250 HP. The seventh Marine stays at
(190,534), and all seven retain 40 HP. A separate HOME process
restores the observed friendly roster, positions, health and
300/300 bank with byte-identical SRAM.

The escort doubles back and takes the south corridor. The
previously separated Marine rejoins the group. At frame 21,884
all nine actors reach the area around (480,836) at full health.
The fresh HOME readback matches the frame, roster, positions,
300/300 bank and SRAM. In the first onward branch toward
(552,824), one Marine dies by frame 22,279. Both heroes keep
full health. The earlier save preserves the full escort.

A separate ordinary Attack Move order toward (600,790) defeats the
nearby Vulture and loses the same Marine. At frame 22,304 Kerrigan,
Raynor and the six surviving Marines are at full health. Fresh HOME
restores their positions, the 300/300 bank and byte-identical SRAM.
The Vulture is absent after reload. Recruitment and victory remain open.

Short ordinary orders continue past the Wraith area and along the
southern approach. A fresh HOME process restores frame 27,001
around (1070,440) with both heroes at full health. All six
remaining Marines survive, four wounded. The bank remains 300/300,
and the SRAM export matches exactly. A farther branch reaches
the (1289,434) area at frame 28,190 but loses two Marines and
leaves Raynor at 134 HP. Recruitment is unobserved in that
branch. The full-escort journal remains available.

Focused attack orders clear two bunkers on a separate branch.
The first falls at (1160,400) with all six Marines and both
heroes alive. The independent replay matches the full result
and exact SRAM at frame 27,325. A second engagement clears
the bunker at (1272,384) and costs one wounded Marine.
At frame 27,771, Kerrigan and Raynor retain full health, with
five surviving Marines. Fresh HOME restores the observed
positions and health and the exact SRAM. A third bunker and
hostile units remain near the east corridor into the base.

Raynor takes the east corridor into the base-entry rectangle and
survives there. A fresh replay of the follow-up identifies Kerrigan
by raw identity 16 and uses the explicit Move command to send her
to (1376,240). At frame 30,681 the player roster grows from four to
27 units and from zero to 19 buildings. The neutral unit count drops
from 20 to zero. Kerrigan has 250 HP and Raynor 269 HP. Fresh HOME
restores both heroes and the recruited roster with identical SRAM.
An independent replay from the post-bunker save reproduces each
of the four checkpoints and its SRAM.

The recruited Factory accepts a paid Vulture order for 75
minerals. The bank falls from 300 to 225. The unit finishes by
frame 31,349 at 80 HP. Fresh HOME at frame 31,549 restores the
unit, both heroes and the bank with byte-identical SRAM.

The captured Starport at (1184,72) then queues a Wraith for
150 minerals and 100 gas. The bank moves from 225/300 to 75/200.
The new Wraith finishes with 120 HP by frame 32,826. The
original recruited Wraith is still present. Fresh HOME
restores both Wraiths, both heroes and the exact SRAM.
The mission still needs a winning attack on the hostile base.

Two recruited SCVs collect minerals and later split between
minerals and the captured Refinery. A fresh frame-39,237 save
retains both 60-HP workers, the 471/72 bank and four surviving
Wraiths. Four Wraiths knock out three hostile Wraiths while
advancing south. The two heroes remain at the recruited base.

Reinforcing to six Wraiths opens an eastern approach around
the western Missile Turrets. The eastern Command Center's
Goliaths repel the first small assault. Three more paid
Wraiths join the formation. The nine-aircraft attack destroys
the eastern Command Center at (1040,1496) by frame 56,984.
Seven Wraiths survive with 20–120 HP. Both heroes remain alive
and the fresh HOME export exactly matches the attack save.
Twenty-two hostile buildings remain, including a western
Command Center and the Missile Turret line. A further attack
destroys the eastern Refinery at (1376,1296). At frame 57,882
all seven Wraiths and both heroes survive. An independent fresh
HOME process verifies both eastern structures are gone, the
21 remaining hostile buildings, and byte-identical SRAM.

A western staging run pays for two more Wraiths and reaches
42/42 displayed supply (84/84 half-units). A nine-Wraith attack
removes the northern Missile Turret at (528,1344). Fresh HOME at frame 64,503 restores all
nine aircraft, both heroes and byte-identical SRAM. The hostile
census contains seven turrets, eight depots and five other
buildings. Six of the seven remaining enemy Wraiths are clustered
by the western Starport. They engage the force after reload.
A second turret attack from that position loses five Wraiths
without damaging the target. Use the earlier staging save for a
new approach that separates enemy aircraft from the turret line.

| Local receipt | SHA-256 |
| --- | --- |
| Earlier staging SRAM | `3049d226fa3b9f60c32d70b15ed0376a08a763edb2ccc68c96b4ced097d85cf0` |
| Western Missile Turret attack | `0587019f430cec3382e80b5ee6030bd763ad3c8274c3bbb3d076218c4656f89d` |
| Western Missile Turret fresh HOME | `94ef81c771f372d71a38eea6d6857fefc2e57b0f0292a070b1ca21e89cd41596` |
| Second turret attack | `f784de0e78d93711283e484fe308de0f9da740755ca9039450512897b2213a1f` |

#### Expand supply and reinforce before the western assault

An ordinary construction order from the earlier staging
save pays 100 minerals to build a Supply Depot at (1104,200).
It completes by frame 65,724 and raises displayed supply
capacity from 42 to 50. The captured Starport takes a paid
150-mineral, 100-gas Wraith order at frame 65,790. The
tenth Wraith completes by frame 66,990.

Fresh HOME restores the force at 44/50 supply and the bank
at 812 minerals and 448 gas. Raynor has 269 HP and Kerrigan
250 HP. The SCV at the captured Refinery is still harvesting.
During the attempt, an A input moved the original nine
Wraiths back near the recruited base. This branch prepares a
larger force at home. The earlier western turret-kill save
holds the forward position. A second run of the same ordinary
inputs reproduces the checkpoint observations and save byte
for byte.

| Local receipt | SHA-256 |
| --- | --- |
| Depot and paid Wraith continuation | `6d52271de7406614aaeef881a77047bc20a9f86792f017aa2c67c8ba53a758ea` |
| Fresh HOME observations | `226f101271cbeaa22ae94cf789d94ebf105eeae74886be8b80044ba87987de9c` |
| Reinforced home force SRAM | `5443aa0f97ff386c2a7c8932fa964cf873e8e79001245431c6370b1384583e61` |

From the reinforced home save, three more paid Wraiths
complete. Fresh HOME at frame 75,800 has thirteen Wraiths
at 50/50 supply and both heroes at 269 and 250 HP. An
approaching hostile Wraith disappears before the assault.
The larger wing attacks from the northeast and removes the
turret at (528,1344) in this branch. Fresh HOME after the
withdrawal restores ten friendly Wraiths, two hostile
Wraiths and 20 hostile buildings at frame 84,749.

Another two paid Wraiths reinforce the attack on the turret at
(432,1280). It falls. Both remaining hostile Wraiths are absent
from the full fresh HOME census. The Starport takes incidental
damage and remains at 929 HP. Two friendly Wraiths are lost.
At frame 92,815 the ten remaining aircraft and both heroes
fresh-load with 932 gas.

A follow-up attack removes the western Starport. All ten
friendly Wraiths retreat, including three below 20 HP.
Fresh HOME at frame 94,976 restores both heroes at 269 and
250 HP, 1,020 gas and 44/50 supply. The census finds zero
hostile Wraiths and 18 hostile buildings, including the
western Command Center and six Missile Turrets. An independent
controller replay reproduces the attack checkpoints and SRAM
byte for byte.

The next sortie removes the northern Missile Turrets at
(400,944) and (336,944) and loses two Wraiths. Fresh HOME at
frame 96,735 restores eight Wraiths, both heroes, 1,092 gas
and 62 minerals. The hostile census has 16 buildings,
including four Missile Turrets, and zero Wraiths. An
independent replay reproduces the sortie and save.

The mineral worker is idle beside the recruited base and its
prior field is empty. An ordinary Harvest order to an adjacent
nonempty field returns eight minerals, lifting the bank from
62 to 70. Fresh HOME preserves the two workers' mineral and
gas orders, both heroes and all eight Wraiths. The order that
preceded the interruption still needs a replay from before
depletion to test automatic handoff.

| Local receipt | SHA-256 |
| --- | --- |
| Thirteen-Wraith fresh HOME | `d73a5ad034ea834c924683ce1711e88adf6e73bb7ffb06f226d5aef5bd8f7a7a` |
| Northeast turret attack and withdrawal | `9690aff56cb652598e3d4c290408290a40e8798ed9ec79c6932e3aadc1822b9e` |
| Fresh HOME with two hostile Wraiths | `1cd4a36633bd25ef67923955634514d8f661b3ae59d5cd81ef55772ae871d783` |
| Fresh save after the second turret falls | `964f9224e27b0f7832d09fd88c32b5b083af1f023038cfae2dfa58caaa46c3a1` |
| Western Starport assault | `1e89975fc608e0665f6e2f3bcff26947df8775a50d1eebc7176d530ab6d4a874` |
| Fresh HOME after Starport destruction | `3a5ef357c5fa7f930b4583829ae73a227f69e8de2a4c67e58a1797e1f76ad808` |
| Post-Starport SRAM | `012672dc8f05301c04183180285cb31b2574f0273da9e804026d41ef175a4591` |
| Two northern Missile Turrets removed | `34692b8b1155f99ba0730a8193a5ab8b153bd7b3644d8ac70e136de27e71ecb9` |
| Fresh HOME after the northern sortie | `b4e91d5b2e67c13b01faed8ba20aa1251ce12444a5e61484558c068922f0ab4a` |
| Mineral worker reassignment and deposit | `8dee86c45e2031215bd43f51ccd585e428b5bc15fd05f0dd57395811f39d7592` |
| Fresh HOME with resumed mineral income | `b21dfea87a4aa5177b2ba0305c05b0263223cc7f1ac7e653a6eaa0c4d30fffed` |

#### Terran 5 victory and fresh HOME

The recovered mineral and gas orders fund five more Wraiths.
The first air attack removes a Missile Turret at (304,1264).
The next removes the western Factory and four other buildings,
including a second turret. Fresh HOME at frame 135,584
preserves eleven Wraiths, both heroes, 1,128 minerals, and
ten hostile buildings. The exported SRAM is byte-identical.

The next stages remove the Barracks, both remaining turrets
and western Command Center. Twelve Wraiths attack the final
four Supply Depots. At frame 170,041, the game reaches the
victory screen with both heroes alive. A fresh HOME load
restores RESULT, the Terran 5 completion bit and the same
victory SRAM. All orders use normal controls on the exact
installed Terran image. A second controller replay of the
final attack produces the same checkpoints and victory SRAM.
From a separately loaded victory save, A advances into the
playable Terran 6 field and retains the Terran 5 completion bit.

| Local receipt | SHA-256 |
| --- | --- |
| West Factory SRAM | `8f4e43561641363f17c1d72453f65bbdd6a3f26d4e68847532921f0d78e74e71` |
| West Factory fresh HOME | `958e327a17647a47d8e5f8d140804652571b928b62f1053c66cddd3a2b9246da` |
| Final depot attack | `e20dff44342515903f644ae77ce0060afdeef06ce392fc15c1c003ac73f4e71b` |
| Victory SRAM | `ae43a22a259d651ff4e975449b993209b53e72d147cd893bdf6d30752261f876` |
| Fresh HOME and Terran 6 deployment | `5addd6b63f37eb6153d1f4c80cbd4dfbc2ac5e0e8e57c7dd703114a953f16908` |

## Custom Game saves

Thirty-six early Custom Game save/continue pairs pass across all
four products. Each saves after 120 updates, loads from a fresh
HOME process and advances another 120 updates. The checks compare
the saved and restored lobby, game type, seat masks, resolved starts
and world state, then compare the resumed world with uninterrupted
play. There are 24 Free for All cases across both maps and 12 Blood
Bath Melee cases, spanning one, two and three CPU lineups.

Six later matches also pass across the three race cartridges and
both maps. They save after 11,000 updates with Terran, Zerg and
Protoss CPUs active. The saved worlds contain 120–156 living
units and 45–58 buildings. A fresh HOME load restores the lobby,
game type, seats, starts and persisted-world fingerprint in each
case. The fingerprint 180 updates later matches the uninterrupted
run, and readback preserves the full 32 KiB journal byte for byte.

### Random races and restart

Twenty-seven ordinary-button tests cover the three race ROMs and
both maps. On Blood Bath, six cases per cartridge use Human Random
and one, two or three CPUs across Melee and Free for All. Three
Outpost cases per cartridge use a named human Protoss player and
the same CPU layouts. Each race cartridge resolves Human Random to
all three playable factions somewhere in its six Blood Bath trials.

Each case confirms the chosen rows and starting locations, saves
after 240 updates, loads in a fresh HOME process and matches the
uninterrupted world after 120 more updates. SRAM readbacks match
byte for byte. Restart from pause uses the same resolved human and
CPU races, occupied rows and hall locations. The [native launch
observation](2026-11-12-n64-blood-bath-opening.md#random-races-consume-the-launch-stream-first)
records native Random race and start draws for its pinned seeds.

## Three-CPU cartridge pacing

With the player using the cartridge's race, three explicit CPU
races join each map. The player provides no orders after launch.
Outpost uses Free for All and Blood Bath uses Melee. Three windows
sample advancing main-loop cycles, including presentation. Rates
use the ARM7 guest clock in mGBA.

| Cartridge and map | Opening | After 9,000 updates | After 11,000 updates |
| --- | ---: | ---: | ---: |
| Terran 10 Outpost | 29.85 | 18.71 | 16.30 |
| Terran 10 Blood Bath | 29.90 | 23.21 | 17.90 |
| Zerg 10 Outpost | 29.90 | 17.52 | 14.98 |
| Zerg 10 Blood Bath | 29.90 | 21.21 | 15.45 |
| Protoss 10 Outpost | 29.92 | 17.71 | 14.14 |
| Protoss 10 Blood Bath | 29.91 | 20.86 | 14.20 |

Rates are emulated updates per second. All windows contain 600
updates except the late Protoss Blood Bath window, which ends at
RESULT after 521 advancing updates. Every later sampled update
exceeds the two-VBlank budget. Even the openings have 123–170
overruns of that budget per 600 updates. The 30 fps target remains
open for busy play on these cartridges.

### Pacing with one or two CPUs

The same three race cartridges also run both maps with a Terran CPU
in the first row, then with a Zerg CPU added in the second row in a
separate reset. Both maps keep the same game type as above, and the
human uses the cartridge's race and issues no orders. Each launch
checks its requested CPU mask, factions and distinct start locations.
The table compares the final window beginning after 11,000 updates.

| Cartridge and map | One CPU | Two CPUs | Three CPUs |
| --- | ---: | ---: | ---: |
| Terran 10 Outpost | 28.25 | 19.75 | 16.30 |
| Terran 10 Blood Bath | 29.89 | 24.69 | 17.90 |
| Zerg 10 Outpost | 26.19 | 18.43 | 14.98 |
| Zerg 10 Blood Bath | 29.86 | 24.36 | 15.45 |
| Protoss 10 Outpost | 26.13 | 19.47 | 14.14 |
| Protoss 10 Blood Bath | 29.45 | 23.65 | 14.20 |

All 12 new cases start at 29.85–29.93 emulated updates/s. In the
late two-CPU cases, every update exceeds the two-VBlank budget.
Late one-CPU Outpost overruns range from 510 to 600 of 600 updates.
The one-CPU Blood Bath range is 140 to 411 overruns of 600 despite
its higher mean rate. Blood Bath resolves different start locations
across lineups, so these are full-match observations for the stated
configurations. They measure guest cycles in mGBA.

### Late frame budget with one or two CPUs

A second set of 12 ordinary-button runs samples 120 consecutive
updates at launch and after updates 9,000 and 11,000 in each
configuration. All 4,320 total-cycle samples and resolved
seats match the corresponding portions of the retained pacing
replays. The component counters also pass the nesting checks.
The player gives no orders, with the starting camera fixed.

| Race ROM and map | One CPU total | One CPU orders | One CPU draw | Two CPUs total | Two CPUs orders | Two CPUs draw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran Outpost | 592 | 251 | 272 | 842 | 424 | 299 |
| Terran Blood Bath | 561 | 191 | 222 | 669 | 334 | 223 |
| Zerg Outpost | 641 | 251 | 308 | 902 | 441 | 329 |
| Zerg Blood Bath | 561 | 198 | 263 | 681 | 295 | 265 |
| Protoss Outpost | 641 | 252 | 300 | 852 | 392 | 320 |
| Protoss Blood Bath | 567 | 230 | 247 | 712 | 328 | 252 |

Values are mean thousands of guest cycles after update 11,000.
Outpost needs a 5.2–12.4% reduction in total mean cost with
one CPU and 33.2–37.7% with two. On Outpost the extra order cost
is 66–73% of the increase from one to two CPUs. In the one-CPU
Blood Bath cuts, the mean is near budget but 25–77 of 120 frames
still exceed two VBlanks. With two CPUs, every complete frame
exceeds the budget across both maps. Simulation alone exceeds
budget on 12–63 of 120 two-CPU Outpost samples. Keep a one-CPU
Blood Bath tail check and prioritize savings in orders for
Outpost, then measure those savings together with drawing.

The detailed local receipt has SHA-256
`161b4ffa9d460ac48acb701817d9a1d47e543b5c8f21ad36ec848ae988bf761a`.

Eight read-only instruction cuts per Outpost configuration
match separate uninstrumented runs on elapsed cycles, both
framebuffer pages, main world state, palette and camera.
Adding the second CPU increases inclusive movement cost from
178 to 334k cycles on Terran, 195 to 338k on Zerg, and 116
to 193k on Protoss. Movement accounts for 61–81% of the
additional order cycles in these cuts. Uncached contact
masks account for 68–203k of the two-CPU updates; point route
lookup accounts for 10–13k. Those routines can be nested
inside movement. Target contact work before expanding the
point-route cache for this measured workload.

The instruction receipt has SHA-256
`6f0adcd8555466213ebc464a7a3ac065489e07cf4a9df0066000d0d78f6bc0ff`.

#### Contact queries in those Outpost windows

Read-only entry and return observations record 1,299 contact
queries across the same six eight-update Outpost cuts. Each
cut matches the uninstrumented run on elapsed cycles, cycle
counters, both video pages, palette, display, camera and
world state. The installed Terran and Zerg kernels run in
Thumb ROM. Protoss uses ARM in IWRAM.

Author-created ARM fixtures replay the captured bucket inputs
at the corresponding placements. All masks match their
observed cartridge values. Keeping the query origin and map
dimensions in locals saves 0.11–0.19% of the Terran and Zerg
fixture kernel cost and 6.50–7.03% on Protoss. The Protoss
fixture explicitly keeps the kernel out of line and checks
its IWRAM address. Trials with
incremental bucket row addresses or a prepared geometry
table offset make the busiest Thumb cases slightly slower.

The early axis check already used in the IWRAM kernel saves
more when enabled in the speed-optimized ROM fixture.

| Case | Original kernel cycles | Early check cycles | Reduction |
| --- | ---: | ---: | ---: |
| Terran, one CPU | 841,775 | 627,127 | 25.5% |
| Terran, two CPUs | 1,394,919 | 1,088,709 | 22.0% |
| Zerg, one CPU | 945,323 | 690,035 | 27.0% |
| Zerg, two CPUs | 1,555,013 | 1,213,921 | 21.9% |

The kernel gains equal approximately 4.9–5.2% of two-CPU
loop cycles if the fixture rates carry to the measured
contact costs. The trial adds 92 bytes to each fixture
kernel and changes the linked BSS endpoint by zero or four
bytes. Whole-ROM timing, stack, RAM and save checks still
need a paired run. Use the ROM early-axis trial first for
busy Terran and Zerg Outpost qualification.

| Local receipt | SHA-256 |
| --- | --- |
| Verified Outpost query census | `835855435a5343bbbaf28187f24febce13960f95cbd9d026f0ea337a6fa13a86` |
| Thumb local variables and incremental row addresses | `adf2bcdd19826bd73eba8360b4cc8a17fbf5c738ad34228531f0833d283742b8` |
| Protoss IWRAM fixture with placement verified | `4acf4b134893ac7020cd593f0c7865c7473c81add398df6461f6493e5b7c4d2d` |
| Prepared geometry offset | `96fc97d65e95a7eb099dbff5f46a0ddd962632bfc1a8ac7f0c8e295b72cbaad7` |
| ROM early-axis matching masks, timing and memory | `32218fb7d46a7a91778cc2b27dbb88e760b875e56bd7678a9c8556cdcd594d9e` |

### Late camera movement

The six three-CPU match configurations also run a 360-update
window following update 11,000 with ordinary directional input.
The player pans toward the center and then reverses toward the
starting area. Each pair starts with the same camera and resolved
seats. Five world fingerprints match between the moving and fixed
runs in each pair. The camera moves on every sampled update.

| Cartridge and map | Fixed camera | Moving camera | Change in mean cycles |
| --- | ---: | ---: | ---: |
| Terran 10 Outpost | 15.60 | 15.76 | −0.99% |
| Terran 10 Blood Bath | 20.10 | 19.15 | +4.92% |
| Zerg 10 Outpost | 13.99 | 14.39 | −2.75% |
| Zerg 10 Blood Bath | 17.22 | 17.67 | −2.57% |
| Protoss 10 Outpost | 13.36 | 13.57 | −1.59% |
| Protoss 10 Blood Bath | 15.45 | 16.05 | −3.75% |

Rates are emulated updates per second. Every sampled update
exceeds two VBlanks. Camera motion also changes visible draw
work, so each pair measures that complete view change. A fixed
camera already incurs most of the measured late-frame cost.

### Late simulation and drawing budget across the race ROMs

On the same three-CPU configurations, the ordinary-button observer
reads the release ROM's built-in cycle counters for 120 consecutive
advancing updates at launch, after update 9,000 and after update
11,000. The player gives no orders and leaves the camera at its
starting location. Each launch checks the occupied rows, resolved
races and start locations. All 2,160 samples pass the counter
nesting checks and keep the ROM, ELF and runner hashes unchanged.
The six late windows match the corresponding fixed-camera replays
in starting world fingerprint and all 720 total-cycle readings.

| Cartridge and map | Total | Simulation interval | Orders | Drawing interval | Actors and effects | Simulation overruns |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Terran 10 Outpost | 1,058 | 636 | 481 | 421 | 262 | 117/120 |
| Terran 10 Blood Bath | 850 | 606 | 478 | 243 | 85 | 82/120 |
| Zerg 10 Outpost | 1,275 | 804 | 628 | 470 | 283 | 120/120 |
| Zerg 10 Blood Bath | 929 | 603 | 457 | 325 | 134 | 84/120 |
| Protoss 10 Outpost | 1,268 | 799 | 610 | 469 | 288 | 120/120 |
| Protoss 10 Blood Bath | 989 | 661 | 501 | 328 | 153 | 110/120 |

The table shows mean thousands of guest cycles after update 11,000.
Orders are part of the simulation interval, and actors and effects
are part of the drawing interval. Each simulation mean exceeds the
561,792-cycle two-VBlank budget by 41,354–242,681 cycles. All 720
total samples in this late window exceed the same budget. Orders
account for 45–56% of total cycles across the six means. Actors
and effects account for 35–62% of drawing, compared with 43–52k
cycles for the measured terrain background stage. The launch
windows average 29.72–29.99 emulated updates per second and the
late windows average 13.16–19.73.

A later independent pair repeats these six launches and
all 2,160 cycle observations twice with byte-identical
receipts. The repeated census has SHA-256
`faa11662f1f3d880a5e8c8a7efc66e9c16aeb831a6132f63ea2badc170f769b0`.

The profiling intervals charge interrupt work to the phase in
which it occurs. The simulation interval also includes input reads.
The residual includes audio update and presentation outside those
two intervals. It typically includes a pacing wait in the launch
window and averages under 500 cycles in the late windows. Use the
instruction attribution below when comparing individual routines.
These are cycle measurements in mGBA.

Eight updates per late window also run under the instruction and
call-stack observer. All 48 cuts match ordinary unprofiled replays
on sampled world state, both framebuffer pages, palette, camera,
display, and elapsed cycles. The built-in phase counters also
match the table's underlying samples. The instruction and stack
totals reconcile with each cut's emulator clock.

| Cartridge and map | Movement | Uncached contact masks | Point route lookup | Depth-ordered drawing |
| --- | ---: | ---: | ---: | ---: |
| Terran 10 Outpost | 198 | 91 | 15 | 235 |
| Terran 10 Blood Bath | 326 | 105 | 99 | 69 |
| Zerg 10 Outpost | 292 | 129 | 23 | 257 |
| Zerg 10 Blood Bath | 247 | 92 | 21 | 104 |
| Protoss 10 Outpost | 210 | 56 | 23 | 264 |
| Protoss 10 Blood Bath | 225 | 51 | 41 | 131 |

Each entry is a mean in thousands of inclusive guest cycles over
eight updates. Movement is `move_toward`, contact masks are
`sc64_contact_blocked_mask_uncached`, and point route lookup is
`sc64_route_next`. Calls can nest. Within the Terran Blood Bath
point route lookup, `sc64_route_monotone_probe` alone accounts for
81k direct cycles. Uncached contact remains a substantial cost
on all six configurations. Depth-ordered drawing is consistently
heavier at the sampled Outpost starting views. These cuts support
separate probes for collision traversal and the monotone route
test, with actor and effect drawing paired across both maps.

### Monotone route search outcomes

The exact Terran 10 and Protoss 10 images run both three-CPU maps
with ordinary buttons and no player orders. At launch, after update
9,000 and after update 11,000, read-only breakpoints observe the
monotone point-route call and return for 24 advancing updates.
All 288 measured updates match uninstrumented controls on world
fingerprints, both framebuffer pages, palette, display, camera,
phase counters and elapsed cycles. Zerg 10 uses a separate point
route path in these builds and is outside this call-site census.

| Cartridge and map | Calls after update 11,000 | Searches visiting at least 32 nodes | Most visits in one search | Mean probe cycles per update |
| --- | ---: | ---: | ---: | ---: |
| Terran 10 Outpost | 68 | 0 | 10 | 6,566 |
| Terran 10 Blood Bath | 70 | 29 | 226 | 88,808 |
| Protoss 10 Outpost | 83 | 0 | 15 | 8,382 |
| Protoss 10 Blood Bath | 59 | 12 | 135 | 25,126 |

All 280 late calls find a path. Across all three cuts, 496 of 498
calls find a path and the other two exhaust their monotone search
without hitting the 512-visit budget. The longest individual
Terran Blood Bath call takes 82,428 guest cycles in this sample.
The late Terran Blood Bath probes account for 10.3% of mean
update-loop cycles. Prioritize cheaper successful long routes in
these windows. Increasing the budget would skip none of the
observed visits.

An expanded Terran Blood Bath run reads 277 calls over 96 advancing
updates from the same late boundary. All succeed. The observed
walkability fingerprint stays identical, and 122 distinct endpoint
pairs occur. Searches with at least 32 visits account for 96.0% of
measured probe cycles. Four calls repeat a query within the same
update and another 151 repeat one from a prior update.

An offline eight-entry LRU simulation using exact endpoints and
the walkability fingerprint would cover 83 calls. Those calls are
all one-visit searches and account for 0.20% of the total observed
loop cycles. A 32-entry LRU would cover 131 calls and 1.28% of
loop cycles. The simulation has no lookup or invalidation cost.
These bounds favor improving the long searches over an endpoint
cache for this workload.

A second replay of those 96 updates also tags calls whose origin
is an earlier returned predecessor for the same goal and grid.
It produces the same 277 calls and cycle readings. Among first-seen
endpoint pairs, 99 meet that continuation condition. They account
for 5.75 million guest cycles, or 7.07% of complete loop cycles in
the sample. Seventy of those calls occur ten updates after the
earlier query. These counts measure workload overlap before any
cache overhead or route-slot eligibility checks.

An asset-free sanitizer check records the parent of the returned
step in the original search and asks for a fresh route from that
step to the same goal. Across 110,940 successful nontrivial cases
on byte and packed grids, the fresh search returns the recorded
parent within the original visit count. This covers all 3×3 masks
and endpoint choices at five caps, plus randomized larger grids
and 26,640 successful cases with blocked goals. A runtime design
would retain the actual path and preserve route-slot precedence,
request aging, and invalidation on topology changes or load.

### Protoss 10 late battle attribution

On the same Protoss 10 Blood Bath Melee lineup, a read-only ARM
instruction profiler samples eight updates after 9,000 and another
eight after 11,384. The profiled and ordinary runs match the observed
world, both framebuffer pages, palette, camera, display and elapsed
cycles in each cut. Instruction and call-stack cycle totals balance.

The early cut averages 778,400 guest cycles per update. The late cut
averages 1,342,585, about 2.39 times the two-VBlank budget. In that
late cut, the game update uses about 654,022 cycles, rendering uses
581,292 and audio IRQ work uses 106,345. Orders account for about
495,182 cycles within the update. Movement and resource-worker
orders account for 152,204 and 82,688 respectively. Depth-ordered
actor drawing uses about 396,340 cycles within rendering, including
154,678 for unit images and 106,647 for tracers. These nested costs
identify the expensive paths in the measured windows.

The same 16 cuts contain 450 uncached ground contact queries
with speeds one through three. Of 2,406 observed bucket visits,
1,622 are nonempty. Each mover, position and speed combination
occurs at most once within a sampled update. The late cut also
draws 164 packed sprites across 3,251 rows. These counts provide
workload sizes for synthetic renderer and collision tests.

### Sprite crop and projectile draw calls

Read-only call/return traces cover frames 11,002–11,009 on each
race cartridge and map. The 48 fixed-camera cuts match the
ordinary runs on world, both framebuffer pages,
palette, camera, display, phase counters and elapsed cycles.

The renderer makes 18 exact unit crop calls. Seven are repeated
successful checks on the same unit between painter admission and
body drawing. The repeats cost 11,028 cycles across the 48 cuts,
with six of the seven on Zerg 10 Blood Bath. A separate eight-update
Protoss 7 cut after frame 48,426 makes zero exact crop calls. That
run matches the retained frame-48,426 world and video and the
frame-55,892 victory state and video. It covers one camera position.
A cache for repeated exact crops has little payoff in these cuts.

| Cartridge | Outpost tracer calls/update | Outpost tracer cycles/update | Blood Bath tracer cycles/update |
| --- | ---: | ---: | ---: |
| Terran 10 | 55.6 | 50,801 | 0 |
| Zerg 10 | 62.3 | 78,069 | 3,059 |
| Protoss 10 | 57.8 | 55,215 | 6,408 |

The base projectile-only pass accounts for 39,563–64,678 cycles
per Outpost update. It spends more cycles than the attacker-image
calls or the split trail passes on each race cartridge. None of
these calls collects sorted ground-trail picks. A separate readback
of each sampled draw slot finds only layers 5 and 19, above the
ground-layer world-Y projection. Both proposed reuse paths have
zero covered draws here. This package predates the [FOLLOW
viewport change](https://github.com/ggu/starcraft-gba/pull/782).
Profile the current source's base projectile motion and raster
work at this fixed camera position and after an ordinary-input
pan to the visible fight. These are mGBA guest-cycle observations.

#### Pan from the base to the Outpost fight

Across three exact race-ROM runs, an ordinary R and D-pad hold
moves the Outpost camera from (40,106) to (520,790) before frames
11,002–11,009. Both views match the world and route counters. The
traced pan matches a second uninstrumented pan across both video
pages, display, palette, counters, and elapsed cycles.

| Cartridge | Fixed tracer cycles/update | Panned tracer cycles/update | Panned cycles/update in the watched raster function |
| --- | ---: | ---: | ---: |
| Terran 10 | 50,801 | 85,317 | 9,713 |
| Zerg 10 | 78,069 | 390,611 | 204,004 |
| Protoss 10 | 55,215 | 324,503 | 155,848 |

The watched function is the raster entry on Terran and Protoss
and the resolved image-frame entry on Zerg, where the raster is
inlined. Those costs are inclusive and are part of the tracer
costs. The panned Zerg and Protoss cuts spend about half of their
tracer cost inside that function.

All 272 split trail draws across both camera cuts use bullet type
0. The merged FOLLOW viewport rule therefore does not need to
expand to those split draws for this workload. The fixed camera
places every sampled cached launch and target envelope outside
the viewport using the conservative 66- and 61-pixel radii. These bounds
exclude none after the pan. Compare the offscreen rejection and
visible raster work separately. This measures the installed build,
which predates the merged FOLLOW change.

#### Reusing effect visibility for each tile row

A paired 120-update P10 Outpost replay uses the installed image
and a retained build from its source revision with the tile-row
renderer trial. It starts at frame 11,002 and tests both views.
All 240 frame pairs match the world, camera, both framebuffer pages,
palette, display and route counters. Both links have the same
IWRAM and EWRAM static endpoints.

| Camera | Mean loop cycles, original → trial | Mean draw cycles, original → trial | p95 loop cycles, original → trial |
| --- | ---: | ---: | ---: |
| Fixed | 1,268,499 → 1,268,401 | 469,200 → 468,820 | 1,485,000 → 1,491,629 |
| Panned | 1,456,009 → 1,337,136 | 597,250 → 476,840 | 1,696,269 → 1,572,520 |

At the fight, mean loop cycles fall 8.16%, mean drawing falls
20.16% and p95 falls 7.30%. The fixed-camera mean is flat and
its p95 rises 0.45%. All 120 updates in each case remain above
the two-VBlank budget. These measurements apply to the retained
P10 pair. Qualify the current combined source against the other
cartridges, campaign and Showcase, including save continuation,
before promoting the renderer to the release builds.

#### Screen-edge visibility in the whole-raster path

The November 25 trial limits the whole-raster fog check to horizontally
visible pixels while keeping the blitter's original source coordinates and
stride. The sanitizer raster oracle confirms identical pixels at the left
map edge and the right screen edge with hidden offscreen tiles. The four
prepared ARM links retain their static memory endpoints.

Seven 160-update pairs cover panned three-CPU Outpost for every race,
three Blood Bath variations and an active Z9 campaign checkpoint. The
sampled worlds, both video pages, palette, camera, pause SRAM and fresh
continuation agree in all pairs. Seven sampled display-phase states differ.
Mean loop time is flat or rises by up to 0.12%. The panned Outpost drawing
means increase by 0.06–0.63%. The Z9 p95 improves from 1,877,834 to
1,868,710 cycles while its mean loop time rises 0.05%. Budget overruns stay
constant in all seven windows. Keep the current raster and use traces of
the visible fight to identify a larger remaining cost.

The local comparison receipt SHA-256 is
`99b8ddef059b6d8ab32b9d66ac083c05c4e1b66da9a04df3a82fdacc9e27df96`.

#### Blood Bath holdout and late skirmish journals

The same P10 pair completes 120 Blood Bath Melee updates in each
of two camera positions from frame 11,385. Each pair matches world
state, route counters, both framebuffer pages, palette and the
selected scanout. In one panned frame, 11,386, the display registers
differ by the single Mode 4 page bit. The raw bytes prove the two
pages are identical within and across builds at that boundary.
All remaining display register readings match exactly.

| Blood Bath camera | Mean loop cycles, original → trial | Mean draw cycles, original → trial |
| --- | ---: | ---: |
| (24,850) | 1,394,509 → 1,394,508 | 677,980 → 678,255 |
| (512,594) | 955,730 → 955,579 | 279,836 → 279,581 |

The effect on mean frame time in this holdout is under 0.02% at
both views. Every measured update still misses the two-VBlank
deadline.

Two separate P10 runs save the mixed three-CPU matches at frame
11,001 on Outpost and Blood Bath. Ordinary fresh HOME loads each
saved lobby and continues 180 updates. Both saves and both exports
match the original byte for byte. Lobby settings, resolved seats,
starting positions, world and continuation checks also match.

The current source also passes a 5,000-case asset-free Mode 0
differential under ASan and UBSan. The baseline and candidate
digest the same tile pixels, dirty, drawn and art masks, and
HUD flags. This is host renderer evidence. The current combined
race and Showcase cartridge matrix remains open.

#### Protoss 7 victory from a saved campaign

Both builds in the retained P10 tile-row pair load the same frame-8,845
Protoss 7 journal and replays 48,540 recorded controller updates.
All 47 sampled checkpoints match the world, visible and explored
tiles, impacts, mission state, raw framebuffer pages, palette
and selected scanout. Both runs win at frame 55,892. Their victory
SRAM matches the original recorded run byte for byte, and a
fresh HOME process per build restores the victory result.

The original uses 84,448,954,675 cycles and the trial uses
84,447,543,282 cycles across the replay, a 0.0017% reduction.
This extends gameplay and save coverage for the retained pair.
The latest combined source still needs equivalent production
cartridge checks.

### Paired ARM replay of observed route queries

A repeat of the 96-update Terran Blood Bath cut records the ordered
inputs and results of all 277 point probes on one unchanged packed
walkability grid. It uses the installed cartridge and ordinary
controller inputs. The breakpoint and uninstrumented runs agree
on the world, video, camera, phase counters and elapsed cycles at
each update. The calls spend 8,440,631 cycles within an 81,397,508-cycle
window on that image.

A freestanding ARM7 Thumb fixture runs the original and incremental
probe from [draft #802](https://github.com/ggu/starcraft-gba/pull/802)
with the observed inputs, an EWRAM grid and cartridge wait setting
`0x4317`. All 277 statuses, first steps and five search counters
match the original cartridge and both fixture builds.

| Compiler mode | Original fixture cycles | Incremental fixture cycles | Reduction |
| --- | ---: | ---: | ---: |
| `-O2` | 7,880,987 | 5,940,318 | 24.62% |
| `-Os` | 9,291,740 | 5,943,830 | 36.03% |

At `-O2`, eight calls grow by at most 13 cycles each. Applying the
fixture's measured deltas to the original 96-update total suggests
a 2.38% loop-cycle reduction. This projection uses one steady-grid
window and excludes integrated code placement and dispatch interactions.
Whole-cartridge timing, RAM qualification and GBA SP measurements
remain open. The captured grid and query inputs stay local.

The same 277 inputs also exercise the incremental route wrapper
with and without the 16-path continuation table from
[draft #805](https://github.com/ggu/starcraft-gba/pull/805).
Both return the expected next cell on every call. The `-O2`
fixture drops from 6,273,133 to 1,749,728 cycles, a 72.11%
reduction. A total of 169 calls grow by up to 2,441 cycles.
The savings occur in 62 update buckets, while 34 grow, with
a maximum growth of 4,062 cycles in one bucket.

This wrapper replay starts with empty route slots and an empty
continuation table. The request clock is pinned to an eligible
value. Qualification with the full dispatcher must cover existing
route hits, age boundaries, earlier cache history and topology
changes. The fixture adds 360 bytes of linked BSS.

### Projectile fog spans on current Terran and Showcase

[PR #823](https://github.com/ggu/starcraft-gba/pull/823) was
linked with prepared inputs on the source release at `ba2541bd`.
The tested merge tree is `048c4022`. The initial Terran and
Showcase links use the unpacked shared sight path. The Terran runs
load the same late skirmish journals. Eight 120-update
windows cover two and three CPU opponents, both maps and
fixed and panned views. Worlds, camera, palette, both
framebuffer pages and every save continuation agree. The
six two-CPU and Outpost cases match all cycle samples. The
three-CPU Blood Bath means improve by less than 0.01%.

Five Showcase scenes start through the ordinary menu and
compare 120 updates per scene. Worlds, palette and both
pages agree. Carrier Approach and Protoss Crossfire mean
frame costs fall 0.37% and 0.29%. The other three means
change by less than 0.03%. Both product pairs have identical
linked working RAM and IWRAM endpoints.

The selected page differs at one Terran and one Showcase
stop. At both stops, the candidate has already completed one
more VBlank flip and has no pending page. The control still
has a queued page. Cumulative flip counts and selected
page agree again at the next input boundary, with both pages
matching throughout. This comparison does not measure
physical display duration. The current samples show modest
or flat timing, and the earlier stronger panned Protoss
result is covered by the canonical comparisons below.

| Local receipt | SHA-256 |
| --- | --- |
| Late Terran pair summaries and per-window hashes | `bcbf5aca52f33fb33b051c2c7958f0edb65fb4470fd15eca12740b9ed87e5a5e` |
| Five paired Showcase scenes | `ebb6721eb8869ffffb11a0eea85d65afe787d2f6fc14a8b2b1e19b835431f2f0` |
| Selected page, pending flip and cumulative flip checks | `5314da82da885fedd5fd7ebaf70f2a8432fdd2e45c7c56d83f24fa448df18b5f` |

#### Canonical renderer qualification on all four profiles

The corrected Terran and Showcase links include the normal
packed and sparse shared sight flags. Z10 and P10 use their
normal sight configurations. All four links pass the existing
RAM reserve guards. The three-CPU Terran samples match in both
views on both maps, including both pages, selected scanout
and fresh HOME saves. Mean timing is effectively flat.
The two Showcase scene pairs match selections and pixels
and improve mean timing by about 0.2–0.25%.

Canonical P10 and Z10 each measure 120 ordinary updates
after panning to (520, 790) in a three-CPU Outpost fight.
Both pairs match world, both framebuffer pages, palette
and selected display at the observed input boundaries.
The regular pause journal and 60-update fresh continuation
also agree. Z10's presentation interval counts differ, so
matching boundary images do not establish equal flip times.

| Product | Panned mean cycles, control → renderer | Reduction |
| --- | ---: | ---: |
| Protoss P10 | 1,474,500 → 1,349,855 | 8.45% |
| Zerg Z10 | 1,279,205 → 1,186,722 | 7.23% |

The fixed P10 holdout and both default-camera Z10 runs
match timing. Every sampled update in the two panned fights
still exceeds the two-VBlank budget. The renderer merged
in [#823](https://github.com/ggu/starcraft-gba/pull/823).

| Local receipt | SHA-256 |
| --- | --- |
| Canonical Terran four skirmish pairs | `35086baa1b95424274c69df70992e3b39f42f8c09d0c46aa1da686bfb6329f19` |
| Canonical Showcase scene pairs | `d5397043aab33cc2f777d8e90a506380d2848d96bd73887fce57ea6f486e91a3` |
| Canonical P10 fixed and panned Outpost | `e8cf4c1185ef2046d72681ed3065218d68b4206f95b41c93d0133e0e8ddfd5c4` |
| Canonical Z10 fixed Outpost | `70bbc2cda6f6725dfc031339f22fb4680ad91758bd46746c5d4524568b703abd` |
| Canonical Z10 panned Outpost | `611fc257aa9b6079dfc858ab4bd2caa16fa1f66abef8cf5cf6d43003fae9db3d` |

#### Compose the renderer and ROM collision option

The combined Z10 merge tree `992670ca` builds with the
collision option off and on. The off build reproduces the
renderer-only ROM byte for byte. The same panned Outpost
replay saves 1.03% in mean whole-step cycles when enabling
the option, from 1,186,722 to 1,174,526. The p95 rises
from 1,527,386 to 1,536,482. All 120 samples remain over
budget. The observed world, both framebuffer pages, selected
display, pause save and fresh continuation match. The resumed
presentation histogram differs. The flag stays off by default.
The local receipt has SHA-256
`8ba2f573608a1ff263362cbbef0e5bfa5bda9c8293f82b1963e5ef419736e6ff`.

### Reusing contact scan coordinates in complete frames

The [query-coordinate trial](https://github.com/ggu/starcraft-gba/pull/828)
matches observed worlds, both framebuffer pages and save
continuations in 12 Terran skirmish pairs with unpacked
sight. All mean timing changes are within 0.02%. Two stops
select different display pages. Five Showcase scene pairs
with the same sight configuration match frames and cycles.
A canonical current P10 pair from the late three-CPU Outpost
journal gives exactly the same fixed and panned loop-cycle
samples and matches the display and journals. The kernel
fixture improvement does not carry over to these samples.

| Local receipt | SHA-256 |
| --- | --- |
| Terran skirmish pairs | `4dd8cabf52bcf62bb97ebce46f61d641bcdef38b81a46176180ed1fc105b8338` |
| Showcase scene pairs | `2e4265c793d5aac9a144bc1044f6aca39df1a7d4ea4c340f05d6d8b7b21a363a` |
| Canonical P10 fixed and panned pairs | `4388e7b4e96ea461d641234cffdfa0a6738d68404ebb39da01960b4cbafaa46e` |

### Monotone route witnesses on two current Terran saves

The [witness trial](https://github.com/ggu/starcraft-gba/pull/805)
uses merged tree `b79233fd`. Both links enable the existing
clear-prefix path. Only the candidate enables the witness
table. Each pair loads the same late Terran journal and
compares 120 fixed-camera updates. Two-CPU Outpost mean
cost is flat at about 868,840 cycles. Three-CPU Blood Bath
falls from 849,265 to 836,050 mean frame cycles, a 1.56%
reduction. Every measured frame remains over budget.

Worlds, palette, camera, both framebuffer pages and saves
agree. One Blood Bath input boundary selects a different page.
The trial does not observe live witness-hit counts. Extend
the pair to active orders and profile the cache before
changing the default.

| Local receipt | SHA-256 |
| --- | --- |
| Outpost stock / witness | `835e54c19a185059dbc188f28308e340b57e6cc7cb3b8e38a86c05cbb9b80bb4` / `1ddef2d373f6e25e4dbff3137add25478c81494c2f90369cd8c6dd2cac259db6` |
| Blood Bath stock / witness | `c4f3e288b41471ef0ffba990bdb9da632da13295d2ac786a01d1a385aa46e716` / `ddc65b9d89773734f09343c63d6d04708e767137edd040effd244d85791f2ca8` |

### Pylon lookup in dense missions and panned skirmishes

The [Pylon grouping trial](https://github.com/ggu/starcraft-gba/pull/834)
links canonical Z10 and P10 from prospective merge tree
`c7cb9cc2` with the probe off and on. The off images match
the canonical renderer build exactly. Each ordinary HOME
load runs 160 measured updates. The skirmish camera pans
to (520, 790) before measurement. The grouped path
is exercised at 69 completed Pylons and the short-list
path at nine.

| Workload | Complete Pylons | Mean cycles off → on | Reduction |
| --- | ---: | ---: | ---: |
| Z9 opening technology, frame 8,925 | 69 | 865,254 → 848,370 | 1.95% |
| Z9 first road assault, frame 27,560 | 69 | 1,725,547 → 1,707,993 | 1.02% |
| P10 Outpost with three CPUs | 9 | 1,351,530 → 1,351,102 | 0.03% |
| Z10 Outpost with three CPUs | 9 | 1,179,049 → 1,178,436 | 0.05% |

The world and entity observations, palette, both VRAM
pages, fresh HOME and 60 resumed updates agree. The
four journal pairs are byte-identical. One Z9 and two
Z10 samples stop with different displayed pages. The
P10 skirmish 95th percentile increases from 1,568,384
to 1,594,906 cycles. All 640 frames per build configuration
exceed the 561,792-cycle target. The grouping trial stays
off while whole-frame, stack and display timing proofs
are incomplete. Local receipt SHA-256
`3596e376bc4d8fef988bf62ebe4517a7bec3d371dcfdf5e9aa5d4a5f1e407774`.


## ROM space available for cutscenes

The final ROM-backed load segment in each matching ELF ends exactly
where the cartridge's `0xff` padding begins. The 32 MiB files leave
the following space in the installed package.

| Cartridge | Padding bytes | MiB |
| --- | ---: | ---: |
| Terran 10 | 2,947,440 | 2.811 |
| Zerg 10 | 2,053,104 | 1.958 |
| Protoss 10 | 2,060,716 | 1.965 |
| Showcase | 3,396,720 | 3.239 |

The [native campaign audit](2026-11-04-n64-parity/README.md#what-was-reached)
observed a captioned still-image cutscene at the opening of a Brood
War episode. The USA N64 ROM's BOLT archive gives the actual
stored sizes below. Compressed entry sizes count input consumed
by the decoder and exclude inter-entry padding.

| N64 content | Stored bytes | MiB |
| --- | ---: | ---: |
| Opening animation, 308 indexed 160×100 frames | 2,017,192 | 1.924 |
| Dedicated opening soundtrack and six palettes | 171,288 | 0.163 |
| Complete opening payload | 2,188,480 | 2.087 |
| All 13 campaign slideshow scripts and their bank | 1,363,958 | 1.301 |
| Opening plus slideshow bank | 3,552,438 | 3.388 |

The slideshow scripts reference 38 unique images with 1,292,358
stored bytes. Their scripts consume another 8,356 bytes. The full
98-entry bank also counts palettes, display assets and any unused
entries there. It excludes separate shared audio banks and mission
briefings. Individual scenes reference roughly 24–166 KiB of still
images. One image is shared across two scripts. The [BOLT format
notes](https://github.com/dmang-dev/sc64-maps/blob/main/docs/FORMAT.md#55-related-script-directories)
identify the link between scripts and image indices. The
[native opening trace](https://github.com/n0madic/StarCraft64-recomp/blob/master/docs/research.md#audio-path-and-cinematics)
identifies the indexed animation with separate audio. The original
startup path selects video `002/008`, palettes `002/009`–`002/00E`,
and track `015/00F`.

The whole slideshow bank fits the measured space on each race ROM
by bytes. The native opening payload exceeds the Zerg and Protoss
headroom. Opening plus slideshow content exceeds the space on all
four ROMs. For a smaller re-encode, 30 seconds at a combined
96–160 kbit/s would use 360,000–600,000 bytes. This is a storage
estimate before decoder, alignment and other content. An authored
test clip is needed to verify decode time, scratch RAM and playback
quality on the GBA SP.

## Local receipts

| Check | SHA-256 |
| --- | --- |
| Custom Game launches | `8c990bc25747578cdb2ad4f269046e46e7f6163565a9ea3af2edc16e276622c5` |
| Native opening and slideshow storage, including opening audio | `635b532ef788ef11e04ce387b01ad156890f66ecd8945967864497615daf8b0e` |
| Random human and CPU saves and restarts | `19489f05aabd0dacfee3ac22a595a01a649308faeba4b92dabd039617bddb6a8` |
| Last open CPU row on the three race ROMs | `8000e67d551101aab15b51ef8d5d3f2816f150038fefe7388445aad82437b872` |
| N64 Blood Bath Melee last CPU replay | `05cba75a4bc6a037250ae56b65e32b18d1b32160a1cd4350242f2daf5036ed3b` |
| N64 Blood Bath Free for All last CPU replay | `c663999f091021032510d2bb8a3b1ba79e85e5c3c49515d3e843d0904e229aac` |
| Late three-CPU Custom Game saves across both maps | `e79983735242bb069af925993797d8c7e8edba8b2f715eab29432e6dc0bbdaa2` |
| One- and two-CPU Terran and Zerg pacing | `debd5b6b54170dd4dd49497ca50856c5db4341e2226429044bf6e55f7d37d739` |
| One- and two-CPU Protoss pacing | `05918d9053971077e8cc8b54f73c77dfa6446fe668610e2552413100b6f3e5ed` |
| Late camera movement across both maps | `130911b38530064b0e3f846fa32585752a276030565a8949a1479184dbc9a075` |
| Three-race frame-budget breakdown | `336e4cfc4cf66286f99b97ec10166c29bdb38c3b6e6cf6861396dd166e372bbe` |
| Three-race order and drawing instruction attribution | `fae6f5e019544c8bd74587010652717b685c93afbbc05f5bb31babea211e9874` |
| Late three-race exact crop calls | `71c00f4b93bd65b4f32f7ad80db1a516ec7fc8487b25bf24bc50ef50b7a9c207` |
| Protoss 7 late exact crop calls and victory | `1a716f7f8c938d9b8a1945c068c94a9d8c0bcaeeb059cd294028ca07a9d9e18f` |
| P10 tile-row paired Protoss 7 continuation, victory and fresh HOME | `ae8f7da97bb90b6d687cd7ec00b20d9e1767180924aa568149973f2b6a445b4d` |
| Late three-race tracer draw calls and timings | `e3ec7ca92ca312e0417034da49f19079826ce2faa992b3e0581621601b275ad6` |
| Late three-race Outpost pan and nested tracer/raster attribution | `367528075a2cffa35f0c45de9fc89215731a243305e3f3e310dfdfb361c2a50e` |
| P10 Blood Bath tile-row holdout and selected scanout proof | `f706a49a4dab2d4d429c10ab392cb93c29d3e2eb5b63dab0e0604fa852f03ec0` |
| P10 tile-row late mixed-lobby saves, both maps | `89348bccc7c827a9adc51218f835b076ed49a8e5125c3f3b4b4efc8455ba15a4` |
| Authored Mode 0 fog raster differential fixture | `1f6cf26092f226fea8e3b360140e298876f9f9dfda25d5f1a1a89601494e20bd` |
| P10 paired fixed and panned tile-row effect bench | `7e8d95ce4175b2befa682238ef2c02551265cecf25489522540a43c4011cf843` |
| Current-source asset-free Mode 4 ARM differential | `66b15e3234ecf39ebe02a087b5ba8d8d7c7fda447fa602f468c2598131fd2d34` |
| Late three-race tracer depth readback | `90963fb5bb184ef515bf078de40366c193d459e6fdf1437dff11754a4d67f9eb` |
| Monotone point-route search outcomes | `f301862e71f5c4b52fba25cf93bc89f95b4ef769cd5952fa43553fc1db6fc43e` |
| Terran Blood Bath exact endpoint repeats | `36e893debf67f5f26106a8fdbbc97af360742faabe3ff995e65e99af19773067` |
| Terran Blood Bath calls that continue an earlier route | `1544bbbd940e70a3d4d964ffc0d6a28a715ed8c2e70da36f1f76d20da643676c` |
| Terran Blood Bath ordered point-query capture | `fd2ed95b78f2d9261c99660ad5106745502c16c8e7c4deaab6337e2a8694d948` |
| Paired ARM incremental probe replay | `027bbcec6709093824b200e5986d4244d8ebba2f07c958f8e821327462bf624c` |
| Paired ARM continuation-table wrapper replay | `6f4b6225d1262d95014aef47b96472a976881e2479140e310733a6a9e50871cb` |
| N64 D-pad Left witness manifests, Melee and Free for All | `34f44b7849a7590b6c31263c61c29f7fcc5f06f683caca0886ddfad3a8ac1831` / `b5ea9ab761801e97eab89383e0316bbdb78f54ad801dc6a57518f873e98ccc77` |
| GBA D-pad Left and launch matrix | `228c88f2fdc12e9bce1711e1247a9f9b61d5c4e864ea782de01aaa5c420d23cd` |
| Campaign starts | `6f3b47661f367e051f938c8f7ad23b190b45389bacccfd9d2adde760ebea796f` |
| Terran chapter 1 full run and fresh loads | `8c3642922e2358bcfe2d0264918d1cf381bf254e95c7b993a55ab55f8046cc94` |
| Terran chapter 2 full run and fresh loads | `e51366eeaa6c55309aa3630a5c9e2da47ad51608be0b25f85f33d60fd20902b2` |
| Terran chapter 3 full run and fresh loads | `48487a363f9112b3d443e625ccba187254320a29f9e2ae028e4438fa4bf1fdd9` |
| Terran chapter 4 full run and fresh loads | `16434fb2891a5426c396f615312c0ab4ff992f6bb7206ada371cba276c9f922c` |
| Terran chapter 5 screen transitions | `91317b3116e15d7d9b92626a37fd52af05cc6009a30a697319c490c9b30e2e17` |
| Terran chapter 5 pause-corrected divergence | `19133241bc024bc6096a698611de030155e3c341679fb26105ea012e843835ab` |
| Terran chapter 5 pause-corrected defeat | `5045cef4db7af2cd8c214745ef1761e011972a1266df7490112d6e5fe6606aea` |
| Terran chapter 5 Kerrigan retreat and survival | `742686095642b17d6c7b92d4d80566fd3a61e245954e404b7238d06f3fcaa78b` |
| Terran chapter 5 retreat with the older suffix | `99ecca6a4f17266c65ccafbf47d41b93cd5e6830adaa7ce71aaf1325b2ed3cad` |
| Terran chapter 5 hero-safe local selection and order | `1848f80e7fe00f9d303bb853b661dd32add344459f56f878facaf4474a833698` |
| Terran chapter 5 explicit Move with stalled hero waypoint | `b0a318d3447b14132d09f800c2602ff5ba28b7682100c973dc89a8f2fee06fb0` |
| Terran chapter 5 earlier Raynor route | `0248569ec88a421a65c06afa37a0429bab5c98b470a05344c134c2b1f2290f9c` |
| Terran chapter 5 Raynor staging through frame 19,160 | `2f2f3d191678933c7fdbfb7161d7af993922228e403f3d59a3a515bd5f5bd437` |
| Terran chapter 5 both heroes healthy at frame 19,288 | `abd770a652624759162b89b7f73a5f5b07fd1723991baa5a8f7e5dda366dfee5` |
| Terran chapter 5 Raynor entry-area approach | `c3dc75496ad1a132b8167009000f52b0a81e11f7306ba9c4921b2a66d24862bb` |
| Terran chapter 5 seven-Marine early checkpoint | `11bb068c5d26f5dd67c45b311099e5bdf8a27b2601c68ddb6d03bfaab4c8786c` |
| Terran chapter 5 short escort waypoints, frame 13,603 | `ecdfc1cbd05edfd866f9206329846f730980b5556da8cc62136c32b6ffe078e4` |
| Terran chapter 5 escort fresh HOME reload and SRAM readback | `c4c78410820830d55d0de4e9d35dbeef53a41246f471612e8cbcd8fd8cad2336` |
| Terran chapter 5 northern escort waypoint and fresh HOME reload | `c5899c4509f6ae8620bda41c2f3f33ad51b5130813d34c230cda11a69a26c3c2` |
| Terran chapter 5 five-order escort continuation and fresh HOME | `9a2a67c8895b07b99c6a8f3836675564fcf35ca1c050d7a82de246dee858fc6a` / `1bb601fef43f7b7cc99b1a4a5f7560f6b5ab03ca1cb3d7a441c636439adf2693` |
| Terran chapter 5 south corridor continuation and fresh HOME | `dc814b7a03aa182c456fd1a53fda79271db261317b0a224c6164e0a20bc76a6a` / `5acc65e12322e807a3db8ef4742c2dbabb8615b52592db4ffa3761a8e7544d1e` |
| Terran chapter 5 Vulture engagement and independent fresh HOME check | `cf09d3654cd0dc8991838856dc9370fa61ac8cfdebbabde9b14066de32663bfc` / `4c89607b5dca55c9d371970eb6ea03664784b9df0c791bdc460524e8943643c3` |
| Terran chapter 5 south approach at frame 27,001 and fresh HOME | `eb7aef2e8afd2f9b36b4d0c741f0cf6003cce7ff19d13f833655e45d07765c69` / `b4e25c4f7ae7cd7993f1f635bc68e4ac594a836eecb08a42618ff8c6345c2998` |
| Terran chapter 5 first bunker removal and fresh HOME | `8fa035f4d3b9eebe6c0c19099bc04a05f3caebab347dcfe97115fcbe37af255b` / `7a7fc9af1e06d8cb12f87d56ba552846a48b716be59b1cc702a4f593d76c2b61` |
| Terran chapter 5 second bunker removal and fresh HOME | `f93a0deae5f9008a92097b6a32ac1b8581979df58b4ca56a2c120d7a8ee4ac1a` / `e254169bc8482e3aadce0e6f1a1022bcc7ae4cc0fe300979f50b526f4535fe21` |
| Terran chapter 5 recruitment and fresh HOME actor and building checks | `6a23eddb9a52ff2c035670638a2aa669e4bffc16b6941c3e177a7abf3593ecd8` / `a3d5c5ff57357af597720ea42ffad5a80660f5460a3f91d5bb5efb537b7c88d6` |
| Terran chapter 5 independent four-stage recruitment and fresh HOME | `31968bfc272f8faa187e28eddf086873bfd249ff5cdc22744bf8d362cb296b72` |
| Terran chapter 5 paid Vulture and fresh HOME | `1da0fa927fc9f7f556b7a93ab54de12f3a8d5db7e51555e9cc585a37b8c44cbb` / `a2d875fc61f9563eab413c6988deeb93e888dfa3d6743f1c09784e1f1f9a08f8` |
| Terran chapter 5 paid Wraith and fresh HOME | `98aa3f1d508177d0f03482756e63c36da1900df782dd1c5cc06253d7fd6aee4b` / `48a6f3878187a272f89e640c0471ca6ae554aa235b94ec1f6f42516d88ee4b30` |
| Terran chapter 5 mineral and gas SCVs and fresh HOME | `8cc5bfab0ad27d665cd4fc8d562f15ec3e3c5e7fa16ad8d86c67c1fd26476af4` / `3b3ad3bf54e55dac8388a5b550f7c79370e6bd7ed2281001f2dcc2035494079a` |
| Terran chapter 5 eastern Command Center destruction and fresh HOME | `c7cda989d7dd11ed4b2ad05c153b59d9b10c451b6ae2963ebfa881df828d1424` / `24458b5b15b48aaf6e74eabfe2557f78822655875427d0e822d7e862e08fc2ac` |
| Terran chapter 5 independent Command Center endpoint and hostile census | `b7f1769ff2f6e2fd9575e801724bcd1324937f5b8f234a2669ccb87a09503844` |
| Terran chapter 5 eastern Refinery and independent fresh HOME | `567159f9cd73903650f2ac8287455c3ad73ba7584d9e8be928ec9b4f5e037dcb` / `f37cfcdb13e075a453419d5f8f46dab401c8522b68fc607270de22600e90fe40` |
| Terran chapter 6 full run and fresh loads | `ff878c6135aee07caf12a4e3f26cd9f9e9b8a8943d4c0a2e90f7471d00ae91ee` |
| Terran chapter 7 full run and fresh loads | `79c457980d4e9e3346e4ccba496c44dbc4a1e09433588f2d3c00ca11fc149736` |
| Terran chapter 8 full run and fresh loads | `053b93180ce502b3ec281bf308ad34357a50a16321ce8f747a62d3faa811fac1` |
| Terran chapter 9 full run and fresh loads | `6c41688de2a262328a542e7fc5809d1833c399de539deb5ab87b0f96c835aa14` |
| Terran chapter 10 full run and fresh loads | `aa7affb64e6f9bb455758e4f7b20d9e8bb26b8aaefd6da625bb28834657d21f4` |
| Zerg chapter 1 previous-version pause and victory journal loads | `855aac8e466c4b7fa0266a0150dff9ba86420ae3dd746836cc6246881587a3c5` |
| Zerg chapter 1 worker production and first gas from earlier pause | `ab38f823f63fefd21938bb7cc2fdf1df7b1d0a1efe016ecac6dad235c7243c81` |
| Zerg chapter 1 independent fixed-input continuation | `4f5eee2586d2a301cd8c25ea701f24346e2b99eafd85a72680d116b1475a774f` |
| Zerg chapter 1 paid Hydralisk Den from the earlier pause | `58d5cd1704e6badfa09cde17168fc1abd9b098bbd8bf191c4ab27900d1724cce` |
| Zerg chapter 1 completed Den, new pause journal and fresh load | `7955f55008dff0fb8df79d35c6d26ad91c9205155ffbd1474c02fff25f54f5c3` |
| Zerg chapter 1 Overlord hatching and Den loss | `b61993ff8441be3c34eaa209b4343522941ca4c30ff3a27745148ab4eb3ce7e3` |
| Zerg chapter 1 earlier Overlord and first paid Hydralisk | `61157044ccd65d5b3c373cd0a3d6358597ed0694f254f731396053776e91373a` |
| Zerg chapter 1 defended opening, independent replay and fresh load | `c07133279a15e890fc3f28192096dc7ccc2b85fbf5853ea319d8ecf15671bb6d` |
| Zerg chapter 1 replacement workers, save and fresh load | `3fd25cfa02801e85481074f0d8a37b93601e3b8db07fd4afe5123f800f621a41` |
| Zerg chapter 1 alternate Extractor rebuild and long continuation | `bfc4527f96611a285f3e52558b3d085e08fa89b9b9383df12296cc2227a51173` / `1f427cb257d3dec0488a07bfcbba7432fe9c3112e372671c63a3637d202cd4b3` |
| Zerg chapter 1 three early-defense saves, gas delivery and fresh load | `78b17df4800f550c6a3346ebb072365975aa9cb2f1e4ac00956204b5cfdb3004` |
| Zerg chapter 1 sustained gas delivery, sixth Hydralisk and fresh load | `8788f3c9c8d7b24cc1f2a0e0463409dde1bddc18fa0585829f4a0d7e15968b02` |
| Zerg chapter 1 three assault checkpoints and fresh load | `9ae9c0dbcd4d146678eac97d235f6e52632afb3ab9f47c16587f4de3565b00cc` |
| Zerg chapter 1 five raid and reinforcement checkpoints, with fresh load | `0aa1c8b34e4f9d8a18b0f6ec89be486c1c31c0b8ac9ce8a913afdd318487e4e6` |
| Zerg chapter 1 assault, Pool recovery and three fresh loads | `fe4c144210505c8a6921fd9f6701d4cf9c8fb67b778ffc0e5681903e3e0331ec` |
| Zerg chapter 1 southwest evacuation and final assault, with two fresh loads | `c8e7af793d05a7252a2e5cfa8abce99e8ccc6b48006b32b4dda44e1f57abd835` |
| Zerg chapter 1 composed ordinary input from frame 984 to victory, with fresh load | `1972dd567660b95aedef54d66cc1425f1966cfc20f218861101f955bb85147ed` |
| Zerg chapter 1 victory load and Zerg chapter 2 briefing | `e9b826443cda4a4dd44290896cf841577909b3eeba297e81fc5b9081d0266e4e` |
| Zerg chapter 3 blank HOME deployment, paid economy and fresh pause continuation | `f00dfa77e6a95579fee276f2466d9b1f51215bd51b8dbf887f4f105e01201b7b` |
| Zerg chapter 3 paid army, first turret destroyed and fresh reload | `465c036febfef23eebbd9028718ddde9fbcc2a5260290a38b4946ff3f6f39bf2` |
| Zerg chapter 3 larger staging army and paired assault journals | `add479597eb38b1346b659a915453c0ff18e6780dd44698be74bf382d438594e` |
| Zerg chapter 3 paid Lair, Spire, Mutalisk and fresh load | `19b35e7e434fb4a1e2ffe1f23ce38e49ab41459e21fc3915f907f83d17f7010b` |
| Zerg chapter 3 two gas workers, replacement Drone, second paid Mutalisk and fresh load | `c03824f923eeccb8a403b2b300575e049ac4084477f558acc9bc9d23351667fa` |
| Zerg chapter 3 concentrated force, two turrets destroyed and fresh load | `7ab53d373dce59910590b59f49578a5321981a5606444fcd64fc5b2a7781ca1b` |
| Zerg chapter 3 air-only staging and two front turrets | `da15345f50c14de6ba9bc3e0596e3cf5d5e549d979164daa4e5009a52151e3e0` |
| Zerg chapter 3 exposed tank and third building, with fresh load | `4edc0c308e971a5c01f44756e7de9224fb7e93bb394ac68454b0f4a520cd1ee2` |
| Zerg chapter 3 northwest advance from 23 to nine hostile buildings | `56574ada7f77938fd7933883c334b44e444fd8ca2b288f8e8f560f6515689c79` |
| Zerg chapter 3 base defense and five-building fresh checkpoint | `a5df21d6dddfbcdc5ea7ebdeba1339a07c89fd8cc2339bda6bde96681f87b6ab` |
| Zerg chapter 3 rejected pause save with 12 pending impacts | `dda019fe449f93ef34b3c922cd683d6a6305b536a574d06ad428d53e4cf35f51` |
| Zerg chapter 3 coordinated ground and air checkpoint | `ba1377b8ac2fdb5bce7f7513206820f03a1848c1fa0bf38eb4cfe1c3a522765a` |
| Zerg chapter 3 victory, exact autosave reload and Zerg chapter 4 deployment | `4fdf64e727696beabe40bf58d4c61fb2df92e78d75a9400a3937a27e14a087c6` |
| Zerg chapter 4 fresh air-production save | `d1fe57cc5a21e1d44ff004d583b5ac4e71eaf6ebcafc72345542a31c766d4e0e` |
| Zerg chapter 4 independent ten-stage victory replay | `5870e567b34e5d044deb4313cbb84229347a6a987c8f1975a2ed719ccb5a29a8` |
| Zerg chapter 4 victory readback and Zerg chapter 5 deployment | `43529b8ca8065c56258378257bd92bca46f119e2728293a8b0e829406cff7232` |
| Zerg chapter 5 independent fourteen-stage victory replay | `09d7b8bedd7217ff2b5d921f43877cfd55be3891206983f0db2efa68a7c5890c` |
| Zerg chapter 5 victory readback and Zerg chapter 6 deployment | `f3411d20910da9f646bf692ad390db1ef8908a6fb65b4bbadb0475a0b30cfdcd` |
| Zerg chapter 6 paid Lair and fresh readback | `11cd5eff2a143bb1919d47f31791250182e73b14e5c3a6a03ac2350470686c1b` |
| Zerg chapter 6 paid Extractor, first gas and fresh readback | `4a60c576ac69a5bcbee6fe2782063b74963e006a82d0ca792087987533c6ef86` |
| Zerg chapter 6 independently replayed region 5 buildup, Storm and evacuation | `0552fb2ccd653f7af25f6f7f530c6ea18e7629d2b88136364a0974c92a02a931` |
| Zerg chapter 6 second ferry, further Storms and safe return, six-stage independent replay | `14923c08c5e52518cbfa7c33cf2e49eef3f2953bdd1424b9f74d05cf93cba298` |
| Zerg chapter 6 final sorties and structure clearance, ten-stage replay | `e49ba9d8bff12a103c3a3e53d607c1e400b4a80673d0ef4517081159838769d6` |
| Zerg chapter 6 final defenders, victory and Zerg 7, independent replay | `edfd65ace98a422cec802aea684fee66ee1c82372251c031c46c9f924aa572f5` |
| Zerg chapter 7 independent opening, Hatchery defense and economy replay | `a70f9adddee6d48ba68ef13285deb01d10dda0103287b35d92f68c749b634613` |
| Zerg chapter 7 three-stage independent Pool, Lair and Den replay | `570877a52262e4ac222e19f62e44121c33dd5e0496c3b9e11963bcc366038e81` |
| Zerg chapter 7 fresh completed Pool, Lair and Den census | `f19e2d5b56cedf317753eb739ed7c21c94b3ae913a8d30aef59f8d55a6a1d636` |
| Zerg chapter 7 three-stage independent Sunken Colony and air defense replay | `ceb9061a7d9056893ff96b96bee8435c197390034e7ae3386ee0feed1e2d85b0` |
| Zerg chapter 7 early Spore Colony raid comparison | `4932c66f978c8496e5663ecb4792a4f4f4a1ea770bd41cd2532aabccee0036ab` |
| Zerg chapter 6 fresh victory load and Zerg 7 deployment | `13d4cb35f525874774cb9bb5ae808f134483d31e0577432d6e0b1ca3731f01a3` |
| Zerg chapter 6 seven-stage third-sortie replay, safe evacuation and fresh load | `a24ec13fcb7a82d06b2d11236a51f0194c9772986aa145386499198b4b350b06` |
| Zerg chapter 6 frame-147,057 fresh force and hero census | `032ffeb493f4c87d1a072c4851f935c530711bc322f772a5c0ccbc74433cf5cf` |
| Zerg chapter 6 frame-130,610 fresh terminal census | `32f8f2944e2b88118473bb5b21c4ee48df4ebb37a5872bd718a011b9c837b156` |
| Zerg chapter 6 fresh terminal region 5 and army census | `5229b39b62dbe14eff9dd34b8a007659f05983bf58af9831831859439d9017e3` |
| Zerg chapter 6 independent four-stage opening replay | `94edc6c57c802767d1dbe0ba180efc349829947e60d9bbb606decb683051b7d4` |
| Zerg chapter 6 paid Ventral Sacs and fresh readback | `04ffdd063ccf3d5b3176de0bb24ae2f3c8839d08c988a32c06132182a5e24a0d` |
| Zerg chapter 6 ordinary boarding and fresh readback | `0f995723b88ab792ed1b3a50929ed2c19574d016026bfffb47689aca07ed3a6c` |
| Zerg chapter 6 independent research and boarding replays | `af7a0420ab248b560d4d342b805d3432f0b9c818b837387354530765fa8e5d9d` / `31ccd1d2128474c5f77c90d59826df89c49647bb7a197f0896d29a54e5fcb3c6` |
| Zerg chapter 6 ferry unload and fresh readback | `d3f796f9254cc971986f2b286557d9c36655a77a61406ccb17fa0e095169c0aa` |
| Zerg chapter 6 independent five-stage ferry and assault replay | `3fb1c91697ff0f5226a18d5ff5cad8c425e31c8f1bf18b69c5c6cdfe5930ed15` |
| Zerg chapter 6 fresh region 4 and 5 census | `45d96017ef85d4803d30b3aa107f0ed4b5d884f7c0229dbd43bae00abf659ca6` |
| Zerg chapter 6 reinforced attack and fresh one-cannon census | `d35d5dcba88744bb1a867d765e63a3418187bf32d370483268ef08345c1b4c75` |
| Zerg chapter 6 independent three-stage reinforcement replay | `1a24cf08c260312e0b6b32ae3005ea3ce1cc0bbbe291dac3e901442ad463b9e4` |
| Zerg chapter 6 complete region 4 clearance and fresh readback | `20fb9761e797f750a54099086ea1f8b8be9fa4438bd10fcbf3418b292023dba0` |
| Zerg chapter 6 independent region 4 clearance replay | `e14fa4d1f5b0352e5601cd7fe7b3f28474192a6bfc17c59ffc99c12f651d502e` |
| N64 Zerg chapter 1 numeric victory record | `188c9ad4bd8221c31b971c724b0211b1592d96353db2b242928a58db567b0e17` |
| N64 Wasteland fresh ordinary-input continuation manifest | `ab23626bc8b0db67555455514119b22fd0cfa6c9707d8c2038bb5a4ebb3be80c` |
| N64 Wasteland ordinary group selection and movement manifest | `4b85737a0b876efbc6ab541b8c64693f1eb3e4e0fa8007a363b2848e75218066` |
| N64 Wasteland ordinary save with persistent media enabled | `c864ce55ae39739209c890552f956d783d8a34e9bbe156d20ab457d133aec9b5` |
| N64 Wasteland ordinary fresh title load manifest | `16a8056ac4ac05cf2bd7e7e08ee89d75a8d86f6e0672322c5de251a89b41bf2b` |
| N64 Wasteland second-slot save and fresh load manifests | `911d224bd8d631db1f0c00dc91dc3038f642e293c1883ba628b091c9ae875953` / `073a5f77a61d7703a8668e4ec9304d75fd926775fe6d2ec8a6497ac577b60cbc` |
| N64 Wasteland eastern-shore movement manifest | `42afda84d1d1eb620e709d738be1e1f9cbc3f641077041b0b1e03a5ea9783a2a` |
| N64 Wasteland Raynor survival objective manifest | `ab665a6572572da554ecc7da81a67ad8a02f21306149d2b22009a5b54190cdbd` |
| N64 Wasteland post-encounter save and fresh title-load manifests | `e3ace99f57f04f37b5c743e92b8c38f516e4d6af4b4078890b16de5817e71d43` / `0bcebe5d9b9519e578797c3b2a33bcedcd6522c3d9b1243411b46ae9a0ee249a` |
| N64 Wasteland base save and fresh title-load manifests | `f62e7af3a55ffce8f08db891617ccdc65c5fb62a5d90aac5d12d65b7dfd84b96` / `a2dd3465ae47ab3f73eca7f875f598eec2e96ad306f7cd572e179a8c38a9e2dc` |
| N64 Wasteland ordinary SCV selection, movement, save and reload | `f5b1a943c39b5a5a4b64684ae0216ac533d4d999ad52307ecc9e89a62f575198` / `2d31726e8ffada29493e690cf45dbe355b4559f3454853c3103e33a291a10000` / `a55eac283971c6c0421fde5364bcd4df02a11de86e222cd2aabf61629fb7cc8d` / `53d4ac078bb01e005b5739e53a95797896b830b5ff48ede37c36c0d98bb344ec` |
| N64 Wasteland SCV reselection and southward move | `d634dc26e7a39a6cb21b1939c929db6de2612e9d0c4a79ea251a08f032fe899e` |
| N64 Wasteland B-enabled scratch bridge source | `2bcb6ce41b28950fdbc5abb891e263a8a28ff74c255ea3093905d189ea2ff0b6` |
| N64 Wasteland B-enabled scratch bridge binary | `52ca094d2fbc0d723305546b0e6cef7f0b0a19a7a436de09dfab30950b05a3da` |
| N64 Wasteland B Harvest deliveries before the observer failure | `cd7844c9cb1cf4acc307d3c40a158d7c7011a084d07374b291375535f00137e2` |
| N64 Wasteland first mineral delivery and ordinary FlashRAM save | `6625cd459e1e46a7cf95d6259e6e462db23cdc90a752d0224952608d354cb577` |
| N64 Wasteland 120-mineral save checkpoint | `85b3c46e143e6e74ec9a5930278fff04918b52e8fc5412d94ec7d0128f529563` |
| N64 Wasteland bank checkpoint reload and 152 minerals | `d9c4111ea52ca8b9ad0669c2ac8e7092307cd8f42e613d5f89bb6c45ed7f75fb` |
| N64 Wasteland first slot fresh reload, continued mining and FlashRAM readback | `7106dfa869740ea0d859dfb9882b815e61655b470bcec19e93ee19e6bac61299` |
| Exact KODAK Wasteland fresh start, matching pause journal and victory | `46368011e850ec911ce585edb1c8a64747a7fe14a2a08ba9d6a676e0adbd7b1f` |
| Exact KODAK Wasteland two SCV deposits and independent fresh HOME continuation | `568c075b6d633c88b85975fa2979ce3910c0dc163efd22c88dfddd02353616e0` |
| KODAK Wasteland contextual Mine, eight-mineral cargo and return-path loss | `ac949cbde3012f37ff65a20296c9cfc0b4ba5fc8327856bb73c96df18c7dab21` |
| N64 Wasteland Barracks placement, payment and construction | `45ba6eb34972427f2dabf51588937f2c81ef48319ef84bd131c72eeb96453b1d` |
| N64 Wasteland first-slot save during paid Barracks construction | `521410b9a16e3f610efdc9ac41e2db268dfc15fea88f04f18be80d62b3365ec8` |
| N64 Wasteland fresh Barracks load and construction completion | `b7eaee824b6d675614eec4f3ab08c5f74f35688b348a7a6b1854793d14dc99ed` |
| N64 Wasteland advisor activates five-of-ten Marine counter | `8f320c0688a82e90b457a56f4e991e43b3d85d919f5fea46abb27841c8367f76` |
| N64 Wasteland reselects SCV after the Barracks completes | `0c3b4daaf35c04fad8835cc94115ae5b1d8665fe2ec92cbda115ed7b565bce99` |
| N64 Wasteland mining resumes after the Barracks completes | `f0e9024b73022e629dd8e05dfd80f00c2276bb6ff69daa869b8c00927d51917f` |
| N64 Wasteland first paid Marine and six-of-ten count | `c31ad1788d22f742d33233fcca8de0fa39e64843b3d6b0057d8d65deb9757c72` |
| N64 Wasteland fresh six-Marine load and second paid Marine | `203a609a90089d60376da24517dd40cadfdb5679efb443ff0fc3af511c8be8c4` |
| N64 Wasteland fresh seven-Marine load and third paid Marine | `f2785703c13676c42af8dbf3afad3cce84de4256b30085723b283617c039f309` |
| N64 Wasteland eight-Marine save | `cc8ae1cbf59f686bc70bb14ad46aae9ff4d6554948130e4c0a10489118c73878` |
| N64 Wasteland ninth Marine and new first-slot save | `e2be9ce7aa63ec7af1769d41c663ace9737e0271dff7f781a30e9b6f1460ca3a` |
| N64 Wasteland tenth Marine and victory | `29d8eb7046ba393b37b369885e813f5aafa399dab35f9829942062853b5fd7cb` |
| N64 Wasteland result acceptance and Backwater Station briefing | `e4f9495cd5285b8f5ab33246e3145397e7addfc51ee08ff6d429eec8478d528e` |
| N64 Backwater Station unlock retained after fresh load | `e175a0e7c2b4ba90d783ae2c19a5657bac34f86ba82618f2211fa565486249fb` |
| N64 Backwater Station selection and briefing after fresh load | `6c65e723b9bc00dea2e1d010ab25f5fddfcff4616ce7a838e74636e872a926e2` |
| N64 Backwater Station deployment after fresh load | `7b2e77e2db1cacceab60cc18214a252c366c3934b70391776b6296458f0721d5` |
| Exact KODAK Wasteland Barracks payment, completion and Marine queue | `a4ea776e6036165fa886abc96e0be45dff38b78a70dba3a0e845010f31af90b4` |
| N64 Wasteland SCV over the mineral patch with B Harvest prompt | `3b16c2cf6a181705d6adb76e365ef6cee5553f1708483f2bedc72fdbfe44d8a8` |
| Zerg chapter 2 full run and victory readback | `a1234c2bb542e1527b263bc8b7c4bc65b6e3d1ea6a8e3c6163108212507bc208` |
| Zerg chapter 2 fresh pause continuation | `df6b1fa2d25f8c0151f4a3aa84a0c9340a011e6c8ca155157183479f88bfc8dd` |
| Protoss chapter 1 full run and victory journal | `6176b76aa5daae99ca5f32871a90f5e756ac671a8d77173bee0964d467d17547` |
| Protoss chapter 2 full run and victory journal | `aefbd075604bdf941464e02443ee7625e799c165c41084bf5c94ed3b3c7cffd7` |
| Protoss chapter 3 full run and fresh loads | `dcca91454a924eafebc47440a7a1fda1a0dd25b21d9b23024ee6e05690e6e221` |
| Protoss chapter 4 full run and both hero positions | `369935061512182039552c30dfa01c4ac18f4aaa746729ef0a245fed1c4c5db6` |
| Protoss chapter 4 fresh field and victory loads | `ccb6f8e31782bbcbdf067e31f6c3d68dfac9a31f0e79cdeddb4f481e038d4fad` |
| Protoss chapter 5 full run and fresh loads | `6135c1f184640ecf9f4cf77a9c48da534bc19f60a0e7c33685be970a50e051a2` |
| Protoss chapter 6 full run and fresh loads | `fe85625beb73116126eecb82bc244a3d6328fee40ce881f3ee67e89191ef2974` |
| Protoss chapter 7 full run and fresh loads | `54d8fd3811c16883ad7bbd0c0991240a8e80737af43d282e8c2b62bcc1a548d0` |
| Protoss chapter 8 full run and fresh pause load | `5e74681fbd24e824a60d4ea3a8d9cc02ac854177c69afd7ada032bab79c0c458` |
| Protoss chapter 8 victory journal continuation and readback | `c0a3a95e628bb675fd64f64a0682b658af78fe8bf2305a812cb9928e426ae1ce` |
| Protoss chapter 9 full comparison | `93151835a97657694c868c183a81ec82aac9f41c9ff1ee49af472e299761f48c` |
| Protoss chapter 10 full run and fresh loads | `a018c1ee1cc0fd42631ae5a91101db087adf0466f680ea532e0115ce556d0605` |
| Showcase scene starts | `5ceefdcea04deea53c7d5795810dbef47e3873b33415449528b668e5d8e34029` |
| Fresh-process Custom Game saves | `9e8ef3557675969874a6b8697c5b7b5d7df03481c0d107fb295ce938916adb10` |
| Three-CPU timing samples | `9f7147388661e7ec06e5c9bf0f84c263082e386514ee3362bd066c1cb45cebe5` |
| Protoss 10 Blood Bath instruction attribution | `e942247d1615ef449b116e9fcaf85950d335d59ed063701d7dcd20b1153e9799` |
| Protoss 10 sprite and contact workload census | `67f20dae00085613182e2471d1d594100e038637edf1a29e61b035c2c47e8b6f` |
| N64 Challenger reset replay | `5c84d1b8ae1e5f57a5731ceb9a6417d636788f58a2d39c67a63cca34bc6c1fdc` |
| N64 Blood Bath reset replay | `66a9c657369c58decea5a0f27079ed785fadc60f6c43357590541b70c70eb426` |
| N64 Wasteland opening and R reset replay | `9b2616e869232132aeb149afb9e953b4321618c8c7a440ced8576e97e52e8fd8` |
| KODAK Wasteland mineral grant and R selection | `c6941222e9f2ebb4e0672b0ed381d48e56fe3e60c7b67b92501b04ca413bf4f4` |

Wider campaign victory and mid-mission SRAM coverage and hardware
timings remain open on this package.
