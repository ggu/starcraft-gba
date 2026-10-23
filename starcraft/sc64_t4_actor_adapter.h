#ifndef SC64_T4_ACTOR_ADAPTER_H
#define SC64_T4_ACTOR_ADAPTER_H

/* Included after the shared race adapter. T4 source provenance stays in
 * raw_owner; live owner controls diplomacy and current-player selectors. */
static int
race_t4_actor_has_sprite(int type)
{
    unsigned flags = sc64_race_t4_actor_flags(type);
    return !flags || (flags & SC64_T4_ACTOR_DRAWABLE);
}

static int
race_t4_human_near(int raw_owner, int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && race_t4_actor_has_sprite(u[i].type) &&
            sc64_t4_rescue_contact(1, raw_owner, x * 2, y * 2,
                                  SIDE, u[i].x * 2, u[i].y * 2)) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            sc64_t4_rescue_contact(1, raw_owner, x * 2, y * 2,
                                  SIDE, b[i].x * 2, b[i].y * 2)) return 1;
    return 0;
}

static void
race_t4_transfer_unit(int slot)
{
    Unit *q = &u[slot];
    q->owner = SIDE; q->selected = 0; q->order = ORDER_IDLE;
    q->target = -1; q->tx = q->x; q->ty = q->y;
    q->path_timer = q->path_retry = 0; q->weapon = weapons_level;
}

static void
race_t4_transfer_group(int raw_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].owner == RESCUABLE && race_t4_actor_has_sprite(u[i].type) &&
            sc64_t4_rescue_transfer_candidate(1, u[i].alive, u[i].raw_owner,
                                              raw_owner, 0)) race_t4_transfer_unit(i);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].owner == RESCUABLE &&
            sc64_t4_rescue_transfer_candidate(1, b[i].alive, b[i].raw_owner,
                                              raw_owner, 0)) b[i].owner = SIDE;
}

static void
race_t4_world_tick(void)
{
    int joined = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == RESCUABLE &&
            race_t4_actor_has_sprite(u[i].type) &&
            race_t4_human_near(u[i].raw_owner, u[i].x, u[i].y)) {
            race_t4_transfer_unit(i); joined = 1;
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE &&
            race_t4_human_near(b[i].raw_owner, b[i].x, b[i].y)) {
            int depot = b[i].type == B_COMMAND || b[i].type == B_NEXUS ||
                        sc64_race_is_zerg_townhall(b[i].type);
            if (sc64_t4_rescue_depot_group(!b[i].under_construction, 1, depot))
                race_t4_transfer_group(b[i].raw_owner);
            else b[i].owner = SIDE;
            joined = 1;
        }
    if (!joined) return;
    /* Ownership conversion invalidates hostile target references. Static
     * collision is unchanged; both query indices rebuild on the next tick. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].order == ORDER_ATTACK_TARGET &&
            u[i].target >= 0 && u[i].target < MAX_UNITS + MAX_BUILDINGS) {
            int target = u[i].target;
            int owner = target < MAX_UNITS ? u[target].owner : b[target - MAX_UNITS].owner;
            if (!owners_hostile(u[i].owner, owner)) {
                u[i].order = ORDER_IDLE; u[i].target = -1;
            }
        }
    target_lists_valid = 0;
    sc64_target_index_end();
}

#endif
