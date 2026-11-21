# Mixed world-depth overlaps

The native USA N64 painter sorts by unsigned sprite elevation, world anchor
Y at elevations 0–4, turret flag, and live sprite pool ID. The picker
uses the same depth key. In the ordinary Challenger replay, A selected an
Overlord covering a Hatchery even though the cursor was closer to the
Hatchery. This establishes foreground selection in a mixed overlap.
[Native ordering and replay evidence](2026-11-21-world-depth-picking.md)

The GBA painter ordered the full scene, but its mixed picker compared
only the nearest unit with the nearest building. A farther unit could
cover both at the cursor and still be missed. The picker now walks the
eligible buildings and units together using the same key functions as
the painter. It preserves the proximity fallback for a transparent
initial hit or an unverified foreground pose. The shared path applies
in Campaign, Custom Game and Showcase. The save format is unchanged.

The N64 uses a shared sprite pool for exact ties. The native Wasteland
step 121,802 has a layer-4 Barracks (sprite 1078) and an unfinished Marine
(sprite 984) at the same anchor, (1184, 1808). The key puts the Barracks
later. The Marine has no draw-admission bit while in production, so this
cut only establishes the sprite order. The local state SHA-256 is
`0b13a0272b8f1dd18b4e782049542c8a76fd28d1dbeda528ca92e21c725c9ac4`.
The live GBA painter still uses actor kind and slot for exact ties.
Retain the [native FIFO lifetime case](2026-11-21-world-depth-edge-and-ties.md#replay-cases)
to qualify any tie-policy change.

## Detached trails at ground elevations

The [native spawn opcode](2026-11-21-native-depth-followup.md#missile-trails)
allocates a separate child at parent elevation plus one. The GBA drew
a low-elevation flight trail in its parent's turn. The shared painter
now gives each low trail its own key using the birth anchor that the
art renderer resolves. The parent and each subsequent child free one
heap entry for the next birth. The added cursor array is 128 bytes on
normal 32-tracer builds. At elevations above 4, world Y has no effect
and the existing group draw remains in use.

The prepared Showcase fixture emits a Vulture grenade at a burrow-flagged
Drone, with a layer-2 projectile and layer-3 trail. The layer-3 Bunker
overlap distinguishes 19 pixels on each indexed page. All 19 were wrong
on `7c463fab`. The candidate matches the expected order at all 19.
A second cut places two live trail anchors at Y 49 and Y 58 around the
Bunker's Y 54. Each grouped alternative changes 19 pixels. The sorted
painter matches the independent composition on both pages.

These are GBA staged emission tests. Native Goliath render records
establish the parent-plus-one rule. A native Vulture low-layer opaque
overlap and ordinary detected-burrow firing replay remain open. Same-Y
ties with unrelated native sprites and trail persistence into later
projectile phases have separate evidence requirements.

## Harness cases

Use the [depth_pair replay contract](2026-11-21-world-depth-replay-contract.md#replay-cases)
for each pair in a mixed scene. Hold camera, fog, palette, facing and
animation tick fixed. Render each opaque body in isolation, the scene
with the tested foreground actor removed, then the full composition.
Record both indexed pages and assert the order at a pixel that changes
when the actual foreground actor is removed. Record the HUD band hashes
and the raw A selection on the same cursor pixel.

| Case ID | Setup and expected result |
| --- | --- |
| `mixed-three-body-depth-pick` | Stage a Hatchery at (96, 56), Overlord at (96, 54) and Wraith at (96, 48). Ensure all three friendly bodies cover the cursor. The cursor must be closer to the Overlord than to the Wraith, with the Hatchery inside building-pick range. Layer 18 makes the Wraith win the painted pixel and A. Swap the two unit slots and repeat on the other framebuffer page. Change Wraith ownership and check that A now selects the eligible Overlord. |
| `mixed-pick-fallback` | Move the cursor to a transparent pixel on the nearest candidate and record proximity. Repeat with an unsupported higher attack pose, with the cursor hidden by fog and outside the playable display. A nearby friendly gas or mineral target should still use its contextual command. |
| `detached-trail-low-elevation` | Stage a Vulture shot against a detectable burrowed target. Verify the parent elevation at release and each independent trail's birth anchor. Place a layer-3 Bunker between the parent and child in the sort. Then place the Bunker between two simultaneous trails at their own birth rows. Compare north/Bunker/south against both grouped compositions on both pages. The native spawn opcode establishes child elevation = parent + 1. A native opaque Vulture overlap is still needed. |
| `native-pool-id-mixed-tie` | Stage a completed ground building and visible ground unit at equal layer and anchor Y. Capture both ID orders, last-image retirement, reuse and the A result. Keep native sprite ID and GBA kind/slot in separate receipt fields. A production-hidden actor gives a key witness only. |

Use the Terran frame and Showcase native-weapon sanitizer fixtures for
the staged cases with prepared inputs. Follow with
ordinary-input replays across Campaign, Custom Game and Showcase where
the actors are available. Record unavailable combinations explicitly.
Keep native and GBA states, binary images and captures in local evidence.
