# World depth regression cases

The USA N64 renderer sorts visible sprites back to front by elevation. For
elevations 0 through 4 it sorts by ascending world Y within each elevation.
Higher elevations ignore Y. A turret flag and a sprite ID break remaining
ties. The target picker calls the same depth helper.

The layer values come from the 228-byte elevation column at offset 4068 in
BOLT section `00C/001`. The section SHA-256 is
`54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`.
The USA ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.
The retained Wasteland observer capture at step 7121 has RAM SHA-256
`d30d79bcd796c4c535c2667875da309a045bfe482bba9720ba01eb5f92951b9b`.
Its helper at `0x80122750` feeds the render queue at `0x80045f48`, which
drains back to front at `0x80120460`. The depth helper is also present at
ROM offset `0xE6440` (64 bytes, SHA-256
`f9a75923b37bbc55602345e4b1d20c0449c4ac922d8548afd203089c010cfaf1`).
In native gameplay footage the
Overlords cover the Hatchery rim, and a Probe covers the southeast Nexus lip.

The GBA Mode 4 painter uses those elevation and Y rules for units, buildings
and resources. Composite turrets remain with their hull. GBA actor kind and
slot resolve exact ties as a deterministic approximation of the native
sprite-ID tie. Native depth changes for special states still need individual
checks. The change covers world actors in the shared campaign, skirmish and
Showcase renderer. Target picking, free-standing effects, HUD and the
experimental tile/OAM renderer need separate coverage.

## Harness cases

For each case, render five frames with one fixed camera and the same
terrain, palette and animation tick. Draw the empty scene, each actor by
itself and the pair in both creation orders. Restrict the overlap mask to
pixels both isolated actors change to different palette indices. Require
a nonempty mask and compare every masked pixel in both paired frames against
the expected foreground actor. The Terran host frame coverage fixture
executes the first four cases. The expected counts are diagnostic for its
pinned sprites and are allowed to change when the private art is regenerated.

| ID | Setup | Foreground | Measured overlap |
| --- | --- | --- | --- |
| `air-over-ground` | Tank and Wraith at (96,50), layers 4 and 18 | Wraith in both unit slot orders | 192/192 |
| `ground-by-y` | Tank at (96,50) and Goliath at (96,56), both layer 4 | Goliath in both unit slot orders | 115/115 |
| `building-over-unit` | Tank at (96,50), Barracks at (96,58), both layer 4 | Barracks | 459/459 |
| `unit-over-building` | Barracks at (96,43), Tank at (96,50), both layer 4 | Tank | 380/380 |
| `air-tiers-ignore-y` | Move a Wraith north and south across a Carrier or Overlord, layers 18 and 16 | Wraith during overlap | Future replay |
| `camera-and-hud` | Scroll an overlapping pair past screen rows 12 and 119 while flipping framebuffer pages | Foreground stays stable, HUD rows stay clear of actor pixels | Future paired replay |
| `selected-overlap` | Select the Overlord above a Hatchery and the Tank behind a Barracks | Overlord ring over the Hatchery, Overlord body over its own ring, Barracks over the Tank ring | 15, 14 and 29 distinguishable pixels |

The old renderer flipped all 192 Wraith/Tank pixels on unit slot swap and
drew the Tank over all 459 pixels occupied by the southern Barracks. Use
`SC64_DISABLE_WORLD_DEPTH` to replay the former pass order from the same
source. The current canonical Terran framebuffer test is
`build/test-sc64-terran-full-frame`. Add campaign and skirmish replay cases
for picking the topmost actor and for dynamic layers such as workers against
resources or Interceptors around Carriers.

In native Challenger melee, a selected Overlord moves across the Hatchery.
The cursor is away from the overlap. At observer step 3920 the Overlord's
lower ring arc covers two pixels of the Hatchery lip that were opaque at
steps 3612 and 3740. The native capture SHA-256 is
`765ca89ac1f501f021ecde36762ca79af2f0e3953cdae00d54647900b84e5926`.
The GBA depth painter now draws a selected unit's ring immediately before
its body. Foreground actors can cover the ring. The legacy path still draws
the rings in a separate pass. The PC implementation also draws the ring
within its selected sprite's turn in the sorted pass.
[OpenBW sprite renderer](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/ui/ui.h)

The terrain cache now holds background and fog. Profile counters attribute
the sorted actor pass, including buildings, to `sc64_unit_draw_cycles`.
For before/after comparisons sum building and unit draw counters and compare
the combined cost with background and full-update timings.

## Local ARM performance

An ordinary-input Protoss 10 ROM replay starts P7 and runs 9,020 tape
steps. The comparison covers 958 advancing updates in the final 1,000
tape steps. Both builds had the same frame progression, camera and route
counters. Eleven sampled world states, fog masks, and actor arrays
matched. One sample had changed indexed frame pixels.

