/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DEATH_VFX_RENDER_H
#define SC64_DEATH_VFX_RENDER_H

#include <stdint.h>
#include "../common/gba.h"
#include "sc64_death_vfx.h"

/* Authored death art drawn with GBA primitives. Coordinates are screen
 * pixels (already camera adjusted). Every write is clipped to the battlefield
 * above the lower HUD, and each effect's pixel work is bounded by its extent
 * (at most SC64_DEATH_VFX_MAX_EXTENT). This approximates the source death
 * families; it is not a reproduction of the source GRP frames. */
#define SC64_DEATH_VFX_VIEW_LEFT 0
#define SC64_DEATH_VFX_VIEW_RIGHT 239
#define SC64_DEATH_VFX_VIEW_TOP 12
#define SC64_DEATH_VFX_VIEW_BOTTOM 119

/* Clipped run of 'rows' (1 or 2) rows starting at y. */
static void
sc64_death_vfx_band(int x0, int x1, int y, int rows, int color)
{
    if (y < SC64_DEATH_VFX_VIEW_TOP) { rows -= SC64_DEATH_VFX_VIEW_TOP - y; y = SC64_DEATH_VFX_VIEW_TOP; }
    if (y + rows - 1 > SC64_DEATH_VFX_VIEW_BOTTOM) rows = SC64_DEATH_VFX_VIEW_BOTTOM - y + 1;
    if (rows <= 0) return;
    if (x0 < SC64_DEATH_VFX_VIEW_LEFT) x0 = SC64_DEATH_VFX_VIEW_LEFT;
    if (x1 > SC64_DEATH_VFX_VIEW_RIGHT) x1 = SC64_DEATH_VFX_VIEW_RIGHT;
    if (x0 > x1) return;
    gfx_rect(x0, y, x1 - x0 + 1, rows, (u8)color);
}

static void
sc64_death_vfx_span(int x0, int x1, int y, int color)
{
    sc64_death_vfx_band(x0, x1, y, 1, color);
}

static void
sc64_death_vfx_dot(int x, int y, int color)
{
    if (x < SC64_DEATH_VFX_VIEW_LEFT || x > SC64_DEATH_VFX_VIEW_RIGHT ||
        y < SC64_DEATH_VFX_VIEW_TOP || y > SC64_DEATH_VFX_VIEW_BOTTOM) return;
    gfx_pixel(x, y, (u8)color);
}

/* 255*sqrt(1-(i/32)^2): quarter-circle half widths, so rows need neither a
 * division nor a square root. */
static const unsigned char sc64_death_vfx_circle[33] = {
    255, 255, 255, 255, 254, 253, 251, 250, 248, 246, 243, 240, 237, 234, 230, 226, 222, 217, 212, 206, 200, 193, 186, 178, 169, 160, 149, 137, 124, 108, 89, 63, 0
};

/* Filled ellipse; radii are clamped to the effect bound. One reciprocal per
 * ellipse maps rows to the table, and each fill call covers two rows: the
 * per-call overhead, not pixel writes, dominates these small fills. */
static void
sc64_death_vfx_ellipse(int x, int y, int rx, int ry, int color)
{
    unsigned step;
    if (rx < 0 || ry < 0) return;
    if (rx > SC64_DEATH_VFX_MAX_EXTENT) rx = SC64_DEATH_VFX_MAX_EXTENT;
    if (ry > SC64_DEATH_VFX_MAX_EXTENT) ry = SC64_DEATH_VFX_MAX_EXTENT;
    if (!ry) { sc64_death_vfx_span(x - rx, x + rx, y, color); return; }
    step = (32u << 16) / (unsigned)ry;
    for (int dy = -ry; dy <= ry; dy += 2) {
        int row = y + dy, rows = dy < ry ? 2 : 1;
        if (row + rows <= SC64_DEATH_VFX_VIEW_TOP || row > SC64_DEATH_VFX_VIEW_BOTTOM) continue;
        int mid = dy < 0 ? -dy - (rows - 1) : dy; /* row of the pair nearer the center */
        unsigned index = ((unsigned)(mid < 0 ? 0 : mid) * step) >> 16;
        int hw = rx * sc64_death_vfx_circle[index > 32u ? 32u : index] / 255;
        sc64_death_vfx_band(x - hw, x + hw, row, rows, color);
    }
}

