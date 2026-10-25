/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <string.h>
static int vfx_test_visible(int x,int y);
#define SC64_WEAPON_VFX_VISIBLE(x,y) vfx_test_visible((x),(y))
#include "../starcraft/sc64_weapon_vfx_render.h"

static unsigned drawn_pixels;
static int pixel_seen[240][128];
static int pixel_color_seen[240][128][32];
static int visibility_enabled=1;

static int vfx_test_visible(int x,int y)
{
    (void)x; (void)y;
    return visibility_enabled;
}

void gfx_pixel(int x, int y, u8 color)
{
    (void)color;
    assert(x >= SC64_WEAPON_VFX_VIEW_LEFT && x <= SC64_WEAPON_VFX_VIEW_RIGHT);
    assert(y >= SC64_WEAPON_VFX_VIEW_TOP && y <= SC64_WEAPON_VFX_VIEW_BOTTOM);
    ++drawn_pixels;
    pixel_seen[x][y] = 1;
    if(color<32)pixel_color_seen[x][y][color]=1;
}

static void
reset_draw(void)
{
    drawn_pixels=0;
    memset(pixel_seen,0,sizeof(pixel_seen));
    memset(pixel_color_seen,0,sizeof(pixel_color_seen));
}

static int
has_pixel_color(int x,int y,int color)
{
    return x>=0&&x<240&&y>=0&&y<128&&color>=0&&color<32&&
           pixel_color_seen[x][y][color];
}

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
test_styles_and_bounded_lifetimes(void)
{
    static const int styles[]={SC64_WEAPON_VFX_STYLE_NEEDLE,
        SC64_WEAPON_VFX_STYLE_MUZZLE,SC64_WEAPON_VFX_STYLE_MISSILE,
        SC64_WEAPON_VFX_STYLE_PLASMA,SC64_WEAPON_VFX_STYLE_FLAME,
        SC64_WEAPON_VFX_STYLE_SLASH,SC64_WEAPON_VFX_STYLE_ACID,
        SC64_WEAPON_VFX_STYLE_SHELL,SC64_WEAPON_VFX_STYLE_ARC,
        SC64_WEAPON_VFX_STYLE_BURST,SC64_WEAPON_VFX_STYLE_SPINES,
        SC64_WEAPON_VFX_STYLE_BEAM};
    for(unsigned i=0;i<sizeof(styles)/sizeof(styles[0]);++i) {
        int life=sc64_weapon_vfx_lifetime(styles[i]);
        assert(life>=1 && life<=12);
        reset_draw();
        sc64_weapon_vfx_draw(styles[i],C_GOLD,24,48,190,96,life);
        assert(drawn_pixels>0);
    }
    assert(SC64_WEAPON_VFX_STYLE_NONE==0);
    assert(SC64_WEAPON_VFX_STYLE_NEEDLE==1);
    assert(SC64_WEAPON_VFX_STYLE_MUZZLE==2);
    assert(SC64_WEAPON_VFX_STYLE_BEAM==12);
    assert(sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_NONE)==0);
    assert(sc64_weapon_vfx_lifetime(99)==0);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NONE,C_WHITE,20,30,40,50,1);
    sc64_weapon_vfx_draw(99,C_WHITE,20,30,40,50,1);
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_WHITE,20,30,40,50,0);
    assert(drawn_pixels==0);
}

static void
test_moving_head_and_impact_timing(void)
{
    int life=sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_NEEDLE);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,20,40,100,40,life);
    assert(pixel_seen[20][40]); /* first visible frame starts at the muzzle */
    assert(!pixel_seen[100][40]);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,20,40,100,40,3);
    assert(pixel_seen[60][40]);
    assert(!pixel_seen[100][40]);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,20,40,100,40,2);
    assert(pixel_seen[100][40]); /* impact begins in the final two life ticks */
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,20,40,100,40,1);
    assert(pixel_seen[100][40]);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,20,40,100,40,0);
    assert(drawn_pixels==0);
}

