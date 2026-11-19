/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MINIMAP_DRAW_H
#define SC64_MINIMAP_DRAW_H

#ifdef SC64_IMPORTED_MISSION
static void sc64_hud_bottom_band_invalidate(void);
#endif
#if defined(SC_FRONTIER_FACTION) && !defined(SC64_IMPORTED_MISSION)
#include "frontier_minimap.h"
#endif

enum { SC64_MINIMAP_X = 197, SC64_MINIMAP_Y = 129, SC64_MINIMAP_SIZE = 24 };

static inline int
sc64_minimap_building_visible(const Building *q)
{
    if (q->owner != SIDE &&
        (q->x < 0 || q->y < 0 || q->x >= WORLD_W || q->y >= WORLD_H ||
         !currently_visible(q->x, q->y))) return 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    unsigned flags = sc64_game_building_actor_flags(q->type);
    if (flags && !(flags & SC64_GAME_ACTOR_DRAWABLE)) return 0;
#endif
    return 1;
}

static inline int
sc64_minimap_unit_visible(int slot)
{
    const Unit *q = &u[slot];
    if (SC64_TRANSPORT_LOADED(q) || SC64_SCARAB_HIDDEN(q)) return 0;
    if (q->owner != SIDE &&
        (q->x < 0 || q->y < 0 || q->x >= WORLD_W || q->y >= WORLD_H ||
         !currently_visible(q->x, q->y))) return 0;
#ifdef SC64_P7_OBSERVER_ENABLED
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (q->owner != SIDE &&
        !sc64_p7_observer_target_revealed_to(slot, 7, SIDE)) return 0;
#else
    if (race_p7_active() && q->owner != SIDE && q->raw_type == 84 &&
        !sc64_p7_observer_detector_sees(slot, 7, SIDE)) return 0;
#endif
#endif
#ifdef SC64_Z8_RUNTIME
    if (q->owner != SIDE && race_z8_unit_hidden_from_side(slot)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_unit_burrowed(slot)) return 0;
#endif
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed(slot)) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    unsigned flags = sc64_game_unit_actor_flags(q->type);
    if ((flags & SC64_GAME_ACTOR_HIDDEN) || SC64_CARRIER_STATE_HIDDEN(q) ||
        (flags && !(flags & SC64_GAME_ACTOR_DRAWABLE))) return 0;
#endif
    return 1;
}

static inline void
sc64_minimap_project(int x, int y, int *map_x, int *map_y)
{
#if defined(SC64_IMPORTED_MISSION) || defined(SC_ORIGINAL_SKIRMISH)
#ifdef SC64_RACES
    *map_x = SC64_MINIMAP_X + sc64_minimap_pixel(x, MAP_COLS);
    *map_y = SC64_MINIMAP_Y + sc64_minimap_pixel(y, MAP_ROWS);
#else
    *map_x = SC64_MINIMAP_X + x * SC64_MINIMAP_SIZE / WORLD_W;
    *map_y = SC64_MINIMAP_Y + y * SC64_MINIMAP_SIZE / WORLD_H;
#endif
#else
    *map_x = 185 + x / 8;
    *map_y = 126 + y / 8;
#endif
}

static inline int
sc64_minimap_inside(int x, int y)
{
    return (unsigned)(x - SC64_MINIMAP_X) < SC64_MINIMAP_SIZE &&
           (unsigned)(y - SC64_MINIMAP_Y) < SC64_MINIMAP_SIZE;
}

static inline void
sc64_minimap_dot(int x, int y, u8 color)
{
    gfx_pixel(x, y, color);
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    /* The next band restore clears marks drawn outside the raster. */
    if (!sc64_minimap_inside(x, y))
        sc64_hud_bottom_band_invalidate();
#endif
}