static uint32_t
sc64_death_vfx_hash(uint32_t v)
{
    v ^= v >> 16; v *= 0x7feb352du;
    v ^= v >> 15; v *= 0x846ca68bu;
    v ^= v >> 16;
    return v;
}

static const signed char sc64_death_vfx_dirs[8][2] = {
    { 0, -4 }, { 3, -3 }, { 4, 0 }, { 3, 2 },
    { 0, 3 }, { -3, 2 }, { -4, 0 }, { -3, -3 }
};

/* One fireball; t is 0..255 across its own life. */
static void
sc64_death_vfx_fireball(int x, int y, int radius, int t, const u8 pal[4],
                        int sparks)
{
    int r;
    /* Bound overdraw: nested fills of a large fireball dominate the cost. */
    if (radius > 32) radius = 32;
    r = radius * (96 + t * 160 / 256) / 256;
    if (r < 2) r = 2;
    if (t < 168) {
        /* Two fills and a hot rim keep large blasts cheap to redraw. */
        sc64_death_vfx_ellipse(x, y, r, r * 3 / 4, pal[0]);
        sc64_death_vfx_ellipse(x, y - 1, r / 2, r * 3 / 8, pal[2]);
        sc64_death_vfx_span(x - r * 3 / 4, x - r / 2, y, pal[1]);
        sc64_death_vfx_span(x + r / 2, x + r * 3 / 4, y, pal[1]);
        if (t < 72) sc64_death_vfx_dot(x, y - 1, pal[3]);
    } else {
        /* Dissipating smoke puffs with a few embers. */
        int puffs = t < 212 ? 6 : 3;
        int pr = r / 4 < 1 ? 1 : r / 4;
        for (int i = 0; i < puffs; ++i) {
            const signed char *d = sc64_death_vfx_dirs[i * 8 / puffs];
            int px = x + d[0] * r / 6, py = y + d[1] * r / 8 - (t - 168) / 16;
            sc64_death_vfx_ellipse(px, py, pr, pr * 3 / 4, i & 1 ? C_DIM : C_SHADOW);
        }
        sc64_death_vfx_dot(x - 1, y, pal[1]);
        sc64_death_vfx_dot(x + 2, y - 1, pal[0]);
    }
    if (sparks && t < 200) {
        for (int i = 0; i < 8; i += 2 - (sparks > 1)) {
            const signed char *d = sc64_death_vfx_dirs[i];
            int k = radius * t / 512 + 2;
            sc64_death_vfx_dot(x + d[0] * k / 3, y + d[1] * k / 4, pal[3]);
        }
    }
}

static const u8 sc64_death_vfx_fire[4] = { C_RED, C_ORANGE, C_GOLD, C_WHITE };
static const u8 sc64_death_vfx_psi[4] = { C_BLUE_DARK, C_BLUE, C_CYAN, C_WHITE };
static const u8 sc64_death_vfx_gore[4] = { C_RED_DARK, C_BLOOD, C_PURPLE, C_GREEN };

static void
sc64_death_vfx_gibs(int x, int y, int extent, int t, int fluid, int chunk)
{
    int land = t < 128 ? t : 128;
    if (t < 96) sc64_death_vfx_ellipse(x, y, extent * (96 - t) / 160 + 1,
                                       extent * (96 - t) / 256 + 1, fluid);
    for (int i = 0; i < 8; i += 1 + (t >= 128)) {
        const signed char *d = sc64_death_vfx_dirs[i];
        int k = extent * land / 128;
        int cx = x + d[0] * k / 4, cy = y + d[1] * k / 6;
        sc64_death_vfx_band(cx, cx + (t < 176), cy, 1 + (t < 176), i & 1 ? chunk : fluid);
    }
}

/* Rounded three-row puddle in two fill calls: a wide middle row over a
 * narrower three-row core. A flat two-row strip read as an orphaned health
 * bar (same height and dark red as an empty bar). Many remains can be on
 * screen, and per-call overhead, not pixels, is what each one costs. */
static void
sc64_death_vfx_puddle(int x, int y, int w, int edge, int core)
{
    if (w < 1) w = 1;
    sc64_death_vfx_band(x - w, x + w, y, 1, edge);
    sc64_death_vfx_band(x - w * 2 / 3, x + w * 2 / 3, y - 1, 3, core);
}

/* Source remnants decay through three frames; authored remains shrink in
 * three matching steps. stage is 0..2. */
