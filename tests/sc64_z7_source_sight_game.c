/* Source-sight radius and visibility-cache checks against the actual Z7
 * engine arrays. This is a focused fog-of-war fixture, not a mission replay. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_Z7_SOURCE_SIGHT
#error "Compile this fixture with SC64_Z7_SOURCE_SIGHT"
#endif

static void clear_fixture(void)
{
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(explored,0,sizeof(explored));
    race_mode=0;
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    sc64_visibility_reset();
}

static void put_unit(int slot,int raw,int owner,int x,int y)
{
    Unit *q=&u[slot];
    memset(q,0,sizeof(*q));
    q->alive=1;
    q->owner=owner;
    q->raw_owner=owner==SIDE?3:4;
    q->raw_type=(s16)raw;
    q->type=sc64_race_unit_from_raw(raw);
    assert(q->type>=0);
    q->x=x;q->y=y;q->target=-1;q->order=ORDER_IDLE;
}

static void put_building(int slot,int raw,int owner,int x,int y)
{
    Building *q=&b[slot];
    memset(q,0,sizeof(*q));
    q->alive=1;
    q->owner=owner;
    q->raw_owner=owner==SIDE?3:4;
    q->raw_type=(s16)raw;
    q->type=sc64_race_building_from_raw(raw);
    assert(q->type>=0);
    q->x=x;q->y=y;
}

/* Full scan oracle: unlike the incremental source-count/span updater, this
 * recomputes every cell from the currently live sources each time. */
static void assert_visible_matches_full_scan(void)
{
    for(int row=0;row<MAP_ROWS;++row)for(int col=0;col<MAP_COLS;++col) {
        int x=col*16+8,y=row*16+8,expected=0;
        for(int i=0;i<MAX_UNITS&&!expected;++i)if(u[i].alive&&u[i].owner==SIDE&&
                !SC64_TRANSPORT_LOADED(&u[i])) {
            int radius=sc64_actor_unit_sight_pixels(u[i].type)/2;
            if(radius<=0)radius=IS_WORKER(u[i].type)?56:72;
            int dx=x-u[i].x,dy=y-u[i].y;
            expected=dx*dx+dy*dy<=radius*radius;
        }
        for(int i=0;i<MAX_BUILDINGS&&!expected;++i)if(b[i].alive&&b[i].owner==SIDE) {
            int sight_key=sc64_z7_building_visibility_key(&b[i]);
            int radius=sc64_actor_building_sight_pixels(sight_key)/2;
            if(radius<=0)
                radius=(b[i].type==B_COMMAND||sc64_race_is_zerg_townhall(b[i].type)||
                        b[i].type==B_NEXUS)?112:64;
            int dx=x-b[i].x,dy=y-b[i].y;
            expected=dx*dx+dy*dy<=radius*radius;
        }
        assert(SC64_GRID_GET(visible,col+row*MAP_COLS)==expected);
    }
}

static void test_authenticated_radius_table(void)
{
    static const struct {int raw,pixels;} units[]={
        {37,160},{38,192},{39,224},{41,224},{42,288},{43,224},
        {44,352},{45,320},{46,320},{47,160},{50,160},{51,288},
        {59,128},{62,320},{97,128},{103,256}
    };
    static const struct {int raw,pixels;} buildings[]={
        {130,320},{131,288},{132,320},{133,352},{134,256},{135,256},
        {136,256},{137,256},{138,256},{139,256},{140,256},{141,256},
        {142,256},{143,320},{144,320},{146,320},{149,224}
    };
    assert(race_z7_active());
    for(unsigned i=0;i<sizeof(units)/sizeof(units[0]);++i)
        assert(sc64_z7_source_unit_sight_raw(units[i].raw)==units[i].pixels);
    for(unsigned i=0;i<sizeof(buildings)/sizeof(buildings[0]);++i)
        assert(sc64_z7_source_unit_sight_raw(buildings[i].raw)==buildings[i].pixels);
    assert(sc64_z7_source_unit_sight_pixels(U_MUTALISK)==224);
    assert(sc64_z7_source_unit_sight_pixels(U_DRONE)==224);
    assert(sc64_z7_source_unit_sight_pixels(U_HYDRALISK)==192);
    assert(sc64_z7_source_unit_sight_pixels(U_OVERLORD)==288);
    assert(sc64_z7_source_unit_sight_pixels(U_Z6_GUARDIAN)==352);
    assert(sc64_z7_source_building_sight_pixels(B_HATCHERY)==288);
    assert(sc64_z7_source_building_sight_pixels(B_SPORE_COLONY)==320);

    race_selected_mission=SC64_Z6_GLOBAL_ID;
    assert(!race_z7_active());
    assert(sc64_z7_source_unit_sight_pixels(U_MUTALISK)==0);
    assert(sc64_z7_source_building_sight_pixels(B_HATCHERY)==0);
}

