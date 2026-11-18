# Movement and acquisition costs in dense Zerg 7

On the routing, census and sparse-sight build, the 477 advancing
updates in controller calls 47,758–48,237 average 1,111,345 cycles
in orders out of 2,048,123 total. The existing `SC64_DIAGNOSTIC_ORDERS`
and `SC64_DIAGNOSTIC_SETUP` timers put the outer orders loop in ROM
and add timing calls. Use them for attribution. The instrumented
whole update averages 2,200,213 cycles and orders average 1,264,228.
[Sparse sight control](2026-11-19-z7-sparse-sight-rows.md#paired-arm-replay)

| Instrumented scope | Mean guest cycles |
| --- | ---: |
| `move_toward`, including route work in the mover | 790,046 |
| Timed nearest enemy lookup | 134,214 |
| Orders setup | 117,457 |
| Setup of collision buckets | 37,843 |
| Setup of owner lists | 16,873 |
| Setup of target index | 59,759 |

Among the 476 advancing calls that have a preceding observation,
119 leave the route-build counter unchanged and average 401,997
movement cycles. The 357 calls with a change average 920,348.
Timed nearest-enemy cost stays near 134,200 in both groups. This
association identifies route-heavy movement as the next place to
attribute cost. It does not assign the entire difference to floods.
The instrumented replay matches all 131 historical checks, 480 route
and frame records, and nine raw world, visibility and display cuts.

## Read-only timing on the uninstrumented ROM

The physical observer times instruction edges on 24 evenly spaced
advancing calls of the exact sparse-sight binary. It matches that
binary's full-update and orders timers at all 24 cuts and all 131
historical replay checks. The direct-terrain corridor checker costs
3,660 inclusive cycles per sampled update across 5.88 calls. The
repeated combat eligibility check after automatic acquisition costs
377 inclusive cycles per sampled update. Broad corridor memoization
has a small ceiling in this workload.

An independent ARM comparison enabled eligibility reuse only when
the idle or attack-move dispatcher immediately uses the exact target
checked by the normal lookup. Its host direct-attack and Scourge
sanitizer fixtures passed, including an invulnerable nearer target.
All 131 historical checks, 480 route and frame records, and nine raw
world and display cuts match in the actual ARM replay. Mean whole
update cost falls from 2,048,123 to 2,047,694 cycles and p95 rises
from 2,727,732 to 2,728,523. The prototype stays outside the release.

Prioritize exact floor and tail attribution inside the route-heavy
movement calls. Count which goal fields are evicted and requested
again, and distinguish productive builds from failed searches and
partial frontier continuation. Use the measured route mix before
spending more writable memory on retention.

- Instrumented replay SHA-256 `f2322664bd08a55584be4d2d2ee252ff38414e6ad667afda07b7dcab6662ff31`.
- Uninstrumented edges SHA-256 `69b5fe9edfdb25f095da5312e36d2b6d8358b120c8ad2bbb39c71d9a4150901e`.
- Eligibility reuse replay SHA-256 `6953a0ef4a4848882c8959c973fa60f4f2249e9c630cd04eb00648738958e70f`.
