# Native Gantrithor capacity and initial hangar

The owned USA N64 executable gives Gantrithor an eight-Interceptor
capacity. The helper at `0x80127558` recognizes standard Carrier raw 72
and Gantrithor raw 82. It reads the unit flags at `0x800c99dc + 4 * raw`.
Bit `0x40` returns eight before the owner upgrade check. The normalized
step-26,877 Blood Bath RAM contains flags `0x5c000004` for raw 72 and
`0x5c000044` for raw 82. Standard Carriers use the owner's upgrade byte
43 to choose four or eight. The 288-byte helper matches the owned USA
ROM at offset `0xeb248` byte for byte. Its SHA-256 is
`c3bc5db12e49bfd9c2f5bc1c5c446b562b58f87d46cf9547159a53ebb277c7a1`.
The normalized RAM SHA-256 is
`c564005d460558a307f550955a5ffd86dc88fc53cfb1fa4e6ff6b167ce6e6134`.
This proves the native capacity independently of P10's upgrade rows.

## Initial stock and flight observations

The authenticated P10 scenario CHK SHA-256 is
`59d0adcb12d7be73082a069d900acf119643cd3c0c754f6cfe80b2613a35a99f`.
UNIT row 256 places raw 82 with rescue-passive source owner 2 at
(3559, 3402). Its property-valid word is `0x27`, its raw hangar word
is zero, and it has no linked serial. The map contains no authored raw
73 rows. These source fields leave the initializer and the rescue-time
child count open.

The retained ordinary N64 captures did not contain a Gantrithor before
or after rescue or a raw-72 Carrier launch. An older Z9 diagnostic
snapshot has two raw-72 Carriers at their authored positions. Each has
eight docked actors, but its provenance includes a modified menu
state and lacks a pinned ordinary input trace. Keep it as a lead.

The owned executable also verifies these child sprite layer stores.
With the native parent layer 16, the corresponding launch, ascent and
approach layers are 15, 17 and 14.

| Cut | Native sprite write | USA ROM offset |
| --- | --- | --- |
| Release | `0x801282e0` stores parent layer minus one | `0xebfd0` |
| First ascent | `0x801287e8` adds two to the child layer | `0xec4d8` |
| Final approach | `0x8012898c` stores parent layer minus two | `0xec67c` |

## Separable GBA changes

The current P10 rescue code stocks four children when the Gantrithor
joins without linked children. A future capacity change can derive the
maximum from the parent type, with eight for raw 82 and the current
four for ordinary raw 72 until its owner upgrade is supported. Keep
rescue stock explicit until an ordinary native capture establishes it.

The 28-byte P10 unit row already carries three rank bits inside the
14-bit flags field, and the candidate validator's byte per parent can
track ranks 0–7. No new field is needed. Apply the same parent-specific
limit to admission, training, stocking and candidate validation. Replace
the current hard rank-4 rejection with that validated parent's limit and
retain the contiguous-rank check. An older executable will reject a world
with a hangar it cannot represent under its four-child policy.

Resolve the existing queue boundary before allowing a larger hangar.
The paid path can raise the pending count to five when the cap exceeds
four, while slot admission and save validation currently reject five.
The HUD cache also shifts the pending count by three bits, so eight
children could alias one pending order. Preserve the 240-actor ceiling
and shared production reservations.

## Harness cuts

| Case ID | Required capture |
| --- | --- |
| `gantrithor-rescue-stock` | Open P10 from a legitimate native campaign save. Retain input records and snapshots immediately before and after the raw-82 owner transfer without buying ammunition. Record owner, the parent count bytes at `+0xc4/+0xc5`, and the linked raw-73 actor IDs and orders. Repeat after no combat updates. |
| `native-carrier-launch-depth` | From an authenticated N64 P8 opening or a paid Carrier in ordinary Custom Game, record held buttons, parent/child actor slots, target, sprite IDs, anchors, and child layer at release, ascent, both far-return histories, final approach and dock. Sample visible overlap pixels in the same cuts. |
| `gba-gantrithor-capacity-eight` | Once capacity is implemented, pay for the fifth through eighth children, reject an overfill, and replay active and docked journals through fresh HOME. Exercise rank 7, a full pending queue, parent death, a detached child, and actor-pool pressure. Check an older save and repeat the ordinary P10 victory under the same rescue-stock policy. |

Keep captured ROM, RAM, SRAM, and images local. The retained native
start candidates lack authenticated P10 rescue or Carrier combat input
provenance, so this audit did not resume them.
