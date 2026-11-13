/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by game.c at the terrain rendering call site. */
/* Static objects are part of the saved terrain band. Keep their painter
 * order and existing visibility/viewport culls together for cache redraws. */
static void
draw_map_static_objects(void)
{
    int i, x, y;
    for (i = 0; i < MAX_MINERALS; i++)
	if (m[i].amount > 0) {
	    x = m[i].x - camx;
	    y = m[i].y - camy + 12;
	    if (x > -6 && x < 245 && y > 7 && y < 123 &&
	        currently_visible(m[i].x, m[i].y)) {
#ifdef SC64_HAS_SPRITES
		const Sc64SpriteFrame *sprite = mission_mineral_sprite(m[i].style);
		if (sprite) draw_static_mineral_sprite(sprite, x, y);
		else {
#endif
		gfx_diamond(x, y, 7, 4, C_CYAN);
		gfx_pixel(x, y, C_WHITE);
#ifdef SC64_HAS_SPRITES
		}
#endif
	    }
	}
    for (i = 0; i < geyser_count; i++) {
	x = geysers[i].x - camx;
	y = geysers[i].y - camy + 12;
	if (geyser_in_view(x, y) &&
	    currently_visible(geysers[i].x, geysers[i].y)) {
#ifdef SC64_HAS_SPRITES
            const Sc64SpriteFrame *sprite = mission_geyser_sprite();
            if (sprite) draw_mission_sprite(sprite, x, y);
            else
#endif
	        gfx_diamond(x, y, 6, 4, C_GREEN);
        }
    }
#ifdef SC64_IMPORTED_MISSION
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive)
            draw_building_art(&b[i]);
#endif
}

