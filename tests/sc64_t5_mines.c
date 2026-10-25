/* Asset-free fixture for the bounded T5 mine policy. These states are
 * constructed directly and are not controller/gameplay evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"
#include "../starcraft/sc64_weapon_vfx.h"

#define SC64_T5_RUNTIME 1
#define MAX_UNITS 8
#define WORLD_W 256
#define WORLD_H 256
#define SIDE 0
#define ENEMY 1
#define U_SPIDER_MINE 13
#define U_VULTURE 2
#define U_RAYNOR_VULTURE 19
#define U_MARINE 0
#define U_SCV 7
#define U_WRAITH 8
#define U_KERRIGAN 16
#define ORDER_IDLE 0

typedef struct {
    int x, y, hp, maxhp, owner, type, alive, selected, cd, order, tx, ty;
    int carry, build, work_timer, armor, weapon, target, path_timer, path_retry;
    int path_x, path_y, shield, raw_type, raw_owner, flags, shield_remainder;
} Unit;
typedef struct { int left, top, right, bottom; } Sc64T4QueryRect;

static Unit u[MAX_UNITS];
static int invincible_units[MAX_UNITS];
static int target_lists_valid = 1, target_end_count, bucket_end_count;
static int hit_count, hit_target_log[16], hit_damage_log[16];
static unsigned explosion_visuals;
static void add_tracer(int x0,int y0,int x1,int y1,int profile)
{
    assert(x0==x1 && y0==y1);
    assert(sc64_weapon_vfx_style(profile)==SC64_WEAPON_VFX_STYLE_BURST);
    ++explosion_visuals;
}

static int owners_hostile(int a, int b) { return a != b; }
static int sc64_race_unit_flying(int type) { return type == U_WRAITH; }
static int sc64_race_raw_unit(int type) { return type; }
static int sc64_t5_actor_bounds_source(int type, int *l, int *up,
                                        int *r, int *down)
{
    int a, b, c, d;
    switch (type) {
    case U_SPIDER_MINE: a=7; b=7; c=7; d=7; break;
    case U_RAYNOR_VULTURE: a=16; b=16; c=15; d=15; break;
    case U_KERRIGAN: a=7; b=10; c=7; d=11; break;
    default: return 0;
    }
    if (l) *l=a; if (up) *up=b; if (r) *r=c; if (down) *down=d;
    return 1;
}
static int sc64_t4_query_actor_rect(unsigned raw, int cx, int cy,
                                    Sc64T4QueryRect *r)
{
    int l=8, up=9, right=8, down=10;
    if (raw == 2) { l=16; up=16; right=15; down=15; }
    else if (raw == 7) { l=11; up=7; right=11; down=14; }
    else if (raw == 19) { l=16; up=16; right=15; down=15; }
    else if (raw == 16) { l=7; up=10; right=7; down=11; }
    else if (raw != 0) return 0;
    r->left=cx-l; r->top=cy-up; r->right=cx+right+1; r->bottom=cy+down+1;
    return 1;
}
static int direct_path_clear(int x, int y, int tx, int ty)
{ (void)x; (void)y; (void)tx; (void)ty; return 1; }
static int path_next_tile(int x, int y, int tx, int ty, int *nx, int *ny)
{ (void)x; (void)y; *nx=tx; *ny=ty; return 1; }
static int is_traversable(int col, int row, int from)
{ (void)from; return col >= 0 && row >= 0 && col < WORLD_W/16 && row < WORLD_H/16; }
static int unit_step_clear(const Unit *q, int x, int y)
{ (void)q; return x >= 0 && y >= 0 && x < WORLD_W && y < WORLD_H; }
static void sc64_unit_bucket_move(Unit *q) { (void)q; }
static void sc64_target_index_move(Unit *q) { (void)q; }
static void sc64_target_index_end(void) { ++target_end_count; }
static void sc64_unit_buckets_end(void) { ++bucket_end_count; }
static void hit_target(int target, int damage, int source_type, int owner)
{
    assert(source_type == U_SPIDER_MINE && owner == ENEMY);
    if (hit_count < (int)(sizeof(hit_target_log)/sizeof(hit_target_log[0]))) {
        hit_target_log[hit_count] = target;
        hit_damage_log[hit_count] = damage;
    }
    ++hit_count;
}

#include "../starcraft/sc64_t5_mine_adapter.h"

static void reset_fixture(void)
{
    memset(u, 0, sizeof(u)); memset(invincible_units, 0, sizeof(invincible_units));
    memset(hit_target_log, 0xff, sizeof(hit_target_log));
    memset(hit_damage_log, 0, sizeof(hit_damage_log));
    hit_count = target_end_count = bucket_end_count = 0;
    explosion_visuals=0;
    target_lists_valid = 1;
}

static Unit *make_mine(int slot, int x, int y)
{
    Unit *q = &u[slot];
    q->x=x; q->y=y; q->hp=q->maxhp=20; q->owner=ENEMY;
    q->type=U_SPIDER_MINE; q->alive=1; q->raw_type=13; q->raw_owner=6;
    q->target=-1;
    return q;
}

static Unit *make_actor(int slot, int type, int raw, int owner, int x, int y)
{
    Unit *q = &u[slot];
    q->x=x; q->y=y; q->hp=q->maxhp=100; q->owner=owner; q->type=type;
    q->raw_type=raw; q->raw_owner=owner; q->alive=1;
    return q;
}

static void test_arm_timer_and_target_filters(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 80, 80);
    make_actor(1, U_VULTURE, 2, SIDE, 80, 80); /* known hover identity */
    make_actor(2, U_WRAITH, 8, SIDE, 80, 80);  /* source flyer */
    make_actor(3, U_MARINE, 0, ENEMY, 80, 80); /* allied to mine */
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_ARMING);
    assert(mine->work_timer == SC64_T5_MINE_ARM_TICKS - 1);
    assert(mine->target == -1);
    for (int i=0; i<SC64_T5_MINE_ARM_TICKS-1; ++i) race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_ARMED);
    assert(mine->work_timer == 0);
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_ARMED); /* no hostile ground unit */
    assert(mine->target == -1);
    make_actor(4, U_MARINE, 0, SIDE, 80, 80);
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_CHASING && mine->target == 4);
}

