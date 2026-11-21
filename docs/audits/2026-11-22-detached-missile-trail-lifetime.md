# World depth when a missile reaches impact

The USA N64 missile script creates independent trail sprites one layer
above their projectile, each at its own birth anchor.
[Native spawn rule](2026-11-21-native-depth-followup.md#missile-trails)

An isolated ares continuation of the retained Terran 10 state followed
Goliath missile sprite 528 at elevation 5. At frame 18,242 its anchor is
(3213, 3601). At frame 18,243 it stops at (3213, 3616) and adds its
impact image. Independent trail sprite 507 has a live image at its
fixed anchor (3213, 3601), elevation 6. At frame 18,244 the original
missile image has retired. The trail image remains live at the same
anchor through the final sample at frame 18,249. Trail records 517 and
519 are also live after the image transition. Use the sample ordinal
where the native frame counter repeats.

The input state SHA-256 is
`89e8cb1a135076946f405c374b55a98e753fe986fb4556a4e6822899862fb668`.
The ordered sprite record SHA-256 is
`8f39962b2a14b26d2895f338e40e0599bf97c842d13680dfa5bd5f939188cd49`.
The sample establishes image lifetime, birth anchors and elevation.
An active-camera N64 framebuffer overlap and damage timing are separate
checks.

The GBA renderer stopped sampling flight trails when the impact
animation started. The shared painter now stops new births at the last
flight tick and samples existing children at their own ages. Supported
Vulture, Wraith, Goliath and Scout trails keep their birth anchors and
parent-plus-one depth until they expire. Low trails keep their independent
world-Y turns. The prepared child lifetimes fit within the existing
tracer lifetime for these four missiles. The change adds no saved state
or persistent render storage.

## Replay harness

Record these cases under the
[`depth_pair` contract](2026-11-21-world-depth-replay-contract.md#replay-cases).
Use both indexed pages and compare both possible draw orders with
the observed composition. Record live native sprite and image IDs,
projectile phase, trail age, birth anchor, both depth keys, camera and
visibility. Keep ROMs, captures and saves local.

| Case ID | Required cuts and expected result |
| --- | --- |
| `missile-trail-after-impact-ground-y` | Stage a short Vulture shot at a detectably burrowed enemy. Preserve the launch layer when the target disappears. On the first impact tick overlap its layer-3 trail with a Bunker one world pixel north, then one south. The trail covers the north Bunker. The south Bunker covers the trail. Compare the last live child tick and its first expired tick, check that no new child appears at the impact anchor, and verify both HUD bands. |
| `missile-trail-after-impact-high-layer` | Continue the native Terran 10 Goliath tape across frames 18,242–18,249. Track IDs 528 and 507, and compare against a fixed-camera GBA Goliath shot with a layer-5 Storm and a higher flyer crossing the trail. Repeat for Wraith and Scout. Keep native live-record evidence separate from any later opaque overlap witness. |

Run the staged GBA cases with the prepared Showcase weapon fixture.
Follow with ordinary input in Campaign, Custom Game and Showcase wherever
the encounter is available. For exact same-elevation and same-Y ties, use
the [native sprite lifetime cases](2026-11-21-world-depth-replay-contract.md#native-pool-id-and-pixel-evidence).
The GBA still resolves exact ties by actor kind and slot.
