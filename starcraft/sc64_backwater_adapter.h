#ifndef SC64_BACKWATER_ADAPTER_H
#define SC64_BACKWATER_ADAPTER_H

/* Adapter from the fixed CHK trigger interpreter to the small RTS engine.
 * Region rectangles come only from the private generated 008/00B header. */
static int
bw_runtime_owner(int raw_owner)
{
    if (raw_owner == 1 || raw_owner == 13) return SIDE;
    if (raw_owner == 2) return NEUTRAL;
    if (raw_owner == 3) return RESCUABLE;
    if (raw_owner == 4) return ENEMY;
    return -1;
}

static int
bw_owner_matches(int selector, int owner)
{
    if (selector == 17) return owner >= SIDE && owner <= RESCUABLE;
    if (selector == 18) return owner == SIDE || owner == NEUTRAL || owner == RESCUABLE;
    if (selector == 19) return owner == ENEMY;
    return bw_runtime_owner(selector) == owner;
}

static int
bw_location_bounds(int location, int *left, int *top, int *right, int *bottom)
{
    const Sc64ImportedLocation *r;
    if (location < 1 || location > SC64_LOCATION_COUNT) return 0;
    r = &sc64_locations[location];
    *left = (int)(r->left / 2u);
    *top = (int)(r->top / 2u);
    *right = (int)(r->right / 2u);
    *bottom = (int)(r->bottom / 2u);
    return *right >= *left && *bottom >= *top;
}

static int
bw_inside(int x, int y, int location)
{
    int left, top, right, bottom;
    if (location == SC64_BW_LOC_ANYWHERE) return 1;
    if (!bw_location_bounds(location, &left, &top, &right, &bottom)) return 0;
    return x >= left && x <= right && y >= top && y <= bottom;
}

static int
bw_building_matches_type(int raw_type, int type)
{
    if (type == SC64_BW_ANY_UNIT || type == SC64_BW_BUILDINGS) return 1;
    if (type == SC64_BW_FACTORIES)
        return raw_type == 106 || raw_type == 111 || raw_type == 112 ||
               raw_type == 130 || raw_type == 143;
    {
        int actual = imported_building_type(raw_type);
        int requested = imported_building_type(type);
        return actual >= 0 && requested >= 0 && actual == requested;
    }
}

static int
bw_unit_matches_type(int raw_type, int type)
{
    int game_type;
    if (type == SC64_BW_ANY_UNIT || type == SC64_BW_MEN) return 1;
    if (type == SC64_BW_BUILDINGS || type == SC64_BW_FACTORIES) return 0;
    game_type = imported_unit_type(raw_type);
    return game_type >= 0 && game_type == imported_unit_type(type);
}

static int
bw_count(void *context, int player, int type, int location)
{
    int i, count = 0, owners = 0;
    int unit_kind = 0, building_kind = 0, unit_type = -1, building_type = -1;
    int left = 0, top = 0, right = WORLD_W - 1, bottom = WORLD_H - 1;
    int any_location = location == SC64_BW_LOC_ANYWHERE;
    (void)context;
    if (player == SC64_BW_ALL_PLAYERS)
        owners = (1 << SIDE) | (1 << NEUTRAL) | (1 << RESCUABLE) | (1 << ENEMY);
    else if (player == SC64_BW_FORCE_1)
        owners = (1 << SIDE) | (1 << NEUTRAL) | (1 << RESCUABLE);
    else if (player == SC64_BW_FORCE_2) owners = 1 << ENEMY;
    else {
        int owner = bw_runtime_owner(player);
        if (owner >= 0) owners = 1 << owner;
    }
    if (!owners) return 0;
    if (!any_location && !bw_location_bounds(location, &left, &top, &right, &bottom))
        return 0;
    if (type == SC64_BW_ANY_UNIT) {
        unit_kind = building_kind = 1;
    } else if (type == SC64_BW_MEN) {
        unit_kind = 1;
    } else if (type == SC64_BW_BUILDINGS) {
        building_kind = 1;
    } else if (type == SC64_BW_FACTORIES) {
        building_kind = 2;
    } else {
        unit_type = imported_unit_type(type);
        building_type = imported_building_type(type);
        unit_kind = unit_type >= 0;
        building_kind = building_type >= 0;
    }
    if (!unit_kind && !building_kind) return 0;
    if (unit_kind) for (i = 0; i < MAX_UNITS; i++) {
        Unit *q = &u[i];
        if (q->alive && (owners & (1 << q->owner)) &&
            (any_location || (q->x >= left && q->x <= right &&
                              q->y >= top && q->y <= bottom)) &&
            (unit_type < 0 || q->type == unit_type)) count++;
    }
    if (building_kind) for (i = 0; i < MAX_BUILDINGS; i++) {
        Building *q = &b[i];
        int matches = building_kind == 1 ||
            (q->type == B_COMMAND || q->type == B_BARRACKS ||
             q->type == B_ACADEMY || q->type == B_INFESTED_COMMAND ||
             q->type == B_CREEP_COLONY);
        if (q->alive && matches && (owners & (1 << q->owner)) &&
            (any_location || (q->x >= left && q->x <= right &&
                              q->y >= top && q->y <= bottom)) &&
            (building_type < 0 || q->type == building_type)) count++;
    }
    return count;
}

