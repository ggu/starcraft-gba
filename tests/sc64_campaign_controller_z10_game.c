/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST boundary cases using the real Z10 launch and actor pools. */
#define main sc64_existing_z10_fixture_main
#include "sc64_z10_game.c"
#undef main

int main(void)
{
    int building=-1, unit=-1, produced, origin, old_owner;
    launch_z10_from_normal_menu();
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==ENEMY && sc64_controller_building(i)==2) { building=i; break; }
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && sc64_controller_unit(i)>=0) { unit=i; break; }
    assert(building>=0 && unit>=0);
    origin=b[building].raw_owner; old_owner=b[building].owner;
    assert(race_zl_source_owner(2)==ENEMY && race_zl_source_owner(7)==ENEMY);
    assert(sc64_controller_transfer_building(building,7));
    assert(b[building].owner==old_owner && b[building].raw_owner==origin);
    assert(sc64_controller_building(building)==7);
    produced=race_spawn_product(ENEMY,U_ZEALOT,b[building].x,b[building].y,-1,building);
    assert(produced>=0 && sc64_controller_unit(produced)==7);
    assert(!sc64_controller_transfer_building(building,8));
    assert(sc64_controller_building(building)==7);
    sc64_current_building[building]=255;
    assert(sc64_controller_building(building)==-1);
    produced=race_spawn_product(ENEMY,U_ZEALOT,b[building].x,b[building].y,-1,building);
    assert(produced>=0 && sc64_controller_unit(produced)==-1);
    assert(!race_save_game() && !race_load_game());
    u[produced].alive=0;
    assert(sc64_controller_unit(produced)==-1);
    sc64_current_unit[produced]=7;
    assert(spawn_unit(ENEMY,U_ZEALOT,100,100)==produced);
    assert(sc64_controller_unit(produced)==-1);
    puts("Z10 current controller stage: PASS");
    return 0;
}
