/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exhaustive pure-math fixture for the optional ROM geometry kernel. This
 * compiles the real contact header with minimal context for unrelated game
 * paths; it does not test full game.c integration. The arithmetic fallback
 * under test is the implementation in sc64_contact.h, not a copied oracle. */
#include <assert.h>
#include <stdio.h>

typedef unsigned char u8;
typedef signed char s8;
typedef struct Unit {
    int type, raw_type, x, y, alive, order;
    unsigned flags;
} Unit;
typedef struct Sc64StockUnitProfile { unsigned top_speed_q10; } Sc64StockUnitProfile;

#define MAP_COLS 64
#define MAP_ROWS 64
#define WORLD_W 1024
#define WORLD_H 1024
#define U_CHRYSALIS 255
#define ORDER_MINE 1
#define ORDER_GAS 2
#define GBA_HOT
#define IS_WORKER(type) 0
#define SC64_TRANSPORT_LOADED(unitp) 0
#define SC64_GAME_ACTOR_COLLIDABLE 4u
#define SC64_GAME_ACTOR_MOVABLE 2u

/* These declarations only satisfy unrelated bucket/mover helpers compiled
 * from the real header. The tests below call its geometry helpers directly. */
static Unit u[256];
static u8 sc64_unit_bucket_head[MAP_COLS * MAP_ROWS];
static u8 sc64_unit_bucket_next[256];
static int sc64_unit_bucket_active;
static int sc64_race_unit_flying(int type) { (void)type; return 0; }
static unsigned sc64_game_unit_actor_flags(int type) { (void)type; return 0; }
static int sc64_game_unit_collision_radius(int type) { (void)type; return 6; }
static int unit_step_clear(const Unit *q, int x, int y);
static int sc64_unit_bucket_step_clear_reference(const Unit *q, int x, int y, int strict);
static const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw)
{ (void)raw; return 0; }

#include "../starcraft/sc64_contact.h"

static int unit_step_clear(const Unit *q, int x, int y)
{ (void)q; (void)x; (void)y; return 1; }
static int sc64_unit_bucket_step_clear_reference(const Unit *q, int x, int y, int strict)
{ (void)q; (void)x; (void)y; (void)strict; return 1; }

#ifndef SC64_CONTACT_MASK_TABLE
#error "Compile this fixture with SC64_CONTACT_MASK_TABLE"
#endif

static void
check_full_table(void)
{
    unsigned tuples = 0, pending_checks = 0;
    const unsigned masks[] = {0x00u, 0xffu, 0x55u, 0xaau, 0x01u, 0x80u};
    for (int radius = 6; radius <= 19; ++radius)
        for (int speed = 0; speed <= 14; ++speed)
            for (int oy = -32; oy <= 32; ++oy)
                for (int ox = -32; ox <= 32; ++ox) {
                    unsigned want = sc64_contact_mask_math_reference(
                        0, radius, speed, ox, oy);
                    unsigned got = sc64_contact_mask_table_lookup(
                        radius, speed, ox, oy);
                    assert(got == want);
                    ++tuples;
                    if (((unsigned)tuples & 1023u) == 0u) {
                        for (unsigned i = 0; i < sizeof(masks)/sizeof(masks[0]); ++i) {
                            unsigned seeded = masks[i];
                            assert(sc64_contact_mask_apply_geometry(
                                seeded, radius, speed, ox, oy) ==
                                sc64_contact_mask_math_reference(
                                    seeded, radius, speed, ox, oy));
                            ++pending_checks;
                        }
                    }
                }
    assert(tuples == 887250u);
    assert(pending_checks > 5000u);
}

static void
check_fallback_domain(void)
{
    const int radii[] = {0, 1, 5, 20, 30, 31};
    const int speeds[] = {-1, 15, 255, 1024};
    const int offsets[] = {-100, -33, 0, 33, 100};
    unsigned checks = 0;
    for (unsigned ri = 0; ri < sizeof(radii)/sizeof(radii[0]); ++ri)
        for (unsigned si = 0; si < sizeof(speeds)/sizeof(speeds[0]); ++si)
            for (unsigned yi = 0; yi < sizeof(offsets)/sizeof(offsets[0]); ++yi)
                for (unsigned xi = 0; xi < sizeof(offsets)/sizeof(offsets[0]); ++xi)
                    for (unsigned mask = 0; mask < 256u; ++mask) {
                        int radius = radii[ri], speed = speeds[si];
                        int ox = offsets[xi], oy = offsets[yi];
                        assert(!sc64_contact_mask_table_domain(radius,speed,ox,oy));
                        assert(sc64_contact_mask_apply_geometry(mask,radius,speed,ox,oy) ==
                               sc64_contact_mask_math_reference(mask,radius,speed,ox,oy));
                        ++checks;
                    }
    assert(checks == 6u * 4u * 5u * 5u * 256u);
}

int
main(void)
{
    check_full_table();
    check_fallback_domain();
    /* With zero stock speed every offset coincides; a deep overlap blocks
     * every direction, while a separated actor blocks none. */
    assert(sc64_contact_mask_apply_geometry(0,19,0,0,0) == 0xffu);
    assert(sc64_contact_mask_apply_geometry(0,19,0,20,0) == 0u);
    puts("contact mask math: exhaustive table and fallback domains PASS");
    return 0;
}
