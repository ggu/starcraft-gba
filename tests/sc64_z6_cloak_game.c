/* Actual game-array Z6 cloak integration, with normal menu input and the
 * mission's real starting actors. Synthetic detector placements below are
 * isolated checks, not a campaign win claim. */
#define main sc64_z6_runtime_test_main
#include "sc64_z6_runtime.c"
#undef main
#include <stdio.h>

#ifndef SC64_Z6_CLOAK_ENABLED
#error "Compile this fixture with the Z6 cloak profile"
#endif

static unsigned z6_held_keys;
static void z6_input(unsigned held)
{
    game_update((u16)held,(u16)(held & ~z6_held_keys));
    z6_held_keys=held;
}
static void z6_press(unsigned key)
{ z6_input(key); z6_input(0); }

static void launch_z6(void)
{
    int ordinal;
    game_init();
    z6_held_keys=0;
    assert(race_front.page==SC_FRONT_HOME);
    ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z6_GLOBAL_ID);
    assert(ordinal==5);
    z6_press(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    for(int page=0;page<ordinal/2;page++) z6_press(KEY_RIGHT);
    if(ordinal&1) z6_press(KEY_DOWN);
    z6_press(KEY_A);
    assert(race_front.page==SC_FRONT_BRIEFING && race_front.mission==SC64_Z6_GLOBAL_ID);
    z6_press(KEY_A);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==SC64_Z6_GLOBAL_ID);
    assert(race_z6_active() && race_z6_cloak_world_state_valid());
}

static int select_kerrigan(void)
{
    int hero=race_z6_cloak_hero_slot();
    assert(hero>=0 && u[hero].alive && u[hero].raw_type==51 && u[hero].raw_owner==3);
    clear_selection();
    u[hero].selected=1;
    return hero;
}

static void open_command(unsigned row)
{
    z6_press(KEY_B);
    assert(menu_open && menu_cursor==0);
    for(unsigned i=0;i<row;i++) z6_press(KEY_DOWN);
    assert(menu_cursor==(int)row);
}

static void cloak_controls_energy_and_pause(void)
{
    launch_z6();
    int hero=select_kerrigan();
    assert(u[hero].carry==SC64_Z5_CLOAK_MAX_ENERGY_Q8);
    assert(!race_z6.cloak.active && !race_z6.cloak.regen_remainder);
    assert(race_z6_cloak_menu_available() && field_menu_count()==MENU_STOP+3);

    open_command(MENU_STOP+1);
    int before=u[hero].carry;
    z6_press(KEY_A);
    assert(!menu_open && race_z6.cloak.active);
    /* The command spends 25 energy; the normal update then advances one
       source-converted drain tick. */
    assert(u[hero].carry==before-25*256-8);
    z6_press(KEY_START);
    assert(screen==SCREEN_PAUSE);
    before=u[hero].carry;
    Sc64Z5CloakState saved_phase=race_z6.cloak;
    for(unsigned i=0;i<60;i++) z6_input(0);
    assert(u[hero].carry==before &&
           memcmp(&saved_phase,&race_z6.cloak,sizeof(saved_phase))==0);
    z6_input(KEY_A);
    assert(screen==SCREEN_PLAY && u[hero].carry==before);

    open_command(MENU_STOP+1);
    before=u[hero].carry;
    z6_press(KEY_A);
    assert(!race_z6.cloak.active);
    /* Uncloaking has no activation refund or extra cost; it retains energy
       and starts the ordinary fractional regeneration on subsequent ticks. */
    assert(u[hero].carry>=before-8 && u[hero].carry<=before+8);
    assert(race_z6_cloak_world_state_valid());
}

static int add_observer(int owner, int raw_owner, int x, int y)
{
    int slot=spawn_unit(owner,U_DRAGOON,x,y);
    assert(slot>=0);
    u[slot].raw_type=84;
    u[slot].raw_owner=(s16)raw_owner;
    return slot;
}

static int add_cannon(int owner, int raw_owner, int x, int y, int constructing)
{
    int slot=spawn_build(owner,B_PHOTON_CANNON,x,y,constructing);
    assert(slot>=0);
    b[slot].raw_type=162;
    b[slot].raw_owner=(s16)raw_owner;
    return slot;
}

