# World depth at the camera edge and at exact ties

The USA N64 renderer drains unsigned keys from back to front. The key
contains the live sprite elevation at bit 27, the sprite's world Y at
bit 14 for elevations 0–4, the turret flag at bit 13, and the live
sprite pool ID in the low bits. The picker calls the same key function
before testing the foreground image. Horizontal row admission at ROM
offset `0xe405c` has no center-X test. The image painter clips the
visible art. [Native renderer and pool evidence](2026-11-19-world-depth.md)

The shared Mode 4 renderer already queues world actors and supported
effects by elevation and world Y. Completed building bodies still had
a second cull that dropped them when their anchors went past the old
envelope. The clipped renderer now admits the intersecting crop of a
completed building's sprite at those anchors. The same queued turn
paints the building at its world depth. The fog decision still samples
the building anchor. This applies to Campaign, Custom Game and Showcase.

## Replay cases

Use a fixed camera away from the map edges, with cursor and menus clear
of the overlap. Freeze actor pose and the palette for each comparison.
Capture both indexed pages. Compare a blank or single-actor frame, the
expected composition, its reverse, and the actual renderer. Require at
least one distinguishing pixel in the overlap and zero incorrect
pixels. Check every pixel in rows 0–11 and 120–159 against the blank
HUD bands. Compare each platform within its own scale and palette.

| Case ID | Setup and expected result | Current coverage |
| --- | --- | --- |
| `completed-building-edge-crop` | Stage a completed Command Center at screen anchors (-21, 66), (261, 66), (120, 1), and (120, 131). Place a Wraith at (4, 66), (235, 66), (120, 17), and (120, 111), respectively. The building crop remains visible where the Wraith does not cover it. The Wraith wins opaque overlap. With the building hostile and its anchor hidden, the frame matches the Wraith alone. Move a fully clipped building back through the same boundary to check entry and exit. | Terran sanitizer fixture covers the four staged cuts, both backbuffers and hostile fog. An ordinary Custom Game pan covers the left edge and fully clipped exit. Other ordinary edges and native overlap pixels remain open. |
| `same-air-layer-ignore-y` | Put two flyers at elevation 18 and different world Y. Sample an opaque overlap with the northern sprite assigned the greater tie key. Repeat with the southern sprite assigned the greater tie key. Keep anchors fixed and require foreground to change with the key. | GBA frame fixture uses a Wraith and Mutalisk and swaps unit slots. The native witness establishes the key sort. A native active-camera pixel pair and matching ID lifetime are open. |
| `pool-id-cross-kind-reuse` | Capture a ground building and a ground unit with the same layer and sprite Y. Require both sprites in the active camera and visibility mask. Test both pool-ID orderings and repeat after the last image of an actor retires and its ID is reused. For the turret cut, capture hull, building and turret as distinct sprite records at the same Y. Test ordinary selection at the opaque overlap. | The retained Terran 10 capture proves the key order hull → Barracks → turret. Those records lie outside that capture's camera and visibility mask. GBA ties still use kind and slot, and foreground selection is tracked in [draft #587](https://github.com/ggu/starcraft-gba/pull/587). |
| `mining-effect-world-depth` | Capture the SCV and Probe mineral working loop while an independent mining effect is active. Check that its layer 5 places it above ground actors and below flyers. Sample worker cargo on the return trip as an attachment in the worker's layer-4 turn. | Ordinary N64 Blood Bath step 75 has both mining sprite records. The GBA has a cargo badge in the worker's turn. The independent working effect and opaque comparisons are open. |

For ordinary replay, exercise the building case in a Terran Campaign
opening, Terran Custom Game on each race ROM, and a Showcase scene
where both actors are available. Repeat with a completed selected
building and its ring. Save the source commit, ROM and input tape hash,
scene, simulation tick, camera, active page, palette, actor and sprite
anchors, visible flags, effective depth keys, and overlap pixel counts.
For a native exact tie, log sprite IDs and the free-list head before
and after retirement. Keep ROM, SRAM, emulator states and frame captures
in local evidence storage.

The normal native allocator uses one FIFO for units, structures and
independently spawned effects. Retirement follows the last image.
The GBA's per-kind actor and effect slots do not provide that lifetime.
Verify pool reuse, effect retirement and ordinary save continuation
before changing the tie rule. [Native depth follow-up](2026-11-21-native-depth-followup.md#replay-cases)

## Recorded checks

The combined Terran framebuffer suite passed under ASan and UBSan. The
building case matched 90, 124, 150 and 123 distinguishing pixels per
page at the left, right, top and bottom edges. Those frames also contained
121, 112, 340 and 304 visible building pixels outside the Wraith's draw.
The aircraft case matched 71 and 121 distinguishing pixels per page in
the two slot orders. The combined log SHA-256 is
`9e4597932c90d91784f1b81227246728b56cd3d6c09ddc1d5dc27027fb1ffbab`.

In mGBA, ordinary Right input pans away from the starting Command Center
in a Terran Blood Bath Custom Game on all four products. Compared with
`60dd3d94`, each new ROM restores 90 and 17 visible pixels when the anchor
is at X −24 and −28. All changed pixels belong to the building crop. At
X −16, −20, −32 and −36 the displayed frames match exactly. All 24 samples
match in world state, camera and palette, with zero different HUD pixels.
Opening Campaign gameplay on all three race ROMs and the first Showcase
scene match in world state, palette and both framebuffer pages at frames
61 and 121. The Terran and Zerg replays dismiss dialogue through normal A
input before advancing the simulation.
Terran, Z10, P10 and Showcase pass ARM header, SRAM signature, payload
and RAM checks and retain their prior RAM endpoints.

| Receipt | SHA-256 |
| --- | --- |
| Terran camera | `851665f688460a9b0354420ae68c0a3307e99c234d700d35b7a5a159be4bb9a3` |
| Z10 camera | `6a3d06dde745146d1b1de5626e7740f165d8226cdbf2a7e18ed997f23b168775` |
| P10 camera | `7e7b764f00f65bf6cd2df100f15cac4a0e53e921260b3d61e941cc8a2fe04217` |
| Showcase camera | `ef250195f8449575836d9f81c6e929e91bb86843f2902a4150ac87610593b231` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| ARM link checks | `6d1ecb81c405a7989edbce4586bda33a8ffdca1a2963176928e02a619302bd00` |

In N64 Blood Bath step 75, SCV mining sprite ID 2105 is at (250,1957)
on layer 5, within the active render rows and matching the active
visibility mask. Probe mining sprite ID 2107 is at (1760,1960) on
layer 5, outside the player's visibility mask. The live SCV body key
`0x21e5084d` sorts behind the southern mineral key `0x21ec0866`.
They overlap at 284 opaque source pixels after applying the native
screen rectangles. The independent layer-5 mining sprite key
`0x28000839` sorts above both and intersects 20 opaque SCV pixels and
88 opaque mineral pixels. These counts come from sprite records and
source opacity. A composed N64 framebuffer comparison remains open.
The retained normalized RAM SHA-256 is
`54e8ca57b3fd01efca731b46d516eefc56cbcc139ed436995d20039121bde73e`.
