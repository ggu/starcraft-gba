/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private host-seeded SRAM experiment; no normal-menu claim. */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void)
{
    game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    /* A pre-feature save with no BC remains loadable as zero tech. */
    assert(race_save_game());
    race_data.state[27]=0; race_data.version=SC64_LEGACY_SAVE_VERSION;
    save_write(&race_data,sizeof(race_data)-SC64_BC_SAVE_EXTRA_BYTES,SC64_LEGACY_SAVE_VERSION);
    save_write(&race_data,sizeof(race_data)-SC64_BC_SAVE_EXTRA_BYTES,SC64_LEGACY_SAVE_VERSION);
    sc64_battlecruiser_tech_bits=0xffff;
    assert(race_load_game() && sc64_battlecruiser_tech_bits==0);
    int type=unit_raw(12), slot=spawn_unit(SIDE,type,500,500);
    assert(type>=0 && slot>=0 && u[slot].carry==50*256);
    int maximum=0;
    assert(sc64_unit_energy(&u[slot],&maximum)==50*256 && maximum==200*256);
    u[slot].carry=137*256+8;
    sc64_battlecruiser_tech_bits=SC64_BC_TECH_REACTOR << (SIDE*2);
    assert(race_save_game());
    u[slot].carry=0; sc64_battlecruiser_tech_bits=0;
    assert(race_load_game());
    assert(u[slot].alive && u[slot].carry==137*256+8);
    assert(sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,SIDE,SC64_BC_TECH_REACTOR));
    assert(sc64_unit_energy(&u[slot],&maximum)==137*256+8 && maximum==250*256);
    u[slot].carry=250*256;
    assert(race_save_game()); u[slot].carry=0;
    assert(race_load_game() && u[slot].carry==250*256);
    u[slot].carry=137*256+8;
    /* Reject an impossible live energy before changing SRAM. */
    u[slot].carry=250*256+1;
    assert(!race_save_game());
    u[slot].carry=137*256+8;
    /* A legacy BC has no defined energy and must be rejected before applying. */
    assert(race_save_game());
    race_data.state[27]=0; race_data.version=SC64_LEGACY_SAVE_VERSION;
    save_write(&race_data,sizeof(race_data)-SC64_BC_SAVE_EXTRA_BYTES,SC64_LEGACY_SAVE_VERSION);
    save_write(&race_data,sizeof(race_data)-SC64_BC_SAVE_EXTRA_BYTES,SC64_LEGACY_SAVE_VERSION);
    int before_energy=u[slot].carry, before_minerals=minerals;
    unsigned before_tech=sc64_battlecruiser_tech_bits;
    assert(!race_load_game());
    assert(u[slot].alive && u[slot].carry==before_energy &&
           minerals==before_minerals && sc64_battlecruiser_tech_bits==before_tech);
    puts("experimental BC energy, tech roundtrip and legacy rejection pass");
    return 0;
}
