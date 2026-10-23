#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_frontend.h"

static int press(Sc64Frontend *f, unsigned key)
{
    int action = sc64_frontend_update(f, key);
    (void)sc64_frontend_update(f, 0);
    return action;
}

int main(void)
{
    Sc64Frontend f;
    sc64_frontend_reset(&f, 99, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(f.page == SC_FRONT_HOME);
    assert(f.mission != 7);
    assert(!sc64_catalog_mission_allowed(7, SC64_FACTION_TERRAN));
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 2);

#ifdef SC64_T4_REPLAY_ENTRY
    assert(press(&f, KEY_SELECT) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_HOME && f.mission != 7);
    assert(press(&f, KEY_START | KEY_SELECT) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 7);
    assert(f.faction == SC_FRONT_TERRAN);
    assert(!sc64_catalog_mission_allowed(f.mission, SC64_FACTION_TERRAN));
    assert(sc64_catalog_playable_count(SC64_FACTION_TERRAN) == 2);
    assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
    assert(f.page == SC_FRONT_BRIEFING && f.mission == 7);

    assert(press(&f, KEY_B) == SC_FRONT_NONE);
    assert(f.page == SC_FRONT_HOME && f.mission == 7);
    sc64_frontend_reset(&f, 7, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(f.mission == 7 && f.page == SC_FRONT_HOME);
    sc64_frontend_reset(&f, 8, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(f.mission != 7 && sc64_catalog_mission_allowed(f.mission, SC64_FACTION_TERRAN));
#else
    assert(press(&f, KEY_START | KEY_SELECT) == SC_FRONT_NONE);
    assert(f.page != SC_FRONT_BRIEFING && f.mission != 7);
    sc64_frontend_reset(&f, 7, SC_FRONT_RACE_TERRAN, 1, -1);
    assert(f.mission != 7);
#endif

    puts("sc64 T4 internal frontend entry: ok");
    return 0;
}
