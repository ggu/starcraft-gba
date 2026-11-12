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
their existing new-match starts. The port's existing Random race
selection is separate from the native start draw. Other N64 game modes
are not covered.

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
SRAM checksums were rejected before publishing a world. This candidate
has not yet been tested as an ARM build.

The private observer manifests identify the source, pinned emulator and
input tapes without distributing them. Their SHA-256 hashes in table order
are `b9dd5f5f79f3eba7bcfa5c36d8c8fc7485c2c6ce7794a80b96822c94b7dafbb5`,
`27a94551461d7ae6f24d3aa8e7c1c46192ed21e37208f9baca88b2f3632c7cbc`,
`e9df0ebbdb2162d4e4b5133fed0d3880a0c8a42817da555d3a788ed32659a0cc`,
`5cc21827e6d71d145a1dca94d98db15c49ce684f07674b834f88b27db2d43157`
and `7e245d3378b968b3144b6205d1353b4059792ab678f66db4270210b5e6569594`.
