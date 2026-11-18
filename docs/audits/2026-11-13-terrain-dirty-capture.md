# Capture only dirty terrain bands

The retained normal P10 steady trace attributes an average 38,504 guest
cycles per sample to `gfx_read_band` (17 of 18 samples), and the pinned Z7
window averages 29,973 (10 of 12). Both use HUD page versions, and the
render-path read calls are the terrain cache capture. These costs bound
potential savings; cold captures and moving cameras cannot use this probe.
P10's steady presentation wait and queue call averaged only 25 and 93
cycles respectively, so changing presentation is a weaker first target.

`CANONICAL_TERRAIN_DIRTY_CAPTURE=1` reuses the existing static dirty-row
plan on split-cache builds. After a same-camera cache restore, all map
writes before capture are inside those bands: dirty tiles and clipped
static objects. The cache already contains the pixels outside the bands.
The probe copies each range back into the cache, splitting at the VRAM /
EWRAM boundary, before dynamic overlays run. It leaves the existing cache
metadata publication in place. The same backbuffer and clear generation,
mission, camera and static key must match. Missing, malformed, whole-height
and stale plans use the existing full capture. A failed partial copy also
falls back to full capture. No persistent buffer or hot function is added.

`make test-terrain-dirty-capture test-terrain-soft-fog` passes under ASan
and UBSan. The focused test checks a 15-row capture in three disjoint bands
against all 25,920 bytes of a full capture, including a band crossing the
split and adjacent VRAM canaries. A stale clear generation, changed page,
changed key and overlapping plan each require a full capture and preserve
pixels changed outside the proposed spans. The default stays off. The next
qualification should measure eligible capture frequency, DMA rows and full
loop cycles on the existing P10 and Z7 physical tapes, with exact video.

## Paired regular P10 replay: leave the probe off

On current `6cfaa326`, the P10 runtime is unchanged from the previously qualified control. The candidate adds only `SC64_TERRAIN_DIRTY_CAPTURE` to the same generated inputs and compiler vector. Both links end at `0x03004c80` IWRAM and `0x0203c0f8` EWRAM. They load the authenticated ordinary frame-36,761 save (213 units, 81 buildings) via title L, replay the original physical key tail over 2,070 advancing P10 calls, and save again through START/B. Both match the known loaded and endpoint worlds, all 82 paired world and video cuts, including both indexed pages, palette and fog, every sampled frame, camera and route delta, and all 32 KiB final SRAM bytes. The host dirty capture and soft fog oracles also pass.

| Guest cycles | Control | Partial capture |
| --- | ---: | ---: |
| Whole loop mean | 1,918,498 | 1,920,448 |
| Whole loop p95 | 2,475,108 | 2,470,877 |
| Background mean | 304,368 | 304,541 |
| Background mean, 193 calls with unchanged camera | 70,016 | 70,032 |
| Background at the most expensive unchanged-camera call (39,065) | 210,265 | 174,037 |

The camera stays in place for only 193 of the sampled calls. The capture preflight also checks the page, mission and static content, so 193 is an upper bound on when this path could run, not a count of accepted partial captures. The candidate cuts one high background cost by 36,228 cycles, but total mean loop cycles rise 0.10%. Two updates fit the 561,792-cycle budget in each arm. This window does not justify enabling the feature. A stationary-camera work sample with frequent fog changes would be a different test; no hardware performance claim follows.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Control ROM | `a4c5c93c307bc275aca13a61e4e2526b1fabccfdfa4465382fa72afd28be0dff` |
| Partial capture ROM | `a6b328448e7caa84c4634f4a36fdd0e70c053282abc31d134d8f3444c390b4d2` |
| Paired receipt | `47ea725eb24cf9a55920729fe58979974af6881f4c1ba7346b892c235271b870` |
| Matched state, pages, palette and fog | `081a999338327b12db174b624670ee264fc62a0adb7ba1ccf466acd22d6bd941` |
| Matched final SRAM | `e7b752829e198e8d8ae04d18c43c221ffe230e2e5b0e58b1048253b6256f3cea` |