static void
draw_map(void)
{
    int i, x, y, col, row, col0 = 0, row0 = 0, col1 = MAP_COLS, row1 = MAP_ROWS;
#ifdef SC64_IMPORTED_MISSION
    u32 profile_stage_start;
#endif
#if (defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)) && \
    defined(SC64_IMPORTED_MISSION)
    unsigned int terrain_static_key;
#endif
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
    Sc64TerrainCapturePage capture_page = sc64_terrain_capture_page();
    Sc64StaticDirtyRowsPlan capture_rows = {0};
#endif
    camx = viewport_left();
    camy = viewport_top();
#ifdef SC64_IMPORTED_MISSION
    profile_stage_start = gba_clock();
#endif
#if (defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)) && \
    defined(SC64_IMPORTED_MISSION)
    /* Rendering below only reads world state and writes pixels. The exact
     * roster/visibility key therefore remains valid through cache capture. */
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    { u32 diag_t=gba_clock(); terrain_static_key = sc64_static_objects_key(); diag_key_cycles=gba_clock()-diag_t; }
#else
    terrain_static_key = sc64_static_objects_key();
#endif
#endif
#ifdef SC64_IMPORTED_MISSION
    col0 = limit(camx / 16, 0, MAP_COLS - 1);
    col1 = limit((camx + 240) / 16 + 1, 0, MAP_COLS);
    row0 = limit(camy / 16, 0, MAP_ROWS - 1);
    row1 = limit((camy + 108) / 16 + 1, 0, MAP_ROWS);
#endif
#ifdef SC64_TILE_RENDERER
    if (gfx_tile_active()) {
        unsigned int tile_static_key = SC64_TERRAIN_STATIC_KEY;
        int refresh;
#ifdef SC64_DIAGNOSTIC_TILES
        u32 tile_diag = gba_clock();
        ++sc64_tile_diag_frames;
#endif
        gfx_tile_clear_world();
#ifdef SC64_DIAGNOSTIC_TILES
        SC64_TILE_DIAG(sc64_tile_diag_clear, tile_diag); tile_diag = gba_clock();
#endif
        sc64_tile_world_update(camx, camy);
#ifdef SC64_DIAGNOSTIC_TILES
        SC64_TILE_DIAG(sc64_tile_diag_world, tile_diag); tile_diag = gba_clock();
#endif
        refresh = sc64_tile_art_needs_refresh(camx, camy, tile_static_key);
        if (refresh || sc64_tile_art_overflow) {
            /* The same static-object code as the mode-4 background below. */
            if (refresh) gfx_tile_art_capture(1);
            for (i = 0; i < MAX_MINERALS; i++)
                if (m[i].amount > 0 && currently_visible(m[i].x, m[i].y)) {
                    x = m[i].x - camx;
                    y = m[i].y - camy + 12;
                    if (x > -6 && x < 245 && y > 7 && y < 123) {
#ifdef SC64_HAS_SPRITES
                        const Sc64SpriteFrame *sprite = mission_mineral_sprite(m[i].style);
                        if (sprite) draw_mission_sprite(sprite, x, y);
                        else {
#endif
                        gfx_diamond(x, y, 7, 4, C_CYAN);
                        gfx_pixel(x, y, C_WHITE);
#ifdef SC64_HAS_SPRITES
                        }
#endif
                    }
                }
            for (i = 0; i < geyser_count; i++)
                if (currently_visible(geysers[i].x, geysers[i].y)) {
                    x = geysers[i].x - camx;
                    y = geysers[i].y - camy + 12;
                    if (x > 0 && x < 240 && y > 12 && y < 120)
                        gfx_diamond(x, y, 6, 4, C_GREEN);
                }
            for (i = 0; i < MAX_BUILDINGS; i++)
                if (b[i].alive)
                    draw_building_art(&b[i]);
            if (refresh) {
                gfx_tile_art_capture(0);
#ifdef SC64_DIAGNOSTIC_TILES
                SC64_TILE_DIAG(sc64_tile_diag_art_draw, tile_diag); tile_diag = gba_clock();
#endif
                if (sc64_tile_art_lift(camx, camy, tile_static_key)) sc64_tile_art_clear_pixels();
                else sc64_tile_art_keep_pixels();
#ifdef SC64_DIAGNOSTIC_TILES
                SC64_TILE_DIAG(sc64_tile_diag_lift, tile_diag);
#endif
            }
        }
        goto sc64_tile_background_done;
    }
#endif
    /* Each tile gets one terrain/fog pass. Unknown space stays black from
     * gfx_begin, so it needs no second opaque fill after sprites are drawn. */
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
    if (!sc64_terrain_cached_key(camx, camy, terrain_static_key)) {
#else
    if (!sc64_terrain_cached(camx, camy)) {
#endif
#endif
#ifdef SC64_SOFT_FOG
    const u8 *fog_shade = (const u8 *)(const void *)SC64_OBJ_VRAM_FOG;
    /* A lookup that did not reach its signature pass leaves none behind. */
    sc64_terrain_prepare(camx, camy);
#endif
    for (row = row0; row < row1; row++)
	for (col = col0; col < col1; col++) {
	    int idx = col + row * MAP_COLS;
#ifdef SC64_SOFT_FOG
            int part = 0;
#endif
#ifdef SC64_IMPORTED_MISSION
            if (sc64_terrain_tile_clean(col, row)) continue;
#ifdef SC64_SOFT_FOG
            part = sc64_terrain_tile_partial(col, row);
#endif
            if (!SC64_GRID_GET(visible, idx) && !SC64_GRID_GET(explored, idx)) {
                /* Restored pixels must disappear after load/restart removes fog. */
#ifdef SC64_SOFT_FOG
                sc64_terrain_fog_cell(part, col * 16 - camx, 12 + row * 16 - camy, 0, 0, 0);
#else
                gfx_rect(col * 16 - camx, 12 + row * 16 - camy, 16, 16, C_BLACK);
#endif
                continue;
            }
#else
	    if (!SC64_GRID_GET(visible, idx) && !SC64_GRID_GET(explored, idx)) continue;
#endif
	    x = col * 16 - camx;
	    y = 12 + row * 16 - camy;
	    if (x >= 240 || x + 16 <= 0 || y >= 120 || y + 16 <= 12)
		continue;
#ifndef SC64_SOFT_FOG
	    if (!SC64_GRID_GET(visible, idx)) {
		gfx_rect(x, y, 16, 16, C_FOG);
		continue;
	    }
#endif
#ifdef SC64_HAS_TERRAIN_ART
#ifdef SC64_SOFT_FOG
            sc64_terrain_fog_cell(part, x, y, sc64_terrain_state_view(col, row),
#else
	    gfx_blit16(x, y,
#endif
#ifdef SC64_RACES
                race_mode ?
#ifdef SC64_IMPORTED_SKIRMISH
                    (race_skirmish_map>0?sc64_skirmish_maps[race_skirmish_map-1].tile_pixels[sc64_skirmish_maps[race_skirmish_map-1].tile_lookup[idx]]:sc64_tile_pixels[0])
#else
                    sc64_tile_pixels[0]
#endif
                    :sc64_race_maps[race_selected_mission].tile_pixels[sc64_race_maps[race_selected_mission].tile_lookup[idx]]
#elif defined(SC64_TERRAN_CAMPAIGN)
                sc64_campaign_maps[campaign_mission].pixels[
                  sc64_campaign_maps[campaign_mission].lookup[idx]]
#else
                sc64_tile_pixels[
#ifdef SC64_SKIRMISH
                skirmish_ground_tile
#else
                sc64_tile_lookup[idx]
#endif
                ]
#endif
#ifdef SC64_SOFT_FOG
                , fog_shade
#endif
                );
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
            /* The compact arena has authored barriers; make collision visible. */
            if (
#ifdef SC64_RACES
                race_mode && race_skirmish_map==0 &&
#endif
                terrain_blocked(col, row)) {
#ifdef SC64_SOFT_FOG
                sc64_fog_compact_barrier(part, x, y, sc64_terrain_state_view(col, row), fog_shade);
#else
                gfx_rect(x, y, 16, 16, C_STONE);
                gfx_frame(x + 2, y + 2, 12, 12, C_SILVER);
#endif
            }
#endif
#else
	    if (terrain_blocked(col, row)) {
		x = col * 16 - camx;
		y = 12 + row * 16 - camy;
		gfx_rect(x, y, 16, 16, C_STONE);
		gfx_line(x + 2, y + 11, x + 7, y + 5, C_SILVER);
		gfx_line(x + 7, y + 5, x + 13, y + 10, C_DIRT);
	    } else
		gfx_rect(x, y, 16, 16, C_GRASS);
#endif
	    }
#ifdef SC64_STATIC_DIRTY_ROWS
    {
        u16 saved_clip = gfx_get_clip_y();
        int view_col0, view_row0, view_col1, view_row1;
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
        Sc64StaticDirtyRowsPlan *plan = &capture_rows;
#else
        Sc64StaticDirtyRowsPlan plan_storage, *plan = &plan_storage;
#endif
        int planned = 0;

        /* Only partial cache reuse has a complete current-view dirty mask.
         * The cache helper sets reused_base after a successful restore and
         * fills every dirty cell before returning. */
        if (saved_clip == (u16)(12u | (120u << 8)) &&
            sc64_terrain_cache_valid && sc64_terrain_cache_reused_base &&
            sc64_terrain_cache_view(camx, camy, &view_col0, &view_row0,
                                    &view_col1, &view_row1) &&
            view_col0 == col0 && view_row0 == row0 &&
            view_col1 == col1 && view_row1 == row1 &&
            view_col1 > view_col0 && view_row1 > view_row0 &&
            (view_col1 - view_col0) * (view_row1 - view_row0) <=
                SC64_TERRAIN_CACHE_CELLS)
            planned = sc64_static_dirty_rows_plan(
                sc64_terrain_cache_dirty,
                (unsigned)(view_col1 - view_col0),
                (unsigned)(view_row1 - view_row0), view_row0, camy,
                (int)(saved_clip & 0xffu), (int)(saved_clip >> 8), plan);

        if (planned) {
            for (unsigned band = 0; band < plan->count; ++band) {
                gfx_set_clip_y(plan->bands[band].top, plan->bands[band].bottom);
                draw_map_static_objects();
#ifdef GBA_HOST
                ++sc64_static_dirty_rows_host_bands;
#endif
            }
            gfx_set_clip_y((int)(saved_clip & 0xffu), (int)(saved_clip >> 8));
        } else {
            /* Full misses and non-map caller clips retain the full pass. */
            draw_map_static_objects();
        }
    }
#else
    draw_map_static_objects();
#endif
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
    sc64_terrain_store_key(camx, camy, terrain_static_key
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
                           , &capture_rows, capture_page
#endif
                           );
#else
    sc64_terrain_store(camx, camy
#ifdef SC64_TERRAIN_DIRTY_CAPTURE_ACTIVE
                       , &capture_rows, capture_page
#endif
                       );
#endif
    }
#ifdef SC64_TILE_RENDERER
sc64_tile_background_done:
#endif
    sc64_background_cycles = gba_clock() - profile_stage_start;
    profile_stage_start = gba_clock();
#endif
    for (i = 0; i < MAX_BUILDINGS; i++)
#ifdef SC64_IMPORTED_MISSION
        if (b[i].alive)
            draw_building_overlay(&b[i]);
    sc64_building_cycles = gba_clock() - profile_stage_start;
    profile_stage_start = gba_clock();
#else
        if (b[i].alive) {
            draw_building_far_arc(&b[i]);
            draw_building_art(&b[i]);
            draw_building_overlay(&b[i]);
        }
#endif
#ifndef SC64_DEATH_VFX_DISABLED
    draw_death_vfx(1);
#endif
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].selected)
	    draw_unit_pass(&u[i], 1);
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive)
	    draw_unit(&u[i]);
