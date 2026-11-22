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

## Native sprite retirement follow-up

The allocator at `0x8011fde8` removes from the linked free-list head at
`0x80112890`. The retirement path at `0x80120840` appends to the tail
at `0x80112894` after the image list clears. A 41-cut ares continuation
reads those globals at frame 17,168 with head sprite 368 and tail 2138.
At frame 17,369 the head is sprite 125 and the tail is 2139. Sprite
2138 remains free. Its previous-link path to the head shrinks from
2,008 to 1,772 records. This explains why a short continuation is
unlikely to observe reuse of that particular ID.

The recorded snapshots do not establish intervening allocation
events or composed pixel ownership. For `native-pool-id-cross-kind-reuse`,
select an active battle that keeps allocating, sample both globals
and image-list ownership at each render update, and keep the opaque
overlap and selection assertions independent.

The local receipt SHA-256 is
`784458de755ca00bc5608e57a3217ce2fbb4ea592ffbdefde586ac8f4aa6391a`.
The sampled records SHA-256 is
`6e45b3f8360f33bda890cd9223a21231d575d0f00bc3dc2b0f375c98abe4e8d7`.

## GBA qualification

The Terran sanitizer fixture checks real opaque art for a two-unit ground
overlap and a three-flyer overlap on both indexed pages. It also checks
that a hostile foreground body is ineligible and that an unsupported
foreground attack pose retains proximity. The Showcase weapon fixture
compares both Tank impact draw orders. The fallback Siege impact has 14
distinguishing pixels in each of the three blocker cases on each page
and leaves both HUD bands unchanged. The separate Tank-only renderer
passes the legacy raw-LUT and fog oracle.

All four normal ARM products pass ROM and RAM checks. The ordinary
Campaign and Showcase openings match the preceding release at frames
61 and 121. All four products match at 41 Custom Game camera cuts each.
The race ROMs pass 54 ordinary Custom Game launches and fresh HOME
continuations across both maps, all human races and one to three CPU
seats.

In a 3,600-update Z10 three-CPU Blood Bath continuation, all 31 world
and video checkpoints, 16 fresh continuation cuts and the SRAM bytes
match the preceding release. Mean guest cycles change from 825,582 to
825,087 and p95 from 1,115,886 to 1,115,670. This covers a busy ordinary
match. Measure the cost of a sustained opaque overlap under the cursor
and physical GBA pacing separately.

The ordinary Zerg 7 input tape reaches victory at frame 228,128 after
231,031 controller updates, including a fresh HOME continuation.
All 254 world observations, ten indexed display cuts and six route
cache cuts match the preceding release. The pause, victory and fresh
victory readback journals also match byte for byte. The three timed
combat windows average 0.03–0.05% fewer guest cycles, with small peak
increases of at most 0.10%.

| Receipt | SHA-256 |
| --- | --- |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Custom Game camera cuts | `356b3078e1ebb4c9386f0bbd2de45b4c4bac8388fc8e967a7f77a47a1b3a1d59` |
| Custom Game SRAM matrix | `201234294efee5100618685f13bd790d1975415b59cba7e21f27ef11a83d589b` |
| Busy Blood Bath timing and continuation | `36beb92a437b84f53fab6566b61c28ea08b0ef54ff229a23123cc346a695bb10` |
| Zerg 7 pause, fresh replay and victory | `526e1e70607ba800b99b097567d2af8e5e4d6b30430030193169151841a1fa3c` |