| Arm cycles per advancing update | Former renderer | Depth painter | Change |
| --- | ---: | ---: | ---: |
| Mean | 1,877,754 | 1,799,786 | -4.15% |
| 95th percentile | 2,176,419 | 2,098,029 | -3.60% |
| Peak | 2,558,598 | 2,464,640 | -3.67% |

The tape SHA-256 is
`44e68fec4bd45ef8e8df86144dcfe877dc99592f2adf99107351cfc1bb0ed3a6`.
The former-renderer receipt SHA-256 is
`b21cc587f2f6fec72aeba1f5eac38e977a2426bfc01c5f10db49c08a8ffd5849`.
The depth-painter receipt SHA-256 is
`0eec46ab8ea1510e12095aa8485ffc4d2dbdd941f4b26354ff4be7b5758de410`.
The result is scoped to the emulator replay. Hardware timing remains open.

## Three-AI skirmish follow-up

The Protoss 10 ROM launches Blood Bath through the Custom Game menu with
a Zerg player and Zerg, Terran and Protoss CPUs. The last 1,000 updates
end at frame 17,301. Twenty sampled world states, actor arrays, resource
rows, and fog masks match in all four builds.

| P10 renderer and steering combination | Mean cycles | p95 | Peak |
| --- | ---: | ---: | ---: |
| Release `f924b9df` | 1,001,597 | 1,407,402 | 2,193,638 |
| Early steering only | 999,806 | 1,403,992 | 2,195,675 |
| World depth only | 1,028,478 | 1,427,845 | 2,211,542 |
| Early steering and world depth | 1,026,648 | 1,426,164 | 2,209,088 |

World depth increases mean cycles by 2.68% on this skirmish. The
combined build takes 25,051 more cycles on average than the release.
The combined build still saves 6.28% mean in the P7 replay with the
same sampled world and one changed frame pixel hash. Keep the paired
skirmish run in the release gate. The main cost increase is the actor
pass, which now paints resources and buildings on every frame. A
trial that rejects hidden actors before sorting adds about 520 mean
cycles to this skirmish, so it stays out of the candidate.

The stock, depth-only and combined skirmish receipt SHA-256 values are
`7e18c69916088d3a9c07fe3abfef68606d9e95323a6090fe55105fc2c1431385`,
`af4bf37336699599bea1da3be7f05c69c6cc0ba7c056ad558951b9255565ab13`, and
`51ef3c3342ba4af6b2a30d89ded095f88b070f64da7aba8ac09fc11525feb76e`.
The combined P7 receipt SHA-256 is
`381c66228c11c7e30c296ee5a4956f5cc278e5a0c2c8975b122a895ea5a0cbe9`.

## Packed Hatchery draw

On P10, the three-AI camera repeatedly draws a visible completed Hatchery
after the terrain cache restore. The P10 build now packs just this frame
using the existing masked-word blitter. Other frame pointers retain the
byte-sprite path. The normal build selects the packed frame when its
existing sprite compositor flags are present. The asset is generated
locally from the prepared sprite bank.

The composed steering and depth builds use the same frozen inputs in
both arms. Twenty Blood Bath cuts and eleven P7 cuts match the complete
indexed framebuffer pages, palette and sampled gameplay. The P10
framebuffer and packed geometry sanitizer checks also pass. The linked
ARM builds use the same IWRAM and EWRAM. The frame adds about 23 KiB of
linked ROM payload within the existing cartridge size.

| ARM cycles | Byte Hatchery | Packed Hatchery |
| --- | ---: | ---: |
| Blood Bath mean | 1,026,648 | 1,003,303 |
| Blood Bath p95 | 1,426,164 | 1,404,322 |
| Blood Bath peak | 2,209,088 | 2,182,928 |
| P7 mean | 1,766,436 | 1,766,498 |
| P7 p95 | 2,066,260 | 2,066,456 |
| P7 peak | 2,433,880 | 2,433,880 |

Compared with the original release in the preceding table, the combined
Blood Bath build is now 0.17% slower on mean cycles and 0.22% faster at p95.
P7 stays within 62 mean cycles of the faster depth-and-steering build.
Use `P10_HATCHERY_WORDS=0` to repeat the paired comparison.

The integrated Blood Bath receipt SHA-256 is
`c4a8090ef2854b13110591fc44a11c6ec541560fcfba797ab1fd10a6fd70cb0c`.
The integrated P7 receipt SHA-256 is
`de9aa79d7b953b32accf6d88b1ac14d0d1b7f6b4585a8c644b279ab1cd9ad350`.

## Release composition