static int
sc64_death_vfx_remnant_stage(uint32_t remnant_age, uint32_t remnant_total)
{
    if (!remnant_total) return 2;
    uint32_t s = remnant_age * 3u / remnant_total;
    return s > 2u ? 2 : (int)s;
}

static void
sc64_death_vfx_rubble(int style, int x, int y, int extent, int stage,
                      uint32_t seed)
{
    static const u8 terran[4] = { C_STONE, C_DIM, C_SHADOW, C_BROWN };
    static const u8 zerg[4] = { C_PURPLE, C_BROWN, C_BLOOD, C_SHADOW };
    static const u8 protoss[4] = { C_STONE, C_BLUE_DARK, C_SHADOW, C_SILVER };
    const u8 *pal = style == SC64_DEATH_VFX_BUILDING_ZERG ? zerg :
                    style == SC64_DEATH_VFX_BUILDING_PROTOSS ? protoss : terran;
    int rx = extent, ry = extent * 2 / 3;
    int pieces = stage == 0 ? 10 : stage == 1 ? 7 : 4;
    sc64_death_vfx_puddle(x, y + 2, rx * (4 - stage) / 5,
                          style == SC64_DEATH_VFX_BUILDING_ZERG ? C_BROWN : C_SHADOW,
                          style == SC64_DEATH_VFX_BUILDING_ZERG ? C_BLOOD : C_SHADOW);
    for (int i = 0; i < pieces; ++i) {
        uint32_t h = sc64_death_vfx_hash(seed + (uint32_t)i * 0x9e3779b9u);
        int px = x + (int)(h % (unsigned)(2 * rx + 1)) - rx;
        int py = y + (int)((h >> 12) % (unsigned)(2 * ry + 1)) - ry;
        int w = (int)((h >> 24) & 3) + 1;
        sc64_death_vfx_band(px, px + w, py, 2, pal[(h >> 20) & 3]);
    }
    if (style == SC64_DEATH_VFX_BUILDING_PROTOSS && stage < 2) {
        sc64_death_vfx_dot(x - rx / 3, y, C_CYAN);
        sc64_death_vfx_dot(x + rx / 4, y + 1, C_CYAN);
    }
}

/* Remnant (under units). age_su is the entry age. */
static void __attribute__((noinline))
sc64_death_vfx_draw_remnant(const Sc64DeathVfx *e, int x, int y, uint32_t age_su)
{
    uint32_t remnant_age, remnant_total;
    int stage, extent = e->extent;
    if (age_su < e->remnant_start || age_su >= e->total_su) return;
    remnant_age = age_su - e->remnant_start;
    remnant_total = (uint32_t)e->total_su - e->remnant_start;
    stage = sc64_death_vfx_remnant_stage(remnant_age, remnant_total);
    switch (e->style) {
    case SC64_DEATH_VFX_INFANTRY:
        sc64_death_vfx_puddle(x, y, extent * (3 - stage) / 4 + 1,
                              stage == 2 ? C_BROWN : C_BLOOD,
                              stage == 2 ? C_BROWN : C_BLOOD);
        break;
    case SC64_DEATH_VFX_ZERG_SMALL:
    case SC64_DEATH_VFX_ZERG_LARGE:
        /* Purple ichor at the core while fresh, drying to brown. */
        sc64_death_vfx_puddle(x, y, extent * (3 - stage) / 3 + 1,
                              stage == 2 ? C_BROWN : C_BLOOD,
                              stage == 2 ? C_BROWN : C_PURPLE);
        break;
    case SC64_DEATH_VFX_BUILDING_TERRAN:
    case SC64_DEATH_VFX_BUILDING_ZERG:
    case SC64_DEATH_VFX_BUILDING_PROTOSS:
        sc64_death_vfx_rubble(e->style, x, y, extent, stage,
                              ((uint32_t)(uint16_t)e->x << 16) ^ (uint16_t)e->y ^
                              e->raw);
        break;
    default: break;
    }
}

