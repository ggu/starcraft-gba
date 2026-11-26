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
preserve the reported work counts. This original qualification used
the clear prefix on its own. The later
[combined comparison](2026-10-29-monotone-route-step/README.md#clear-prefix-and-retained-path-comparison)
also checks the retained-path table with the shortcut enabled.

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

## Exact-package route-call coverage

Read-only traces of the installed Terran and Protoss cartridges capture
all 280 monotone calls in the 24 updates after update 11,000 on both
maps. The traces match uninstrumented field fingerprints, both display
pages, palette, camera, phase counters and elapsed cycles. A pair of
generic ARM7 probes reads each test grid and query list from external
SRAM. The two test ROMs contain no captured grids. Wait states match the
original `0x4317` setting, and the timer covers the probe call alone.

All status codes, predecessors, node counts, candidate counts, memo
checks and seeds match the captured calls. The candidate can skip the
memo clear when its first path succeeds.

| Installed workload | Calls | Baseline probe cycles | Clear-prefix cycles | Reduction | Mean cycles saved per sampled update |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran Outpost | 68 | 138,456 | 130,352 | 5.85% | 338 |
| Terran Blood Bath | 70 | 1,521,301 | 1,463,384 | 3.81% | 2,413 |
| Protoss Outpost | 83 | 173,050 | 168,156 | 2.83% | 204 |
| Protoss Blood Bath | 59 | 437,205 | 427,113 | 2.31% | 421 |

The isolated totals improve, though 94 individual queries still cost
more and the largest increase is 159 cycles. The original full updates
in these windows average 859k–1.292M cycles against the 562k budget.
The probe replay does not measure a frame-rate change on the cartridge.

The installed Z10 `sc64_route_next` calls its ordinary route-slot
dispatcher. The same probe also appears at two far cached-read call
sites. Neither site runs in the four sampled 24-update windows on
Outpost and Blood Bath after updates 9,000 and 11,000. The sampled
Z10 workload therefore has no clear-prefix work to speed up. Prioritize
frequent movement, collision and actor drawing work for busy frames.
Release activation still needs a paired full-frame and save check.

| Local receipt | SHA-256 |
| --- | --- |
| Four Terran and Protoss route-call traces | `afb5ec58059756d3135f6d0bde5fb43b41891be9c3f07a797ef1ff982ed66621` |
| External-SRAM paired ARM probe replay | `781be0359a7cd30f03ba233de3f5f2cb81be3c63fdb2e2aa7711f1b2bac47b10` |
| Z10 far-read watch on both maps | `6eb1f671e0afca17527a12f3a844bb30f7808caba95e99c74abf0d7ff6bb3ff5` |
