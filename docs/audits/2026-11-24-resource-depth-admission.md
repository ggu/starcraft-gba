# Resource depth admission

The Mode 4 depth queue admits a mineral when its amount is positive,
its drawn footprint intersects the viewport and its center is
currently visible. Geysers use the viewport and visibility tests.
The resource dispatch now uses the two art painters directly after
admission. The non-depth path still calls the checked draw routines.
The resource entries and painter ordering follow the existing depth
keys.

An ASan/UBSan test extracts the resource admission and dispatch from
both revisions. The eight framebuffer captures match across two
pages, nonzero scroll, hidden and visible cells, explored cells and
depleted slots. The fixture includes resources on viewport edges.
It also compares each extracted path with its direct resource draws.

| ARM7 fixture, 128 draws per case | Baseline cycles per draw | Candidate cycles per draw | Reduction |
| --- | ---: | ---: | ---: |
| Five visible resources | 10,798.45 | 10,379.45 | 3.88% |
| Thirty-two visible resources | 61,520.45 | 58,860.45 | 4.32% |
| Thirty-two visible, hidden and depleted resources | 32,112.45 | 30,780.45 | 4.15% |

All three framebuffer hashes agree. The Thumb fixture function
shrinks from 420 to 372 bytes. It uses the production graphics
primitives with authored diamond art and a simple append queue.
Full depth sorting, source sprite costs and cartridge frame pacing
need separate measurements.
