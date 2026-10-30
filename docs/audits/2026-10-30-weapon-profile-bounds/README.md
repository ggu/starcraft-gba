# Profile-specific source weapon bounds — 2026-10-30

## Change and compatibility

This presentation-only optimization retains the original global envelope as
its first culling test. If that proves a trace is offscreen it returns without
any per-profile lookup. Otherwise a bank may provide a tighter conservative
ROM bound. No simulation field, save state, RNG, actor limit or reserve changes.
The existing four-byte derived global cache remains the only bounds cache.

The exporter optionally emits version 1 of
`sc64_weapon_source_profile_bounds[]`, one packed value per generated profile.
Only an exact aligned element of the immutable generated profile array may
select an entry. Copied/mutated profiles, NULL, missing/invalid metadata,
unsupported versions and legacy banks retain the global fallback. Wide
coordinate and invalid-direction cases retain the original global wide path.
Old banks continue to compile; regenerate a bank to enable the tighter bounds.

The union includes emitted projectile frames and all one-level child images
referenced by actor/projectile animations, including mirror/fallback frames.
Relevant direct offsets and all matching parent/kind LO offsets are included
across their frames and slots. Both direct and LO coordinates are retained for
legacy and corrected opcode-21 renderers. Unknown links, malformed ranges,
negative LO dimensions or packing overflow fail closed to the global bound.
Actor body art itself is drawn by another pass and is not included here.

See DEVELOPMENT's optional profile-bounds recipe. Generate into a new ignored
output directory; existing prepared bank data is not rewritten for these checks.

## Source and data identities

Base shared integration: `977fdb9e72ef859c1a731c2515edffa5c71fd112`.
Runtime global-first culling: `3b61b09120ee883500bc45c15d09244334177125`.
Selected-offset generation: `8d10cd4b3b6200c0a7efd471acd359f3e841570d`.
Clean source-only qualification: `6220c9b9d98ffd76231d9a847bce8b40dd0daa51`.
Compatibility hardening: `0b3977efe8b2099a1d646483633abaed53a941e6`.
Deferred-impact fixture correction: `7fd6c8082c37c94240947f726e15fee8d5cdd4e9`.

The old 12 arrays, defines, typedefs and lookup functions are byte-identical;
the bank adds only its capability marker and ROM bounds array. There are 61
specialized profiles and five global fallbacks. Measured v5 and final-generator
v6 headers are byte-identical, SHA-256
`0e03fcc54e860ab7d7a9904bcacdd47077045754294193ea7759dc9c949d9845`.
The final generator SHA-256 is
`ff6a66abf981726eea395c185e1372797b793cd0b4b27594af5780df62bacce0`.
No proprietary data is included in this audit or the repository.

## Qualification

- The actual-bank raster oracle passes 1,410,048 cases, including 495,648
  proven culls. The legacy bank's 344,832 proven culls remain separately scoped.
- The 3,526,824-case wide-arithmetic differential passes. Copied profiles
  retain exact global decisions; canonical wide-path cases retain equality.
  Canonical bounded cases may cull more, so raster equivalence is the oracle.
- Authored asset-free generator/runtime tests cover conservative child/LO
  coverage, unrelated offsets, malformed metadata, profile provenance, maximum
  bounds, unsupported/old banks and clip/wide boundaries. Final focused
  discovery passes ten tests after the compatibility change.
- Clean indexed source `6220c9b` passes boundary, doctor, default build and
  default test with numeric exit-0 receipts: 716 Python tests, 23 skips,
  separate eight-test suite and C checks. Later changes are the conservative
  generator compatibility case and test-only fixture correction. The final
  bank remains identical; those focused tests and fixture checks are separate.
- Four normal Make ROM recipes with the new bank pass unchanged memory
  guards, independent header/payload/padding/audio checks and 32-MiB checks.
  BSS/IWRAM ends exactly match the preceding four qualified BC products.
- Fresh four-profile frame/HUD checks, all four 66-profile/eight-heading/full-
  lifetime source renderer checks, unified checks and full Terran host aggregate
  pass. Four non-mine native cases and the legacy immediate-hit path pass after
  the fixture waits for production queued impacts. Damage assertions remain.
- Prepared per-profile input closures match before and after builds (35/30/31/30
  files), aside from the explicitly replaced and independently compared bank.
  P7 uses the previously corrected fingerprint header, SHA-256 `295a2ed0…`.
- Asset-free parent/candidate default ROMs are byte-identical, SHA-256
  `6e5556d7fbbab348ab54bdc34ea8a228077b2981817f27100a1dd9f9f90aea0d`.

The complete canonical native fixture exposes a preexisting mine/Zealot geometry
failure on the parent with the original bank too. This is not reported as green
or replaced by the renderer-only checks. A separate mine geometry fix is being
qualified; it is not included in the bounds optimization.

## Timing and scope

The same debuggerless ordinary-menu Zerg/three-AI tape measures calls 8021–9020
inclusive, 1,000 calls, with no gameplay injection. All 90 sampled selected-
state/fog/displayed RGB555 checkpoints match. The totals include presentation
wait and instrumentation; they are not physical-hardware/general FPS.

| Variant | Guest cycles | Equivalent calls/s |
| --- | ---: | ---: |
| Shared parent baseline | 723,533,683 | 23.18789 |
| Unconditional profile lookup, rejected | 723,208,024 | 23.19833 |
| Global-first, global offset union | 718,197,136 | 23.36018 |
| Global-first, selected offsets | 718,197,178 | 23.36018 |

The selected candidate saves 0.73756% in this scene. The 42-cycle difference
between the two global-first runs is negligible and is not an extra speedup.
The first variant increased effect-stage work and was superseded rather than
promoted for its tiny total change. Broader scenes and larger armies need their
own measurement; near 30 fps remains unfinished.

Private receipts in this worktree: `.local/bc-weapon-bounds-perf-v1/` through
`v3/`, `.local/build/weapon-final-v1/` and
`.local/weapon-profile-bounds/clean-qualification-logs-v1/`.
All validation ran locally. No Actions, debugger ports or SD writes.
