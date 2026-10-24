/* Actual Probe menu, placement, payment, warp-in and second-Nexus production.
 * Economy/position setup isolates this regression; this is not a mission win. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static unsigned held_keys;
static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}
static void press(unsigned key) { input(key); input(0); }

int main(void)
{
    int probe=-1, site_x=-1, site_y=-1;
    gba_init(); game_init();
    race_mode=1; race_human=R_PROTOSS; race_bots=1;
    race_skirmish_init(); screen=SCREEN_PLAY;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && u[i].type==U_PROBE) {
            probe=i; break;
        }
    assert(probe>=0);
    for (int y=64;y<WORLD_H-64 && site_x<0;y+=16)
        for (int x=64;x<WORLD_W-64;++x)
            if (!race_site_powered(SIDE,x,y) &&
                race_can_site(SIDE,B_NEXUS,x,y) && can_place(B_NEXUS,x,y)) {
                site_x=x; site_y=y; break;
            }
    assert(site_x>=0); /* A Nexus does not require a Pylon's power field. */
    clear_selection();
    u[probe].order=ORDER_IDLE;
    cx=u[probe].x; cy=u[probe].y;
    press(KEY_A);
    assert(u[probe].selected && race_nexus_command_available());
    int shuttle=spawn_unit(SIDE,
#ifdef SC64_P5_SHUTTLE_ENABLED
                          U_P5_SHUTTLE,
#else
                          U_ZEALOT,
#endif
                          500,500);
    assert(shuttle>=0);
    u[shuttle].selected=1;
    assert(!race_nexus_command_available()); /* Contextual unload keeps priority. */
    u[shuttle].selected=0;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && u[i].type==U_PROBE)
            u[i].selected=1;
    assert(race_nexus_command_available()); /* A group of only Probes also works. */
    assert(field_menu_count()==MENU_STOP+2);
    press(KEY_B);
    for (int i=0;i<MENU_STOP+1;++i) press(KEY_DOWN);
    assert(menu_cursor==MENU_STOP+1);
    game_render(); /* Scroll to and render NEXUS without an out-of-bounds label. */
    press(KEY_A);
    assert(!menu_open && mode==MODE_BUILD && menu==B_NEXUS);
    cx=site_x; cy=site_y;
    minerals=399; gas=0;
    press(KEY_A);
    assert(mode==MODE_BUILD); /* No free or discounted expansion. */
    int count_before=0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        count_before+=b[i].alive && b[i].owner==SIDE && b[i].type==B_NEXUS;
    minerals=400;
    press(KEY_A);
    assert(mode==MODE_NORMAL && u[probe].order==ORDER_BUILD);
    int nexus=u[probe].target;
    assert(nexus>=0 && b[nexus].type==B_NEXUS && b[nexus].under_construction);
    assert(b[nexus].build_progress==-1 && minerals<400);
    /* Isolate the existing arrival clock; the UI placement above made the order. */
    u[probe].x=b[nexus].x; u[probe].y=b[nexus].y;
    assert(race_builder_arrived(probe));
    assert(u[probe].alive && u[probe].order==ORDER_IDLE);
    int cap_before=race_owner_supply_cap(SIDE);
    int duration=(sc64_race_building_rules[B_NEXUS].build_frames*5+3)/4;
    for (int tick=0;tick<duration;++tick) race_tick_buildings();
    assert(building_complete(nexus));
    assert(race_owner_supply_cap(SIDE)==cap_before+18);
    int count_after=0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        count_after+=b[i].alive && b[i].owner==SIDE && b[i].type==B_NEXUS;
    assert(count_after==count_before+1);
    minerals=50;
    assert(race_queue_unit(SIDE,nexus,U_PROBE));
    assert(b[nexus].queue_count==1);
    puts("Probe NEXUS menu/payment/warp-in/expansion production checks passed");
    return 0;
}
