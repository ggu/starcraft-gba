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
to reproduce the trial. The normal build leaves the flag off. Other wide
campaign samples and hardware qualification remain open.

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
Profile grid setup, traversal, and packing on each cold request before
changing their admission rules. The separate [resumable route trial](2026-11-16-resumable-frontier-budget.md)
cuts this update from 5,363,816 to 5,167,782 cycles in its paired full
P10 run. That replay predates the wide depot change, so the gains are
measured independently.

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
