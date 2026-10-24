/* Differential checks for the optional P6 rescue candidate list. The oracle
 * below is the original nested full scan, kept independent of the grouped
 * implementation under test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_P6_RESCUE_GROUPED
#error "Compile this fixture with SC64_P6_RESCUE_GROUPED"
#endif

typedef struct {
    u8 alive;
    int owner, x, y, target, tx, ty;
    u8 selected, order;
} RescueOracleUnit;
typedef struct { u8 alive; int owner, x, y; } RescueOracleBuilding;
int sc64_p6_rescue_fullscan_oracle(RescueOracleUnit *units, unsigned unit_count,
                                    RescueOracleBuilding *buildings,
                                    unsigned building_count);

static u32 rng_state = 0x8d31a5c7u;
static u32 next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void launch(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_P6_GLOBAL_ID;
    race_campaign_init();
    assert(screen == SCREEN_PLAY && race_p6_runtime_state_valid());
}

static void clear_world(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    for (unsigned i=0;i<MAX_UNITS;++i) {
        u[i].target=-1;
        u[i].raw_owner=-1;
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) b[i].raw_owner=-1;
}

static void add_unit(unsigned slot, int owner, int x, int y)
{
    assert(slot<MAX_UNITS);
    u[slot].alive=1; u[slot].owner=owner; u[slot].raw_owner=(s8)(owner==SIDE?7:2);
    u[slot].raw_type=65; u[slot].x=x; u[slot].y=y;
    u[slot].selected=1; u[slot].target=slot+1; u[slot].order=ORDER_MOVE;
    u[slot].tx=x+17; u[slot].ty=y-9;
}

static void add_building(unsigned slot, int owner, int x, int y)
{
    assert(slot<MAX_BUILDINGS);
    b[slot].alive=1; b[slot].owner=owner; b[slot].raw_owner=(s8)(owner==SIDE?7:2);
    b[slot].raw_type=154; b[slot].x=x; b[slot].y=y;
}

static void run_pair(void)
{
    static RescueOracleUnit reference_u[MAX_UNITS];
    static RescueOracleBuilding reference_b[MAX_BUILDINGS];
    static Unit input_u[MAX_UNITS], expected_u[MAX_UNITS];
    static Building input_b[MAX_BUILDINGS], expected_b[MAX_BUILDINGS];
    int expected_changed, expected_lists, expected_buckets, expected_index;
    memcpy(input_u,u,sizeof(u)); memcpy(input_b,b,sizeof(b));
    for (unsigned i=0;i<MAX_UNITS;++i) {
        reference_u[i].alive=u[i].alive; reference_u[i].owner=u[i].owner;
        reference_u[i].x=u[i].x; reference_u[i].y=u[i].y;
        reference_u[i].selected=u[i].selected; reference_u[i].target=u[i].target;
        reference_u[i].order=u[i].order; reference_u[i].tx=u[i].tx; reference_u[i].ty=u[i].ty;
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        reference_b[i].alive=b[i].alive; reference_b[i].owner=b[i].owner;
        reference_b[i].x=b[i].x; reference_b[i].y=b[i].y;
    }
    expected_changed=sc64_p6_rescue_fullscan_oracle(reference_u,MAX_UNITS,
                                                      reference_b,MAX_BUILDINGS);
    memcpy(expected_u,input_u,sizeof(u)); memcpy(expected_b,input_b,sizeof(b));
    for (unsigned i=0;i<MAX_UNITS;++i) if (input_u[i].owner!=reference_u[i].owner) {
        expected_u[i].owner=SIDE; expected_u[i].selected=0; expected_u[i].target=-1;
        expected_u[i].order=ORDER_IDLE; expected_u[i].tx=input_u[i].x;
        expected_u[i].ty=input_u[i].y;
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        if (input_b[i].owner!=reference_b[i].owner) expected_b[i].owner=SIDE;
    target_lists_valid=1; sc64_unit_bucket_active=1; sc64_target_index_active=1;
    race_p6_rescue_tick();
    assert(memcmp(u,expected_u,sizeof(u))==0);
    assert(memcmp(b,expected_b,sizeof(b))==0);
    for (unsigned i=0;i<MAX_UNITS;++i) {
        assert(u[i].alive==reference_u[i].alive && u[i].owner==reference_u[i].owner);
        assert(u[i].x==reference_u[i].x && u[i].y==reference_u[i].y);
        assert(u[i].selected==reference_u[i].selected && u[i].target==reference_u[i].target);
        assert(u[i].order==reference_u[i].order && u[i].tx==reference_u[i].tx && u[i].ty==reference_u[i].ty);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        assert(b[i].alive==reference_b[i].alive && b[i].owner==reference_b[i].owner);
        assert(b[i].x==reference_b[i].x && b[i].y==reference_b[i].y);
    }
    expected_lists=expected_changed?0:1;
    expected_buckets=expected_changed?0:1;
    expected_index=expected_changed?0:1;
    assert(target_lists_valid==expected_lists);
    assert(sc64_unit_bucket_active==expected_buckets);
    assert(sc64_target_index_active==expected_index);
}

static void test_chains_and_radius(void)
{
    clear_world(); add_unit(0,SIDE,100,100);
    add_unit(1,RESCUABLE,132,100); add_unit(2,RESCUABLE,164,100);
    add_unit(3,RESCUABLE,132,133); /* diagonal squared distance 2113 > 1024 */
    add_unit(4,RESCUABLE,132,132); /* exact 32,32 diagonal is outside */
    add_unit(5,RESCUABLE,132,101); /* radius 32 inclusive */
    add_building(0,RESCUABLE,196,100); /* follows the last chained unit at x164 */
    run_pair();
    assert(u[1].owner==SIDE && u[2].owner==SIDE && u[5].owner==SIDE);
    assert(u[3].owner==RESCUABLE && u[4].owner==SIDE); /* no backward revisit */
    assert(b[0].owner==SIDE); /* building pass sees the complete unit pass */

    clear_world(); add_unit(0,SIDE,100,100);
    add_unit(1,RESCUABLE,132,132); /* diagonal is outside Euclidean radius */
    add_unit(2,RESCUABLE,132,100); /* exact 32-pixel radius */
    add_unit(3,RESCUABLE,165,100); /* 33 pixels beyond the chained rescue */
    run_pair();
    assert(u[1].owner==RESCUABLE && u[2].owner==SIDE && u[3].owner==RESCUABLE);

    clear_world(); add_unit(0,RESCUABLE,132,100);
    add_unit(1,RESCUABLE,164,100); add_unit(2,SIDE,100,100);
    run_pair();
    assert(u[0].owner==SIDE && u[1].owner==SIDE); /* descending chain */

    clear_world(); add_unit(0,RESCUABLE,500,500);
    add_unit(1,RESCUABLE,532,500); add_unit(2,SIDE,564,500);
    run_pair();
    assert(u[0].owner==RESCUABLE && u[1].owner==SIDE); /* no backward revisit */

    clear_world(); add_unit(10,SIDE,200,200);
    add_unit(11,RESCUABLE,232,200); add_building(2,RESCUABLE,264,200);
    run_pair();
    assert(u[11].owner==SIDE && b[2].owner==SIDE); /* buildings scan after units */
}

