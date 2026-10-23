/* Source actor integration checks; this is a rules fixture, not gameplay proof. */
#include <assert.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_t5_actor_adapter.h"

int main(void)
{
    const Sc64RaceUnitRule *kerrigan;
    const Sc64RaceUnitRule *raynor;
    const Sc64RaceUnitRule *mine;

    assert(U_KERRIGAN != U_RAYNOR_VULTURE &&
           U_RAYNOR_VULTURE != U_SPIDER_MINE);
    assert(sc64_race_unit_from_raw(16) == U_KERRIGAN);
    assert(sc64_race_unit_from_raw(19) == U_RAYNOR_VULTURE);
    assert(sc64_race_unit_from_raw(13) == U_SPIDER_MINE);
    assert(sc64_race_raw_unit(U_KERRIGAN) == 16);
    assert(sc64_race_raw_unit(U_RAYNOR_VULTURE) == 19);
    assert(sc64_race_raw_unit(U_SPIDER_MINE) == 13);

    kerrigan = sc64_race_unit_rule(U_KERRIGAN);
    raynor = sc64_race_unit_rule(U_RAYNOR_VULTURE);
    mine = sc64_race_unit_rule(U_SPIDER_MINE);
    assert(kerrigan && kerrigan->hp == 250 && kerrigan->producer == 255 &&
           kerrigan->supply == 0 && kerrigan->minerals == 0 &&
           !(kerrigan->flags & (SC_RACE_F_PRODUCER | SC_RACE_F_WORKER)));
    assert(raynor && raynor->hp == 300 && raynor->producer == 255 &&
           raynor->supply == 0 && raynor->minerals == 0 &&
           !(raynor->flags & (SC_RACE_F_PRODUCER | SC_RACE_F_WORKER)));
    assert(mine && mine->hp == 20 && mine->producer == 255 &&
           mine->supply == 0 && mine->minerals == 0 &&
           !(mine->flags & (SC_RACE_F_PRODUCER | SC_RACE_F_WORKER)));

    assert(sc64_race_unit_damage_vs(U_KERRIGAN, 0) == 30);
    assert(sc64_race_unit_damage_vs(U_KERRIGAN, 1) == 30);
    assert(sc64_race_unit_range_vs(U_KERRIGAN, 0) == 192);
    assert(sc64_race_unit_cooldown_vs(U_KERRIGAN, 0) == 22);
    assert(sc64_race_unit_damage_vs(U_RAYNOR_VULTURE, 0) == 30);
    assert(sc64_race_unit_damage_vs(U_RAYNOR_VULTURE, 1) == 0);
    assert(sc64_race_unit_range_vs(U_RAYNOR_VULTURE, 0) == 160);
    assert(sc64_race_unit_cooldown_vs(U_RAYNOR_VULTURE, 0) == 22);
    assert(sc64_race_unit_damage_vs(U_SPIDER_MINE, 0) == 0);
    assert(sc64_race_unit_damage_vs(U_SPIDER_MINE, 1) == 0);
    assert(!sc64_race_unit_can_attack_target_class(U_SPIDER_MINE, 0));
    assert(!sc64_race_unit_can_attack_target_class(U_SPIDER_MINE, 1));
    assert(!sc64_t5_actor_intrinsically_invincible(U_SPIDER_MINE));

    assert(sc64_t5_actor_weapon_damage(U_KERRIGAN, 0, 0) == 30);
    assert(sc64_t5_actor_weapon_damage(U_KERRIGAN, 2, 1) == 32);
    assert(sc64_t5_actor_weapon_damage(U_RAYNOR_VULTURE, 0, 0) == 30);
    assert(sc64_t5_actor_weapon_damage(U_RAYNOR_VULTURE, 3, 1) == 0);
    assert(sc64_t5_actor_weapon_damage(U_SPIDER_MINE, 3, 0) == 0);
    assert(sc64_t5_actor_weapon_damage(U_KERRIGAN, -7, 0) == 30);
    assert(sc64_t5_actor_weapon_damage(U_KERRIGAN, 99, 0) == 33);
    assert(sc64_t5_actor_weapon_damage(U_RAYNOR_VULTURE, 3, 0) == 36);

    /* Same post-shield integer policy as the established Ghost/Concussive
       path: Small full, Medium half, Large quarter after armor. */
    assert(sc64_t5_concussive_hp_damage(0, 10, 0) == 10);
    assert(sc64_t5_concussive_hp_damage(19, 10, 0) == 5);
    assert(sc64_t5_concussive_hp_damage(106, 10, 0) == 2);
    assert(sc64_t5_concussive_hp_damage(16, 10, 3) == 7);
    assert(sc64_t5_concussive_hp_damage(19, 10, 3) == 3);

    assert((sc64_t5_actor_flags(U_KERRIGAN) &
            (SC64_T5_ACTOR_SELECTABLE | SC64_T5_ACTOR_MOVABLE |
             SC64_T5_ACTOR_DRAWABLE | SC64_T5_ACTOR_WEAPON)) ==
           (SC64_T5_ACTOR_SELECTABLE | SC64_T5_ACTOR_MOVABLE |
            SC64_T5_ACTOR_DRAWABLE | SC64_T5_ACTOR_WEAPON));
    assert((sc64_t5_actor_flags(U_RAYNOR_VULTURE) & SC64_T5_ACTOR_MEN) != 0);
    assert(sc64_t5_actor_flags(U_SPIDER_MINE) ==
           SC64_T5_ACTOR_AUTONOMOUS_MINE);
    int left, up, right, down;
    assert(sc64_t5_actor_bounds_source(U_KERRIGAN, &left, &up, &right, &down));
    assert(left == 7 && up == 10 && right == 7 && down == 11);
    assert(sc64_t5_actor_bounds_source(U_RAYNOR_VULTURE, &left, &up, &right, &down));
    assert(left == 16 && up == 16 && right == 15 && down == 15);
    assert(sc64_t5_actor_bounds_source(U_SPIDER_MINE, &left, &up, &right, &down));
    assert(left == 7 && up == 7 && right == 7 && down == 7);
    assert(!sc64_t5_actor_bounds_source(U_MARINE, &left, &up, &right, &down));
    assert(!sc64_t5_actor_intrinsically_invincible(U_KERRIGAN));
    assert(sc64_t5_actor_flags(U_MARINE) == 0);
    assert((sc64_race_t4_actor_flags(U_KERRIGAN) &
            (SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
             SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
             SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON)) ==
           (SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
            SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
            SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON));
    assert(sc64_race_t4_actor_flags(U_SPIDER_MINE) == SC64_T4_ACTOR_DRAWABLE);
    return 0;
}
