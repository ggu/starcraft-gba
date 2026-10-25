/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DEATH_VFX_H
#define SC64_DEATH_VFX_H

#include <stdint.h>

/* Transient death presentation. Entries are observed after a simulation step
 * from actor rows that changed from alive to dead with non-positive HP; they
 * are never read by the simulation and are absent from every save format.
 * Timing uses source updates (24 Hz) derived from the 30-Hz game frame, as
 * the construction-art sampler does. The style families follow the source
 * IScript Death animations of StarCraft 64: Terran infantry and Zerg ground
 * units play their own collapse frames and leave a remnant sprite; vehicles,
 * aircraft and Protoss units spawn an explosion overlay; buildings explode
 * and leave rubble. Authored styles approximate those families with GBA
 * primitives when source frames are not compiled in. */
enum {
    SC64_DEATH_VFX_NONE = 0,
    SC64_DEATH_VFX_INFANTRY,
    SC64_DEATH_VFX_EXPLOSION_SMALL,
    SC64_DEATH_VFX_EXPLOSION_LARGE,
    SC64_DEATH_VFX_ZERG_SMALL,
    SC64_DEATH_VFX_ZERG_LARGE,
    SC64_DEATH_VFX_ZERG_AIR,
    SC64_DEATH_VFX_PROTOSS_SMALL,
    SC64_DEATH_VFX_PROTOSS_LARGE,
    SC64_DEATH_VFX_BUILDING_TERRAN,
    SC64_DEATH_VFX_BUILDING_ZERG,
    SC64_DEATH_VFX_BUILDING_PROTOSS,
    SC64_DEATH_VFX_STYLE_COUNT
};

/* Source rubble lasts minutes; a transient pool keeps it for 30 seconds. */
#define SC64_DEATH_VFX_RUBBLE_SU 720u
/* Source unit remnants: three frames of 51 updates each. */
#define SC64_DEATH_VFX_REMNANT_SU 153u
#define SC64_DEATH_VFX_MAX_EXTENT 48

typedef struct {
    uint16_t burst_su;      /* collapse/explosion phase */
    uint16_t remnant_start; /* remnant appears (under the burst) here */
    uint16_t total_su;      /* entry expires at this age */
} Sc64DeathVfxTiming;

typedef struct {
    int16_t x, y;
    uint32_t birth;         /* game frame of the observed death */
    uint16_t raw;           /* source unit id, or 0xffff when unknown */
    uint16_t burst_su, remnant_start, total_su;
    uint8_t style, extent;  /* style 0 marks a free slot */
    uint8_t source;         /* 1 when compiled source frames drive it */
    uint8_t color;          /* owner color for authored infantry */
} Sc64DeathVfx;

_Static_assert(sizeof(Sc64DeathVfx) == 20, "Death effect pool row budget");

/* Authored timelines mirror the source frame counts: 3-update frames, 7-10
 * frame unit collapses or explosions, and 12-14 frame building explosions.
 * Unit remnants follow the collapse; building rubble appears four updates in,
 * under the explosion, as the source lowsprul does. */
static inline Sc64DeathVfxTiming
sc64_death_vfx_timing(int style)
{
    Sc64DeathVfxTiming t = { 0, 0, 0 };
    switch (style) {
    case SC64_DEATH_VFX_INFANTRY:
        t.burst_su = 24; t.remnant_start = 24;
        t.total_su = 24 + SC64_DEATH_VFX_REMNANT_SU; break;
    case SC64_DEATH_VFX_ZERG_SMALL:
        t.burst_su = 24; t.remnant_start = 24;
        t.total_su = 24 + SC64_DEATH_VFX_REMNANT_SU; break;
    case SC64_DEATH_VFX_ZERG_LARGE:
        t.burst_su = 30; t.remnant_start = 30;
        t.total_su = 30 + SC64_DEATH_VFX_REMNANT_SU; break;
    case SC64_DEATH_VFX_EXPLOSION_SMALL:
    case SC64_DEATH_VFX_ZERG_AIR:
    case SC64_DEATH_VFX_PROTOSS_SMALL:
        t.burst_su = t.remnant_start = t.total_su = 27; break;
    case SC64_DEATH_VFX_EXPLOSION_LARGE:
    case SC64_DEATH_VFX_PROTOSS_LARGE:
        t.burst_su = t.remnant_start = t.total_su = 30; break;
    case SC64_DEATH_VFX_BUILDING_TERRAN:
    case SC64_DEATH_VFX_BUILDING_PROTOSS:
        t.burst_su = 42; t.remnant_start = 4;
        t.total_su = 4 + SC64_DEATH_VFX_RUBBLE_SU; break;
    case SC64_DEATH_VFX_BUILDING_ZERG:
        t.burst_su = 36; t.remnant_start = 4;
        t.total_su = 4 + SC64_DEATH_VFX_RUBBLE_SU; break;
    default: break;
    }
    return t;
}

/* Family selection uses StarCraft unit ids, so every profile (including the
 * original-art build, which maps its few types to those ids) shares it. */
