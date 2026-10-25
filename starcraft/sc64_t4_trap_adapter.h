#ifndef SC64_T4_TRAP_ADAPTER_H
#define SC64_T4_TRAP_ADAPTER_H

#ifdef SC64_T4_SCENARIO_RUNTIME

enum {
    SC64_T4_TRAP_COOLDOWN = 28,
    SC64_T4_TRAP_EXPLOSIVE = 1,
    SC64_T4_TRAP_CONCUSSIVE = 2,
    SC64_T4_TRAP_NORMAL = 3
};

typedef struct {
    int raw_type;
    int damage;
    int factor;
    int damage_type;
    int range;
} Sc64T4TrapRule;

/* Values follow pinned BWAPI WeaponType defaults for weapons 96-99. Ranges
   and cooldowns are scaled at the engine boundary (source pixels / 2). */
static int
race_t4_trap_rule(int building_type, Sc64T4TrapRule *rule)
{
    if (!rule) return 0;
    switch (building_type) {
    case B_FLOOR_MISSILE_TRAP: /* raw 203, weapon 99 */
        rule->raw_type = 203; rule->damage = 10; rule->factor = 2;
        rule->damage_type = SC64_T4_TRAP_EXPLOSIVE; rule->range = 80;
        return 1;
    case B_FLOOR_GUN_TRAP: /* raw 209, weapon 96 */
        rule->raw_type = 209; rule->damage = 10; rule->factor = 1;
        rule->damage_type = SC64_T4_TRAP_NORMAL; rule->range = 80;
        return 1;
    case B_WALL_FLAME_TRAP: /* raw 211, weapon 98 */
        rule->raw_type = 211; rule->damage = 8; rule->factor = 1;
        rule->damage_type = SC64_T4_TRAP_CONCUSSIVE; rule->range = 32;
        return 1;
    case B_RIGHT_WALL_MISSILE_TRAP: /* raw 212, weapon 97 */
        rule->raw_type = 212; rule->damage = 10; rule->factor = 2;
        rule->damage_type = SC64_T4_TRAP_EXPLOSIVE; rule->range = 80;
        return 1;
    default:
        return 0;
    }
}

/* BW applies size multipliers after armor. The engine stores integral HP, so
   fractional damage rounds down, with one HP as the minimum positive hit. */
static int
race_t4_trap_hp_damage(int raw_target, int damage, int armor,
                       int damage_type)
{
    int after_armor;
    int size = sc64_race_t4_source_size(raw_target);
    if (damage < 0) damage = 0;
    if (armor < 0) armor = 0;
    after_armor = damage - armor;
    if (after_armor < 1) after_armor = 1;
    if (damage_type == SC64_T4_TRAP_EXPLOSIVE) {
        if (size == SC64_T4_SIZE_SMALL) after_armor /= 2;
        else if (size == SC64_T4_SIZE_MEDIUM)
            after_armor = (after_armor * 3) / 4;
    } else if (damage_type == SC64_T4_TRAP_CONCUSSIVE) {
        if (size == SC64_T4_SIZE_MEDIUM) after_armor /= 2;
        else if (size == SC64_T4_SIZE_LARGE) after_armor /= 4;
    }
    return after_armor > 0 ? after_armor : 1;
}

static void
race_t4_trap_apply_hit(int target, int damage, int damage_type)
{
    int *shield, *hp, armor, raw_type;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS ||
        !target_alive(target)) return;
    if (target < MAX_UNITS) {
        Unit *q = &u[target];
        if (sc64_race_t4_actor_flags(q->type) & SC64_T4_ACTOR_HIDDEN) return;
        if (sc64_race_t4_unit_intrinsically_invincible(q->type) ||
            invincible_units[target] || (q->flags & 16)) return;
        shield = &q->shield; hp = &q->hp; armor = q->armor;
        if (q->owner == SIDE) armor += armor_level;
        raw_type = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
    } else {
        int slot = target - MAX_UNITS;
        Building *q;
        if (slot < 0 || slot >= MAX_BUILDINGS) return;
        q = &b[slot];
        if (sc64_race_t4_building_intrinsically_invincible(q->type) ||
            invincible_buildings[slot] || (q->flags & 16)) return;
        shield = &q->shield; hp = &q->hp;
        armor = sc64_race_building_armor(q->type);
        raw_type = q->raw_type >= 0 ? q->raw_type :
                   sc64_race_raw_building(q->type);
    }
    if (*shield > 0) {
        int absorbed = *shield < damage ? *shield : damage;
        *shield -= absorbed;
        damage -= absorbed;
    }
    if (damage <= 0) return;
    damage = race_t4_trap_hp_damage(raw_type, damage, armor, damage_type);
    *hp -= damage;
    if (*hp > 0) return;
    if (target < MAX_UNITS) {
        Unit *q = &u[target];
        if (q->owner == SIDE) supply_used -= unit_supply(q->type);
        q->alive = 0;
    } else {
        Building *q = &b[target - MAX_UNITS];
#ifndef SC64_IMPORTED_MISSION
        if (q->type == B_COMMAND && q->owner == SIDE) lost = 1;
#endif
        if (q->type == B_DEPOT && q->owner == SIDE && !q->under_construction)
            supply_cap -= 16;
        q->alive = 0;
        invalidate_path_grid();
    }
}

