# Blood Bath opening locations

Five ordinary controller runs on the owned N64 ROM observed the following
Melee starts. Each run verified three cache-aware read-only samples against
the source map. Both the live actor list and the world viewport identified
participants and locations; all 60 native resource placements matched the
extracted Blood Bath map. Each opening hall had four workers.

| Lobby (human and visible computer rows) | Human start | Computer starts in open row order |
|---|---|---|
| Terran; Closed / Terran / Closed | Northwest | Southwest |
| Same middle row, Start delayed by 137 callbacks | Northwest | Southwest |
| Terran; Terran / Closed / Closed | Northwest | Southwest |
| Terran; Closed / Closed / Protoss | Northwest | Southwest |
| Terran; Zerg / Terran / Protoss | Northeast | Southeast / Northwest / Southwest |

These are five outcomes of the [native seeded draw](2026-11-12-n64-blood-bath-opening.md#seeded-melee-start-draw),
not a fixed table. The optional GBA Melee preview uses that draw on Blood
Bath for any open rows, with the current GBA's elapsed milliseconds as
the seed. It observes the existing Timer 2/3 cycle counter once per main
loop to extend its 256-second wrap, then samples again at Launch. This
approximates the native clock's role without claiming the same elapsed
time or draw for the same button tape. Outpost and default products keep
their existing new-match starts. Default Random selection remains separate from the native start draw.
The optional preview's CPU Random sequence is qualified separately
below. Other N64 game modes are not covered.

Use `make sc64-native-melee-start-probe` with all four prepared profiles to
build the private opt-in cartridges. The preview uses distinct ROM names
and cartridge identities. The default targets do not enable it.

The port's Blood Bath location IDs stay northwest, southeast, northeast,
southwest. A new resolved-start SRAM marker stores the occupied lobby mask
and the physical location IDs in human/active computer order. The record
length and outer save tag stay the same. The loader checks the mode, map,
number of active seats, legal race and distinct locations before publishing
the world. Existing untagged dense and original sparse saves keep their
previous locations for AI fallback and match restart. Changing the clock
seed after a save does not change the restored locations. A newer default
build can read a preview save and retains its resolved starts. Releases
without this loader reject the new marker; their save journal may still
fall back to an older valid record if one exists.

Canonical host sanitizer checks cover 234 normal starts plus eight sparse
starts in each of Terran, Zerg Z10, Protoss P10 and Showcase, with the
preview both off and on. The preview checks include tagged save/load,
AI fallback with the starting hall absent, and pause-menu restart with
a different clock seed. The source-only fixture checks clock conversion
across the hardware counter wrap. The three frontend sanitizer variants
cover legacy and malformed marker decoding. An unchanged prior Terran ARM SRAM image
from the middle-only Blood Bath lobby loaded with the original northeast
AI hall and fallback origin, kept its old marker on re-save, and restarted
at the original positions. Six malformed tagged records with recomputed
SRAM checksums were rejected before publishing a world.

Rebased source `c2b184174457d17ee050e9a12765c7b7d5a0c251` built all four 32 MiB ARM previews. The ROM
headers, distinct cartridge identities and SRAM signatures passed. Using
only physical controller input and read-only guest observations, the
Terran, Z10, P10 and Showcase previews each launched Blood Bath. The
observed layout was consistent with the hardware-clock interval around
Launch, and the actual town halls matched it. Each exported SRAM image
had a valid checksum and resolved-start word. A pause restart, a fresh
emulator load and 60 subsequent updates preserved the locations and the
SRAM bytes. The runs cover sparse and dense lobbies, named and Random
computer rows, and all three human races. They do not measure frame
rate or constitute GBA hardware testing.

| Preview | ARM ROM SHA-256 | Saved layout word |
|---|---|---|
| Terran | `6040d6eb382d1fb000bc6950b5efb0a2658e5174237d3bc261d5939c887e17db` | `0x534d0068` |
| Z10 | `7ee9572e8e43d43c3394b47caa81e2f9331b579523bc986cecc764e613fe4e03` | `0x534d01d6` |
| P10 | `a47f5fc48086e8feefc7b8904b5e6fa9899d5af81aa712359a5054ba5b996f16` | `0x534d097e` |
| Showcase | `1a58b38ace579c63686be50034c2fadeed20a2326f145c72b3130b30885db35c` | `0x534d01b8` |

The four-profile physical receipt SHA-256 is
`114326c8a5cd62a1262b69eb35419bd7dde7848e3ed32e89cb46ad0a0083894f`.
The normal Terran ARM ROM from the same source,
`a5375ee144fc105c072410385b2b2ed0a6f31d5abe883f4bbad6b4ff54c7f337`,
also loaded the actual preview SRAM and the pre-preview sparse SRAM in
separate fresh processes. Normal and preview matched immediately after
load, after 60 further updates and after pause restart. Both left the
SRAM bytes unchanged. The preview record kept its southwest human and
northwest CPU; the older record kept its northwest human and northeast
CPU. Starting a new normal game also kept the original positions and
did not use a new mapping. The normal/preview cross-load receipt
SHA-256 is `8cba3fcac843dc2545ff3446e98419c7e22c8c5b229a504ea29241ae68ce19a0`.

The private observer manifests identify the source, pinned emulator and
input tapes without distributing them. Their SHA-256 hashes in table order
are `b9dd5f5f79f3eba7bcfa5c36d8c8fc7485c2c6ce7794a80b96822c94b7dafbb5`,
`27a94551461d7ae6f24d3aa8e7c1c46192ed21e37208f9baca88b2f3632c7cbc`,
`e9df0ebbdb2162d4e4b5133fed0d3880a0c8a42817da555d3a788ed32659a0cc`,
`5cc21827e6d71d145a1dca94d98db15c49ce684f07674b834f88b27db2d43157`
and `7e245d3378b968b3144b6205d1353b4059792ab678f66db4270210b5e6569594`.


## CPU Random sequencing in the preview

The [owned N64 CPU captures](2026-11-12-n64-blood-bath-opening.md#random-races-consume-the-launch-stream-first)
confirmed that one or two Random CPU rows consume one draw each before
physical start assignment. The opt-in Blood Bath preview follows that
order with one GBA clock seed; the normal title menu and Outpost use
their existing selector. Resolved race and placement are stored in the
already qualified v2 record, with no new format.


Source `4db23aa7422e07f675cb51bb19d9d9adfa49b05f` passed the four
canonical sanitizer profiles both with and without the preview. This
includes exact sparse and dense CPU Random lobby vectors and malformed
lobby rejection. The Terran preview ARM ROM
`a6d1864c2b00f1698f1c7c25e7fa69dfd6eb5850afaf3db87d5c9ab134411fab`
also passed physical-button runs for one and two Random CPU rows. The
first clock sample before each launch (4104 and 5311 ms) predicts both
the resolved races and starts. The actual checksummed v2 saves preserved
both after a fresh load, 60 updates and a pause restart.

The prior normal Terran ROM and this preview independently loaded the
new Random v2 save, the previous preview's v2 save, and the older sparse
v1 save. They matched on load, continuation and restart and left every
SRAM image unchanged. The physical receipt SHA-256 is
`d9056d6934a9cc0e4652c7111dd040b38d1b1d74990495e82076124300eac8c9`;
the cross-load receipt SHA-256 is
`1d8042ffb6f0444fc5f1a99b4777fac8a035c551982e2b04fa0bfc09dc12c4af`.
The CPU Random change was physically checked on ARM for Terran; the
other three products have host checks for this change.
