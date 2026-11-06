/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual campaign initialization and authored rescue transitions. This is
 * controller/payer context evidence, not ordinary campaign completion. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_research_human_binding.h"
_Static_assert(SIDE == SC64_PR_BINDING_HUMAN_OWNER, "compact human ABI");

static void roles_for_current_map(const Sc64RaceMap *map, int8_t roles[8])
{
    for (unsigned source = 0; source < 8; ++source)
        roles[source] = (int8_t)race_campaign_owner(map, (int)source);
}
static Sc64ResearchHumanBinding bind_current(const Sc64RaceMap *map, int owner)
{
    int8_t roles[8]; Sc64ResearchHumanBinding out;
    roles_for_current_map(map,roles);
    assert(sc64_research_bind_protoss_human(race_mode,map->mission_id,
        map->human_slot,roles,owner,&out));
    return out;
}
static void campaign_census(void)
{
    int count = 0;
    for (int id = 0; id < SC64_RACE_MAP_COUNT; ++id) {
        const Sc64RaceMap *map = &sc64_race_maps[id];
        if (!map->mission_id || map->mission_id[0] != 'P') continue;
        game_init(); race_mode = 0; race_selected_mission = id; init_state();
        assert(race_selected_mission == id && screen == SCREEN_PLAY);
        Sc64ResearchHumanBinding out = bind_current(map,SIDE);
        assert(out.current_controller_key == map->human_slot &&
               out.payer_key == map->human_slot && out.wallet_slot == SIDE &&
               out.compact_owner == SIDE);
        /* The human bank accessors use these globals rather than the
         * independently stored AI bank array. Authored values prove routing. */
        minerals = 123; gas = 456; race_minerals[SIDE] = 17; race_gas[SIDE] = 19;
        assert(race_owner_minerals(out.wallet_slot) == 123 &&
               race_owner_gas(out.wallet_slot) == 456);
        ++count;
    }
    assert(count == 10);
}
static void rescue_and_detached_context(void)
{
    game_init(); race_mode = 0; race_selected_mission = SC64_RACE_MISSION_P1; init_state();
    const Sc64RaceMap *map = &sc64_race_maps[race_selected_mission];
    int visitor = -1, nexus = -1; int8_t roles[8];
    roles_for_current_map(map,roles);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { visitor = i; break; }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_NEXUS && b[i].owner == RESCUABLE &&
            b[i].raw_owner == 1) { nexus = i; break; }
    assert(visitor >= 0 && nexus >= 0);
    Sc64ResearchHumanBinding out;
    assert(!sc64_research_bind_protoss_human(0,map->mission_id,
        map->human_slot,roles,b[nexus].owner,&out));
    u[visitor].x = 400; u[visitor].y = 900; race_campaign_tick();
    assert(b[nexus].owner == SIDE && b[nexus].raw_owner == 1);
    out = bind_current(map,b[nexus].owner);
    assert(out.current_controller_key == 7 && out.payer_key == 7 &&
           b[nexus].raw_owner == 1);
    /* A detached resolver must consume its captured canonical context, not
     * whichever live mission happens to be selected during preflight. */
    race_selected_mission = SC64_P10_GLOBAL_ID; race_mode = 1;
    assert(sc64_research_bind_protoss_human(0,map->mission_id,
        map->human_slot,roles,SIDE,&out) && out.current_controller_key == 7);
    assert(!sc64_research_bind_protoss_human(1,map->mission_id,
        map->human_slot,roles,SIDE,&out));
}
int main(void)
{
    gba_init(); campaign_census(); rescue_and_detached_context();
    puts("actual P1-P10 human binding, wallet routing and P1 retained-origin rescue: PASS");
    return 0;
}
