#ifndef SC64_T4_EFFECTS_ADAPTER_H
#define SC64_T4_EFFECTS_ADAPTER_H

#ifdef SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_runtime.h"
#include "sc64_t4_queries.h"
#include "sc64_t4_actions.h"

/* Authored adapter policy: source operands and geometry are validated before
 * any world mutation. Unsupported types, locations or modifiers fail closed. */
static int
race_t4_effect_location(const Sc64T4RuntimeState *state, unsigned id,
                        Sc64T4QueryRect *out)
{
    u32 left, top, right, bottom;
    /* MRGN IDs are 1-based; ID 0 means no location. Anywhere is ID 64. */
    if (!state || !out || !id || id > sc64_t4_scenario.location_count) return 0;
    if (id == 30 && state->location30_moved) {
        left = state->location30[0]; top = state->location30[1];
        right = state->location30[2]; bottom = state->location30[3];
    } else {
        const Sc64T4Location *loc = &sc64_t4_scenario.locations[id - 1];
        if (loc->elevation) return 0;
        left = loc->left; top = loc->top;
        right = loc->right; bottom = loc->bottom;
    }
    if (left >= right || top >= bottom || right > (u32)race_cols * 32u ||
        bottom > (u32)race_rows * 32u) return 0;
    out->left = (s32)left; out->top = (s32)top;
    out->right = (s32)right; out->bottom = (s32)bottom;
    return 1;
}

static int
race_t4_effect_area(const Sc64T4RuntimeState *state, unsigned id,
                    Sc64T4QueryRect *out)
{
    if (!out) return 0;
    /* Authored unbounded-operation sentinel. MRGN Anywhere itself is ID 64. */
    if (!id) {
        out->left = 0; out->top = 0;
        out->right = race_cols * 32; out->bottom = race_rows * 32;
        return race_cols > 0 && race_rows > 0;
    }
    return race_t4_effect_location(state, id, out);
}

static int
race_t4_effect_unit_rect(const Unit *unit, Sc64T4QueryRect *rect)
{
    int raw;
    if (!unit || !rect) return 0;
    raw = unit->raw_type >= 0 ? unit->raw_type :
          sc64_race_raw_unit(unit->type);
    return sc64_t4_query_actor_rect((unsigned)raw, unit->x * 2,
                                    unit->y * 2, rect);
}

static int
race_t4_effect_building_rect(const Building *building, Sc64T4QueryRect *rect)
{
    int left, top, right, bottom;
    if (!building || !rect ||
        !sc64_race_t4_building_bounds_source(building->type, &left, &top,
                                             &right, &bottom)) return 0;
    rect->left = building->x * 2 - left;
    rect->top = building->y * 2 - top;
    rect->right = building->x * 2 + right + 1;
    rect->bottom = building->y * 2 + bottom + 1;
    return 1;
}

static int
race_t4_effect_men_raw(int raw)
{
    /* Explicitly enumerated T4 source actor classes: the Terran combatants,
     * workers, Raynor, Firebat, Zergling, and the two neutral critters. */
    switch (raw) {
    case 0: case 1: case 2: case 3: case 7: case 15: case 20:
    case 32: case 37: case 89: case 95:
        return sc64_race_unit_from_raw((unsigned)raw) >= 0;
    default:
        return 0;
    }
}

static int
race_t4_effect_type_matches(unsigned wanted, int raw, int building)
{
    if (wanted == 229) return 1; /* Any unit, including buildings. */
    if (wanted == 230) return !building && race_t4_effect_men_raw(raw);
    return raw >= 0 && raw == (int)wanted;
}

static int
race_t4_effect_query_unit(const Sc64T4Condition *condition, unsigned owner,
                          const Unit *unit, const Sc64T4QueryRect *area,
                          u32 *count)
{
    Sc64T4QueryRect actor;
    int selected, overlap;
    int raw = unit->raw_type >= 0 ? unit->raw_type : sc64_race_raw_unit(unit->type);
    selected = sc64_t4_query_owner_matches(condition->player, owner,
                                            unit->owner, unit->raw_owner);
    if (selected < 0) return -1;
    if (!selected || !race_t4_effect_type_matches(condition->unit, raw, 0)) return 1;
    if (area->left == 0 && area->top == 0 &&
        area->right == race_cols * 32 && area->bottom == race_rows * 32) {
        ++*count;
        return 1;
    }
    if (!race_t4_effect_unit_rect(unit, &actor)) return -1;
    overlap = sc64_t4_query_rect_overlap(area, &actor);
    if (overlap) ++*count;
    return 1;
}

