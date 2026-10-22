#ifndef SC64_SKIRMISH_ADAPTER_H
#define SC64_SKIRMISH_ADAPTER_H

/* Adapter between the deterministic, engine-independent planner and the
 * actual fixed-pool game simulation. Include at the end of game.c, after the
 * engine's unit/building/economy helpers have been defined. */

static const int skirmish_owner_ids[3] = { ENEMY, RESCUABLE, 4 };
static const int skirmish_base_x[4] = { 160, 864, 864, 160 };
static const int skirmish_base_y[4] = { 160, 864, 160, 864 };

static int
skirmish_base_index(int owner)
{
    if (owner == SIDE) return 0;
    if (owner == ENEMY) return 1;
    if (owner == RESCUABLE) return 2;
    if (owner == 4) return 3;
    return -1;
}

static int GBA_HOT
skirmish_bot_index(int owner)
{
    int i;
    for (i = 0; i < skirmish_bots && i < 3; ++i)
        if (skirmish_owner_ids[i] == owner) return i;
    return -1;
}

static int
owner_minerals(int owner)
{
    int bot = skirmish_bot_index(owner);
    return owner == SIDE ? minerals : bot >= 0 ? skirmish_ai[bot].minerals : 0;
}

static int
owner_gas(int owner)
{
    int bot = skirmish_bot_index(owner);
    return owner == SIDE ? gas : bot >= 0 ? skirmish_ai[bot].gas : 0;
}

static int
owner_supply_cap(int owner)
{
    int i, cap = 0;
    if (owner == SIDE) return supply_cap;
    if (skirmish_bot_index(owner) < 0) return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != owner) continue;
        if (b[i].type == B_COMMAND) cap += 20;
        else if (b[i].type == B_DEPOT && !b[i].under_construction) cap += 16;
    }
    return cap;
}

static void
add_owner_resources(int owner, int ore, int vespene)
{
    int bot = skirmish_bot_index(owner);
    if (owner == SIDE) {
        minerals += ore;
        gas += vespene;
    } else if (bot >= 0) {
        int value = (int)skirmish_ai[bot].minerals + ore;
        skirmish_ai[bot].minerals = value > 65535 ? 65535 : value < 0 ? 0 : value;
        value = (int)skirmish_ai[bot].gas + vespene;
        skirmish_ai[bot].gas = value > 65535 ? 65535 : value < 0 ? 0 : value;
    }
}

static int GBA_HOT
skirmish_hostile(int a, int other)
{
    int bot;
    Sc64AiTarget target;
    if (a == other || a == NEUTRAL || other == NEUTRAL) return 0;
    if ((a != SIDE && skirmish_bot_index(a) < 0) ||
        (other != SIDE && skirmish_bot_index(other) < 0)) return 0;
    bot = skirmish_bot_index(a);
    if (bot < 0) return 1; /* Human is hostile to every other live skirmish slot. */
    target.player = (unsigned char)other;
    target.team = (unsigned char)other; /* Free-for-all: each owner is its own team. */
    target.alive = 1;
    target.neutral = 0;
    target.army = 0;
    target.distance = 0;
    return sc64_ai_hostile(&skirmish_ai[bot], &target);
}

static int
skirmish_owner_alive(int owner)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) return 1;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) return 1;
    return 0;
}

static int
skirmish_unit_supply_for_owner(int owner)
{
    int i, result = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) result += unit_supply(u[i].type);
    return result;
}

static int
owner_reserved_supply(int owner)
{
    int i, j, result = skirmish_unit_supply_for_owner(owner);
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner)
        for (j = 0; j < b[i].queue_count; ++j)
            result += unit_supply(b[i].queue[(b[i].queue_head + j) % 5]);
    return result;
}

/* These two action wrappers only enqueue/assign work. The planner commit is
 * the one authoritative debit, and happens after the engine accepts it. */
static int
owner_queue_unit(int owner, int building, int type)
{
    int tail;
    if (skirmish_bot_index(owner) < 0 || !building_complete(building) ||
        b[building].owner != owner || b[building].queue_count >= 5 ||
        find_unit_slot() < 0) return 0;
    if (type == U_SCV) {
        if (b[building].type != B_COMMAND) return 0;
    } else if (type == U_MARINE || type == U_FIREBAT) {
        if (b[building].type != B_BARRACKS) return 0;
        if (type == U_FIREBAT && !has_building(owner, B_ACADEMY)) return 0;
    } else return 0;
    if (owner_minerals(owner) < unit_cost(type) || owner_gas(owner) < unit_gas_cost(type) ||
        owner_reserved_supply(owner) + unit_supply(type) > owner_supply_cap(owner)) return 0;
    tail = (b[building].queue_head + b[building].queue_count) % 5;
    b[building].queue[tail] = type;
    ++b[building].queue_count;
    return 1;
}

