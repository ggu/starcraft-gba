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
/* P8 has a CreateUnit opcode but no supported kill/remove opcode. This
 * test-only callback exercises removal through the real engine death path;
 * it is not presented as an authored native P8 trigger action. */
static void synthetic_remove_action(int slot)
{
    assert(slot>=0 && slot<MAX_UNITS && u[slot].alive);
    hit_target(slot,100000,U_MARINE,SIDE);
    assert(!u[slot].alive);
}
static void test_source_action_between_conditions(void)
{
    const Sc64P8SourceAction *action=0;
    for(unsigned i=0;i<SC64_P8_SOURCE_ACTION_COUNT;i++) {
        const Sc64P8SourceAction *a=&sc64_p8_source_actions[i];
        if(a->opcode==11u && !a->group2 && a->location && a->group1<8u &&
           a->unit<106u && a->unit!=101u && a->trigger==4u) {action=a;break;}
    }
    assert(action);
    Sc64P8SourceCondition condition={0};
    condition.opcode=2u;condition.comparison=0u;condition.player=action->group1;
    condition.unit=action->unit;
    Sc64Z8Condition z=sc64_p8_world_condition_as_z8(&condition);u32 before=0;
    assert(sc64_z8_world_count(&race_p8_view,&z,SC64_P8_HUMAN_SLOT,0,&before)==1);
    condition.amount=before+1u;
    assert(sc64_p8_world_condition(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==0);
    assert(race_p8_condition_live_scan(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==0);
    u8 alive[MAX_UNITS];for(unsigned i=0;i<MAX_UNITS;i++)alive[i]=(u8)(u[i].alive!=0);
    assert(sc64_p8_source_action_dispatch(&race_p8_source_ops,action->trigger,
            action->index,SC64_P8_HUMAN_SLOT,0)==SC64_P8_ACTION_COMPLETE);
    int created=-1;for(int i=0;i<MAX_UNITS;i++)if(!alive[i] && u[i].alive) {
        assert(created<0);created=i;
    }
    assert(created>=0 && u[created].raw_type==(int)action->unit &&
           u[created].raw_owner==(int)action->group1);
    assert(sc64_p8_world_condition(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==1);
    assert(race_p8_condition_live_scan(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==1);
    synthetic_remove_action(created);
    assert(sc64_p8_world_condition(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==0);
    assert(race_p8_condition_live_scan(&race_p8_world,&condition,SC64_P8_HUMAN_SLOT)==0);
    assert(race_p8_condition_live_scan(&race_p8_world,&condition,8u)==-1);
    assert(race_p8_view.actor_at==sc64_z8_game_actor_at &&
           race_p8_view.actor_count==MAX_UNITS+MAX_BUILDINGS);
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
    test_source_action_between_conditions();
    puts("P8 live-slot condition scan matches the original provider");return 0;
}
