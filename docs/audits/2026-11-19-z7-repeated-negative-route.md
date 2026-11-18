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

The read-only instruction trace of the memo binary explains the
cost tradeoff. The input and result of every ordinary point query
matches the control. The memo answers 513 of those requests, while
retained-field acquisitions fall from 13 to 6 and real complete
fields increase from 151 to 168. The complete acquisitions now use
about 272,600 cycles per advancing update. A raw memo hit says little
about the overall benefit when the cache residency changes.

- Memo instruction trace SHA-256 `9c74af5429fc80e071aaa2cbe156006d4658f95315ef8123afbf82bd69579396`.

## Deferred field with ordinary cache insertion

An independent analysis labels the captured static walkability grid.
For all 163 observed failed acquisitions, the first eligible complete
resident field other than the victim proves the two open endpoints
are disconnected. The existing parked-prefix pool has spare capacity
at each of those acquisitions.

With `SC64_ROUTE_POINT_SEED_COMPONENT_PROBE`, a disconnected request
still chooses, inserts and ages the ordinary cache slot. The slot
starts with just the goal cell in the existing FIFO. A repeated move
can use a fresh disconnection proof. Other reads use the established
exact materialization path when the parked field cannot answer, and
the old build path remains available when the proof or a FIFO slot
is unavailable. Link-time EWRAM and IWRAM ends match the control.

`make test-sc64-route-point-seed-component` checks stale, parked,
missing and complete witnesses, then compares per-query cache state
across 386,646 route queries and 26,306 independent resident-field
checks per variant. The physical fill counters can differ because
the candidate defers fills. Logical cache state matches byte for
byte. The existing range-copy sanitizer fixture also passes all
8,320 cases with the new flag.

| Dense Z7, 477 advancing updates | Control | Deferred field |
| --- | ---: | ---: |
| Mean cycles per update | 2,048,123 | 1,822,508 |
| p95 cycles per update | 2,741,649 | 2,652,372 |
| Slowest update, cycles | 4,258,918 | 3,500,472 |
| Mean order cycles | 1,111,345 | 885,334 |
| Cycle-based emulated updates per second | 8.19 | 9.21 |

Mean cost falls 11.02%, p95 falls 3.26%, and the slowest update
falls 17.81%. Logical route counters match at every measured call.
All 131 historical checkpoints, 480 frame and screen records, nine
raw world and display cuts, and per-call sampled actor changes
match. The separate authenticated late Z7 save reaches victory with
all six raw world and display cuts identical to the control.
The deferred field is the candidate for further qualification.
Check dense skirmish and other campaign timing, fresh saves, and
physical hardware before enabling it in regular builds.

- Deferred-field ROM SHA-256 `2d9b4311908ee477bf711d425dc2a23ae45bac85ad86017f966064fcd8e2bfdb`.
- Dense replay receipt SHA-256 `0c0555eb675867fc641a17c0fe39c0768e9a999a9ecc341d2da104e71c6daae5`.
- Late save receipt SHA-256 `8110b55f5920b04225a86480f338ce0d598a4f43e2e6fa648bcefa26fee501f2`.

## Three-opponent Blood Bath check

The same two Z10 ROMs launch from blank SRAM with a human Zerg and
Zerg, Terran and Protoss computer seats. Blood Bath is 64 × 64, outside
the deferred wide-field path. Both runs advance through the opening
and measure 1,000 consecutive updates beginning at game frame 16,302.
All sampled logical counters, 25 world/route/display samples and the
32 KiB pause-save export match. The final sample follows a title-L
load in a new process and 180 more updates. The comparison includes
every sampled map page and selected page.

| Guest cycles per update | Control | Deferred field |
| --- | ---: | ---: |
| Mean | 795,983 | 796,128 |
| p95 | 1,038,189 | 1,035,642 |
| Maximum | 2,008,978 | 2,009,039 |

The mean changes by +0.02%. The 64 × 64 match needs separate throughput
work. Frame 17,056 is the slowest update in both runs.

