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

An ordinary Blood Bath step-75 replay shows the SCV and mineral on
layer 4, with the mining effect on layer 5. The mining image advanced
from frame 12 in the retained state to frame 16 in the displayed cut.
The active-camera capture confirms visibility and the ordering keys.
The count of 20 opaque pixels over the SCV and 88 over the mineral
comes from the source masks at frame 12. In the step-9053 state the ore
image is attached to each returning worker's layer-4 sprite. All 12
observed workers use the same rule across SCV, Drone and Probe.
[Retained mining key evidence](2026-11-21-world-depth-edge-and-ties.md#recorded-checks)

## Replay cases

| Case ID | Setup and expected result |
| --- | --- |
| `goliath-turret-same-y` | Enable an independently supplied raw-4 turret frame bank. Put the hull, an opaque ground blocker and the turret at equal world Y. Use a later-slot ordinary unit as the GBA blocker under the current tie rule. Require hull → blocker → turret. Repeat with the blocker one world pixel north and south. The southern blocker covers both hull and turret. Repeat in the N64 active camera with a blocker whose pool ID puts it between the hull and turret keys. |
| `goliath-turret-attack-and-edges` | Repeat at idle, during movement and through each attack window. Capture the turret pose and separately released projectile. Move across all four field edges, including a crop with only the turret visible. Repeat with a hostile Goliath visible and then hidden by fog. Match the native ordering separately from native attack pose parity. |
| `goliath-turret-legacy-bank` | Use the existing facing bank without raw 4. The previous hull-only composition renders once, with no queued turret. Compare the standing and moving body, clip, and both HUD bands. |
| `pool-id-cross-kind-reuse-active` | Use a layer-4 unit and building at the same sprite Y and on the visible plane. Log each allocator ID and key, then retire an actor's last image and spawn again. Require both pool-ID orderings at an opaque overlap. Repeat with two layer-18 flyers at different Y. Record the current GBA kind-and-slot result separately. The selection check belongs to the same mask once the active picker uses the painter key. |
| `worker-mining-effect-and-cargo` | Freeze the native effect while the layer-4 worker crosses a mineral edge and a flyer crosses the effect. The layer-5 working sprite covers ground art and stays behind the flyer. Use a second cut of the ore-bearing worker where overlap probes the attached overlay. The GBA comparison of a working sprite remains open. |

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