static void detector_identity_owner_liveness_and_ranges(void)
{
    launch_z6();
    int hero=select_kerrigan();
    race_z6.cloak.active=1;
    race_z6.cloak.regen_remainder=0;
    u[hero].x=1000; u[hero].y=1000;
    assert(!race_z6_cloaked_target_attackable(hero));
    assert(!race_can_attack(U_MARINE,hero));

    int observer=add_observer(SIDE,3,1000,1000);
    assert(!race_z6_cloaked_target_attackable(hero)); /* Allied detector. */
    u[observer].owner=ENEMY; u[observer].raw_owner=1;
    assert(race_z6_cloaked_target_attackable(hero));
    u[observer].x=u[hero].x+156;
    assert(race_z6_cloaked_target_attackable(hero)); /* Inclusive box-distance edge. */
    u[observer].x=u[hero].x+157;
    assert(!race_z6_cloaked_target_attackable(hero)); /* One GBA pixel beyond edge. */
    u[observer].x=1000; u[observer].alive=0;
    assert(!race_z6_cloaked_target_attackable(hero)); /* Dead detector. */

    int cannon=add_cannon(ENEMY,1,1000,1000,1);
    assert(!race_z6_cloaked_target_attackable(hero)); /* Incomplete detector. */
    b[cannon].under_construction=0;
    assert(race_z6_cloaked_target_attackable(hero));
    b[cannon].owner=SIDE; b[cannon].raw_owner=3;
    assert(!race_z6_cloaked_target_attackable(hero)); /* Allied detector. */
    b[cannon].owner=ENEMY; b[cannon].raw_owner=1;
    b[cannon].x=u[hero].x+126;
    assert(race_z6_cloaked_target_attackable(hero)); /* Inclusive box-distance edge. */
    b[cannon].x=u[hero].x+127;
    assert(!race_z6_cloaked_target_attackable(hero)); /* One GBA pixel beyond edge. */
    b[cannon].x=1000; b[cannon].alive=0;
    assert(!race_z6_cloaked_target_attackable(hero));

    race_z6.cloak.active=0;
    assert(race_z6_cloaked_target_attackable(hero));
    assert(race_can_attack(U_MARINE,hero));
}

static void save_roundtrip_and_candidate_rejection(void)
{
    launch_z6();
    int hero=select_kerrigan();
    race_z6.cloak.active=1;
    race_z6.cloak.regen_remainder=0;
    u[hero].carry=50000;
    for(unsigned i=0;i<3;i++)
        assert(race_z6_cloak_tick(0));
    assert(race_save_game());
    RaceSave good=race_data;
    assert(SC64_RACE_SAVE_GENERATION==31u);
    assert(race_z6_save_candidate_valid(&good,&good.z6,
                                       SC64_Z6_MAP_COLS,SC64_Z6_MAP_ROWS));
    assert(race_load_game());
    hero=race_z6_cloak_hero_slot();
    assert(hero>=0 && race_z6.cloak.active &&
           race_z6.cloak.regen_remainder==good.z6.cloak.regen_remainder);
    Unit saved_hero;
    assert(race_save_decode_unit(&good,(unsigned)hero,WORLD_W,WORLD_H,&saved_hero));
    assert(u[hero].carry==saved_hero.carry);

    /* Keep the actual live world unchanged on a rejected saved candidate. */
    Unit old_units[MAX_UNITS];
    Building old_buildings[MAX_BUILDINGS];
    Sc64Z6PersistentState old_state=race_z6;
    memcpy(old_units,u,sizeof(u)); memcpy(old_buildings,b,sizeof(b));
    RaceSave bad=good;
    bad.z6.cloak.active=2;
    save_write(&bad,sizeof(bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(old_units,u,sizeof(u))==0 && memcmp(old_buildings,b,sizeof(b))==0);
    assert(memcmp(&old_state,&race_z6,sizeof(old_state))==0);

    bad=good;
    Unit decoded;
    assert(race_save_decode_unit(&bad,(unsigned)hero,WORLD_W,WORLD_H,&decoded));
    decoded.carry=SC64_Z5_CLOAK_MAX_ENERGY_Q8+1;
    assert(sc64_zp_unit_save_encode(&bad.units[hero],&decoded,WORLD_W,WORLD_H));
    save_write(&bad,sizeof(bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(old_units,u,sizeof(u))==0 && memcmp(old_buildings,b,sizeof(b))==0);
    assert(memcmp(&old_state,&race_z6,sizeof(old_state))==0);
    save_write(&good,sizeof(good),SAVE_VERSION);
    assert(race_load_game() && race_z6_cloak_world_state_valid());
}

int main(void)
{
    cloak_controls_energy_and_pause();
    detector_identity_owner_liveness_and_ranges();
    save_roundtrip_and_candidate_rejection();
    puts("Z6 cloak actual arrays, controls, detector checks and save validation passed");
    return 0;
}
