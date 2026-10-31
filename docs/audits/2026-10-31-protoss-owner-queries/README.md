# Source-owner snapshots and Arbiter owner lists

This is an unpromoted performance candidate on public P8 parent `85c9a47`.
It does not promote P8, add a mission, alter unit values, change source tick
cadence or establish the near-30-fps target.

## Changes and invariants

Runtime `d487dee` builds the eight source-owner mappings lazily alongside
the existing powered-cannon snapshot, then reuses them for detection
refreshes during that source tick. The pass only writes unit cache words
and its service clock; the owner bindings and building state do not change.
The standalone detection query still builds a fresh owner map. The snapshot
uses eight local ints, with no persistent state or save-format change.

Runtime `cf97162` restricts the indexed Arbiter aura traversal to the
Arbiter's live-owner chain. The existing exact source-slot and owner check
still follows. `update_orders` rebuilds and validates the owner lists and
spatial index immediately before processing the cloak cache, with no owner
mutation between them. Invalid/unavailable index construction retains the
original full scan. `fc13c5b` adds an independent all-pairs aura check for
all five generic owner lanes and the unavailable-index fallback, including
raw-owner values that differ from the derived generic binding.

Independent source review found no concrete mutation/precondition blocker
in either change. The tests require callers of the direct cache helper to
build a current spatial index themselves.

## Completed qualification

At `fc13c5b`, ASan/UBSan checks pass for both 80-world randomized aura-union
variants and their generic owner/fallback cases; independent 2,000-world
detector-owner oracles for each of P7, P8 and Showcase; actual product-vector
Protoss service fixtures for those three products; and the cache cadence
fixture. All 109 frozen prepared inputs remain unchanged. The unavailable
Carrier/flight branch in actual Showcase is explicitly not credited.

The actual normal P8 ARM build passes the unchanged linker guards, header,
ELF payload, SRAM signature and 32 MiB FF-padding checks. BSS/IWRAM ends
remain `0203899c` / `030049e0`. Its ROM SHA-256 is
`5069554cba388ea19c8eef610192d0aaf9010a4789907c01854fa66973bb8141`.
The snapshot-only predecessor ROM was
`f4d57cee4cb9d0a7d2ad7e8cf911a97ee1924aee99791e12338165e132fb529b`.

A separate P8 compile-only stack-usage probe measured the snapshot-only
change's cache source frame at 568 → 600 bytes, and the process-due wrapper
at eight bytes in both versions. This is a local static-frame comparison,
not a complete call-chain/interrupt stack proof; the later owner-list change
still needs its own stack receipt.

The control ROM is the retained normal P8 runtime `8453909`, SHA-256
`1b2700822d8c362c57ac03e228303a58320dce41e98371fbc5cd06584cec2c30`.
Its gameplay sources under `common/` and `starcraft/` match public parent
`85c9a47`; the intervening changes are build/tests/audit and Showcase cache
selection. Each candidate uses the same retained natural P8 SRAM and pinned
port-free runner. Across 901 active frames the captured Unit, Building,
mineral, visibility/exploration, resource-bank, P8/P7 state, service clock
and cache due/initialized bytes match. Nine displayed-frame/palette hashes
also match in each comparison. These are named captured fields, not every
global or an entire mission replay.

| Mean guest cycles over 900 updates | Control | Snapshot only | Snapshot and owner lists |
| --- | ---: | ---: | ---: |
| Full loop | 1,732,890 | 1,731,642 | 1,716,974 |
| Update | 1,210,878 | 1,209,086 | 1,200,305 |
| Orders | 433,977 | 432,241 | 423,594 |

The combined candidate's full-loop p95/p99/max is
3,383,247 / 4,212,455 / 5,582,277, compared with
3,406,922 / 4,212,551 / 5,562,025 in the control. Thus the sampled full-loop
maximum worsens. Full-loop values include presentation waiting. These
workload-specific emulator results do not establish general improvement,
hardware performance, steady near-30 fps or campaign admission.

## Attribution and remaining gates

A separate `SC64_DIAGNOSTIC_UPDATE` P8 build at `fc13c5b` matches the normal
candidate's same 901 captured-state frames and nine pixel checkpoints.
Instrumented mean mission/production/race-state/defense/research/remainder
costs are 265,043 / 29,891 / 46,865 / 181,366 / 138 / 1,368 guest cycles.
Mission max is 2,183,016. The counters add overhead: these are attribution,
not a normal-product speedup comparison. They identify larger remaining
mission and defense work than the owner-query reduction.

Normal P7/Showcase links stopped at the explicit 2 GiB free-space prebuild
guard before compiling; an immediate check found zero mismatches among all
109 inputs. Those products, current combined stack usage, fresh-process
SRAM, clean unprepared default checks and broader workload qualification
remain pending. Complete P8 win evidence remains attached to its previously
documented predecessor, not this candidate. No release or SD update is made.

Literal arguments, hashes, maps, ROMs, every captured-state digest, sampled
pixels, per-update cycles, stack reports, guard failures and verified
transparent artifact-compression receipts remain under ignored
`.local/qualification/` in worktree `protoss-binding-perf-20261031`.
No proprietary data, ROMs, saves or captures are committed; no Actions run.
