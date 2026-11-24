# Packed sprite row setup on ARM

The shared packed sprite painter now computes the first source and
destination row addresses and the clipped group count once. Subsequent
rows use the packed stride and the 240-byte screen stride. The
transparent, opaque and mixed opacity branches retain the same transfers.

## Authored ARM batch

Each batch draws 32 authored mixed-opacity sprites under the same
physical mGBA observer. All eight cases at both optimization levels
match the previous renderer on both indexed framebuffer pages and the
full palette. The test cycles through the four X phases, alternates
physical pages and tests clipping at display row 159.

| Sprite or clip case | `-O2` guest cycles before → after |
| --- | ---: |
| 11×17 | 86,136 → 79,960 |
| 20×26 | 174,360 → 164,728 |
| 16×16, clipped bottom | 99,575 → 93,855 |
| 74×68 | 1,263,328 → 1,237,568 |
| Bottom right | 34,841 → 32,085 |
| Bottom left | 62,810 → 60,054 |
| Top left | 52,669 → 49,601 |
| Rejected outside viewport | 16,915 → 16,947 |

The seven drawing batches save 2.04–7.91% at `-O2`.
The host byte and packed sprite tests pass with sanitizers, including
the builds with and without scene clipping. The ARM fixture keeps the
same EWRAM and IWRAM endpoints. Compiling the shared graphics object
with the recorded Terran, Z10, P10 and Showcase defines, with the
optional sampled audio omitted, reduces static IWRAM by four bytes
in each object. A pointer-loop variant saved cycles and increased the
fixture's IWRAM endpoint by 72 bytes. The measured implementation uses
integer row addresses and preserves the memory endpoints.

## Workload on the retained Protoss cartridge

A read-only census observes eight early and eight late Blood Bath
updates on the existing `1f0ad66e` Protoss 10 cartridge. It reproduces
the recorded world, camera, both draw pages and palette at every
cut. The late cut has 164 packed sprite calls and 3,251 drawn rows.
The most common frame sizes include 20×26, 11×17 and 16×16. The
fixture also tests the larger 74×68 size seen in both windows.
Full-frame timing with the new path in the race cartridges remains open.

### Ordered geometry replay

A second read-only pass records sprite rectangles, clipping bounds
and order for both eight-update cuts. All sampled worlds, cameras,
framebuffer pages and palettes match the retained cuts. The early
window has 11 packed calls and 552 rows. The late window has 164
calls and 3,251 rows. A freestanding fixture replays the rectangles
with authored color and opacity data stored in ROM. It alternates
real Mode 4 pages and uses wait setting `0x4317`.

| Compiler mode and ordered batch | Previous cycles | Row-stride cycles | Reduction |
| --- | ---: | ---: | ---: |
| `-O2`, early | 245,329 | 238,826 | 2.65% |
| `-O2`, late | 614,069 | 576,861 | 6.06% |
| `-Os`, early | 246,165 | 239,022 | 2.90% |
| `-Os`, late | 618,216 | 576,445 | 6.76% |

Every batch matches on both framebuffer pages and the palette at
both optimization levels. EWRAM and IWRAM endpoints match in each
pair. The fixture times the packed draw batches. Full-frame race
timing and GBA SP measurements remain open.

### Packed group opacity

Read-only instruction stops count the masks actually tested in the
same 16 updates. All world, camera, both framebuffer pages and palette
checks match the uninstrumented run. The counts are of four-pixel
groups, including groups at a sprite or clipping boundary.

| Update window | Transparent | Opaque | Mixed |
| --- | ---: | ---: | ---: |
| 9,001–9,008 | 4,675 | 4,732 | 1,505 |
| 11,385–11,392 | 8,480 | 8,048 | 6,563 |

A two-group unroll costs 88 more bytes of IWRAM and regresses the
small right-edge clip from 32,085 to 33,121 guest cycles. The larger
74×68 batch improves from 1,237,568 to 1,180,992. A shorter predicated
loop saves eight IWRAM bytes and 1–2.3% in the original authored mix.
It replaces the opaque fast path with a framebuffer read for every
nontransparent group, so the result depends on group composition.

Four additional authored mixes approximate the measured early and
late group rates, with both clustered and spread masks. Padding and
clipping shift the resulting counts toward mixed groups. At `-O2`,
the 74×68 batch regresses in all four mixes, by 0.28–1.84%. The
20×26 batch improves by 0.33% in the late spread case and regresses
in the other three. Both framebuffer pages and the palette match for
both alternatives at `-O2` and `-Os`. Keep the row-stride change as
the packed drawing candidate.