static int
bw_spawn(void *context, int player, int type, int location, int amount)
{
    int i, made = 0, owner = bw_runtime_owner(player);
    int left, top, right, bottom;
    (void)context;
    if (type == 101) {
        if (!bw_location_bounds(location, &left, &top, &right, &bottom)) return 0;
        for (i = top / 16; i <= bottom / 16 && i < MAP_ROWS; i++) {
            int x;
            for (x = left / 16; x <= right / 16 && x < MAP_COLS; x++)
                SC64_GRID_SET(explored, x + i * MAP_COLS, 1);
        }
        return amount;
    }
    if (owner < 0 || !bw_location_bounds(location, &left, &top, &right, &bottom)) return 0;
    for (i = 0; i < amount; i++) {
        int x = (left + right) / 2 + (i - (amount - 1) / 2) * 5;
        int y = (top + bottom) / 2;
        int game_type = imported_unit_type(type);
        if (game_type >= 0) {
            if (spawn_unit(owner, game_type, x, y) < 0) break;
            made++;
            continue;
        }
        game_type = imported_building_type(type);
        if (game_type >= 0) {
            if (spawn_build(owner, game_type, x, y, 0) < 0) break;
            made++;
        }
    }
    return made;
}

static void
bw_kill(void *context, int player, int type, int location)
{
    int i, owner = bw_runtime_owner(player);
    (void)context;
    if (type == 101) return; /* Map Revealer is implemented as explored cells. */
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == owner &&
            bw_unit_matches_type(u[i].type == U_RAYNOR ? 19 :
                                 u[i].type == U_MARINE ? 0 :
                                 u[i].type == U_SCV ? 7 :
                                 u[i].type == U_FIREBAT ? 32 : -1, type) &&
            bw_inside(u[i].x, u[i].y, location)) {
            if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
            u[i].alive = 0;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == owner &&
            bw_building_matches_type(b[i].type == B_COMMAND ? 106 :
                                     b[i].type == B_BARRACKS ? 111 :
                                     b[i].type == B_INFESTED_COMMAND ? 130 : -1,
                                     type) &&
            bw_inside(b[i].x, b[i].y, location)) b[i].alive = 0;
}

static void
bw_give(void *context, int from, int to, int type, int location)
{
    int i, old_owner = bw_runtime_owner(from), new_owner = bw_runtime_owner(to);
    (void)context;
    if (old_owner < 0 || new_owner < 0) return;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == old_owner &&
            bw_unit_matches_type(u[i].type == U_RAYNOR ? 19 :
                                 u[i].type == U_MARINE ? 0 :
                                 u[i].type == U_SCV ? 7 :
                                 u[i].type == U_FIREBAT ? 32 :
                                 u[i].type == U_ZERGLING ? 37 :
                                 u[i].type == U_HYDRALISK ? 38 : -1, type) &&
            bw_inside(u[i].x, u[i].y, location)) {
            if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
            u[i].owner = new_owner;
            if (new_owner == SIDE) supply_used += unit_supply(u[i].type);
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == old_owner &&
            bw_building_matches_type(b[i].type == B_COMMAND ? 106 :
                                     b[i].type == B_DEPOT ? 109 :
                                     b[i].type == B_BARRACKS ? 111 :
                                     b[i].type == B_BUNKER ? 125 :
                                     b[i].type == B_INFESTED_COMMAND ? 130 :
                                     b[i].type == B_CREEP_COLONY ? 143 : -1,
                                     type) &&
            bw_inside(b[i].x, b[i].y, location)) {
            b[i].owner = new_owner;
            if (new_owner == SIDE && b[i].type == B_COMMAND) supply_cap += 20;
            if (old_owner == SIDE && b[i].type == B_COMMAND) supply_cap -= 20;
            if (new_owner == SIDE && b[i].type == B_DEPOT && !b[i].under_construction)
                supply_cap += 16;
            if (old_owner == SIDE && b[i].type == B_DEPOT && !b[i].under_construction)
                supply_cap -= 16;
        }
}

