# Z8 native CreateUnit placement path

This note records source-level facts about opcode 11 (`CreateUnitwithProperties`)
from the pinned StarCraft 64 USA native program. It does not claim the Z8
controller is playable or that the GBA collision model is equivalent.

## Native call path

The resident dispatcher at `0x80081804` handles opcode 11 by reading the four
signed bounds for the requested location, calculating the signed midpoint of
each axis, and calling game-overlay routine `0x80133B80` with the unit type and
center. A zero modifier becomes one creation; nonzero modifiers bound the
number of calls.

`0x80133B80` checks native type availability and uses type-specific signed
footprint extents from the table based at `0x800CAF88`, plus map dimensions at
`0x800A2ECC`/`0x800A2ECE`, to keep the footprint in-map. It reads a type flag
from `0x800C99DC`; only the set-bit path reads placement dimensions from
`0x800CAA78`, rounds the candidate to the 32-source-pixel grid (correcting
negative arithmetic shifts), and calls resident `0x80014BE4` for placement
preflight. The unit-creation routine is reached only on zero return from this
preflight. The other flag path goes directly to the actual create routine.
The relevant table bytes are zero in the static ROM image, so the runtime
flag/dimension values for Z8's raw 74/101 rows are not established by this
read.

On the placement-flag path, preflight derives tile extents from the unit dimensions, rejects negative
coordinates or tile rectangles past the map dimensions, then calls
`0x800141A0` and `0x80014694`. The first routine scans the terrain/map tile
flags (including mask `0x28800000`) and uses `0x80013884` for an additional
terrain/rectangle check. The second builds a nearby-unit list through
`0x8012F59C`, scans the candidate tile area, and reads existing units'
footprint fields at offsets `0x12C`–`0x132` while considering occupancy and
owner/force masks. This proves the native path does map and live-unit
preflight; it does not yet establish the full meaning of every flag or status
code.

The actual create path at `0x80133A38` receives the coordinates selected by
`0x80133B80`; static flow shows no random x/y selection. It requests an RNG byte
from `0x8003DDA8` and forwards it into the create helper. The opcode-11 wrapper
at `0x800819EC` saves RNG state, sets seed `0x075BCD15`, performs the requested
calls, then restores the saved state. The RNG-dependent create argument is
therefore repeatable for the same action sequence; this is not evidence of
randomized position selection.

`0x801338D0` is an additional footprint-aware edge check on the actual-create
path: after subtracting the left/top bounds, it requires nonnegative results
and requires the right/bottom footprint endpoints to be strictly less than
map width/height.

## Z8's two authored unit types

The branch at `0x800C99DC + 4 * type` reads a 32-bit per-type flag word and
tests bit 0. A canonical 8 MiB RDRAM snapshot from the existing ordinary-input
First Strike gameplay capture directly establishes these initialized values:

| Raw type | Meaning | Runtime flags | Bit 0 |
|---:|---|---:|---:|
| 37 | Control actor | `0x18110480` | clear |
| 74 | Hero Dark Templar | `0x18410000` | clear |
| 101 | Special Map Revealer | `0x00000004` | clear |
| 154 | Control building | `0xC4001001` | set |

The capture event for final gameplay task 1473 at field 3597 records RDRAM
SHA-256 `d547813f6c9bce416ecf70602a15795dbc16176784c0bcede357293413de0761`;
the existing offline reconstruction has the same size and hash. The virtual
table base maps to physical RDRAM offset `0x000C99DC`, and its words were read
big-endian. The set bit on building control 154 cross-checks the address and
endianness. This directly proves raw 74 and 101 take the clear-bit branch in
that initialized retail session and bypass its building placement grid and
tile/nearby-unit preflight.

The semantic cross-reference remains pinned OpenBW: `unit_type_t::flag_building`
is bit 0 (`data_types.h`) and `load_units_dat` reads a 32-bit flags value per
type (`data_loading.h`). This does not prove that every native mode/build uses
the exact PC table layout. The shared type-footprint edge clamp and
`0x801338D0` strict bounds check still apply; the native midpoint should not
be snapped to the building grid for these two types.

## Provenance and limits

The native ROM was read from the owned USA image (SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`). Symbols are from pinned
`sc64-recomp` NTSC-U recipe commit
`425f409e02edacf897354cd0872c61ce67a9067a`. Its ADR-013 records that the game
overlay ROM range `[0xE38E0,0x12CA10)` loads at `0x8011FBF0`; this maps
`0x80133B80` to ROM offset `0xF7870`. Instructions were decoded in memory with
the already-installed Rabbitizer from the pinned checkout. No ROM bytes,
generated code, or extracted payloads are included here.

The precise collision mask meanings and all preflight return-status
combinations, pathing/terrain-equivalence requirements, and Z8 raw-101
owner-vision behavior remain open. Any GBA
adaptation must state which current-engine overlap checks it uses instead of
claiming native collision parity.
