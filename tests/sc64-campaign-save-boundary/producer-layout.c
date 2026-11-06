/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Read-only ABI witness: compile with the recorded predecessor vector. */
#include <stddef.h>
#include <stdio.h>
#include SC64_PRODUCER_GBA
#include SC64_PRODUCER_GAME
_Static_assert(sizeof(RaceSave)==16296u, "predecessor payload size changed");
_Static_assert(offsetof(RaceSave,state)==15652u, "predecessor state offset changed");
_Static_assert(offsetof(RaceSave,campaign)+offsetof(Sc64RaceCampaignState,mission)==15954u,
               "predecessor mission offset changed");
_Static_assert(sizeof(((RaceSave *)0)->units[0])==35u, "predecessor unit row changed");
_Static_assert(sizeof(((RaceSave *)0)->buildings[0])==28u, "predecessor building row changed");
#define SHOW(field) printf("offset.%s=%zu size.%s=%zu\n", #field, offsetof(RaceSave,field), #field, sizeof(((RaceSave *)0)->field))
int main(void)
{
    printf("save=%zu impact=%zu units=%d buildings=%d minerals=%d\n",sizeof(RaceSave),sizeof(race_impact_record),MAX_UNITS,MAX_BUILDINGS,MAX_MINERALS);
    printf("unit_row=%zu building_row=%zu mineral_row=%zu mapcells=%d\n",sizeof(((RaceSave *)0)->units[0]),sizeof(((RaceSave *)0)->buildings[0]),sizeof(((RaceSave *)0)->minerals_map[0]),MAP_CELLS);
    printf("save_version=%08lx transport=%08lx\n",(unsigned long)SAVE_VERSION,(unsigned long)SC64_TRANSPORT_SAVE_VERSION);
    SHOW(version);SHOW(units);SHOW(buildings);SHOW(minerals_map);SHOW(fog);
    SHOW(state);SHOW(players);SHOW(ore);SHOW(vespene);SHOW(geysers);SHOW(campaign);
    SHOW(backwater);SHOW(z3);SHOW(z4);SHOW(z7);
    return 0;
}