static int
race_t4_effect_query_building(const Sc64T4Condition *condition, unsigned owner,
                              const Building *building,
                              const Sc64T4QueryRect *area, u32 *count)
{
    Sc64T4QueryRect actor;
    int selected, raw = building->raw_type >= 0 ? building->raw_type :
                        sc64_race_raw_building(building->type);
    selected = sc64_t4_query_owner_matches(condition->player, owner,
                                            building->owner,
                                            building->raw_owner);
    if (selected < 0) return -1;
    if (!selected || !race_t4_effect_type_matches(condition->unit, raw, 1)) return 1;
    if (area->left == 0 && area->top == 0 &&
        area->right == race_cols * 32 && area->bottom == race_rows * 32) {
        ++*count;
        return 1;
    }
    if (!race_t4_effect_building_rect(building, &actor)) return -1;
    if (sc64_t4_query_rect_overlap(area, &actor)) ++*count;
    return 1;
}

static int
race_t4_trigger_count(void *context, const Sc64T4Condition *condition,
                      unsigned owner, u32 *count)
{
    Sc64T4RuntimeState *state = context;
    Sc64T4QueryRect area;
    u32 result = 0;
    int type_known;
    if (!state || !condition || !count || owner >= 8 ||
        (condition->opcode != 2 && condition->opcode != 3) ||
        condition->mask || (condition->flags & ~0x12u)) return 0;
    if (sc64_t4_query_owner_matches(condition->player, owner,
                                   NEUTRAL, 11) < 0) return 0;
    if (condition->opcode == 2) {
        area.left = 0; area.top = 0;
        area.right = race_cols * 32; area.bottom = race_rows * 32;
        if (race_cols <= 0 || race_rows <= 0) return 0;
    } else if (!race_t4_effect_location(state, condition->location, &area)) {
        return 0;
    }
    type_known = condition->unit == 229 || condition->unit == 230 ||
                 sc64_race_unit_from_raw(condition->unit) >= 0 ||
                 sc64_race_building_from_raw(condition->unit) >= 0;
    if (!type_known) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int r;
        if (!u[i].alive) continue;
        r = race_t4_effect_query_unit(condition, owner, &u[i], &area, &result);
        if (r <= 0) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int r;
        if (!b[i].alive) continue;
        r = race_t4_effect_query_building(condition, owner, &b[i], &area, &result);
        if (r <= 0) return 0;
    }
    *count = result;
    return 1;
}

static int
race_t4_effect_action_owner(const Sc64T4Action *action, unsigned trigger_owner,
                            int live_owner, int raw_owner)
{
    return sc64_t4_query_owner_matches(action->player, trigger_owner,
                                       live_owner, raw_owner);
}

static int
race_t4_effect_action_match(const Sc64T4Action *action, unsigned owner,
                            int runtime_owner, int raw_owner, int raw_type,
                            int is_building, int *matches)
{
    int selected;
    if (!matches) return -1;
    selected = race_t4_effect_action_owner(action, owner, runtime_owner, raw_owner);
    if (selected < 0) return -1;
    *matches = selected && race_t4_effect_type_matches(action->unit, raw_type,
                                                        is_building);
    return 1;
}

static int
race_t4_effect_action_area(const Sc64T4RuntimeState *state,
                           const Sc64T4Action *action,
                           Sc64T4QueryRect *area)
{
    return race_t4_effect_area(state, action->location, area);
}

