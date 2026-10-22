#ifndef SC64_T5_WORLD_ADAPTER_H
#define SC64_T5_WORLD_ADAPTER_H

#if defined(SC64_T5_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED)

#include "sc64_t5_ai.h"

/* These are explicit bounded GBA fallbacks for opaque source script bodies.
 * The ROM accepts the Rscu script for controller-state 7, but no ownership
 * conversion body was verified. The policies below are therefore recorded as
 * approximations and never described as native script execution.
 */
enum {
    SC64_T5_AUTHORED_RESCUE_HALF_EXTENT = 32, /* 64 source pixels / 2 */
    SC64_T5_AUTHORED_AI_PERIOD = 60,
    SC64_T5_AUTHORED_ATTACK_PERIOD = 5400,
    SC64_T5_AUTHORED_QUEUE_LIMIT = 2,
    SC64_T5_AUTHORED_SCV_LIMIT = 7
};

static int
sc64_t5_world_runtime_owner(unsigned source_owner)
{
    switch (source_owner) {
    case 0: return SIDE;
    case 2: case 3: return RESCUABLE;
    case 6: return ENEMY;
    default: return -1;
    }
}

static int
sc64_t5_in_rect(int x, int y, const Sc64T4Location *area)
{
    if (!area || area->left >= area->right || area->top >= area->bottom) return 0;
    return x >= (int)(area->left / 2u) && x < (int)(area->right / 2u) &&
           y >= (int)(area->top / 2u) && y < (int)(area->bottom / 2u);
}

static void
sc64_t5_convert_rescue_owner3(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == 3 && u[i].owner == NEUTRAL)
            u[i].owner = RESCUABLE;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == 3 && b[i].owner == NEUTRAL)
            b[i].owner = RESCUABLE;
}

static void
sc64_t5_transfer_unit(int slot)
{
    Unit *q = &u[slot];
    q->owner = SIDE; q->selected = 0; q->order = ORDER_IDLE;
    q->target = -1; q->tx = q->x; q->ty = q->y;
    q->path_timer = q->path_retry = 0;
    q->path_x = q->x; q->path_y = q->y;
    q->weapon = weapons_level;
}

