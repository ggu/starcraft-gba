#include <assert.h>
#include <limits.h>
#include "../starcraft/sc64_weapon_vfx.h"

static void
test_tag_preserves_base_color(void)
{
    int packed = sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_NEEDLE, 11);
    assert(sc64_weapon_vfx_style(packed) == SC64_WEAPON_VFX_STYLE_NEEDLE);
    assert(sc64_weapon_vfx_color(packed) == 11);
    assert(sc64_weapon_vfx_style(11) == SC64_WEAPON_VFX_STYLE_NONE);
    assert(sc64_weapon_vfx_color(11) == 11);
}

static void
test_horizontal_and_lifetime_clamp(void)
{
    Sc64WeaponVfxSegment s;
    assert(sc64_weapon_vfx_segment(0, 24, 100, 24, 4, &s));
    assert(s.tail_x == 20 && s.head_x == 25);
    assert(s.tail_y == 24 && s.head_y == 24);
    assert(sc64_weapon_vfx_segment(0, 24, 100, 24, 3, &s));
    assert(s.tail_x == 45 && s.head_x == 50);
    assert(sc64_weapon_vfx_segment(0, 24, 100, 24, 1, &s));
    assert(s.tail_x == 95 && s.head_x == 100);
    assert(sc64_weapon_vfx_segment(0, 24, 100, 24, 99, &s));
    assert(s.tail_x == 20 && s.head_x == 25);
    assert(sc64_weapon_vfx_segment(0, 24, 100, 24, -8, &s));
    assert(s.tail_x == 95 && s.head_x == 100);
}

static void
test_vertical_diagonal_and_reversed_shots(void)
{
    Sc64WeaponVfxSegment s;
    assert(sc64_weapon_vfx_segment(80, 0, 80, 64, 2, &s));
    assert(s.tail_x == 80 && s.head_x == 80);
    assert(s.tail_y == 43 && s.head_y == 48);

    assert(sc64_weapon_vfx_segment(0, 0, 40, 28, 3, &s));
    assert(s.tail_x == 15 && s.tail_y == 11);
    assert(s.head_x == 20 && s.head_y == 14);

    assert(sc64_weapon_vfx_segment(40, 28, 0, 0, 3, &s));
    assert(s.tail_x == 25 && s.tail_y == 17);
    assert(s.head_x == 20 && s.head_y == 14);
}

static void
test_degenerate_and_map_edge_endpoints(void)
{
    Sc64WeaponVfxSegment s;
    assert(sc64_weapon_vfx_segment(0, 0, 0, 0, 4, &s));
    assert(s.tail_x == 0 && s.tail_y == 0);
    assert(s.head_x == 0 && s.head_y == 0);

    /* Full-size 128x128 race-map world boundaries. Every point stays between
     * the original endpoints, including at the final life tick. */
    assert(sc64_weapon_vfx_segment(0, 0, 2048, 2048, 1, &s));
    assert(s.tail_x == 2043 && s.tail_y == 2043);
    assert(s.head_x == 2048 && s.head_y == 2048);
    assert(s.tail_x <= s.head_x && s.tail_y <= s.head_y);

    /* Extreme integer endpoints exercise the wide interpolation products;
     * the result still lands on the original endpoints at phase four. */
    assert(sc64_weapon_vfx_segment(INT_MIN, INT_MAX, INT_MAX, INT_MIN, 1, &s));
    assert(s.head_x == INT_MAX && s.head_y == INT_MIN);
    assert(s.tail_x == INT_MAX - 5 && s.tail_y == INT_MIN + 5);
}

static int64_t
abs64(int64_t x)
{
    return x < 0 ? -x : x;
}

static void
assert_segment_bounded(const Sc64WeaponVfxSegment *s,
                       int x0, int y0, int x1, int y1)
{
    int min_x = x0 < x1 ? x0 : x1, max_x = x0 > x1 ? x0 : x1;
    int min_y = y0 < y1 ? y0 : y1, max_y = y0 > y1 ? y0 : y1;
    int64_t dx = (int64_t)s->head_x - s->tail_x;
    int64_t dy = (int64_t)s->head_y - s->tail_y;
    int64_t major = abs64(dx) > abs64(dy) ? abs64(dx) : abs64(dy);
    assert(s->tail_x >= min_x && s->tail_x <= max_x);
    assert(s->head_x >= min_x && s->head_x <= max_x);
    assert(s->tail_y >= min_y && s->tail_y <= max_y);
    assert(s->head_y >= min_y && s->head_y <= max_y);
    assert(major <= 5);
}

static void
test_all_lifetimes_keep_short_streak_inside_endpoints(void)
{
    static const int points[][4] = {
        {0, 0, 640, 224}, {2048, 2048, 0, 0},
        {0, 1024, 1, 0}, {2047, 0, 0, 2047},
        {-100, 40, 100, -40}
    };
    Sc64WeaponVfxSegment s;
    unsigned i;
    int life;
    for (i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
        for (life = 1; life <= 4; ++life) {
            assert(sc64_weapon_vfx_segment(points[i][0], points[i][1],
                                           points[i][2], points[i][3],
                                           life, &s));
            assert_segment_bounded(&s, points[i][0], points[i][1],
                                   points[i][2], points[i][3]);
        }
    }
}

static void
test_invalid_output_pointer(void)
{
    assert(!sc64_weapon_vfx_segment(1, 2, 3, 4, 2, 0));
}

int
main(void)
{
    test_tag_preserves_base_color();
    test_horizontal_and_lifetime_clamp();
    test_vertical_diagonal_and_reversed_shots();
    test_degenerate_and_map_edge_endpoints();
    test_all_lifetimes_keep_short_streak_inside_endpoints();
    test_invalid_output_pointer();
    return 0;
}