static int
owner_start_build(int owner, int worker, int type, int x, int y)
{
    int i, slot;
    if (skirmish_bot_index(owner) < 0 || worker < 0 || worker >= MAX_UNITS ||
        !u[worker].alive || u[worker].owner != owner || u[worker].type != U_SCV ||
        type != B_BARRACKS || !has_building(owner, B_DEPOT) || !can_place(type, x, y))
        return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return 0;
    if (owner_minerals(owner) < building_cost(type) ||
        owner_gas(owner) < building_gas_cost(type)) return 0;
    slot = find_build_slot();
    if (slot < 0) return 0;
    if (u[worker].carry > 0) {
        add_owner_resources(owner, u[worker].order == ORDER_GAS ? 0 : u[worker].carry,
                            u[worker].order == ORDER_GAS ? u[worker].carry : 0);
        u[worker].carry = 0;
    }
    slot = spawn_build(owner, type, x, y, 1);
    if (slot < 0) return 0;
    u[worker].order = ORDER_BUILD;
    u[worker].target = slot;
    u[worker].tx = x;
    u[worker].ty = y;
    u[worker].path_timer = 0;
    u[worker].path_retry = 0;
    return 1;
}

static void
owner_issue_order(int owner, int order, int x, int y)
{
    int i, tx, ty;
    if (skirmish_bot_index(owner) < 0) return;
    for (i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != owner || u[i].type == U_SCV || u[i].type == U_MEDIC)
            continue;
        tx = x;
        ty = y;
        if (u[i].order == order && u[i].tx == tx && u[i].ty == ty) continue;
        u[i].order = order;
        u[i].tx = tx;
        u[i].ty = ty;
        u[i].target = -1;
        u[i].path_timer = 0;
        u[i].path_retry = 0;
    }
}

static int
skirmish_find_worker(int owner, int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff, d;
    for (i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != owner || u[i].type != U_SCV) continue;
        if (u[i].order == ORDER_BUILD) continue;
        d = dist2(x, y, u[i].x, u[i].y);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

static int
skirmish_barracks_site(int owner, int *x, int *y)
{
    int base = skirmish_base_index(owner);
    static const int dx[6] = {-48, 48, 0, 0, -48, 48};
    static const int dy[6] = {0, 0, -48, 48, -48, -48};
    int i;
    if (base < 0) return 0;
    for (i = 0; i < 6; ++i) {
        int px = skirmish_base_x[base] + dx[i];
        int py = skirmish_base_y[base] + dy[i];
        if (can_place(B_BARRACKS, px, py)) { *x = px; *y = py; return 1; }
    }
    return 0;
}

static int
skirmish_target_point(int owner, int *x, int *y, int *distance)
{
    int i, best = 0x7fffffff, d, sx, sy, found = 0;
    int base = skirmish_base_index(owner);
    int cc = owner_command_center(owner);
    if (base < 0) return 0;
    sx = cc >= 0 ? b[cc].x : skirmish_base_x[base];
    sy = cc >= 0 ? b[cc].y : skirmish_base_y[base];
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) {
        d = dist2(sx, sy, u[i].x, u[i].y);
        if (d < best) { best = d; *x = u[i].x; *y = u[i].y; found = 1; }
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) {
        d = dist2(sx, sy, b[i].x, b[i].y);
        if (d < best) { best = d; *x = b[i].x; *y = b[i].y; found = 1; }
    }
    if (found && distance) *distance = FAST_ABS(*x - sx) / 16 + FAST_ABS(*y - sy) / 16;
    return found;
}

static void
skirmish_sync_ai(int index)
{
    Sc64AiState *ai = &skirmish_ai[index];
    int owner = skirmish_owner_ids[index], i, q, army = 0, workers = 0;
    int queued_workers = 0, queued_army = 0, mining = 0, gas_workers = 0;
    ai->active = skirmish_owner_alive(owner);
    ai->has_townhall = owner_command_center(owner) >= 0;
    ai->supply_cap = owner_supply_cap(owner);
    ai->barracks = 0;
    ai->queued_buildings = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) {
        if (u[i].type == U_SCV) {
            ++workers;
            if (u[i].order == ORDER_GAS) ++gas_workers;
            else if (u[i].order == ORDER_MINE) ++mining;
        } else if (u[i].type != U_MEDIC) ++army;
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) {
        if (b[i].type == B_BARRACKS) {
            if (!b[i].under_construction) ai->barracks = 1;
            else ++ai->queued_buildings;
        }
        for (q = 0; q < b[i].queue_count; ++q) {
            int type = b[i].queue[(b[i].queue_head + q) % 5];
            if (type == U_SCV) ++queued_workers;
            else ++queued_army;
        }
    }
    ai->workers = workers;
    ai->army = army;
    ai->supply_used = skirmish_unit_supply_for_owner(owner);
    ai->queued_workers = queued_workers;
    ai->queued_army = queued_army;
    ai->workers_mining = mining + (workers - mining - gas_workers);
    ai->workers_gas = gas_workers;
    ai->desired_workers = 8;
    ai->desired_gas_workers = 0;
    ai->attack_threshold = 8;
}

