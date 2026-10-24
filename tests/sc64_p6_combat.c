#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p6_actor_rules.h"
#include "../starcraft/sc64_p6_combat.h"

static Sc64P6DamageTarget target(u32 hp, u32 shields, unsigned size,
                                u32 armor, u32 shield_armor)
{
    Sc64P6DamageTarget t;
    memset(&t, 0, sizeof(t));
    t.hp_q8 = hp * 256u; t.shields_q8 = shields * 256u;
    t.size = (u8)size; t.armor_q8 = armor * 256u;
    t.shield_armor_q8 = shield_armor * 256u;
    t.has_shields = shields != 0;
    t.ground_target = 1;
    return t;
}

static void test_infested_explosion(void)
{
    Sc64P6DamageResult r;
    Sc64P6DamageTarget t = target(1000, 0, SC64_P6_SIZE_LARGE, 0, 0);
    const Sc64P6AddedActorFact *infested =
        sc64_p6_added_actor_fact(SC64_P6_RAW_INFESTED_TERRAN);
    const Sc64P6AddedActorFact *trap =
        sc64_p6_added_actor_fact(SC64_P6_RAW_WALL_MISSILE_TRAP);
    assert(infested && infested->size == SC64_P6_SIZE_SMALL);
    assert(trap && trap->size == SC64_P6_SIZE_LARGE);
    assert(sc64_p6_infested_splash_tier(0) == SC64_P6_SPLASH_INNER);
    assert(sc64_p6_infested_splash_tier(20) == SC64_P6_SPLASH_INNER);
    assert(sc64_p6_infested_splash_tier(21) == SC64_P6_SPLASH_MIDDLE);
    assert(sc64_p6_infested_splash_tier(40) == SC64_P6_SPLASH_MIDDLE);
    assert(sc64_p6_infested_splash_tier(41) == SC64_P6_SPLASH_OUTER);
    assert(sc64_p6_infested_splash_tier(60) == SC64_P6_SPLASH_OUTER);
    assert(sc64_p6_infested_splash_tier(61) == SC64_P6_SPLASH_NONE);

    assert(sc64_p6_infested_explosion_damage(&t, 20, &r));
    assert(r.hp_damage_q8 == 500u * 256u && r.hp_after_q8 == 500u * 256u);
    assert(r.shield_damage_q8 == 0);
    assert(sc64_p6_infested_explosion_damage(&t, 40, &r));
    assert(r.hp_damage_q8 == 250u * 256u);
    assert(sc64_p6_infested_explosion_damage(&t, 60, &r));
    assert(r.hp_damage_q8 == 125u * 256u);

    /* Explicit normalized-size mapping: Small/Medium/Large are 50/75/100;
     * Independent is zero before the native half-HP minimum. */
    t = target(1000, 0, SC64_P6_SIZE_SMALL, 0, 0);
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.hp_damage_q8 == 250u * 256u);
    t.size = SC64_P6_SIZE_MEDIUM;
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.hp_damage_q8 == 375u * 256u);
    t.size = SC64_P6_SIZE_LARGE;
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.hp_damage_q8 == 500u * 256u);
    t.size = SC64_P6_SIZE_INDEPENDENT;
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.hp_damage_q8 == 128u);
    assert(sc64_p6_infested_burrowed_targetable(20, 1));
    assert(!sc64_p6_infested_burrowed_targetable(21, 1));
    assert(sc64_p6_infested_burrowed_targetable(60, 0));
    assert(sc64_p6_explosive_damage_q8(&t, 0x7fffffu, 1, &r));

    t = target(1000, 0, SC64_P6_SIZE_MEDIUM, 2, 0);
    assert(sc64_p6_infested_explosion_damage(&t, 40, &r));
    assert(r.hp_damage_q8 == 186u * 256u); /* (250 - 2 armor) * 3/4 */

    /* Armor can reduce a small hit below the source half-HP minimum. */
    t = target(3, 0, SC64_P6_SIZE_SMALL, 0, 0);
    t.armor_q8 = 500u * 256u;
    assert(sc64_p6_infested_explosion_damage(&t, 20, &r));
    assert(r.hp_damage_q8 == 128u && r.hp_after_q8 == 3u * 256u - 128u);

    /* Shields receive damage first with shield armor; residual damage then
     * uses target armor and explosive size scaling. */
    t = target(1000, 100, SC64_P6_SIZE_LARGE, 3, 1);
    assert(sc64_p6_infested_explosion_damage(&t, 20, &r));
    assert(r.shield_damage_q8 == 100u * 256u);
    assert(r.shields_after_q8 == 0);
    assert(r.hp_damage_q8 == 396u * 256u);
    assert(r.hp_after_q8 == 604u * 256u);

    /* The reference only enters shield absorption at >=1 HP of shields. A
     * fractional half-shield remains untouched while HP armor/minimum apply. */
    t = target(1000, 0, SC64_P6_SIZE_SMALL, 500, 0);
    t.has_shields = 1; t.shields_q8 = 128;
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.shield_damage_q8 == 0 && r.shields_after_q8 == 128);
    assert(r.hp_damage_q8 == 128);

    /* A partially overflowing shield packet sends the remainder through
     * armor and the normalized small-unit factor. */
    t = target(1000, 1, SC64_P6_SIZE_SMALL, 0, 0);
    assert(sc64_p6_explosive_damage_q8(&t, 500, 1, &r));
    assert(r.shield_damage_q8 == 256);
    assert(r.hp_damage_q8 == (499u * 128u));

    t.invincible = 1;
    assert(sc64_p6_infested_explosion_damage(&t, 20, &r));
    assert(r.hp_damage_q8 == 0 && r.shield_damage_q8 == 0);
    t.invincible = 0; t.ground_target = 0;
    assert(sc64_p6_infested_explosion_damage(&t, 20, &r));
    assert(r.hp_damage_q8 == 0 && r.shield_damage_q8 == 0);
    t.ground_target = 1;
    assert(sc64_p6_infested_explosion_damage(&t, 61, &r));
    assert(r.hp_damage_q8 == 0 && r.shield_damage_q8 == 0 &&
           r.hp_after_q8 == t.hp_q8 && r.shields_after_q8 == t.shields_q8);

    /* Invalid requests fail without changing the output record. */
    memset(&r, 0x5a, sizeof(r));
    {
        Sc64P6DamageResult old = r;
        t.size = 4;
        assert(!sc64_p6_infested_explosion_damage(&t, 10, &r));
        assert(memcmp(&r, &old, sizeof(r)) == 0);
        t.size = 2;
        assert(!sc64_p6_explosive_damage_q8(&t, 0x800000u, 1, &r));
        assert(memcmp(&r, &old, sizeof(r)) == 0);
    }
}

