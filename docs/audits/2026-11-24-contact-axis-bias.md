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

### Opt in to the early bound in speed-optimized ROM builds

`CANONICAL_CONTACT_ROM_AXIS=1` requests the existing early
bound in the speed-optimized ROM kernel with packed actor
coordinates. A change to the option
invalidates the canonical ROM ELF configuration stamp. The
default is zero.

The option covers the Zerg Z10 geometry table and the Terran
arithmetic path. Protoss P10 and Showcase use the separately
configured IWRAM path.

The [Outpost query census](2026-11-24-kodak-package-checks.md#contact-queries-in-those-outpost-windows)
supplies 874 Terran and Zerg queries from one- and two-CPU
matches. The actual guarded source matches all cartridge
masks and repeats each timing from the direct-enable trial.
The fixture uses 21.9–27.0% fewer kernel cycles and adds
92 kernel bytes. Its BSS endpoint moves by 0–4 bytes. Verify
complete frame time, working RAM, stack and save continuation
before enabling it in the release builds.

The guarded source fixture receipt has SHA-256
`d16ffcb2b180f27ae038582eecc93384e613dc8a9be04fe156188a57ce4c0a2f`.
A separate `-Os` pair produces byte-identical ROMs and
matching cycles. Its receipt has SHA-256
`ff2918f5e1f136b0f0273e11ebf40f79fcf79bc7fe76377e3d424585d1bbd83e`.

### Whole-frame Terran arithmetic comparison

Two local Terran links from `4e24202f` use the same prepared
inputs, including audio, with the unpacked shared sight
path. They differ only in the early-axis
compile definition. After adding the option to the arithmetic
path, a second link for each setting matches its measured
ROM byte for byte. Both links meet the existing static RAM
guards. The contact kernel grows by 44 ROM bytes. The linked
IWRAM and working RAM endpoints are identical. Both contact
kernel prologues allocate 176 bytes including saved registers.

Each pair starts a match through the ordinary Custom Game
menu. The player gives no orders and keeps the starting
camera. The cases place Terran, then Zerg and Protoss CPUs
in order, with Outpost in Free for All and Blood Bath in
Melee. Each row covers 120 consecutive updates starting
after 11,000. The two-VBlank budget is 561,792 guest cycles.

| Map and CPU count | Mean cycles, base → early | Reduction | Budget overruns, base → early |
| --- | ---: | ---: | ---: |
| Outpost, 1 CPU | 580,859 → 565,614 | 2.62% | 98 → 65 |
| Outpost, 2 CPUs | 824,818 → 798,788 | 3.16% | 120 → 120 |
| Outpost, 3 CPUs | 1,018,688 → 1,006,875 | 1.16% | 120 → 120 |
| Blood Bath, 1 CPU | 561,367 → 561,367 | 0.00% | 30 → 30 |
| Blood Bath, 2 CPUs | 629,575 → 616,898 | 2.01% | 120 → 120 |
| Blood Bath, 3 CPUs | 908,885 → 897,867 | 1.21% | 120 → 120 |

Nine checkpoints per match pair agree on world state, both
framebuffer pages and palette. The six pause saves and
fresh HOME exports agree between builds. Each pair matches
again after 60 resumed updates. The three-CPU Blood Bath
peak rises from 1,215,014 to 1,225,063 cycles, despite the
lower mean and p95. The 1-CPU Outpost mean reaches 29.66
emulated updates per second. Busy matches still exceed
the budget.

The paired receipt has SHA-256
`e76e2b17291ff5de075e7638fd750240d6dd983482a689d673b9c34ece9fb228`.

A campaign pair fresh-loads a late Terran 5 KODAK journal at
frame 97,617 and plays 160 updates with two camera moves.
All nine observed worlds and framebuffer pairs agree. The
ordinary save, fresh HOME, and 60 neutral continuation
updates agree again. Mean cost changes from 734,317 to
734,299 cycles in that window. The campaign receipt has
SHA-256 `a8973a1a9a057011f09611fd0a755ba7e3e65892b591a459e071e32724f7c7a0`.

### Canonical Terran and Z10 whole-frame pairs

The release builds include persistent collision buckets and
native Melee starts. Terran also uses the canonical packed
and sparse sight tables. Late pairs measure 120 updates each
after 11,000 updates of ordinary skirmish play. The sampled
worlds, both framebuffer pages, save journals and fresh HOME
continuations match on all ten pairs.

| Product and map | CPUs | Mean cycles, off → on | Reduction |
| --- | ---: | ---: | ---: |
| Terran Outpost | 1 | 573,831 → 561,151 | 2.21% |
| Terran Outpost | 2 | 818,078 → 792,180 | 3.17% |
| Terran Outpost | 3 | 1,012,069 → 1,000,027 | 1.19% |
| Terran Blood Bath | 1 | 561,368 → 561,368 | 0.00% |
| Terran Blood Bath | 2 | 620,192 → 607,469 | 2.05% |
| Terran Blood Bath | 3 | 898,966 → 887,885 | 1.23% |
| Z10 Outpost | 2 | 843,126 → 802,554 | 4.81% |
| Z10 Outpost | 3 | 1,174,639 → 1,156,382 | 1.55% |
| Z10 Blood Bath | 2 | 691,169 → 670,949 | 2.93% |
| Z10 Blood Bath | 3 | 816,476 → 790,240 | 3.21% |

The canonical Terran one-CPU Outpost average reaches the
561,792-cycle budget with 52 of 120 updates still over it.
All Z10 samples remain over the budget. Z10 adds 88 linked
text bytes and preserves its data and BSS section sizes.
Protoss P10 and Showcase links are byte-identical with the
ROM option on or off because their contact kernels run in
IWRAM. The option remains off by default. These comparisons
measure emulated cycles rather than GBA SP frame delivery.

| Local receipt | SHA-256 |
| --- | --- |
| Canonical Terran skirmish pairs | `f815ac0165f94bf524f2a087a18a906c7a61c79b4a3b2b8eb372b23666ec457c` |
| Canonical Z10 skirmish pairs | `4a99b3085809b11f2f2aecb32ef1a7ce846bba1fd5c41fc8c1c2a28f495276a3` |
| Byte-identical P10 and Showcase links | `faa1ca7b9588c287f57ec72abdbda5f4734d47231509d4f888b205124c5c7fc6` |
