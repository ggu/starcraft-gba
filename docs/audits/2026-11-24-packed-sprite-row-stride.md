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

| Local receipt | SHA-256 |
| --- | --- |
| Authored packed sprite timing and pixels | `8d1a6e2fee3345903f204ca4c9bf430c6299eeab73a8ee016a386e077cc6f1eb` |
| Four graphics object profiles | `fc938f69f67eb6769f9388a71e37b664d641daaa76e6d90f87d7296c96abdc67` |
| Exact Protoss 10 sprite and movement census | `67f20dae00085613182e2471d1d594100e038637edf1a29e61b035c2c47e8b6f` |
| Byte sprite baseline | `caa0f2fb81e5cf39e33af1e831026179010130e776817dfc4ea79a15b3a35cbd` |
| Byte sprite row-address trial | `ba2522af2f4d63dc112aacb2ddf12aa3c6f81eb280251a4191944ced0ff0b111` |
| Identical contact traversal receipts | `8e67a3015a7857de17bf6706f6bc8e50be7a0bcb487db34ae3f5ca3e2390dc46` |