static void test_permanent_cloak(void)
{
    assert(sc64_p6_requires_detector(74));
    assert(sc64_p6_requires_detector(75));
    assert(!sc64_p6_requires_detector(73));
    assert(sc64_p6_cloaked_target_detected(74, 2, (u32)1u << 2));
    assert(!sc64_p6_cloaked_target_detected(74, 2, 0));
    assert(!sc64_p6_cloaked_target_detected(75, 32, 0xffffffffu));
    assert(sc64_p6_cloaked_target_detected(37, 31, 0));
}

static void test_wall_missile_trap(void)
{
    u16 cooldown = 0;
    assert(sc64_p6_wall_trap_can_target(1, 1, 0, 0, 1, 160));
    assert(!sc64_p6_wall_trap_can_target(1, 1, 0, 0, 1, 161));
    assert(!sc64_p6_wall_trap_can_target(1, 1, 1, 0, 1, 20));
    assert(!sc64_p6_wall_trap_can_target(1, 1, 0, 1, 1, 20));
    assert(!sc64_p6_wall_trap_can_target(1, 1, 0, 0, 0, 20));
    assert(!sc64_p6_wall_trap_can_target(1, 0, 0, 0, 1, 20));
    assert(!sc64_p6_wall_trap_can_target(0, 1, 0, 0, 1, 20));
    assert(sc64_p6_wall_trap_fire(&cooldown));
    assert(cooldown == 22 && !sc64_p6_wall_trap_fire(&cooldown));
    for (unsigned i = 0; i < 21; ++i) sc64_p6_wall_trap_cooldown_tick(&cooldown);
    assert(cooldown == 1 && !sc64_p6_wall_trap_fire(&cooldown));
    sc64_p6_wall_trap_cooldown_tick(&cooldown);
    assert(cooldown == 0 && sc64_p6_wall_trap_fire(&cooldown));
}

int main(void)
{
    test_infested_explosion();
    test_permanent_cloak();
    test_wall_missile_trap();
    return 0;
}
