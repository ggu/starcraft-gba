# Word packing for linear route fields

The largest retained P10 gameplay stalls still spend most of their
time in worker depot routing after enabling the facility FIFO. These
are instruction-profiled ordinary inputs from the earlier paired
facility candidate, rather than a profile of the latest merged
source. Inclusive cycles are shown unless labeled otherwise.

| P10 HOME call | Whole loop | Display wait | Facility fill | FIFO expansion | Fill minus expansion | Fill self |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 9,674 | 19,792,901 | 25 | 17,363,925 | 6,781,522 | 10,582,403 | 10,305,154 |
| 11,045 | 19,432,711 | 25 | 16,985,958 | 7,178,707 | 9,807,251 | 9,550,877 |
| 32,644 | 24,678,587 | 25 | 16,734,544 | 5,894,165 | 10,840,379 | 10,556,656 |

The first trace shows the path through resource worker update,
nearest depot, depot reachability, facility next, slot lookup, and
facility fill. The conversion code is inlined in fill self together
with seed iteration and other bookkeeping; the table gives an upper
bound for conversion work, not a measured cost of the output pack
alone. Call 32,644 additionally spends 4,282,611 cycles in the scalar
point fill.

The original Z7 call 46,504 has the same 25-cycle display wait and
33,953,565 cycles in the scalar point fill. The independent
[wide point FIFO qualification](2026-11-13-z7-wide-point-flood.md)
already cut that whole update from 37,547,713 to 21,313,017 cycles;
the [composition replay](2026-11-13-composed-performance.md) does
not enable that option. Word packing can affect Z7 point work only
when the wide point FIFO is also enabled. Neither those historical
Z7 gains nor the P10 fill-minus-expansion values are gains measured
for this prototype.

`CANONICAL_ROUTE_LINEAR_WORD_PACK=1` opts into a common compositor
for the existing facility and wide point FIFO paths. For each 32
interior cells it computes four existing 24-bit groups and writes
three aligned 32-bit words in little-endian order. Other destination
alignments or widths retain the byte writer. The output is still
written only after successful expansion: seed order, whole-pop stop,
tie order, overflow and fallback, route cache and visible update
timing retain their original behavior. The probe adds no persistent
RAM or IWRAM placement and does not change the queue or scratch
layout.

The direct operation count is 12 byte stores versus three word
stores per 32 cells and one inner-loop iteration versus four. A
96×128 field changes 4,608 byte stores to 1,152 word stores; a
128×128 field changes 6,144 to 1,536. The unchanged packing math
still gathers the same directions from dense scratch. Shift and
register costs, generated instructions, ROM size and guest-cycle
benefit need a target measurement.

`make test-sc64-route-linear-word-pack` uses only asset-free
ASan/UBSan host fixtures. The output fixture independently composes
bits for 64-, 80-, 96- and 128-column padded rows at all four output
alignments and checks surrounding sentinel bytes. Facility fixtures
also compare packed bytes, cache reuse, validity, walkability,
diagnostics and forced overflow. Point fixtures compare both normal
and forced-overflow query and cache observations with the scalar
reference. The option remains off by default.

## Exact composed P10 comparison

The later, bounded ARM comparison uses the retained composed P10 ROM
(`6ac056503bf28b8ce7af09b3313b90b78f764cfd9a3fd805e75ae12d5bd08c97`).
The isolated candidate applies only the production header delta to the
same source (`153ae6b0`) and adds the word-pack macro to the same archived
compiler command; facility FIFO, direct census, and typed reader stay on.
The candidate verifies as a 32 MiB ROM. IWRAM ends at `0x03004c70` and
BSS ends at `0x0203c0e8` on both. Text grows 440 bytes; data and BSS
sizes match.

Both fresh ordinary HOME replays match through 32,645 calls: 13 HOST
checkpoints, ten briefing pages, seven authored acknowledgments, and
eight ordered samples of both indexed pages, palette, fog, cursor,
actor and mineral pools, route metadata, and active packed route bytes.
This stops before victory. Only the four specified calls were traced.

| Call | Whole loop, control → word pack | Facility fill, control → word pack | Control pack loop → candidate pack function |
| --- | ---: | ---: | ---: |
| 9,674 | 19,775,810 → 19,311,830 | 17,363,836 → 16,907,580 | 5,347,896 → 5,122,841 |
| 11,045 | 19,246,413 → 18,972,183 | 16,986,428 → 16,563,167 | 4,956,517 → 4,747,933 |
| 11,046 | 1,702,308 → 1,679,593 | No fill | No pack |
| 32,644 | 24,649,859 → 24,173,790 | 16,734,571 → 16,267,103 | 5,478,285 → 5,247,900 |