static void
sc64_minimap_draw_indicators(void)
{
#if defined(SC64_IMPORTED_MISSION) || defined(SC_ORIGINAL_SKIRMISH)
    int vx, vy;
    int vw = (240 * SC64_MINIMAP_SIZE + WORLD_W - 1) / WORLD_W;
    int vh = (108 * SC64_MINIMAP_SIZE + WORLD_H - 1) / WORLD_H;
    sc64_minimap_project(camx, camy, &vx, &vy);
    gfx_frame(vx, vy, vw, vh, C_HIGHLIGHT);
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    if (vx < SC64_MINIMAP_X || vy < SC64_MINIMAP_Y ||
        vx + vw > SC64_MINIMAP_X + SC64_MINIMAP_SIZE ||
        vy + vh > SC64_MINIMAP_Y + SC64_MINIMAP_SIZE)
        sc64_hud_bottom_band_invalidate();
#endif
#else
    gfx_frame(185 + camx / 8, 126 + camy / 8, 30, 14, C_HIGHLIGHT);
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active() && screen == SCREEN_PLAY && !won && !lost &&
        race_t4.ping_ticks) {
        int px = sc64_minimap_pixel(race_t4.ping_x, MAP_COLS);
        int py = sc64_minimap_pixel(race_t4.ping_y, MAP_ROWS);
        unsigned radius = (race_t4.ping_ticks & 8u) ? 1u : 2u;
        if (px >= 0 && px < SC64_MINIMAP_SIZE && py >= 0 && py < SC64_MINIMAP_SIZE) {
            /* The cross pulses between two radii. Clip to the 24x24 map, not
             * merely the containing panel, so edge pings cannot paint its
             * frame or neighboring HUD. */
            for (int offset = -(int)radius; offset <= (int)radius; ++offset) {
                int ax = px + offset, ay = py + offset;
                if (ax >= 0 && ax < SC64_MINIMAP_SIZE)
                    gfx_pixel(SC64_MINIMAP_X + ax, SC64_MINIMAP_Y + py, C_GOLD);
                if (ay >= 0 && ay < SC64_MINIMAP_SIZE)
                    gfx_pixel(SC64_MINIMAP_X + px, SC64_MINIMAP_Y + ay, C_GOLD);
            }
            gfx_pixel(SC64_MINIMAP_X + px, SC64_MINIMAP_Y + py, C_WHITE);
        }
    }
#endif
}

static void
draw_minimap(void)
{
#ifndef SC64_MINIMAP_RASTER_ACTIVE
    gfx_rect(181, 124, 56, 34, C_BLACK);
    gfx_frame(181, 124, 56, 34, C_EDGE);
#endif
#ifdef SC64_IMPORTED_MISSION
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    /* Every map pixel is replaced, including marks left on this page. */
    gfx_blit_opaque(SC64_MINIMAP_X, SC64_MINIMAP_Y,
                    SC64_MINIMAP_SIZE, SC64_MINIMAP_SIZE, sc64_minimap_raster);
#else
#ifdef SC64_TILE_RENDERER
    if (gfx_tile_active()) {
        /* The panel fill above already marked these tiles; store the same
         * colors straight into the linear software layer. */
        u8 *panel = gfx_tile_buffer_pixels() + SC64_MINIMAP_Y * 240 + SC64_MINIMAP_X;
        for (int row = 0; row < SC64_MINIMAP_SIZE; row++, panel += 240)
            for (int col = 0; col < SC64_MINIMAP_SIZE; col++) {
                int cell = row * SC64_MINIMAP_SIZE + col;
                if (sc64_minimap_visible[cell]) panel[col] = C_GRASS;
                else if (sc64_minimap_explored[cell]) panel[col] = C_FOG;
            }
    } else
#endif
    sc64_minimap_draw_terrain(SC64_MINIMAP_X, SC64_MINIMAP_Y,
                              sc64_minimap_visible, sc64_minimap_explored);
#endif
#elif defined(SC_FRONTIER_FACTION)
    frontier_minimap_draw_terrain(SC64_MINIMAP_X, SC64_MINIMAP_Y);
#elif defined(SC_ORIGINAL_SKIRMISH)
    for (int row = 0; row < SC64_MINIMAP_SIZE; row++) {
        int grid_row = (2 * row + 1) * MAP_ROWS / (2 * SC64_MINIMAP_SIZE);
        for (int col = 0; col < SC64_MINIMAP_SIZE; col++) {
            int cell = grid_row * MAP_COLS + (2 * col + 1) * MAP_COLS / (2 * SC64_MINIMAP_SIZE);
            if (SC64_GRID_GET(visible, cell))
                gfx_pixel(SC64_MINIMAP_X + col, SC64_MINIMAP_Y + row, C_GRASS);
            else if (SC64_GRID_GET(explored, cell))
                gfx_pixel(SC64_MINIMAP_X + col, SC64_MINIMAP_Y + row, C_FOG);
        }
    }
#else
    /* Neighboring tiles of the same shade can be painted together.
     * Unexplored tiles leave the panel's background and frame intact. */
    for (int row = 0; row < MAP_ROWS; row++) {
        int start = 0, shade = -1;
        for (int col = 0; col <= MAP_COLS; col++) {
            int next = -1;
            if (col < MAP_COLS && cell_explored(col, row))
                next = SC64_GRID_GET(visible, col + row * MAP_COLS) ? C_GRASS : C_FOG;
            if (next == shade)
                continue;
            if (shade >= 0)
                gfx_rect(185 + start * 2, 126 + row * 2, (col - start) * 2, 2, (u8)shade);
            start = col;
            shade = next;
        }
    }
#endif
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        int x, y;
        if (!q->alive || !sc64_minimap_building_visible(q)) continue;
        sc64_minimap_project(q->x, q->y, &x, &y);
        sc64_minimap_dot(x, y,
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
                       owner_color(q->owner)
#else
                       q->owner == SIDE ? C_TERRAN : C_RED
#endif
        );
    }
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        int x, y;
        if (!q->alive || !sc64_minimap_unit_visible(i)) continue;
        sc64_minimap_project(q->x, q->y, &x, &y);
        sc64_minimap_dot(x, y,
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
                       owner_color(q->owner)
#else
                       q->owner == SIDE ? C_WHITE : C_RED
#endif
        );
    }
    sc64_minimap_draw_indicators();
}

