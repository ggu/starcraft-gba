# Z7 canonical render-cache trial

At canonical source `e52bd8af`, the verified 227-argument normal P10
compiler vector already includes all five members of `CANONICAL_CACHE_FLAGS`:
`SC64_RENDER_KEY_ONCE`, `SC64_TERRAIN_CACHE_VRAM`, `SC64_HUD_VRAM_BANDS`,
`SC64_TERRAIN_ROW_MASKS`, and `SC64_MINIMAP_RASTER`. There is no new P10 flag
to promote. The expanded Z7 standalone release flags, derived from
`ZERG_Z7_CANONICAL_FLAGS` and `-DGBA_NO_DEBUG_INPUTS`, contain none of the
five. They include `SC64_IMPORTED_MISSION`, `SC64_NO_FRAME_CLEAR`,
`SC64_HUD_BUFFER_REUSE`, `SC64_TERRAIN_SCROLL`, `GBA_WINDOW_BANDS`, and
`SC64_ZP_CAMPAIGN_STORAGE`; `SC64_TILE_RENDERER` and
`SC64_P7_PROJECTILES_ENABLED` are absent. Consequently the packed-grid row
masks, persistent minimap raster, and reusable HUD page bands can activate.
The actual product includes Z7 campaign and its imported custom-game path.

A prospective private candidate adds only the existing canonical cache group
to the exact Z7 release vector. It is not enabled in any product by this
commit. In mode 4 the framebuffers occupy `0x06000000..0x060095ff` and
`0x0600a000..0x060135ff`; Z7 leaves OBJ display disabled. With no P7
projectile arena, `sc64_obj_vram.h` assigns the 16 KiB scratch range
`0x06014000..0x06017fff` to terrain cache, and
`sc64_terrain_cache.h` uses 68 rows (16,320 bytes) there and 40 in EWRAM.
The 0x06009600 audio arena and page-padding death-effects region are separate;
the tile-renderer combination is compile-time rejected. HUD bands are owned
by the front/back framebuffer pages; per-page version/clear tracking copies
only valid bands and redraws otherwise. The minimap raster tracks visibility
and exploration changes. Memory, page lifetime and gameplay equivalence for
Z7 remain to be checked against the actual candidate, not inferred from other
profiles.

The earlier Z1–Z6 frame oracle and paired ARM cache trial are documented in
`docs/audits/2026-10-28-render-caches/README.md`; they do not qualify Z7.
Before any promotion require source/pin verification and both uncached and
candidate Z7 actual-engine poisoned dual-page frame oracles covering scrolling,
fog, HUD, scene transitions and save/restart. Then require actual Z7 ARM
baseline/candidate on ordinary input in early and busy/custom scenes, matching
state, pixels, modal counts and including mean and tail cycles. Check both
normal ROM link guards, page and scratch ownership, and source-boundary checks.
Do not interpret P7 measurements as Z7 timing or claim a gain before the pair.
