# Scalar prefix for small bounded facility floods

On the composed normal Z10 Blood Bath tape, call 11,539 rises from 2,235,622
to 2,447,899 guest cycles. The paired stack traces attribute 361,686 →
570,727 cycles to facility lookup and flood, with identical route builds and
hits (2,336 / 29,673). The candidate FIFO fill takes 470,415 cycles. Only
36,148 cycles (7.7%) are in its expansion kernel; the fill function itself
accounts for 406,186 cycles. That makes conversion and setup a better target
than further tuning expansion for this call. The trace does not identify the
number of pops in each individual fill or establish a crossover threshold.
Outpost benefits from the composed options, so a map-size-only scalar
fallback could discard useful FIFO work.

The isolated experiment (`3054a401`) defines
`CANONICAL_ROUTE_FACILITY_PREFIX=1` with
`CANONICAL_ROUTE_FACILITY_FIFO=1`. The prefix is not part of the normal
source-release build. Only a bounded request on 64×64 tries the
scalar loop first. On reaching the origin or exhausting the queue within 64
whole pops, it publishes the scalar result, including a possible seed-only
field. The scalar loop could continue exactly at the cutoff, but choosing
that for every remaining request would disable FIFO on all bounded 64×64
requests. The experiment instead restores all borrowed walkability bits
and restarts in the FIFO when work remains. Its successful packed output
replaces the tentative bytes; a FIFO overflow restarts the scalar loop
without the cutoff. No partial attempt updates cache validity, age or flood
counters. Unbounded, blocked-origin, rectangular and wide requests keep the
existing path.

The tradeoff is at most 64 repeated scalar pops plus setup on requests that
switch, instead of converting the entire map for requests the prefix can
answer. A continuation that imports the current packed directions and
translates only the pending FIFO is possible, but it adds conversion work
and a second handoff invariant. The simple restart makes a first timing
comparison easier to interpret. Neither option changes the default.

In the isolated experiment, `make test-sc64-facility-fifo` runs the
independent ASan/UBSan packed-field oracle on the scalar, FIFO, overflow, prefix and prefix-plus-overflow builds.
It checks origin on a seed, discovery on pops 64 and 65, longer requests,
complete and partial validity, grid restoration, cache behavior and exact
node diagnostics, including other map shapes.

## Normal Z10 comparison

The composed cartridge build adds the prefix flag to the same compile
vector. The 32 MiB ROM verifies with SHA-256
`16c87da288191c4b3250a3226b9cb463535d3ed6c78a5bdc24a0bdd7ee3c1df2`.
The prefix adds 240 text bytes; the data, BSS and IWRAM endpoints stay
unchanged. Icon control, composed and composed-plus-prefix all match 76
Outpost and 77 Blood Bath ordered checkpoints, including both indexed
pages, the palette, gameplay state and outcomes.

| Map | Metric, guest cycles | Icon control | Composed | Composed with prefix |
| --- | --- | ---: | ---: | ---: |
| Outpost | Mean | 1,320,506 | 1,256,742 | 1,259,439 |
| Outpost | p95 | 2,142,033 | 1,680,193 | 1,686,431 |
| Outpost | Max | 3,816,683 | 2,253,203 | 2,314,022 |
| Blood Bath | Mean | 1,186,105 | 1,194,986 | 1,188,370 |
| Blood Bath | p95 | 1,534,488 | 1,536,048 | 1,527,473 |
| Blood Bath | Max | 2,278,463 | 2,447,406 | 2,421,430 |

The same 1,000 updates (10,860–11,859) are compared for each map. Against
composed, the prefix improves Blood Bath's mean by 0.55% but raises
Outpost's mean by 0.21% and maximum by 2.70%. Blood Bath's maximum is
still 6.28% above the icon control; a separate menu opening at call
11,847 costs 140,775 cycles more with the prefix than without it.

The scoped replay of Blood Bath call 11,539 falls from 2,447,899 to
2,421,923 cycles, with the same route counters. Facility inclusive cost
falls from 570,727 to 540,622 cycles and the remaining FIFO fills total
244,266 cycles. Most of the reduction in direct FIFO work is offset by
the added scalar work. Keep the threshold off: it does not resolve the
Blood Bath spike and costs time in the measured Outpost window. If this
is revisited, measure per-fill pop counts before choosing another threshold
or changing the handoff. These are emulated guest costs.

Candidate replay receipt SHA-256:

- Outpost: `9fbc18f51ba7f10a917f6ebe717161b960a16f66ded1958298030837388d5196`.
- Blood Bath: `5b41487732abc8640d3dc794a7271c876a6e92e5a07e04be2017b73c7094b654`.