static int
race_t4_effect_actor_in_area(const Sc64T4Action *action, unsigned owner,
                             const Sc64T4QueryRect *area, const Unit *unit,
                             int *matches)
{
    Sc64T4QueryRect rect;
    int result = race_t4_effect_action_match(action, owner, unit->owner,
                                              unit->raw_owner,
                                              unit->raw_type >= 0 ? unit->raw_type :
                                              sc64_race_raw_unit(unit->type),
                                              0, matches);
    if (result <= 0 || !*matches) return result;
    if (!race_t4_effect_unit_rect(unit, &rect)) return -1;
    *matches = sc64_t4_query_rect_overlap(area, &rect);
    return 1;
}

static int
race_t4_effect_building_in_area(const Sc64T4Action *action, unsigned owner,
                                const Sc64T4QueryRect *area,
                                const Building *building, int *matches)
{
    Sc64T4QueryRect rect;
    int result = race_t4_effect_action_match(action, owner, building->owner,
                                              building->raw_owner,
                                              building->raw_type >= 0 ? building->raw_type :
                                              sc64_race_raw_building(building->type),
                                              1, matches);
    if (result <= 0 || !*matches) return result;
    if (!race_t4_effect_building_rect(building, &rect)) return -1;
    *matches = sc64_t4_query_rect_overlap(area, &rect);
    return 1;
}

static int
race_t4_effect_remove(const Sc64T4Action *action, unsigned owner,
                      Sc64T4RuntimeState *state, int use_area)
{
    u8 selected_units[MAX_UNITS], selected_buildings[MAX_BUILDINGS];
    Sc64T4QueryRect area;
    int remove_buildings = action->unit == 229;
    int changed = 0;
    if (sc64_t4_query_owner_matches(action->player, owner,
                                    NEUTRAL, 11) < 0) return -1;
    memset(selected_units, 0, sizeof(selected_units));
    memset(selected_buildings, 0, sizeof(selected_buildings));
    if (use_area && !race_t4_effect_action_area(state, action, &area)) return -1;
    if (action->unit != 229 && action->unit != 230 &&
        sc64_race_unit_from_raw(action->unit) < 0) return -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int match;
        int r = use_area ? race_t4_effect_actor_in_area(action, owner, &area, &u[i], &match) :
            race_t4_effect_action_match(action, owner, u[i].owner, u[i].raw_owner,
                 u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type),
                 0, &match);
        if (r <= 0) return -1;
        selected_units[i] = (u8)match;
    }
    if (remove_buildings) for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int match;
        int r = use_area ? race_t4_effect_building_in_area(action, owner, &area, &b[i], &match) :
            race_t4_effect_action_match(action, owner, b[i].owner, b[i].raw_owner,
                 b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type),
                 1, &match);
        if (r <= 0) return -1;
        selected_buildings[i] = (u8)match;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (selected_units[i]) {
        u[i].alive = 0; u[i].selected = 0; u[i].target = -1;
        u[i].order = ORDER_IDLE; changed = 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (selected_buildings[i]) {
        b[i].alive = 0; changed = 1;
        if (selected_building == i) selected_building = -1;
        /* THG2 bindings retain immutable provenance even after a trap dies.
         * T4 creates only unit-pool actors after its initial world load. */
    }
    if (changed) {
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
            int stale = 0;
            if (u[i].order == ORDER_ATTACK_TARGET && u[i].target >= 0 &&
                u[i].target < MAX_UNITS + MAX_BUILDINGS)
                stale = u[i].target < MAX_UNITS ? selected_units[u[i].target] :
                    selected_buildings[u[i].target - MAX_UNITS];
            else if ((u[i].order == ORDER_BUILD || u[i].order == ORDER_GAS) &&
                     u[i].target >= 0 && u[i].target < MAX_BUILDINGS)
                stale = selected_buildings[u[i].target];
            if (stale) { u[i].target = -1; u[i].order = ORDER_IDLE; }
        }
        supply_used = race_owner_supply(SIDE);
        supply_cap = race_owner_supply_cap(SIDE);
        target_lists_valid = 0;
        sc64_unit_bucket_active = 0;
        sc64_target_index_end();
        invalidate_path_grid();
    }
    return 1;
}

