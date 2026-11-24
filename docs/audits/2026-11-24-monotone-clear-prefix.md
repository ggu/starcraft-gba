# Clear segments in the monotone route search

The optional `SC64_ROUTE_MONOTONE_CLEAR_PREFIX` flag accelerates
the first horizontal and vertical segments of the bounded
goal-rooted search. Both follow the existing search priority.
If a cell blocks the first path, the search writes the
ancestor phases and resumes from the last clear cell. The
first branch consumes the original node budget. The scratch
capacity check, goal-seed ordering, route-field lookup and
request aging still run through the existing code.

The shortcut skips the bitmap clear only when the first path
reaches the origin. Diagnostics keep the standard search to
preserve the reported work counts. A configuration that records
witness paths also uses the standard search.

## Evidence

A read-only capture from the installed Terran Blood Bath ROM
supplies 381 ordered route dispatcher requests over 96 updates.
The ARM fixture restores the initial packed fields and applies
the observed pre-call metadata outside its timed region. Both
variants return the captured coordinates and match the route
clock, slot ages, hits and builds on all 381 requests.

Separately scanning for a clear L first increased cycles on this
workload. Among its 277 monotone calls, 217 had blocked goals.
Each of the 60 open-goal calls had an obstacle on the preferred
L. Scanning packed row bytes reduced this extra cost. Advancing
the search state past the clear cells produced the net gain.

| Dispatch implementation | Measured cycles | Change |
| --- | ---: | ---: |
| Existing search | 7,284,156 | — |
| Separate packed L scan | 7,420,785 | +1.88% |
| Resume from the clear prefix | 7,037,803 | −3.38% |

With the resumed search, 83 individual calls still cost more.
The largest increase was 159 cycles. Per-call p95 falls from
65,044 to 62,869 cycles.

The authored race-engine fixture uses 20 moving actors, a
64×64 grid, request aging, two live grid edits and matching
D-pad input. All 280 world, route-counter, display and palette
samples match. The route dispatcher remains in IWRAM.

| Frame-loop measure | Existing search | Resume from clear prefix |
| --- | ---: | ---: |
| Mean simulation cycles | 240,905 | 229,195 |
| Frame p95 cycles | 581,253 | 561,393 |
| Updates above two VBlanks | 18 | 13 |

The authored build adds 392 bytes of text, uses the same BSS
and passes the linker reserves. The route fixture measures an
isolated dispatcher with metadata injected at call boundaries.
The authored fixture has its own terrain and actor mix. Busy
race-cartridge timing, save boundaries and hardware pacing
remain open before release activation.

The host differential runs 30,095 cases with ASan and UBSan.
It covers live fields, the aging boundary, blocked horizontal
and vertical prefixes, blocked-goal seeds, changing shapes,
distance-254 paths, and exact or short budget and scratch
inputs. Route outputs and metadata match. Fully clear paths
skip their bitmap writes. The diagnostics build emits the
same probe counters as the baseline.

| Local receipt | SHA-256 |
| --- | --- |
| Separate packed L scan on captured calls | `d9500e9b2366f35c55eaec4b1a4c5741a4192d869e6fabf89f9bd417ab4b525b` |
| Resumed probe on captured ARM dispatcher calls | `27fbf520565244540118e05173e70333e19b2ad87b1b95d6006117007adce03f` |
| Authored frame-loop comparison | `5c6c557811ad66a41ef9b5f63877157fe5f69f09e700ead85135718d1deccb34` |
