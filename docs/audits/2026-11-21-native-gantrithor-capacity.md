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
track ranks 0–7. The compact Terran and Zerg unit rows carry 16 flag
bits. No new field is needed. Resolve the limit from a validated
parent's immutable unit rule in both the live and decoded unit arrays.
Future owner-upgrade limits would also need the decoded upgrade context
during save preflight. Apply the same parent-specific limit to admission,
training, stocking and candidate validation. Replace the current hard
rank-4 rejection with that validated parent's limit and retain the
contiguous-rank check. An older executable will reject a world with a
hangar outside its four-child policy.

Before the configurable capacity trial, the paid path could raise the
pending count to five with a larger cap, while slot admission and save
validation rejected five. The HUD cache shifted the pending count by
three bits, so eight children could alias one pending order. The AI
budget reserved one hull and four child slots. A wider hangar rule
needs matching HUD and AI reservations while preserving the 240-actor
ceiling and shared production reservations.

## Configurable capacity trial

The shared engine trial accepts a pure `SC64_P7_HANGAR_CAPACITY_FOR_TYPE`
policy returning a capacity of one to eight for a supported parent. The
built-in policy uses four and the rescue code supplies its initial child
count separately. Live admission and decoded save validation use the
same unit-type policy and the existing serialized rank bits.

The eight-slot HOST fixture exercises five paid orders, rank seven,
capacity-based AI reservations, distinct HUD keys, and immediate rejection
of an invalid queue or duplicate rank. It resumes a normal pause journal
with HOME L in a fresh process and matches 1,500 continuation updates.
The continuation digest is
`20ead61f18b4bdba4e8478c43a9732c73668b30531d3b98ea1c19798f42d4ccf`.
The optional combined journal also rejects five pending orders alongside a
paired Egg when the live unit pool is one slot short. It accepts and
restores the journal when the sixth free slot is available.

## Ordinary ARM checks with the four-slot policy

The normal Terran, Z10, P10, and Showcase ARM builds compose the trial
with the [packed building release](2026-11-21-terran-building-word-draw.md)
at `60dd3d94`. The trial source was `ca594583` and the composed runtime tree
was `498d3abf`. Each build used its authenticated production command and
private inputs. The packed banks and both generated fact banks matched.
The 32 MiB payloads and RAM endpoints passed the ordinary product guards.
The relevant Carrier validator, slot-reservation, save, and HUD stack
entries in the three race products matched their packed ARM controls.

| Product | ARM ROM SHA-256 | IWRAM end | EWRAM end |
| --- | --- | --- | --- |
| Terran | `1b298fbbd39a62a15de35983ef7ddf84b2283a12212f174307af39d577d6ba6f` | `0x030049f8` | `0x0203aa0c` |
| Z10 | `fa17bacfc510a363f6271c19ac50ccb107030680cf1b4b1358f63b2a7a995088` | `0x03004d60` | `0x02039b30` |
| P10 | `2da1eba67f634a70450243c6322d650ad9cae3c5a39734ec2ce3f0293816ed64` | `0x03004e20` | `0x0203b62c` |
| Showcase | `a08e67f17a2fe1966ce7e4dad81533baecba6658b611cb29c6327beb244dad64` | `0x03004fb8` | `0x02039168` |

Normal Showcase reproduced the packed control ROM byte for byte. Its
ELF differs only in debug sections and temporary object filenames.
Link receipt SHA-256
`6855ae869bde1b414201ddfd65cc5c7b3ca82848a762ce2bfd4095f935d73903`.

The normal Terran controller tape buys a Carrier for 350 minerals and
250 gas and an Interceptor for 25 minerals. It saves a live attack at
frame 16,100 and the docked child at 16,140. The active and docked SRAM
images are byte-identical to the earlier ordinary paid tape, and both
restore via fresh HOME L. The paired receipt SHA-256 is
`3b17ca7c1ecb682e954baea328b52e944e7f7039577443a47830823d6f99c030`.