static int
race_t4_effect_invincibility(const Sc64T4RuntimeState *state,
                             const Sc64T4Action *action, unsigned owner)
{
    u8 selected[MAX_UNITS];
    Sc64T4QueryRect area;
    if ((action->modifier != 4 && action->modifier != 5) ||
        (action->unit != 15) || action->location != 9 ||
        sc64_t4_query_owner_matches(action->player, owner, NEUTRAL, 11) < 0 ||
        !race_t4_effect_action_area(state, action, &area)) return -1;
    memset(selected, 0, sizeof(selected));
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int match;
        if (race_t4_effect_actor_in_area(action, owner, &area, &u[i], &match) < 0)
            return -1;
        selected[i] = (u8)match;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (selected[i]) invincible_units[i] = action->modifier == 4;
    return 1;
}

static int
race_t4_effect_center_location(Sc64T4RuntimeState *state, unsigned id,
                               int *x, int *y)
{
    Sc64T4QueryRect rect;
    if (!x || !y || !race_t4_effect_location(state, id, &rect)) return 0;
    *x = ((rect.left + rect.right) / 2) / 2;
    *y = ((rect.top + rect.bottom) / 2) / 2;
    return 1;
}

static int
race_t4_effect_move_location(Sc64T4RuntimeState *state,
                             const Sc64T4Action *action, unsigned owner)
{
    Sc64T4QueryRect source, destination;
    int found_x = 0, found_y = 0, found = 0;
    if (action->location != 64 || action->argument != 30 ||
        action->player != 0 || action->unit != 20 || action->modifier != 0 ||
        !race_t4_effect_location(state, 64, &source) ||
        !race_t4_effect_location(state, 30, &destination)) return -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int match;
        int r = race_t4_effect_actor_in_area(action, owner, &source, &u[i], &match);
        if (r <= 0) return -1;
        if (match) { found_x = u[i].x * 2; found_y = u[i].y * 2; found = 1; break; }
    }
    if (!found) return 1; /* OpenBW's MoveLocation no-match behavior. */
    int width = destination.right - destination.left;
    int height = destination.bottom - destination.top;
    int max_x = race_cols * 32, max_y = race_rows * 32;
    if (width <= 0 || height <= 0 || width > max_x || height > max_y) return -1;
    int left = found_x - width / 2, top = found_y - height / 2;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (left + width > max_x) left = max_x - width;
    if (top + height > max_y) top = max_y - height;
    state->location30[0] = (u16)left;
    state->location30[1] = (u16)top;
    state->location30[2] = (u16)(left + width);
    state->location30[3] = (u16)(top + height);
    state->location30_moved = 1;
    return 1;
}

static int
race_t4_effect_men_destination(const Sc64T4RuntimeState *state,
                               const Sc64T4Action *action,
                               Sc64T4QueryRect *source,
                               Sc64T4QueryRect *destination)
{
    int pair = (action->location == 14 && action->argument == 15) ||
               (action->location == 1 && action->argument == 2) ||
               (action->location == 7 && action->argument == 8) ||
               (action->location == 3 && action->argument == 4);
    return pair && action->player == 13 && action->unit == 230 &&
           action->modifier == 0 &&
           race_t4_effect_area(state, action->location, source) &&
           race_t4_effect_area(state, action->argument, destination);
}

static int
race_t4_effect_fit(const Unit *unit, int center_x, int center_y,
                   const u8 *planned_slots, const u16 *planned_x,
                   const u16 *planned_y, unsigned planned_count)
{
    if (!is_traversable(center_x / 16, center_y / 16, -1)) return 0;
    if (!unit_step_clear(unit, center_x, center_y)) return 0;
    for (unsigned i = 0; i < planned_count; ++i)
        if (dist2(center_x, center_y, planned_x[i], planned_y[i]) < 36)
            return 0;
    (void)planned_slots;
    return 1;
}

/* Authored destination policy: search up to eight tiles from the destination
 * center, in deterministic Manhattan rings, using the engine's actual terrain,
 * building and unit collision check. Source native MoveUnit placement details
 * remain unaudited; failure retries atomically without moving a subset. */