static void test_random_real_slots(void)
{
    for (unsigned trial=0;trial<1200;++trial) {
        clear_world();
        unsigned occupancy=next_random()%241u;
        for (unsigned i=0;i<occupancy;++i) {
            u[i].alive=(u8)(next_random()%5u!=0);
            u[i].owner=(int)(next_random()%5u);
            /* Alternate dense rescue chains with worlds retaining enemy,
               neutral and additional-owner rows that must stay untouched. */
            if (!(trial&1u) &&
                (u[i].owner==1 || u[i].owner==2 || u[i].owner==4))
                u[i].owner=RESCUABLE;
            u[i].raw_owner=(s8)(next_random()%8u);
            u[i].raw_type=(s16)(next_random()%227u);
            u[i].x=(int)(next_random()%2049u)-16;
            u[i].y=(int)(next_random()%2049u)-16;
            u[i].selected=(u8)(next_random()&1u);
            u[i].target=(int)(next_random()%MAX_UNITS)-1;
            u[i].order=(u8)(next_random()%(ORDER_GAS+1));
            u[i].tx=(int)(next_random()%2049u)-16;
            u[i].ty=(int)(next_random()%2049u)-16;
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) if ((next_random()&15u)==0) {
            b[i].alive=1; b[i].owner=(next_random()&1u)?RESCUABLE:SIDE;
            b[i].raw_owner=(s8)(next_random()%8u);
            b[i].raw_type=(s16)(next_random()%227u);
            b[i].x=(int)(next_random()%2049u)-16;
            b[i].y=(int)(next_random()%2049u)-16;
        }
        run_pair();
    }
}

