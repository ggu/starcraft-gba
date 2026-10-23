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
    const int p3_enabled =
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
        1;
#else
        0;
#endif

    assert(sc64_catalog_mission_faction(22) == SC64_FACTION_PROTOSS);
    assert(!sc64_catalog_is_playable(23));
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == (p3_enabled ? 3 : 2));
    assert(sc64_catalog_mission_allowed(22, SC64_FACTION_PROTOSS) == p3_enabled);
    assert(!sc64_catalog_mission_allowed(22, SC64_FACTION_ZERG));

    sc64_frontend_reset(&f, 22, SC_FRONT_RACE_PROTOSS, 1, 22);
    assert(f.faction == SC_FRONT_PROTOSS);
    assert(f.mission == (p3_enabled ? 22 : 2));
    assert(f.completed == (p3_enabled ? 22 : 5));
    if (p3_enabled) {
        assert(f.mission_page == 1 && f.row == 0);
        assert(press(&f, KEY_A) == SC_FRONT_NONE);
#ifdef SC64_CAMPAIGN_RACE
        assert(f.page == SC_FRONT_MISSION);
#else
        assert(f.page == SC_FRONT_CAMPAIGN && f.row == SC_FRONT_PROTOSS);
        assert(press(&f, KEY_A) == SC_FRONT_NONE);
#endif
        assert(f.page == SC_FRONT_MISSION && f.mission_page == 1 && f.row == 0);
        assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS,
                                        f.mission_page * 2 + f.row) == 22);
        assert(press(&f, KEY_A) == SC_FRONT_NONE);
        assert(f.page == SC_FRONT_BRIEFING && f.mission == 22);
        assert(press(&f, KEY_A) == SC_FRONT_START_CAMPAIGN);
        assert(f.mission == 22);
    } else {
        assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS, 2) == -1);
        assert(f.mission_page == 0 && f.row == 0);
    }

    puts("P3 frontend/catalog gate and mission paging passed");
    return 0;
}
