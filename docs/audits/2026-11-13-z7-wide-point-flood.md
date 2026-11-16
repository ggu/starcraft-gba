# Point route stalls in The Culling

The [qualified normal Z7 replay](2026-11-12-z7-normal-physical.md) was
repeated from HOME on normal Z10 and on the opt-in 96-column FIFO. Both
matched all 253 observations, won at frame 194,998, and averaged about
1,795,000 guest cycles per loop. Z7 is 128×128, so the earlier FIFO
does not service its point fields.

A read-only observer recorded the completed-update counter and the 64
largest loops in each run. The overall 58,707,335-cycle maximum on the
normal ROM is the final update, call 197,455, when the screen changes to
the victory result. This is separate from gameplay stalls. The largest
ordinary gameplay update is call 46,504, frame 45,262. At that call an
instruction and call-stack replay matches the measured cycle clock:
33,953,565 cycles fall under `sc64_route_fill` within movement orders,
out of a 37,547,713-cycle loop. It is a point-field problem, not a depot
lookup or drawing problem. The existing 96-column flag leaves this
update at 37,548,239 cycles.

## Use the same complete FIFO on wide maps

The wide probe reuses the same padded-byte expansion, seed order,
neighbor order and complete packed output as the 96-column version.
The 128-column layout reserves at most 6,144 queue entries instead of
8,192 to make room for its wider padded grid within the same scratch
area. Compaction keeps pending entries in order. Overflow falls back
without writing any destination field or live grid bytes. The 64×64
path, other widths, and route cache behavior stay unchanged.

`make test-sc64-route-linear` compares 386,646 host queries against the
existing packed flood. Both the normal and forced-overflow variants
match every cache observation and fill count. Open 96×128 and 128×128
fixtures compare every route cell and check the retained topology copy
before any fallback. Z10 ARM passes ROM and RAM guards: EWRAM is unchanged
at `0x02038b00`, IWRAM ends at `0x03004d68` (24 bytes above the earlier
linear probe and below `0x03005000`).

| Z7 call 46,504 | Normal | 96-column probe | Wide probe |
| --- | ---: | ---: | ---: |
| Loop cycles | 37,547,713 | 37,548,239 | 21,313,017 |
| Simulation cycles | 36,980,883 | 36,981,389 | 20,740,513 |
| Movement orders | 36,185,273 | 36,185,776 | 19,943,291 |

The ordinary replay matches the first 127 observations through that
call; the route build and hit counters also match. The gameplay spike
is 43.24% shorter. The complete wide run matches all 253 normal and
96-column observations and wins at the same frame 194,998 (call
197,455). Its mean drops from 1,794,976 to 1,679,547 guest cycles
(−6.43%, 9.35 to 9.99 emulated updates per second). Its worst ordinary
gameplay loop is 21,320,327 cycles at call 50,263, down 43.22% from
the worst normal gameplay loop. The separate result transition still
takes 57,817,607 cycles. On the 96×128 Z10 mission, the same wide cartridge
matches all 61 state checkpoints, 24 video/palette/fog samples and the
full win at frame 59,997. Its two measured window means are within 0.02%
of the earlier 96-column probe.

Terran, Protoss P10 and Showcase also link with unchanged EWRAM, with
IWRAM ends at `0x030048d0`, `0x03004c30` and `0x03004e80`. Their three
physical 64×64 save/fresh-load/continuation runs match the normal
controls, including resolved starts and identical SRAM bytes. Paired
128×128 campaign prefixes match nine world, route-cache, camera, fog and
video samples each. Terran T4 exercises seven wide point fills through
frame 1,476. Protoss P10 exercises eleven through frame 1,476, with seven
ordinary authored-message acknowledgments. These are prefixes, not
full Terran or Protoss mission wins.

Both complete Z10 custom matches with three independently selected AIs
also reproduce their normal control through defeat (76 and 77 state
checkpoints). The measured 64×64 custom window means are within 0.04%
of normal; the existing small-map FIFO remains the active path.

## Routine battle and menu cost