static void
skirmish_make_world(int index, Sc64AiWorld *world)
{
    int i, owner, self = skirmish_owner_ids[index];
    int sx = 0, sy = 0, army_count = 0, cc;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == self && u[i].type != U_SCV && u[i].type != U_MEDIC) {
            sx += u[i].x;
            sy += u[i].y;
            ++army_count;
        }
    if (army_count) {
        sx /= army_count;
        sy /= army_count;
    } else {
        cc = owner_command_center(self);
        if (cc >= 0) { sx = b[cc].x; sy = b[cc].y; }
        else {
            int base = skirmish_base_index(self);
            if (base < 0) base = 0;
            sx = skirmish_base_x[base];
            sy = skirmish_base_y[base];
        }
    }
    world->count = 0;
    for (i = 0; i <= 4; ++i) {
        int x = 0, y = 0, army = 0, j;
        if (i != SIDE && i != ENEMY && i != RESCUABLE && i != 4) continue;
        owner = i;
        if (!skirmish_hostile(skirmish_owner_ids[index], owner) || !skirmish_owner_alive(owner))
            continue;
        for (j = 0; j < MAX_UNITS; ++j)
            if (u[j].alive && u[j].owner == owner && u[j].type != U_SCV && u[j].type != U_MEDIC) ++army;
        if (!skirmish_target_point(owner, &x, &y, 0)) continue;
        world->targets[world->count].player = (unsigned char)owner;
        world->targets[world->count].team = (unsigned char)owner;
        world->targets[world->count].alive = 1;
        world->targets[world->count].neutral = 0;
        world->targets[world->count].army = (unsigned short)army;
        world->targets[world->count].distance = (unsigned short)(FAST_ABS(x - sx) / 16 + FAST_ABS(y - sy) / 16);
        ++world->count;
    }
}

static void
skirmish_execute_intent(int index, const Sc64AiIntent *intent)
{
    Sc64AiState *ai = &skirmish_ai[index];
    int owner = skirmish_owner_ids[index];
    if (intent->kind == SC64_AI_INTENT_MINE) {
        int accepted = 1, i, seen_gas = 0;
        if ((int)intent->amount + intent->workers_gas != ai->workers) return;
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner && u[i].type == U_SCV) {
            if (seen_gas < intent->workers_gas) {
                int refinery = -1, j;
                for (j = 0; j < MAX_BUILDINGS; ++j)
                    if (building_complete(j) && b[j].owner == owner && b[j].type == B_REFINERY) { refinery = j; break; }
                if (refinery >= 0) { u[i].order = ORDER_GAS; u[i].target = refinery; ++seen_gas; }
                else accepted = 0;
            } else if (u[i].order == ORDER_GAS) {
                u[i].order = ORDER_IDLE;
                u[i].target = -1;
            }
        }
        if (accepted) sc64_ai_commit(ai, intent);
    } else if (intent->kind == SC64_AI_INTENT_BUILD && intent->type == SC64_AI_BUILD_BARRACKS) {
        int x, y, worker;
        if (!skirmish_barracks_site(owner, &x, &y)) return;
        worker = skirmish_find_worker(owner, x, y);
        if (worker >= 0 && owner_start_build(owner, worker, B_BARRACKS, x, y))
            sc64_ai_commit(ai, intent);
    } else if (intent->kind == SC64_AI_INTENT_TRAIN) {
        int building = owner_command_center(owner), type = U_SCV;
        if (intent->type == SC64_AI_TRAIN_SOLDIER || intent->type == SC64_AI_TRAIN_HEAVY) {
            building = -1;
            type = intent->type == SC64_AI_TRAIN_HEAVY ? U_FIREBAT : U_MARINE;
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (building_complete(i) && b[i].owner == owner && b[i].type == B_BARRACKS) { building = i; break; }
        }
        if (building >= 0 && owner_queue_unit(owner, building, type))
            sc64_ai_commit(ai, intent);
    } else if (intent->kind == SC64_AI_INTENT_ATTACK) {
        int x, y, distance;
        if (skirmish_target_point(intent->target_player, &x, &y, &distance))
            owner_issue_order(owner, ORDER_ATTACK_MOVE, x, y);
    }
}

