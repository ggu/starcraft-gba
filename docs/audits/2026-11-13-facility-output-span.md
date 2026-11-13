# Output span packing for facility fields

The isolated option (`50e1e7f`, enabled using
`CANONICAL_ROUTE_FACILITY_SPAN_PACK=1` with
`CANONICAL_ROUTE_FACILITY_FIFO=1`) is not in the release tree. It keeps the existing padded initialization
and hot FIFO expansion. On a bounded 64×64 fill, it scans inward from the
first and last rows for directions 0–4. The unchanged compositor packs the
interior span. Outside it, walkable 7 and blocked 15 both encode as 7, so
aligned word stores can fill the packed bytes with `0xff`. Unaligned output
uses the existing byte fill; fallback and unbounded cases keep their paths.
The option adds no persistent RAM or queue metadata.

The isolated branch passes the independent packed-field oracle,
forced overflow, and all four output alignments with byte sentinels. In a
tiny asset-free ARM fixture, hashes and validity match for all four
queries. Medians of seven mGBA updates:

| Query on open 64×64 grid | Eager packing | Span packing |
| --- | ---: | ---: |
| Origin on seed | 76,858 | 65,369 |
| Four tiles east of center | 79,048 | 68,105 |
| Twelve tiles east of center | 97,600 | 89,569 |
| Opposite corner | 368,843 | 367,732 |

The fixture adds 464 text bytes; IWRAM and BSS are unchanged. A byte fill
version was slower for short cases. These costs establish the direction
for the synthetic requests, not the normal game.

## Normal Z10 custom replays

The composed Z10 compile vector adds only the span flag. The verified
32 MiB candidate SHA-256 is
`8e8f5ff9e93f847b11e99a3adc35602dbaae3486cfbcb5c80bd6d8f1812811c8`.
It adds 464 text bytes, with unchanged data, BSS and IWRAM endpoints.
The icon, composed and composed-plus-span ROMs match the full custom
outcomes and all 76 Outpost and 77 Blood Bath checkpoints. Both indexed
pages, palette and gameplay state match at every checkpoint.

| Map | Metric, guest cycles | Icon control | Composed | Composed with span |
| --- | --- | ---: | ---: | ---: |
| Outpost | Mean | 1,320,506 | 1,256,742 | 1,256,630 |
| Outpost | p95 | 2,142,033 | 1,680,193 | 1,681,457 |
| Outpost | Max | 3,816,683 | 2,253,203 | 2,354,760 |
| Blood Bath | Mean | 1,186,105 | 1,194,986 | 1,193,297 |
| Blood Bath | p95 | 1,534,488 | 1,536,048 | 1,528,553 |
| Blood Bath | Max | 2,278,463 | 2,447,406 | 2,398,901 |

All timings use the same 1,000 updates, calls 10,860–11,859. Outpost's
mean is effectively flat against composed, while its maximum rises 4.51%.
Call 11,759 is 163,803 cycles higher than the same call on composed.
A paired instruction trace attributes 162,790 of those cycles to
`wait_flip` inside `gfx_begin_preserve`. Route counters and the
simulation frame match, and unit orders are 10,525 cycles lower.
This points to the presentation boundary in that update rather than
more facility work. Blood Bath's p95 falls 0.49% and its maximum falls
1.98%, but both its mean and maximum still exceed the icon control.

At Blood Bath call 11,539, the profiled update falls from 2,447,899 to
2,399,394 cycles with route builds and hits unchanged (2,336 / 29,673).
Facility lookup and fill fall from 570,727 to 519,650 cycles. Expansion
stays flat (36,148 → 36,174), confirming the gain is outside the flood
kernel. The icon control took 2,235,622 cycles. Keep the option off pending
an explanation of broader qualification, including presentation timing.

Candidate full replay receipt SHA-256:

- Outpost: `fe35cc3d29f5ae27a6c26e2d2871b8ed18b9bdbff923a5076c949ceb1eaf9fbb`.
- Blood Bath: `38bce09d2145c180e49d638bbfccc1a641a985a63e8873bbb9e62acb8bdc3595`.
