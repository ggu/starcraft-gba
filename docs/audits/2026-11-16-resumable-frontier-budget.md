# Work budget for resumable route fields

Read-only register and memory attribution on the exact `5d4b085` Z10
cartridge matches named gameplay checks and every sampled frame, total
and order counter against the independent release runs. Host reconstruction
in canonical seed and neighbor order matches all observed ARM pop counts.

| Release workload | Named checks | Wide fills / pops | Setup / expand / pack cycles per update | Share of total loop | First / farthest observed queried origin at percent of all pops | Pending frontier p95 / max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Zerg 7, calls 47,758–48,237 | 131 | 448 / 4,312,000 | 110,853 / 485,886 / 134,247 | 25.95% | 38.76% / 38.82% | 41 / 52 |
| Zerg 9, calls 31,001–31,500 | 15 | 56 / 614,044 | 13,380 / 66,794 / 16,140 | 4.95% | 2.37% / 2.42% | 38 / 52 |

With the first-origin check every 32 pops, a cutoff at 1,024 pops and
a 128-cell frontier, 285 of 448 Z7 fills and 54 of 56 Z9 fills would
qualify for early pause. The expansion-only upper bound is 10.54% of
the sampled Z7 loop and 3.29% of the Z9 loop. This assumes the measured
pop rate, ignores staging and field access cost, and cannot rule out
additional readers or demand after the window. It is not a measured
speedup or a hardware result.

The prior partial-fill restart spiked worst orders; skipping logical
insertion changed subsequent range moves; deferring and restarting full
work did not reduce mean cost. A new probe should retain discovered
directions and the ordered pending FIFO, publish the same logical
slot and age, and resume without clearing the field. At the 1,024-pop
admission cutoff the existing wide queue still represents all discovered
cells, so those cells can be packed without scanning the whole plane.
Start with one pending field on donor-free wide geometry. Audit direct
range and save readers, blocked/unreachable starts, topology changes,
and scratch ownership before comparing authoritative ordinary replays.

A separate render probe on the same Z7 release tests adding the existing
fog row-mask and cached minimap raster paths individually. Each matches
131 paired raw framebuffer/palette/fog observations and the raw state
endpoints. Row masks cost 0.12% more per active update; the persistent
minimap reduces HUD cycles 11.64% but saves only 0.62% overall in this
window. Neither change was enabled. Prioritize reducing route work
over these rendering switches.

| Local receipt | SHA-256 |
| --- | --- |
| Zerg 7 route attribution | `31ad233e50a616836fd60cef7ec3b5ff4736695dfc1219b144ffd8518c9e1eab` |
| Zerg 9 route attribution | `acacbeaeef21465c76520b5b901a848c641786e87909c7dd48b474abcc6cbd98` |
| Paired Zerg render checks | `e21b768f69d2edc4064a8ab142e22d0a09420c6b249712be4073f398e4f3bf70` |