#ifndef SC64_DEATH_VFX_DISABLED
    draw_death_vfx(0);
#endif
    draw_tracers();
#ifdef SC64_SHARED_NUKE_PROBE
    race_nuke_draw();
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (race_protoss_services_active()) race_p7_projectile_draw();
#endif
#ifdef SC64_P4_STORM_ENABLED
    race_p4_storm_draw();
#endif
#ifdef SC64_P6_STORM_ENABLED
    race_p6_storm_draw();
#endif
#ifdef SC64_Z6_STORM_ENABLED
    race_z6_storm_draw();
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_unit_draw_cycles = gba_clock() - profile_stage_start;
#endif
    x = cx - camx;
    y = cy - camy + 12;
#ifdef SC_DEFAULT_UI
    field_ui_draw_cursor(x, y);
#else
#ifdef SC64_SHARED_NUKE_PROBE
    if (mode == MODE_NUCLEAR_TARGET)
        draw_selection_ellipse(x, y, 4, 2, C_GOLD, SELECTION_RING, 0);
    else {
#endif
    gfx_frame(x - 4, y - 4, 9, 9, (mode == MODE_BUILD
#ifdef SC64_BATTERY_SERVICE_ENABLED
              || mode == MODE_RECHARGE
#endif
              ) ? C_GOLD :
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
              mode == MODE_P4_STORM ? C_CYAN :
#endif
              C_WHITE);
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
    if (mode == MODE_P4_STORM) {
        gfx_line(x, y-24, x+24, y, C_CYAN);
        gfx_line(x+24, y, x, y+24, C_CYAN);
        gfx_line(x, y+24, x-24, y, C_CYAN);
        gfx_line(x-24, y, x, y-24, C_CYAN);
    }
#endif
    gfx_line(x - 7, y, x + 7, y, C_HIGHLIGHT);
    gfx_line(x, y - 7, x, y + 7, C_HIGHLIGHT);
#ifdef SC64_FIELD_CONTROLS
    field_controls_draw_cursor(x, y);
#endif
#ifdef SC64_SHARED_NUKE_PROBE
    }
