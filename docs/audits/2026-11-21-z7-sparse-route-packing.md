# Sparse route-field packing in late Z7

In the normal Z10 controller replay, 49 Hydralisks begin a 12-update
path retry at each of calls 153,946, 154,198 and 154,566. The retained
read-only GBA observations on `f69b2bbd` record walkable start cells
in the 9,790-cell main component and attack-move destinations in the
same disconnected 361-cell component. The three calls build 13, 14
and 13 route fields. The two early sampled calls retry in the opposite
direction toward a 9,625-cell component.

Sparse packing keeps the existing point route request, FIFO and completed
field. On a 128-wide field, it clears the packed field and writes only
the discovered cells if there are at most 384 and the queue could not
have compacted. Whole-field packing handles the other cases. Route
cache retention, movement and save encoding use their existing paths.
The GBA host fixture `make test-sc64-route-linear-sparse` checks 386,646
queries and 26,306 resident fields against the independent route oracle.
It also checks the 361-, 384- and 385-cell boundary cases, a blocked
goal with two diagonal seeds, map edges and a compacted full field.

## Exact Z7 replay

The normal `83edffe8` Z10 control and opt-in trial each create the same
32 KiB save by ordinary input at frame 5,641 and win after a fresh
boot with HOME L at frame 228,128. All 254 named world checkpoints,
ten dual framebuffer pages with palette and camera, and six complete
resident point cache cuts match. The timing comparison reconciles
the completed update count and cumulative `demo_total_cycles` against
every sampled `demo_last_cycles` in each window.

| Tape calls | Mean change | p95 cycles, control → sparse | Peak cycles, control → sparse |
| --- | ---: | ---: | ---: |
| 47,758–48,237 | −0.03% | 3,130,515 → 3,130,140 | 5,026,594 → 5,028,581 |
| 153,812–154,741 | −2.45% | 4,549,000 → 3,885,215 | 6,810,905 → 5,821,240 |
| 154,742–155,650 | −2.55% | 4,567,098 → 3,761,779 | 6,370,087 → 5,428,959 |

The 131 late samples with ten or more logical builds improve by
18.2–18.8% on average. The two no-build means change by at most 0.02%.
Across the full 225,208-call fresh suffix, mean emulated cycles change
from 1,312,138 to 1,307,936. Code adds 88 bytes to this ROM's text and
keeps both RAM endpoints. On-device presentation timing is open.

| Case ID | Required evidence |
| --- | --- |
| `z7-sparse-route-crowd` | Play exact normal Z10 from an ordinary pause export and fresh HOME L. Record the three build windows in the table, verify both route counters on each sample, and hash the resident packed point fields at 47,795, 48,149, 153,946, 154,198, 154,566 and 155,410. |
| `z7-sparse-fifo-fallback` | Repeat the host cut with a blocked goal, an isolated 361-cell component, both sides of the threshold and a large compacted FIFO. Compare every direction and unreachable cell against a fresh reverse flood. |

The first narrow measurement wrapper compared emulator clock totals
with the engine's completed-update counters and failed at the early
window. Use the completed-update boundary for the main-loop comparison.
Keep the emulator clock available for instruction-level traces that
reconcile their own guest cycle scope.

| Receipt | SHA-256 |
| --- | --- |
| Read-only GBA observations | `9824cbddec14821f9cc711e953f7e5717f51200e399d8c09bc993e09e2b81f9d` |
| Walkability component analysis | `a1ce75d934df77e459a1cd83a82c1d47ed7f8fa52e02d994f9a3eb68db25720f` |
| Host logical cache transcript | `78777abdbaf6ee8144d7f7facad3a3fc15183785df72af4b6803e1baef7d0be4` |
| Paired Z7 victory and timed windows | `0d99b62f18a80b7c298883ebebd0b4739a2c3e9a88f4cda3f673f6398877d48b` |

See the [late Z7 order budget](2026-11-21-z7-late-order-budget.md) for
the instruction profiles and the separate targeting measurements.

## Shared normal build option

The four normal imported builds enable sparse packing by default. Set
`CANONICAL_ROUTE_LINEAR_SPARSE_PACK=0` for an A/B control. The flag is
absent when linear routing or fresh-cell writes are disabled. The
canonical configuration key changes when sparse packing is active, so
switching the option rebuilds the affected product.

On `e92caada`, all four normal recipes link and pass the ROM header,
SRAM signature and payload checks with the prepared inputs. The trial
changes the normal link recipe by one compiler definition, with these
unchanged RAM endpoints.

| Product | IWRAM end | EWRAM end |
| --- | --- | --- |
| Terran | `0x030048a8` | `0x02038f5c` |
| Z10 | `0x03004d30` | `0x02038078` |
| P10 | `0x03004e20` | `0x0203b62c` |
| Showcase | `0x03004fb8` | `0x02039168` |

The four-product build receipt SHA-256 is
`805f6330b307a296f2318715706279f8eefac6ecce0a314941ded2782daef930`.

The `e92caada` Z10 replay with the targeting policy enabled again
matches 254 world checkpoints, ten display cuts, six packed cache
cuts and the full 32 KiB SRAM through victory. The two late windows
improve 2.60% and 2.71% in mean cycles. Their 95th-percentile costs
drop from 4,418,954 to 3,699,514 and from 4,427,492 to 3,606,158
guest cycles. The first window changes by −0.03% in mean cost. Its
maximum is 0.06% higher in the trial. The full comparison receipt is
`a871401a8b680975cb580d65b77c76101d6cde5d6b19bd9ef04021deb571af30`.

## Normal build qualification on `13de558d`

With the Carrier and sprite-depth changes included, the default build
commands and explicitly enabled commands match. Disabling sparse packing
removes exactly one compiler definition from each of the four normal
link recipes and the Z10 Carrier fact generator. The generated fact
table has the same SHA-256 as the control and passes 2,019 checks.

All four link with unchanged static RAM endpoints. The earlier Z10
endpoint table above applies to `e92caada`; the endpoint after Carrier
is `0x03004d60` for IWRAM and `0x02039b30` for EWRAM. The four-ROM
matrix includes eight ordinary Blood Bath and Outpost custom replays
and three campaign opening replays. Each control and default candidate
pair produces identical 32 KiB SRAM and identical fresh HOME L
continuation observations. The matrix receipt SHA-256 is
`8709f82d6aa6af7b16ac9ce2f0101b7a5572f17dc24d9e6d6d3b953f7cf55b23`.

The earlier `5dcacdce` Carrier check covers active and docked Interceptor
saves through fresh HOME L, with byte-identical saves and restored
arena records. The comparison receipt SHA-256 is
`df70cb775921ec29ccc73c2f644387c22f3403545842eda86be3469ea73d7331`.

The Z7 timing result measures isolated 128-wide point floods in the late
campaign. Measure a representative late P10 scene and a wider custom
map before using the Z7 tail improvement as a performance expectation
for those products. The 64-by-64 path and depot-specific packing continue
through their existing implementations.
