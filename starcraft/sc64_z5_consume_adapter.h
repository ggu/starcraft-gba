#ifndef SC64_Z5_CONSUME_ADAPTER_H
#define SC64_Z5_CONSUME_ADAPTER_H

#if defined(SC64_Z5_CONSUME_ENABLED)
#if !defined(SC64_Z5_CLOAK_ENABLED)
#error "Z5 Consume requires the source-checked Kerrigan energy profile"
#endif
#include "sc64_z5_consume.h"

_Static_assert(SC64_Z5_ABILITIES_CONSUME_TECH_ID == SC64_Z5_CONSUME_TECH_ID &&
               SC64_Z5_ABILITIES_CONSUME_AVAILABLE == 1u &&
               SC64_Z5_ABILITIES_CONSUME_RESEARCHED == 0u &&
               SC64_Z5_ABILITIES_CONSUME_HERO_RESEARCH_EXEMPTION == 1u &&
               SC64_Z5_ABILITIES_CONSUME_RANGE_SOURCE_PIXELS == 16u &&
               SC64_Z5_ABILITIES_CONSUME_ENERGY_GAIN == 50u &&
               SC64_Z5_ABILITIES_CONSUME_ENERGY_COST == 0u &&
               SC64_Z5_ABILITIES_CONSUME_MAX_ENERGY == 250u,
               "Z5 Consume requires authenticated source permission and numeric facts");

/* The authenticated mission's eligible human targets are raw37 Zerglings.
 * Fail closed for types without source geometry rather than deriving native
 * race/eligibility from the runtime pool or its placeholder race enum. */
static int race_z5_consume_target_valid(const Unit *caster, const Unit *target)
{
    return caster && target && caster != target &&
        race_z5_cloak_actor(caster) && caster->alive && caster->hp > 0 &&
        caster->type == U_ZP_INFESTED_KERRIGAN && !(caster->flags & 8u) &&
        target->alive && target->hp > 0 && target->owner == caster->owner &&
        target->raw_owner == SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER &&
        target->raw_type == 37 && target->type == U_ZERGLING &&
        !(target->flags & 16u);
}

static int race_z5_consume_order_valid(const Unit *caster, const Unit *target)
{
    return race_z5_consume_target_valid(caster, target) &&
        caster->carry >= 0 && caster->carry <= SC64_Z5_CONSUME_MAX_ENERGY_Q8;
}

static int race_z5_consume_issue_selected(int x, int y)
{
    int hero = race_z5_cloak_hero_slot(), target = -1, best = 14 * 14 + 1;
    if (!race_z5_cloak_menu_available() || hero < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!race_z5_consume_order_valid(&u[hero], &u[i]) || invincible_units[i]) continue;
        int distance = dist2(x, y, u[i].x, u[i].y);
        if (distance < best) { best = distance; target = i; }
    }
    if (target < 0) return 0;
    Unit *caster = &u[hero];
    caster->order = ORDER_Z5_CONSUME;
    caster->target = target;
    caster->tx = u[target].x; caster->ty = u[target].y;
    caster->path_timer = caster->path_retry = 0;
    return 1;
}

/* Authored bounded order adapter: approach using ordinary GBA movement, then
 * resolve the native zero-cost kill/+50 energy effect. Native animation,
 * projectile delay and inertia are not emulated; movement allowance is zero.
 * The verified source collision extents are used at the 2:1 map boundary. */
static int __attribute__((noinline)) race_z5_consume_tick_unit(int slot)
{
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    Unit *caster = &u[slot];
    int target_slot = caster->target;
    if (caster->order != ORDER_Z5_CONSUME) return 0;
    if (!race_z5_active() || target_slot < 0 || target_slot >= MAX_UNITS ||
        target_slot == slot || !race_z5_consume_order_valid(caster, &u[target_slot]) ||
        invincible_units[target_slot]) {
        caster->order = ORDER_IDLE; caster->target = -1; return 1;
    }
    Unit *target = &u[target_slot];
    Sc64Z5DetectBox caster_box = {caster->x*2,caster->y*2,7,10,7,11};
    Sc64Z5DetectBox target_box = {target->x*2,target->y*2,8,4,7,11};
    unsigned distance = sc64_z5_box_distance(&caster_box, &target_box);
    caster->tx = target->x; caster->ty = target->y;
    if (!sc64_z5_consume_in_range((int)distance, 0)) {
        move_toward(caster, target->x, target->y); return 1;
    }
    Sc64Z5ConsumeActor source = {51, caster->owner, caster->hp, caster->carry,
                                1,1,0,0,0,0,0,0,0};
    Sc64Z5ConsumeActor food = {37, target->owner, target->hp, 0,
                              1,1,!!(target->flags & 8u),0,0,0,0,0,0};
    if (sc64_z5_consume_cast(&source, &food,
            SC64_Z5_ABILITIES_CONSUME_AVAILABLE, (int)distance, 0) != SC64_Z5_CONSUME_APPLIED)
        return 1;
    target->alive = food.alive;
    target->selected = 0;
    target->order = ORDER_IDLE; target->target = -1;
    supply_used -= unit_supply(target->type);
    caster->carry = source.energy_q8;
    caster->order = ORDER_IDLE; caster->target = -1;
    return 1;
}
#endif
#endif