Z10 loads the earlier compact journal, then writes a new pause at frame
17,305 when a shot carries its launch layer. It matches the historical
whole journal, render cuts, and observed stack watermarks. The fresh
HOME L run matches the uninterrupted pool for 180 updates with 12 world
checks. Receipt SHA-256
`5ab1999dcd06c64b03a27ef500f9e210539c05f9188850db5281e18348e880ad`.

P10 writes an identical active-shot journal at frame 37,589. The earlier
world and video cuts match, with 36 matching pool and target-health
updates after fresh HOME L. Receipt SHA-256
`c7111554e0bc34aadec576f69090c31832f47b8e0697ea98925efd0968669948`.
These observations cover emulated ARM at the built-in four-slot policy.
The eight-slot case remains covered by the HOST fixture.

### Current renderer composition

The follow-up composition `eab2418c` includes the
[building-edge depth change](2026-11-21-world-depth-edge-and-ties.md) from
`9398b13c`. All four normal ARM builds use the same pinned inputs and
retain the IWRAM and EWRAM endpoints above. The normal Showcase ROM
matches the qualified `9398b13c` Showcase byte for byte.

| Product | Composed ARM ROM SHA-256 |
| --- | --- |
| Terran | `215f5a178e4a596a9f2d21f2e0e3f338190912a1e687ea31dd32ae2e870d45a3` |
| Z10 | `d664d71c80a6aa959cc98c018e596d69726a6a0c00570873e2d60e50fdc1a5d4` |
| P10 | `8862f340736df0aac75776659e6cfb8fec6c404cfc4b737edc667b50ff00be3b` |
| Showcase | `9127bb13474f9098e07b031b072c459211c0ef0210a44c03891eebc0e5e5bcca` |

P10 repeated the ordinary pause at frame 37,589 and loaded from HOME L
in a fresh emulator. The 32 KiB SRAM, world, shot rows, video cuts, and
36 continuation updates match the prior four-slot run. The comparison
SHA-256 is
`7fa781d241036ddbc722c7752618467065f2369bb398b141af13c216b272194f`.
Terran now repeats the paid Carrier and child route on the composed
ROM. Its frame-16,100 active and frame-16,140 docked saves match both
prior journals byte for byte, and both restore from fresh HOME.
Z10 repeats the legacy load and compact frame-17,305 pause. Its full
journal matches, and 12 world cuts across the next 180 updates agree
with uninterrupted play. The current Terran and Z10 replay hashes are
`64085df7c58270d231fdef3722a1e68b3983a7f6b726ea8f3d44bc9107178d78` and
`eadbcc23bc3f39e753adba74b045ad123f4cdaeec2d1a81e3fe6f53dcca10387`.
The current four-link receipt index SHA-256 is
`de79b6098a2d3ff10313b2b3234fbf407fa126608944af265970ae211b6fe3b0`.

## Harness cuts

| Case ID | Required capture |
| --- | --- |
| `gantrithor-rescue-stock` | Open P10 from a legitimate native campaign save. Retain input records and snapshots immediately before and after the raw-82 owner transfer without buying ammunition. Record owner, the parent count bytes at `+0xc4/+0xc5`, and the linked raw-73 actor IDs and orders. Repeat after no combat updates. |
| `native-carrier-launch-depth` | From an authenticated N64 P8 opening or a paid Carrier in ordinary Custom Game, record held buttons, parent/child actor slots, target, sprite IDs, anchors, and child layer at release, ascent, both far-return histories, final approach and dock. Sample visible overlap pixels in the same cuts. |
| `gba-gantrithor-capacity-eight` | Once capacity is implemented, pay for the fifth through eighth children, reject an overfill, and replay active and docked journals through fresh HOME. Exercise rank 7, a full pending queue, parent death, a detached child, and actor-pool pressure. Check an older save and repeat the ordinary P10 victory under the same rescue-stock policy. |

Keep captured ROM, RAM, SRAM, and images local. The retained native
start candidates lack authenticated P10 rescue or Carrier combat input
provenance, so this audit did not resume them.
