# Incremental range-field primitive

`starcraft/sc64_range_work.h` is a source-only, caller-owned BFS worker. It is
not connected to the game loop. The caller supplies a read-only bit-packed
walk grid, a writable 3-bit gradient field, and scratch storage. No map-sized
storage is allocated by the worker, and it does not use the engine's shared
`bfs_queue`.

For `N = cols * rows`, the required capacities are `ceil(N/8)` walk bytes,
`ceil(3N/8)` gradient bytes, and `2*ceil(N/8)` scratch bytes. The two
frontier bitsets occupy `[0, ceil(N/8))` and `[ceil(N/8), 2*ceil(N/8))` inside
one packed route field. For every `N >= 3`, their combined size is no greater
than `ceil(3N/8)`; geometries with fewer than three cells are rejected. At
128x128, the planes use 4,096 bytes in a 6,144-byte field.

`begin` and `reset` touch only the small state object. After begin, a query
stays `PENDING` until gradient clearing and seed enumeration finish, so
poisoned old bytes cannot be observed. `pump(budget, &used)` consumes no more
than `budget` work units. A unit clears one byte, examines one candidate tile
for the exact inclusive squared Euclidean range band, or processes one
frontier byte (at most eight cells and four neighbors per cell). The flood
clears each current-plane byte as it consumes it, so that plane is empty when
it is reused as the next layer.

Queries distinguish `ERROR`, `PENDING`, `READY`, and `NO_PATH`. A walkable
origin is ready once its gradient is known; a seed returns itself. An
undiscovered origin remains pending until the entire seeded flood is
exhausted, after which it is no-path. For a blocked origin, ready returns a
4-bit mask of currently discovered walkable cardinal exits (E/S/W/N in bits
0..3); the caller chooses an exit and handles collision policy. Reset is
required after any topology change; stale buffer contents are inaccessible
until the next begin's bounded clear and seed phases complete.

The total work is finite: at most `ceil(3N/8) + 2*ceil(N/8) + N + N*ceil(N/8)`
units. The last term bounds a full packed-byte scan for each of at most `N`
shortest-path layers in an arbitrary maze. This is an intentionally generous
completion bound, not a recommended frame quota. Choosing the per-frame quota
and integrating a pump remain separate work.

This primitive does not solve scheduling multiple target keys, starvation,
moving-target snapshots, cancellation policy, or how a combat caller should
react to `PENDING`. Those remain integration decisions; this file makes no
engine behavior or SRAM claims.

Asset-free verification command:

```sh
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function \
  -fsanitize=address,undefined -fno-omit-frame-pointer -g \
  tests/sc64_range_work.c -o /tmp/sc64_range_work
/tmp/sc64_range_work
```