- Control receipt SHA-256 `6a3965ff2efda55260333afaf6f1c88065571bbc9ece4a70cc2d8069caa1cc30`.
- Deferred-field receipt SHA-256 `ec8a426b02b2956acc7ee747f0bf65648d3482bbd898b8db448c052127b5c5cc`.
- Common save SHA-256 `820e7f5ca85072ca8d117e9848ac42c7cbf9f0d54987cc2aca8dd9e95649540d`.

### Separate 64 × 64 headroom

A read-only facility-call trace of the control matches every
frame, whole-update cycle count, orders counter and logical count
from the 1,000-update run. It observes 2,263 facility lookups and
110 builds. Facility lookup averages 19,473 inclusive cycles per
update, inside 290,210 orders cycles. It accounts for 745,478 cycles
on the slowest update, frame 17,056. The orders peak at frame 16,449
uses 1,428,270 cycles with just 5,472 spent in facility lookups.

The trace checks a carryover only when the previous update kept the
same active partial field, geometry, and packed bytes and the new open
origin has no direction in that field. Only one build meets the
observed carryover condition, with 70 earlier FIFO pops. Keeping
the existing facility frontiers alive across updates has little
measured headroom in this match.

Map drawing averages 244,689 cycles, including 114,794 for units
and battle effects and 72,407 for the background. HUD drawing adds
90,258 cycles. Profile these recurring costs alongside the
non-facility part of the orders spikes.

- Facility instruction trace SHA-256 `5881003bbf99080ffff34b7d279c5f4110215f37b309b787733dcbdd8b0d8e00`.

An additional instruction-edge trace samples every 25th update and
four of the largest update or orders spikes. It matches the
control timeline and counters across all 1,000 updates. In the 44
traced updates, tracer drawing averages 69,821 inclusive cycles.
The mean body-draw costs are 23,395 cycles for 3.5 calls with a
center inside the admitted rectangle and 10,581 for 87.3 calls
outside it. Death effects average 2,124 cycles over their two
passes. The traced attack-pose lookup has no calls. Separate
tracer culling and setup from pixels drawn before adding new
pose indexing or changing drawing rules.

- Render instruction trace SHA-256 `3dc2a7eb36e8f6bffeffaad2760a6eee6a19e5f1dca37c9e826ead600901d048`.

A local experiment replaces the facing-vector scan with an ordered
search over exact direction boundaries. The host oracle matches
16,785,409 directions within ±2,048 on both axes, all 32 large-delta
ties and their neighbors, and the existing T5 framebuffer fixture.
The ARM experiment matches all 25 Blood Bath cuts, logical totals
and save bytes. It averages 795,439 cycles compared with 795,983 for
the control, a 0.07% reduction. Keep the experiment local and use
the measured tracer workload for the next rendering comparison.

- Facing experiment receipt SHA-256 `d764a7fa66695b37da41c2f1709c5e820f364c0bc33fd0d359de9e5302a65615`.

Three finer instruction traces use the same 44 cuts. Each matches
the 1,000-update control's frame, cycle and route counters. The
renderer admits about 5.6 of 9.1 tagged traces per measured update.
The lookup and immediate cull checks average 608 and 2,553 cycles.
Work after acceptance averages 65,293 cycles. The overlay projector
averages 23,599 inclusive cycles, including 3,352 cycles of the
6,902-cycle raster total. Tracer division routines account for
6,433 inclusive cycles across these stages, all in 32-bit routines.
There are no calls to the partial-fog per-row edge in these cuts.

The camera stays fixed through this run. On 8,660 of 8,967 live
tracer observations, endpoints and keys match the preceding update
and remaining life decreases by one. Even with that reuse, caching
the existing cull results can avoid only about 2,846 cycles before
cache bookkeeping. Prioritize reducing the work on accepted effects.
The existing executable already enables its profile timing cache.
An explicit build define reproduced identical ROM bytes and timings.

- Tracer stage receipt SHA-256 `005d7de209c4b884234fc5e2f4895f1de54664774deb63ee634e5495d464496a`.
- Overlay and raster receipt SHA-256 `766856190dfd046d774d3583e45bf9ac0cd57ce4378a86ecf6e516c32566a2cc`.
- Tracer division receipt SHA-256 `c327a2a1d10f9e1f423ddd394ea468e71b3e257b238a78d7dbb0b5a63d90f9d6`.

