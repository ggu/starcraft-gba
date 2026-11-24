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

## Incremental coordinates during the search

The shared probe now counts horizontal and vertical steps remaining
toward the origin. Its bitmap index decreases by one on a horizontal
step and by the rectangle width on a vertical step. Each stack entry
records the node's search phase. On backtracking the parent's phase
identifies which step to undo. The search no longer converts the
current cell to a row and column on every visit.

The search keeps the original seed order, horizontal-first tie break,
iteration budget, scratch allocation rule and full-route fallback.
Under ASan and UBSan, 460,800 direct comparisons match the original
status, predecessor, and all five diagnostic counters for byte and
packed grids. The comparisons cover all 3×3 occupancy masks and
endpoint pairs at five budgets, as well as randomized rectangles,
unsupported inputs and short or odd scratch lengths. Scratch guard
bytes match. The three `make test-sc64-route-monotone` configurations
also pass the independent FIFO and dispatch checks.

The exact source runs on ARM7 Thumb in mGBA with eleven authored
cases. Each case repeats 64 times with the original and incremental
loops. Status, predecessor, and all counters match at both compiler
optimization levels and with both grid representations. The results
below use `-O2`, packed grids, and the production cartridge wait-state
setting of `0x4317`, read back by the fixture before each timed batch.

| 64-query case | Original cycles | Incremental cycles | Reduction |
| --- | ---: | ---: | ---: |
| Open 64-column grid | 2,665,538 | 1,897,602 | 28.81% |
| Open 96-column grid | 3,655,938 | 2,486,850 | 31.98% |
| Open 128-column grid | 6,390,274 | 4,682,114 | 26.73% |
| Open 95-column grid | 4,901,826 | 1,824,322 | 62.78% |
| Barrier requiring backtracking | 3,112,450 | 2,160,130 | 30.60% |
| Blocked goal with a walkable seed | 2,946,370 | 2,043,778 | 30.63% |
| Exhausts the 512-visit budget | 10,271,298 | 7,468,354 | 27.29% |
| Exhausts the 1,536-visit budget | 30,192,898 | 22,296,642 | 26.15% |

In the `-O2` packed fixture, text increases by 48 bytes and the
function's static stack allocation grows by 8 bytes. Data and BSS
stay the same. All eleven cases improve with the production wait
setting. With reset wait states, the one-cell zero-budget case takes
0.36–2.61% more cycles at `-Os`, less than 42 cycles per call. The
fixture measures the synchronous probe and its caller loop. Full
race-cartridge RAM and timing qualification remain open.

| Local ARM receipt | SHA-256 |
| --- | --- |
| Production wait states | `096a6ced99d4361f90681cdebdccf50367c272e924998db7524f7171d06778a7` |
| Reset wait states | `f81e50522bafd76ce187c81767314668376e3d7fba0751e0d894ce19e7f57878` |

### Shared dispatcher comparison

On source release `369b6008`, the original and incremental probes
run behind the same shared dispatch with sanitizers. Two packed-grid
configurations exercise active stride with and without retaining
fields across topology changes. Each runs 321,445 queries on
authored maps with blocked cells, edited terrain, rotating goals
and cache eviction. Both builds pass the independent FIFO oracle.
Each pair produces byte-identical semantic cache-state traces and
identical rebuild counts. The archived comparison covers 642,890
dispatch samples and has SHA-256
`512730c1ddefbc8cf249467563d37a1e64dbcfac9e33a9e913d79ed78570e49f`.

