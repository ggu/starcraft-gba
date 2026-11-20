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

## Visibility and building overlay cost

The shared Mode 4 queue now skips objects that the fog test would reject
in the paint pass. Friendly unit and building visibility keeps its
existing treatment. Construction and selection resolve building
footprints when they need them. Weapon, damage, and other overlays use
their own frames and anchors. The depth key and pixel order remain the
same.

Paired normal Z10 and P10 replays on release `9ee056c2` match sampled
world state, palette and both indexed pages. Z8 has 21 matching samples
and P10 has four through the result screen. Each Z8 timed sample also
matches the frame and route counters.

| Replay window | Main-loop mean cycles | Actor and effect mean cycles |
| --- | ---: | ---: |
| Z8 calls 17,400–17,480 | 1,101,537 → 1,099,631 | 125,554 → 123,198 |
| Z8 calls 31,050–31,250 | 1,593,060 → 1,589,338 | 482,107 → 478,790 |
| Z8 calls 42,000–42,203 | 1,587,927 → 1,574,512 | 660,949 → 647,109 |
| P10 calls 45,218–45,275 | 2,018,288 → 2,006,353 | 359,837 → 346,210 |

Across these windows, the mean drops by 0.17% to 0.84%. All four normal
products keep their RAM endpoints. Ordinary custom replays cover
one middle-seat opponent on the compact map and three named opponents
on Blood Bath. Each pair matches resolved seats, world state, palette,
and both indexed pages at 120 and 360 updates. The Terran, Z7 and P10
framebuffer fixtures and Showcase weapon fixture pass under sanitizers.

The comparison receipt SHA-256 is
`9f801806018fbe1e960bf8c004329fc017906f58830bdc946583dc275b7040ff`.
The custom replay receipt SHA-256 is
`e72ce8250143c07c5c9b55e4bb4bbabe64b22ca4992987cd50ff7439c911a672`.

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

### Death sprite depth

The authenticated USA N64 order dispatcher calls the ordinary Death routine
at `0x8012c3bc`. That routine starts animation 1 on the actor's existing
sprite and preserves its live elevation at entry. The 280-byte function at
ROM offset `0xf00ac` has SHA-256
`c34b5423cecba28d75020590fd0d100f89e3937a369363c7d92cac2b2db1b1fc`.
A retained capture at native game frame 26,877 has a dying SCV at (46,1260)
with HP zero, order zero and sprite ID `0x066c`. Its sprite is in render row
39 with elevation 4 and sort key `0x213b066c`. The normalized RAM SHA-256 is
`c564005d460558a307f550955a5ffd86dc88fc53cfb1fa4e6ff6b167ce6e6134`.
The SCV Death script uses `imgol` to add image 332. Native sprite
`0x802163a0` has that image at the head of its image list, and the image
points back to the same sprite and shares its key.

The N64 IScript handler for opcode 17 (`lowsprul`) passes fixed elevation 1
to the independent sprite constructor. The 68-byte handler at ROM offset
`0x13238` has SHA-256
`8a258b2707f64dd1a8d60f14561de2f027dcd906b9c2ed09d7d5b153b0aec14a`.
Two Hydralisk remnants in the frame 26,877 render list have image 32 and
elevation 1. Two Marine remnants with image 241 also have elevation 1 at
frame 19,444 (RAM SHA-256
`0924c9a329824371d7271968eb0b16ca1eaccceae76b54c44ca66a741866bac1`).

The shared Mode 4 painter queues the source actor's own collapse frames
and direct `imgol` Death attachments at the actor's captured elevation and
world Y. Independently spawned opcode-17 corpses and rubble use elevation 1
and their own anchors. The bank records both classes apart from the
under/over bit. The prepared bank has 157 actor profiles with main-image
or direct attachment frames and 84 with at least one fixed-low layer.
The destruction scan reads lift state from the retained building flag.
The existing off-screen and fog rules apply. Depth data occupies the same
20-byte transient row and clears with the effect pool. Prepared builds
require the current depth metadata. Regenerate the local bank with
`./dev sc64 prepare --rom PATH --profile PROFILE`.

| Case ID | Required overlap and checks |
| --- | --- |
| `death-root-ground-by-y` | Isolate a Zergling collapse at layer 4 and a Tank at a slightly smaller or larger world Y. Require the larger Y to win every distinguishing pixel in each pair. Repeat with a Marine or Hydralisk and capture its native Death image, sprite ID and elevation. |
| `death-attachment-depth` | Compare the SCV's attached image 332 against a Tank north and south of its anchor, then against a Wraith. Use the SCV sprite key for the native attachment. Repeat with a lifted Facility, whose burst sorts at 12 and whose rubble sorts at 1. |
| `death-root-under-flyer` | Place a Wraith above the collapse and move the camera past both HUD bands. Require the flyer to cover the root on each Mode 4 page and zero HUD changes. |
| `death-remnant-low` | Advance past the collapse to the independent corpse or rubble phase. Verify layer 1 at the remnant's own anchor and require a layer-4 survivor to cover it. Include building rubble and two layer-1 sprites at different world Y values. |
| `death-source-retained-depth` | Recycle the victim slot before the collapse ends and draw both products from the same camera. Require the effect to keep its death snapshot, exclude hidden effects and confirm that fresh SRAM load clears cosmetic entries. |

