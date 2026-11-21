# Shared world depth regression cases

Run these cases through the shared Mode 4 painter in Campaign, Custom
Game and Showcase wherever the actors are available. Keep emulator
states, generated sprite banks and captures in local evidence storage.

## N64 reference

The [native key](2026-11-19-world-depth.md) sorts by elevation, then
world anchor Y at elevations 0–4, then the turret flag and the shared
sprite pool ID. Higher keys paint later. Images attached to a sprite
share its draw turn.

The [retained Terran 10 step 18,228](2026-11-21-native-depth-followup.md#native-sort-and-attachments)
places all three sprites at layer 4 and Y 1808. The Goliath hull, Barracks
and Goliath turret keys are `0x21c4078b`, `0x21c4079f` and
`0x21c4278a`. Those records are outside the active camera. The separate
turret image is 235. It uses frames 119–135 at rest. Goliath ground fire
alternates its own turret poses, and the air missile has a separate
sprite key.

In the N64 Death scripts, `imgol` passes the emitting image's sprite
pointer to the attachment routine. The routine inserts the new image
after its parent in the painter's traversal. In the Overlord Death
sequence, image 58 begins at source age 2 and remains visible after its
emitter ends at age 3. The existing bank has eight such nested layers.
They now share the original death sprite's turn. Independently spawned
rubble keeps its own key and lifetime.

An ordinary Blood Bath step-75 replay shows the SCV and mineral on
layer 4, with the mining effect on layer 5. The mining image advanced
from frame 12 in the retained state to frame 16 in the displayed cut.
The active-camera capture confirms visibility and the ordering keys.
The count of 20 opaque pixels over the SCV and 88 over the mineral
comes from the source masks at frame 12. In the step-9053 state the ore
image is attached to each returning worker's layer-4 sprite. All 12
observed workers use the same rule across SCV, Drone and Probe.
[Retained mining key evidence](2026-11-21-world-depth-edge-and-ties.md#recorded-checks)
The local replay receipt SHA-256 is
`2291cf1ae4c37d97a8046937f0ec16ce1e56251ead498ad9450f3f6b04ad346b`.

## Replay cases

| Case ID | Setup and expected result |
| --- | --- |
| `goliath-turret-same-y` | Enable an independently supplied raw-4 turret frame bank. Put the hull, an opaque ground blocker and the turret at equal world Y. Use a later-slot ordinary unit as the GBA blocker under the current tie rule. Require hull → blocker → turret. Repeat with the blocker one world pixel north and south. The southern blocker covers both hull and turret. Repeat in the N64 active camera with a blocker whose pool ID puts it between the hull and turret keys. |
| `goliath-turret-attack-and-edges` | Repeat at idle, during movement and through each attack window. Capture the turret pose and separately released projectile. Move across all four field edges, including a crop with only the turret visible. Repeat with a hostile Goliath visible and then hidden by fog. Match the native ordering separately from native attack pose parity. |
| `goliath-turret-legacy-bank` | Use the existing facing bank without raw 4. The previous hull-only composition renders once, with no queued turret. Compare the standing and moving body, clip, and both HUD bands. |
| `pool-id-cross-kind-reuse-active` | Use a layer-4 unit and building at the same sprite Y and on the visible plane. Log each allocator ID and key, then retire an actor's last image and spawn again. Require both pool-ID orderings at an opaque overlap. Repeat with two layer-18 flyers at different Y. Record the current GBA kind-and-slot result separately. The selection check belongs to the same mask once the active picker uses the painter key. |
| `worker-mining-effect-and-cargo` | Freeze the native effect while the layer-4 worker crosses a mineral edge and a flyer crosses the effect. The layer-5 working sprite covers ground art and stays behind the flyer. Use a second cut of the ore-bearing worker where overlap probes the attached overlay. The GBA comparison of a working sprite remains open. |
| `death-nested-image-inherits-sprite-depth` | At source ages 2 and 4, overlap the Overlord's nested image 58 with a Wraith and then a Tank. The nested image stays on elevation 16 even when its parent image has expired. It draws behind the Wraith and in front of the Tank. Repeat with the Overmind with Shell (raw 147) at age 69. Require one layer-4 turn for images 89 and 60, plus a separate layer-1 turn for detached rubble image 111. Include a sample at each HUD boundary and a last-image expiration cut. |

Freeze camera, palette and animation tick across each comparison. Capture
isolated parts and the expected and reversed compositions. Require a
nonempty distinguishing mask and zero incorrect pixels on both indexed
pages. Compare every pixel in HUD rows 0–11 and 120–159. Keep the
platforms at their own palette and scale.

For each receipt record source, ROM, state and tape digests, product,
mode, camera, simulation tick, visible flags, active page, sprite and
image IDs, anchors, effective keys, and pixel counts. Use ordinary input
for a second pass after staged overlaps. Include any build with body-only
art as a separate legacy-bank check. Native exact-tie parity requires a
shared sprite lifecycle covering effects, last-image retirement, ID
reuse, and save continuation.

## Local regression checks

With the usual Terran inputs prepared, run
`make test-sc64-terran-turret-depth` for the synthetic turret and
`make build/test-sc64-terran-full-frame && ./build/test-sc64-terran-full-frame`
for the normal bank. The synthetic fixture requires a distinguishing
overlap at both Y values and a visible turret-only crop on both pages.
Its original rectangle art exists only in the host fixture. Terran
production banks currently provide the Goliath body. The independent
turret's source attack poses and active-camera N64 pixels remain open.

Terran HOST passed the Tank and sieged Tank overlaps with the original
body-only Goliath bank. With an original synthetic turret fixture, it
also matched 149 same-row and 144 next-row distinguishing pixels on
each page, and both pages of a turret-only crop at X −9. Both linked
raw-3 hull and raw-4 turret frames are required to enable the split.
Showcase HOST passed the prepared weapon overlaps with that synthetic bank.

The new death fixture failed on the previous renderer and passed under
ASan and UBSan on Showcase. It checks 230 and 228 pixels under the
Wraith at source ages 2 and 4, 430 pixels over the Tank at age 4, and
227 pixels under the Wraith at Overmind source age 69. It also checks
that the Overmind uses two queue entries for three simultaneous images.

The four normal ARM links retained their IWRAM and EWRAM endpoints.
Paired ordinary mGBA runs matched world state, palette and both indexed
pages in all three Campaign openings, at Showcase Carrier Approach
startup and in both Custom Game maps on all four products. Zerg Last
Stand reached the staged defeat at frame 150 in both builds. Its 95
paired indexed page cuts match. That scene uses a different death
sequence from the Overmind fixture.

The normal P10 replay starts from a fresh process at campaign frame
36,761 and reaches victory at frame 46,057. All 68 world, palette and
page checkpoints match the release, and the resulting SRAM is identical.
The 9,489 controller updates use 0.018% fewer guest cycles. The replay
verifies campaign continuity. The staged overlap fixture establishes
foreground ownership where the two compositions differ.

| Receipt | SHA-256 |
| --- | --- |
| Terran synthetic turret Make target | `f31b06a253818f7a7bdb6b986ffcbc2dbd4dcfddd7f0dc0c8963665873027a7f` |
| Terran body-only facing fixture | `cc7f059bb87b573f103b15da6b732f0c57acad82250c1c8ec6e44a3ba94d7550` |
| ARM link checks | `a1a613570e608b079bc614c9870c301df2ebdf6e010a8ca790612ca169ac4197` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Both Custom Game maps | `88eff7eb1afe41180a9927f5896022509aae8432f4a98df68d6019942bf74ec1` |
| Zerg Last Stand | `9d3f4749f84cc8ccb85f7ba50ed88b26e76f9704ef2d6d9b8138463c5e210f30` |
| P10 fresh start to victory | `16f258a34e56a706bf2733e8f6a9ad1e8e55210c16815f95d736049de32122d3` |
