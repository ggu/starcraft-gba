# Worker depot route stalls in the final Zerg mission

The packed-sight Z10 cartridge replayed normal input from the current
physical SRAM image. In two 1,000-call battle windows, seven updates spent
at least 5 million cycles in simulation. In all seven, unit orders were
the largest timed stage. Per-instruction tracing those same updates
matched the ordinary replay and accounted for the elapsed guest clock.

| Call | Full loop cycles | Worker home selection | Depot field lookup (nested) |
| --- | ---: | ---: | ---: |
| 19,359 | 27.72M | 26.38M | 24.43M |
| 19,457 | 35.38M | 32.86M | 30.61M |
| 19,724 | 28.99M | 26.64M | 22.24M |
| 35,250 | 38.51M | 36.00M | 35.55M |

Call 19,531 instead spent most of its 14.97M cycles in point floods. The
route problem is separate from the usual defense, sight and drawing costs.

An opt-in trial completed a depot field on the first miss of an already
partial field, preserving the original roots and neighbor order. All 22
host route oracle variants passed. The normal ARM trial matched 61 raw
state checkpoints, 24 video/fog samples and the same full Z10 win. Its
earlier maximum fell from 35.38M to 30.30M cycles, but the later maximum
rose from 38.51M to 39.02M. Mean loop cycles improved by 1.64% and 0.32%
against the packed-sight-only control. This is a probe, not a release flag.

The [nearest-first follow-up](2026-11-12-z10-depot-first-nearest.md) now
removes most of the observed depot stalls in normal Z10. The earlier
window's remaining maximum is the point flood at call 19,531. Keep the
30 fps budget separate from these worst update stalls.
