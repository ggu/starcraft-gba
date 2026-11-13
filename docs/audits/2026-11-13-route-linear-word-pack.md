# Word packing for linear route fields

The largest retained P10 gameplay stalls still spend most of their
time in worker depot routing after enabling the facility FIFO. These
are instruction-profiled ordinary inputs from the earlier paired
facility candidate, rather than a profile of the latest merged
source. Inclusive cycles are shown unless labeled otherwise.

| P10 HOME call | Whole loop | Display wait | Facility fill | FIFO expansion | Fill minus expansion | Fill self |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 9,674 | 19,792,901 | 25 | 17,363,925 | 6,781,522 | 10,582,403 | 10,305,154 |
| 11,045 | 19,432,711 | 25 | 16,985,958 | 7,178,707 | 9,807,251 | 9,550,877 |
| 32,644 | 24,678,587 | 25 | 16,734,544 | 5,894,165 | 10,840,379 | 10,556,656 |

The first trace shows the path through resource worker update,
nearest depot, depot reachability, facility next, slot lookup, and
facility fill. The conversion code is inlined in fill self together
with seed iteration and other bookkeeping; the table gives an upper
bound for conversion work, not a measured cost of the output pack
alone. Call 32,644 additionally spends 4,282,611 cycles in the scalar
point fill.

The original Z7 call 46,504 has the same 25-cycle display wait and
33,953,565 cycles in the scalar point fill. The independent
[wide point FIFO qualification](2026-11-13-z7-wide-point-flood.md)
already cut that whole update from 37,547,713 to 21,313,017 cycles;
the [composition replay](2026-11-13-composed-performance.md) does
not enable that option. Word packing can affect Z7 point work only
when the wide point FIFO is also enabled. Neither those historical
Z7 gains nor the P10 fill-minus-expansion values are gains measured
for this prototype.

`CANONICAL_ROUTE_LINEAR_WORD_PACK=1` opts into a common compositor
for the existing facility and wide point FIFO paths. For each 32
interior cells it computes four existing 24-bit groups and writes
three aligned 32-bit words in little-endian order. Other destination
alignments or widths retain the byte writer. The output is still
written only after successful expansion: seed order, whole-pop stop,
tie order, overflow and fallback, route cache and visible update
timing retain their original behavior. The probe adds no persistent
RAM or IWRAM placement and does not change the queue or scratch
layout.

The direct operation count is 12 byte stores versus three word
stores per 32 cells and one inner-loop iteration versus four. A
96×128 field changes 4,608 byte stores to 1,152 word stores; a
128×128 field changes 6,144 to 1,536. The unchanged packing math
still gathers the same directions from dense scratch. Shift and
register costs, generated instructions, ROM size and guest-cycle
benefit need a target measurement.

`make test-sc64-route-linear-word-pack` uses only asset-free
ASan/UBSan host fixtures. The output fixture independently composes
bits for 64-, 80-, 96- and 128-column padded rows at all four output
alignments and checks surrounding sentinel bytes. Facility fixtures
also compare packed bytes, cache reuse, validity, walkability,
diagnostics and forced overflow. Point fixtures compare both normal
and forced-overflow query and cache observations with the scalar
reference. The option remains off by default. No ARM build or timing
was run for this prototype.
