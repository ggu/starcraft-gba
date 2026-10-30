/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <string.h>
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    int type=unit_raw(12), slot=spawn_unit(ENEMY,type,500,500);
    assert(slot>=0 && type>=0);
    u[slot].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_REACTOR<<(2*ENEMY));
    assert(race_bc_before_unit_transfer(&u[slot],SIDE,-1));
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_YAMATO));
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_REACTOR));
    assert(u[slot].owner==ENEMY && u[slot].carry==200*256);
    u[slot].owner=SIDE;
    assert(race_save_game());sc64_battlecruiser_tech_bits=0;u[slot].carry=0;
    assert(race_load_game() && u[slot].carry==200*256);
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_REACTOR));
    race_mode=0;
#ifdef SC64_CAMPAIGN_RACE
    race_human=SC64_CAMPAIGN_RACE;
#else
    race_human=R_PROTOSS;
#endif
    race_selected_mission=sc64_frontend_first_mission(race_human);
    int human=sc64_race_maps[race_selected_mission].human_slot;
    int source=(human+1)%8;
    u[slot].owner=RESCUABLE;u[slot].raw_owner=source;
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_REACTOR<<(2*source));
    assert(race_bc_before_unit_transfer(&u[slot],SIDE,-1));
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)human,SC64_BC_TECH_YAMATO));
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)human,SC64_BC_TECH_REACTOR));
    u[slot].owner=SIDE;
    assert(sc64_battlecruiser_owner_key(0,race_selected_mission,SIDE,source)==human);
    u[slot].owner=RESCUABLE;u[slot].raw_owner=-1;
    unsigned before=sc64_battlecruiser_tech_bits;
    assert(!race_bc_before_unit_transfer(&u[slot],SIDE,-1));
    assert(sc64_battlecruiser_tech_bits==before && u[slot].owner==RESCUABLE);
    puts("BC owner-transfer grants, source provenance, invalid owner and save pass");
    return 0;
}