static int
sc64_t5_transfer_rescue_actor(int unit_slot, int building_slot)
{
    int raw_owner, group_transfer = 0;
    if (unit_slot >= 0) {
        Unit *q;
        if (unit_slot >= MAX_UNITS) return 0;
        q = &u[unit_slot];
        if (!q->alive || SC64_TRANSPORT_LOADED(q) || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 3)) return 0;
        raw_owner = q->raw_owner;
    } else {
        Building *q;
        if (building_slot < 0 || building_slot >= MAX_BUILDINGS) return 0;
        q = &b[building_slot];
        if (!q->alive || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 3)) return 0;
        raw_owner = q->raw_owner;
        /* The group-transfer hook is tied to the rescued actor itself being
         * a completed Command Center. A nearby human depot is not sufficient. */
        group_transfer = building_complete(building_slot) && q->type == B_COMMAND;
    }

    if (group_transfer) {
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == RESCUABLE && u[i].raw_owner == raw_owner)
                sc64_t5_transfer_unit(i);
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == RESCUABLE && b[i].raw_owner == raw_owner)
                b[i].owner = SIDE;
    } else if (unit_slot >= 0) {
        sc64_t5_transfer_unit(unit_slot);
    } else {
        b[building_slot].owner = SIDE;
    }

    /* Ownership changes can invalidate hostile target references and cached
     * owner lists. Mining/resource targets are deliberately left untouched. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int target_owner;
        if (!q->alive || q->order != ORDER_ATTACK_TARGET || q->target < 0 ||
            q->target >= MAX_UNITS + MAX_BUILDINGS) continue;
        if (q->target < MAX_UNITS) {
            if (!u[q->target].alive) continue;
            target_owner = u[q->target].owner;
        } else {
            int bi = q->target - MAX_UNITS;
            if (!b[bi].alive) continue;
            target_owner = b[bi].owner;
        }
        if (!owners_hostile(q->owner, target_owner)) {
            q->order = ORDER_IDLE; q->target = -1;
            q->tx = q->x; q->ty = q->y;
        }
    }
    target_lists_valid = 0;
    sc64_target_index_end();
    return 1;
}

static int
sc64_t5_world_contact_candidate(int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE &&
            FAST_ABS(x - u[i].x) <= SC64_T5_AUTHORED_RESCUE_HALF_EXTENT &&
            FAST_ABS(y - u[i].y) <= SC64_T5_AUTHORED_RESCUE_HALF_EXTENT)
            return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            FAST_ABS(x - b[i].x) <= SC64_T5_AUTHORED_RESCUE_HALF_EXTENT &&
            FAST_ABS(y - b[i].y) <= SC64_T5_AUTHORED_RESCUE_HALF_EXTENT)
            return 1;
    return 0;
}

static void
sc64_t5_rescue_contact_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive || SC64_TRANSPORT_LOADED(q) || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 3)) continue;
        if (sc64_t5_world_contact_candidate(q->x, q->y))
            (void)sc64_t5_transfer_rescue_actor(i, -1);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!q->alive || q->owner != RESCUABLE ||
            (q->raw_owner != 2 && q->raw_owner != 3)) continue;
        if (sc64_t5_world_contact_candidate(q->x, q->y))
            (void)sc64_t5_transfer_rescue_actor(-1, i);
    }
}

static void
race_t5_world_tick(void)
{
    if (!race_t5_active()) return;
    sc64_t5_rescue_contact_tick();
}

static int
race_t5_area_for_action(const Sc64T4Action *action,
                        const Sc64T4Location **area)
{
    if (!action || !area || action->opcode != 16 || action->location == 0 ||
        action->location > sc64_t5_scenario.location_count) return 0;
    const Sc64T4Location *p = &sc64_t5_scenario.locations[action->location - 1];
    if (p->elevation || p->left >= p->right || p->top >= p->bottom ||
        p->right > 96u * 32u || p->bottom > 96u * 32u) return 0;
    *area = p;
    return 1;
}

static int
race_t5_ai_script_callback(void *context, unsigned trigger_owner, u32 script_id,
                           const Sc64T4Location *area, int has_area)
{
    (void)context;
    if (trigger_owner >= 8) return -1;
    switch (script_id) {
    case SC64_T5_SCRIPT_RSCU:
        if ((trigger_owner != 2 && trigger_owner != 3) || has_area || area) return -1;
        /* Authored substitute, not a native-script claim: the T5 map's raw
         * owner 3 starts neutral and the Rscu trigger makes it rescuable. */
        sc64_t5_convert_rescue_owner3();
        return 1;
    case SC64_T5_SCRIPT_TER5:
    case SC64_T5_SCRIPT_TE5H:
        if (trigger_owner != 6 || !has_area || !area) return -1;
        /* The opaque scripts enable the authored paid backend; their original
         * build/attack bytecode is not represented. */
        return 1;
    case SC64_T5_SCRIPT_ENBK: {
        int runtime_owner = sc64_t5_world_runtime_owner(trigger_owner);
        if (runtime_owner < 0 || !has_area || !area) return -1;
        /* Authored passive guard: armed nonworkers already in the area hold
         * position and return fire through the normal combat system. */
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == runtime_owner &&
                !sc64_race_is_worker(u[i].type) &&
                sc64_race_unit_damage(u[i].type) > 0 &&
                sc64_t5_in_rect(u[i].x, u[i].y, area)) {
                u[i].order = ORDER_ATTACK_MOVE;
                u[i].target = -1; u[i].tx = u[i].x; u[i].ty = u[i].y;
            }
        return 1;
    }
    case SC64_T5_SCRIPT_CLRC: {
        int runtime_owner = sc64_t5_world_runtime_owner(trigger_owner);
        if (runtime_owner < 0 || !has_area || !area) return -1;
        /* Authored clear-combat fallback: clear ordinary attack orders of the
         * current trigger owner in the source rectangle. */
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == runtime_owner &&
                (u[i].order == ORDER_ATTACK_TARGET ||
                 u[i].order == ORDER_ATTACK_MOVE) &&
                sc64_t5_in_rect(u[i].x, u[i].y, area)) {
                u[i].order = ORDER_IDLE; u[i].target = -1;
                u[i].tx = u[i].x; u[i].ty = u[i].y;
            }
        return 1;
    }
    default: return -1;
    }
}

