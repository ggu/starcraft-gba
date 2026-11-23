# Batched target index geometry checks

The initial experiments below precede the
[canonical release qualification](#canonical-release-qualification).

The shared target index rebuild used to validate the same map dimensions
for every actor. The optional owner-list builder validates them once,
then checks each actor's coordinates and links the resulting cell. It
keeps the stock insertion order, dirty-head tracking, invalid-coordinate
fallback and conservative occupancy masks. The standalone builder and
cross-cell movement still use the checked insertion path.

| Normal Z10 campaign window | Mean loop cycles, stock → probe | Mean update cycles, stock → probe |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,532,236 → 1,518,653 (−0.89%) | 791,066 → 777,731 (−1.69%) |
| Calls 35,000–35,999 | 1,914,170 → 1,897,618 (−0.86%) | 991,873 → 975,629 (−1.64%) |

Both cartridges use packed walking and the split terrain cache. With the
same 32 KiB SRAM and physical controller input, all 34 gameplay checkpoints
and 24 drawn-page, palette, camera and fog samples match. The baseline
ROM is `b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`;
the probe is `e9cad13c53f31c2170abcd7dbff2f856f5c6e8cb3758027d9b42e63d0872c207`.
Both pass the Z10 ROM verifier and original memory guards. The probe adds
344 bytes of linked text, with unchanged EWRAM and 144 bytes less IWRAM.
A separate build of this source with the probe off reproduces the exact
baseline ROM.

Asset-free ASan/UBSan differential checks run stock and probe in the
24×24, 32×24 and 32×32 target grids. The six configurations cover 60,000
randomized builds plus exact dirty-order checks, partial and invalid
geometry, loaded or reassigned units, cell crossings and overflow recovery.

Use a clean build tree for each flag setting:

```sh
python3 -m unittest discover -s tests -p test_sc64_target_owner_index.py
make SC64_DEFENSE_PROBE_FLAGS=-DSC64_TARGET_BULK_REBUILD_PROBE verify-zerg-z10
```

The default remains off. Next measure the other cartridge layouts,
long custom matches and the separate cross-cell move cost. These Z10
battle windows still exceed the 561,792-cycle frame budget.

## Current flag-off ROMs

Rebased on `e22f6cc` with the probe flag off, all four guarded normal
ARM links reproduced the exact staged ROMs, including the default P10
nearest-depot and typed-save changes. The six sanitizer configurations
also passed again. The table above measures the earlier Z10 candidate;
this rebase does not qualify enabling the option by default.

| Normal ROM | SHA-256 |
| --- | --- |
| Terran 10 | `6f869a5e295aa95d9ecfecebc0f18eba4be4b8ce5b5f5a7ee67a6ba664304a0c` |
| Zerg 10 | `ef36f3f0e8f44ea1207ef39c05ac1417c727b498a2596e93b98b765c74b9768e` |
| Protoss 10 | `0505b233994dca90cda47d92f1a153a14f01e781d980b3498390d3b4005e7c41` |
| Showcase | `5b9cf1ec7cee3809cec4a84130e60d656a26d9a2c4706d6c83f793bb98d33adb` |

## Reuse validated cells and occupancy bits

The optional fast link skips duplicate owner-axis writes when a cell is
already occupied. Those masks only clear with the index. A cross-cell move
also reuses the cell checked against the current map instead of computing
and checking it again. The owner and dirty-list overflow checks still
precede or guard the relink. Rebuild order, hostility, and tie rules are
unchanged. Nine ASan/UBSan vectors compare the regular, bulk, and fast
variants across the three geometries (90,000 randomized index builds).

Compare both probes on the current regular wide-lookup Z10 ROM, running
22,000 physical-input calls in Zerg 9. All 12 named checkpoints, 21,987
sampled frame identities, and 34 raw world/cache cuts match, including
all 370 route-field observations. Both ARM links pass the RAM guards
with unchanged EWRAM. IWRAM ends at `0x03004d70` in the regular build,
`0x03004ce0` with bulk rebuild only, and `0x03004cd0` with both changes.

| Guest cycles per advancing update | Regular | Bulk only | Bulk and fast link |
| --- | ---: | ---: | ---: |
| Whole prefix mean | 1,319,069 | 1,306,607 | 1,301,649 |
| Whole prefix p95 | 1,613,855 | 1,603,353 | 1,596,834 |
| Busy tail mean | 1,720,501 | 1,706,404 | 1,700,064 |
| Busy tail p95 | 4,255,649 | 4,244,721 | 4,232,251 |
| Peak | 22,395,567 | 22,392,690 | 22,394,404 |

The bulk and fast path cuts the whole-prefix mean by 1.32% and the tail
mean by 1.19%. The peak is effectively unchanged. This is a bounded
campaign emulator result, not a GBA SP rate. Keep the probes off in regular
builds while checking other products, longer mixed skirmishes, and saves.

```sh
make SC64_DEFENSE_PROBE_FLAGS='-DSC64_TARGET_BULK_REBUILD_PROBE -DSC64_TARGET_FAST_LINK_PROBE' verify-zerg-z10
```

| Local artifact | SHA-256 |
| --- | --- |
| Bulk Z10 ROM | `16c576e86d965c7870c186bb676e921553cf4112727cd172cbea0e40556b7d64` |
| Bulk and fast Z10 ROM | `48b971185e63b58e61466c64afa5eafcb2fea39c103b4cbe7a9af5f0bbd7bff5` |
| Paired Z9 comparison | `e88924f3b9903966ee17a15e42563a459cb5f3be284fdfe335cccd42ac9d4046` |

## Canonical release qualification

The four canonical ARM recipes enable both exact index updates. The
same recipes accept `CANONICAL_TARGET_INDEX=0` for a reference build.
The local configuration stamp tracks both settings. Earlier campaign
previews and generated inputs keep their own flags.

The reference cartridges match the release with the defense and
saved-command changes. After enabling the index options, the same
physical P7 tape matches all 1,000 frame, mode and route observations
and all 11 world, fog, framebuffer and palette samples. There are 958
advancing updates in the sample.

| P7 replay scope | Reference cycles | Updated target index cycles |
| --- | ---: | ---: |
| Mean whole update | 1,921,336 | 1,901,329 |
| 95th percentile whole update | 2,215,763 | 2,196,884 |
| Peak whole update | 2,587,134 | 2,566,152 |
| Mean orders stage | 913,404 | 893,781 |

Mean whole-update cost falls 1.04% on this mGBA workload. The nine
sanitizer variants pass, and all four links retain the EWRAM bounds
while freeing 160 IWRAM bytes per product. Eight physical reads pass
in both save directions across both maps, preserve the nondefault
command and the same 32 KiB SRAM, and match 180 continued updates.

On the updated Terran ROM the unchanged Outpost tape also wins after
fresh title load at the same frame 41,951. The match verifies all 56
remaining action observations after the save cut.

P10 wins the full P7 replay at frame 55,892 after 57,678 physical calls.
All 59 gameplay checkpoints, 11 briefing pages and 10 in-field pages
match. The same ROM also [wins P2 and P3 after fresh title loads](2026-11-18-saved-repeat-command.md#early-protoss-campaign-continuation)
at the previously divergent cuts.

| Local receipt | SHA-256 |
| --- | --- |
| Composed P7 timed comparison | `66a6a7f86744a9aae0d5d2d23e817dc9cfea43662264dbdf9a75bda3c56a0b97` |
| Canonical link guards | `bc219f6fbb25a9a292d7581446542df01de251769f1dc7e583aa7216156f8a08` |
| Bidirectional skirmish saves | `dd68a5697c8c2f108a84ee9c653fbd40ee36d36b5067d0a937348ffce9f441ac` |
| Outpost fresh load through victory | `cba959cbf24af140bcbe84cc9c43469a4e9805b83845da7549f17cf16efe28a3` |
| Full physical P7 victory | `ceb663b9638c3d0b83bc65747ae6d08376ebcc5bfe9ba722c5dbc1d9a2db3bc3` |

## Retention and fused setup on dense Z7

Draft [#590](https://github.com/ggu/starcraft-gba/pull/590) checks whether a
target chain can be reused without changing scan order. The early-rejection
variant first checks whether the movement flag could have disturbed that order.
A separate trial builds the owner lists and a fresh target index in one pass.
Each pair matches 131 gameplay checkpoints, 480 frame and route-counter
observations, and both indexed pages, the displayed page and the palette
at all eight sampled cuts. Compare timing only within each control pair.

| Trial | Whole update mean | Whole update p95 | Peak update | Median change in paired orders cycles |
| --- | ---: | ---: | ---: | ---: |
| Retention control | 2,340,889 | 3,389,340 | 4,696,376 | — |
| Original retention | 2,340,034 | 3,405,579 | 4,709,175 | +12,434 |
| Early rejection | 2,334,112 | 3,402,619 | 4,694,904 | +7,194 |
| Fused rebuild control, `81f62111` | 2,338,639 | 3,390,991 | 4,692,183 | — |
| Fused rebuild, `f07c3599` | 2,352,696 | 3,401,407 | 4,714,382 | +13,601 |

The table uses 477 advancing updates and nearest-rank p95. Early rejection
saves 0.29% of mean cycles and raises p95 by 0.39%. The fused rebuild slows
orders in all 477 updates and uses 64 more IWRAM bytes. Keep both trials
outside the default build.

| Local receipt | SHA-256 |
| --- | --- |
| Retention control | `6b11688c76f516bbb0aaa0fff501b1299ee8cfd6707b20abb268c4ecdc57073c` |
| Original retention | `60a70ce28204be31052a02f9c9f7ff4a2bd22ada2090321d5363bf97cb976aba` |
| Early rejection | `6a2f52db50a638b8bbdd9e4f22971123da874af3d38bafc48ff5120a4e148a9a` |
| Fused control | `a1a008ed9dffdd9fb6c8294bf0373fe990c83796070dc6045d33d523ebf4b89d` |
| Fused rebuild | `2d34beb6788dfe386e510047cddd33f8b2fdf3a648890b206763c311cdf0d9d0` |

## Batch the fresh dirty-cell count

The optional `SC64_TARGET_BATCH_DIRTY_PROBE` holds the dirty-cell count
locally while rebuilding from validated owner lists. Each entity can add
at most one entry to the empty dirty list. A compile-time bound covers
the case where every unit and building occupies a unique cell. A partial
rebuild still publishes its occupied prefix for the next clear. Relocation
and the standalone builder retain the checked counter.

All 12 sanitizer configurations pass across the three target grids.
The oracle compares ordered links and dirty lists, invalid geometry,
crossings and overflow recovery, and now fills every actor slot with a
distinct cell. All four ARM links retain their EWRAM and IWRAM ends.
The P10 build with the trial disabled reproduces the current release ROM
byte for byte.

| Matched P10 workload | Mean guest cycles with cursor | With batch count | Change |
| --- | ---: | ---: | ---: |
| Earlier 16-update P9 window | 1,096,589 | 1,090,538 | −0.55% |
| Later 16-update P9 window | 1,097,055 | 1,091,118 | −0.54% |
| 9,489-update P10 victory | 1,464,990 | 1,460,178 | −0.33% |

The target index accounts for 5,602 and 5,709 fewer cycles per update
in the two P9 windows. All 32 sampled P9 worlds, framebuffers, palettes
and route counts match. The P10 victory matches all 81 ordered cuts
and exports identical 32 KiB SRAM. These are emulator timings.

The 30 ordinary campaign openings and 24 ordinary Custom Game save
continuations agree with the cursor release. Six 12,000-update matches
with three CPUs agree at all 120 sampled world, video, palette and route
cuts. Total guest cycles fall by 0.13–0.21% in those matches. The full P9
route wins at frame 272,857 after 281,447 ordinary calls. All 93
checkpoints match the cursor build, including 80 after a fresh pause
reload. The pause, victory and result-readback SRAM each match byte for
byte. Total cost falls from 383,550,668,426 to 382,136,090,332 guest
cycles (0.37%) and the mean reaches 12.36 emulated updates per second.

| Local receipt | SHA-256 |
| --- | --- |
| P9 instruction windows | `ef320144e03b4a72d328aac003a910795969f3dda92ccb518020e22595a37aac` |
| P10 victory tail | `c677652aa26566fe892e6435c392821437c807ce1f98f9f9b619a14a091c1b58` |
| Six three-CPU matches | `fdcb7c5d949f968ff6da1438a61576307f1634d916bec88a28e14b87ed4929ba` |
| Full P9 victory and SRAM readback | `90fac9b7bf721731c8343ad435d9b2173eb7a6a56608db5bc6f6bbb4c1bd9c25` |

## Strict reuse on the generated-bounds release

The November 22 trial applies the draft to `aa1d49c8`. It verifies
each retained graph member while the owner lists already scan live
slots. The membership check covers owner, eligible slot, exact cell
and world dimensions. Every retained cell chain must remain in
canonical order, including Protoss. Exhausted membership, a slot
change, an out-of-order link or a geometry change triggers the
current fresh index path. The parked index stays inactive. The
15 host sanitizer configurations pass including slot replacement,
an extra eligible member, reordered ties, geometry and overflow.

The Protoss 10 saved-game pair reaches victory in all variants and
matches all 81 sampled world, route, fog, HUD, palette and two-page
framebuffer cuts plus final SRAM. Across 9,489 updates, the current
image averages 1,523,161 cycles. The strict separate traversal
averages 1,519,068. Fusing that validation into the owner-list loop
averages 1,518,245. Checking chain order on every candidate removes
the ordering flag but gives 1,521,432 cycles. The final four-argument
validator with ROM placement for fast-memory orders builds averages
1,530,854 on P10. Exact instruction attribution shows the final
candidate still rebuilds the index in 12 of 16 earlier samples and
9 of 16 later samples. The fused check adds setup work to updates
that rebuild anyway. These variants are experimental.

Z10 fails its original IWRAM link guard with the fused variants,
including removal of the ordering flag, an out-of-line remove,
and a ROM-resident validator. For a separate matched comparison,
the current algorithm and retention both run the orders routine
from ROM. Both link with the original guards. The current algorithm
also runs in its ordinary fast-memory configuration as a reference.
All three replay the same ordinary Blood Bath save with 72 starting
units, 49 buildings and three CPU races. They match route counters
on all 3,600 updates, all 31 gameplay and frame cuts, the saved SRAM
and 16 fresh-load continuation cuts. The fingerprints include live
actors, production, projectile state and both framebuffer pages.

| Z10 orders placement and algorithm | Mean cycles | p95 | Peak |
| --- | ---: | ---: | ---: |
| Current, IWRAM | 812,646 | 1,102,874 | 3,677,137 |
| Current, ROM | 841,977 | 1,134,121 | 3,714,437 |
| Strict retention, ROM | 852,229 | 1,146,291 | 3,727,932 |

Retention raises the mean by 1.22% and p95 by 1.07% against the matched
ROM control. Leave the release index unchanged. Use this dense save
alongside the P10 battle tape for future movement, contact and target
costs. Retention would need to avoid most revalidation or repair only
changed cells while preserving same-update target order.

| Local evidence | SHA-256 |
| --- | --- |
| Unfused strict P10 replay | `56e980e432b86df9533351c99c9f9a19de48bc2c085b586a80ac6a4b590b3bc2` |
| Fused P10 replay with ordering flag | `84f2b8631f7a2093074eac2b1c6f1d171f405f7805ac89940adda87b9251c875` |
| Final fused P10 replay | `7d65cc45d4e75faa32620aa94c741d451151e4502a3d9194e2128422ac8a0d9b` |
| Current Z10 dense custom replay | `e0aa7344e104b7d91e275246d5ca8b42624b4e7fb9cbd353ccc7d66e69dcd4a2` |
| Z10 ROM control replay | `0a687bc6a89f2da5f2008a9b644d7067df84fc4b02409aef2dcdbef1045daeef` |
| Z10 ROM retention replay | `886041da05e2dbe17e8e1085f74df7be04ceedd562cf0158fc813a377d29c64e` |

### The next dense skirmish bottleneck

Independent profiled and unprofiled runs of the ordinary Z10 image
match at 32 cuts across two 16-update windows starting at replay
updates 1,750 and 2,050. Instruction totals account for all measured
guest cycles. Orders use 241,012 cycles on average in the first
window and 558,598 around the spike. Rebuilding the target index uses
28,919 and 30,736 respectively. Focus on the route work around the
spike, where movement and worker return paths recur in the same update.

The exact peak at game frame 19,360 takes 3,677,630 guest cycles
between input samples. Worker facility routing accounts for 1,089,014
cycles and movement point-route fills account for another 870,183.
The preceding frame removes building slot 44 (raw 144) and clears
all 32 active mining and gas workers' stored depot slots. At the
peak 31 workers reselect their former depot. The last worker reselects
its former depot one update later. Global invalidation is followed
by ten route builds at the peak and five more in the next update.
Measure a proof of when opening a changed tile preserves the chosen
reachable depot, including closer candidates, changed resource sites
and any worker crossing. Keep exact reselection for inconclusive cases.

| Local evidence | SHA-256 |
| --- | --- |
| Profiled and unprofiled 32 cuts | `b4fe6c77b8a1831c124a264f805488aa8f63f5280d0c55a8340a50681c5cbbac` |
| Two-window attribution | `1025bf448c8997ae1a92883685b5c832d0f06737b6c2a91b4ded28e35f7afc70` |
| Exact peak attribution | `e1f948bfdeaa8a87754ee86475b718372a6199b7a317061b59c06fb8693a8874` |
| Worker and building state around the peak | `199111201e384f6f5350604f151168e634b3fae8c23793f6fe1aad73515f9fb5` |