static void
skirmish_init(void)
{
    static const int mineral_dx[4] = {56, 42, 56, 42};
    static const int mineral_dy[4] = {24, 40, 8, 20};
    int i, j, owner;
    memset(skirmish_ai, 0, sizeof(skirmish_ai));
    skirmish_bots = skirmish_bots < 1 ? 1 : skirmish_bots > 3 ? 3 : skirmish_bots;
    geyser_count = 4;
    geysers[0].x = 224; geysers[0].y = 160;
    geysers[1].x = 800; geysers[1].y = 864;
    geysers[2].x = 864; geysers[2].y = 224;
    geysers[3].x = 160; geysers[3].y = 800;
    for (i = 0; i < MAX_MINERALS; ++i) {
        int base = i / 4, patch = i % 4;
        int sx = base % 2 == 0 ? -1 : 1;
        int sy = base == 0 || base == 2 ? 1 : -1;
        m[i].x = skirmish_base_x[base] + sx * mineral_dx[patch];
        m[i].y = skirmish_base_y[base] + sy * mineral_dy[patch];
        m[i].amount = 1200;
#ifdef SC64_IMPORTED_MISSION
        m[i].style = SC64_RAW_MINERAL_FIELD_1 + patch % 3;
#endif
    }
#ifdef SC64_IMPORTED_MISSION
    skirmish_ground_tile = 0;
    for (i = 0; i < MAP_CELLS; ++i)
        if (!sc64_terrain_blocked[i]) {
            skirmish_ground_tile = sc64_terrain_tile_ids[i];
            break;
        }
#endif
    minerals = 200;
    gas = 0;
    supply_used = 0;
    supply_cap = 36;
    won = lost = 0;
    screen = SCREEN_PLAY;
    selected_building = -1;
    cx = skirmish_base_x[0]; cy = skirmish_base_y[0];
    camx = camy = 0;
    for (i = 0; i < 1 + skirmish_bots; ++i) {
        owner = i == 0 ? SIDE : i == 1 ? ENEMY : i == 2 ? RESCUABLE : 4;
        spawn_build(owner, B_COMMAND, skirmish_base_x[i], skirmish_base_y[i], 0);
        spawn_build(owner, B_DEPOT, skirmish_base_x[i] + (i % 2 ? -48 : 48), skirmish_base_y[i], 0);
        if (owner == SIDE) supply_cap = 36;
        for (j = 0; j < 4; ++j) {
            int x = skirmish_base_x[i] + (i % 2 == 0 ? -1 : 1) * mineral_dx[j];
            int y = skirmish_base_y[i] + (i == 0 || i == 2 ? 1 : -1) * mineral_dy[j];
            /* Mineral records are installed above; each worker begins nearby. */
            spawn_unit(owner, U_SCV, x + 10, y + 10);
        }
    }
    for (i = 0; i < skirmish_bots; ++i) {
        owner = skirmish_owner_ids[i];
        sc64_ai_init(&skirmish_ai[i], (unsigned char)owner, (unsigned char)owner, 200, 0, 4, 36);
        skirmish_ai[i].desired_workers = 8;
        skirmish_ai[i].attack_threshold = 8;
    }
    frame = 0;
    enemy_timer = 0;
    wave = 0;
    invalidate_path_grid();
    update_exploration();
}

static void
skirmish_update(void)
{
    int i;
    if (frame % 60) return;
    for (i = 0; i < skirmish_bots; ++i) {
        Sc64AiWorld world;
        Sc64AiIntent intents[SC64_AI_MAX_INTENTS];
        unsigned char count, j;
        skirmish_sync_ai(i);
        if (!skirmish_ai[i].active) continue;
        skirmish_make_world(i, &world);
        count = sc64_ai_plan(&skirmish_ai[i], &world, intents);
        for (j = 0; j < count; ++j) skirmish_execute_intent(i, &intents[j]);
    }
}

static void
skirmish_outcome(void)
{
    int owner, found_enemy = 0;
    if (!skirmish_owner_alive(SIDE)) { lost = 1; return; }
    for (owner = 0; owner <= 4; ++owner)
        if (skirmish_hostile(SIDE, owner) && skirmish_owner_alive(owner)) found_enemy = 1;
    if (!found_enemy) won = 1;
}

#endif /* SC64_SKIRMISH_ADAPTER_H */