Composed source `063c54fb` applies the renderer and packed Hatchery to
release `2dec1556`, including the P10 steering and standalone P6 save
changes. All four ARM builds pass ROM, header, SRAM signature and memory
checks. Each image is byte-identical to its previously qualified renderer
image, including the packed P10 run above.

Thirteen local renderer checks pass. They cover framebuffer and HUD
rendering for the three race products, dirty-row transitions, packed
Hatchery clipping, and Showcase with its normal, packed and indexed
sprite paths.

The exact three race images complete 702 ordinary-input custom-game
launches. The matrix covers both maps, each human race, and every ordered
one-, two- and three-CPU race lineup. Each case checks the starting roster
and plays 120 updates. Across 54 sampled lineups the player saves through
the pause menu, reloads in a fresh emulator, and reproduces the following
60 updates. This is early skirmish and save coverage. Longer campaigns,
endgame skirmishes and hardware timing have their own checks.

The renderer receipt SHA-256 is
`e68e0fae1c284e6a44d8694e0944636823fb281050080d39e032c83b63b19098`.
The skirmish receipt SHA-256 is
`99571add436acc888849844cbbb50d04f2a4c6078d3866aa19c2dadc40082e0c`.

The selected-ring follow-up also passes all four normal ARM links and
header checks. Against the exact `9dfba082` P10 ROM, the 958-advancing-
update P7 window matches all 1,000 timeline rows and 11 sampled world,
palette and framebuffer states. The release mean of 1,766,498 cycles
falls to 1,760,602, p95 from 2,066,456 to 2,059,279, and peak from
2,433,880 to 2,427,848. The isolated overlap fixture exercises the
changed ring pixels. The ordinary P7 sample checks the wider scene.

The selected-ring P7 receipt SHA-256 is
`c909430793b05122a7ccd6e7c5180e2dac03e22d70d0eb2a2cfdf110b8cd4745`.

## Friendly actor selection

One starting Overlord moved across the Hatchery in a fresh native
Challenger match. At N64 observer step 4,320, the four Drones were
selected and the cursor was over opaque pixels shared by the Overlord
and Hatchery. The next ordinary A press selected the Overlord with
200 HP at step 4,345. The cursor was closer to the center of the
Hatchery's displayed selection ring. The capture observes pixels and
selection through the interface. Internal anchor coordinates were
outside the observer's scope.

The input tape SHA-256 is
`631cd9bce29df25060cdf4163f1d48f6a107143e2555160a97d21888825871ab`.
The clean overlap, pre-click and post-click frame SHA-256 values are
`ea0147de1d0cce43e2e0ecddb9bf8c5377b23e952e244b0e6369baf18185a6d1`,
`0f272222c547928ad0667d59b623bb6f68b903c6e9315057ddc9693e8363f464` and
`099d107cb93306c6aba103932101f16a53df043e6446bbaab3162d3f9415aa1a`.

`SC64_FOREGROUND_PICK` opts the shared Mode 4 renderer's friendly
selection into using the same depth key at an opaque intersection
of the nearest eligible unit and building. It keeps the existing
proximity result on single-body clicks and when the primary sprite
cannot establish both footprints. The supported frames cover source
flyers and stationary ground units against ordinary finished
buildings. The picker reads indexed sprite art and does not advance
animation state. In a fresh prepared build, pass the flag with
`make CC='arm-none-eabi-gcc -DSC64_FOREGROUND_PICK' verify-protoss-p10`.

`make test-sc64-foreground-pick` passes under ASan/UBSan. It uses the A
handler to select an Overlord
over a Hatchery and a Marine behind a Barracks using each actor's
isolated painted mask. It also checks proximity selection where only one
body has an opaque pixel or both pixels are transparent.

### P10 on ARM

The paired 32 MiB P10 builds use source `27ce74df` (release
`99e4c26d` plus the opt-in) and the same 40 prepared inputs. Both
pass ROM header, SRAM signature, and memory checks. Both end IWRAM
at `0x03004d78` and EWRAM at `0x0203b434`. The opt-in adds 1,272
ROM text bytes. The flag-off ROM matches the retained release P10
image byte for byte.

The same 48 physical controller calls launch Outpost from HOME with
human Zerg and one Terran computer. A at (160,160), where only the
Hatchery has opaque art, selects the Hatchery on both builds. Select
clears it. Ten Left updates and one Down update move the cursor to
(140,162), where both sprite bodies have an opaque pixel in both
Overlord hover poses. At that point the cursor is closer to the
Hatchery (squared distances 404 and 416). A advances frame 15 to 16
and selects the Overlord with the opt-in, or the Hatchery with the
flag off. The sampled world state matches on either side of that
click. This checks two clicks in one ordinary custom-game opening.
The selector remains opt-in.

