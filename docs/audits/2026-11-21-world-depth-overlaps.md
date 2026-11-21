# Selected building rings and attack sprites

This adds cases to the [world depth regression harness](2026-11-19-world-depth.md).
Run the cases against the shared Mode 4 renderer. Keep native ROMs, state
captures, and images in the local evidence directory.

## Selected building ring

The owned USA ROM draws a sprite's images from the tail at sprite offset
`0x20` to the head, following each image's link at offset `0x00`. The loop
starts at `0x80120314` and calls the image painter at `0x8012033c`.
The 60 bytes at ROM offset `0xe4004` match the captured native RAM and have
SHA-256 `a31fb2e565998f0194acdc63960b949c3d9a748248dc398fdae18c545757c0bd`.
The USA ROM SHA-256 is
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`.

In the retained `alerts/t10-comsat2.bs1`, the selected Command Center at
(3328, 3632) has native sprite ID 1818 and live elevation 4. Its image
records point back to the same sprite, `0x80217c18`, and draw in this order.

| Image | Role | Modifier | Native image address |
| --- | --- | ---: | --- |
| 277 | Shadow | 10 | `0x801f20c8` |
| 569 | Selection ring | 13 | `0x802066b8` |
| 275 | Command Center body | 0 | `0x801f2104` |
| 560 | Vitals | 11 | `0x80205584` |

The save state SHA-256 is
`fe0c6642270d5982c5afa02b3a28ef2c9de2f5c0dd71f4486ac139f103fc6239`.
Its normalized RDRAM SHA-256 is
`2b03a299d535a4c01ba26747fcbc759ec073175484f3b23f0779bea3b6e8c6a8`.
The ordinary Challenger screenshot `04-11-37.png` shows the foreground
feet of a selected Nexus interrupting the green near arc. Its vitals remain
in front. The screenshot SHA-256 is
`fce4c3d227512560cb2f76d1714dbca03ad02a37b3fa560ecccf5c8f21c03a1e`.
The near and far arcs both belong before the building art in the shared
building draw turn. Vitals stay after the art.

## Detached attack sprite

The retained Blood Bath `keep-019444.bs1` has a Hydralisk (actor 397,
raw unit 38) at (802, 1144), elevation 4, sprite ID 1962 and body image
29. Its attack uses independent sprites of type 332 with image 35.
The two live records (IDs 1641 and 1634) have elevation 5 and anchor
(795, 1138). The normalized RDRAM
SHA-256 is
`0924c9a329824371d7271968eb0b16ca1eaccceae76b54c44ca66a741866bac1`.
This is a retained defeat-suppressed observer cut. It witnesses distinct
sprite records, anchors, and elevations. The attack animation's
attached images still share their owner's draw turn.

The shared GBA painter uses a separate draw turn for the supported single,
nonrepeating `sprol` and `sproluselo` attack events. The event takes the
owner's live layer plus one and snapshots its position at birth. Source
offsets place the new sprite at its own world anchor. The tests must
distinguish this event from the released weapon projectile and any
image that remains attached to the shooter. The prepared attack bank
uses this path for the Hydralisk, Hunter Killer, Drone, and Scourge.

## Replay cases

Use fixed camera, palette, and animation tick at each overlap. Capture
an empty frame, the isolated components, the expected composition, the
reversed composition, and the shared renderer result. Require a nonempty
mask where the two compositions produce different pixels. This rule also
works for remapping effects that depend on the underlying palette index.
Every masked pixel must match the expected composition. At each HUD edge,
also compare the complete HUD bands on both indexed pages.

| Case ID | Required cuts and expected result |
| --- | --- |
| `building-ring-own-art` | Select a completed Terran structure and a Protoss Nexus. Capture the near and far arcs and body at one camera. Sample at least one opaque body pixel that the near arc would cover in the reversed composition, plus a visible arc pixel against terrain. The body covers its ring and the vitals draw after the body. Repeat with a constructing or flying building where supported. |
| `building-ring-actor-overlap` | Place a Tank north and then south of a selected ground building at distinct world Y. Change actor creation order between repetitions. Use a Wraith for the air case. The entire building turn covers the Tank to its north. The southern Tank and Wraith cover the overlapping building ring and art. |
| `building-ring-owner-fog-edge` | Test an eligible friendly or visible hostile selection, a hostile building after it leaves sight, and an invincible building governed by the existing selection restriction. Move a selected building across the upper and lower field edges and flip pages. Check the relationship color where a ring is visible, the absence of a ring for a hidden or ineligible building, and unchanged HUD pixels. |
| `hydralisk-detached-attack-depth` | Sample image 35 before and after the ordinary attack spawn. On a visible tick put a layer-4 Tank and a layer-18 Wraith at the effect anchor in separate runs, including both slot orders. The detached sprite covers the Tank. The Wraith covers the detached sprite. Isolate the directional weapon projectile from the mask. |
| `detached-attack-birth-anchor` | Move the Hydralisk between wind-up and birth, then move it again after birth. Sample source offsets on facing directions with and without mirroring. After birth, remove the shooter and reuse its slot. Record the child's birth layer and anchor, its first and last drawn ticks, and the attached-image visibility. The independently spawned visual keeps its recorded position during its lifetime. |
| `detached-attack-fog-edge` | Hold the same effect tick and look at a visible anchor, a hidden anchor, and the two field edges. Record visibility at the detached anchor as distinct from visibility at the shooter and target. Verify the effect's field clip on both indexed pages and continue through its expiration. |

Exercise the cases on Terran, Z10, P10, and Showcase wherever the actor
and action are present. Include campaign and Custom Game, plus a Showcase
scene with both actors if available. Cover a natural ordinary-input
attack as well as staged cases. Treat startup and quiet-scene smoke as
shared-build coverage. Use a separate active cut to judge foreground.
For each receipt record the source commit, ROM and input digests, mode,
camera, active page, palette, actor and effect anchors, effective layers,
sprite IDs where available, and distinguishing and wrong pixel counts.
Compare each platform within its own pixel scale and palette.

## Local verification

The Terran building fixture passed under ASan and UBSan in the normal,
scene-clipped, and depth-disabled configurations. It checks nine source
buildings and an Armory ring covered by a Wraith, plus both HUD edges.
The Showcase weapon fixture checks distinguishing overlap pixels against a
Tank and a Wraith. It also covers moving during wind-up, slot reuse before
and after birth, and a Scourge effect that survives the attacker's removal.

All four normal ARM products passed ROM, SRAM signature and RAM guards.
The tracer rows add 256 transient bytes in each build. IWRAM endpoints
are unchanged and the painter reserves up to 128 more stack bytes for
the additional keys. Skirmishes on both maps cover one, two, and three
AI seats and match 240 updates after a fresh load in each product.
The three race campaign openings each match 60 updates after a fresh
load. The skirmish receipt SHA-256 is
`90e4f0ff13f7656da6f99bcf8c25e830eda8eb9e3f36e10ea97f27a736f1039c`.
The campaign receipt SHA-256 is
`77f319495089a586df169c0d8b8fed1dbafe30b78c4a00e1cf08a7f2cdf76106`.

Paired ordinary-button Showcase runs against `f69b2bbd` match world,
HUD and palette across 144 samples. Siege Line matches both indexed
pages throughout. Carrier Approach has active attack sprites in 59 of
60 samples and different field pixels in 51. In Protoss Cross Fire
the field matches on both pages and one sample differs in which page
was active. Total emulated guest cycles fall 1.2% in Carrier Approach
and 0.9% in Protoss Cross Fire. The Showcase receipt SHA-256 is
`d34b9ca6f922bde8fae771858d2bfdd3639a701e18036738a983c0f27509045d`.

## Exact ties still open

The native unsigned key uses the sprite pool ID for exact ties after
elevation, ground anchor Y, and the turret flag. Native allocation and
retirement can reuse a slot, and the GBA still uses actor kind and slot for
many ties. [Tank turrets use a separate draw turn](2026-11-21-native-depth-followup.md).
Preserve the T10
same-Y Barracks/Goliath and equal-elevation Wraith key witnesses and add
a third actor overlapping the Siege hull and turret. Those records
establish key ordering. The native opaque foreground and an exact GBA
match at those ties still need independent pixel witnesses. The
selected-building and Hydralisk cases here exercise image order and
different elevations.

Also capture foreground picking at the same opaque overlap. GBA
contextual picking still has a separate provisional implementation in
[#587](https://github.com/ggu/starcraft-gba/pull/587). The shared painter
now keeps partly visible Wraiths and Carriers in depth order. Use the
`aircraft-crop-past-anchor` case in the [follow-up harness](2026-11-21-native-depth-followup.md#replay-cases)
to add ordinary-input and native overlap captures to the existing GBA
edge fixtures.