## Byte sprites and contact traversal trials

The byte-sprite painter also tried advancing the row addresses once
per row. A synthetic ARM mix of 1,328 draws uses the observed P10
dimensions and authored pixels. With `-O2` and scene clipping,
the combined source and destination change takes 11,795,119
cycles versus 11,819,997 before the change. Both indexed pages
match byte for byte. IWRAM grows by 40 bytes, and the smaller
single-address variants run slower. Keep the existing byte path.

A separate authored contact traversal fixture uses the production
header for 1,536 ground queries across four-, six- and nine-cell
windows. Explicitly advancing the row pointer produces the same
compiled ROM, the same 1,536 returned masks and the same
12,449,234 guest cycles. Continue to measure the contacts visited
inside each occupied bucket.

## Opaque rectangle word-copy trials

On the current graphics path, a per-row alignment test gives fast
word copies for an aligned 64×32 batch. The 11×17 and small clipped
fallbacks cost more. A test that checks whole-rectangle alignment once
reduces the 64×32 batch from 1,827,187 to 653,203 guest cycles at `-O2`.
The mixed 16×16 rectangle batch goes from 274,619 to 229,315. The
24×24 minimap-shaped batch rises from 262,163 to 273,555. At `-Os`, a
large unaligned batch rises from 5,368,236 to 6,968,340. Both trials
match both indexed pages and the palette for all 13 authored batches
at each optimization level. Their fixture RAM endpoints stay fixed.

The exact Protoss 10 Blood Bath profile records one opaque rectangle
call and 3,724 guest cycles per late update across a 16-update cut.
That is 0.28% of the 1,350,466-cycle loop. The read-only profile counts
calls and cycles. It does not log their dimensions or alignment.
Keep the existing opaque path and prioritize depth-ordered sprite
drawing and movement costs on the combined builds.

| Local receipt | SHA-256 |
| --- | --- |
| Authored packed sprite timing and pixels | `8d1a6e2fee3345903f204ca4c9bf430c6299eeab73a8ee016a386e077cc6f1eb` |
| Four graphics object profiles | `fc938f69f67eb6769f9388a71e37b664d641daaa76e6d90f87d7296c96abdc67` |
| Exact Protoss 10 sprite and movement census | `67f20dae00085613182e2471d1d594100e038637edf1a29e61b035c2c47e8b6f` |
| Exact Protoss 10 ordered sprite geometry | `90b0425df4acd3b53c375644ff65773b759ee38d29455288cb0ca66657d39f58` |
| Paired ARM draw batches on observed geometry | `f6881dbf1c00306febc7af7a0f4ae96589da861e4b4335937f7554457148af6b` |
| Exact Protoss 10 packed group mask counts | `58db3b6fab1d25b44bb5d5f19c9059205f6b0799493383684d70613dd0e650b9` |
| Packed two-group unroll | `b46a46708b8aa57d2f91aabc172605cb58f577acd2439fe6000cfb2483632c49` |
| Predicated mixed-loop timing | `e728250881ae7300c54f3f28896cd0ebfd0ad59e5e63c9835caaeac69c3eb337` |
| Predicated early clustered / spread timings | `416d5fd39f848284064bd210deda06ec0b413f4823aa79a900c4e368d3156217` / `ffb1d0ac4e3009046dcc122b2afe1025e6f3023490a30986a99e247a8d59997a` |
| Predicated late clustered / spread timings | `c23c0f42336120d740b469161a79b86b038efa026b6456bec093e00903640be6` / `cdba04d96e35d47261597d630051b9e5b6ee8b038b8954ccfd87bca79d11f0f2` |
| Byte sprite baseline | `caa0f2fb81e5cf39e33af1e831026179010130e776817dfc4ea79a15b3a35cbd` |
| Byte sprite row-address trial | `ba2522af2f4d63dc112aacb2ddf12aa3c6f81eb280251a4191944ced0ff0b111` |
| Identical contact traversal receipts | `8e67a3015a7857de17bf6706f6bc8e50be7a0bcb487db34ae3f5ca3e2390dc46` |
| Opaque per-row alignment trial | `e4888d685fe322e50dff7eb26da323ab1d029adb3974e020be10c1eb1caf44b5` |
| Opaque whole-rectangle alignment trial | `d16f735aeb56b4ebdfba43fb8781784d8febe6159d424d1aff30fc62a0927613` |
| Exact Protoss 10 opaque call and cycle attribution | `45a99a47d032f7004a61d8af6a9b44d0d6dc84de7bb70b967d778d775786b302` |