| Local receipt | SHA-256 |
| --- | --- |
| Release P10 ROM | `4091d3ba0c6838bc4152d222dcace0007bcd20ded876dffb8247231f92111343` |
| Opt-in P10 ROM | `01f5d36ee92f2f623046a098cf1bab38ca9dbc27a0556a676415273184fc2cd6` |
| Physical controller observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Paired physical input and selection receipt | `5be75d7c6cf781b073dc79e8ac664770726cb44b071aa69835d2d55fb9dccae8` |

### Showcase and Z10 on ARM

The Showcase and Z10 opt-in builds at `2b7cb12a` pass the ROM
header, SRAM signature and memory checks. They use the same 32 and
38 prepared inputs as their retained `99e4c26d` controls, respectively.
Showcase ends at IWRAM `0x03004f10` and EWRAM `0x02038f64`.
Z10 ends at IWRAM `0x03004ce8` and EWRAM `0x02037e80`. Both pairs
have identical memory endpoints.

In Showcase, 21 ordinary calls open the scene menu, enter Zerg
Last Stand, resume, select the Hatchery with A, return to the same
scene row with Select and go back to HOME with B. The sampled world,
selection and menu states match the control. The Z10 pair uses 226
ordinary calls to start Blood Bath as Protoss against one Terran,
select the Nexus and save at frame 124. Both pause saves are the same
32 KiB. A fresh opt-in process loads the release journal and matches
the original world through frame 184.

| Local receipt | SHA-256 |
| --- | --- |
| Release Showcase ROM | `975487861480120e50d1e4294979cfa6d7a10c699f9ef43fdff7b925310e6b8e` |
| Opt-in Showcase ROM | `6873e081d5cea03386db10de070ef328e5344279b200903570cc9be26936afae` |
| Showcase physical input receipt | `7b0947ee406e4d48b6d4f518be3d4b2779bff1689c050cfbbc02e397e46493e1` |
| Release Z10 ROM | `f784da09d7f772b1598b8f7c31be212d648e955a0c7f171c7e0a357db035da99` |
| Opt-in Z10 ROM | `9091074cd2401e2446701c9c79ac80e27def634da2f13ba08310874700d93ead` |
| Z10 physical selection and save/load receipt | `2b037ab3516cdd55710137368b14f842cdd38fe2aab94260172a23f907f0b874` |

### Terran on ARM

The Terran opt-in at `f2908551` composes the selector with the
`ea041527` release and uses the same 38 prepared inputs as that
release's existing control ROM. The opt-in passes the ROM header,
SRAM signature, and memory checks. Both images end IWRAM at
`0x030048a8` and EWRAM at `0x02038d5c`.

From HOME, the first 1,100 physical calls of the T4 campaign tape
reach frame 1,079 in both images. Both match all frame and screen
observations, eight A command-menu transitions, and six sampled
world, framebuffer, palette and fog states. The release also matches
the retained T4 checkpoint at call 1,000.

A second route follows the tape through 91 calls, presses Select to
clear the group, moves left seven steps and up five, and presses A
at (1825,1940). At frame 96 both builds select only unit slot 84 and
leave the building selection empty. This covers a direct click in
the campaign.

A 44-call Terran Outpost tape repeats the opaque overlap case from HOME
as human Zerg against one Terran computer. Both ROMs select the isolated
Hatchery at (160,160). At (140,162) both actors cover the cursor in both
hover poses. With A, frame 15 advances to 16. The release selects the
Hatchery; the opt-in selects the foreground Overlord. The world matches
before and after the click. Both indexed pages, palette and displayed page
also match immediately before the click.

| Local receipt | SHA-256 |
| --- | --- |
| Release Terran ROM | `f0691c9e08bf2a30b554382a179998c2daa96b6295b2f0f61f1f642693e66973` |
| Opt-in Terran ROM | `4bbe59da8a42cf90647d425d69e4a1e91539eb16ae2e4ed7a6180e6974929dac` |
| T4 controller tape | `e9b505111c6777ed4adc98218c2009091702be171f4259fbeaa56114c72dd5a0` |
| Physical controller observer | `ff2c5a8b1f6764ed399d8a1b59df6ffde135ae2cd01a6d67500684b8429d23b7` |
| Bounded T4 campaign receipt | `14db9a85889a2fe1095498a652279f1fc8b6e0d4dcb336990707b95493b3e2b0` |
| Direct A selection receipt | `8ef8301197af3f14d3e64c505a77ed2df636676a36efb832571ccdb32b09bd53` |
| Terran Outpost overlap receipt | `3f3df62631c6929c5756dfddcae41e6fde693fa8ce756216039f2ea094b4816d` |
