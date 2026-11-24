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

An independent host check under UBSan compared 78,773,741 signed
and guarded unsigned decisions, including all signed 16-bit
coordinate differences over limits −300 through 300 and additional
integer boundaries. Every tested decision matched.

The `-O2` synthetic Thumb fixture uses the shared contact header,
occupied buckets and 1,536 ground queries. The ARM observer reads
the returned masks and counts guest cycles.

| Trial | Guest cycles | Fixture text bytes | Data and BSS bytes |
| --- | ---: | ---: | ---: |
| Original comparisons | 12,449,234 | 890,172 | 8 and 15,372 |
| Guarded unsigned comparisons | 12,224,610 | 890,116 | 8 and 15,372 |

The two runs produce byte-identical masks. The guarded version
saves 1.80% in this fixture. Production race-ROM timing and RAM
budgets still require qualification.

A separate 1,536-query fixture mixes air and ground actors, six
collision radii, an undescribable class, and steps including zero
and negative inputs. The test runs with and without the geometry
table. All returned masks match in each comparison.

| Mixed fixture | Original cycles | Guarded cycles | Reduction |
| --- | ---: | ---: | ---: |
| Geometry table | 24,759,086 | 23,188,603 | 6.34% |
| Arithmetic | 23,957,727 | 23,557,456 | 1.67% |

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