/* Burst (over units). */
static void __attribute__((noinline))
sc64_death_vfx_draw_burst(const Sc64DeathVfx *e, int x, int y, uint32_t age_su)
{
    int extent = e->extent, t;
    if (!e->burst_su || age_su >= e->burst_su) return;
    t = (int)(age_su * 256u / e->burst_su);
    switch (e->style) {
    case SC64_DEATH_VFX_INFANTRY:
        /* Collapse: the body sinks while blood spreads. */
        if (t < 160) sc64_death_vfx_ellipse(x, y - 2 + t / 64, extent / 2,
                                            extent * (160 - t) / 400 + 1, e->color);
        sc64_death_vfx_ellipse(x, y + 1, extent * t / 384 + 1, extent * t / 768 + 1,
                               C_BLOOD);
        if (t < 128) for (int i = 0; i < 8; i += 2) {
            const signed char *d = sc64_death_vfx_dirs[i + 1];
            sc64_death_vfx_dot(x + d[0] * t / 64, y + d[1] * t / 96, C_RED);
        }
        break;
    case SC64_DEATH_VFX_EXPLOSION_SMALL:
        sc64_death_vfx_fireball(x, y, extent + 4, t, sc64_death_vfx_fire, 1);
        break;
    case SC64_DEATH_VFX_EXPLOSION_LARGE:
        sc64_death_vfx_fireball(x, y, extent + 8, t, sc64_death_vfx_fire, 2);
        break;
    case SC64_DEATH_VFX_PROTOSS_SMALL:
    case SC64_DEATH_VFX_PROTOSS_LARGE: {
        int big = e->style == SC64_DEATH_VFX_PROTOSS_LARGE;
        sc64_death_vfx_fireball(x, y, extent + (big ? 8 : 4), t,
                                sc64_death_vfx_psi, big + 1);
        if (t < 112) {
            /* Shock ring of the blue energy release. */
            int r = (extent + 6) * (64 + t) / 176;
            for (int i = 0; i < 8; ++i) {
                const signed char *d = sc64_death_vfx_dirs[i];
                sc64_death_vfx_dot(x + d[0] * r / 4, y + d[1] * r / 5, C_CYAN);
            }
        }
        break;
    }
    case SC64_DEATH_VFX_ZERG_SMALL:
    case SC64_DEATH_VFX_ZERG_LARGE:
        sc64_death_vfx_gibs(x, y, extent + (e->style == SC64_DEATH_VFX_ZERG_LARGE ? 6 : 2),
                            t, C_BLOOD, C_PURPLE);
        break;
    case SC64_DEATH_VFX_ZERG_AIR:
        sc64_death_vfx_fireball(x, y, extent + 2, t < 128 ? t : 128 + (t - 128) / 2,
                                sc64_death_vfx_gore, 0);
        sc64_death_vfx_gibs(x, y + t / 32, extent + 4, t, C_GREEN, C_PURPLE);
        break;
    case SC64_DEATH_VFX_BUILDING_TERRAN:
    case SC64_DEATH_VFX_BUILDING_ZERG:
    case SC64_DEATH_VFX_BUILDING_PROTOSS: {
        static const unsigned char starts[4] = { 0, 40, 84, 124 };
        const u8 *pal = e->style == SC64_DEATH_VFX_BUILDING_ZERG ? sc64_death_vfx_gore :
                        e->style == SC64_DEATH_VFX_BUILDING_PROTOSS ?
                        sc64_death_vfx_psi : sc64_death_vfx_fire;
        uint32_t seed = ((uint32_t)(uint16_t)e->x << 16) ^ (uint16_t)e->y;
        for (int i = 0; i < 4; ++i) {
            int lt = (t - starts[i]) * 2;
            if (lt < 0 || lt > 255) continue;
            uint32_t h = sc64_death_vfx_hash(seed + (uint32_t)i * 0x45d9f3bu);
            int ox = i == 3 ? 0 : (int)(h % (unsigned)(extent + 1)) - extent / 2;
            int oy = i == 3 ? 0 : (int)((h >> 10) % (unsigned)(extent / 2 + 1)) - extent / 4;
            sc64_death_vfx_fireball(x + ox, y + oy, i == 3 ? extent : extent / 2 + 4,
                                    lt, pal, i == 3 ? 2 : 0);
        }
        if (e->style == SC64_DEATH_VFX_BUILDING_ZERG)
            sc64_death_vfx_gibs(x, y, extent, t, C_BLOOD, C_PURPLE);
        break;
    }
    default: break;
    }
}