The pack column compares the control inner loop's instruction addresses
with the candidate's full helper scope, so the roughly 4.2% reduction
is conservative. FIFO expansion changes by at most 82 cycles, and
explicit display wait stays at 25. The no-fill control update also
changes in whole-loop time while its measured simulation differs by
one cycle. Use the facility scope for attributing the route gain.
Prefix maximum falls 1.93%, from 24,649,332 to 24,173,263 guest cycles.
Mean falls 0.027%, from 1,639,374 to 1,638,936. This is a small peak
improvement on P10 and no material sustained speedup. The option stays
off; no wide Z7 or other product replay was run.

- Candidate ROM: `a6226bd9419213043d449eca8d682f32f91ae6eee1d37821d995cb77c1a38a13`.
- Control replay: `b5d94f99d11842314c6a03f9b7ad4c8d9be887b1f2206a9ecf9de42bb424a5fa`.
- Candidate replay: `3b9cc6315510930e16be1aa65b4bee8470fb41671d04d7c8fd6cc963acf59539`.
- Matched ordered state/video: `5fd998f18b6ea21bd9b49142fa30a0bc151eb30eb28a8493f62c42a249b4d9d6`.
- Profile comparison: `5916b98489e072d4198de92f7536180bb6cf5f591c655db0daefbb857e55ee4a`.

## Dense Z7 on the pointer FIFO

On `ecc8a6ac`, the stock pointer FIFO ROM replayed calls 47,758–48,237
with read-only breakpoints at the wide point fill and expansion entry and
return addresses. All 131 ordinary HOME checkpoints and all 480 frame,
loop and orders counters matched the earlier pointer timing. Across 477
active calls, its 448 point fills average 736,160 cycles per active call:
111,655 in grid setup and seeding, 489,348 in FIFO expansion and 135,158
in output packing and return. Point fills account for 41.25% of orders
and 25.02% of the full loop. The rest of orders costs 1,048,326 cycles
per active call in aggregate; the breakpoint trace does not attribute
that remainder to individual functions. The worst loop (call 47,823)
spends 3,123,144 cycles in four point fills, including 2,078,957 in
expansion.

The comparison ARM build uses the same compiler command and adds only
`SC64_ROUTE_LINEAR_WORD_PACK_PROBE`. It matches all 131 checkpoints,
frame progression and route counters on all 480 calls, and the raw
world, walkability, route metadata and every valid route-field hash at
both endpoints. The normal and 32-entry forced-overflow pointer FIFO
variants with word packing also match the scalar reference on 386,646
asset-free ASan/UBSan point queries apiece; both facility packing fixtures
pass. The 32 MiB Z10 ROM passes its guards; text grows by 400 bytes with
unchanged EWRAM and IWRAM ends (`0x02038b00` and `0x03004d50`).

| Guest cycle counter | Pointer baseline | Word packing | Change |
| --- | ---: | ---: | ---: |
| Whole loop, active mean | 2,941,972 | 2,926,725 | −0.52% |
| Movement orders, active mean | 1,784,487 | 1,769,346 | −0.85% |
| Whole loop, all calls p95 | 4,420,534 | 4,362,853 | −1.30% |
| Whole loop, all calls maximum | 5,583,925 | 5,515,951 | −1.22% |

The result establishes a small improvement on this tape. Keep the
option off pending comparable evidence from the other products; this
experiment does not replay a full win, saves or hardware timing. FIFO
expansion remains the largest measured part of point fills. The earlier
stock victim trace placed every one of its 448 point fills in logical
slots 1, 2 or 4, away from the two range scratch planes. That makes
pointer-only retention a better next trial than another packed copy:
confirm the same victims on the pointer ROM, then exchange a safe
victim's backing pointer with one extra plane. Keep logical keys, ages
and eviction unchanged, and leave both scratch bindings fixed. Check
all four RAM reserves before timing; if the extra plane cannot fit,
focus on reducing expansion cost without changing FIFO order.

- Pointer breakpoint receipt: `52d7288321c40742c2bf6fdae57ed90ed83c8dbde8c362b52f35ff9d3ab9cb62`.
- Word-pack cycle receipt: `c25fd850c3960f0b277bbc61625986e43e9cc78d66df8b89adbd9fe28cce8d9b`.
- Matching raw endpoints: `12665bdcd588a704e58296b07cdc5aad4d87cc5c4679007881b52f77740a0cd4`.
- Word-pack ROM: `0c0811c8efc9b98f7fcd08f5733fb2c6f69f7a4b3a37bbe310a2de530014a5e2`.