#endif
#endif
    if (box_state == 2) {
        int left = box_anchor_x < box_end_x ? box_anchor_x : box_end_x;
        int top = box_anchor_y < box_end_y ? box_anchor_y : box_end_y;
        int width = FAST_ABS(box_anchor_x - box_end_x) + 1;
        int height = FAST_ABS(box_anchor_y - box_end_y) + 1;
        gfx_frame(left - camx, top - camy + 12, width, height, C_HIGHLIGHT);
    }
#ifdef SC_COMMAND_RING
    command_ring_draw(x, y);
#endif
#ifdef SC_DEFAULT_UI
    if (menu_open) field_ui_draw_command_menu();
#elif defined(SC64_FIELD_CONTROLS)
    /* The imported command panel is drawn above transient mission messages
     * in draw_hud(), after the world and its overlays. */
#else
    if (menu_open) {
        int first = menu_cursor >= 10 ? menu_cursor - 9 : 0;
#ifndef SC64_RACES
        static const char *labels[MENU_COUNT] = {
	    "MOVE ORDER", "ATTACK MOVE", "BUILD BARRACKS", "BUILD DEPOT",
	    "BUILD REFINERY", "BUILD ACADEMY", "BUILD ENG BAY",
	    "WEAPON UPGRADE", "ARMOR UPGRADE", "STOP / HOLD"
	};
#endif
	gfx_rect(137, 14, 100, 96, C_PANEL);
	gfx_frame(137, 14, 100, 96, C_EDGE);
	for (i = first; i < field_menu_count() && i < first + 10; i++) {
            const char *label =
#ifdef SC64_RACES
                race_menu_label(i);
#else
                labels[i];
#endif
	    gfx_text(142, 17 + (i - first) * 9, i == menu_cursor ? ">" : " ", C_GOLD);
	    gfx_text(149, 17 + (i - first) * 9, label, i == menu_cursor ? C_WHITE : C_DIM);
	}
    }
#endif
#if !defined(SC_DEFAULT_UI) && !defined(SC64_FIELD_CONTROLS) /* field_ui previews the snapped site */
    if (mode == MODE_BUILD && box_state != 2) {
        int build_x = cx, build_y = cy;
#ifdef SC64_RACES
        if (menu == B_REFINERY || menu == B_EXTRACTOR || menu == B_ASSIMILATOR) {
            int geyser = geyser_at(cx, cy);
            if (geyser >= 0) {
                build_x = geysers[geyser].x;
                build_y = geysers[geyser].y;
            }
        }
#endif
	gfx_frame(build_x - camx - 8, build_y - camy + 12 - 6, 17, 13, (can_place(menu, cx, cy)
#ifdef SC64_RACES
            &&race_can_site(SIDE,menu,cx,cy)
#endif
            ) ? C_GREEN : C_RED);
    }
#endif
    gfx_frame(0, 12, 240, 108, C_EDGE);
}
