# Owner-query candidate on current P8

This is an unpromoted source checkpoint. Runtime `aae2c29` combines the
owner snapshot, same-owner aura traversal and signed-safe sight cull from
`a24d845` with public P8 `60734e5`, including the qualified P8 condition
probe. The only merge conflict combined the two new test-module includes;
both remain included. No gameplay timing, source radius, actor cap, save
layout or product feature gate is intentionally changed by this candidate.

## Evidence

Literal normal `-g` ARM builds pass unchanged linker guards and ROM header,
SRAM signature, ELF payload and FF-padding checks. All 109 frozen prepared
input hashes remain unchanged. The actual endpoints and ROM hashes are:

| Product | BSS end | IWRAM end | ROM SHA-256 |
| --- | --- | --- | --- |
| P7 | `020382f4` | `030049e0` | `80b60c703b3483bf630f40fc79eebd7c46828b077b1785dbef71e3fc39af0090` |
| P8 | `02038cbc` | `030049e0` | `a2078fcd48a2fd2d91712f9a9d9f3e0fafa1c2f3fefebc826c69434ed83de5c8` |
| Showcase | `02036fe8` | `03004c78` | `2243d679f297d6225fa89e54753271f1b269ee50f9a06b8e4592df0cfa57446d` |

Each paired replay uses a retained natural SRAM through the ordinary loader
and the pinned port-free mGBA runner. Each comparison matches 901 captured
frames of complete Unit, Building, mineral, visibility/exploration, resource
bank, available P7/P8 mission state, service-clock, cache-due/initialized and
seed bytes. Nine displayed Mode 4 framebuffer/palette checkpoints match.
These are named fields and samples, not a whole-world or full-mission proof.

| Mean guest cycles over 900 samples | Current normal P7 | Candidate P7 | Current normal P8 | Candidate P8 |
| --- | ---: | ---: | ---: | ---: |
| Full loop | 1,054,739 | 1,061,605 | 1,678,587 | 1,663,919 |
| Update | 657,895 | 661,211 | 1,158,897 | 1,146,712 |
| Orders | 471,580 | 475,109 | 434,000 | 422,018 |

P7 full-loop p95/p99/max changes from 1,123,777 / 1,403,996 /
1,685,241 to 1,123,619 / 1,404,193 / 1,685,103. P8 full-loop
p95/p99/max changes from 3,089,253 / 3,651,360 / 5,562,636 to
3,089,392 / 3,651,052 / 5,589,291. Full-loop values include
presentation waiting. **P7 mean cost regresses, and P8's maximum worsens.**
The candidate is therefore held; the smaller P8 mean is not a general
performance result or evidence of near-30-fps hardware operation.

The P7 control ROM is retained runtime `8453909`, SHA-256
`0e1cd2c7c45ecb735b186a7a5ea40f1cc1761d5d21c6deabc9046b2769e33e8a`;
the P8 control is the qualified normal condition-probe runtime `71e7714`,
SHA-256 `54cfbe2da8169767424cc71172b1b6319d7be508fb2c58dec62225f7f734cf68`.
Earlier source-only tests at `a24d845` include the 3,307,910-case independent
signed-domain axis oracle, three detector-owner oracles, product service
fixtures, aura traversal/fallback and cache cadence checks; those are not
relabeled as fresh full tests of this combined runtime.

## Holds and receipts

Do not promote this candidate or substitute it for the verified race-ROM
package. Attribute and remove the P7 regression before admission. Combined
stack depth, fresh-process SRAM continuity, full normal P8 victory, clean
unprepared default checks and representative other workloads remain open.
No Showcase replay or current all-four-product proof is claimed here.

The first local verification wrapper attempted objcopy output through
`/dev/stdout`, which the macOS tool rejected after the P7 link had passed.
The corrected wrapper retained that attempt and verified the same artifact
through a uniquely named raw file, then built P8 and Showcase. A P8
comparison initially stopped before launching because its baseline path
was absent; the corrected driver used the separately retained, hash-matching
normal baseline. Neither issue was a runtime or compiler failure.

Literal arguments, logs, all captured frame digests, cycles, pixels, input
manifests and compression receipts remain ignored under
`.local/qualification/` in `protoss-sight-current-20261031`. Closed artifacts
were transparently compressed only after verifying content hashes, with
mode and mtime preserved. No originals, prepared banks or saves were removed.
No proprietary payloads, ROMs, captures, hosted Actions or SD writes form
part of this source checkpoint.
