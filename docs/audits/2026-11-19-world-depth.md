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
and resources. The static lookup matches all 228 entries in the pinned N64
unit table. Composite turrets remain with their hull. GBA actor kind and
slot resolve exact ties as a deterministic approximation of the native
sprite-ID tie. Native depth changes for special states still need individual
checks. The change covers world actors in the shared campaign, skirmish and
Showcase renderer. See the projectile section for moving weapon sprite
coverage and the remaining passes that need separate depth references.

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
| 1 | `scv-foundation-depth` | At a paid Terran foundation, compare an SCV north of the foundation as it approaches, attaches, works and completes. The foundation covers the idle SCV at layer 4. The working SCV at layer 5 covers the foundation. | See the [SCV foundation overlap](#scv-foundation-overlap). |
| 1 | `interceptor-flight-depth` | Compare launch (15), attack (17), early and late far return (15 or 17), and final approach (14) against a Carrier at 16. Cover both actor slot orders and confirm docked children are hidden. | The Protoss framebuffer and save fixture covers the GBA runtime. Ordinary-input N64 and ARM overlap captures remain open. |
| 1 | `effect-crosses-actor-depth` | Capture a moving bullet fired at a ground target (layer 5) crossing a ground actor (4), a flying building (12) and a flyer (16 or 18). Repeat for a flyer target, using its live layer plus one. Sample after the target moves, lifts or dies. For point-target fire use the shooter's live layer plus one. | Use the [projectile replay cuts](#projectile-replay-cuts). |
| 2 | `opaque-pick-depth` | Reuse the overlap mask for selection. A over opaque foreground pixels selects the visible eligible actor, and A on transparent pixels follows the ordinary fallback. Include exact ground Y ties, equal air layers and a lifted building. | Challenger Overlord/Hatchery reference in draft [#587](https://github.com/ggu/starcraft-gba/pull/587). See [foreground picking](#foreground-picking) for the shared live layer key and current opt-in checks. |
| 2 | `same-anchor-sprite-id` | Capture same-layer, same-Y SCVs with distinguishable opaque pixels. The native sprite ID breaks the tie when both turret bits are clear. | Blood Bath observer step 4230 has stacked layer-4 SCVs at (1940, 293) with sprite IDs 0x858 and 0x857. The native key puts 0x858 in front. Pixel ownership remains open. |

For replay receipts record the product, source commit, controller tape hash,
world anchors, effective layers, active page, clip rows, overlap count and
wrong-pixel count. For native dynamic actors also record the actor and
sprite addresses and their captured layer. Keep native screenshots and
memory dumps local. Compare the isolated GBA masks within each version's
palette. The N64 and GBA use different pixel scales and palettes.

### Foreground picking

Both native selection comparisons after the depth helper use unsigned
`sltu`, at ROM offsets `0x8b240` and `0x8b3c0`. Elevation uses the high
bits of the key, which include bit 31 for layers 16 and above. The opt-in
GBA picker compares the same unsigned key and live layers that its painter
uses. Its kind and slot break GBA ties. Native turret and sprite-ID ties
need separate captures.

Run `make test-sc64-foreground-pick` with prepared local assets. Repeat
the Overlord/Hatchery and Marine/Barracks overlap with both bodies
opaque, each body transparent in turn, and both transparent. Also check
a same-layer and same-Y Marine/Barracks pair, a lifted Science Facility
over a Marine, and a Wraith over the lifted Facility. Outside the left
and right viewport edges, in either HUD band, in fog, and for unsupported
construction or attack poses, keep the normal proximity result. Keep
friendly eligibility, same-kind selection and contextual resource orders.
The opt-in only resolves a competing friendly unit and building.

The current Terran, Z10, P10 and Showcase ARM pairs replay the same
ordinary inputs per product from HOME through a Zerg Outpost opening.
Both builds choose the Hatchery where its body is the only opaque one.
At (140,162) the opt-in chooses the foreground Overlord and the release
chooses the closer Hatchery. Both indexed pages, palette and gameplay
state match before the click. Gameplay state still matches after it.
All four opt-ins link with unchanged RAM endpoints. The rebuilt flag-off
Terran ROM matches the release byte for byte. This remains opt-in.

| Cartridge | Calls | Paired replay receipt SHA-256 |
| --- | --- | --- |
| Terran | 44 | `862778b67b9f22b90c16c9e32dbe04500132c9494f493e584998a8416bd06483` |
| Z10 | 50 | `edf9c26e6571b4adb34b272f6b043c54e10c4cdd187d4fd4f226fdfbfb575e65` |
| P10 | 48 | `433af1542b2798a2af55da6ee5efe7fae672c05a9c469562f2025e3ef68fc589` |
| Showcase | 48 | `8ee69094529f9905fac5e34a724109be725c4cbca0349a42a9ad8b00da5fa27e` |

### SCV foundation overlap

The owned USA ROM raises the builder's live sprite elevation by one when
placing a Terran foundation or attaching to an unfinished one. The
stores to sprite offset `0x0d` are at `0x80126854` (ROM `0xea544`)
and `0x80125e58` (ROM `0xe9b48`). The reset-collision order restores
the unit type's base elevation at `0x80125cc4` (ROM `0xe99b4`). Both
attachment paths queue that reset. The SCV therefore uses elevation 5
during construction and returns to 4 when normal collision resumes.
[OpenBW construction and collision reset](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L4312-L4489)

The shared GBA Mode 4 painter derives the layer from the existing worker
order and foundation. A build-pending SCV and an assigned SCV outside
the engine's 36-pixel work radius keep layer 4. An SCV in work range of
its live, friendly, unfinished target uses layer 5. Completion or loss
of the job restores layer 4. The GBA uses its work radius to approximate
native attachment, with no additional save state.

The Terran framebuffer fixture exercises `scv-foundation-depth` with
isolated actor masks, a north-side overlap in idle and working states,
both sides of the work-radius boundary, and a build tick that completes
the structure. All 149 distinguishing pixels match. The Terran, Z10,
P10 and Showcase ARM builds preserve their IWRAM and EWRAM endpoints
from `0b1fbde3`. Normal menu smoke replays match the sampled world,
palette and both indexed pages after 60 and 120 input calls. These
smoke samples occur before construction.

For ordinary-input replay, start a Barracks in T1 and in Terran Custom
Game on all three race ROMs. Retry with a replacement CPU SCV
approaching an interrupted build in a custom match. Use a Terran build
scene on Showcase when available. Save during construction and reload
in a fresh emulator before completion. Check an idle, stopped or
orphaned SCV at the same anchor. Compare overlap pixels with
uninterrupted play and check HUD clipping on both pages. The retained
[native Barracks replay](2026-11-17-native-scv-foundation-interruption.md)
supplies known working cuts at 9,308 and 9,332 and completed cuts at
11,844 and 11,900 for a new framebuffer and sprite-layer capture.

### Native bullet depth

The native bullet constructor at ROM `0x93a4` reads the shooter's order
target. The branches at `0x800087c0` and `0x800087f0` assign the
bullet's sprite elevation once at creation from the target or shooter
respectively. The 116 bytes at ROM `0x9394` have SHA-256
`2824fe5e99a1890914746cb35cdbd74e46bfc84876ee41787561704fd5a81c01`.
The N64 render traversal at `0x8012036c` walks the sprite row heads at
`0x801128a0` and submits sprites to the same queue as units. The unmodified
Blood Bath capture at step 4230 shows a live mining effect in that queue
with sprite ID 2088 at (1958,284), elevation 5. Its RAM SHA-256 is
`bcacbc2bc25e1a0f4b0321654ff28b42e98788e81dfc40869dfb5c2ceb3d58f1`.
The native target-detach routine at `0x8000859c` copies the target position
and clears the pointer. The sprite retains its existing layer. In a retained
late-game trace, a bullet at layer 5 outlives its target while its Lurker
shooter remains at layer 1. The late-game trace used a defeat-suppression
patch. Its game frame 31138 RAM SHA-256 is
`3688890b01895af6bd85ecc7eb23cefd26506da8a742ff30dd7fdafd2b30cdbc`.
[OpenBW bullet creation](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L13759-L13769)

The shared GBA Mode 4 painter interleaves source-backed weapon
sprites with actors. It reads live depth on the first GBA update that reaches
the source release tick, then holds the result. If an actor disappears
during wind-up, the attack-init snapshot supplies the depth. Reusing its
slot cannot redirect the depth sample to the new occupant. Layers 0–4 use
the projectile's current world anchor, including fixed-position effects.
The projectile and its current visual
children draw together. Shooter overlays still use
their separate pass. Death effects, spell pools and Interceptor pulses
also need their own depth comparisons, as do independently spawned impact
and trail sprites. The existing queue slot order breaks GBA projectile
ties. The experimental Mode 0 renderer still separates cached static art
from dynamic actors.

The native constructor writes the sprite elevation before it dispatches
on weapon behavior. The behavior table load at `0x80008814` and jump at
`0x80008834` follow both elevation writes above. A stationary shell burst
therefore takes the same target-or-shooter elevation plus one as a flying
bullet. The Gauss Rifle (weapon 0) and siege cannon (27 and 28) all take
behavior 2 to `0x800088c0`. At game frame 10,001, a staged native
Academy capture contains two Gauss Rifle sprites at layer 5 targeting the
same layer-4 unit. The normalized RAM SHA-256 is
`da3a833eca8432feb659271d2ac5f5609704b3d0b6f8bfd7602183dff33b6c87`.
The GBA formerly drew stationary effects in a final pass above
every actor. Their depth now uses the shared release snapshot and queue.
An effect keeps its captured anchor under the existing GBA animation
model. The native behavior-2 launch path at `0x8012a538` calls a helper
to place the effect within the target's bounds. Use the captured sprite
anchor for the N64 Y oracle. Exact anchor placement and independent
child sprites require their own tests.

#### Projectile replay cuts

Use the same five-frame overlap oracle as the actor cases above. Capture
the isolated projectile part without the shooter's overlays when measuring
foreground ownership. Run through the normal campaign, Custom Game and
Showcase entry points. Record the product, source hash, tape hash, scene,
camera, trace slot, target or shooter ref, source and GBA release ticks,
that actor's layer, captured projectile layer, active page and both overlap
counts. Store ROM-derived pixels and memory locally.

| ID | Setup | Expected outcome |
| --- | --- | --- |
| `ground-shot-through-flyer` | Pause a shot across a ground unit, then repeat it across a lifted building and a flyer. Repeat with reverse actor creation order. | At layer 5 the shot covers the ground unit. Layer 12 and higher sprites cover the shot. |
| `air-shot-over-ground` | Fire at a flyer backed by a ground unit and sample the projectile's flight. | The projectile takes the target's layer plus one and covers the ground actor. |
| `ground-shot-by-moving-y` | Fire diagonally at a layer-3 Bunker. At the same animation step, put a Tank north of the projectile anchor, then south. | The layer-4 projectile covers the north Tank and passes behind the south Tank. |
| `stationary-shot-depth` | Fire a sieged Tank at a layer-3 Bunker, pause on a visible impact phase, then repeat with a Tank three GBA pixels north or south of the impact or with a Wraith over it. Sample after target removal. Use the native weapon behavior and sprite anchor to identify the matching N64 effect. | The layer-4 impact covers the north Tank. The south Tank and layer-18 Wraith cover it. The released effect keeps its layer when the Bunker disappears. Compare both draw orders directly when the source splash remaps underlying palette colors. |
| `shot-release-depth` | Change target layer during wind-up, change it again during flight, then remove the target or recycle its slot. For a point shot change the shooter's layer during wind-up. | The projectile uses the release tick's actor layer and keeps it through flight. A slot recycled during wind-up leaves the attack-init snapshot in effect. |
| `projectile-hud-page-flip` | Shift the camera so the flight intersects screen rows 12 and 119. Step two page flips with the cursor out of the overlap. | Terrain, bodies and projectiles are clipped to the field. HUD pixels are identical in both paired comparisons. The N64 oracle checks foreground UI without requiring GBA-specific row coordinates. |
| `projectile-load-hit-once` | Save with a queued hit, then load in a fresh process. Repeat with the target obscured during the shot. | Cosmetic traces clear on RaceSave load. The persisted damage resolves once, using the existing eligibility and impact rules. |
| `independent-effect-depth` | Capture muzzle, projectile body, trail, impact and corpse or shadow on separate native animation ticks against the same covering actor. Repeat with a special-service projectile. | Record the native sprite ID and elevation when each independent sprite exists. Use the parent sprite's depth for image attachments and derive a separate oracle for independently spawned sprites. |

#### Projectile host coverage

Run `make test-sc64-native-weapon-game` with the prepared weapon bank. The
Showcase sanitizer fixture exercises all 66 source weapon profiles and
isolates the palette-indexed moving Dragoon projectile at fixed ages. The
ground shot covers the Tank on all 47 distinguishing pixels and goes
behind the Wraith on all 28. An air shot covers the Wraith on all 28.
Boundary clips preserve both HUD bands and match 8 upper-edge and 38
lower-edge overlap pixels. The diagonal, layer-4 shot matches 48 pixels
on each side of a Tank's anchor. The fixture also verifies the SCV's
wind-up layer change, shooter fallback for a point shot and a target slot
reused before release. The stationary case replays the same siege splash
in both explicit paint orders and matches all 14 distinguishing pixels
in each of its three setups. It runs the production queue and post-world
pass and checks both HUD bands. The four normal ARM products link with the
same RAM endpoints as `06353a5d`. Their ordinary Custom Game startup
samples match both indexed pages, palette, and world at 60 and 120 updates.
The Showcase Siege Line also matches both indexed pages, palette and
world at 24 samples over 120 advancing updates after the ordinary menu
chord. The run uses 116,541,162 emulated cycles versus 116,108,952 in the
control, an increase of 0.37%. The comparison receipt SHA-256 is
`2af496b84e91b948dfad6dd4314e617e8437434520424b2463ce8dc38c3530db`.
The overlap fixture supplies the changed pixel-order evidence. Paired
ordinary-input N64/GBA siege overlap captures remain pending.

#### Projectile cartridge coverage

Against release `6720a2fb`, the current Terran, Z10, P10 and Showcase ROMs
pass header, SRAM signature and RAM guards. IWRAM endpoints are unchanged.
EWRAM grows by 120–128 bytes, and the painter's bounded stack array reserves
up to 128 additional bytes. Ordinary Custom Game inputs produce matching
indexed pages, palette and world fingerprints at 60 and 120 updates on all
four products. Smoke receipt SHA-256 is
`29f45efa2d2aaee32c5e012339ea62003dec87ff6cc84b68956d4698b01d23f4`.

The full Z8 ARM controller replay completes 46,487 calls. All 47 world
checkpoints and 24 sampled two-page framebuffer pairs match the control.
The overlap oracle above covers the foreground differences directly. Mean
cycles per advancing gameplay call come to 1,024,346 versus 1,025,307 in
the control, a difference of 0.09%. This measures emulated guest cycles.
The comparison receipt SHA-256 is
`41e74a51fbdda1892a41c108454d9c90d20bc655c4cbb6d7ed1a42fd90f56648`.

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
sampled world at 60 and 120 calls on all four products. The host fixture
exercises Interceptor overlap.

The Terran, Z10 and Showcase Carrier previews also link with valid
headers and within their RAM guards. The ordinary controller chord and
scene menu reach the Showcase preview's Carrier battle. Over 450 updates,
the first observed docked, launch, early far return, attack, approach and
late far return states occur at frames 2, 60, 75, 80, 87 and 90.
The observer records both indexed pages and the palette at each first
observation. The receipt SHA-256 is
`d60671fd05299b37f13cb06fd21a3b0866428ba9a2a935291a37caa01022a527`. These are state and frame captures. The isolated-host mask
provides the pixel-ownership check.

For the ordinary-input harness, record these cuts in Protoss campaign
and custom games, then repeat in the Terran, Zerg and Showcase Carrier
previews. Capture the first release, the first ascent, a far-return cut
for each history, the first approach cut, and dock. Compare the visible
indexed page and the isolated actor mask at a fixed camera. At each
far-return cut save via the pause menu, load via HOME in a fresh process,
and replay the tail through docking. The full Carrier service is enabled
in regular Protoss and the Terran, Zerg and Showcase previews. The N64
test should sample the child's sprite elevation and owner, both anchors,
and the finished framebuffer for each cut.

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
