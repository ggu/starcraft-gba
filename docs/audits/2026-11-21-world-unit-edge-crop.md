# Unit crops at the battlefield edge

The shared depth queue and unit painter used to reject a unit when its
screen anchor fell outside `[-8,247] × [10,125]`. The source art can
still contain field pixels at those anchors. The ordinary clipped
renderer now uses the projected crop for adjacent anchors outside that
fast path. Facing, animation, mirroring, and stride use the same inputs
as the sprite draw. The coarse limit bounds pose lookups using the
prepared body art's reach. The crop decides whether the body enters
the queue. Queue order still uses the actor's world depth. Fog still
uses the actor's world position.

## Harness cases

Run `make test-sc64-unit-edge-crop` with the prepared normal P10 inputs.
The fixture uses the source art's Carrier (raw 72, layer 16) and Wraith
(raw 8, layer 18) on the same anchor. The Wraith's ordinary westward
order selects a mirrored raster. Its stored facing starts north before
the renderer reads its order. Isolate both source crops and compare the
shared painter to the Carrier followed by the Wraith.

| Screen anchor | Carrier pixels | Wraith pixels | Foreground overlap pixels |
| --- | ---: | ---: | ---: |
| (-9, 66) | 124 | 12 | 9 |
| (248, 66) | 188 | 9 | 9 |
| (120, 9) | 617 | 40 | 39 |
| (120, 126) | 259 | 47 | 46 |

The `unit-edge-crop` witness matches every field and HUD pixel in both
slot orders and on both indexed pages. With an enemy Wraith and an
empty visibility grid, the Carrier crop stays and the Wraith is
absent. Both the packed and byte sprite variants pass under ASan and
UBSan. A follow-up ordinary camera replay should pan over these same
sprites and capture the expected clipped foreground on each page.

The Terran Yamato art fixture uses a left-edge anchor of −22 and a top
anchor of −9. The Battlecruiser's body does not intersect either crop,
and the effect alone contributes 119 and 53 pixels respectively. This
keeps the charge-only coverage independent of the expanded body check.
