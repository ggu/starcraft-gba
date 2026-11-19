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

### Validated cross-update prototype

Source `620a887e` adds the opt-in `SC64_TARGET_RETAIN_PROBE` beside
the canonical bulk and fast-link flags. The index stays inactive at
the update boundary. The following orders pass checks the current
eligible slots, owner and 64-pixel cell of each actor, total
membership and world geometry before reactivating it. Relevant
mutations invalidate retention. Dirty list compaction restores
a fresh build's overflow capacity and occupancy masks. Profiles
with order-sensitive targeting also validate descending chains.
Defense still performs its existing separate pass.

The focused ASan/UBSan test passes all 15 mode and geometry vectors
in `test_sc64_target_owner_index.py`. Targeted cases cover actor
replacement with the same count, owner and map changes, unindexed
movers, dirty-list duplicates, overflow recovery and profile-specific
chain order. The normal-profile P10 replay binary is byte-identical
before and after moving the phase macros out of the main source file.

| Normal compiler recipe with probe | ROM hash prefix | IWRAM end | EWRAM end |
| --- | --- | --- | --- |
| Terran | `cdbe3a99` | `0x03004918` | `0x02038e94` |
| Z10 | `e83630d0` | `0x03004df8` | `0x02038028` |
| P10 | `f6391256` | `0x03004e70` | `0x0203b5d4` |
| Showcase | `f9ae2cee` | `0x03004fe8` | `0x02039110` |

All four pass the ROM header, SRAM signature and RAM guards using the
same prepared inputs as release. Showcase has 24 bytes below the
existing IWRAM link guard. That is a link measurement. The P10
recipe with the flag absent reproduces the normal release ROM
byte-for-byte.

The P10 Homeland comparison covers 958 advancing updates across
the last 1,000 input calls. Late Blood Bath covers 1,000 advancing
updates from frame 16,302 with three independently selected AI
seats. Both match all recorded timeline and route rows and the
sampled worlds, palettes and complete framebuffer pages. At the
11 Homeland and 32 Blood Bath cuts, the hardware page also matches.

| ARM cycles per advancing update | Release mean | Preview mean | Mean change | p95 change | Peak change |
| --- | ---: | ---: | ---: | ---: | ---: |
| P10 Homeland | 1,728,244 | 1,696,767 | −1.82% | −1.36% | −1.78% |
| P10 Blood Bath late | 998,765 | 983,379 | −1.54% | −1.41% | −0.88% |
| Z10 dense Z7 | 2,340,889 | 2,340,034 | −0.04% | +0.48% | +0.27% |

Mean orders cost falls 3.91% in Homeland and 3.42% in Blood Bath.
The dense Z7 window covers 477 advancing updates. It matches all
131 historical gameplay checkpoints, all 480 timeline and route
rows, and eight sampled world, fog, palette and framebuffer pairs.
Its p95 increase leaves the strict-chain cost open. All three
whole-update means exceed the 30 fps cycle budget.

| Local preview receipt | SHA-256 |
| --- | --- |
| P10 Homeland | `8df1dda22a845655a5b511f8b2a05e62cfa7e43bfc7f92bd320b4f2314298b5c` |
| P10 three-AI Blood Bath | `8fde02fad6725095fdefe61a1f205317f5b8cba60de918b648ccd8c54b1f9ff7` |
| Dense Z7 | `60a70ce28204be31052a02f9c9f7ff4a2bd22ada2090321d5363bf97cb976aba` |
| Terran T4 paired replay | `cb806758b109711969ddef9fc1833c2746db2cb754bc992e3c25ca1638eac9ce` |
| Showcase Carrier scene pair | `3b58e0aae72c6f40e9af65f9d89588c040b6ad0d52f4cc5d2e723f56f4d4a9e9` |

The Terran T4 tape wins both runs at frame 8,911 and matches 10 periodic,
terminal and post-save checkpoints, 45 video cuts and the final SRAM.
The busiest 600-call control window drops from 981,731 to 968,602 mean
elapsed cycles. An internal fog-tile cache hash differs. Visible fog,
palette and both framebuffer pages match throughout the sampled cuts.
The 1,071-call Showcase Carrier scene matches the frame sequence,
two checkpoints and six video cuts. Its 1,000-call active window is
nearly flat at 1,080,128 versus 1,079,281 mean elapsed cycles, with
a 0.33% higher peak.

Each opt-in cartridge also saves through the ordinary pause menu
at skirmish frame 601, loads from HOME in a fresh process, and matches
its own uninterrupted next 180 controller updates. The 32 KiB SRAM is
unchanged after every load.

| Product | Map and human race | AI seats | Fresh save and load receipt SHA-256 |
| --- | --- | --- | --- |
| Terran | Blood Bath, Terran | Zerg, Protoss, Terran | `9e403d4f127e08087a6c539b8cc362b3889b95c57eda6ce827d6068c93ae4425` |
| Z10 | Outpost, Zerg | Terran | `35cf96f06fd89370e4cad18dd6cb234e36dc8117e8c458220d0ba83486d576dd` |
| P10 | Blood Bath, Protoss | Zerg, Terran, Protoss | `79af69f060c3a723b9f894639fd1a67b84750af3db3387a900e38186c9625506` |
| Showcase | Outpost, Zerg | Terran, Protoss | `ec9dcd2fa947fddd3e4762248317c8f2bb337a79713a3fdcff52c8b30198865a` |

Repeat the strict-chain timing before enabling the probe. The draft
retains the unmodified default selector.