static inline int
sc64_death_vfx_style_for_raw(int raw, int building)
{
    if (building) {
        if (raw >= 131 && raw <= 152) return SC64_DEATH_VFX_BUILDING_ZERG;
        if ((raw >= 154 && raw <= 172) || raw == 174 || raw == 175)
            return SC64_DEATH_VFX_BUILDING_PROTOSS;
        return SC64_DEATH_VFX_BUILDING_TERRAN;
    }
    switch (raw) {
    case 0: case 1: case 20: case 34:          /* collapse frames + remains */
    case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96:
        return SC64_DEATH_VFX_INFANTRY;
    case 5: case 9: case 12: case 22: case 23: case 25:
    case 27: case 28: case 29: case 30:        /* large unit explosion */
        return SC64_DEATH_VFX_EXPLOSION_LARGE;
    case 35: case 36: case 37: case 38: case 40: case 41: case 46:
    case 50: case 52: case 53: case 54: case 103:
        return SC64_DEATH_VFX_ZERG_SMALL;
    case 39: case 48:
        return SC64_DEATH_VFX_ZERG_LARGE;
    case 42: case 43: case 44: case 45: case 47: case 49: case 51:
    case 55: case 56: case 57: case 59: case 62:
        return SC64_DEATH_VFX_ZERG_AIR;
    case 68: case 72: case 76: case 81: case 82: case 83: case 84:
        return SC64_DEATH_VFX_PROTOSS_LARGE;
    default: break;
    }
    if (raw == 60 || raw == 61 || (raw >= 63 && raw <= 88))
        return SC64_DEATH_VFX_PROTOSS_SMALL;
    return SC64_DEATH_VFX_EXPLOSION_SMALL;
}

/* Elapsed source updates (floor(frames*4/5)) for any frame distance. Every
 * entry expires within about a thousand frames, so distances of 16384 frames
 * or more saturate; below that the multiply-shift is exact and avoids a
 * library division in the per-frame draw loop. */
#define SC64_DEATH_VFX_AGE_SATURATED 0xffffu
static inline uint32_t
sc64_death_vfx_age_su(uint32_t birth, uint32_t now)
{
    uint32_t frames = now - birth;
    if (frames >= 16384u) return SC64_DEATH_VFX_AGE_SATURATED;
    return (frames * 52429u) >> 16;
}

static inline int
sc64_death_vfx_expired(const Sc64DeathVfx *e, uint32_t now)
{
    return !e->style || sc64_death_vfx_age_su(e->birth, now) >= e->total_su;
}

/* Choose a slot: a free or expired row first; otherwise the oldest row whose
 * burst has finished (only a remnant remains); otherwise the oldest row.
 * Fresh deaths therefore never wait behind long-lived rubble. */
static inline int
sc64_death_vfx_slot(const Sc64DeathVfx *pool, int count, uint32_t now)
{
    int best = -1, best_remnant = 0;
    uint32_t best_age = 0;
    for (int i = 0; i < count; ++i) {
        const Sc64DeathVfx *e = &pool[i];
        uint32_t age;
        int remnant;
        if (sc64_death_vfx_expired(e, now)) return i;
        age = sc64_death_vfx_age_su(e->birth, now);
        remnant = age >= e->burst_su;
        if (best < 0 || remnant > best_remnant ||
            (remnant == best_remnant && age > best_age)) {
            best = i; best_remnant = remnant; best_age = age;
        }
    }
    return best;
}

static inline int
sc64_death_vfx_spawn(Sc64DeathVfx *pool, int count, uint32_t now, int style,
                     int raw, int x, int y, int extent,
                     const Sc64DeathVfxTiming *timing, int source,
                     int color)
{
    Sc64DeathVfx *e;
    int slot;
    if (!pool || count <= 0 || style <= SC64_DEATH_VFX_NONE ||
        style >= SC64_DEATH_VFX_STYLE_COUNT || !timing || !timing->total_su)
        return -1;
    slot = sc64_death_vfx_slot(pool, count, now);
    if (slot < 0) return -1;
    if (extent < 4) extent = 4;
    if (extent > SC64_DEATH_VFX_MAX_EXTENT) extent = SC64_DEATH_VFX_MAX_EXTENT;
    if (x < -32768) x = -32768; else if (x > 32767) x = 32767;
    if (y < -32768) y = -32768; else if (y > 32767) y = 32767;
    e = &pool[slot];
    e->x = (int16_t)x; e->y = (int16_t)y;
    e->birth = now;
    e->raw = raw >= 0 && raw < 0xffff ? (uint16_t)raw : 0xffffu;
    e->burst_su = timing->burst_su;
    e->remnant_start = timing->remnant_start;
    e->total_su = timing->total_su;
    e->style = (uint8_t)style;
    e->extent = (uint8_t)extent;
    e->source = (uint8_t)(source != 0);
    e->color = (uint8_t)color;
    return slot;
}

static inline int
sc64_death_vfx_active(const Sc64DeathVfx *pool, int count, uint32_t now)
{
    int active = 0;
    for (int i = 0; i < count; ++i) active += !sc64_death_vfx_expired(&pool[i], now);
    return active;
}

#endif
