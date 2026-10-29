# Exact shortest-step shortcut

The runtime at `1b687154b4f0ec849db74d7255490424ca99b3be` can build several
complete reverse route fields in one update. A 55,000-tick host replay found
1,818 movement field builds and two reachability builds. The worst update
expanded 183,201 cells across 19 movement fields. The last 5,000 ticks account
for 1,489 movement builds. This identifies movement misses as the main source
of this workload's route churn, rather than formation reachability checks.

## Contract

Complete exact-goal and cached-suffix hits retain their lookup and age
behavior. On a miss, the shortcut searches only cardinal steps that decrease
Manhattan distance from a goal seed to the origin. A successful path attains
the global distance lower bound. Horizontal-before-vertical DFS selects the
same preferred path as the current reverse FIFO L/R/U/D flood. For a blocked
goal, only the walkable 3x3 seeds at the minimum Manhattan lower bound are
tried, in their original seed order. A longer or unproved route uses the
existing complete flood. Blocked origins retain the existing fallback.

The query permits at most 512 search iterations across its eligible seeds.
Its rectangle bitmap and iterative stack borrow the existing BFS queue only
for the synchronous call. No search survives a frame, no route slots are
removed, and no persistent BSS or save fields are added. This bounds the
shortcut, not the subsequent fallback or total frame cost. It is an exact
shortcut for this engine's route choices, not a claim about native N64 path
selection or an asynchronous pathfinding implementation.

`CANONICAL_ROUTE_PROBE_FLAGS` enables the shortcut in the delivered Terran,
Zerg, P7 and Showcase flags. Empty that variable for an A/B build. Partial
flood builds retain their existing implementation. Optional host diagnostics
require an explicit diagnostic flag and are excluded from ARM builds.

## Host checks

The standalone independent FIFO oracle passes 24,336 sanitizer cases,
including exhaustive 3x3 masks, random larger grids and active 64/96/128-wide
maps. The full 55k replay reproduces all 137 trace checkpoints and its host
stdout hash `3fb772b99e82813b453028feba4e78e4f887c476147b6d7fd87c7480ddb8aa2f`.
Movement builds fall from 1,818 to 822, expanding 7,651,931 rather than
17,260,190 cells. A rectangle bitmap limits clearing to 227,549 bytes across
18,145 probe attempts; 17,338 succeed and 347 exhaust the search budget.

`make test-sc64-route-monotone` exercises actual public route dispatch in
three asset-free sanitizer configurations. It checks an independent BFS
oracle, exact/suffix cache precedence and ages, blocked origins/goals, raw
and clamped endpoints, budget limits and scratch canaries, active geometry,
visit-grid restoration, and partial-flood exclusion. The default build and
full asset-free suite pass: 649 Python checks (20 skips), eight separate
Python checks and the C fixtures. The default ROM hash is unchanged.

## ARM measurements

The dense canonical replay compares the exact previously executed memo-on
image `205a143dbf33ee0eaa9998075d897c80ec640ab212e7bb26d59f7a0b2dcf70d4`
against probe image
`9e013e77ddcd47ee00d3373592f4e60366c18f6aedd521d7508c81b16ccb79a2`.
All 137 trace, three world and three displayed-pixel checkpoints match.
Window/profile sample counts, accounting residuals and terminal fields pass;
the owned process terminated and was waited on. This replay has sampled
imported audio disabled.

| Dense interval | Timed calls | Total-cycle change | Update-cycle change | Field builds before/after |
| --- | ---: | ---: | ---: | ---: |
| 1–25,000 | 24,153 | -0.5841% | -1.1536% | 291 / 187 |
| 25,001–50,000 | 24,765 | +0.0234% | +0.0291% | 40 / 36 |
| 50,001–55,000 | 4,901 | -13.5079% | -17.5005% | 1,489 / 601 |

Total measured cycles fall 2.2857%. The maximum update falls only 3.2503%,
from 42,984,485 to 41,587,381 cycles, still about 2.48 seconds. The largest
host update still needs 17 complete floods. This is useful progress, not a
bounded-frame or steady-30-fps result.

Normal-menu Blood Bath tests use the actual race-profile flags and sampled
audio, three mixed-race AIs, neutral human input and a fixed starting camera.
Each candidate reaches native game-over with world/RGB checkpoints identical
to its baseline. All owned emulator processes terminate and are waited on;
input pins remain unchanged. The three race baselines are byte-identical to
the archived, executed memo-on images, with the same emulator/configuration.
Showcase has a fresh baseline/candidate pair using its own captured flags.

| Profile | Calls | Baseline fps | Probe fps | Total cycles | Largest frame before/after |
| --- | ---: | ---: | ---: | ---: | ---: |
| Terran | 10,855 | 28.981 | 29.338 | -1.2146% | 299.6 / 133.9 ms |
| Zerg | 14,128 | 28.751 | 29.193 | -1.5132% | 217.6 / 183.2 ms |
| Protoss P7 | 12,876 | 28.781 | 29.058 | -0.9508% | 216.4 / 138.3 ms |
| Showcase | 12,876 | 28.189 | 28.474 | -1.0014% | 234.3 / 150.6 ms |

The fps figures derive from guest cycles. The host native-menu matrix also
checks every human race and 1–3 mixed AIs in each compiled race profile,
including world/pixel endpoints after 1,000 calls. These tests cover Blood
Bath, not all maps, cameras, active-player matches or complete campaigns.
Showcase's separate pair matches all seven world/pixel checkpoints and five
timing windows through native game-over at frame 12,888. Its host matrix also
covers all nine race/AI-count combinations. The runner rejects missing or
malformed timing, incomplete byte streams and mismatched checkpoint order.

## Provenance and limits

Fresh public normal builds match the frozen measured prototype in normalized
preprocessed code, loadable ELF sections and packaged ROM bytes. All 260
authored `common/` and `starcraft/` files match their captured snapshot.
Existing memory guards pass without changes. Zerg retains 16 bytes below the
IWRAM guard and Showcase 56 bytes. The source-boundary check passes on all
949 indexed text files, and the final whitespace check passes.

The first private canonical compile had stale include/linker/output paths
and was not executed. It overwrote an archived linker map; that text was not
reconstructed as original evidence. The failure and expected/actual hashes
are preserved locally. A root audit checks the other 1,232 historical pins;
the ROM, ELF, source, dependency and execution logs remain intact. Corrected
builds use owned source/linker/map paths, re-parse actual per-TU dependencies,
and rebuild a baseline byte-identical to the executed image. The lost map is
explicitly excluded from the new baseline evidence.

The Showcase host compiler's multi-source dependency output retained only the
last translation unit. A root preflight rejected that incomplete list before
emulator launch. Separate per-unit dependency outputs were then captured
from the same compiler options and checked against the frozen source trees;
the original file and failed preparation remain local. The ARM dependency
outputs are already separate per unit. Two identical candidate probe defines
in the private Showcase command are recorded explicitly; normal public
products have exactly one. Large preprocessed text intermediates were later
losslessly compressed, with original and decompressed hashes checked.

Private manifests, input tapes, imported assets and ROMs remain local. The
initial comparison summary mislabeled route hits as builds; a separate
corrected summary reads the actual build field, with cycle calculations
unchanged. No Actions, physical-hardware, full-campaign or fresh-process
SRAM-continuation result is claimed. Remaining campaign work and large route
stalls stay open.