The [paired observed-query replay](../2026-11-24-kodak-package-checks.md#paired-arm-replay-of-observed-route-queries)
matches the original cartridge response on all 277 inputs and
saves 24.62% of ARM fixture cycles at `-O2`. The changed function
adds no persistent state. Together the dispatch and ARM checks
qualify the source change. Full race-cartridge RAM and pacing,
including timing on GBA SP, still need a paired release test.

## Retaining successful path prefixes

`SC64_ROUTE_MONOTONE_WITNESS` is an opt-in experiment for profiles
using the monotone point-route shortcut. The initial eight-entry
version retains the latest path after two qualifying successful
searches for a goal, with one bit per edge. Each entry can serve the same
tile again or follow the next tile toward the same goal. Qualifying
searches visit at least 32 nodes and take at least two steps. The
caller packs the path before returning the borrowed search stack.

The existing L-shaped fast path and cached route fields keep their
priority. A retained hit ages the route request at the original
point and still uses the current raw target pixel at the endpoint.
Grid rebuilds, route resets and shape changes clear the retained
paths and recent goals. The flag is inactive with partial-flood
routes or host monotone diagnostics. Diagnostic counters therefore
continue to describe actual probe work.

The sanitizer checks run the public dispatch and FIFO oracle,
including the opt-in byte and packed grids. The retained-path cases
follow all four directions and a blocked goal, reuse scratch between
calls, exercise a blocked L-shaped path, confirm cached-field
priority and check route aging. Reopening a tile changes the chosen
predecessor after rebuilding the grid. Shape rebinding clears the
witness as well.

An authored 64×64 ARM7 fixture uses `-O2` and the production wait
states. Four or eight interleaved streams take the returned step
toward their own goals around a barrier, with short requests mixed
in. The cold batch asks for 96 distinct goals once each. All 1,196
returned steps match with the flag on or off, both with and without
the L-shaped fast path.

| Workload | Baseline cycles | Retained-path cycles | Change |
| --- | ---: | ---: | ---: |
| Four streams | 4,068,922 | 1,308,556 | −67.84% |
| Eight streams | 7,889,384 | 2,384,966 | −69.77% |
| Distinct goals | 2,357,504 | 2,365,760 | +0.35% |
| Four streams with L fast path | 3,367,390 | 1,007,537 | −70.08% |
| Eight streams with L fast path | 6,436,402 | 1,964,303 | −69.48% |
| Distinct goals with L fast path | 2,469,104 | 2,477,360 | +0.33% |

Admitting every qualifying path gives a larger warm improvement
and makes the distinct-goal batch 12.93–13.55% slower. The two-query
admission keeps that miss cost below 0.4% in the fixture. The selected
configuration adds 360 bytes of linked BSS, 640 bytes of ROM text
and 40 static stack bytes in `sc64_route_monotone_next_step`. The
fixture's IWRAM code size is unchanged. Race-cartridge RAM reserves,
replay equivalence and whole-match effects still need qualification.

The same admission and replacement rules were also simulated from an
empty cache on 277 observed calls from the installed Terran Blood Bath
cartridge. The 96-update run matches the retained plain replay on
world, display and guest cycles. The eight-entry simulation covers
34 calls and 2.82% of full loop cycles. Twelve entries cover 86 calls
and 6.96%. These are observed costs of calls that could be reused,
before lookup, admission, or path packing costs. The run has one
observed walkability fingerprint. A larger count needs a separate
memory check.

| Local ARM receipt | SHA-256 |
| --- | --- |
| Admission on the second qualifying query | `0c5e5543490c32b6ede7c60ab1602bba081af7a5f283cb52f9b740398534d550` |
| Immediate admission trial | `f25e979eeeb63f5de940498115b6f9241cdc11e7ffbd3c6431856cd6141aad8e` |
| Exact Terran call identities and costs | `9acd03c2d3998302dff7e8f0d3c710121e8546153cf5d1ba5e19cd5f8b584ac5` |
| Offline admission and capacity comparison | `4bd2748c01f9afe2f2a4380108f52e33e15e6655f69c80a349bb289d75f8c24f` |

### Sixteen entries in the same RAM

The compact cache puts the two direction signs in the unused high
bits of the goal key and stores one map shape for the table. The
sixteen entries retain at most 112 edges each, starting nearest the
origin. When the retained steps end, the caller runs the full probe,
which preserves the current target pixels at the actual goal.
The entire table, including admission history and geometry, is 358
bytes before linker alignment.

The 128×128 sanitizer case follows 112 stored edges of a 254-edge
path, then verifies the next step against the independent FIFO
oracle and observes a fresh probe. A separate case fills sixteen
entries and checks that the next admission replaces the oldest
entry only. The existing direction, fallback, topology and aging
checks still pass.

The authored fixture adds sixteen interleaved streams and compares
the flag-off baseline, the original eight-entry cache and the
compact sixteen-entry cache. All 2,526 route steps match in each
configuration, with and without the L-shaped fast path.

| Workload | Flag off cycles | Eight entries | Sixteen entries |
| --- | ---: | ---: | ---: |
| Sixteen interleaved routes | 15,455,833 | 15,658,313 | 4,696,189 |
| Sixteen interleaved routes with L fast path | 11,482,145 | 11,067,096 | 3,843,827 |
| One request per distinct goal | 2,357,504 | 2,365,760 | 2,388,178 |
| One request per goal with L fast path | 2,469,104 | 2,477,360 | 2,499,778 |

The compact cache saves 66.52–69.62% of cycles in the sixteen-route
batches. The four- and eight-route batches still save 67.65–70.03%.
The one-off batches take 1.24–1.30% more cycles than the flag-off
baseline. Both cache layouts add 360 linked BSS bytes and 40 static
stack bytes in the same fixture. The compact version adds 24 ROM
text bytes relative to the eight-entry version. The linked IWRAM
size matches.

The observed late Terran Blood Bath probes that qualify for storage
have goal distances of 41–64 cells. Even a blocked goal's adjacent
seed leaves those paths within the compact limit. In the earlier
offline simulation, sixteen entries with two-query admission cover
99 measured calls and 8.04% of loop cycles before cache costs. A
matched whole-race ROM replay is still required to measure net gain.

The compact-cache ARM receipt SHA-256 is
`35f5962bd9d5ee8d43437e5d6fa3a386902d497cca137426ee342a0a96e8723b`.

The full host dispatcher also matches the incremental probe alone
over 642,890 authored route samples. Two sanitizer configurations
exercise active map stride with and without retained fields. Both
pass the independent FIFO oracle and emit identical cache-state
traces and rebuild counts. An instrumented rerun observes four
actual witness hits per configuration. Runs with sustained long
movement exercise reuse more often. Whole-cartridge qualification
remains open. The host receipt SHA-256 is
`933881e9bef623dc2c1667c36124883053753ab67d48b366dbebe7aa8fe3009c`.

### Route dispatcher chronology in late Terran Blood Bath

Two read-only captures of the exact November 24 Terran cartridge
recorded all 381 point-route dispatcher calls during the 96-update
window beginning at update 11,000. The captures matched the plain
control at each update on sampled world state, both display pages,
palette, camera and elapsed guest cycles. All calls succeeded and
used 9,924,632 guest cycles in total. The 104 route-field hits
advance the route clock. The other 277 calls match the independently
captured monotone origins, goals and returned cells in frame order.

The capture retains the initial 64×64 packed grid, the 14 logical
slots and the 86,016-byte pool of 48 physical fields. The full
pool hash is constant at all 762 before-call and after-call
boundaries. Between successive calls, 133 transitions change the
recorded route clock, slot ages or hit count outside the point
dispatcher. No slot topology change appears in those observations.

A conditional paired ARM replay restores the initial pool,
relocates its pointers and applies the captured clock, slot ages,
hits and builds before each call. It retains candidate prefix
entries between calls. Both variants match all 381 returned
coordinates, status values, post-call clocks, slot ages, hits
and build counts from the installed cartridge. The fixture uses
the non-inlined ARM dispatcher in IWRAM at `-O2`.

Guest cycles inside the timed call boundaries drop from
7,255,012 to 2,817,726, a 61.16% reduction in the fixture.
Thirty-seven of 96 per-update groups still cost more, with a
maximum increase of 4,813 cycles. The fixture uses selected
release flags and stand-ins for unrelated routines. It applies
metadata snapshots from the original interleaved activity
outside the timed region. A paired full game loop is still needed
to qualify frame timing and linked memory across the cartridges.

| Local capture | SHA-256 |
| --- | --- |
| All point-route dispatcher calls | `001e144d4d336358f0b669b9db114d87cce689fa599f3fb65cd2b6920c414bfc` |
| Initial private route state | `f7bc3f46b18cfac42da2e931c9847fb94b4f99d68d6f74699da87a878e40cae6` |
| Full route pool boundary checks | `1257bf6e413e9910056b1f62a1da3e2d518aad49312dbdc6a219ed03854385c7` |
| Paired ARM dispatcher replay with metadata checks | `7428bbfe88b54f0362dfa794555d2150d8c56019f734765819ea5746bda010ad` |

### Paired authored game-loop test

A standalone ARM route loop first moves 16 synthetic actors
for 48 updates per batch on an authored 64×64 grid. It exercises
a resident point field and six depot queries per batch. The
second batch also changes a tile and resets the route state
every eight updates. Both variants return the same 1,536 cells
and report the same field and depot hit counts. The witness
variant records 566 hits in the steady batch and 516 during
resets. Measured cycles fall from 14,887,194 to 4,150,644
and from 24,629,346 to 14,990,879 respectively. These batches
include initialization, resets and the candidate hit counter.

A second fixture uses the normal race-engine update and draw
loop with authored 64×64 terrain, 20 moving actors, two live
building placements and identical D-pad input. Blank display
data supplies the optional interface schemas. Both profiles
use the ARM dispatcher in IWRAM and differ only by the
prefix-cache flag.

All 280 updates match on the actor state, walk grid, clock,
route-field counters, both display pages, palette and display
state. Both walk-grid edits are visible in the samples. The
candidate admits up to 16 cached prefixes. An independent
breakpoint pass on that binary observes 148 live prefix hits,
100 before the first edit and 48 between the two edits.
The breakpoint pass reproduces the plain candidate's cycle,
world and page observations.

| Cycle measure | Baseline | Prefix cache |
| --- | ---: | ---: |
| Frame mean | 576,650 | 573,627 |
| Frame p95 | 581,088 | 581,957 |
| Frame peak | 2,028,301 | 2,024,568 |
| Updates above two VBlanks | 18 of 280 | 18 of 280 |
| Simulation mean | 240,444 | 233,617 |
| Simulation p95 | 281,517 | 274,167 |

The authored scene's simulation mean falls 2.84%, and frame
p95 rises 0.15%. The pair links with the standard working
RAM guards. The prefix cache adds 360 BSS bytes and 808 text
bytes in this fixture. Qualification for routine activation
still requires paired busy-window budgets and save checks on
the intended race profiles.

Moving the building placements earlier and enabling request
aging gives a second 280-update pair. All sampled outcomes
and route counters match, with admissions reaching 16
prefixes. Mean simulation cycles drop from 240,905 to 237,583
and both variants still miss the two-VBlank budget 18 times.

With the older preferred-L shortcut also enabled, neither
variant admits a prefix on that workload. The shortcut itself
cuts baseline mean simulation cycles to 224,653 and misses to
13. Actor state and display still match the aging-only
baseline, but route clocks differ on 279 updates. The shortcut
returns before the dispatcher ages or touches a field. Confirm
clock and slot policy before considering it for release.

A private follow-up places the shortcut after live-slot and
request-aging checks and advances the clock on success. In
the same authored aging workload, all 280 world, display,
clock and field-counter samples match the shortcut-off baseline.
Mean simulation cycles fall from 240,905 to 228,754 and
two-VBlank misses fall from 18 to 13. This experiment is
separate from the prefix cache. Before enabling it, exercise
resident exact and nearby slots, the recency boundary, blocked
endpoints and fallback. Also check diagnostic counts and
probe scratch limits. The authored frame fixture has no
resident route-field builds or hits.

| Local receipt | SHA-256 |
| --- | --- |
| Authored route loop with resident fields and resets | `97276056727236757bdd0150dade9fef0fa5e43120cdacdc15b5fc77adc9a586` |
| Authored race-engine frame comparison | `1ff4a3aeb2476857bd4a0b33fda98c66ae2b35fb69f274f9cce7e6f04f219a0b` |
| Read-only trace of actual prefix hits | `90b79f0361880b3224c8e8b5b51243f4ac53e20fb4f01531ba32504688a69055` |
| Paired game loop with route request aging | `3d63b6770ea06c81b4e0fc7e7be26b574a4bf9325f7dd7cc8cdce35aa5565f9b` |
| Preferred-L experiment with zero prefix admissions | `a82cfc684980ccc1d5a9ccf24d3db2d53fa77fadcf4865f7ea005e39edaacdb6` |
| Preferred-L experiment after slot and clock checks | `266a3c1bf98a99c9e0b589eb05f388b3c9bd39881bf083dffceef300217cf12a` |

### Clear-prefix and retained-path comparison

The clear-prefix probe and retained-path table can be enabled independently
or together. When a clear L succeeds, the combined path records the length
of its horizontal run. The table copies that run and the following vertical
run only when admitting a path. Obstructed searches retain the ordinary
DFS stack. Both encodings preserve edge order when the retained path is cut
to its last 112 steps toward the origin.

The combined sanitizer fixture drives the public dispatcher with a retained
path, a depot hit, a nearby point field and an exact field. The normal field
builds on recency rollover. A world reset empties the table, and closing and
reopening a cell on the path each invalidate witnesses before the next
dispatch. The topology checks use a synthetic grid rebuild callback.

An ARM replay compares four variants of the authored 280-update race scene
with route aging and two actual building placements. Every variant matches
actor state, walkability, route clock and counters, both display pages and
palette in every sample. Point and depot slot validity, roots and ages also
match. Both table variants fill all 16 entries and have byte-identical
retained-path state across all samples.

| Opt-in features | Mean simulation cycles | Frame p95 cycles | Frames above two VBlanks | BSS change |
| --- | ---: | ---: | ---: | ---: |
| Neither | 240,905 | 581,253 | 18 | 0 B |
| Clear prefix | 229,195 | 561,393 | 13 | 0 B |
| Retained paths | 237,586 | 582,461 | 18 | +360 B |
| Both | 229,023 | 561,393 | 13 | +360 B |

The clear prefix covers most of the improvement in this scene. Qualify it
first on the busy Terran, Zerg and Protoss windows. The combined option
still needs exact-product timing, memory and fresh SRAM replays. Local ARM
receipt SHA-256 `16aa76820fcfaf17aa8af99c1209a62b7c5ac9a3811f3063825f8485fe73e37a`.