#if defined(SC64_HUD_MINIMAP_CENSUS) && defined(SC64_RACES) && \
    defined(SC64_MINIMAP_RASTER_ACTIVE)
#define SC64_HUD_MINIMAP_CENSUS_ACTIVE 1

/* Scratch for one HUD draw. Fog keeps its own raster and can change freely
 * between draws. Copy whole words to avoid the freestanding bytewise memcpy. */
enum { SC64_MINIMAP_WORDS = SC64_MINIMAP_SIZE * SC64_MINIMAP_SIZE / sizeof(u32) };
static _Alignas(4) u32 sc64_minimap_composed[SC64_MINIMAP_WORDS];
static int sc64_minimap_requires_scalar;

static void
sc64_minimap_census_begin(void)
{
    for (unsigned i = 0; i < SC64_MINIMAP_WORDS; ++i)
        __builtin_memcpy(&sc64_minimap_composed[i],
                         sc64_minimap_raster + i * sizeof(u32), sizeof(u32));
    sc64_minimap_requires_scalar = 0;
}

static inline void
sc64_minimap_census_dot(int wx, int wy, int owner)
{
    int x, y;
    sc64_minimap_project(wx, wy, &x, &y);
    if (!sc64_minimap_inside(x, y)) {
        /* A mark may touch the cached HUD border. The scalar path owns
         * its clipping and page invalidation. */
        sc64_minimap_requires_scalar = 1;
        return;
    }
    u8 *pixels = (u8 *)(void *)sc64_minimap_composed;
    pixels[(y - SC64_MINIMAP_Y) * SC64_MINIMAP_SIZE + x - SC64_MINIMAP_X] =
        owner_color(owner);
}

static inline void
sc64_minimap_census_building(int slot)
{
    const Building *q = &b[slot];
    if (sc64_minimap_building_visible(q))
        sc64_minimap_census_dot(q->x, q->y, q->owner);
}

static inline void
sc64_minimap_census_unit(int slot)
{
    const Unit *q = &u[slot];
    if (sc64_minimap_unit_visible(slot))
        sc64_minimap_census_dot(q->x, q->y, q->owner);
}

static void
sc64_minimap_draw_census(void)
{
    if (sc64_minimap_requires_scalar) {
        draw_minimap();
        return;
    }
    gfx_blit_opaque(SC64_MINIMAP_X, SC64_MINIMAP_Y,
                    SC64_MINIMAP_SIZE, SC64_MINIMAP_SIZE,
                    (const u8 *)(const void *)sc64_minimap_composed);
    sc64_minimap_draw_indicators();
}
#endif

#endif
