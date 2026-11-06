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

## Path-grid rebuild and bulk-grid trial

A follow-up on the same exact packaged input adds two diagnostic-only
counters around the complete `build_path_grid` function, including terrain and
building overlays. The 500-call selected persisted-state/screen/mode stream
matched the exact delivered-ROM control. The diagnostic ROM SHA-256 was
`4d0eadcdde38c9395911add7d0779eca19278c955702adac2b751d94b19de8c4`;
BSS/IWRAM ends were `0203debc/03004918`, with existing guards intact. Its
mean elapsed cycles were 611,274 versus the 611,692 control (−0.068% net),
which reflects instrumentation and placement variation rather than a speedup.
Source and all 392 package dependencies remained hash-verified.

Four grid rebuilds totaled 3,612,663 inclusive cycles. Calls 223, 255, 373
and 402 spent 900,184, 902,118, 903,979 and 906,382 cycles respectively in
the grid rebuild; call 256 had none. These values are nested within the orders
and other intervals, and timer/IRQ costs may overlap. They identify a material
part of the previously unclassified spike; they do not establish the cost of
all other work in those calls.

The exact shipped Z9 vector lacks `SC64_BULK_TERRAIN_GRID` and walking-word
sprite optimization; normal Terran, early Zerg, Protoss and Showcase vectors
include the existing path-grid capability. These are separate capabilities.
For imported race and custom maps, the scalar `terrain_blocked` lookup and
`sc64_base_blocked_map` both select bit 0 of the corresponding retained map;
procedural and Showcase cases use the scalar fallback. The existing complete
grid ASan/UBSan fixture passed 33 scalar-versus-packed map/overlay cases using
the actual package's hash-verified generated inputs, including supported
campaign and custom maps and the procedural arena. This matters because a
previous prepared bank differed in map-blocking bytes from this package.

A separate normal-vector pair changed only `SC64_BULK_TERRAIN_GRID`, with
all diagnostic guards off. Control ROM SHA-256 exactly matched the shipped
`d86f1e23...`; the bulk-only ROM SHA-256 was
`c5e50ced4e434f8ae916a0f8c8c7113092d86b708ce3437c49589a1c9e218f47`.
Both linked within guards with identical BSS/IWRAM ends
`0203dce4/03004b20`. All 500 selected persisted-state fingerprints, frame
advances, screens and modes matched from the freshly produced ordinary-input
seed; the package control reproduced the prior per-call cycle stream. Mean
elapsed cycles were 611,691.662 control and 606,218.422 bulk (−0.895%). The
largest observed call fell from 3,102,721 to 2,527,936 (−18.5%). These are
emulator results for one passive-human custom opening, not hardware FPS or
complete game/campaign qualification. The physical backend cannot compare the
guest framebuffer or palette; these checks do not establish full transient
state or visual equality. No production flag is changed by this diagnostic
branch. Before proposing default enablement, qualify exact full state and
composited pixels on representative campaign and skirmish inputs, both pages,
scroll/fog/overlays, all applicable late-Zerg guarded release links and broader
busy/sparse performance, while retaining the known strict Z7 campaign failure.