Use `make test-sc64-showcase-death-source` with the current private bank for
the isolated pixel checks. Add ordinary-input cuts from a battle in campaign,
Custom Game and Showcase. Record frame, screen page, actor and effect anchor,
layer, and distinguishing and wrong-pixel counts. Keep capture data local.

The Showcase sanitizer fixture matches all 2,110 distinguishing pixels
across nine SCV and Zergling overlaps, including both HUD edges. The Terran
fixture confirms a lifted Facility's layer-12 attachment and independent
layer-1 rubble after destruction. All four normal ARM profiles pass header
and SRAM checks with unchanged IWRAM and EWRAM endpoints. The painter stack
frame adds 192 to 200 bytes, with a 2,784-byte maximum across these builds.
Normal Custom Game launch checks match the prior release's gameplay,
palette and both indexed pages at calls 60 and 120. These samples precede
combat. The ARM matrix receipt SHA-256 is
`c47baa6654c6b0d69a8bca16233b87204aa174eef46d87651393d1860f38c5c5`.

An ordinary Showcase Siege Line replay advances 120 updates after unpause.
All 24 sampled world states, displayed pages, HUD bands and palettes match.
The battlefield differs in nine samples with active death effects. The first
four of those samples have 13 effects born immediately after unpause. Total
emulator cycles rise from 116,932,815 to 119,300,827 (+2.03%). The measured
cost is specific to this emulator replay. The detailed receipt SHA-256 is
`874b311ac6e6d14b6ecd383c37ef852f569eba16e5fc1d11c3e40749597267d2`.

#### Detached death sprite replay cuts

The USA ROM's sprite-spawn handlers read the parent's live elevation.
`sprol` and `sproluselo` add one, `highsprol` and `sprul` subtract
one, and `spruluselo` passes it through. The child gets its own sprite
record and anchor. The common spawn wrapper at ROM offset `0x128f4`
matches retained native RAM. Its 120-byte SHA-256 is
`8ad52e8696f17392798b811c3576690294c9f1b3ccbfbd9044aef73f23fd85b5`.
This rule is established from the native code and authenticated animation
scripts. A live native framebuffer cut for these deaths is still pending.

The prepared bank contains six drawable actor scripts that use `sprol`
in Death. Refinery, Extractor and Assimilator bursts sort at 5. Queen,
Matriarch and Devourer bursts sort at 19. Their second image joins the
detached sprite at source age 2, draws over its main image while both
exist, and keeps that sprite's layer after its main image ends at age 3.
The shared painter takes the original live actor layer from the death
snapshot. The importer caps the queue by the greatest number of
concurrent layers, which is two per effect for the prepared bank.

| Priority | Case ID | Replay cuts and expected foreground |
| --- | --- | --- |
| 1 | `death-detached-gas-burst` | Destroy a Refinery, Extractor and Assimilator. At a visible burst tick, overlap a layer-4 Tank south of the source and a layer-18 Wraith at the source. The burst covers the Tank and the Wraith covers the burst. |
| 1 | `death-detached-air-overlay` | Overlap a Wraith with a dying Queen, Matriarch or Devourer. Capture source ages 0, 2, 3 and the last overlay tick. The burst and its second image cover the Wraith. At age 3 the second image remains on top after the first image expires. |
| 2 | `death-detached-hud-page-flip` | Scroll the Refinery/Wraith overlap across rows 12 and 119. Repeat across both indexed pages and check every HUD row. Carry the replay through pause and fresh save load, then confirm the old cosmetic burst is gone. |