static int
race_t4_trap_find_target(const Building *trap, const Sc64T4TrapRule *rule)
{
    int selected = -1;
    int best = rule->range * rule->range;
    /* Slot order is the deterministic tie breaker. Unit slots precede
       building slots, matching the engine's target namespace. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        int d;
        if (!u[i].alive || !owners_hostile(trap->owner, u[i].owner) ||
            invincible_units[i] || (u[i].flags & 16) ||
            (sc64_race_t4_actor_flags(u[i].type) & SC64_T4_ACTOR_HIDDEN) ||
            sc64_race_t4_unit_intrinsically_invincible(u[i].type) ||
            sc64_race_unit_flying(u[i].type)) continue;
        d = dist2(trap->x, trap->y, u[i].x, u[i].y);
        if (d <= best) {
            if (selected < 0 || d < best) selected = i;
            if (d < best) best = d;
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int d, target = MAX_UNITS + i;
        if (!b[i].alive || !owners_hostile(trap->owner, b[i].owner) ||
            invincible_buildings[i] || (b[i].flags & 16) ||
            sc64_race_t4_building_intrinsically_invincible(b[i].type)) continue;
        d = dist2(trap->x, trap->y, b[i].x, b[i].y);
        if (d <= best && (selected < 0 || d < best)) {
            selected = target;
            best = d;
        }
    }
    return selected;
}

/* Build the source-slot-ordered candidate superset once for the trap pass.
 * Trap resolution can kill actors, but does not change owner, type, flags,
 * flight, or invincibility; the cached selector therefore rechecks alive on
 * each query while reusing only those invariant filters. */
static unsigned
race_t4_trap_build_candidates(u16 *targets)
{
    unsigned count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && owners_hostile(4, u[i].owner) &&
            !invincible_units[i] && !(u[i].flags & 16) &&
            !(sc64_race_t4_actor_flags(u[i].type) & SC64_T4_ACTOR_HIDDEN) &&
            !sc64_race_t4_unit_intrinsically_invincible(u[i].type) &&
            !sc64_race_unit_flying(u[i].type)) targets[count++] = (u16)i;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && owners_hostile(4, b[i].owner) &&
            !invincible_buildings[i] && !(b[i].flags & 16) &&
            !sc64_race_t4_building_intrinsically_invincible(b[i].type))
            targets[count++] = (u16)(MAX_UNITS + i);
    return count;
}

static int
race_t4_trap_find_candidate_target(const Building *trap,
                                   const Sc64T4TrapRule *rule,
                                   const u16 *targets, unsigned count)
{
    int selected = -1;
    int best = rule->range * rule->range;
    for (unsigned n = 0; n < count; ++n) {
        int target = targets[n];
        int x, y, dx, dy, d;
        if (!target_alive(target)) continue;
        x = target_x(target); y = target_y(target);
        dx = x - trap->x; dy = y - trap->y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx > rule->range || dy > rule->range) continue;
        d = dist2(trap->x, trap->y, x, y);
        if (d <= best && (selected < 0 || d < best)) {
            selected = target;
            best = d;
        }
    }
    return selected;
}

static void
race_t4_traps_tick(void)
{
    u16 candidates[MAX_UNITS + MAX_BUILDINGS];
    unsigned candidate_count = 0;
    int candidates_ready = 0;
    if (!race_t4_active()) return;
    for (unsigned row = 0; row < 32; ++row) {
        int slot = race_t4.doodad_slot[row];
        Building *trap;
        Sc64T4TrapRule rule;
        int target;
        if (!race_t4.doodad_enabled[row] || slot < 0 ||
            slot >= MAX_BUILDINGS || !b[slot].alive ||
            b[slot].owner != 4 || b[slot].raw_owner != 6 ||
            !sc64_race_t4_is_trap(b[slot].type) ||
            !race_t4_trap_rule(b[slot].type, &rule)) continue;
        trap = &b[slot];
        if (trap->queue_progress > 0) {
            --trap->queue_progress;
            if (trap->queue_progress > 0) continue;
        }
        if (!candidates_ready) {
            candidate_count = race_t4_trap_build_candidates(candidates);
            candidates_ready = 1;
        }
        target = race_t4_trap_find_candidate_target(trap, &rule,
                                                    candidates,
                                                    candidate_count);
        if (target < 0) continue;
        add_tracer(trap->x, trap->y, target_x(target), target_y(target), sc64_weapon_vfx_building_profile(trap->type));
        for (int hit = 0; hit < rule.factor && target_alive(target); ++hit)
            race_t4_trap_apply_hit(target, rule.damage, rule.damage_type);
        if (trap->alive) trap->queue_progress = SC64_T4_TRAP_COOLDOWN;
    }
}

#endif /* SC64_T4_SCENARIO_RUNTIME */
#endif /* SC64_T4_TRAP_ADAPTER_H */