static void test_cached_spans_match_moving_full_scan(void)
{
    clear_fixture();
    test_authenticated_radius_table();
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    put_unit(0,43,SIDE,168,168);
    put_unit(1,38,SIDE,316,296);
    put_unit(2,42,SIDE,8,8); /* edge clipping */
    put_building(0,131,SIDE,704,672);
    put_building(1,144,SIDE,1056,1056);
    b[0].under_construction=1;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==-(B_HATCHERY+1));
    assert(sc64_actor_building_sight_pixels(sc64_z7_building_visibility_key(&b[0]))==128);
    assert_visible_matches_full_scan();

    /* Subtile movement, tile crossings and multiple sources changing at once. */
    u[0].x=171;u[0].y=177;
    u[1].x=340;u[1].y=312;
    b[0].x=720;b[0].y=681;
    sc64_update_exploration();
    assert_visible_matches_full_scan();

    /* Completion at a fixed position must replace the four-tile unfinished
       footprint with the building's pinned nine-tile sight range. */
    b[0].under_construction=0;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert(sc64_actor_building_sight_pixels(B_HATCHERY)==288);
    assert_visible_matches_full_scan();

    /* The engine adapter switches to the target enum immediately, while BW
       retains the source enum through the morph. Keep the Hatchery's nine
       tile radius until the Lair actually completes. */
    b[0].type=B_LAIR;b[0].raw_type=132;b[0].under_construction=1;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert(sc64_actor_building_sight_pixels(B_HATCHERY)==288);
    assert_visible_matches_full_scan();
    b[0].under_construction=0;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_LAIR);
    assert(sc64_actor_building_sight_pixels(B_LAIR)==320);
    assert_visible_matches_full_scan();

#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    /* The Creep Colony keeps its native ten-tile vision during the authored
       in-place Sunken/Spore morph even though construction is in progress. */
    b[0].type=B_CREEP_COLONY;b[0].raw_type=143;
    b[0].flags=SC64_Z7_SPORE_MORPH_FLAG;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_CREEP_COLONY);
    assert(sc64_actor_building_sight_pixels(B_CREEP_COLONY)==320);
    assert_visible_matches_full_scan();
    b[0].flags=0;
#endif

    /* Transported units stop contributing; unloading restores their source. */
    u[0].order=ORDER_TRANSPORTED;
    sc64_update_exploration();
    assert_visible_matches_full_scan();
    u[0].order=ORDER_IDLE;
    sc64_update_exploration();
    assert_visible_matches_full_scan();

    /* Removal of both a unit source and a native-ranged building clears only
     * cells with no remaining SIDE source. */
    u[1].alive=0;
    b[1].alive=0;
    sc64_update_exploration();
    assert_visible_matches_full_scan();
}

static void test_runtime_lair_morph_uses_native_predecessor_radius(void)
{
    clear_fixture();
    race_human=R_ZERG;
    minerals=1000;gas=1000;
    put_building(0,131,SIDE,480,480);
    b[0].hp=b[0].maxhp=1250;
    put_building(1,142,SIDE,600,480); /* required Spawning Pool */
    b[1].hp=b[1].maxhp=750;
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert(race_start_lair_morph(0));
    assert(b[0].type==B_LAIR && b[0].raw_type==132 && b[0].under_construction);
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert(sc64_actor_building_sight_pixels(sc64_z7_building_visibility_key(&b[0]))==288);
    sc64_update_exploration();
    assert_visible_matches_full_scan();
    b[0].under_construction=0;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_LAIR);
    assert_visible_matches_full_scan();
}

int main(void)
{
    init_state();
    clear_fixture();
    test_cached_spans_match_moving_full_scan();
    test_runtime_lair_morph_uses_native_predecessor_radius();
    puts("Z7 source sight table and incremental visibility match full scan");
    return 0;
}
