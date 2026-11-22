# Framebuffer traffic before terrain restore

The [busy Blood Bath profile](2026-11-21-blood-bath-current-profile.md)
attributes about 41,000 guest cycles to the terrain restore calls.
On released Z10 `ef37fe4a`, the read-only observer resumes the same
three-CPU match from ordinary HOME and inspects both bands at their DMA
entry. The 56 observed updates retain all 31 world checks and match
the prior guest loop and route counters. The camera stays at (0, 66)
and both framebuffer pages are observed.

Every sampled update restores an upper 47-row band from object VRAM
and a lower 61-row band from EWRAM. All 108 rows differ in every
sample at the transfer boundaries. The two DMA calls average a
combined 41,621 guest cycles. The raw comparison includes the one-pixel
viewport frame drawn after terrain capture.

## Upper bound after excluding the viewport frame

Repeat the comparison with columns 0 and 239 and rows 12 and 119
excluded. These pixels are overwritten by the viewport frame. The
remaining rows still average 80.21 with different pixels. A single
full-width contiguous span per backing store averages 84.95 rows
total, from 72 to 106. The modeled mean saving is 11,145 cycles.

| Transfer shape with perfect knowledge of changed pixels | Mean bytes copied | Modeled mean cycles saved |
| --- | ---: | ---: |
| Existing full bands | 25,920 | 0 |
| One full-width span per backing store | 20,388 | 11,145 |
| Word-aligned horizontal spans, grouped in eight-row slices | 8,629 | 29,967 |

The single-span bound keeps the two existing transfers. The
horizontal-span bound assumes transfers can touch only the necessary
words. It excludes setup, row dispatch and tracking overhead. These are
transfer estimates. The sampled updates average
1.41 million guest cycles, so even the optimistic horizontal figure
accounts for about 2% of those cycles.

Keep whole-band DMA as the baseline for this workload. A future
region trial would need cheap per-page damage bounds and still has
to cover camera and fog changes, handoff from title and menus, and
effects that read pixels under their own draw. Prioritize the full
point floods, path-grid rebuild and the larger steady actor and
effect draw costs first.

| Local evidence | SHA-256 |
| --- | --- |
| Full-band observer | `1774ef31be55f0de07e0faaf86aca2f9039d3aecd004cb5928346dd6a95fdba9` |
| Observer with interior comparison | `5c7eac87173e104fcc9bb6a9c527d2f4f3a9c82a8d5046cf047989dd9c52fa45` |
| Interior transfer comparison | `281b89e1a61b0d3653508ee65c6fbbbbf1c871efcbb3b1d2e2fec5f453b9b45c` |
