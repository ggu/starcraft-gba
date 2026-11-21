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
| `same-y-attached-death-continuity` | Capture an opaque equal-elevation, equal-Y overlap alive, on the first attached death image, during actor-slot reuse, and on last-image retirement. Repeat with the victim's ID above and below its neighbor's. Include an independently detached effect with its own key. | On N64 the attached images keep the victim's sprite ID and key until its last image retires. Log both IDs and the free-list head around retirement and reuse. On current GBA a lower-slot unit is behind a higher-slot unit alive, then sorts in front solely because its attached death image changes draw kind. Keep this as an open GBA expectation until shared sprite identity has been qualified. |

The [native key witness](2026-11-21-world-depth-edge-and-ties.md#replay-cases)
establishes the native sort and allocator rule. The exact death-tie
case still needs an opaque N64 framebuffer cut. Use independent native
and GBA replays and keep ROMs, saves and captures local.
