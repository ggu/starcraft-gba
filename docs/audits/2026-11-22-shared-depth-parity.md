# Shared world depth parity

The USA N64 renderer and picker use the same unsigned depth key.
Elevation sorts first. Sprite anchor Y sorts next at elevations 0–4.
The turret flag and then the live sprite pool ID break ties. A higher
key draws later. Attached images stay in the parent's turn. An independent
effect has its own key. [Native sort and attachments](2026-11-21-native-depth-followup.md#native-sort-and-attachments)

The shared Mode 4 picker now checks the painter key across nearby units
when the nearest eligible unit has a verified opaque body pixel at the
cursor. A higher opaque body can win on the ground or in flight, including
when three or more units are in reach. An unverified pose above the best
verified mask keeps proximity as the fallback. The existing unit/building
path handles overlaps between those two kinds. The GBA uses actor kind
and slot for exact ties.

The separate prepared Tank effect bank supplies a fallback when an attack
is missing from the main weapon bank. Its two parts used to draw together
after the world queue. Newly fired fallback rounds now draw the muzzle in
the live Tank's turn, including on its separately queued turret when
available. The stationary impact draws at the captured target position
and one layer above the target's launch-time elevation, matching the
native shell rule. The impact can outlive both actors. Older saved
tracers without the needed depth data use the legacy composition.
[Native bullet depth](2026-11-19-world-depth.md#native-bullet-depth)

## Replay cases

Use the [depth_pair assertion](2026-11-21-world-depth-replay-contract.md#replay-cases)
on both indexed pages. Freeze camera, visibility, animation tick and palette
for the isolated and composed cuts. Record native sprite anchors and IDs
where available, GBA actor slots and keys, the expected and observed
winner, distinguishing pixel count and both HUD band hashes. Keep ROMs,
saves and images local.

| Case ID | Required cuts and expected result |
| --- | --- |
| `opaque-ground-pick-by-y` | Overlap a standing Hydralisk north of a Dragoon at layer 4. Place the cursor on a distinguishing opaque pixel closer to the Hydralisk. The southern Dragoon wins the painted pixel and A. Repeat with reversed slot order, a transparent pixel, a moving ground pose without a verified mask, and a hostile or hidden foreground unit. |
| `opaque-three-flyer-pick` | Overlap an Overlord, Mutalisk and Wraith on the same cursor pixel. Put the Wraith at greater depth than both, away from the nearest anchor. The visible Wraith wins the pixel and A. Repeat with an unsupported foreground pose and the Wraith outside the eligibility set. Include the next A edge to check the double-tap window. |
| `fallback-tank-split-depth` | Use the prepared fallback Tank bank for a shot at a layer-3 Bunker. Sample the muzzle in the turret turn, then the impact at layer 4 with a ground blocker north and south and with a layer-18 Wraith. The impact covers the northern blocker. The other two cover the impact. Compare both compositions because the splash remaps existing colors. Retire or replace the shooter and target. The muzzle retires with the shooter and the impact keeps its captured Y and elevation. Include a legacy bank and a saved tracer lacking capture metadata. |
| `lurker-burrow-keeps-sprite` | Use the ordinary native Blood Bath cuts below. Check that each Lurker keeps its sprite ID and anchor when it drops from layer 4 to layer 1. Capture an in-camera ground overlap before and after the change. On GBA use a supported encounter with a retained burrow flag and verify the layer independently of concealment. |
| `native-pool-id-cross-kind-reuse` | Capture a visible equal-Y building/unit pair in both sprite-ID orders. Capture the last attached image, the FIFO retirement and a later reuse. Use the same distinguishing pixels to check A selection. Keep the current GBA kind/slot key in the receipt until a shared sprite lifecycle is qualified. |

Run the shared checks in Campaign, Custom Game and Showcase wherever the
actors and controls are available. Use an ordinary controller replay for
each supported case after the staged comparisons.

## Native Blood Bath burrow witness

A fresh ares continuation of an ordinary Blood Bath game captured three
CPU Lurkers after morph. Slot 118's sprite 1783 keeps anchor (324, 1759).
Between updates 11,283 and 11,296 its layer drops from 4 to 1 and its key
changes from `0x21b7c6f7` to `0x09b7c6f7`. Slots 119 and 120 make the same
transition by update 11,310, retaining their sprite IDs 1784 and 1786.
Flags were `0x02` at these cuts. The actors were outside the active camera,
so the capture establishes the transition and identity. The exact visible
tick and foreground pixels still need an active-camera cut.

The local receipt SHA-256 is
`26f5bfaf3a0122c8037055e3125ed8707c3ec2766be846ff4022761ac4f1f0cd`.
The raw sprite record log SHA-256 is
`d214fcd86b135027a90ea572216919faddd0ba9eb445114e5edf7105b83c26c7`.