## Current Z8 and P10 pause replays

Both comparisons use the normal attacker-image-depth runtime from
`a9f0137c` and the same prepared source inputs. The only compiler
flag added to the trial is `SC64_ROUTE_LINEAR_WORD_PACK_PROBE`.
The trial stores the same complete field during the same route
request and keeps the existing seed order, FIFO, capacity checks,
and cache publication. Unaligned outputs retain the byte writer.

Z10 loads the ordinary Z8 pause journal from HOME at tape call
17,122 and simulation frame 16,659. It follows the same ordinary
inputs and three modal acknowledgments through call 33,900. P10
loads from HOME at call 37,705 and simulation frame 36,761. Both
P10 ROMs win at call 47,194, frame 46,057.

| Profile | Timed updates | Mean guest cycles | p95 guest cycles | Peak guest cycles |
| --- | ---: | ---: | ---: | ---: |
| Z10, Z8 continuation | 203 | 1,685,501 → 1,678,389 | 3,311,417 → 3,276,408 | 12,272,717 → 12,092,659 |
| P10 continuation | 114 | 2,100,789 → 2,099,802 | 3,193,685 → 3,168,459 | 4,754,975 → 4,709,504 |

Z8 call 26,346 saves 143,962 guest cycles and call 33,800 saves
180,058. P10 calls 43,604, 45,231, and 45,452 save 43,055, 45,471,
and 14,831 respectively. The Z8 middle window's maximum adds
888 cycles. The P10 window containing call 45,231 adds 240 cycles
at p95. Aggregate timings cover the selected windows and do not
measure a whole-campaign frame rate.

All 32 named samples match the world, fog, display page, palette,
and both indexed pages. All 337 sampled resident fields match at
6,144 bytes each, including all 14 logical slots across each
profile. All 317 timed frames match route build and hit counts.
The aligned writer stays in ROM and both RAM endpoints match
the corresponding control.

| Profile | Control ROM SHA-256 prefix | Trial ROM SHA-256 prefix | Comparison receipt SHA-256 prefix |
| --- | --- | --- | --- |
| Z10 | `d9b38cc619c0f969` | `970cd016b85715f8` | `5a7ab1a9dee026db` |
| P10 | `85ef7abb9a96f726` | `769293f4122db540` | `dac279df68183c01` |

The shared configuration selects the aligned writer when the
linear point or facility FIFO is enabled. Set
`CANONICAL_ROUTE_LINEAR_WORD_PACK=0` to rebuild with the byte writer.
The compile configuration key distinguishes the two settings.

## Four-profile default qualification

The default change at `cd60befd` composes with release `0811942b`.
The same runtime and build definitions are present at `691c9ebc`.
Dry runs for all four normal verification targets select exactly one
`SC64_ROUTE_LINEAR_WORD_PACK_PROBE` flag. The explicit opt-out removes
that flag and selects a separate build configuration key. The original-art
skirmish recipe does not select the flag.

With the prepared source bank pinned, each normal cartridge passes
the header, SRAM signature, and unchanged linker guards. Z10 and P10
reproduce the exact measured trial ROM SHA-256 values above. IWRAM and
EWRAM endpoints match the corresponding pretrial builds in every profile.

| Profile | Default ROM SHA-256 prefix | IWRAM end | EWRAM end | Mixed skirmish comparison SHA-256 prefix |
| --- | --- | --- | --- | --- |
| Terran | `1ff78b901e13b6af` | `0x030048a8` | `0x02038e5c` | `0e7b2a69159035c9` |
| Z10 | `970cd016b85715f8` | `0x03004d88` | `0x02037f78` | `a40da287b9927c3a` |
| P10 | `769293f4122db540` | `0x03004e20` | `0x0203b52c` | `65a3c31fc73854aa` |
| Showcase | `9c2e62a0d7240a72` | `0x03004fb8` | `0x02039068` | `4bf60cfee57c842b` |

All four enter mixed three-AI Blood Bath from the ordinary lobby.
The [retained Bunker control](2026-11-20-terran-bunker-opening.md#current-release-with-death-depth)
matches 35 named opening samples on Terran and Showcase and 36
on Z10 and P10. The active construction and four-passenger pause
journals match byte for byte in each profile. Both journals restore
the same world, resources, and assignments in a fresh process, then
match the next 600 uninterrupted updates. The sampled fresh windows
run 0.005% to 0.155% slower against that retained control. This is a
64×64 skirmish save and gameplay check outside the affected wide
packing path.
