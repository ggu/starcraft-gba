/* Source-sight radius and visibility-cache checks against the actual Z8
 * engine arrays. This is a focused fog-of-war fixture, not a mission replay. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#if !defined(SC64_Z7_SOURCE_SIGHT) || !defined(SC64_Z8_RUNTIME)
#error "Compile with canonical Z8 source-sight flags"
#endif

static void clear_fixture(void)
{
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(explored,0,sizeof(explored));
    race_mode=0;
    race_selected_mission=SC64_Z8_GLOBAL_ID;
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

static void test_z8_facts_and_visibility(void)
{
    clear_fixture();
    assert(race_z8_active() && !race_z7_active());
    assert(sc64_source_sight_raw_pixels(42)==288);
    assert(sc64_actor_unit_sight_pixels(U_OVERLORD)==288);
    assert(sc64_actor_unit_sight_pixels(U_DRONE)==224);
    assert(sc64_actor_unit_sight_pixels(U_ZP_INFESTED_KERRIGAN)==288);
    assert(sc64_actor_building_sight_pixels(B_HATCHERY)==288);
    assert(race_z8_overlord_detect_px()==sc64_source_sight_raw_pixels(42)/2);
    put_unit(0,42,SIDE,168,168);
    put_unit(1,38,SIDE,316,296);
    put_building(0,131,SIDE,704,672);
    put_building(1,144,SIDE,1056,1056);
    b[0].under_construction=1;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==-(B_HATCHERY+1));
    assert(sc64_actor_building_sight_pixels(-(B_HATCHERY+1))==128);
    assert_visible_matches_full_scan();
    u[0].x=187;u[0].y=177;u[1].x=340;b[0].x=720;
    sc64_update_exploration();assert_visible_matches_full_scan();
    b[0].under_construction=0;sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert_visible_matches_full_scan();
    b[0].type=B_LAIR;b[0].raw_type=132;b[0].under_construction=1;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_HATCHERY);
    assert_visible_matches_full_scan();
    b[0].under_construction=0;sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_LAIR);
    assert_visible_matches_full_scan();
    b[0].type=B_CREEP_COLONY;b[0].raw_type=143;
    b[0].under_construction=1;b[0].flags=SC64_Z7_SPORE_MORPH_FLAG;
    sc64_update_exploration();
    assert(sc64_z7_building_visibility_key(&b[0])==B_CREEP_COLONY);
    assert_visible_matches_full_scan();
    u[0].order=ORDER_TRANSPORTED;sc64_update_exploration();
    assert_visible_matches_full_scan();
    u[0].order=ORDER_IDLE;u[1].alive=0;b[1].alive=0;
    sc64_update_exploration();assert_visible_matches_full_scan();

    race_selected_mission=SC64_Z6_GLOBAL_ID;
    assert(!race_z8_active() && !race_z7_active());
    assert(sc64_actor_unit_sight_pixels(U_OVERLORD)==0);
    race_selected_mission=SC64_Z7_GLOBAL_ID;
    assert(sc64_actor_unit_sight_pixels(U_OVERLORD)==288);
    race_mode=1;
    assert(sc64_actor_unit_sight_pixels(U_OVERLORD)==0);
}

static void test_z8_detector_boundaries(void)
{
    clear_fixture();
    put_unit(0,42,SIDE,500,500);
    put_unit(1,74,ENEMY,500,500);
    int radius=race_z8_overlord_detect_px();
    u[1].x=(s16)(500+radius);race_z8_update_cloak();
    assert(!race_z8_unit_hidden_from_side(1));
    u[1].x=(s16)(501+radius);race_z8_update_cloak();
    assert(race_z8_unit_hidden_from_side(1));
    u[0].order=ORDER_TRANSPORTED;u[1].x=500;race_z8_update_cloak();
    assert(race_z8_unit_hidden_from_side(1));
    put_building(0,144,SIDE,500,500);b[0].under_construction=1;
    race_z8_update_cloak();assert(race_z8_unit_hidden_from_side(1));
    b[0].under_construction=0;race_z8_update_cloak();
    assert(!race_z8_unit_hidden_from_side(1));
    u[1].x=(s16)(500+SC64_Z8_SPORE_DETECT_PX+1);
    race_z8_update_cloak();assert(race_z8_unit_hidden_from_side(1));
}

static unsigned held_keys;
static void input(unsigned held)
{ game_update((u16)held,(u16)(held & ~held_keys));held_keys=held; }
static void press(unsigned key) { input(key);input(0); }

static void test_ordinary_save_load_visibility(void)
{
    u8 expected_visible[sizeof(visible)],expected_hidden[sizeof(race_z8_hidden)];
    game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z8_GLOBAL_ID);
    assert(ordinal==7);
    press(KEY_A);
    for(int page=0;page<ordinal/2;++page)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A);press(KEY_START);
    assert(screen==SCREEN_PLAY && race_z8_active());
    for(int i=0;i<150;++i){assert(screen==SCREEN_PLAY);input(0);}
    for(int i=0;i<20 && race_z8_camera_release;++i)input(0);
    assert(screen==SCREEN_PLAY && !race_z8_camera_release);
    press(KEY_START);assert(screen==SCREEN_PAUSE);
    press(KEY_B);press(KEY_START);
    for(int i=0;i<20;++i)input(0);
    memcpy(expected_visible,visible,sizeof(visible));
    memcpy(expected_hidden,race_z8_hidden,sizeof(race_z8_hidden));
    game_init();held_keys=0;press(KEY_L);
    assert(screen==SCREEN_PLAY && race_z8_active());
    for(int i=0;i<20;++i)input(0);
    assert(memcmp(expected_visible,visible,sizeof(visible))==0);
    assert(memcmp(expected_hidden,race_z8_hidden,sizeof(race_z8_hidden))==0);
}

int main(void)
{
    init_state();test_z8_facts_and_visibility();test_z8_detector_boundaries();
    test_ordinary_save_load_visibility();
    puts("Z8 shared source sight, cache and detector boundaries");
    return 0;
}
