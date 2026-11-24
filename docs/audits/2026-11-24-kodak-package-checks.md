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

An ordinary move order after input call 22,000 takes Raynor to the
northeast edge. He reaches it by game frame 24,775 and stays at
full health through frame 129,658 in the neutral continuation.
The mission is still active with all 21 hostile buildings and
four friendly buildings remaining.

Applying the old suffix after that retreat recalls Raynor through
the global combat-group selection and later orders. That variant
loses at frame 108,946 with all 21 hostile buildings still standing.

A fresh retreat test uses an ordinary L-drag box to select two
nearby Marines. A ground order moves both with formation
offsets. Kerrigan retains her idle target, and Raynor retains
his retreat target and idles on arrival. This verifies local
command of an assault group with both heroes excluded.

The explicit Move test selects Kerrigan and uses B, Up and A to
choose Move from the command menu. Placing the cursor at (1379,203)
sets her movement order and destination. She remains around
(1319,203–205) at 45 HP for 2,000 updates. Raynor reaches (1528,7)
at 250 HP. This establishes command acceptance and isolates the
remaining route problem to reaching that waypoint. A previous
contextual A click on nearby ground selected another actor.

The retained route puts Kerrigan at the stalled location by input
call 20,000. A new branch at call 18,000 verifies selection of her
slot and issues Move toward (1448,398). She moves from (1352,290)
to (1456,383) by 200 updates and stays there through update 500.
She keeps 45 HP and Raynor keeps 250 HP. The test establishes an
earlier route that clears the original position. This waypoint is
short of its target, and the mission outcome is still open.

An earlier branch at call 12,000 redirects Kerrigan from (945,397)
to a staging area near (856,398). She reaches it within 100 updates
of the Move order and keeps 294 HP through game frame 19,160. Raynor
follows his prior order and falls to 23 HP. A later attempt selects
his verified slot at frame 13,229 while he still has 250 HP. That
attempt ends before issuing an order because the menu sequence does
not enter Move mode.

A second early branch selects Raynor at frame 13,229 and uses an
ordinary A click on empty ground at (702,538). This replaces his
old attack order with Move. At frame 19,288 Kerrigan remains at
(856,398) with 294 HP and Raynor is at (702,527) with 250 HP. They
are the two remaining player units. The mission is still active
and this branch has yet to produce a victory.

The direct recruitment attempt moves the rescued Kerrigan alone
toward the rebel base. At frame 12,814 her position is (1348,257),
inside the base-entry rectangle, and she has 118 of her earlier
294 HP. She dies by frame 13,306 and the mission is lost before
any observed recruitment. The existing fixture allows up to 2,400
updates for its delayed sequence. This attempt did not establish
a safe approach or dwell window for it.

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

The profiling intervals charge interrupt work to the phase in
which it occurs. The simulation interval also includes input reads.
The residual includes audio update and presentation outside those
two intervals. It typically includes a pacing wait in the launch
window and averages under 500 cycles in the late windows. Use the
instruction attribution below when comparing individual routines.
These are cycle measurements in mGBA.

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
| Campaign starts | `6f3b47661f367e051f938c8f7ad23b190b45389bacccfd9d2adde760ebea796f` |
| Terran chapter 1 full run and fresh loads | `8c3642922e2358bcfe2d0264918d1cf381bf254e95c7b993a55ab55f8046cc94` |
| Terran chapter 2 full run and fresh loads | `e51366eeaa6c55309aa3630a5c9e2da47ad51608be0b25f85f33d60fd20902b2` |
| Terran chapter 3 full run and fresh loads | `48487a363f9112b3d443e625ccba187254320a29f9e2ae028e4438fa4bf1fdd9` |
| Terran chapter 4 full run and fresh loads | `16434fb2891a5426c396f615312c0ab4ff992f6bb7206ada371cba276c9f922c` |
| Terran chapter 5 screen transitions | `91317b3116e15d7d9b92626a37fd52af05cc6009a30a697319c490c9b30e2e17` |
| Terran chapter 5 pause-corrected divergence | `19133241bc024bc6096a698611de030155e3c341679fb26105ea012e843835ab` |
| Terran chapter 5 pause-corrected defeat | `5045cef4db7af2cd8c214745ef1761e011972a1266df7490112d6e5fe6606aea` |
| Terran chapter 5 Raynor retreat and survival | `742686095642b17d6c7b92d4d80566fd3a61e245954e404b7238d06f3fcaa78b` |
| Terran chapter 5 retreat with the older suffix | `99ecca6a4f17266c65ccafbf47d41b93cd5e6830adaa7ce71aaf1325b2ed3cad` |
| Terran chapter 5 hero-safe local selection and order | `1848f80e7fe00f9d303bb853b661dd32add344459f56f878facaf4474a833698` |
| Terran chapter 5 explicit Move with stalled hero waypoint | `b0a318d3447b14132d09f800c2602ff5ba28b7682100c973dc89a8f2fee06fb0` |
| Terran chapter 5 earlier Kerrigan route | `0248569ec88a421a65c06afa37a0429bab5c98b470a05344c134c2b1f2290f9c` |
| Terran chapter 5 Kerrigan staging through frame 19,160 | `2f2f3d191678933c7fdbfb7161d7af993922228e403f3d59a3a515bd5f5bd437` |
| Terran chapter 5 both heroes healthy at frame 19,288 | `abd770a652624759162b89b7f73a5f5b07fd1723991baa5a8f7e5dda366dfee5` |
| Terran chapter 5 direct recruitment approach | `c3dc75496ad1a132b8167009000f52b0a81e11f7306ba9c4921b2a66d24862bb` |
| Terran chapter 6 full run and fresh loads | `ff878c6135aee07caf12a4e3f26cd9f9e9b8a8943d4c0a2e90f7471d00ae91ee` |
| Terran chapter 7 full run and fresh loads | `79c457980d4e9e3346e4ccba496c44dbc4a1e09433588f2d3c00ca11fc149736` |
| Terran chapter 8 full run and fresh loads | `053b93180ce502b3ec281bf308ad34357a50a16321ce8f747a62d3faa811fac1` |
| Terran chapter 9 full run and fresh loads | `6c41688de2a262328a542e7fc5809d1833c399de539deb5ab87b0f96c835aa14` |
| Terran chapter 10 full run and fresh loads | `aa7affb64e6f9bb455758e4f7b20d9e8bb26b8aaefd6da625bb28834657d21f4` |
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
