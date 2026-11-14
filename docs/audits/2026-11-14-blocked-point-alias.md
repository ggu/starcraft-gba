# Reuse point fields for equal blocked-goal seeds

The optional `SC64_ROUTE_BLOCKED_ALIAS` probe runs on 128×128 maps. A
blocked point goal seeds its reverse flood with each walkable cell in its
clipped 3×3 neighborhood, in row order. If a fresh complete point field
has the same absolute seeds, the two floods visit the same cells in the
same order and write identical directions. The probe copies the field
into the normal victim and installs the requested goal with the stock
cache metadata and counters. It does not change blocked-start lookup,
because the caller still passes the original requested goal to that code.

The seed bounding box proves that every requested seed is inside the
candidate's neighborhood. Checking the candidate neighborhood outside
the request proves that there are no extra seeds. Empty seed sets, stale
fields, depot keys and the currently replaced slot are excluded. The
probe adds no storage and falls through to the regular scalar or wide
FIFO kernel when it cannot prove equality.

`make test-sc64-route-blocked-alias` runs an asset-free ASan/UBSan check.
The fixture compares every packed cell against an independent reverse
FIFO for two distinct blocked goals, verifies their separate logical
insertions, and repeats with an extra donor seed that forces a real fill.
The probe remains off pending a density estimate from a physical trace,
guarded links and paired Z7 timing and route-state checks.

`tools/sc64_z7_point_goals.py` supplies that density check without
changing the stock ROM. Its plan pins the KODAK ROM, ELF and tape and
checks the route-bank layout and 131 expected observations. The linked
physical observer adds one fixed read of `r0`, `r1` and `lr` while stopped
at a breakpoint. On the dense 480-call window the tool counts actual
point fills, same-grid repeat distances, topology barriers and eligible
same-seed donors in the resident bank. Repeated fills are an upper bound
on reusable work, not evidence that a cache of a particular size would
avoid them. Run only after the 5.25 GiB admission guard passes. This
register-enabled binary has linked against pinned mGBA; the dense
register capture has not run yet.

## Exact stock result

The read-only observer replayed calls 47,758–48,237 on KODAK Z10 with
all 131 checkpoints matching. There were 448 point fills over 308 of the
480 updates, to 11 distinct goals. **None had a resident donor with the
same ordered walkable seeds.** There were no topology resets in this
window. All 448 fills called from the ordinary miss at
`0x0802c8a0` (Thumb return `0x0802c8a5` in the pinned ELF), rather than
stale-field materialization. At least 437 fills repeat a goal already
seen in this window; 399 have a preceding same-goal fill within 15 point
fills. This is a point-fill interval, not the number of unrelated
evictions. For this workload there is no reason to add the blocked seed
alias code to normal ROMs. Measure retention of the same goal's field
before spending RAM.

The first attempt hit the file-size guard on diagnostic DMA messages. The
successful attempt used the identical register-observer executable with a
hash-checking stderr wrapper that drops only the routine DMA messages.
The replay guard and the read-only breakpoint protocol were unchanged.
The stock receipt is
`96c975a51fc0c4e54a234abe1195850120227c479ea46b94c0d83bfd1380f3b7`.

The separate point-only retention model observes the pre-fill victim at
that exact callsite. The 14 active physical mappings are identity and
stock has no donor planes. Of the 448 misses, 319 evict a fresh complete
point field. A shadow that retains one, two, or three recently evicted
fields would see 51, 92, or 132 hits respectively (11.4%, 20.5%, 29.5%
of point fills). The model does not redirect a logical slot or run a
candidate, and excludes evictions from facility and range work. One
extra map-sized plane costs 6,144 bytes before tags or transfer code;
measure against the wide FIFO and keep linker reserve checks before
using one. More than one does not fit the historical P10 guard.

Retention receipt:
`8f0a39d5b70702e6ce4572911199186100dfe8bb4d0eac57a40a69ada902b162`.
