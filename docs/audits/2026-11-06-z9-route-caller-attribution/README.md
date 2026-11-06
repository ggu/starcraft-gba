# Z9 route-field caller attribution

This diagnostic-only source change investigates the current delivered Z9
custom window in `docs/audits/2026-11-06-current-z9-custom-phase/README.md`.
The new `SC64_DIAGNOSTIC_ROUTE_CALLERS` guard is off in product vectors. It
records field-build entry/exit cycles, caller PC, key, prior slot key/validity,
and a route-clear epoch in a 16-entry ring, drained read-only after every
update. Overflow or disagreement with the existing build counter fails the
runner. The diagnostic wrapper captures `path_next_tile`, `worker_depot_next`
and the last `invalidate_path_grid` caller; other direct callers retain their
own PC. These observations do not affect route decisions. Recorded invalidater
is the last observed caller, not a separately timestamped proof that every
individual epoch increment came from that caller.

The uninstrumented control rebuilt the exact delivered package ROM SHA-256
`d86f1e23753caa7ab50a93c544758041a0c35796f677abae6c4e137a276e44b3`.
The guarded diagnostic ROM was
`ececf2eddca8c80d49e951efce8f8d89da9e858bda30ccf0bbe0c7adbf801dfd`.
Both linked under unchanged guards: BSS/IWRAM ends were
`0203dce4/03004b20` and `0203deb4/03004908`, respectively. This is not a
stack high-water proof. An isolated source checkout used 392 hash-verified
package dependencies; they, the ordinary-button seed and both binaries were
checked before and after. The diagnostic and control matched selected
persisted-world fingerprints, frame advancement, screen and mode on each of
500 calls. This is not every transient state or guest pixel. Mean elapsed
cycles were 611,691.662 control versus 611,684.762 diagnostic (−0.0011% net);
instrumentation changes code placement, so per-build times are inclusive
observations, not exact isolated costs. No event ring overflow occurred.

## Results

| Actual recorded caller | Builds | Inclusive build cycles, total |
| --- | ---: | ---: |
| `race_resource_depot_reachable` via `worker_depot_next` | 48 | 2,560,182 |
| `move_toward` via `path_next_tile` | 14 | 7,618,062 |
| `worker_continue_minerals` via `path_next_tile` | 2 | 1,088,252 |

No formation or ranged caller produced a recorded field build in this window;
that is not a claim they never do. All 16 point-field builds used an invalid
slot. Of the depot builds, 16 used invalid slots and 32 rebuilt the same-key
partial field when the next origin was not covered; those repeated partial
builds totaled 1,887,970 inclusive cycles. No recorded victim was a valid
field with a different goal. Counts and cycles describe only these 500 calls.

The last tracked grid invalidater resolved to `spawn_build` for 56 events and
`game_init` for eight. Route-clear epochs progressed from 2 to 6; epoch 2
covered calls 2–196, epoch 3 calls 223–232, epoch 4 calls 255–296, epoch 5
calls 373–380, and epoch 6 calls 402–492. `spawn_build` calls
`invalidate_path_grid`, which invalidates both route fields and workers' cached
home-depot choices; this explains a concrete source-level mechanism for
repeated selection after topology changes. The event's last-invalidater PC and
an epoch change are not a complete trace of every invalidation reason.

| Call | Builds | Inclusive field-build cycles | Detail |
| --- | ---: | ---: | --- |
| 223 | 7 | 315,698 | depot keys 0x8001, 0x8002, 0x8003; epoch 3 |
| 255 | 7 | 372,280 | same depot keys; epoch 4 |
| 256 | 4 | 1,186,071 | depot key 0x8000 plus point keys 3518 and 3980; epoch 4 |
| 373 | 5 | 301,875 | depot keys 0x8001–0x8003; epoch 5 |
| 402 | 10 | 1,034,113 | depot keys 0x8001–0x8003 and point key 3980; epoch 6 |

At call 402, total orders were about 2.71 million cycles in the separate stage
probe; the recorded field-build intervals total about 1.03 million, leaving
substantial order cost unclassified. Values are from different diagnostic
builds and must not be treated as a precise subtraction.

## Bounded next candidate

Test, behind an opt-in guard, whether constructing a complete depot field on
the first lookup after an invalidation costs less than repeatedly rebuilding
bounded partial fields for multiple workers. The measured 32 repeated
partial builds are an opportunity, not a promised 1.89-million-cycle saving:
a complete flood may cost more, move work to a peak, alter slot validity,
recency and reuse, or change answers. Preserve identical goal roots, FIFO
order/ties, slot policy, invalidation and all actor/AI/update limits. Before
any use, compare each returned depot step and unreachable result against the
existing bounded and independent full-field oracle over blocked starts,
multiple roots, terrain/building changes, partial-field reuse, slot eviction,
save/load and all races; explicitly test interactions with cached-point and
range work. Then compare full per-update state and composited pixels plus
normal-vector mean and tail cycles on this window and other busy/sparse
ordinary-input scenes. Reject it if equivalence, memory guards or net/tail
behavior regress. Do not skip topology invalidation or increase cache slots
based solely on these observations.
