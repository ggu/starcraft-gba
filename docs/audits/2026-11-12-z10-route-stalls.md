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

## Why the first bounded route shortcut changed combat

On the current normal Z10 input replay, call 19,531 (96×128) spent
14.84M guest cycles. A captured walkability grid and the seven cold
movement requests from that call were replayed against a bounded
reverse FIFO proof. It returned the same next cell as the full point
field for all seven requests. A straight two-leg path only answered
three; a monotone search answered five. The local proof did not address
the separate formation field built in the same update.

Using that proof to skip the entire cache insertion still changed the
campaign. Both complete 64×64 three-AI custom replays matched, but the
normal Z10 replay diverged on call 7,472, game frame 7,112. Its first
different actor was unit slot 105, which stored waypoint (600, 1848) in
the control and (568, 1880) in the probe. A read-only guest breakpoint
trace confirmed that the weapon-range lookup returned a cached point
step for the control and a miss for the probe. The probe then found a
direct firing waypoint. The actor did not call the new local search on
that update. Thus a correct immediate step alone is insufficient: later
combat and route reuse depend on which fields are logically resident.

## Keep the entry; defer its bytes

A second probe inserts, ages and evicts the point field exactly as before.
It defers the flood after a proved step, and materializes the field when
a consumer needs more than a proved step. Range lookup sees the logical
field and still returns the same waypoint. On the host, 386,646 cache
observations (including goals, ages and hit/build counters) match the
stock configuration, and the independent oracle checks the steps. A
focused regression checks range lookup before insertion and after the
deferred insertion, including that it does not age the field.

The normal Z10 cartridge loaded the same physical SRAM and won at frame
59,997 with 61 identical state checkpoints and 24 identical rendered
page, palette and fog samples. Two full three-AI matches on the 64×64
maps matched their state checkpoints through the same defeats. The
deferred ROM kept the EWRAM end and added 168 bytes of IWRAM. It improved
the first battle window's maximum from 14,834,555 to 9,270,546 cycles
(−37.51%), at the same call 19,531. Mean loop cycles rose 0.01% and 0.04%
in the two campaign windows; the earlier simulation p95 rose 5.90%.
The custom means changed by less than 0.07%. These are guest cycles,
and the measured means remain far above the 561,792-cycle target.

The defer-only probe stays off. An independent exact FIFO implementation
that fills the same resident field on a 96-column map lowered call
19,531 to 8,062,174 cycles (−45.65%) and the early window's mean by 1.04%
with 19 matching checkpoints and 12 matching video samples through call
20,000. It is the stronger candidate for full campaign, custom, and
cross-product checks.