The per-profile bounds bank also leaves whole-update cost flat. The
candidate generated header matches the control after removing only its
bounds version marker and table. The 1,000-update comparison matches
all 25 sampled cuts (including the fresh load), all frame IDs and route
build/hit counters, and every exported SRAM byte.

| Guest cycles per controller update | Control | Per-profile bounds |
| --- | ---: | ---: |
| Mean | 795,983 | 796,011 |
| p95 | 1,038,189 | 1,038,171 |
| Maximum | 2,008,978 | 2,008,930 |

The mean changes by +0.0035% in this mixed Blood Bath window. This trial
does not establish a performance benefit or qualify other maps.

- Per-profile bounds replay SHA-256 `702d354903c8cc0a8e8f578cb4be0b21ace0475db3d90859c88345a742a71a71`.
- Candidate ROM SHA-256 `4c04df50194fd5658f01ae1b4ecf1e292d5188ec5cfb53b58d66c8b5c987fdb8`.

### Managed index and empty overlay streams

The sampled route control uses the original lookup bank. Standard builds
already select the managed lookup index. Replay the same 1,000-update
window first with that index as the only link-vector change, then with
the same index and an early guard for empty overlay streams. The guard
avoids sampling the attacker's frame solely to anchor an empty stream.
Projectile Init, Flight and Death still choose their normal frames and
timings. Each skips only its empty overlay pass.

| Guest cycles per controller update | Original lookup | Managed index | Managed index + overlay guard |
| --- | ---: | ---: | ---: |
| Mean | 795,983 | 787,931 | 780,471 |
| p95 | 1,038,189 | 1,025,081 | 1,019,443 |
| Maximum | 2,008,978 | 1,990,887 | 1,981,107 |

The index saves 1.01% on this window. The guard independently saves
0.95% against the indexed control, with unchanged linked data and BSS
sizes and 16 added text bytes. All 25 world and display cuts, frame IDs,
route build and hit counts, and exact 32 KiB SRAM exports match, including
the fresh load and its 180-update continuation. The asset-free sanitizer
suite passes, as does the generated-bank fixture compiled directly with
the retained original and indexed headers. This run measures one fixed-camera
three-AI match in emulation. The two derived images used direct ELF-to-ROM
conversion and have no final boot-header fixup. They are timing probes for
the emulator, which accepts these images. The 30-updates/s budget remains
561,792 cycles.

- Indexed replay SHA-256 `982d97421d4ad397a79acc2eb8cb96099d7e055144966941377a9755a3d05f9b`.
- Overlay guard replay SHA-256 `89f51ae8cc506765eecb9a6fe18feef32407b1a0baecbe48a3a200ab563115c6`.
- Overlay guard ROM SHA-256 `2ab1bfd8a4802457c82b4b6b038b26abda7771357f1520fa314a76d15f6920ef`.

## Independent 128 × 128 Zerg 9 check

The same ROM pair replays 46,500 ordinary controller updates from
fresh HOME. Both open Zerg 9 on a 128 × 128 map and reach game frame
45,265. The actor, resource, campaign and display bytes match at all
68 paired samples and both runs match the 21 historical checks.
Logical route counts and clock match at every sample. The final
counts are 4,382 builds and 119,747 hits.

| Guest cycles per controller update | Control | Deferred field |
| --- | ---: | ---: |
| Mean across the whole prefix | 1,445,458 | 1,445,433 |
| Mean in the last 500 updates | 1,431,814 | 1,432,366 |
| p95 in the last 500 updates | 1,576,751 | 1,577,127 |
| Maximum in the last 500 updates | 4,444,782 | 4,444,554 |

The late-window mean changes by +0.04%. Qualification covers the
measured prefix through frame 45,265. Victory and fresh-load checks
for this mission on this pair remain open.

- Paired prefix receipt SHA-256 `57a5a7482b062d0b72bcfdcda63aadc8205960e20374179a75408ace83cba064`.