static void test_full_capacity_and_guards(void)
{
    clear_world();
    add_unit(0,RESCUABLE,20,30);
    run_pair();
    assert(u[0].owner==RESCUABLE); /* empty SIDE list */

    clear_world();
    add_unit(0,SIDE,20,30);
    run_pair();
    assert(u[0].owner==SIDE); /* populated SIDE, no rescue candidates */

    clear_world();
    for (unsigned i=0;i<MAX_UNITS;++i)
        add_unit(i,(i&1u)?RESCUABLE:SIDE,(int)(i%8u)*4,(int)(i/8u)*4);
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        add_building(i,RESCUABLE,(int)(i%8u)*4,(int)(i/8u)*4);
    run_pair();
    for (unsigned i=0;i<MAX_UNITS;++i) assert(u[i].owner==SIDE);
    for (unsigned i=0;i<MAX_BUILDINGS;++i) assert(b[i].owner==SIDE);
    run_pair(); /* Full SIDE list, with no remaining rescue candidates. */

    clear_world(); add_unit(0,SIDE,20,30); add_unit(1,RESCUABLE,21,30);
    screen=SCREEN_TITLE;
    Unit before[MAX_UNITS]; memcpy(before,u,sizeof(u));
    Building before_buildings[MAX_BUILDINGS]; memcpy(before_buildings,b,sizeof(b));
    target_lists_valid=1; sc64_unit_bucket_active=1; sc64_target_index_active=1;
    race_p6_rescue_tick();
    assert(memcmp(before,u,sizeof(u))==0 && u[1].owner==RESCUABLE);
    assert(memcmp(before_buildings,b,sizeof(b))==0);
    assert(target_lists_valid==1 && sc64_unit_bucket_active==1 && sc64_target_index_active==1);
    screen=SCREEN_PLAY;
    race_p6.reserved[0]=1; /* invalid persistent state must be no-op */
    memcpy(before,u,sizeof(u));
    memcpy(before_buildings,b,sizeof(b));
    race_p6_rescue_tick();
    assert(memcmp(before,u,sizeof(u))==0 && u[1].owner==RESCUABLE);
    assert(memcmp(before_buildings,b,sizeof(b))==0);
    assert(target_lists_valid==1 && sc64_unit_bucket_active==1 && sc64_target_index_active==1);
    race_p6.reserved[0]=0;
    race_selected_mission=-1;
    memcpy(before,u,sizeof(u));
    memcpy(before_buildings,b,sizeof(b));
    race_p6_rescue_tick();
    assert(memcmp(before,u,sizeof(u))==0 && u[1].owner==RESCUABLE);
    assert(memcmp(before_buildings,b,sizeof(b))==0);
    assert(target_lists_valid==1 && sc64_unit_bucket_active==1 && sc64_target_index_active==1);
}

int main(void)
{
    launch();
    test_chains_and_radius();
    test_random_real_slots();
    test_full_capacity_and_guards();
    puts("P6 grouped rescue matches nested full-scan oracle");
    return 0;
}
