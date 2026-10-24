/* Real-engine spell controls and save state. Fixture selection/cursor setup
 * isolates the spell; mission deployment and casting use ordinary UI input.
 * This is not a complete mission playthrough. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if defined(SC64_Z6_STORM_ENABLED)
#define SPELL_STATE race_z6.storm
#define SPELL_HERO_RAW 51
#define SPELL_HERO_OWNER 3
#define SPELL_MENU (MENU_STOP+2)
#define SPELL_MISSION SC64_Z6_GLOBAL_ID
#else
#define SPELL_STATE race_p6.storm
#define SPELL_HERO_RAW 79
#define SPELL_HERO_OWNER 2
#define SPELL_MENU (MENU_STOP+1)
#define SPELL_MISSION SC64_P6_GLOBAL_ID
#endif

static unsigned held_keys;
static void input(unsigned held)
{
    game_update((u16)held,(u16)(held&~held_keys)); held_keys=held;
}
static void press(unsigned key) { input(key); input(0); }
static int hero_slot(void)
{
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_type==SPELL_HERO_RAW &&
           u[i].raw_owner==SPELL_HERO_OWNER && u[i].owner==SIDE) return i;
    return -1;
}

int main(void)
{
    RaceSave good,bad;
    Unit before[MAX_UNITS];
    Sc64PsionicStormState effect;
    game_init(); press(KEY_A); press(KEY_RIGHT); press(KEY_RIGHT);
    press(KEY_DOWN); press(KEY_A); press(KEY_A);
    assert(screen==SCREEN_PLAY && race_selected_mission==SPELL_MISSION);
    int hero=hero_slot(); assert(hero>=0);
    assert(u[hero].carry==250*256);
    for(int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    selected_building=-1; u[hero].selected=1;
    cx=u[hero].x+80; cy=u[hero].y;
    press(KEY_B); assert(menu_open);
    for(int i=0;i<SPELL_MENU;++i) press(KEY_DOWN);
    press(KEY_A); assert(mode==MODE_P4_STORM && !menu_open);
    u[hero].order=ORDER_MOVE; u[hero].tx=u[hero].x+200; u[hero].ty=u[hero].y;
    press(KEY_A); assert(mode==MODE_NORMAL && u[hero].order==ORDER_IDLE);
    assert(u[hero].carry>=175*256 && u[hero].carry<175*256+32);
    assert(sc64_psionic_storm_state_valid(&SPELL_STATE));
    unsigned active=0;
    for(unsigned i=0;i<SC64_STORM_MAX_ACTIVE;++i) active+=SPELL_STATE.storm[i].active;
    assert(active==1);

    int energy=u[hero].carry;
    stop_selected(); assert(u[hero].carry==energy);
    effect=SPELL_STATE;
    assert(race_save_game()); good=race_data;
    memset(&SPELL_STATE,0,sizeof(SPELL_STATE)); u[hero].carry=0;
    assert(race_load_game());
    assert(u[hero].carry==energy && !memcmp(&SPELL_STATE,&effect,sizeof(effect)));

    /* Reject corrupt effect geometry before changing any actor or effect. */
    bad=good;
#if defined(SC64_Z6_STORM_ENABLED)
    bad.z6.storm.storm[0].x=SC64_Z6_MAP_COLS*16u;
#else
    bad.p6.storm.storm[0].x=SC64_P6_MAP_COLS*16u;
#endif
    memcpy(before,u,sizeof(u)); effect=SPELL_STATE;
    save_write(&bad,sizeof(bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before,u,sizeof(u)) && !memcmp(&effect,&SPELL_STATE,sizeof(effect)));

    input(KEY_B|KEY_R); assert(mode==MODE_P4_STORM && !menu_open);
    input(0); press(KEY_B); press(KEY_B);
    /* An exhausted caster keeps the same selection; B+R must not fall
       through to ordinary R's bulk combat-group selection. */
    u[hero].carry=0;
    input(KEY_B|KEY_R);
    assert(mode==MODE_NORMAL && !menu_open && selected_count()==1 && u[hero].selected);
    input(0);
    input(KEY_START); assert(screen==SCREEN_PAUSE);
    effect=SPELL_STATE; energy=u[hero].carry;
    for(int i=0;i<20;++i) input(0);
    assert(!memcmp(&effect,&SPELL_STATE,sizeof(effect)) && u[hero].carry==energy);
#ifdef SC64_P6_STORM_ENABLED
    /* Native raw210 is a NonBuilding trap despite its GBA storage pool. */
    game_init(); race_mode=0; race_selected_mission=SC64_P6_GLOBAL_ID; init_state();
    game_update(0,0); hero=hero_slot(); assert(hero>=0);
    int trap=-1;
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive && b[i].raw_type==210) { trap=i; break; }
    assert(trap>=0);
    for(int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    u[hero].selected=1; selected_building=-1;
    u[hero].x=b[trap].x-100; u[hero].y=b[trap].y;
    assert(race_p6_storm_cast_selected(b[trap].x,b[trap].y));
    u32 deaths=race_p6.deaths_by_source[b[trap].raw_owner];
    for(int i=0;i<100;++i) assert(race_p6_storm_tick(0));
    assert(!b[trap].alive && b[trap].hp==0);
    assert(race_p6.deaths_by_source[b[trap].raw_owner]==deaths+1);
#endif
    puts("Actual campaign spell controls/save/pause checks passed");
    return 0;
}
