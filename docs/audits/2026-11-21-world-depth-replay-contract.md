# World depth replay assertions

The [N64 render records](2026-11-21-native-depth-followup.md#native-sort-and-attachments)
sort by elevation, world anchor Y for elevations 0–4, turret flag, then live
sprite pool ID. The higher key draws in front. Attached images share their
sprite's turn and lifetime. Detached effects receive their own sprite
records. The GBA Mode 4 painter shares the elevation, Y and turret rules
across Campaign, Custom Game and Showcase. Its final tie still uses kind
and slot.

## Fixed elevation loss

The optional GBA VRAM death-effect pool stored the source-art bit and
owner color but omitted the dying actor's elevation. A load preserved
whatever happened to be in the destination's elevation field. The
renderer uses that field for attached collapse images, relative
detached sprites and fallback burst placement. A missing field could
put a collapse in a legacy draw pass outside the world queue.

The codec now stores the five elevation bits in the existing
appearance halfword. Zero still means the actor elevation is
unavailable. The row remains 20 bytes and lives in transient renderer
storage. The normal Terran, Z10, P10 and Showcase cartridges use the
EWRAM pool.

`make test-sc64-death-vfx-storage` checks all 32 field values, both
source-art bit values, the color boundaries and the first and last
valid slots. It reads into a row with the opposite elevation bits set.
The old codec fails that assertion. The fixed codec passes with
AddressSanitizer and UndefinedBehaviorSanitizer.

The same staged Showcase fixture then used both EWRAM and VRAM with
the shared world-depth queue active. Both produced the same 21
bank-backed overlap results, the same five fallback overlaps on each
page, full-pool and fog results, and the same Siege Line death and
expiration results.

The ordinary Showcase Siege Line ARM replay also matches across
storage. All 91 cuts match the world, effect rows, palette and both
indexed framebuffer pages. Ninety cuts contain populated effect
rows. The displayed-page bit differs at one capture boundary.
Over 451 updates, mean guest cycles rise from 887,949 to 897,025 in
the optional VRAM build. The normal four ARM ROMs match their
existing release files byte for byte.

The paired HOST log digest is
`ee0f95c9b1aa7689f971c07a7733065752d7f5630d28e475f81d9e813bb71336`.
The ARM comparison digest is
`a326f99c7d32f33ebc1059d1fb641644dad9453b0213c8ea5f46ced79817882d`.

## Replay cases

Use `depth_pair` as the replay assertion ID for two overlapping
sprites. Record one assertion per tested mode, case variant and
framebuffer page. Retain the source revision, ROM and input-tape
digests, camera, visibility, simulation tick, pose, palette and capture
phase. For each actor record its fixture label, sprite anchor,
elevation, effective Y, turret flag and tie ID. Identify which tie
scheme supplied the ID. Record the expected and observed foreground
and the number of distinguishing, incorrect and changed HUD pixels.

Use isolated draws and both composition orders. This gives an
order-sensitive mask even when an effect remaps the pixels behind
it. Require a nonempty mask and zero incorrect pixels on each indexed
page. Check the HUD rows 0–11 and 120–159 across flips. Record a
selection result only for a case that exercises picking. Mark
unavailable mode or storage combinations as unavailable.

| Case ID | Variants | Assertion |
| --- | --- | --- |
| `death-storage-depth` | Use the same scene and seed in EWRAM and a VRAM qualification build. Test an attached ground collapse with a Tank to its north and south, a burrowed collapse, an Overlord's attached images beside a Wraith and detached rubble. Add the fallback collapse and air burst cuts. | The loaded elevation, queue key, visible overlap and HUD match across storage. Confirm the VRAM flag and source death depth are both active. [Existing overlap setups](2026-11-21-fallback-world-depth.md#replay-cases) |
| `same-y-attached-death-continuity` | Capture an opaque equal-elevation, equal-Y overlap alive, on the first attached death image, during actor-slot reuse, and on last-image retirement. Repeat with the victim's ID above and below its neighbor's. Include an independently detached effect with its own key. | Preserve the dying actor's ordering through the last attached image. On GBA the lower-slot collapse stays behind the Marine on both pages. The reused live actor draws in front on an exact slot tie. Retain native sprite IDs and the free-list head around retirement and reuse. [Fix and native lifetime witness](2026-11-21-attached-death-continuity.md) |
| `worker-mining-effect-and-cargo` | Use an active SCV at a mineral edge and overlap its work effect with visible ground art. Move a Wraith across the effect on a separate cut. Repeat the ordinary gathering loop with a Probe and Drone. Capture approach, active work, loaded return, empty field, hidden worker or field, and each HUD boundary. Pause during work and resume from HOME. | The mineral and worker body sort at layer 4, the independent mining effect at layer 5, and the Wraith above both. Only active mineral work draws the effect. The returning ore badge shares the worker's turn. Require distinguishing masks for both overlap orders on both indexed pages. [Mining reference and fixture](#mineral-work-layer) |

The [native key witness](2026-11-21-world-depth-edge-and-ties.md#replay-cases)
establishes the native sort and allocator rule. The exact death-tie
case still needs an opaque N64 framebuffer cut. Use independent native
and GBA replays and keep ROMs, saves and captures local.

## Mineral work layer

The native Blood Bath step-75 observation records visible SCV mining
sprite ID 2105 on elevation 5. The SCV and mineral itself both use
elevation 4. The independent sprite intersects 20 opaque SCV pixels and
88 opaque mineral pixels in the retained source masks. An off-camera
Probe's working sprite also uses elevation 5. Returning ore is attached
to the worker in the step-9053 observation. [Native records](2026-11-21-shared-world-depth-regressions.md#n64-reference)

A fresh ares continuation of an ordinary Blood Bath checkpoint
reproduced the distinction at native update 62. The render rows contain
workers 2138 and 2135 at layer 4 and their nearby type-340 sprites 2124
and 2125 at layer 5. The four keys are `0x201ac85a`, `0x2032c857`,
`0x2800084c` and `0x2800084d`. The final read-only RDRAM digest is
`322c3ffbc3921c9b44ce55c813e29de4009f0d23dcebfa8e1db6b3e57e75c990`.
The retained framebuffer digest is
`f44e0051e4cb60715f36147b598b46f5984ef3722156624a861a987871fba0f8`.

The shared GBA Mode 4 painter uses an original four-pixel cue at the
worker–field midpoint while the mineral wait is active and within
gathering range. The work effect takes the heap entry released by the
worker body and is drawn at elevation 5. The effect uses the current
worker and field state, including after HOME load, and adds no saved
fields. Compare order within each platform's own framebuffer and
palette. GBA cue pixels and animation timing are authored for this
port. Exact native tie IDs and a composed native overlap mask remain
separate assertions.

Run `make test-sc64-mining-depth` with the normal Terran profile inputs
prepared. Use the regular Campaign, Custom Game and Showcase entry points
for replay wherever a gathering worker is available. For a screenshot
where a flyer fully covers the cue, retain a second ground-only cut so
the expected cue pixels are observable. Record worker slot, field slot and
remaining minerals, work timer, cargo kind, all anchors and the
effective depth keys. Keep native capture data local.

The sanitizer fixture distinguishes three ground pixels and one flyer
pixel on each indexed page. It checks fog and the inactive worker
states, and fails if the work effect's queue insertion is disabled.
An ordinary Terran-versus-Zerg Blood Bath replay on Z10 pans to the
active mineral line and captures frame 113 while an SCV has 32 work
updates left. Both pages change the four cue pixels at X 79–81 and
Y 54–56. The world, HUD and palette agree. Both versions write the same
32 KiB pause journal. A fresh HOME load matches the uninterrupted
world at six checkpoints over the next 24 updates. The paired replay
receipt is `be9c2ec5e6927d257909ca76f60b92b0b1f0b72d38f27b4b3cd7f358ae852ea0`.

Terran, Z10, P10 and Showcase retain their ARM RAM endpoints. Campaign
openings and Showcase Carrier Approach match the existing snapshots
through frame 121. The three race cartridges pass all 54 ordinary
Custom Game lobby, pause and fresh HOME continuations across both
maps, all human races and one to three CPU seats. The matrix receipt
is `197bffaae5d33bc6f0bffa3b60f692c8f9eae4250cc287a8dd92b391e64805de`.
A 3,600-update three-CPU Blood Bath replay matches sampled gameplay,
route counters and pause journal bytes. Mean emulated update cost
rises from 921,678 to 923,198 guest cycles (+0.16%). Physical
frame pacing remains open. The timing receipt is
`8c6020da970e135a34f6155daa6769ff35cd7b96bbe4567c3f573b30032aded2`.

## GBA same-Y reproducer

Stage a Zergling and a Marine on the same world anchor in the HOST
Showcase fixture. Both use elevation 4. Capture the two isolated draws
and the combined draw with the Zergling in the lower slot, then repeat
with it in the higher slot. Use a peer with a single draw turn so a
separately sorted turret cannot enter the overlap mask. After the
alive-to-dead scan, capture the Zergling's first attached collapse
window at an offset of five frames. Assert the attached-image flag,
elevation and anchor before comparing the images.

| Zergling slot | Foreground alive | Foreground after death | Distinguishing pixels per page |
| --- | --- | --- | --- |
| Lower | Marine | Collapse | 65 alive, 58 after death |
| Higher | Zergling | Collapse | 65 alive, 58 after death |

The two indexed pages agree. The lower-slot case reproduces
a visible order flip in the preceding painter, independent of an anchor
or elevation change. The baseline HOST sanitizer observation digest is
`1d83afc2ce220cc95a6659f0816ddca8cff0998d8ec3854d9359bcb88fe9455c`.

The [shared renderer fix](2026-11-21-attached-death-continuity.md)
passes the two-page fixture, actor-slot reuse, effect-slot
replacement and renderer reset. Retain an ordinary native
opaque overlap through last-image retirement and ID reuse.

## Clean Blood Bath opportunity search

One retail N64 Blood Bath run used ordinary ares input, pause and save
copies. The observer read native rows from the copies. Each continuation
resumed the last verified checkpoint from the preceding segment.

| Native frame span | Samples | HP-zero or order-zero actors | Near-equal depth candidates | Receipt SHA-256 |
| --- | ---: | ---: | ---: | --- |
| 1,365–6,922 | 161 | 0 | 0 | `f6501f9fd83c69e2d17ec8506f12f874501284e8f786a7e2fed01ac2c01d167a` |
| 6,922–9,791 | 241 | 0 | 0 | `0ef0ebe98ee2b6031e585c2706fa4942839311d30728f5e51487e77909af9a51` |
| 9,791–12,866 | 241 | 0 | 0 | `2f236a5b8ae3f87996559a829f7cdb81f8410ec9cdb5cc3a586a4993009f7eae` |

The last span sampled every 4–30 frames. The human building still had
1,500 HP at frame 12,866. Actor rows can vanish during a morph, and the
sampling intervals can miss short-lived images. A save-state sample
also leaves active-camera opacity unobserved. Keep
`same-y-attached-death-continuity` pending. For the next capture, put
the camera on a visible fight and retain the adjacent render cuts from
the last live pose through attached collapse and final image retirement.
Record actor and sprite IDs at every cut and keep the same camera,
palette and viewport for the overlap mask.
