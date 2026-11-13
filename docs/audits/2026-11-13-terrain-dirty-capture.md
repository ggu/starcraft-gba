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
