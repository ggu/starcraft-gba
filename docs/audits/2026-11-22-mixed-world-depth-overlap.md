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
| `detached-trail-low-elevation` | Stage a Vulture shot against a detectable burrowed target. Verify the parent elevation at release and each independent trail's birth anchor. For a low-elevation shot, place an actor between the parent and child in the sort, then north and south of the child's actual Y. Compare both paint orders on both pages. The native spawn opcode establishes child elevation = parent + 1. A native opaque Vulture overlap is still needed. |
| `native-pool-id-mixed-tie` | Stage a completed ground building and visible ground unit at equal layer and anchor Y. Capture both ID orders, last-image retirement, reuse and the A result. Keep native sprite ID and GBA kind/slot in separate receipt fields. A production-hidden actor gives a key witness only. |

Run the staged cases first using prepared profile inputs. Follow with
ordinary-input replays across Campaign, Custom Game and Showcase where
the actors are available. Record unavailable combinations explicitly.
Keep native and GBA states, binary images and captures in local evidence.
