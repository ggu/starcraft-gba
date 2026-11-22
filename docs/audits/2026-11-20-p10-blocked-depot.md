# P10 blocked-worker depot route

In the released P10 ROM `9dfba082`, restart from the ordinary pause
save at frame 36,761. At tape call 43,604, the A press advances frame
42,523 to 42,524 and takes 7,542,183 guest cycles. Unit orders take
6,115,750 cycles. A read-only breakpoint trace attributes 4,898,406
cycles to one nearest-depot query for the SCV in slot 173.

The worker is at (161,286), over blocked map cell 2,186. It has no
cached depot and is empty. The query checks depot 67. Its existing
field is partial. The blocked origin requires the full depot field
to compare the distance from each walkable exit. The scalar facility
builder takes 4,885,200 cycles for that query.

## Exact full-field trial

When the worker depot call requires a full 128×128 field, seed the
same walkable tiles in row-major order and use the existing wide
linear FIFO. Preserve the cache lookup, point-tail release, neighbor
order, overflow fallback, and publication. The 64×64 frontiers and
bounded or perimeter queries keep their existing builders.

| Cycles at P10 call 43,604 | Released P10 | Wide depot trial |
| --- | ---: | ---: |
| Full update | 7,542,183 | 3,605,774 |
| Simulation | 6,716,211 | 2,779,674 |
| Unit orders | 6,115,750 | 2,178,370 |

The field stays in slot 11. All 6,144 packed bytes match, as do
the route build and hit counts, ages, goal tags, validity, actors,
resources, and visibility masks. Both fresh-save runs reach P10
victory at frame 46,057, with the same six checkpoint records and an
unchanged input SRAM image. IWRAM and EWRAM link endpoints match.

The asset-free `make test-sc64-facility-fifo` suite passes all six
ASan/UBSan variants, including the wide full-field path with existing
frontier and point-tail support. Use a fresh prepared build directory
and `make CC='arm-none-eabi-gcc -DSC64_ROUTE_WIDE_DEPOT_FULL' verify-protoss-p10`
to reproduce this initial trial. Later production composition and
broader qualification are recorded below.

## Complete P10 controller replay

Both ROMs win from HOME in 47,194 ordinary controller calls at
simulation frame 46,057. All 20 gameplay checkpoints, 10 briefing
records, 14 modal records, and final campaign state match. Across 48
samples, actor and resource bytes, visibility, explored fog, camera,
route counters, and palette match. At call 33,000 the displayed page
and its pixel hash differ. The other 47 sampled display and pixel
records match.

| Timing | Released P10 | Wide depot trial |
| --- | ---: | ---: |
| Whole replay mean main-loop cycles | 1,604,030 | 1,603,948 |
| Late combat p95 update cycles | 2,511,239 | 2,510,479 |
| Late combat peak update cycles | 7,556,344 | 5,416,948 |

The depot update itself falls from 7,556,344 to 3,612,590 cycles
in the uninterrupted run. The peak moves to a different update.
The whole replay remains at about 10.46 emulated updates per second.

## Next late combat stall

The new peak occurs at call 45,231. Starting from the same pause
save reproduces the ordinary A edge and advances frame 44,122 to
44,123. This continuation takes 5,421,981 cycles, with 4,173,362
in orders. Six full point floods take 3,429,423 cycles. The largest
takes 923,721 cycles. The two depot lookups take 3,288 cycles total.

The 16 point lookups issue six distinct cold goals. This table gives
the first observed request for each newly built field, in call order,
on the 128-column map.

| Origin cell | Goal cell |
| ---: | ---: |
| 8,244 | 9,400 |
| 8,245 | 9,401 |
| 8,245 | 9,402 |
| 8,504 | 9,528 |
| 8,632 | 9,530 |
| 8,631 | 9,656 |

The existing cache serves the repeated goals within this update.
Three goals have no walkable seed and still consume 672,175 cycles
building and packing empty fields. The other three floods visit
12,836 cells each. All six use the same walkability grid.
The separate [resumable route trial](2026-11-16-resumable-frontier-budget.md)
cuts this update from 5,363,816 to 5,167,782 cycles in its paired full
P10 run. That replay predates the wide depot change, so the gains are
measured independently.

## Empty point fields

`SC64_ROUTE_LINEAR_EMPTY_GOAL_PROBE` checks the blocked goal’s bounded
3×3 seed region before building the padded grid. If the region contains
no walkable seed, it writes the same packed unreachable field directly.
The caller publishes the same cache identity, age, validity and counters.
All searches with a seed keep the existing expansion and tie order.

The first paired test enabled both flags in a fresh prepared checkout.
The combined ARM build retained the same IWRAM and EWRAM endpoints.

| Cycles at P10 call 45,231 after SRAM load | Wide depot trial | With empty point fields |
| --- | ---: | ---: |
| Full update | 5,421,981 | 4,785,608 |
| Simulation | 4,801,466 | 4,164,862 |
| Orders | 4,173,362 | 3,536,853 |

All 14 resident field hashes, actor and resource bytes, walkability,
visibility, and route cache metadata and counters match. The host route
oracle covers 386,646 queries, including packed output and residency.
The targeted fixture also checks empty goals and diagonal seeds at all
corners on both linear widths. Run `make test-sc64-route-linear-empty-goal`.
The exact combined cartridge also loads the same P10 pause SRAM in a
fresh emulator and wins at frame 46,057, matching six checkpoint records.
The source save stays byte-identical.

