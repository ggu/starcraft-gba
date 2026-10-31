/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The first fixture drives the menu/cast/field/expiry through game_update.
 * The second saturates the real actor budget with synthetic reserved fields. */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_DEFILER_UNIT_FIELD_PROBE
#error "requires experimental unit-row field storage"
#endif
int main(void)
{
    assert(previous_defiler_test()==0);
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    for(int i=0;i<MAX_UNITS;i++) {
        memset(&u[i],0,sizeof(u[i]));
        sc64_defiler_field_set(i,(Sc64DarkSwarmEffect){320,320,1});
        assert(!u[i].alive && u[i].raw_type==202 && !SC64_UNIT_SLOT_FREE(i));
        assert(!target_alive(i));
    }
    assert(find_unit_slot()<0);
    /* No living players exist in this saturated synthetic state; keep the
     * normal play loop active so every reserved field can expire. */
    for(int n=0;n<30;n++) {screen=SCREEN_PLAY;game_update(0,0);}
    for(int i=0;i<MAX_UNITS;i++)assert(!sc64_defiler_field_get(i).remaining && SC64_UNIT_SLOT_FREE(i));
    assert(find_unit_slot()>=0);
    puts("unit-row field pool saturation, expiry and reuse passed");return 0;
}
