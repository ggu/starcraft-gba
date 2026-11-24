# Padded route grid borders

Complete point floods and dense facility floods unpack their walkability
grids into aligned scratch. They mark both horizontal border rows blocked
before visiting cells. The shared border writer fills the two rows with
aligned words in one pass. The edges of interior rows still come from
the walkability unpacker. The same writer handles the temporary borders
of a small-component flood and the outer borders on resume.

The scratch layout, blocked byte value, seed order, FIFO order and
packed direction encoding stay the same. All callers align the scratch
base first. A 64-, 96- or 128-cell row with its eight padding bytes is
divisible into words, including when a temporary border is inside
the grid.

## Authored ARM comparisons

The paired guest builds extract the production functions from the
source at `46ff54b5` and from this change. Both use the eight-cell
walkability lookup. The wider cases also use the normal pointer FIFO
and queue capacities. Each timing includes initialization, seeding and
expansion. The 64-cell timing includes packing the resulting field.

| Synthetic case | Baseline cycles | Word borders | Savings |
| --- | ---: | ---: | ---: |
| Open 64×64 | 256,861 | 255,162 | 1,699 |
| Blocked 64×64 | 44,006 | 42,307 | 1,699 |
| Open 96×96 | 530,903 | 528,270 | 2,633 |
| Open 128×128 | 942,265 | 938,832 | 3,433 |

All 48 packed fields and discovery counts match at 64 cells per row.
All 32 padded fields, including both border rows, and discovery counts
match across the wider grids. The isolated dense ARM routine drops
from 596 to 592 IWRAM bytes. Full cartridge links and on-hardware
frame pacing require separate checks.

The dense facility comparison also matches all 32 packed fields,
availability grids, queue entries, frontier indices and padded
buffers. It includes early goals, exhausted floods and synthetic
resumed frontiers. The fully open exhausted flood drops from 543,917
to 530,004 cycles. The blocked fixture drops from 25,799 to 24,036
cycles. The larger reduction in the open case may include compiler
code layout effects.

Rebasing the wider FIFO's neighbor loads around the popped cell slowed
all 32 expansion cases by about 4.5–5.7%. Packing 32 directions into
three aligned words saved 5,559 cycles per 64×64 fill with this compiler
layout, at a cost of 192 IWRAM bytes. Moving the writer to Thumb or ARM
ROM made the fill slower. A writer for 16 directions saved 4,283 cycles
and cost 68 IWRAM bytes. The selected border change saves cycles and
fits within the measured size of the prior routine.

| Local receipt | SHA-256 |
| --- | --- |
| 64×64 border fills | `f13960d1fe9cee6403748c541cd84710252a76e1cb7168187fdf7ffde021b4c1` |
| Wider border fills | `758228a14fc0a09420a8dbe994e7789667cacbe1f031239f084033906051d2fb` |
| Dense facility border fills | `675b162601d474c08f6b03278ae57e5437fd783b29915c538c25326c7ec9c01f` |
| Wider FIFO pointer trial | `67d5dcbc5384b7a362548d01a3e73882e777f7be282c6a91b0f8f53571a1e859` |
| 32-direction packing trial | `f69c7f3da5cec4574467038060942ebc1cb92e8d3c7acee6eef615e7ec18de48` |
