# Unsigned axis bounds in the contact scan

The shared contact scan rejects an occupant outside either open
interval around the mover. For a positive limit `L`, the accepted
integer offsets are `1 − L` through `L − 1`. Adding `L − 1` in
unsigned arithmetic moves that range to `0` through `2L − 2`.
One unsigned comparison per axis then rejects offsets below or
above the range. The early guard preserves rejection on nonpositive
limits, including a zero radius and zero speed. The shared
occupant filter applies the check before the geometry table and
arithmetic fallback.

The unsigned comparisons are enabled in the speed-optimized
ROM-resident kernel. The IWRAM and size-optimized kernels retain
their original comparisons.

An independent host check under UBSan compared 78,773,741 signed
and guarded unsigned decisions, including all signed 16-bit
coordinate differences over limits −300 through 300 and additional
integer boundaries. Every tested decision matched.

The initial `-O2` synthetic Thumb fixture uses the shared contact header,
occupied buckets and 1,536 ground queries. The ARM observer reads
the returned masks and counts guest cycles.

| Trial | Guest cycles | Fixture text bytes | Data and BSS bytes |
| --- | ---: | ---: | ---: |
| Original comparisons | 12,449,234 | 890,172 | 8 and 15,372 |
| Guarded unsigned comparisons | 12,224,610 | 890,116 | 8 and 15,372 |

The two runs produce byte-identical masks. The guarded version
saves 1.80% in this fixture.

A separate 1,536-query fixture mixes air and ground actors, six
collision radii, an undescribable class, and steps including zero
and negative inputs. The test runs with and without the geometry
table. All returned masks match in each comparison.

| Mixed fixture | Original cycles | Guarded cycles | Reduction |
| --- | ---: | ---: | ---: |
| Geometry table | 24,759,086 | 23,188,603 | 6.34% |
| Arithmetic | 23,957,727 | 23,557,456 | 1.67% |

### Verified kernel placement and late battle inputs

The placed fixture replays 229 consecutive contact queries from
updates 11,385–11,392 of the exact November 24 Protoss cartridge's
late three-CPU Blood Bath match. Capturing the calls preserved the
sampled world, page and palette hashes. The fixture reconstructs
the contact buckets from numeric observations, asserts the linked
kernel address and times a separate, non-inlined kernel.

| Kernel | Original cycles | Scoped cycles | Reduction |
| --- | ---: | ---: | ---: |
| ARM in IWRAM, `-O2`, table | 446,606 | 446,606 | 0.00% |
| Thumb in ROM, `-O2`, table | 999,506 | 995,420 | 0.41% |
| Thumb in ROM, `-O2`, arithmetic | 1,444,668 | 1,386,010 | 4.06% |
| Thumb in ROM, `-Os`, table | 927,090 | 927,090 | 0.00% |

An earlier fixture inlined the kernel into Thumb ROM code. The
placement check corrected that error. With the candidate enabled
unconditionally, the correctly placed ARM kernel lost 1.80% and
size-optimized Thumb lost 1.42%. The scoped fixture produces
byte-identical binaries for those two kernels. All 229 returned
masks match the original cartridge across all four fixture pairs.
The retained Thumb table change slows 92 individual calls, with
a worst increase of 538 cycles, even though the total improves.
The fixture covers the contact kernel. Whole-cartridge frame
timing and RAM budgets still require qualification.

The default asset-free `./dev doctor`, `./dev build`, and `./dev test`
all passed on `441c0cef`. The Python suite ran 1,182 tests with 35
skips, and the host sanitizer checks completed.

| Local receipt | SHA-256 |
| --- | --- |
| Original fixture | `8e67a3015a7857de17bf6706f6bc8e50be7a0bcb487db34ae3f5ca3e2390dc46` |
| Guarded fixture | `a5b14ccca0722729d7279f725788d907a14a5037f56d1f6761b7bc387ee992b3` |
| Matched returned masks | `14400de1959c4d7e9a72e0d57b748a02d1697642fea464239584c97798f6fabe` |
| Mixed table fixture, original and guarded | `cbf0c964b3b450446c186f43adccc3657497e90edefc356fd282e5330a85dd14` / `c5b1065b018a9753723a062582447388e19f2af2c73003ea6d7d5529935ed731` |
| Mixed arithmetic fixture, original and guarded | `cb5f733f77d404c3daed13bf70b50c1bf729a3f49493e5745769d0afeca18c57` / `c7db060aad0f09c2d46c1c0412c32694ec2c9844615ea2fa8b513a98983f8c42` |
| Original cartridge query capture | `21f0cc454fd83205ca96bac703260bbd91099219e0229372d1f4c0ef52ec132d` |
| Scoped placement fixture and mask comparisons | `eb711dac99819bb06f15407cbc762d38a380619a87a301405f54a46e13d4b25d` |

### Reuse the query origin and map dimensions

The contact mask takes one local copy of the mover coordinates
and map dimensions after its early returns. Candidate order,
class checks, offset comparisons and the geometry lookup stay
the same. The fallbacks can fill the type cache. They do not
move the current actor or change map dimensions.

An observed-input ARM fixture replays the 229 late P10 Blood
Bath queries above. A separate read-only capture supplies 221
queries from updates 9,001–9,008 of the same match. Its sampled
world, camera, both framebuffer pages and palette match the
uninstrumented controls. All 450 masks agree with the cartridge
in every paired kernel placement.

| Compiled kernel | Early cycles, original → trial | Late cycles, original → trial |
| --- | ---: | ---: |
| ARM in IWRAM, `-O2`, table | 412,872 → 388,260 | 446,606 → 419,551 |
| Thumb in ROM, `-O2`, table | 902,572 → 902,573 | 995,420 → 994,879 |
| Thumb in ROM, `-O2`, arithmetic | 1,269,675 → 1,254,096 | 1,386,010 → 1,368,227 |
| Thumb in ROM, `-Os`, table | 840,467 → 822,059 | 927,090 → 905,818 |

The IWRAM fixture saves 5.96% and 6.06% in the two windows and
uses 24 fewer IWRAM bytes. BSS endpoints match in all four pairs.
The faster Thumb table build is flat overall, with individual
increases up to 11 cycles. Its arithmetic and size-optimized
table variants save roughly 1.2% and 2.2% respectively. These are
mask-kernel timings, using the retained eight-update samples.
Full frame timing, RAM and ordinary save continuation remain to
be qualified across the release cartridges.

Two 280-update authored race-engine runs compile identical ROMs
within each pair. One uses the arithmetic path, and the other
uses the geometry table in Thumb ROM with the usual linker guards.
These are build-coverage checks for those configurations.

| Local receipt | SHA-256 |
| --- | --- |
| Late coordinates and dimensions trial, including each in isolation | `acd80de5b8d1b11ee9130358ba8cd589019f6f690bbc8774743f6a8fcd4a031b` |
| Early read-only query capture | `b9c05ee7635b6303f702d1f49c8f9303f90c8696f67b3074b3cf78b21fa95f66` |
| Held-out placement and exact masks | `921f3f9f103e94ccadb1deaa89702cc59583af6eb09f46669727fa19e3acbf08` |
| Authored frame checks, arithmetic and Thumb table | `535bdf60f285bd085b6aa7fb5b2950e88c7caf4895426809b8344b797309e9cd` / `df2ed0c9f3aef4d3d4f1620341e44fab292455fdaf5ca8a71bccbf9e2dccf5ac` |