## Current release composition

Compose both shortcuts with release `4a3d18ad`, including the shared
world depth and P10 detector changes. Both P10 cartridges win from
HOME in 47,194 ordinary inputs. All 20 checkpoints, briefing, modal
sequence and final state match. Across 48 samples, both full indexed
framebuffer pages, palette, fog, actors, mineral state, camera, route
counters and the visible page match.

| P10 timing | Release | Both shortcuts |
| --- | ---: | ---: |
| Mean main-loop cycles | 1,599,105 | 1,598,999 |
| Late combat p95 | 2,506,855 | 2,506,866 |
| Late combat peak | 7,545,068 | 4,770,776 |
| Blocked worker at call 43,604 | 7,545,068 | 3,605,028 |
| Empty point goals at call 45,231 | 5,414,354 | 4,770,776 |
| Call 45,452 | 4,019,891 | 3,321,935 |

The late sample covers controller calls 41,913–47,193. The mean remains
about 10.49 emulated updates per second across the full replay.

The 128×128 T4 and T10 controller replays both win in each ARM build.
All 15 gameplay checkpoints, 63 framebuffer samples, per-call frame
and screen sequences, and both final 32 KiB save images match. The
retained T4 fog tile cache differs at one or more samples. The visible
and explored masks and both framebuffer pages match. The busiest 600
play calls average 981,599 and 981,592 cycles on T4, and 1,092,986 in
both T10 builds. These Terran runs verify the combined cartridge across
two other wide missions. The individual shortcut hits were attributed in P10.

The canonical Z10 cartridge also matches every non-timing field at all
131 named checkpoints through 48,237 ordinary Z7 calls, including its
dense late section. This is a campaign prefix. Mean cycles move from
1,251,013 to 1,251,048, and the recorded peak from 17,793,413 to
17,811,948. This replay covers named state and economy fields.

`CANONICAL_ROUTE_WIDE_SHORTCUTS=1` is the common default when the
canonical linear route path is enabled. Use `CANONICAL_ROUTE_WIDE_SHORTCUTS=0`
for a comparison build. The lookup build key carries the setting.
The four normal launch recipes reproduce the qualified candidate ROMs
byte for byte. All four pass ROM header, save signature and RAM checks
with unchanged RAM endpoints. Both host route suites pass, including
the six facility FIFO variants. The normal P10 image loads the retained
pause save from HOME and wins with the same six checkpoint records and
unchanged input SRAM. Physical GBA timing remains open.

| Local receipt | SHA-256 |
| --- | --- |
| Release depot call trace | `69c7d9ccea076be6fb59d37d9f96e2d20f789d23debe44dc652389078edaa705` |
| Paired release update | `6e003d476984c18ea14bd94648f59e660d73ee41eae618861dd792de93a1fcb2` |
| Paired trial update | `1f85ce09cade81664ce134bcec57002d876e3fce651b99647841b19a88213f33` |
| Trial fresh-SRAM victory | `968b49fb59cfca8d1a388371b62e9f3df19a915723f36b33b041fa8d7a3510b5` |
| Release full HOME victory | `dd40e45c0578c03a9a0a59b861c86d61643d44c727ea048a9a85cd9e417c5aae` |
| Trial full HOME victory | `207d3f5ffdd208f62912fcf217598382b45d0c24ae10e34b7d9db0ca80263a3d` |
| Next peak phase trace | `13baad40a7bb7104c7e7c5d542c23112db7f33ae8c12bf2d80e9acc55afcbe4a` |
| Next peak point trace | `fc25b66ca4576e0b11a5e840220db019cf53a164b33ecec5a15da9dead0c257d` |
| Point fill phase and pop trace | `b5f328cc33ee24b37807d71ab29f9de7f1d25ff8920071f0da25c30670d9ee74` |
| Empty goal comparison baseline | `6038c572a1b203159d00bfcfea29e897edbdf5d639e5ce38c4e824bb25060618` |
| Empty goal comparison candidate | `94dacb5e9872f20cfc5d5c39f6fa51e8b575521fe48cc4092e13e8c0fbc32686` |
| Combined trial fresh-SRAM victory | `6d37629432a713aebc3303daaad0d4b3457cbdfd4c4978227d7dd8691ff22e7c` |
| Verified combined trial ROM | `33335858af82835db1b31638c8d9a25c50d20257091c56b3aed54377a7cf4049` |
| Current release P10 full HOME | `37e4783a6a5dab9be893ff0f1fd38ec3faca49d1a45a97e1efbccaee2c92df5f` |
| Current composed P10 full HOME | `a01b0e81b7fd4ee2a7dd95fb36eecd3c3428682dbe1313c3ceab6dcf6e161f0c` |
| Current T4 ARM comparison | `b1c23790e4548597f0f98139d3a59cb04c11e562cda5e51faeff411998a2f84d` |
| Current T10 ARM comparison | `e7bd87a6db72c8545ac959d29e4efdbf474df401b3ead0529fbf41d361dacd13` |
| Current release Z7 prefix | `757c6aaf0538cc55e4e307b18770533dab48d07cfaf4aa45b8c00cc63dc415cd` |
| Current composed Z7 prefix | `8fed0713920ca4ba72f398ff28d44633f0f89cbc46554953b51a4d75e81718ed` |
| Normal P10 fresh-SRAM victory | `90ccb45b794f243184febb7ceaf058911b2fced7a21c37ac82a0fdff843788b5` |