The Showcase HOST overlap fixture checks all three gas structures, the
Queen's second image, and both HUD edges. It compares 1,769 distinguishing
pixels across the seven new cases. For ordinary-input coverage, save both
GBA page hashes, the active page, the palette, source age, each live
elevation and world anchor, foreground overlap count, and wrong-pixel
count. In N64 capture, record the parent and child sprite IDs and the
active child image IDs. Keep the captures local. Compare each platform
within its own scale and palette. Exact depth ties still need a native
sprite-ID witness. Use the [attacker image cases](#attacker-image-depth)
for weapon attachments.

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
| 2 | `opaque-pick-depth` | Reuse the overlap mask for selection. A over opaque foreground pixels selects the visible eligible actor, and A on transparent pixels follows the ordinary fallback. Include exact ground Y ties, equal air layers and a lifted building. | Challenger Overlord/Hatchery reference and opt-in GBA results in draft [#587](https://github.com/ggu/starcraft-gba/pull/587). Native sprite-ID ties and broader poses need separate witnesses. |
| 2 | `same-anchor-sprite-id` | Capture same-layer, same-Y SCVs with distinguishable opaque pixels. The native sprite ID breaks the tie when both turret bits are clear. | Blood Bath observer step 4230 has stacked layer-4 SCVs at (1940, 293) with sprite IDs 0x858 and 0x857. The native key puts 0x858 in front. Pixel ownership remains open. |
| 2 | `burrow-sprite-depth` | In a supported encounter, capture a unit before burrow, at layer 1 underground, and after it returns to its base layer. Compare the body and the anchor used by any shot or death effect. | USA ROM writes layer 1 at `0x80156624` and restores the unit's base layer at `0x80156a30`. GBA campaign culling and native burrow presentation need their own paired test. |

For replay receipts record the product, source commit, controller tape hash,
world anchors, effective layers, active page, clip rows, overlap count and
wrong-pixel count. For native dynamic actors also record the actor and
sprite addresses and their captured layer. Keep native screenshots and
memory dumps local. Compare the isolated GBA masks within each version's
palette. The N64 and GBA use different pixel scales and palettes.

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
The projectile and its current visual children draw together. The
shooter's attached images draw in its actor pass. Independently spawned
shooter sprites, impact and trail sprites, spell pools and Interceptor
pulses need their own depth comparisons. The existing queue slot order
breaks GBA projectile ties. The experimental Mode 0 renderer still
separates cached static art from dynamic actors.

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

### Attacker image depth

The USA Siege Tank's ground attack uses `imgoluselo` (13) at source tick 1.
The opcode handler at `0x80012240` resolves the LO offset and passes the
current image and its owning sprite to the image attachment routine at
`0x80011b08`. The 116-byte handler at ROM offset `0x12e40` has SHA-256
`a2caafe03b35e6e3bdeef66d1efb9c0ee3b2cad0511193a1b9bae5eeac289b8e`.
The normal Tank uses `imgol` (8). Both flashes share their parent sprite's
depth. The retained defeat-suppressed Blood Bath observer at step 26877
shows the siege hull and turret sharing anchor (672,782) and elevation 4.
The turret flag places the turret ahead of the hull and other ordinary
sprites at that same Y.
The native observation establishes the sort and attachment rules. A
framebuffer witness for the active flash remains open.

The shared Mode 4 renderer draws attached attack images in the source
unit or building's existing queue turn and at its current world anchor.
An underlay goes before the body and an overlay after it. Attachments
follow the body's visibility and expire visually on source death or slot
reuse. These references are transient. The separately spawned sprite
events retain their existing pass and timeline. The GBA still uses a
composite Tank, so the native turret tie at exactly the same Y remains
an explicit case for future work.

The prepared Showcase native weapon fixture passes with ASan and UBSan.
The palette-remap oracle compares both paint orders and matches the full
battlefield frame. It finds 6 distinguishing pixels for the north Tank,
1 for the south Tank, and 2 for the Wraith. The upper and lower HUD edge
cases find 1 and 2, respectively, with no HUD changes. A hidden shooter
and a same-type replacement in its slot also pass. The fixture exercises
all 66 source profiles and keeps the released projectile cases.

The normal Terran, Z10, P10 and Showcase ARM products pass ROM header,
SRAM signature and RAM guards with the same prepared inputs. IWRAM stays
at its prior endpoints. EWRAM adds 128 bytes for the transient references
and part masks. The ARM build receipt SHA-256 is
`7d43d240da73ddb09da5df0416d20ca406887d619fcf5c2933c5e19595f00d09`.
An ordinary Showcase Siege Line replay matches all 24 samples of world
state, HUD pixels on both pages and palette over 120 advancing updates.
Three sampled battlefield frames differ. This run consumes 119,809,410
guest cycles versus 119,300,827 in the previously qualified control
(+0.43%). The replay receipt SHA-256 is
`527026608267a14c92778e9d0b5dc8389e3fecf1e6e2efab4a2e7d47dcf4345e`.

| Case ID | Replay setup | Expected result |
| --- | --- | --- |
| `attacker-muzzle-ground-and-air` | Issue an ordinary Tank or Siege Tank attack. Pause on the muzzle frame before release. Place a ground actor at distinct world Y just north, then south, and repeat with a Wraith. Use a fixed camera, both framebuffer pages and both actor creation orders. | The flash covers the north actor. The south actor and the Wraith cover the flash. Compare nonempty masks where the flash and blocker have distinct opaque pixels, excluding pixels owned by the shooter body alone. |
| `attacker-muzzle-live-owner` | Follow the firing source across a fog boundary and move the camera across both HUD bands. Lose the shooter during the effect and replace it in the same slot with the same type and position. Include an attacking structure. | The flash uses the source's current anchor. It disappears with a hidden, dead or replaced source. The independent bullet continues on its existing timeline and no actor image changes a HUD pixel. |
| `attacker-muzzle-turret-tie` | Pause on the native siege muzzle with a layer-4 ordinary sprite at the same world Y. Record the hull, turret and blocking sprite keys. | The native turret flag breaks the equal-Y tie. Track the GBA composite-art approximation separately from tests with distinct world Y. |

Run these cuts from campaign, Custom Game and Showcase. Record the ROM
commit, tape hash, world anchors, effective layer, page, clip rows,
overlap count and wrong-pixel count. Keep native captures local. Same-layer
sprite-ID ties and separate spawned sprite layers need their own witnesses.
An image still follows the coarse viewport admission of its parent. Add a
cut with an offscreen parent whose image extends into the field before
changing that admission rule.

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