static void test_square_acquisition_corners(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 80, 80);
    mine->carry=SC64_T5_MINE_ARMED;
    make_actor(1, U_MARINE, 0, SIDE, 128, 128); /* 96px in both axes */
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_CHASING && mine->target == 1);
}

static void test_tie_and_chase_range(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 100, 100);
    mine->carry=SC64_T5_MINE_ARMED;
    make_actor(2, U_MARINE, 0, SIDE, 120, 100);
    make_actor(1, U_MARINE, 0, SIDE, 80, 100);
    race_t5_mines_tick();
    assert(mine->target == 1); /* equal distance, lowest source slot */
    mine->carry=SC64_T5_MINE_CHASING;
    mine->target=1;
    u[1].x=400; u[1].y=100; /* beyond source 576 px pursuit cap */
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_ARMED && mine->target == -1);
}

static void test_bounded_pursuit_and_index_invalidation(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 100, 100);
    mine->carry=SC64_T5_MINE_ARMED;
    make_actor(1, U_MARINE, 0, SIDE, 145, 100); /* within source acquisition */
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_CHASING && mine->target == 1);
    race_t5_mines_tick();
    assert(mine->x == 106 && mine->y == 100);
    assert(target_end_count == 1 && bucket_end_count == 1 && !target_lists_valid);
}

static void test_contact_splash_and_friendly_fire(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 100, 100);
    mine->carry=SC64_T5_MINE_CHASING; mine->target=1;
    make_actor(1, U_MARINE, 0, SIDE, 100, 100); /* contact, full damage */
    make_actor(2, U_MARINE, 0, ENEMY, 136, 100); /* allied, middle band */
    make_actor(3, U_WRAITH, 8, SIDE, 100, 100); /* flying, not weapon-targetable */
    assert(sc64_t5_mine_splash_divisor_sq(50*50)==1);
    assert(sc64_t5_mine_splash_divisor_sq(51*51)==2);
    assert(sc64_t5_mine_splash_divisor_sq(75*75)==2);
    assert(sc64_t5_mine_splash_divisor_sq(76*76)==4);
    assert(sc64_t5_mine_splash_divisor_sq(100*100)==4);
    assert(sc64_t5_mine_splash_divisor_sq(101*101)==0);
    race_t5_mines_tick();
    assert(!mine->alive && hit_count == 2);
    assert(explosion_visuals==1);
    assert(hit_target_log[0] == 1 && hit_damage_log[0] == 125);
    assert(hit_target_log[1] == 2 && hit_damage_log[1] == 62);
    assert(target_end_count == 1 && bucket_end_count == 1 && !target_lists_valid);
}

static void test_invalidated_target_rearms_and_slot_owner_gate(void)
{
    reset_fixture();
    Unit *mine=make_mine(0, 100, 100);
    mine->carry=SC64_T5_MINE_CHASING; mine->target=1;
    Unit *target=make_actor(1, U_MARINE, 0, SIDE, 100, 100);
    target->alive=0;
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_ARMED && mine->target == -1);
    mine->carry=0; mine->work_timer=0; mine->raw_owner=5;
    race_t5_mines_tick();
    assert(mine->carry == 0 && mine->work_timer == 0);
}

int main(void)
{
    assert(sc64_t5_mine_source_hover_raw(2));
    assert(sc64_t5_mine_source_hover_raw(19));
    assert(!sc64_t5_mine_source_hover_raw(0));
    test_arm_timer_and_target_filters();
    test_square_acquisition_corners();
    test_tie_and_chase_range();
    test_bounded_pursuit_and_index_invalidation();
    test_contact_splash_and_friendly_fire();
    test_invalidated_target_rearms_and_slot_owner_gate();
    puts("T5 Spider Mine adapter fixtures passed (not gameplay proof)");
    return 0;
}
