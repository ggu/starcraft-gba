# Nearest depot first

`SC64_DEPOT_FIRST_NEAREST_PROBE` ranks the live legal depots on a worker's
cache miss before querying a route. It tests the closest depot, with the
same ascending slot tie break as stock. If that route succeeds, no other
depot can beat it. If it fails, the old ordered search runs without
retrying that one slot. A valid cached home and the no-home sentinel
still return before any scan.

The two sanitizer variants exercise the real Z10 selector with and without
the flag. On three progressively nearer reachable depots, stock and the
probe select the same slot while route builds fall from three to one.
With the nearest depot behind a wall, both make two route queries and
choose the lower slot among the equally distant alternatives. Both cases
run through the scalar and valid owner-list enumerations.

The first-nearest query may touch the derived route cache in a different
order or skip earlier routes altogether. The selector proof alone cannot
establish that other route consumers move identically, so the normal
cartridge comparison uses the full ordinary-input campaign tape.

The stock runtime is `6a0e19b`; subsequent commits through current main
`8148cd5` only changed documentation. The candidate was built from
`8148cd5` with the same generated inputs and compiler arguments plus
the single probe define. Both normal Z10 builds pass the ROM verifier.
EWRAM remains at `0x02038af8` and IWRAM at `0x03004ae0`.

Stock SHA-256:
`52876a57eec98b077340dd50c25254cc217f5b56a2629c6a544905e2b38f2911`.
Probe SHA-256:
`c4d187e5d5f014263d9717f071967686cb1e47e1a7154fa8e2517556f8763596`.

The probe matched all 61 raw gameplay checkpoints and 24 drawn-page,
palette, camera and fog samples through the full physical campaign.
Both reached the same zero-fault victory at call 61,938, game frame
59,997, after nine ordinary text dismissals.

| Replay window | Mean loop, stock → probe | p95 loop, stock → probe | Maximum loop, stock → probe |
| --- | ---: | ---: | ---: |
| Calls 19,000–19,999 | 1,780,125 → 1,631,066 (−8.37%) | 3,071,462 → 2,845,031 (−7.37%) | 35,269,976 → 14,924,468 (−57.69%) |
| Calls 35,000–35,999 | 1,764,438 → 1,692,101 (−4.10%) | 2,805,196 → 2,546,575 (−9.22%) | 38,538,695 → 3,602,786 (−90.65%) |

The [previously identified depot stall](2026-11-12-z10-route-stalls.md)
at call 35,250 takes 2.98 million loop cycles in the probe, with eight
route builds and fourteen hits across the whole update. The earlier
window's new maximum occurs at call 19,531, previously attributed to
point routing. The probe reduces large stalls without putting the busy
windows under the two-refresh CPU budget.

```sh
make test-sc64-z10-depot-first-nearest
make SC64_DEFENSE_PROBE_FLAGS=-DSC64_DEPOT_FIRST_NEAREST_PROBE verify-zerg-z10
```

Use separate clean build outputs when changing the probe flag.
