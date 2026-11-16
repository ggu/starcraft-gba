# Lazy rows for bounded 64×64 facility floods

At Blood Bath call 11,539 the composed route spends 470,415 guest cycles
in facility FIFO fill, only 36,148 of those in its expansion kernel.
This probe skips conversion work on rows the flood never touches, without
choosing a work threshold or restarting an in-progress flood.

The isolated prototype (`564b663`) uses
`CANONICAL_ROUTE_FACILITY_LAZY_ROWS=1` with
`CANONICAL_ROUTE_FACILITY_FIFO=1`; the lazy option is not in the release
tree. It only affects bounded requests on a
64×64 map. A seed or vertical neighbor touch initializes its row from
walkability; horizontal neighbors use the current row. The padded index
uses a 128-byte stride so the row number is a shift. The two outside rows
are blocked. An uninitialized origin row cannot satisfy the stop check.
The existing seed and neighbor order, whole-pop stop, cache publication,
packed direction bytes and diagnostic counts stay exact. Packing visits
initialized rows and writes untouched runs as unreachable. Overflow leaves
the field unpublished and uses the original scalar fallback.

The existing 32,768-byte BFS scratch covers a 16,384-byte queue, up to
three alignment bytes, 8,448 dense bytes, 66 byte flags and the 2,048-byte
retained grid, totaling at most 26,949 bytes. The experiment adds no
persistent state. The lazy kernel is placed in ROM; the linked synthetic
fixture has unchanged IWRAM and BSS and adds 1,184 ROM text bytes.

The isolated branch passes the `test-sc64-facility-fifo` ASan/UBSan oracle for
scalar, eager, lazy and forced overflow variants, including cache and
flood counters. A seed-only open depot initializes five rows; a far open
depot initializes all 64. The test also exercises a 64×48 map, which
keeps the eager path.

A pair of tiny asset-free ARM cartridges measures four cold facility
queries against the same open 64×64 grid. Each column is the median of
seven mGBA updates; validity and packed hashes match every pair.

| Query | Eager, hot expansion | Lazy, ROM expansion | Lazy, hot expansion |
| --- | ---: | ---: | ---: |
| Origin on seed | 76,858 | 39,125 | 39,095 |
| Four tiles east of center | 79,048 | 45,614 | 44,335 |
| Twelve tiles east of center | 97,600 | 94,310 | 82,399 |
| Opposite corner | 368,843 | 670,228 | 489,865 |

The matched-placement check moves only the lazy expansion into IWRAM in
the synthetic fixture. It recovers 180,363 cycles at the far origin but
still costs 32.8% more than eager there. It adds 496 IWRAM bytes while
keeping the existing kernel; Showcase has only 320 bytes to its unchanged
guard with eager facility FIFO, so that placement is not a shared-product
option as tested. Switching to eager once all rows are initialized could
reduce later overhead, but cannot recover row checks and ROM execution
already paid before that handoff. Leave this version off. Measure actual
facility pop and row distributions on both custom maps before choosing a
replacement. No normal product ROM was rebuilt for this experiment.
