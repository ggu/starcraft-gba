# Spatial defense target probe

The [Z10 cycle profile](2026-11-12-z10-battle-cycles.md) attributes up to
168,000 cycles per sampled update to repeated defense target searches. The
probe builds one fresh spatial index after mission changes and queries only
cells intersecting each defense's radius. It keeps the live ownership,
targetability and distance checks, strict radius and lowest-slot tie break.
Targets outside the indexed coordinate domain force the scalar path before
any cooldown or health changes. It remains opt-in.

| Workload | Mean loop cycles, stock → probe | Mean simulation cycles, stock → probe | p95 loop, stock → probe |
| --- | ---: | ---: | ---: |
| Z10 calls 19,000–19,999 | 1,568,109 → 1,523,417 (−2.85%) | 791,327 → 746,505 (−5.66%) | 1,965,123 → 1,919,401 |
| Z10 calls 35,000–35,999 | 1,995,868 → 1,876,614 (−5.98%) | 992,461 → 873,460 (−11.99%) | 2,375,134 → 2,258,408 |
| Z10 ROM, 64×64 custom battle | 1,368,916 → 1,368,954 (<0.01%) | — | 1,726,162 → 1,726,121 |

The normal ARM ROM reached all 34 paired campaign checkpoints with identical
raw unit, building and mineral arrays and matching named state. The custom
comparison matched 1,000 frame/screen rows and 21 sampled actor-count rows;
that mode does not use the Z/P campaign dispatch. The Z10 sanitizer test
compares 800 generated worlds with the scalar pass, plus cross-cell ties,
out-of-domain fallback and the exact attack radius boundary.

The ROM verifier passes with unchanged EWRAM and 256 more bytes of IWRAM
(`0x03004be0` end, from `0x03004ae0`). Probe ROM SHA-256:
`6dc1dc724615e37c687ff2c300530469eb342a9ee12d142ee6fa58ea87fbee7f`.
This profile-only comparison does not qualify normal default activation.
Before promoting it, check the tightest IWRAM and dynamic stack limits,
compose and remeasure with the separate Pylon probe, and qualify the other
campaign and custom defense dispatchers with complete ordinary-input
replays and both maps. Busy windows still exceed the two-frame budget.
