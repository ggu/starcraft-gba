# Incremental range-field primitive

`starcraft/sc64_range_work.h` is a source-only, caller-owned BFS worker. It is
connected through the shared game adapter in `sc64_routes.h`; this page
describes the standalone primitive. The caller supplies a read-only bit-packed
walk grid, a writable 3-bit gradient field, and scratch storage. No map-sized
storage is allocated by the worker, and it does not use the engine's shared
`bfs_queue`.

For `N = cols * rows`, required capacities are `ceil(N/8)` walk bytes and
`ceil(3N/8)` bytes each for the gradient and scratch. Each frontier plane
starts with `B = ceil(N/8)` data bytes. If `B > 8`, it appends occupancy
summaries of `ceil(B/8)`, `ceil(ceil(B/8)/8)`, and so on until one byte remains.
The second plane follows the first. `sc64_range_work_scratch_required(N)`
reports the exact combined capacity. For every `3 <= N <= 16,384`, the two
hierarchical planes fit in one packed route field; smaller geometries are
rejected. At 128x128, each plane occupies 2,341 bytes and both use 4,682 of
the field's 6,144 bytes. For maps with `N <= 64`, each plane stays flat and
the pop scan is bounded by eight bytes.

`begin` and `reset` touch only the small state object. After begin, a query
stays `PENDING` until gradient clearing and seed enumeration finish, so
poisoned old bytes cannot be observed. Caller buffers must remain valid and
disjoint until reset, including while completed results are queried.
When `cols` is a power of two, the flood worker maps a popped cell to its row
and column with a mask and shift. Other widths retain the general division and
remainder path; supported non-power-of-two map shapes remain unchanged.
`pump(budget, &used)` consumes no more
than `budget` units. Seed enumeration scans only the clamped target ± maximum
range rectangle, then applies the exact inclusive squared Euclidean range
test to each candidate. Its candidate count is at most `N`. A unit clears at
most 64 bytes, examines one candidate tile, or pops one BFS node and checks
its four neighbors. Large-plane pops descend at most four summary levels; each
set/clear propagates through at most four levels. Thus sparse layers do not
rescan all `N/8` frontier bytes. The consumed frontier bit is removed
immediately; the plane is empty when reused for the next layer.

Queries distinguish `ERROR`, `PENDING`, `READY`, and `NO_PATH`. A walkable
origin is ready once its gradient is known; a seed returns itself. An
undiscovered origin remains pending until the entire seeded flood is
exhausted, after which it is no-path. For a blocked origin, ready returns a
4-bit mask of currently discovered walkable cardinal exits (E/S/W/N in bits
0..3); the caller chooses an exit and handles collision policy. Reset is
required after any topology change; stale buffer contents are inaccessible
until the next begin's bounded clear and seed phases complete.

For a particular target, at most `ceil(ceil(3N/8)/64) +
ceil(scratch_required(N)/64) + C + V` units are consumed, where `C` is the
clamped seed-box area and `V` is the number of flood cells discovered. Since
both `C` and `V` are at most `N`, `ceil(ceil(3N/8)/64) +
ceil(scratch_required(N)/64) + 2N` is a finite upper bound. Phase transitions
add at most `N+3` constant-time checks, since each of at most `N` BFS layers
advances once. These are completion bounds, not recommended frame quotas.
The engine adapter chooses a per-update quota; that work-unit quota is not a
whole-frame cycle guarantee. See `audits/2026-11-01-range-work/README.md`.

This primitive does not solve scheduling multiple target keys, starvation,
moving-target snapshots, cancellation policy, or how a combat caller should
react to `PENDING`. The shared adapter handles those integration decisions; the primitive itself
makes no engine behavior or SRAM claims.

Asset-free verification command:

```sh
make test-sc64-range-work
# Also included by ./dev test. Equivalent direct compiler invocation:
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  tests/sc64_range_work.c -o /tmp/sc64_range_work
/tmp/sc64_range_work
```