#ifdef SC64_DEATH_SOURCE_ART
/* Clip source frames to the battlefield rows above the cached lower HUD. */
static void
death_vfx_blit(const Sc64SpriteFrame *s, int cx, int cy)
{
    int x = cx - s->canvas_w / 2 + s->offset_x;
    int y = cy - s->canvas_h / 2 + s->offset_y, h = s->h;
    const u8 *pixels = s->pixels;
    if (y < 12) {
        int skip = 12 - y;
        if (skip >= h) return;
        pixels += skip * s->w; h -= skip; y = 12;
    }
    if (y + h > 120) h = 120 - y;
    if (h > 0) gfx_blit_sprite(x, y, s->w, h, pixels);
}

static void __attribute__((noinline))
death_vfx_draw_source_layer(const Sc64DeathSourceLayer *layer, int x, int y, unsigned frame)
{
    const Sc64SpriteFrame *sprite = sc64_death_image_frame(layer->image_id, (unsigned short)frame);
    if (!sprite) return;
#if defined(SC64_UNIT_WORD_SPRITES) && !defined(SC64_DEATH_INDEXED_REMNANTS)
    const GbaSpriteWordFrame *words =
        sc64_death_image_word_frame(layer->image_id, (unsigned short)frame);
    int left = x + layer->x - sprite->canvas_w / 2 + sprite->offset_x;
    int top = y + layer->y - sprite->canvas_h / 2 + sprite->offset_y;
    if (words && top >= 12 && top + (int)sprite->h <= 120) {
        gfx_blit_sprite_words(left, top, words);
        return;
    }
#endif
    death_vfx_blit(sprite, x + layer->x, y + layer->y);
}

#ifdef SC64_DEATH_WORLD_DEPTH
static inline int
death_vfx_is_actor_sprite(int raw, const Sc64DeathSourceLayer *piece)
{
    if (piece->flags & SC64_DEATH_SOURCE_ACTOR_SPRITE) return 1;
#if SC64_DEATH_SOURCE_METADATA_VERSION == 2
    /* The legacy compiler marked only direct imgol attachments. These
     * nested rows are also images of the original sprite in the USA
     * Death scripts. Match the existing bank signatures exactly. */
    if (piece->flags || piece->remnant || piece->x || piece->y) return 0;
    if (piece->start_su == 2 && piece->end_su == 18) {
        switch (raw) {
        case 42: case 44: case 56: case 57:
            return piece->image_id == 58;
        case 43: case 55: case 59:
            return piece->image_id == 59;
        }
    }
    return raw == 147 && piece->image_id == 60 &&
        piece->start_su == 66 && piece->end_su == 90;
#else
    (void)raw;
    return 0;
#endif
}

static inline int
death_vfx_world_layer(const Sc64DeathVfx *e, const Sc64DeathSourceLayer *piece)
{
    unsigned flags = piece->flags;
    if (flags & SC64_DEATH_SOURCE_DETACHED_SPRITE) {
        int base = (flags & SC64_DEATH_SOURCE_FIXED_LOW) ? 1 : e->actor_layer_plus1 - 1;
        int offset = (int)(flags >> SC64_DEATH_SOURCE_LAYER_SHIFT) -
            SC64_DEATH_SOURCE_LAYER_BIAS;
        /* Native stores a byte. The painter uses all eight bits for the Y
         * test, then the low five bits for the packed elevation key. */
        return base >= 0 ? (int)((unsigned)(base + offset) & 255u) : -1;
    }
    if (flags & SC64_DEATH_SOURCE_FIXED_LOW) return 1;
    if (death_vfx_is_actor_sprite(e->raw, piece)) return e->actor_layer_plus1 - 1;
    return -1;
}
#endif

static int
death_vfx_draw_source(const Sc64DeathVfx *e, int x, int y, u32 age, int under)
{
    const Sc64DeathSource *art = sc64_death_source(e->raw);
    if (!sc64_death_source_valid(art)) return 0;
    for (unsigned i = 0; i < art->layer_count; ++i) {
        const Sc64DeathSourceLayer *layer = &art->layers[i];
        unsigned frame_index;
#ifdef SC64_DEATH_WORLD_DEPTH
        if (death_vfx_world_layer(e, layer) >= 0) continue;
#endif
        if ((layer->flags & SC64_DEATH_SOURCE_UNDER) != under ||
            !sc64_death_source_layer_frame(layer, age, &frame_index)) continue;
        death_vfx_draw_source_layer(layer, x, y, frame_index);
    }
    return 1;
}
#endif

#endif