static void
test_viewport_clipping_and_extreme_coordinates(void)
{
    static const int styles[]={SC64_WEAPON_VFX_STYLE_NEEDLE,
        SC64_WEAPON_VFX_STYLE_MUZZLE,SC64_WEAPON_VFX_STYLE_MISSILE,
        SC64_WEAPON_VFX_STYLE_PLASMA,SC64_WEAPON_VFX_STYLE_FLAME,
        SC64_WEAPON_VFX_STYLE_SLASH,SC64_WEAPON_VFX_STYLE_ACID,
        SC64_WEAPON_VFX_STYLE_SHELL,SC64_WEAPON_VFX_STYLE_ARC,
        SC64_WEAPON_VFX_STYLE_BURST,SC64_WEAPON_VFX_STYLE_SPINES,
        SC64_WEAPON_VFX_STYLE_BEAM};
    for(unsigned i=0;i<sizeof(styles)/sizeof(styles[0]);++i) {
        int life=sc64_weapon_vfx_lifetime(styles[i]);
        reset_draw();
        sc64_weapon_vfx_draw(styles[i],C_CYAN,-20,-40,270,180,life);
        reset_draw();
        sc64_weapon_vfx_draw(styles[i],C_CYAN,INT_MIN,INT_MAX,
                             INT_MAX,INT_MIN,life);
    }
    /* Local effects wholly above or below the gameplay view are clipped. */
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_FLAME,C_ORANGE,50,0,80,0,6);
    assert(drawn_pixels==0);
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_BURST,C_GOLD,50,150,80,150,6);
    assert(drawn_pixels==0);
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_BURST,C_GOLD,50,120,80,120,6);
    assert(drawn_pixels>0);
    for(int y=120;y<128;++y) for(int x=0;x<240;++x)
        assert(!pixel_seen[x][y]); /* Row 120 belongs to the command HUD. */
}

static void
test_optional_visibility_gate(void)
{
    reset_draw();
    visibility_enabled=0;
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,
                         30,50,60,50,4);
    assert(drawn_pixels==0);
    visibility_enabled=1;
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_NEEDLE,C_GOLD,
                         30,50,60,50,4);
    assert(drawn_pixels>0);
}

static void
test_projectile_silhouettes(void)
{
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_PLASMA,C_GOLD,
                         80,60,180,60,sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_PLASMA));
    assert(has_pixel_color(80,60,C_WHITE));
    assert(has_pixel_color(79,60,C_CYAN) && has_pixel_color(81,60,C_CYAN));
    assert(has_pixel_color(80,59,C_CYAN) && has_pixel_color(80,61,C_CYAN));
    assert(!pixel_seen[75][60] && !pixel_seen[76][60] && !pixel_seen[77][60]);

    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_ACID,C_GOLD,
                         80,60,180,60,sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_ACID));
    assert(has_pixel_color(80,60,C_GREEN));
    assert(has_pixel_color(80,59,C_GREEN) && has_pixel_color(80,61,C_GREEN));
    assert(has_pixel_color(77,60,C_GREEN_DARK)); /* separate trailing droplet */
    assert(!has_pixel_color(80,60,C_CYAN));

    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_SHELL,C_GOLD,
                         80,60,180,60,sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_SHELL));
    assert(has_pixel_color(80,60,C_WHITE));
    assert(has_pixel_color(79,60,C_ORANGE) && has_pixel_color(81,60,C_ORANGE));
    assert(has_pixel_color(78,60,C_RED));
    assert(!has_pixel_color(79,60,C_CYAN));

    /* Missile retains its white head and warm flame trail silhouette. */
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_MISSILE,C_GOLD,
                         80,60,180,60,sc64_weapon_vfx_lifetime(SC64_WEAPON_VFX_STYLE_MISSILE));
    assert(has_pixel_color(80,60,C_WHITE));
    assert(has_pixel_color(80,59,C_ORANGE));
    assert(has_pixel_color(79,61,C_RED));
}

static void
test_beam_vertical_orientation(void)
{
    reset_draw();
    sc64_weapon_vfx_draw(SC64_WEAPON_VFX_STYLE_BEAM,C_GOLD,
                         100,20,100,100,4);
    /* At life 4 the interpolated point is x=100,y=73 and the short beam
     * follows the vertical shot axis, with no horizontal wing. */
    assert(pixel_seen[100][73]);
    assert(has_pixel_color(100,69,C_GOLD));
    assert(has_pixel_color(100,77,C_GOLD));
    assert(!pixel_seen[99][73] && !pixel_seen[101][73]);
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
    test_styles_and_bounded_lifetimes();
    test_moving_head_and_impact_timing();
    test_viewport_clipping_and_extreme_coordinates();
    test_optional_visibility_gate();
    test_projectile_silhouettes();
    test_beam_vertical_orientation();
    test_horizontal_and_lifetime_clamp();
    test_vertical_diagonal_and_reversed_shots();
    test_degenerate_and_map_edge_endpoints();
    test_all_lifetimes_keep_short_streak_inside_endpoints();
    test_invalid_output_pointer();
    return 0;
}
