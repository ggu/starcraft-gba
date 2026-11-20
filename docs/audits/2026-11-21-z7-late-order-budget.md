# Dense Z7 order budget

The normal Z10 ROM `aa3636ed` carries Z7 through a fresh SRAM load and
ordinary-input victory. A read-only replay of that tape profiles three
windows after restoring frame 5,641. One ordinary A reselects the
Hatchery. All 228 named gameplay checks and three guest clock spans
match the qualified run. The table counts advancing game updates.

| Tape calls | Advancing updates | Mean update cycles | Mean orders cycles |
| --- | ---: | ---: | ---: |
| 47,758–48,237 | 477 | 2,161,000 | 1,317,000 |
| 153,812–154,741 | 926 | 2,686,000 | 1,770,000 |
| 154,742–155,650 | 905 | 2,635,000 | 1,727,000 |

In the late windows, orders account for 65.7% of 2.661 million mean
cycles. The 131 route bursts recur every 14 advancing updates and account
for 69.3% of recorded builds. Each burst records 12–15 new builds and
averages 4.997 million cycles, with 4.095 million in orders. The budget
for an update at roughly 30 fps is 561,792 cycles.

The 1,029 updates with zero builds still average 2.212 million cycles,
including 1.296 million in orders. Profile recurring collision, route
reads and target work on those updates as well as periodic route bursts.
The movement code loads a 12-update retry then spends one update consuming
the last waypoint-timer tick. That gives a 14-update interval between
failed plans. The exact instruction replay confirms the same 49 human
Hydralisk slots at all four sampled bursts, occupying 20 tiles with
Attack Move and no current target. Each starts a fresh 12-update retry.
The traced calls follow `update_orders` through `move_toward`,
`sc64_route_next`, and `sc64_route_slot_for` to the point-fill helper.
Point fills average 2.673 million cycles across those four burst cuts.

Five separately sampled zero-build frames average 2.411 million guest
cycles. `sc64_target_nearest` takes 700,159 inclusive cycles per update,
including 497,054 in the candidate permission helper. Its `race_can_attack`
calls use 371,541 cycles. The attached Z6 cloak check alone costs 74,159
cycles in this call chain while Z6 is inactive. The uncached contact mask
takes 145,130 cycles. Test a cheap, exact gate for inactive campaign
policies first, then measure how much common target work remains. Check
Z6 detection with an Observer and Cannon, Z5 and Z8 cloaks, transported
units, Scourge, hidden hangar occupants, and target and fog changes within
an update before using a shared permission snapshot.

## Resumable point-route trial on the shared-depth release

The [point-route trial](https://github.com/ggu/starcraft-gba/pull/576)
is composed with `0034ebd4` in isolation branch `fb086c3e`. This comparison
also enables `SC64_ROUTE_POINT_SECOND_EXACT_HIT`. The control reproduces
the release ROM. The 39 prepared input hashes and both ARM payloads
verify. The trial adds 320 IWRAM and 1,072 EWRAM bytes, within the Z10
link guards. The route sanitizer oracle matches the reference, including
parked frontiers, range readers, donor reuse, and invalidation.

| Calls 47,758–48,237 | Control | Point-route trial |
| --- | ---: | ---: |
| Advancing updates | 477 | 477 |
| Mean total cycles | 2,159,356 | 1,943,620 |
| p95 total cycles | 3,145,671 | 2,699,587 |
| Peak total cycles | 5,062,030 | 5,598,943 |
| Mean order cycles | 1,316,575 | 1,100,748 |

The trial cuts the early dense mean by 9.99% and raises the peak by
10.61%. Both re-created SRAM exports match the qualified frame-5,641
save byte for byte. The three early framebuffer, palette and camera
cuts and timed logical route counters match. The largest increase is
at call 48,149, which rises by 1,114,192 cycles on an update with one
logical build and nine hits. Instruction tracing identifies two parked
completions called by the nearby-root reader at both 47,995 and 48,149.
They cost about 1.63 million cycles together in each case. The preceding
update 48,148 cuts point-fill cost from 701,313 to 118,548 cycles. The
nearby-root caller can require a whole-field completion on a later call.

Both exact cartridges continue to victory at frame 228,128. All 254
resumed gameplay checks and ten framebuffer, palette and camera cuts
match. Logical route counters match on all three timed windows. Over
225,208 resumed main-loop calls, mean cycles fall from 1,324,227 to
1,304,009 (1.53%). This corresponds to 12.67 to 12.87 emulated calls per
second over the full run, including menus and presentation waits.

| Late tape calls | Mean change | p95 cycles, control → trial | Peak cycles, control → trial |
| --- | ---: | ---: | ---: |
| 153,812–154,741 | −1.45% | 4,595,827 → 4,620,898 | 6,856,377 → 6,976,744 |
| 154,742–155,650 | −1.77% | 4,610,035 → 4,713,846 | 6,419,346 → 6,376,968 |

Current Showcase also links with the same IWRAM endpoint and 1,072 extra
EWRAM bytes. The [follow-up source](https://github.com/ggu/starcraft-gba/commit/b961dd31989c92d0afa63b6ad82c09862ce4cea1)
keeps the existing wide fill code when the experiment is disabled.
The off build now reproduces the current stock Z10 ROM byte for byte.
The enabled build is byte-identical to the trial used above. Keep the
trial opt-in. Prioritize the exact nearby-root reader and its completion
spikes before wider qualification.

## Harness priorities

| Case ID | Required evidence |
| --- | --- |
| `z7-late-order-retries` | Replay calls 153,812–155,650 on the pinned ROM. Record fills, caller, origin, retry write and intervening timer. Check the 14-update cadence and state on both sides of the route call. |
| `z7-late-orders-zero-builds` | Trace calls 153,992, 154,202, 154,247, 154,598 and 155,333. Compare collision, targeting, movement, range routing and rendering. Reconcile the independent guest clock and engine stage counters. |
| `z7-route-trial-peak` | Compare fresh Z10 pairs at 47,995 and 48,148–48,149, then both later windows. Require matching named samples, SRAM and framebuffer cuts. Inspect parked-reader materialization whenever the maximum rises. |

| Receipt | SHA-256 |
| --- | --- |
| Qualified exact stage windows | `108cc16e69805e6ac293de3507bc1b8aaa042a207f970f6fa3db352057b38fa9` |
| Late cadence summary | `30c191bedbc4526035e4a755f70bd532274e60e481a20eef13cc7fe3f888ef04` |
| Late instruction attribution | `468fecb6837e3acaa0e45835920ca9a92f6df7ac295e73248992b3fbca3fa2fe` |
| Peak caller attribution, control | `597837bc9992fdd09a77f104eed2337f6c9df2dfac2fa6f1118dfb296dab5fe5` |
| Peak caller attribution, trial | `c607434b7eca8fbc98c4db6f487aa535c9697a88d060a383baa40599ac80b416` |
| Current Z7 paired victory and timed windows | `250329021b4746d6b6880b5c235fe77c3d7531bba0af999c8341822305f2f025` |

The recordings measure emulated guest cycles. Repeat the same replay and
measure wall-clock presentation and slow updates on a GBA SP for the
hardware gate.
