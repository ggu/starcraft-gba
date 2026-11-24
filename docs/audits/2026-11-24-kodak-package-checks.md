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
| 9 | 35,363 | 47,266 | 51 | 17.12 |
| 10 | 2,276 | 3,361 | 6 | 17.83 |

The timing includes briefing, pauses and presentation, and comes
from the GBA guest clock.

The retained [Protoss qualification](2026-11-13-p9-normal-home.md)
also covers full HOME wins on the exact packaged Protoss image.
The package manifest pins both receipts. P9 wins at frame 272,857
after 281,447 calls and P10 wins at frame 93,377 after 94,852
calls. Both restore the mid-mission save and the victory in fresh
HOME processes.

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

## Custom Game saves

Thirty-six early Custom Game save/continue pairs pass across all
four products. Each saves after 120 updates, loads from a fresh
HOME process and advances another 120 updates. The checks compare
the saved and restored lobby, game type, seat masks, resolved starts
and world state, then compare the resumed world with uninterrupted
play. There are 24 Free for All cases across both maps and 12 Blood
Bath Melee cases, spanning one, two and three CPU lineups.

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
occurs at most once within a sampled update. The late cut also draws 164 packed
sprites across 3,251 rows. These counts provide workload sizes
for synthetic renderer and collision tests.

## Local receipts

| Check | SHA-256 |
| --- | --- |
| Custom Game launches | `8c990bc25747578cdb2ad4f269046e46e7f6163565a9ea3af2edc16e276622c5` |
| Campaign starts | `6f3b47661f367e051f938c8f7ad23b190b45389bacccfd9d2adde760ebea796f` |
| Terran chapter 1 full run and fresh loads | `8c3642922e2358bcfe2d0264918d1cf381bf254e95c7b993a55ab55f8046cc94` |
| Terran chapter 2 full run and fresh loads | `e51366eeaa6c55309aa3630a5c9e2da47ad51608be0b25f85f33d60fd20902b2` |
| Terran chapter 3 full run and fresh loads | `48487a363f9112b3d443e625ccba187254320a29f9e2ae028e4438fa4bf1fdd9` |
| Terran chapter 4 full run and fresh loads | `16434fb2891a5426c396f615312c0ab4ff992f6bb7206ada371cba276c9f922c` |
| Terran chapter 5 screen transitions | `91317b3116e15d7d9b92626a37fd52af05cc6009a30a697319c490c9b30e2e17` |
| Terran chapter 5 pause-corrected divergence | `19133241bc024bc6096a698611de030155e3c341679fb26105ea012e843835ab` |
| Terran chapter 5 pause-corrected defeat | `5045cef4db7af2cd8c214745ef1761e011972a1266df7490112d6e5fe6606aea` |
| Terran chapter 6 full run and fresh loads | `ff878c6135aee07caf12a4e3f26cd9f9e9b8a8943d4c0a2e90f7471d00ae91ee` |
| Terran chapter 7 full run and fresh loads | `79c457980d4e9e3346e4ccba496c44dbc4a1e09433588f2d3c00ca11fc149736` |
| Terran chapter 9 full run and fresh loads | `6c41688de2a262328a542e7fc5809d1833c399de539deb5ab87b0f96c835aa14` |
| Terran chapter 10 full run and fresh loads | `aa7affb64e6f9bb455758e4f7b20d9e8bb26b8aaefd6da625bb28834657d21f4` |
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
