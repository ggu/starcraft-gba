#ifndef SC64_WEAPON_VFX_H
#define SC64_WEAPON_VFX_H

#include <stdint.h>

/* Temporary, authored projectile styling. This tags the existing transient
 * tracer without changing its layout or the instant-damage combat model. The
 * low byte remains the ordinary gfx color; the upper byte is visual metadata
 * consumed by the renderer only. */
#define SC64_WEAPON_VFX_STYLE_SHIFT 8
#define SC64_WEAPON_VFX_COLOR_MASK 0xff
#define SC64_WEAPON_VFX_STYLE_MASK 0xff00

enum {
    SC64_WEAPON_VFX_STYLE_NONE = 0,
    SC64_WEAPON_VFX_STYLE_NEEDLE = 1
};

typedef struct {
    int tail_x, tail_y;
    int head_x, head_y;
} Sc64WeaponVfxSegment;

static inline int
sc64_weapon_vfx_pack(int style, int color)
{
    return ((style & 0xff) << SC64_WEAPON_VFX_STYLE_SHIFT) |
           (color & SC64_WEAPON_VFX_COLOR_MASK);
}

static inline int
sc64_weapon_vfx_style(int packed_color)
{
    return (packed_color & SC64_WEAPON_VFX_STYLE_MASK) >>
           SC64_WEAPON_VFX_STYLE_SHIFT;
}

static inline int
sc64_weapon_vfx_color(int packed_color)
{
    return packed_color & SC64_WEAPON_VFX_COLOR_MASK;
}

/* life counts down from four on the existing Tracer. The authored needle
 * travels linearly along the already-computed shot endpoints, drawing only
 * the most recent quarter of its path. This is intentionally not a claim of
 * native Needle Spines timing or IScript frames. */
static inline int
sc64_weapon_vfx_segment(int x0, int y0, int x1, int y1, int life,
                        Sc64WeaponVfxSegment *out)
{
    int age;
    int tail_phase;
    int64_t dx, dy, segment_dx, segment_dy, segment_major;

    if (!out) return 0;
    if (life < 1) life = 1;
    if (life > 4) life = 4;
    age = 5 - life;
    tail_phase = age - 1;
    dx = (int64_t)x1 - (int64_t)x0;
    dy = (int64_t)y1 - (int64_t)y0;

    /* Use wide products: even the difference between INT_MIN and INT_MAX
     * times the four-frame phase remains representable in int64_t. */
    out->tail_x = (int)((int64_t)x0 + dx * tail_phase / 4);
    out->tail_y = (int)((int64_t)y0 + dy * tail_phase / 4);
    out->head_x = (int)((int64_t)x0 + dx * age / 4);
    out->head_y = (int)((int64_t)y0 + dy * age / 4);

    /* Keep the bright head's short streak compact even for long shots. Use
     * the quantized points, so the cap remains exact on shallow/diagonal
     * paths and the tail moves only toward the already-bounded head. */
    segment_dx = (int64_t)out->head_x - out->tail_x;
    segment_dy = (int64_t)out->head_y - out->tail_y;
    segment_major = segment_dx < 0 ? -segment_dx : segment_dx;
    if ((segment_dy < 0 ? -segment_dy : segment_dy) > segment_major)
        segment_major = segment_dy < 0 ? -segment_dy : segment_dy;
    if (segment_major > 5) {
        int64_t short_dx = segment_dx * 5 / segment_major;
        int64_t short_dy = segment_dy * 5 / segment_major;
        out->tail_x = (int)((int64_t)out->head_x - short_dx);
        out->tail_y = (int)((int64_t)out->head_y - short_dy);
    }
    return 1;
}

#endif
