# Read-only facility fill populations

Three physical updates in the composed normal Z10 cartridge show why
64×64 alone is a poor selector for the facility FIFO. The Blood Bath
update contains six short fills; two Outpost updates contain four deep
fills and one short fill. All observed fills stop after finding their
origin, with no queue overflow.

| Map and call | Whole pops, in call order | Discovered cells | Rows with directions |
| --- | --- | --- | --- |
| Blood Bath 11,539 | 25, 48, 85, 116, 49, 165 | 43, 72, 114, 145, 69, 187 | 9, 12, 15, 16, 12, 17 |
| Outpost 11,104 | 5, 2,015, 3,239 | 17, 2,074, 3,277 | 6, 58, 64 |
| Outpost 11,112 | 1,957, 3,199 | 2,015, 3,239 | 57, 64 |

The normal ROM (`d3008b54370012e6ffc70f627e4c4a263268680da6622c6ae4a020fe8a177707`)
and observer are unchanged. Reproduce with the pinned normal Outpost and
Blood Bath controller tapes, the ROM's matching ELF, and the port-free
physical observer. Replay to one call before the target with ordinary
buttons, then use hardware breakpoints in `sc64_route_fill_facility_fifo`:

- `0x08008b50`: before the expansion call. Read 4,752 bytes of padded
  scratch at `0x02023ff8` and optionally the original packed grid. Zero
  cells in the scratch are the already-enqueued seeds in row-major order.
- `0x08008b54`: after expansion, before packing. Read the same bytes.
- To classify the return, break on `0x03001b8c` (origin after a pop),
  `0x03001bb4` (origin on seed), `0x03001bac` (empty), `0x03001c18`
  (exhausted) and `0x03001c20` (overflow). These addresses are specific to
  the pinned ELF. All observed exits were `0x03001b8c`.

Offline, seed a queue with the zero cells. Visit left, right, up, and
down with offsets `-1`, `1`, `-72`, `72`, writing directions 1 through 4
only over byte 7. Stop after the whole pop that yields the observed
post-expansion bytes. This is exact for the observed early exit: the last
pop must write a new direction to the origin. For an exhausted result,
count all discovered cells as popped even if the last pops make no writes.
The completed update's loop cost and route counters still match the
original composed receipts at all three calls. The route build counter
also includes non-facility fields; it is not the fill count in the table.

This is a bounded sample of 11 fills. It supports trying output-only
packing over the touched span while leaving the fast expansion intact.
It does not establish a global cutoff or justify enabling lazy row
initialization across normal cartridges.
