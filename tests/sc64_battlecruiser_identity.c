/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba.h"
#include "../starcraft/race_rules.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
#ifdef SC64_BATTLECRUISER_EXPERIMENT
    for (int type = -2; type <= SC_RU_COUNT + 2; ++type)
        assert(sc64_race_is_battlecruiser(type) == (sc64_race_raw_unit(type) == 12));
#endif
    int t = sc64_race_unit_from_raw(12);
    int sf = sc64_race_building_from_raw(116);
    int lab = sc64_race_building_from_raw(118);
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(t);
    assert(t >= 0 && sf >= 0 && lab >= 0 && r);
    assert(sc64_race_raw_unit(t) == 12 && sc64_race_raw_building(sf) == 116);
    assert(sc64_race_raw_building(lab) == 118);
    assert(r->producer == 255 && r->hp == 500 && r->minerals == 400 && r->gas == 300);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *f = sc64_zp_actor_fact(t);
    assert(f && f->raw == 12 && f->hp == 500 && f->armor == 3 && f->flying);
    assert(f->ground_damage == 25 && f->air_damage == 25);
#endif
    puts("battlecruiser and facility identity facts pass; production disabled");
    return 0;
}
