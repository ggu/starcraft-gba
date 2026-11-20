# Native world depth follow-up

These cases extend the [world-depth framebuffer harness](2026-11-19-world-depth.md#harness-cases).
Run them against the shared Mode 4 renderer in campaign, Custom Game and
Showcase wherever the actors are available. Keep emulator state and captured
images local.

## Native sort and attachments

The owned USA N64 build forms an unsigned depth key from sprite elevation
at bit 27, world anchor Y at bit 14 for elevations 0–4, the turret flag at
bit 13, and the sprite pool ID in the low bits. Read the sprite anchor from
the render record. The game can position it independently of the unit.
The painter uses ascending depth keys. A higher key wins visible overlap.

| Native capture | World anchor and layer | Sprite pool ID, raw unit and flags | Expected draw order |
| --- | --- | --- | --- |
| Blood Bath step 26,877, normal Tank | (608, 560), layer 4 | 1820/raw 5/0x04 and 1756/raw 6/0x0c | Hull, then turret |
| Blood Bath step 26,877, sieged Tank | (576, 631), layer 4 | 1775/raw 30/0x00 and 1723/raw 31/0x08 | Hull, then turret |
| Terran 10 step 18,228, equal-Y Goliath and Barracks | Y 1808, all layer 4 | 1931/raw 3/0x00, 1930/raw 4/0x08 and 1951/raw 111/0x00 | Goliath hull, Barracks, Goliath turret |

The Blood Bath native state SHA-256 is
`e49a3f399cf57a50f6fb25543dc35bfc456566a3009a401ccce78e56a26ca540`.
Its normalized RDRAM SHA-256 is
`c564005d460558a307f550955a5ffd86dc88fc53cfb1fa4e6ff6b167ce6e6134`.
Terran 10 native state SHA-256 is
`89e8cb1a135076946f405c374b55a98e753fe986fb4556a4e6822899862fb668`.
Its normalized RDRAM SHA-256 is
`377c125a2b03b79b201bd1c635811121512ad4897a8b8473246cfaf87050ae44`.
These records witness the native key and attachments. A native framebuffer
cut with a nonempty opaque overlap between all three sprites remains open.

The source weapon bank resolves the normal Tank's attack to raw turret 6,
image 251 and attached flash 536. The sieged attack resolves to raw turret
31, image 254 and attached flash 537. Both flash images belong after the
turret body in its draw turn. Native opcode 8 at `0x80012074` reads the
owning sprite from the active image and calls the common attachment
routine. Opcode 13 at `0x80012240` follows that rule with a source offset.
The shared painter gives the Tank hull and
turret separate turns in both modes. The turret reuses the capacity vacated
by the hull. The world queue still uses GBA kind and slot for exact ties
after the turret flag. Goliath still needs independent raw 4 turret art.

## Missile trails

Native IScript opcode 15 reads the parent elevation at `0x800124d0`
and adds one at `0x800124dc`. The spawn wrapper at `0x80011cf4`
allocates its own sprite. The opcode window at ROM offset `0x130d0`
has SHA-256
`cb7451368de2aa76f64b68fc3c6ed62bb9e15341b7d805158ed6cba526a27710`.
The Goliath missile in the Terran 10 state has anchor (3213, 3382).
Its confirmed child anchors are (3213, 3361) and (3212, 3367).

Supported Wraith, Goliath and Vulture projectiles paint their detached
flight trails one elevation above the released bullet. Same-trace
children on high layers share one heap entry, with their separately
authored birth positions and lifetimes. Attached images stay in the
parent turn. Per-child ties against other sprites at the same depth,
ground-level children, and children that carry into later phases
remain separate parity cases.

## Replay cases

Freeze the camera, palette, unit pose and effect tick. Capture an empty frame,
the isolated sprites, the expected and reversed compositions, and the
shared renderer. For every case that asserts draw order, require a nonempty
mask of pixels where the expected and reversed compositions differ. Compare
both indexed pages, and check that both HUD bands match the empty frame.

| Case ID | Setup and expected foreground | Evidence |
| --- | --- | --- |
| `tank-turret-same-y` | Put a normal Tank and an ordinary layer-4 blocker at the same world Y. Arrange slots so the hull sorts before the blocker. The Tank turret covers both where they overlap. Repeat in siege mode and over the full camera and page flip. | Native key witness. Terran HOST compares both compositions. |
| `tank-turret-next-y` | Move the blocker one GBA world pixel south. Its higher Y covers hull and turret. Move it north to exercise the inverse. Keep layers fixed. | Terran HOST south cut. North turret and native opaque cuts pending. |
| `tank-muzzle-turret-same-y` | Use the normal or sieged muzzle's live image tick and a same-row ordinary blocker. The hull precedes the blocker and the turret and attached flash follow it. Compare both compositions because the siege flash remaps existing colors. | Native owner and layer witness. Siege HOST case, with the normal active cut and native pixels pending. |
| `goliath-turret-same-y` | Start from the native Goliath/Barracks key witness above. Require opaque overlap and both independently visible turret and hull pixels before comparing. | Native keys captured. GBA turret art and matching renderer case pending. |
| `pool-id-exact-tie` | Use two SCVs with equal layer and Y, then two Wraiths with equal air layer and different Y. Record the real pool IDs and at least one destruction and reuse. Use the same sample for foreground picking. | Native keys captured. A shared GBA sprite-ID lifetime and exact pixel witnesses remain open. |
| `aircraft-crop-past-anchor` | Place Wraith and Carrier anchors at screen (-9, 66), (248, 66), (120, 9), and (120, 126), with camera clear of map boundaries. Draw visible cropped art in depth order and check the Wraith over the Carrier at an opaque overlap. Repeat hostile in fog, and suppress an actor wholly outside the playfield. | N64 horizontal row admission at ROM 0xe405c selects visible sprites without a center-X test. A separate native vertical and opaque edge cut is pending. |
| `moving-missile-sprite-child` | Capture the Goliath/Wraith missile or Vulture grenade while a trail sprite is live. Put a layer-5 point Storm on the child and a higher-layer flyer across it. The child covers Storm and the flyer covers both. Trace independent fixed birth anchors through flight. | Native Terran 10 key witness has parent sprite 345/image 529, ID 528, layer 5, and two live child sprites 309/image 422, IDs 527/526, layer 6. Showcase HOST passes the Storm, Wraith and HUD comparisons for all three projectiles. Ordinary-input and native overlap pixels remain open. |

Record per cut the source, ROM and input digests, mode, camera, animation
tick, actor and effect anchors, effective keys, native sprite IDs when
available, active page, and distinguishing and incorrect pixel counts.
Use normal controller input for a second pass after staged overlap checks.
Compare each platform at its own scale and palette.