static int
sc64_t5_find_first_side_actor(int *x, int *y)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) {
            *x = u[i].x; *y = u[i].y; return 1;
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner == SIDE) {
            *x = b[i].x; *y = b[i].y; return 1;
        }
    return 0;
}

static void
sc64_t5_assign_enemy_workers(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        int best = -1, best_distance = 0x7fffffff;
        if (!q->alive || q->owner != ENEMY || !sc64_race_is_worker(q->type) ||
            q->order != ORDER_IDLE) continue;
        for (int j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
            int distance = dist2(q->x, q->y, m[j].x, m[j].y);
            if (distance < best_distance) { best_distance = distance; best = j; }
        }
        if (best >= 0) {
            q->order = ORDER_MINE; q->target = best;
            q->carry = 0; q->work_timer = 0;
        }
    }
}

static int
sc64_t5_enemy_workers_and_queued(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY && u[i].type == U_SCV) ++count;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY)
        for (int j = 0; j < b[i].queue_count; ++j)
            if (b[i].queue[(b[i].queue_head + j) % 5] == U_SCV) ++count;
    return count;
}

static void
sc64_t5_enemy_production(void)
{
    int workers = sc64_t5_enemy_workers_and_queued();
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        int type = -1;
        if (!building_complete(i) || q->owner != ENEMY ||
            q->queue_count >= SC64_T5_AUTHORED_QUEUE_LIMIT) continue;
        if (q->type == B_COMMAND && workers < SC64_T5_AUTHORED_SCV_LIMIT)
            type = U_SCV;
        else if (q->type == B_BARRACKS) type = U_MARINE;
        else if (q->type == B_FACTORY) type = U_VULTURE;
        else if (q->type == B_STARPORT) type = U_WRAITH;
        if (type >= 0 && race_queue_unit(ENEMY, i, type) && type == U_SCV)
            ++workers;
    }
}

static void
sc64_t5_enemy_attack(void)
{
    int x, y, sent = 0;
    if (!sc64_t5_find_first_side_actor(&x, &y)) return;
    for (int i = 0; i < MAX_UNITS && sent < 2; ++i)
        if (u[i].alive && u[i].owner == ENEMY && u[i].order == ORDER_IDLE &&
            !sc64_race_is_worker(u[i].type) &&
            sc64_race_unit_damage(u[i].type) > 0) {
            u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
            u[i].tx = x; u[i].ty = y; ++sent;
        }
}

static int
race_t5_ai_backend_tick(void *context, u32 elapsed_ticks)
{
    (void)context;
    if (!(race_t5.ai.requested_scripts[6] &
          (SC64_T5_SCRIPT_BIT_TER5 | SC64_T5_SCRIPT_BIT_TE5H))) return 1;
    if (elapsed_ticks >= SC64_T5_AUTHORED_AI_PERIOD &&
        elapsed_ticks % SC64_T5_AUTHORED_AI_PERIOD == 0) {
        sc64_t5_assign_enemy_workers();
        sc64_t5_enemy_production();
    }
    if (elapsed_ticks && elapsed_ticks % SC64_T5_AUTHORED_ATTACK_PERIOD == 0)
        sc64_t5_enemy_attack();
    return 1;
}

static const Sc64T5AiOps race_t5_ai_ops = {
    race_t5_ai_script_callback,
    race_t5_ai_backend_tick
};

static int
race_t5_ai_action(const Sc64T4Action *action, unsigned owner)
{
    const Sc64T4Location *area = 0;
    if (!action) return -1;
    if (action->opcode == 16 && !race_t5_area_for_action(action, &area)) return -1;
    if (action->opcode == 15 && action->location) return -1;
    return sc64_t5_ai_action(&race_t5.ai, action, owner, area,
                             &race_t5_ai_ops, 0);
}

static int
race_t5_ai_tick(void)
{
    return sc64_t5_ai_tick(&race_t5.ai, &race_t5_ai_ops, 0);
}

#endif /* T5 runtime */
#endif