static int
race_t4_effect_move_men(Sc64T4RuntimeState *state,
                        const Sc64T4Action *action, unsigned owner)
{
    u8 slots[MAX_UNITS];
    u16 xs[MAX_UNITS], ys[MAX_UNITS];
    unsigned count = 0;
    Sc64T4QueryRect source, destination;
    if (!race_t4_effect_men_destination(state, action, &source, &destination))
        return -1;
    int target_x = ((destination.left + destination.right) / 2) / 2;
    int target_y = ((destination.top + destination.bottom) / 2) / 2;
    target_x = limit(target_x, 0, WORLD_W - 1);
    target_y = limit(target_y, 0, WORLD_H - 1);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int match, r = race_t4_effect_actor_in_area(action, owner, &source,
                                                     &u[i], &match);
        if (r <= 0) return -1;
        if (!match) continue;
        int placed = 0;
        for (int radius = 0; radius <= 8 && !placed; ++radius) {
            for (int dy = -radius; dy <= radius && !placed; ++dy) {
                int dx = radius - (dy < 0 ? -dy : dy);
                int sign_count = dx ? 2 : 1;
                for (int si = 0; si < sign_count && !placed; ++si) {
                    int sx = si ? -dx : dx;
                    int x = target_x + sx * 16;
                    int y = target_y + dy * 16;
                    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
                        x * 2 < destination.left || x * 2 >= destination.right ||
                        y * 2 < destination.top || y * 2 >= destination.bottom)
                        continue;
                    if (race_t4_effect_fit(&u[i], x, y, slots, xs, ys, count)) {
                        slots[count] = (u8)i; xs[count] = (u16)x; ys[count] = (u16)y;
                        ++count; placed = 1;
                    }
                }
            }
        }
        if (!placed) return 0;
    }
    for (unsigned i = 0; i < count; ++i) {
        Unit *q = &u[slots[i]];
        /* Authored placement policy: move the actor into the bounded fit and
         * clear stale path/order state. Native callback placement is not yet
         * verified; this is a deterministic collision-safe approximation. */
        q->x = xs[i]; q->y = ys[i];
        q->order = ORDER_IDLE; q->target = -1;
        q->tx = xs[i]; q->ty = ys[i];
        q->path_x = xs[i]; q->path_y = ys[i];
        q->path_timer = q->path_retry = 0;
        sc64_unit_bucket_move(q);
        sc64_target_index_move(q);
    }
    return 1;
}

static int
race_t4_trigger_action(void *context, const Sc64T4Action *action,
                       unsigned owner)
{
    Sc64T4RuntimeState *state = context;
    int x, y;
    if (!state || !action || owner >= 8) return -1;
    if (action->flags & 2) return 1;
    switch (action->opcode) {
    case 7: case 8: case 9: case 12:
        if (owner != SIDE) return 1;
        state->message = action->opcode;
        state->message_ticks = 90;
        return 1;
    case 10:
        if (owner != SIDE) return 1;
        if (!race_t4_effect_center_location(state, action->location, &x, &y)) return -1;
        cx = x; cy = y;
        return 1;
    case 11:
        return race_t4_create_action(action, owner);
    case 22:
        if (action->unit != 229 || action->player != 17 || action->location != 0)
            return -1;
        return race_t4_effect_remove(action, owner, state, 0);
    case 24:
        if (action->unit != 101 || action->player != 0 || action->location != 0)
            return -1;
        return race_t4_effect_remove(action, owner, state, 0);
    case 25:
        if (action->location != 9 || action->player != 4 || action->unit != 15 ||
            action->modifier != 0) return -1;
        return race_t4_effect_remove(action, owner, state, 1);
    case 28:
        if (owner != SIDE) return 1;
        if (!race_t4_effect_center_location(state, action->location, &x, &y)) return -1;
        state->ping_x = (u16)x; state->ping_y = (u16)y; state->ping_ticks = 90;
        return 1;
    case 38:
        return race_t4_effect_move_location(state, action, owner);
    case 39:
        return race_t4_effect_move_men(state, action, owner);
    case 42:
        return race_t4_doodad_action(action, owner);
    case 43:
        return race_t4_effect_invincibility(state, action, owner);
    default: return -1;
    }
}

#endif /* SC64_T4_SCENARIO_RUNTIME */
#endif
