# World depth and foreground selection

The USA N64 game draws lower elevations first. At elevations 0–4 it
then sorts by the sprite's world Y. The turret flag and the live sprite
pool index resolve remaining ties. Its picker uses the same depth key.
In the retained Challenger replay, an ordinary A press at step 4,345
selected an Overlord where its opaque body covered the Hatchery. The
cursor was closer to the Hatchery's ring center. [Native depth evidence](2026-11-19-world-depth.md)

The shared Mode 4 picker now compares the same unsigned keys as the GBA
painter where both the nearest eligible friendly unit and building have
opaque body pixels under the cursor. This covers supported standing
ground units and flyers against finished buildings. Other poses,
transparent pixels and single-body hits keep proximity selection.
The picker reads the art and leaves the animation state intact. Campaign,
Custom Game and Showcase share the code. The renderer and the picker
both use GBA kind and slot at exact ties.

## Harness cases

Use the existing [`depth_pair` assertion](2026-11-21-world-depth-replay-contract.md#replay-cases)
with a cursor and selection result for each selection case. Hold camera,
palette and simulation phase constant for the isolated actor masks and
the two composition orders. Count pixels that distinguish the orders.
Record both indexed framebuffer pages, with each HUD band compared
against the baseline. In the ordinary controller replay, record the
cursor's world and screen coordinates, the selected actors before and
after A, and the first frame on which the change becomes visible. Keep
the ROMs, controller tapes, SRAM and framebuffer cuts in local evidence.

| Priority | Case ID | Setup and expected result |
| --- | --- | --- |
| P1 | `scout-air-missile-trail-depth` | Fire a Scout at a Dropship and capture the missile and its live trail independently. The missile uses layer 17 and the trail uses layer 18. Overlap a Wraith with the visible trail. Both GBA pages must match the composition with the trail in its separate turn. Capture the native sprite IDs to resolve the layer-18 tie. Also sample the trail's first visible tick and expiration. |
| P1 | `pick-flyer-over-ground-building` | Launch Outpost as Zerg against one Terran from HOME. At world (160,160), A selects the isolated Hatchery. Clear selection with Select. Move Left for ten updates and Down for one to (140,162). Confirm opaque Overlord and Hatchery pixels at the cursor for both hover poses. A selects the foreground Overlord. Repeat in all four ROMs and use the retained N64 Challenger overlap as the native witness. |
| P1 | `pick-lifted-building-world-depth` | With an opaque overlap, compare a Marine and landed Science Facility, then repeat mid-flight and after landing. The layer-12 flying building covers the Marine and sits behind a Wraith. Compare north and south ground anchors at layer 4. Repeat after fresh HOME load wherever flight is persisted. |
| P1 | `pick-transparent-ownership-and-clip` | Sample an opaque overlap, each single-body mask and transparent corners. Repeat outside each field edge, at hostile or neutral ownership, hidden by fog, and with construction or an unsupported animated pose. Check proximity and command handling at the same coordinates and an isolated contextual mineral click. |
| P2 | `pick-same-kind-and-turret` | Overlap same-layer flyers with unequal Y and different IDs. Record both ID orderings. At layer 4, stage a normal or sieged Tank hull and turret with an ordinary actor at the same Y and one pixel north and south. Capture the foreground selection on the hull, turret and transparent pixels. GBA same-kind and independent turret picking remain open. |
| P2 | `pool-id-cross-kind-reuse-active` | Stage an ordinary layer-4 unit and a building at equal sprite Y in the active camera. Test both ID orderings, last-image retirement, and FIFO reuse with an opaque mask and A result. Record the free-list head before and after retirement. Repeat with an independently spawned effect at the same elevation. Keep the native ID outcome and current GBA kind-and-slot outcome explicit. |

For native observations keep the elevation, effective Y, turret flag,
allocator ID, attached image IDs and visible mask from the actual sprite
record. In the GBA fixture retain the actor kind, slot, current flight or
burrow state, and painter key. Distinguish an ordinary active-camera
witness from an off-camera key or a staged framebuffer fixture. Use the
[shared cases](2026-11-21-shared-world-depth-regressions.md#replay-cases)
for independent attacks, attached death images and mining effects.

## Scout missile trail

Scout air weapon 74 launches image 521. Its native flight script uses
opcode 15 at source tick 3 to create separate sprite 373 with image 443.
The common N64 opcode handler uses the parent elevation plus one. The
[native Goliath missile record](2026-11-21-native-depth-followup.md#missile-trails)
shows the same opcode's layer-5 parent and layer-6 children. The Scout
trail now enters the GBA's shared world queue at the child elevation.
The flight animation still determines each trail piece's birth anchor
and age. The group's same-layer tie follows the GBA queue convention.

The Showcase sanitizer fixture emits weapon 74 through the normal Scout
weapon helper and targets a layer-16 Dropship. At source age 4, an
opaque Wraith pixel distinguishes the layer-18 trail's turn from the
layer-17 missile's turn on both indexed pages. The previous renderer
fails the assertion and the updated renderer passes. The native
Scout-specific framebuffer overlap is pending. A fresh ares continuation
from a diagnostic Zerg 9 state observed seven Scouts in the render
records, with no weapon 74 missile or trail in its 61 samples.
The diagnostic's input provenance and sampling interval limit the
conclusions from that run.

## Qualification

Ordinary input in mGBA reproduced the Hatchery-to-Overlord selection
change in Terran, Z10, P10 and Showcase. The isolated Hatchery click
selected the building in all eight control and candidate runs. At the
overlap click the four pairs match in world state, palette, active page
and both indexed framebuffer pages immediately before A. The candidate
selects the Overlord at the cursor where the control selected the
Hatchery. Fresh N64 Wasteland selection attempts reached the field and
sampled A input outside the visible overlap. The retained Challenger
observation supplies the native selection witness.

All four standard ARM links passed and kept their IWRAM and EWRAM
endpoints. Campaign openings on the race ROMs and the Showcase opening
match the control at frames 61 and 121. A three-CPU Blood Bath Z10
replay matches gameplay, palette, both indexed pages and the pause
journal. Over 3,600 updates the mean rises from 871,702 to 876,142 guest
cycles (+0.51%). Physical frame pacing remains open.
Across the three race ROMs, all 54 Custom Game cases resume from a fresh
HOME process and match the control's saved and continued world fingerprints.

| Local receipt | SHA-256 |
| --- | --- |
| Terran foreground fixture under ASan and UBSan | `58eb6b98d06f1b27a129712d193b70d8a7029a1fcab87ac7d217810bb77acde4` |
| Showcase weapon fixture under ASan and UBSan | `0ad0795055bc8cb347457fc8b034fb0821d2f88453c33768be7f3f9d8205e98f` |
| Ordinary active overlap click on all four products | `706227137b1f11209091e918e60165f06cf28ae893f287b7b743921b0ecfea3c` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Busy Blood Bath and fresh continuation | `5077512a5527c7c78c61b0c67645bdfdb00bba71126383a1064d48ab3e1760fa` |
| Custom Game pause and fresh HOME matrix | `7a22d76c772d2b5c214844d6bb44e25e05101de8a7e4742207c53b739ce89afb` |
| N64 Scout diagnostic samples | `6274f9d6b2aa9cae2ac53eb00d97ebb085d0bbeddc2297abbe21611b8dd8f35a` |
