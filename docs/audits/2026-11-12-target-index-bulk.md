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

## How long the current index stays valid

Read-only breakpoints on the normal P10 and Z10 binaries from
`ea041527` sample the completed orders index, the end of orders,
and the defense entry. The observer walks the actual owner-cell
chains and compares their slot membership, compact owner and
64-pixel cell against live actors at each boundary. The Z7 replay
also passes all 131 historical gameplay checks. At the measured cuts,
all three runs reproduce the retained frame and order timings.

| Workload | Sampled updates | Orders index build cycles | Whole defense pass cycles | Orders index covers live actors at defense entry | Exact membership at next sampled build |
| --- | ---: | ---: | ---: | ---: | ---: |
| P10 Homeland | 48 | 59,663 | 63,495 | 48 / 48 | 24 / 24 |
| P10 Blood Bath, three AI seats | 48 | 35,302 | 50,542 | 46 / 48 | 23 / 24 |
| Z10 dense Z7 | 53 | 58,999 | 79,860 | 53 / 53 | 26 / 26 |

Counts require every eligible live slot and no extra index links. The last
column samples adjacent ordinary calls only. Mean defense cycles
include weapon checks and effects. They are an upper bound on what
eliminating a separate defense index could save.

Blood Bath frame 17,141 has a newly spawned unit at defense entry
that is absent from the orders index. Frame 16,860 has an extra link
after a death, which the next fresh rebuild removes. A retained index
still needs a fresh coverage proof. Transport unloads, enemy rescue or
ownership changes, and mission-created units provide additional paths
to invalidate it. Defense has its own geometry, live eligibility and
lowest-slot tie rule. Its normal skirmish dispatch uses the scalar pass.

On the paired Blood Bath cuts only 2 of 24 saved indexes satisfy
both exact membership and the original descending chain order.
Dense Z7 has 9 of 26 and Homeland has 16 of 24. In the Homeland
repeat, eight end-of-orders cuts have at least one dirty entry left
over from a now-empty cell or a repeated link. Moves update
64-pixel buckets during orders. Low-slot ties in nearest-target and
defense queries are explicit. The Egg fallback can instead depend
on the first equal-distance candidate. A reuse path must keep that
iteration order wherever the Egg query is compiled.

| Local read-only receipt | SHA-256 |
| --- | --- |
| P7 Homeland | `38178bd9da0f91753a26a5dd8fa3de8d621bccf3e1f8136df6e37e39d9ac6c7e` |
| P7 repeat with chain order and dirty counts | `e4cc1013123ec37da1fb303aca8e3387184427c34fd95d33dd4e31f737936505` |
| Three-AI Blood Bath | `79a22207c1cf7bbeb0ba4dfcbdd7f5eaf7c0748365c449f9b19fe824eae24e4e` |
| Dense Z7 | `c443b19f0c3b533075cfadbb4999ac9b3d1b64479c81029f6a96238d7511ac94` |