static void
bw_resources(void *context, int ore, int gas_amount)
{
    (void)context;
    minerals = ore;
    gas = gas_amount;
#ifdef SC64_UNIFIED
    race_minerals[SIDE]=ore;race_gas[SIDE]=gas_amount;
#endif
}

static void
bw_invincible(void *context, int player, int type, int location, int on)
{
    int i, owner = bw_runtime_owner(player);
    (void)context;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && bw_owner_matches(player, u[i].owner) &&
            bw_unit_matches_type(u[i].type == U_RAYNOR ? 19 : -1, type) &&
            bw_inside(u[i].x, u[i].y, location)) invincible_units[i] = (u8)!!on;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && bw_owner_matches(player, b[i].owner) &&
            bw_building_matches_type(b[i].type == B_COMMAND ? 106 : -1, type) &&
            bw_inside(b[i].x, b[i].y, location)) invincible_buildings[i] = (u8)!!on;
    (void)owner;
}

static void
bw_center(void *context, int location)
{
    int left, top, right, bottom, i;
    (void)context;
    /* TRIG 14 moves its temporary region onto Raynor before CenterView. */
    if (location == SC64_BW_LOC_RAYNOR) {
        for (i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].type == U_RAYNOR) {
                cx = u[i].x;
                cy = u[i].y;
                return;
            }
    }
    if (bw_location_bounds(location, &left, &top, &right, &bottom)) {
        cx = (left + right) / 2;
        cy = (top + bottom) / 2;
    }
}

static void
bw_result(void *context, int victory)
{
    (void)context;
    if (victory) { won = 1; lost = 0; }
    else { lost = 1; won = 0; }
}

static void
bw_message(void *context, int trigger, int action)
{
    (void)context;
    backwater_message_trigger = trigger;
    backwater_message_action = action;
    backwater_message_ticks = 180;
}

static Sc64BackwaterWorld
bw_world(void)
{
    Sc64BackwaterWorld w;
    w.context = 0;
    w.count = bw_count;
    w.spawn = bw_spawn;
    w.kill = bw_kill;
    w.give = bw_give;
    w.resources = bw_resources;
    w.invincible = bw_invincible;
    w.center = bw_center;
    w.result = bw_result;
    w.message = bw_message;
    return w;
}

static int
bw_near_side(int x, int y)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == SIDE &&
            dist2(u[i].x, u[i].y, x, y) <= 32 * 32) return 1;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == SIDE &&
            dist2(b[i].x, b[i].y, x, y) <= 32 * 32) return 1;
    return 0;
}

static void
bw_auto_rescue(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && (u[i].owner == NEUTRAL || u[i].owner == RESCUABLE) &&
            bw_near_side(u[i].x, u[i].y)) {
            int old = u[i].owner;
            u[i].owner = SIDE;
            supply_used += unit_supply(u[i].type);
            if (old == NEUTRAL) u[i].selected = 0;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == RESCUABLE &&
            bw_near_side(b[i].x, b[i].y)) {
            b[i].owner = SIDE;
            if (b[i].type == B_COMMAND) supply_cap += 20;
            if (b[i].type == B_DEPOT && !b[i].under_construction) supply_cap += 16;
        }
}

static void
backwater_init(void)
{
    Sc64BackwaterWorld w = bw_world();
    int i;
    for (i = 0; i < MAX_UNITS; i++) invincible_units[i] = 0;
    for (i = 0; i < MAX_BUILDINGS; i++) invincible_buildings[i] = 0;
    backwater_message_trigger = backwater_message_action = backwater_message_ticks = 0;
    sc64_backwater_init(&backwater, &w);
}

static void
backwater_update(void)
{
    Sc64BackwaterWorld w = bw_world();
    u8 outcome = sc64_backwater_outcome(&backwater);
    /* Completed campaign saves retain their Backwater state. Reassert the
     * result after load_game clears the transient won/lost presentation flags. */
    if (outcome != SC64_BACKWATER_RUNNING) {
        bw_result(0, outcome == SC64_BACKWATER_WON);
        if (screen != SCREEN_RESULT) {
            reset_selection_gestures();
            screen = SCREEN_RESULT;
            result_timer = 0;
        }
        return;
    }
    bw_auto_rescue();
    sc64_backwater_update(&backwater, &w, (u32)frame);
    if (backwater_message_ticks > 0) backwater_message_ticks--;
}

#endif
