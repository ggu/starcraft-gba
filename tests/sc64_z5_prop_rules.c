#include <assert.h>
#include "../starcraft/race_rules.h"

#if !defined(SC64_Z5_ACTORS) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "Z5 prop fixture requires its actor prerequisites"
#endif

int main(void)
{
#ifndef SC64_Z5_DOODADS
    _Static_assert(SC_RB_COUNT == B_Z5_TERRAN_BEACON + 1,
                   "Z5 prop-off building IDs changed");
    assert(sc64_race_building_from_raw(203) ==
#ifdef SC64_T4_RUNTIME
           B_FLOOR_MISSILE_TRAP
#else
           -1
#endif
    );
    return 0;
#else
    static const int raws[] = {203,205,206,207,208,209,211,213};
    static const int types[] = {
        B_Z5_FLOOR_MISSILE_TRAP, B_Z5_UPPER_DOOR, B_Z5_RIGHT_UPPER_DOOR,
        B_Z5_PIT_DOOR, B_Z5_RIGHT_PIT_DOOR, B_Z5_FLOOR_GUN_TRAP,
        B_Z5_WALL_FLAME_TRAP, B_Z5_RIGHT_WALL_FLAME_TRAP
    };
    _Static_assert(B_Z5_FLOOR_MISSILE_TRAP == B_Z5_TERRAN_BEACON + 1,
                   "Z5 prop IDs must append without reindexing prior actors");
    _Static_assert(B_Z5_RIGHT_WALL_FLAME_TRAP + 1 == SC_RB_COUNT,
                   "Z5 prop enum tail/count mismatch");
    for (unsigned i=0; i<sizeof(raws)/sizeof(raws[0]); ++i) {
        int raw=raws[i], type=types[i], l=0,u=0,r=0,d=0;
        const Sc64Z5BuildingFact *source=sc64_z5_building_fact(raw);
        const Sc64ZpBuildingFact *policy=sc64_zp_building_fact(type);
        unsigned expected_policy = SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING;
        assert(source && policy);
        assert(source->raw==raw);
        assert(source->flags&SC64_Z5_REF_HAS_SPRITE);
        if (raw==205 || raw==206 || raw==207 || raw==208) {
            assert(!(source->flags&SC64_Z5_SOURCE_BUILDING));
            assert(!(source->flags&SC64_Z5_SOURCE_DETECTOR));
            assert(source->size == 3 && policy->size == 3); /* Independent */
            expected_policy |= SC64_ZP_ACTOR_INVINCIBLE;
            assert(source->hp==100000 && (source->flags&SC64_Z5_SOURCE_INVINCIBLE));
            assert(!(source->flags&SC64_Z5_SOURCE_HAS_GROUND_WEAPON));
        } else {
            assert(!(source->flags&SC64_Z5_SOURCE_BUILDING));
            assert(source->flags&SC64_Z5_SOURCE_DETECTOR);
            assert(source->size == 2 && policy->size == 2); /* Large */
            expected_policy |= SC64_ZP_ACTOR_WEAPON;
            assert(source->hp==50 && (source->flags&SC64_Z5_SOURCE_HAS_GROUND_WEAPON));
            assert(!(source->flags&SC64_Z5_SOURCE_INVINCIBLE));
        }
        assert(policy->raw==raw && policy->hp==source->hp && policy->shields==0);
        if (source->flags&SC64_Z5_SOURCE_DETECTOR) {
            assert(raw==203 || raw==209 || raw==211 || raw==213);
            assert(source->sight_px==(raw==203 || raw==209 ? 192 : 96));
            /* Native traps are not buildings, so OpenBW does not apply its
             * grounded-building fixed 7-tile detector range. */
        }
        assert(policy->flags==expected_policy);
        assert(!(policy->flags&(SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE)));
        assert(sc64_race_building_from_raw(raw)==type);
        assert(sc64_race_raw_building(type)==raw);
        assert(sc64_race_building_armor(type)==source->armor);
#ifdef SC64_T4_RUNTIME
        assert(sc64_race_building_max_hp(type)==source->hp);
        assert(sc64_race_t4_source_size(raw)==SC64_T4_SIZE_LARGE);
#endif
        assert(sc64_zp_building_default_hp(type)==source->hp);
        assert(sc64_zp_building_flags(type)==expected_policy);
        assert(sc64_zp_building_bounds_source(type,&l,&u,&r,&d));
        assert(l==source->left && u==source->up && r==source->right && d==source->down);
    }
#endif
    return 0;
}