A separate normal-ROM instruction sample profiles 12 calls spaced 89
inputs apart from call 15,000 and matches 39 gameplay observations.
The eleven samples outside the command menu average 1,196,454 guest
cycles: rendering takes 700,872 and game update takes 462,315. Selected
large render costs include static map objects (124,926) and the generic
sprite blitter (111,752); these scopes can overlap and must not be added.
The one menu sample, call 15,623, spends 2,164,120 cycles drawing the
command list, including 1,070,811 in its row icons. The menu is a separate
workload worth an exact scaling/compositing check.

Wide probe ROM SHA-256:
`1abfd1e56dcc8c7a44522973ad37564136ccf1923b46e0b1486ac2ee718c2499`.
Full wide Z7 receipt SHA-256:
`4c136e7454ea30d6f59c5ab881f0a878d7688cbbb9eb7628b45cb8b52af7bf15`.

The option remains off by default pending hardware qualification.
The 561,792-cycle target
still requires a substantial reduction in routine simulation and draw
work. A separate selection-ring span probe matched all 35 gameplay and
24 video samples in the Z10 battle windows, but improved mean loop cost
by only 0.09–0.11%, so it stays out.

## Dense window on the exact KODAK build vector

The [stock stage sample](2026-11-12-z7-normal-physical.md#uninterrupted-home-on-the-exact-kodak-cartridge)
identified calls 47,758–48,237 as a sustained movement-orders load. A
second stock run matched all 131 prefix checkpoints and used read-only
breakpoints at point-fill entry, facility-field clear, and topology
change. Three slow calls (47,823, 47,855 and 48,073) each enter point
fill four times; two also clear one facility field. Call 48,064 clears
three facility fields and enters no point fill; call 48,222 triggers
none of those events. No sampled call enters the topology-change hook.
This distinguishes the sampled point-heavy calls from facility churn;
it does not count every kind of route work across the whole window.

The candidate adds only `SC64_ROUTE_LINEAR_FLOOD_PROBE` to the normal
KODAK `cbba1c5` build vector. It matches all 131 prefix checkpoints
and each of the 480 calls in frame progression, screen, route-build and
route-hit counters. There are 477 active simulation updates. Times are
guest cycles per call, measured with identical ordinary input.

| Counter | Stock | Wide FIFO | Change |
| --- | ---: | ---: | ---: |
| Whole loop, active mean | 3,766,477 | 3,029,745 | −19.56% |
| Simulation, active mean | 3,160,442 | 2,423,077 | −23.33% |
| Movement orders, active mean | 2,609,769 | 1,872,160 | −28.26% |
| Whole loop, all calls p95 | 7,060,165 | 4,704,489 | −33.37% |
| Whole loop, all calls max | 9,087,886 | 5,970,168 | −34.31% |

The active mean goes from 4.45 to 5.54 emulated updates/s in this
interval, still far below 30. EWRAM ends at `0x02038b00` in both builds;
IWRAM grows by 392 bytes to `0x03004d68`, leaving 664 bytes before the
link guard. The candidate captured raw world and valid route fields at
both endpoints. A matching stock raw capture and refreshed four-profile
links, save checks, and hardware timing are still needed for promotion.

- Stock route breakpoint receipt SHA-256: `a2b15bc6129089b9bf7df2dc27ef0266532951a6a5c4449c6d827e3d305c08fe`.
- Candidate stage receipt SHA-256: `afc895639478e972ea4a6a966d6b6fb2e6c2648cbde34499195cba8b4656b298`.
- Candidate ROM SHA-256: `415f0b994bf43b4e2481ba97bb1e46c06d714fa14da32514129032ac02f4804c`.

## Default qualification on November 14

A fresh stock capture matches both raw endpoints of the paired dense Z7
run: world banks, path grid, cache keys and ages, and the hash of every
valid packed route field. The 131 checkpoints, 480 per-call counters,
and cycle changes above all reproduce.

On current source `4194784`, the four stock and four wide ARM builds
passed the header and unchanged RAM guards. The proposed default build
commands are identical to the qualified wide vectors. No EWRAM grows.

| Product | Stock IWRAM end | Wide IWRAM end |
| --- | ---: | ---: |
| Terran T10 | `0x03004738` | `0x030048c0` |
| Zerg Z10 | `0x03004be0` | `0x03004d68` |
| Protoss P10 | `0x03004aa0` | `0x03004c20` |
| Showcase | `0x03004cf0` | `0x03004e70` |

Four paired Blood Bath skirmishes matched all 21 compared fields after
ordinary input, a pause-menu save at 600 requested updates, a fresh
process load and 180 continuation updates. The complete saves were
byte-identical within each pair. Independent named and Random seats
were covered, including a middle-only closed position and three CPUs.
The 128×128 T4 and P10 campaign prefixes again match nine samples
apiece, including rendered pixels, route state and all authored modal
acknowledgments. They exercised seven and eleven wide point fills
respectively. Publishing the flag makes the same route kernel available
in all four products; 64×64 maps still use their existing dense path.
Hardware timing and the 30 fps target remain open.

| Local receipt | SHA-256 |
| --- | --- |
| Paired raw endpoints | `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4` |
| Eight ROM and RAM checks | `a4953198be35777b7f59aa878a2d7b1dca3fafe3f1c9821bda6f48b515d1c0d1` |
| Four custom SRAM comparisons | `7d1778f12eaf26015be1fc6561c840994b5f22bbe75087c60ab9afac1d87e246` |

The packaged Z10 binary (`c251f81c`) also matches all 131 checkpoints
and all 480 frame and route-counter samples against the archived stock
run. Its mean active loop is 3.034M cycles, a 19.45% reduction against
that stock run; orders fall 28.10%. This is 5.53 emulated updates/s
in the dense window. The package receipt SHA-256 is
`f07f38751269a2aaf166df34e000e4c7c5b660ae5a8d0e6fd5e46c75982c87fe`.

## Pointer FIFO inner loop

ARM7TDMI can store one halfword and advance its address in one instruction.
The wide FIFO now reads and appends through pointers within each already
bounded batch; it keeps the stock four-neighbor order, queue compaction,
scratch allocation and overflow fallback. On Z10 the linked loop loses
20 bytes and IWRAM ends 24 bytes earlier, with the same EWRAM end.
The asset-free sanitizer target `make test-sc64-route-linear-pointer`
matches build and logical residency receipts for 386,646 queries per
configuration, including a 32-entry forced-overflow check.

Over the exact released Z10 dense Z7 window, the pointer candidate
matches 131 checkpoints, all 480 route counters and both raw endpoints.
Mean active loop cost is 3,033,769 → 2,941,972 cycles (−3.03%);
orders cost is 1,876,375 → 1,784,487 (−4.90%). The maximum falls from
5,993,556 to 5,583,925 cycles. The candidate runs at 5.70 emulated
updates/s in that interval. That is an improvement, still far from 30.

All four ARM products link and pass their normal memory guards. Four
Blood Bath save/load pairs match the previous wide ROMs including
full SRAM bytes. The paired T4 and P10 campaign prefixes still match
pixels, world data, cache state and dialogue and exercise seven and
eleven wide fills. The default applies to all four products; set
`CANONICAL_ROUTE_POINTER=0` to rebuild the indexed wide control.

| Local receipt | SHA-256 |
| --- | --- |
| Candidate Z10 ROM | `bfff158af9b6a00ebfaa96cd5e0a65c9e5a0fd508aed28c2400a56e0871fb8b3` |
| Pinned Z10 candidate timing | `1a22dcdced7c8c0f4cce03272301728bdc4fbe8609432a4fe19532636726cbe6` |
| Identical raw endpoints | `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4` |
| Four custom SRAM confirmations | `21f0956d4af37834a46c7c830ea9b4e7ee52f84ffb4c3f1d153e69c2042a7707` |
| Paired T4 and P10 prefixes | `be7683c82e38e94d691cf74c9690cd1b70f4e5a5ed19c2d4201918d441b19a78` |
