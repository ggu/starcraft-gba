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
| `air-tiers-ignore-y` | Place a Wraith six pixels north and then south of an Overlord, layers 18 and 16 | Wraith in both slot and Y orders | Terran HOST fixture |
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

### Dynamic depth harness

The native painter reads the **live** elevation byte at sprite offset
`0x0d`. Rechecking the owned USA ROM and the local N64 observer RAM confirms
that takeoff writes 12 at `0x8014bb9c` (ROM `0x10f88c`) and landing writes 4
at `0x8014b7e8` (ROM `0x10f4d8`). The ordinary-input [Blood Bath lift
capture](2026-11-13-n64-blood-bath-airborne-last-building.md) identifies the
same Command Center before and after takeoff. The layer stores establish the
depth rule, while the retained controller run documents the visible takeoff.
The independent Blood Bath observer RAM from step 4230 has SHA-256
`bcacbc2bc25e1a0f4b0321654ff28b42e98788e81dfc40869dfb5c2ceb3d58f1`.
Its depth helper and layer stores match the owned ROM byte for byte.

The shared GBA painter derives building elevation from the live flight
predicates. Layer 12 goes in front of ground actors regardless of world Y
and behind ordinary flyers at 16 or above. Landing restores the building's
ground depth. The change uses the existing campaign and shared Science
Facility flight state, with no additional save data.

The Terran framebuffer fixture passes under ASan and UBSan. The Terran,
Zerg, Protoss and Showcase ARM links preserve the IWRAM and EWRAM endpoints
from `81f62111` with the same prepared inputs. Frame replay and hardware
timing remain open for the flight overlap.

