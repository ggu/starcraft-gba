/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual P8 actor pools; differential checks of the optional condition scan. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_P8_RUNTIME) || !defined(SC64_P8_LIVE_CONDITION_PROBE)
#error "requires P8 and the live-condition probe"
#endif
static void compare_conditions(void)
{
    for(unsigned i=0;i<SC64_P8_SOURCE_CONDITION_COUNT;i++)
        for(unsigned owner=0;owner<8;owner++) {
            const Sc64P8SourceCondition *c=&sc64_p8_source_conditions[i];
            int baseline=sc64_p8_world_condition(&race_p8_world,c,owner);
            int candidate=race_p8_condition_live_scan(&race_p8_world,c,owner);
            assert(candidate==baseline);
            assert(race_p8_view.actor_at==sc64_z8_game_actor_at &&
                   race_p8_view.actor_count==MAX_UNITS+MAX_BUILDINGS);
        }
}
int main(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_P8_GLOBAL_ID;init_state();
    assert(screen==SCREEN_PLAY && race_p8_active() && race_p8_map);
    compare_conditions();
    int first=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive){first=i;break;}
    assert(first>=0);Unit saved=u[first];
    u[first].alive=0;compare_conditions();u[first]=saved;
    u[first].raw_owner=-1;compare_conditions();u[first]=saved;
    u[first].raw_type=255;compare_conditions();u[first]=saved;
    u[first].x=-1;compare_conditions();u[first]=saved;
    int created=spawn_unit(SIDE,U_MARINE,256,256);assert(created>=0);
    /* Source ownership is seeded for the imported-P8 actor-view contract. */
    u[created].raw_owner=SC64_P8_HUMAN_SLOT;u[created].raw_type=0;
    compare_conditions();
    u[created].alive=0;compare_conditions();
    puts("P8 live-slot condition scan matches the original provider");return 0;
}
