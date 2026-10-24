#ifndef SC64_CONSTRUCTION_VFX_H
#define SC64_CONSTRUCTION_VFX_H

#include "../common/gba.h"

/* Temporary authored construction art. These effects use only existing GBA
 * drawing primitives; they are not a reproduction of the source game's
 * IScript animation or extracted construction GRPs. Coordinates are the
 * building center in screen pixels. */
enum {
    SC64_CONSTRUCTION_RACE_TERRAN = 1,
    SC64_CONSTRUCTION_RACE_ZERG = 2,
    SC64_CONSTRUCTION_RACE_PROTOSS = 3
};

#define SC64_CONSTRUCTION_VFX_MIN_DIM 8
#define SC64_CONSTRUCTION_VFX_MAX_DIM 96

static inline int
sc64_construction_vfx_phase(int progress, int duration, u32 clock)
{
    u32 progress_phase;
    if (duration <= 0 || progress < 0) return -1;
    if (progress > duration) progress = duration;
    progress_phase = (u32)(((uint64_t)(unsigned)progress * 4u) /
                           (unsigned)duration);
    if (progress_phase > 3u) progress_phase = 3u;
    return (int)(((clock >> 2) + progress_phase) & 3u);
}

static inline int
sc64_construction_vfx_clamp_dim(int value)
{
    if (value < SC64_CONSTRUCTION_VFX_MIN_DIM)
        return SC64_CONSTRUCTION_VFX_MIN_DIM;
    if (value > SC64_CONSTRUCTION_VFX_MAX_DIM)
        return SC64_CONSTRUCTION_VFX_MAX_DIM;
    return value;
}

static inline int
sc64_construction_vfx_race_valid(int race)
{
    return race == SC64_CONSTRUCTION_RACE_TERRAN ||
           race == SC64_CONSTRUCTION_RACE_ZERG ||
           race == SC64_CONSTRUCTION_RACE_PROTOSS;
}

/* Draw a bounded overlay; progress < 0 means the worker is still en route.
 * That marker is deliberately static (clock-independent), while construction
 * effects animate from existing progress and the ordinary game clock. */
static inline int
sc64_construction_vfx_draw(int race, int x, int y, int width, int height,
                           int progress, int duration, u32 clock)
{
    int rx, ry, phase;
    if (!sc64_construction_vfx_race_valid(race) || duration <= 0) return 0;

    width = sc64_construction_vfx_clamp_dim(width);
    height = sc64_construction_vfx_clamp_dim(height);
    rx = width / 2;
    ry = height / 2;

    if (progress < 0) {
        gfx_frame(x - 3, y - 3, 7, 7, C_UI);
        gfx_pixel(x, y, C_WHITE);
        gfx_pixel(x - 2, y, C_GOLD);
        return 1;
    }
    phase = sc64_construction_vfx_phase(progress, duration, clock);
    if (phase < 0) return 0;

    if (race == SC64_CONSTRUCTION_RACE_TERRAN) {
        int sx = rx < 6 ? 6 : rx;
        int sy = ry < 6 ? 6 : ry;
        int spark_x = x - sx + ((phase * 7 + 2) % (2 * sx + 1));
        int spark_y = y - sy + ((phase * 5 + 1) % (2 * sy + 1));
        gfx_frame(x - sx, y - sy, 2 * sx + 1, 2 * sy + 1, C_TERRAN);
        gfx_line(x - sx, y - sy, x - sx + 4, y - sy + 4, C_GOLD);
        gfx_line(x + sx, y - sy, x + sx - 4, y - sy + 4, C_GOLD);
        gfx_line(x - sx, y + sy, x - sx + 4, y + sy - 4, C_GOLD);
        gfx_line(x + sx, y + sy, x + sx - 4, y + sy - 4, C_GOLD);
        gfx_pixel(spark_x, spark_y, C_WHITE);
        gfx_pixel(spark_x + 1, spark_y, C_ORANGE);
    } else if (race == SC64_CONSTRUCTION_RACE_ZERG) {
        int pod_x=rx>18?18:rx;
        int pod_y=ry>12?12:ry;
        int pulse_x = 4 + phase * 2;
        int pulse_y = 3 + phase;
        /* Sparse outline keeps large Hatchery morphs inexpensive to redraw. */
        gfx_line(x-pod_x,y,x,y-pod_y,C_ZERG);
        gfx_line(x,y-pod_y,x+pod_x,y,C_PURPLE);
        gfx_line(x+pod_x,y,x,y+pod_y,C_ZERG);
        gfx_line(x,y+pod_y,x-pod_x,y,C_PURPLE);
        gfx_diamond(x, y, pulse_x, pulse_y, C_BLOOD);
        gfx_line(x - rx / 2, y, x - 2, y - 2, C_PURPLE);
        gfx_line(x + rx / 2, y, x + 2, y + 2, C_PURPLE);
        gfx_pixel(x, y - ry / 2, C_GREEN);
        gfx_pixel(x, y + ry / 2, C_GREEN_DARK);
    } else {
        int pulse_x = rx - phase;
        int pulse_y = ry - phase;
        if (pulse_x < 4) pulse_x = 4;
        if (pulse_y < 4) pulse_y = 4;
        gfx_frame(x - rx, y - ry, 2 * rx + 1, 2 * ry + 1, C_PROTOSS);
        gfx_frame(x - pulse_x, y - pulse_y,
                  2 * pulse_x + 1, 2 * pulse_y + 1, C_CYAN);
        gfx_line(x - 3, y, x + 3, y, C_WHITE);
        gfx_line(x, y - 3, x, y + 3, C_GOLD);
        gfx_pixel(x - rx / 2, y, C_CYAN);
        gfx_pixel(x + rx / 2, y, C_CYAN);
    }
    return 1;
}

#endif
