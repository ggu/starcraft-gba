# Repeated unsuccessful point routes in Zerg 7

The read-only register observer runs the exact routing, census and
sparse-sight Z10 ROM from HOME. It records point-route calls, their
returned booleans, point-field insertion and prefix expansion at
instruction edges. All 131 historical checkpoints, the 480 frame and
route records, both timing counters and nine raw world and display
cuts match the unobserved run. The walkability hashes match at all
nine sampled input boundaries. [Prior orders breakdown](2026-11-19-z7-order-cost.md)

| Observed work | Count in calls 47,758–48,237 |
| --- | ---: |
| Ordinary point next-step invocations | 2,442 |
| Returned a usable step | 1,657 |
| Returned failure | 785 |
| Logical route builds across all kinds | 614 |
| Point-field miss acquisitions observed | 448 |
| New fields parked as prefixes | 284 |
| Full point fields made during acquisition | 151 |
| Complete retained point fields restored by swapping | 13 |

All 284 new prefixes correspond to successful next-step calls. The
151 full point-field builds all fail to yield a step. Of the 13
retention hits, 12 still return failure. The full builds consume
245,023 cycles per advancing update on average, including field setup,
flood and packing. Point-field acquisition overall averages 336,714
cycles. The wider next-step calls average 415,899 inclusive cycles.
Those scopes overlap.

All 163 failures associated with a point-field miss use only five
open-origin and goal pairs. After the first observation of each
pair, the remaining 158 repeat acquisitions consume 112,967,352
guest cycles. That is about 236,829 cycles per advancing update.
Use this as an opportunity ceiling for exact failed-query reuse.
The count excludes failed next-step calls without a new point-field
acquisition and excludes facility or range writers.

- Observed ROM SHA-256 `a45e83618e6f4ad870e8a3c85629dd20e1cbaa3a99413173e5de717c5f82fcf8`.
- Read-only register observer SHA-256 `760e45bc10a7b32679697895830acfa40ccf798dd63f43a60bafefdf242f759f`.
- Event receipt SHA-256 `2daf5661a85d425fedc4b6a6de9ce6085d01c1389002f55e6a30637e88fca359`.

## Opt-in reuse of exact failed requests

`SC64_ROUTE_POINT_NEGATIVE_MEMO` keeps a 32-entry table with complete
origin and goal keys for 128 × 128 maps. It records a failure only
when both cells are open and the result came from a complete field
at the requested root. A grid rebuild, topology change, shape change
or route reset clears the table. It adds 128 bytes of transient
EWRAM and leaves the save format unchanged. A memo hit bypasses
ordinary allocation and can change which guidance fields are resident
for later requests. Rechecking those later behaviors is part of
qualification.

Run `make test-sc64-route-point-negative` for the ASan/UBSan fixture.
The focused cases check the exact key, a successful query in the
same hash bucket, a successful step from a blocked origin, and
invalidation on every grid change. The full run checks 386,646 route
queries and 26,306 resident fields against independent floods.

The ARM candidate matches 131 ordinary-input checkpoints, all 480
frame/screen records, and nine sampled actor, visibility, palette,
framebuffer and display-control cuts. A separate fresh mGBA process
loads the authenticated existing late Z7 save and wins after 9,132
input calls at game update 194,998. All six sampled actor, palette,
display-page and page-selection cuts match the shipping-save control.

- Candidate ROM SHA-256 `32f96652bfed15f0c4e02e75aaf1bd305394c5bf61cbc0f07942eeac3ab25c85`.
- Candidate dense replay receipt SHA-256 `e57edee8e4e36fce274e51f172fb9d77aa2455dd984caeb93facb3386f331bda`.
- Late save continuation receipt SHA-256 `9923d07c5e0d3ab592a70fa61e4a328163386a77e946616af09f6d9f95c9055b`.

## Paired ARM timing

Both links use the same Z10 source, prepared inputs, route-prefix,
census and sparse-sight options. Only the memo define differs. The
control also exactly matches the older sparse-sight route and timing
counters in this window. The two new runs match each other in all
recorded frame/screen states, sampled actor changes and raw cuts.

| Dense Z7, 477 advancing updates | Memo off | Memo on |
| --- | ---: | ---: |
| Mean cycles per update | 2,048,123 | 2,004,511 |
| p95 cycles per update | 2,741,649 | 3,219,117 |
| Slowest update, cycles | 4,258,918 | 4,012,758 |
| Mean order cycles | 1,111,345 | 1,067,588 |
| Cycle-based emulated updates per second | 8.19 | 8.37 |
| Logical route builds | 614 | 626 |
| Route hits | 2,420 | 1,898 |

Mean update cost falls 2.13%. The p95 cost rises 17.4%, and total
logical builds increase. Keep the memo experimental. Test a complete
component witness with deferred materialization so an unreachable
request can still take its usual slot and update its age. The 30 fps
target, other dense fights and real GBA timing remain open.

- Same-source control ROM SHA-256 `28e7dd6852ec8e00a98fef6a76de32c14976d7c60f52719644f5c60f7527e73b`.
- Same-source control receipt SHA-256 `5bb1f63cfaea8b883642b891235b1aee1adee43f11946a0d44fc67b5eb5295ee`.