| Priority | Case ID | Setup and expected result | Status |
| --- | --- | --- | --- |
| 1 | `building-liftoff-depth` | Move a Science Facility from grounded to airborne and back. Use a Tank whose Y is just south of the Facility. The Tank covers it on the ground and the Facility covers the Tank in flight. Repeat with a Wraith covering the airborne Facility. | Terran HOST checks cover 396 pixels for each Tank overlap and 223 for the Wraith. HOST flight transitions cover both the Science Facility and late Terran Command Center. Ordinary-input ARM lift and land replay pending. |
| 1 | `airborne-save-depth` | Save after issuing Land but before arrival. In a fresh session, sample the same overlap at load, during flight and after touchdown. Require layer 12 until the live flight flag clears and the same winning pixels as uninterrupted play. | Pending. Use a late Terran Command Center and a shared Science Facility in Custom Game. Include Showcase if the scene supports lift and save. |
| 1 | `air-tiers-ignore-y` | Preserve the Wraith/Overlord pixel result as the actors cross both viewport boundaries. Flip both framebuffer pages and require no actor pixels on HUD rows 0–11 and 120–159. | HOST isolated overlap passes. Ordinary-input page-flip replay pending. |
| 1 | `interceptor-flight-depth` | Compare launch (15), attack (17), early and late far return (15 or 17), and final approach (14) against a Carrier at 16. Cover both actor slot orders and confirm docked children are hidden. | The Protoss framebuffer and save fixture covers the GBA runtime. Ordinary-input N64 and ARM overlap captures remain open. |
| 2 | `effect-crosses-actor-depth` | Capture the same ground projectile crossing opaque pixels of ground and air actors, with animation tick and sight fixed. Verify projectile and impact layers independently before changing a pass. Include a landing shadow and a low remnant. | Pending native frame pairs. Free-standing GBA weapon effects still draw after the actor pass. |
| 2 | `opaque-pick-depth` | Reuse the overlap mask for selection. A over opaque foreground pixels selects the visible eligible actor, and A on transparent pixels follows the ordinary fallback. Include exact ground Y ties, equal air layers and a lifted building. | Challenger Overlord/Hatchery reference and opt-in GBA results in draft [#587](https://github.com/ggu/starcraft-gba/pull/587). Native sprite-ID ties and broader poses need separate witnesses. |
| 2 | `same-anchor-sprite-id` | Capture same-layer, same-Y SCVs with distinguishable opaque pixels. The native sprite ID breaks the tie when both turret bits are clear. | Blood Bath observer step 4230 has stacked layer-4 SCVs at (1940, 293) with sprite IDs 0x858 and 0x857. The native key puts 0x858 in front. Pixel ownership remains open. |

For replay receipts record the product, source commit, controller tape hash,
world anchors, effective layers, active page, clip rows, overlap count and
wrong-pixel count. For native dynamic actors also record the actor and
sprite addresses and their captured layer. Keep native screenshots and
memory dumps local. Compare the isolated GBA masks within each version's
palette. The N64 and GBA use different pixel scales and palettes.

### Interceptor flight overlap

The owned USA ROM uses three writes to sprite offset `0x0d`. At release
`0x801282e4` (ROM `0xebfd4`) writes parent elevation minus one. At attack
ascent `0x801287ec` (ROM `0xec4dc`) adds two to the child elevation. At
final approach `0x80128990` (ROM `0xec680`) writes parent elevation
minus two. The live depth helper uses that byte. Carrier and Gantrithor
both use elevation 16 in the native table. The far return has no elevation
write. That leaves two valid far-return depths according to when the
return started.

The shared painter now reads those live flight phases in any Mode 4
build with Interceptor flight enabled. The existing saved child order
state stores the ascent history for far return. New saves keep the two
histories through reload. Older saves already in far return lack that
history and use 15 until approach. An older build cannot read a new
Interceptor row saved during post-ascent far return. The record size and
other order states stay the same.

Run `make build/test-sc64-protoss-canonical-p7-save` with prepared
inputs and then execute that binary. The fixture drives launch, both
far-return histories, reload, approach and dock through the actual child
order and save routines. Its indexed-frame samples isolate both actors at
the same screen anchor, reverse their slots, and compare opaque overlap
pixels from the world painter. Both history cases produce 87 or 92
distinguishable pixels per capture. The HUD bands remain clear.

Against release `13184991`, all four normal ARM builds pass ROM header
and SRAM signature checks with the same IWRAM and EWRAM endpoints.
Terran, Z10 and Showcase are byte-identical. The P10 ROM SHA-256 is
`8a19f47f8030fc80511c16fdef7887e52e90ba4ab661308c00dace82916c8ca5`.
Ordinary-input smoke replays match both indexed pages, palette and
sampled world at 60 and 120 calls on all four products. Those opening
samples do not exercise Interceptor overlap.

For the ordinary-input harness, record these cuts in Protoss campaign
and custom games, then repeat in the Terran, Zerg and Showcase Carrier
previews. Capture the first release, the first ascent, a far-return cut
for each history, the first approach cut, and dock. Compare the visible
indexed page and the isolated actor mask at a fixed camera. At each
far-return cut save via the pause menu, load via HOME in a fresh process,
and replay the tail through docking. The normal Terran, Zerg and Showcase
products do not yet enable the full Carrier service. The N64 test should
sample the child's sprite elevation and owner, both anchors, and the
finished framebuffer for each cut.

### Cache and clip regressions

| ID | Setup | Expected framebuffer |
| --- | --- | --- |
| `depth-cache-scroll` | Put a building and a mineral patch at each horizontal viewport edge. Scroll either way by 4 pixels across all 16 camera phases on each axis. Change a visible fog cell, construction state or mineral depletion on the next frame. | Every indexed pixel matches a fresh render of the same world and animation tick. The actors keep their depth order as they enter and leave view. |
| `depth-cache-fixed-fog` | Hold the camera, change fog near overlapping actors, and render across alternating backbuffers. Run with full capture and again with optional dirty capture. | Both captures reproduce the fresh render, including the pixels between dirty bands and at each HUD boundary. Dirty capture reports fewer captured rows when its reuse checks pass. |
| `legacy-static-clip` | Compile the static-row fixture with world depth disabled. Change two separated fog rows while overlapping static props share a band. | Each dirty band replays the static props in painter order, and `draw_map` restores the incoming clip. |

`make test-sc64-terrain-transfer-scenes` runs the first case for Terran,
Zerg, Protoss and Showcase. `make test-sc64-static-dirty-rows-game` covers
the legacy path. A local Protoss sanitizer probe checks the fixed-camera
case at 16 camera phases. It reproduces every fresh-frame pixel on all 64
fog changes, capturing 48 of 108 rows per change. Keep the fixed-camera
case in the ordinary-input replay harness backlog.
The world-depth renderer no longer computes static clip bands on the
default full-capture path. With optional dirty capture, the planner still
supplies terrain row ranges, while the sorted actors draw after capture.

### Removing the empty static clip pass

On the release P10 build, the measured Homeland window contains 907
moving-camera partial restores, all with word-aligned horizontal shifts.
Instruction-edge measurements across 113 of those updates attribute a mean
41,254 cycles to restoring the scrolled background, 11,557 to filling fog
signatures, and 42,150 to capturing the two cache regions. The total
background pass averages 191,287 cycles in this subset. These intervals
include any audio interrupt time. The full capture remains a candidate for
further profiling.

The table compares the release renderer on `0994f416` with the same
build inputs after removing the empty clip pass. Eleven sampled Homeland
and 32 Blood Bath states match, including both indexed pages, palette,
fog, units, buildings and resources. Camera positions, route counters
and frame advancement match across the measured windows.

| P10 ARM replay | Advancing updates | Mean background cycles | Mean update cycles | Peak update cycles |
| --- | ---: | ---: | ---: | ---: |
| Homeland | 958 | 183,372 → 181,031 | 1,728,678 → 1,726,409 | 2,418,393 → 2,405,240 |
| Blood Bath, three AIs | 1,000 | 54,903 → 54,657 | 998,765 → 998,489 | 2,184,140 → 2,185,216 |

The Homeland, Blood Bath and instruction-edge receipt SHA-256 values are
`9c4283413d0c2bd41519acefcb17fe07613856988b7a0747e98aa581edc981f3`,
`f48814330def8cb9af91089c03dfaacc3ff93fd682e37edb125a4bae0826dfd8`
and `ac5bf6cb73cd9e48f0911b5d4450035f6d9367cef0899b26acc54b39f3293df5`.
These are emulator cycle measurements. Blood Bath's peak rises 0.05%,
and performance on hardware remains unmeasured.

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
