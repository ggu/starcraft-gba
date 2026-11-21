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
