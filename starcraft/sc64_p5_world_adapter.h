#ifndef SC64_P5_WORLD_ADAPTER_H
#define SC64_P5_WORLD_ADAPTER_H

/* P5 source operands resolved against the live fixed-size game arrays.  This
 * is a bounded authored callback, not a general CHK interpreter. */
#include "sc64_p5_adapter.h"
#include "sc64_p5_source_model.h"
#include "sc64_resource_actions.h"
#ifdef SC64_P5_COUNT_SNAPSHOT
#include "sc64_p5_count_snapshot.h"
#endif

typedef struct {
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    u8 source_force_for_slot[8];
    u32 *deaths_by_source; /* Persisted by the owning mission adapter. */
    u8 victory_requested, defeat_requested, objective_refresh_requested;
    u8 presentation_opcode;
    u16 last_string_id, last_wav_id;
    u16 center_x, center_y;
#ifdef SC64_P5_COUNT_SNAPSHOT
    Sc64P5CountSnapshot count_snapshot;
#endif
} Sc64P5WorldContext;

static int sc64_p5_world_location(unsigned id, int *left, int *top,
                                  int *right, int *bottom)
{
    const Sc64P5SourceLocation *loc;
    if (!id || id > SC64_P5_SOURCE_LOCATION_COUNT || !left || !top ||
        !right || !bottom) return 0;
    loc = &sc64_p5_source_locations[id - 1u];
    if (loc->left >= loc->right || loc->top >= loc->bottom ||
        loc->right > (unsigned)race_cols * 32u ||
        loc->bottom > (unsigned)race_rows * 32u) return 0;
    *left = (int)(loc->left / 2u); *top = (int)(loc->top / 2u);
    *right = (int)(loc->right / 2u); *bottom = (int)(loc->bottom / 2u);
    return *left < *right && *top < *bottom;
}

static int sc64_p5_world_owner_matches(const Sc64P5WorldContext *ctx,
                                        unsigned selector, int raw_owner,
                                        int live_owner)
{
    unsigned slot;
    if (!ctx || raw_owner < 0 || raw_owner >= 12) return 0;
    slot = (unsigned)raw_owner;
    if (!ctx->source_slot_active[slot]) return 0;
    if (selector <= 11) return selector == slot;
    if (selector == 12) return 0;
    /* Current Player follows the actual owner after a rescue transfer. Keep
     * raw_owner for source-slot and force selectors so provenance is retained. */
    if (selector == 13) return live_owner == SIDE;
    if (selector == 17) return slot < 8;
    if (selector >= 18 && selector <= 21)
        return slot < 8 && ctx->source_force_for_slot[slot] == selector - 18u;
    return -1;
}

static int sc64_p5_world_owner_resolve(const Sc64P5WorldContext *ctx,
                                        unsigned selector, unsigned *slot)
{
    unsigned found = 12;
    if (!ctx || !slot) return 0;
    if (selector <= 11) found = selector;
    else if (selector == 13) found = ctx->source_current_player;
    else if (selector >= 18 && selector <= 21) {
        for (unsigned i = 0; i < 8; ++i)
            if (ctx->source_slot_active[i] &&
                ctx->source_force_for_slot[i] == selector - 18u) {
                if (found != 12) return 0; /* Ambiguous selector is rejected. */
                found = i;
            }
    } else return 0; /* All Players is a selector, not one spawn owner. */
    if (found >= 12 || !ctx->source_slot_active[found]) return 0;
    *slot = found;
    return 1;
}

static int sc64_p5_world_selector_resource(const Sc64P5WorldContext *ctx,
                                            unsigned selector, u32 amount,
                                            unsigned resource, unsigned modifier)
{
    u8 visited[5] = {0, 0, 0, 0, 0};
    unsigned matched = 0;
    if (!(selector <= 11 || selector == 13 || selector == 17 ||
          (selector >= 18 && selector <= 21))) return 0;
    if (!ctx || resource > SC64_RESOURCE_BOTH || amount > 65535u ||
        (modifier != SC64_RESOURCE_MOD_SET &&
         modifier != SC64_RESOURCE_MOD_ADD &&
         modifier != SC64_RESOURCE_MOD_SUBTRACT)) return 0;
    for (unsigned source = 0; source < 12; ++source) {
        int owner, minerals_before, gas_before;
        Sc64ResourceActionState values;
        int selected = sc64_p5_world_owner_matches(ctx, selector, (int)source,
                                                    ctx->live_owner_for_source[source]);
        if (selected < 0) return 0;
        if (!selected) continue;
        ++matched;
        owner = ctx->live_owner_for_source[source];
        if (owner < SIDE || owner > 4) return 0;
        if (visited[owner]) continue;
        visited[owner] = 1;
        minerals_before = owner == SIDE ? minerals : race_minerals[owner];
        gas_before = owner == SIDE ? gas : race_gas[owner];
        if (minerals_before < 0 || gas_before < 0 ||
            minerals_before > 65535 || gas_before > 65535) return 0;
        values.current_minerals = (u32)minerals_before;
        values.current_gas = (u32)gas_before;
        values.total_minerals_gathered = 0;
        values.total_gas_gathered = 0;
        if (!sc64_resource_action_apply_one_player(&values, (u16)resource,
                                                   (u8)modifier, amount) ||
            values.current_minerals > 65535u || values.current_gas > 65535u)
            return 0;
        if (resource == SC64_RESOURCE_MINERALS || resource == SC64_RESOURCE_BOTH) {
            race_minerals[owner] = (int)values.current_minerals;
            if (owner == SIDE) minerals = (int)values.current_minerals;
        }
        if (resource == SC64_RESOURCE_GAS || resource == SC64_RESOURCE_BOTH) {
            race_gas[owner] = (int)values.current_gas;
            if (owner == SIDE) gas = (int)values.current_gas;
        }
    }
    return matched != 0;
}

static int sc64_p5_world_type_match(unsigned wanted, unsigned raw, int building)
{
    if (wanted == 229) return 1; /* CHK Any Unit includes structures. */
    if (wanted == 230) return !building && raw != 101 && raw != 218;
    if (wanted >= 228 || wanted != raw) return 0;
    return building ? sc64_race_building_from_raw((int)raw) >= 0 :
                      sc64_race_unit_from_raw((int)raw) >= 0;
}

#ifdef SC64_P5_COUNT_SNAPSHOT
static int sc64_p5_count_snapshot_row(const Sc64P5SourceCondition *c,
                                       unsigned *snapshot_index)
{
    static const u8 source_rows[6] = {0, 1, 4, 5, 7, 8};
    static const u8 expected_players[6] = {13, 17, 13, 13, 13, 13};
    static const u8 expected_units[6] = {229, 79, 79, 65, 229, 229};
    static const u8 expected_locations[6] = {0, 0, 2, 2, 4, 5};
    static const u8 expected_triggers[6] = {0, 1, 4, 4, 6, 7};
    static const u8 expected_indices[6] = {0, 0, 0, 1, 0, 0};
    if (!c || !snapshot_index) return 0;
    for (unsigned i = 0; i < 6; ++i) {
        const Sc64P5SourceCondition *row = &sc64_p5_source_conditions[source_rows[i]];
        if (c != row) continue;
        /* Authenticate all operands before allowing a cached answer. */
        if (row->player != expected_players[i] || row->unit != expected_units[i] ||
            row->location != expected_locations[i] ||
            row->opcode != (i < 2 ? 2 : 3) ||
            row->trigger != expected_triggers[i] ||
            row->index != expected_indices[i] || row->flags != 0x10 ||
            row->comparison != (i == 0 || i == 1 ? 1 : 0) ||
            row->amount != (i == 0 || i == 1 ? 0 : i == 2 || i == 4 || i == 5 ? 1 : 2) ||
            row->resource || row->mask) return 0;
        *snapshot_index = i;
        return 1;
    }
    return 0;
}

static int sc64_p5_count_snapshot_lookup(const Sc64P5WorldContext *ctx,
                                          const Sc64P5SourceCondition *c,
                                          u32 *count)
{
    unsigned index;
    if (!ctx || !count || !ctx->count_snapshot.active ||
        !sc64_p5_count_snapshot_row(c, &index)) return 0;
    *count = ctx->count_snapshot.count[index];
    return 1;
}

static int sc64_p5_count_snapshot_begin(Sc64P5WorldContext *ctx)
{
    int left[3], top[3], right[3], bottom[3];
    u32 counts[6] = {0, 0, 0, 0, 0, 0};
    if (!ctx) return 0;
    ctx->count_snapshot.active = 0;
    memset(ctx->count_snapshot.count, 0, sizeof(ctx->count_snapshot.count));
    if (!sc64_p5_world_location(2, &left[0], &top[0], &right[0], &bottom[0]) ||
        !sc64_p5_world_location(4, &left[1], &top[1], &right[1], &bottom[1]) ||
        !sc64_p5_world_location(5, &left[2], &top[2], &right[2], &bottom[2]))
        return 0;
    for (unsigned category = 0; category < 2; ++category) {
        const int building = (int)category;
        const unsigned limit = building ? MAX_BUILDINGS : MAX_UNITS;
        for (unsigned i = 0; i < limit; ++i) {
            int alive, raw, raw_owner, live_owner, x, y, loaded = 0;
            int current_player, all_players, hero_type = 0, zealot_type = 0;
            if (building) {
                alive = b[i].alive;
                if (!alive) continue;
                raw = b[i].raw_type >= 0 ? b[i].raw_type :
                      sc64_race_raw_building(b[i].type);
                raw_owner = b[i].raw_owner; live_owner = b[i].owner;
                x = b[i].x; y = b[i].y;
            } else {
                alive = u[i].alive;
                if (!alive) continue;
                raw = u[i].raw_type >= 0 ? u[i].raw_type :
                      sc64_race_raw_unit(u[i].type);
                raw_owner = u[i].raw_owner; live_owner = u[i].owner;
                x = u[i].x; y = u[i].y;
#ifdef SC64_P5_SHUTTLE_ENABLED
                loaded = SC64_TRANSPORT_LOADED(&u[i]);
#endif
            }
            /* The six exact rows use only Current Player (13) and All Players
             * (17). Share the common raw-slot/active check, then resolve each
             * selector once instead of calling owner_matches six times. */
            current_player = raw_owner >= 0 && raw_owner < 12 &&
                ctx->source_slot_active[(unsigned)raw_owner] && live_owner == SIDE;
            all_players = raw_owner >= 0 && raw_owner < 8 &&
                ctx->source_slot_active[(unsigned)raw_owner];
            if ((!current_player && !all_players) || raw < 0) continue;
            if (current_player || all_players)
                hero_type = sc64_p5_world_type_match(79, (unsigned)raw, building);
            if (all_players && hero_type) ++counts[1];
            if (!current_player) continue;
            /* CHK Any Unit accepts every nonnegative raw type, including
             * structures and identities without a public race mapping. */
            ++counts[0];
            zealot_type = sc64_p5_world_type_match(65, (unsigned)raw, building);
            if (!loaded && (hero_type || zealot_type) &&
                x >= left[0] && x < right[0] && y >= top[0] && y < bottom[0]) {
                if (hero_type) ++counts[2];
                if (zealot_type) ++counts[3];
            }
            if (!loaded && x >= left[1] && x < right[1] &&
                y >= top[1] && y < bottom[1]) ++counts[4];
            if (!loaded && x >= left[2] && x < right[2] &&
                y >= top[2] && y < bottom[2]) ++counts[5];
        }
    }
    memcpy(ctx->count_snapshot.count, counts, sizeof(counts));
    ctx->count_snapshot.active = 1;
    return 1;
}

static void sc64_p5_count_snapshot_end(Sc64P5WorldContext *ctx)
{
    if (!ctx) return;
    ctx->count_snapshot.active = 0;
    memset(ctx->count_snapshot.count, 0, sizeof(ctx->count_snapshot.count));
}
#endif

static int sc64_p5_world_compare(unsigned comparison, u32 value, u32 amount)
{
    switch (comparison) {
    case 0: return value >= amount;
    case 1: return value <= amount;
    case 10: return value == amount; /* CHK Exactly. */
    default: return -1;
    }
}

static int sc64_p5_world_in_location(unsigned id, int x, int y)
{
    int left, top, right, bottom;
    if (!id) return 1;
    return sc64_p5_world_location(id, &left, &top, &right, &bottom) &&
           x >= left && x < right && y >= top && y < bottom;
}

/* Keep this reference implementation as the exact semantic oracle for the
 * opt-in type-first query path below. */
static int sc64_p5_world_count_full(const Sc64P5WorldContext *ctx,
                                    unsigned selector, unsigned wanted,
                                    unsigned location, u32 *out)
{
    u32 count = 0;
    if (!ctx || !out || wanted > 230 || selector > 21 ||
        (selector > 13 && selector < 17)) return 0;
    if (location) {
        int l, t, r, btm;
        if (!sc64_p5_world_location(location, &l, &t, &r, &btm)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
#ifdef SC64_P5_SHUTTLE_ENABLED
        /* Command retains cargo presence; Bring requires a world actor. */
        if (location && SC64_TRANSPORT_LOADED(&u[i])) continue;
#endif
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int owner = sc64_p5_world_owner_matches(ctx, selector, u[i].raw_owner,
                                                 u[i].owner);
        if (owner < 0) return 0;
        if (owner && raw >= 0 && sc64_p5_world_type_match(wanted, (unsigned)raw, 0) &&
            sc64_p5_world_in_location(location, u[i].x, u[i].y)) ++count;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int owner = sc64_p5_world_owner_matches(ctx, selector, b[i].raw_owner,
                                                 b[i].owner);
        if (owner < 0) return 0;
        if (owner && raw >= 0 && sc64_p5_world_type_match(wanted, (unsigned)raw, 1) &&
            sc64_p5_world_in_location(location, b[i].x, b[i].y)) ++count;
    }
    *out = count;
    return 1;
}

/* Count with cheap raw-type rejection before resolving source owner and map
 * location. This is useful for P5's narrow raw-unit queries; all selectors,
 * including Current Player rescue ownership, still use the same resolver. */
#ifdef SC64_P5_TYPE_FIRST_COUNTS
static int sc64_p5_world_count_type_first(const Sc64P5WorldContext *ctx,
                                           unsigned selector, unsigned wanted,
                                           unsigned location, u32 *out)
{
    u32 count = 0;
    if (!ctx || !out || wanted > 230 || selector > 21 ||
        (selector > 13 && selector < 17)) return 0;
    if (location) {
        int l, t, r, btm;
        if (!sc64_p5_world_location(location, &l, &t, &r, &btm)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
#ifdef SC64_P5_SHUTTLE_ENABLED
        /* Bring excludes cargo; global Command counts retain it. */
        if (location && SC64_TRANSPORT_LOADED(&u[i])) continue;
#endif
        int raw = u[i].raw_type >= 0 ? u[i].raw_type :
                  sc64_race_raw_unit(u[i].type);
        if (raw < 0 || !sc64_p5_world_type_match(wanted, (unsigned)raw, 0))
            continue;
        int owner = sc64_p5_world_owner_matches(ctx, selector, u[i].raw_owner,
                                                 u[i].owner);
        if (owner < 0) return 0;
        if (owner && sc64_p5_world_in_location(location, u[i].x, u[i].y))
            ++count;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type :
                  sc64_race_raw_building(b[i].type);
        if (raw < 0 || !sc64_p5_world_type_match(wanted, (unsigned)raw, 1))
            continue;
        int owner = sc64_p5_world_owner_matches(ctx, selector, b[i].raw_owner,
                                                 b[i].owner);
        if (owner < 0) return 0;
        if (owner && sc64_p5_world_in_location(location, b[i].x, b[i].y))
            ++count;
    }
    *out = count;
    return 1;
}
#endif

static int sc64_p5_world_count(const Sc64P5WorldContext *ctx, unsigned selector,
                                unsigned wanted, unsigned location, u32 *out)
{
#ifdef SC64_P5_TYPE_FIRST_COUNTS
    return sc64_p5_world_count_type_first(ctx, selector, wanted, location, out);
#else
    return sc64_p5_world_count_full(ctx, selector, wanted, location, out);
#endif
}

static int sc64_p5_world_condition(void *context, const Sc64P5RuntimeState *state,
                                    const Sc64P5SourceCondition *c)
{
    Sc64P5WorldContext *ctx = context;
    u32 count = 0;
    if (!ctx || !state || !c || (c->flags != 0 && c->flags != 0x10u) || c->mask)
        return -1;
    switch (c->opcode) {
    case 2: case 3: /* Command counts existing units; Bring also scopes a location. */
#ifdef SC64_P5_COUNT_SNAPSHOT
        if (!sc64_p5_count_snapshot_lookup(ctx, c, &count) &&
            !sc64_p5_world_count(ctx, c->player, c->unit,
                                 c->opcode == 3 ? c->location : 0, &count)) return -1;
#else
        if (!sc64_p5_world_count(ctx, c->player, c->unit,
                                 c->opcode == 3 ? c->location : 0, &count)) return -1;
#endif
        return sc64_p5_world_compare(c->comparison, count, c->amount);
    case 11: /* Switch, comparison 2=set and 3=cleared. */
        if (c->flags || c->resource > 255 || c->comparison < 2 || c->comparison > 3)
            return -1;
        count = sc64_p5_switch_is_set(state, c->resource) ? 1u : 0u;
        return c->comparison == 2 ? (int)count : !(int)count;
    case 12: /* Source Elapsed Time >= 0; validate the authenticated operands. */
        if (c->location || c->player || c->amount || c->unit || c->resource ||
            c->comparison != 0 || c->mask || c->flags) return -1;
        /* Elapsed time is unsigned and nonnegative, so this source predicate
         * is true at every representable frame. */
        return 1;
    case 15: /* Deaths is a separate source counter, not current occupancy. */
        if (!ctx->deaths_by_source || c->player >= 12 || c->unit != 229 ||
            c->location || c->resource) return -1;
        return sc64_p5_world_compare(c->comparison, ctx->deaths_by_source[c->player],
                                     c->amount);
    default: return -1;
    }
}

static int sc64_p5_world_set_invincible(const Sc64P5WorldContext *ctx,
                                         const Sc64P5SourceAction *a)
{
    int left, top, right, bottom;
    u8 value;
    if (!ctx || !a || a->opcode != 43 || a->group2 || a->flags != 0x10u ||
        (a->modifier != 4 && a->modifier != 5) || a->unit > 230 ||
        (a->location && !sc64_p5_world_location(a->location, &left, &top,
                                                &right, &bottom))) return 0;
    value = (u8)(a->modifier == 4);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int owner = sc64_p5_world_owner_matches(ctx, a->group1, u[i].raw_owner,
                                                 u[i].owner);
        if (owner < 0) return 0;
        if (owner && raw >= 0 && sc64_p5_world_type_match(a->unit, (unsigned)raw, 0) &&
            sc64_p5_world_in_location(a->location, u[i].x, u[i].y))
            invincible_units[i] = value;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int owner = sc64_p5_world_owner_matches(ctx, a->group1, b[i].raw_owner,
                                                 b[i].owner);
        if (owner < 0) return 0;
        if (owner && raw >= 0 && sc64_p5_world_type_match(a->unit, (unsigned)raw, 1) &&
            sc64_p5_world_in_location(a->location, b[i].x, b[i].y))
            invincible_buildings[i] = value;
    }
    return 1;
}

static int sc64_p5_world_action(void *context, const Sc64P5SourceAction *a)
{
    Sc64P5WorldContext *ctx = context;
    if (!ctx || !a) return 0;
    switch (a->opcode) {
    case 1: ctx->victory_requested = 1; return 1;
    case 2: ctx->defeat_requested = 1; return 1;
    case 3: case 4: return 1; /* Preserve/wait are scheduler-owned. */
    case 7: case 8: case 9:
        ctx->presentation_opcode = a->opcode;
        ctx->last_string_id = (u16)a->string_id;
        ctx->last_wav_id = (u16)a->wav_id;
        return 1;
    case 10: {
        int l, t, r, btm;
        if (!sc64_p5_world_location(a->location, &l, &t, &r, &btm)) return 0;
        ctx->center_x = (u16)((l + r) / 2); ctx->center_y = (u16)((t + btm) / 2);
        return 1;
    }
    case 11: { /* Create Unit; modifier zero is one. */
        unsigned source_owner, count = sc64_p5_source_create_count(a);
        int type, l, t, r, btm, owner, free_slots = 0;
        if (!count || a->group2 || a->unit >= 228 || a->flags != 0x10u ||
            !sc64_p5_world_owner_resolve(ctx, a->group1, &source_owner) ||
            !sc64_p5_world_location(a->location, &l, &t, &r, &btm)) return 0;
        type = sc64_race_unit_from_raw(a->unit);
        owner = ctx->live_owner_for_source[source_owner];
        if (type < 0 || owner < 0) return 0;
        for (int i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
        if (free_slots < (int)count) return 0;
        for (unsigned n = 0; n < count; ++n) {
            int slot = spawn_unit(owner, type, (l + r) / 2, (t + btm) / 2);
            if (slot < 0) return 0;
            u[slot].raw_type = a->unit;
            u[slot].raw_owner = (int)source_owner;
        }
        return 1;
    }
    case 12: ctx->objective_refresh_requested = 1; return 1;
    case 13: return 1; /* Scheduler applies source switch semantics. */
    case 22: { /* Kill Unit selector; raw 229 selects every unit. */
        int changed = 0;
        if (a->group2 || a->modifier || a->location || a->flags != 0x10u ||
            a->unit > 229) return 0;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
            int raw = u[i].raw_type >= 0 ? u[i].raw_type :
                      sc64_race_raw_unit(u[i].type);
            int owner = sc64_p5_world_owner_matches(ctx, a->group1,
                                                     u[i].raw_owner, u[i].owner);
            if (owner < 0) return 0;
            if (!owner || raw < 0 || !sc64_p5_world_type_match(a->unit,
                                                  (unsigned)raw, 0)) continue;
            /* CHK Kill Unit records a source death; Remove Unit does not.
             * Preserve source-slot provenance before clearing the actor. */
            if (ctx->deaths_by_source) {
                unsigned death_slot = u[i].owner == SIDE ? ctx->source_current_player :
                    (u[i].raw_owner >= 0 ? (unsigned)u[i].raw_owner : 12u);
                if (death_slot < 12 && ctx->deaths_by_source[death_slot] != 0xffffffffu)
                    ++ctx->deaths_by_source[death_slot];
            }
            u[i].alive = 0; u[i].selected = 0; u[i].target = -1;
            u[i].order = ORDER_IDLE; invincible_units[i] = 0; changed = 1;
        }
        if (changed) {
            supply_used = race_owner_supply(SIDE);
            supply_cap = race_owner_supply_cap(SIDE);
            target_lists_valid = 0; sc64_unit_bucket_active = 0;
            sc64_target_index_end();
        }
        return 1;
    }
    case 26: /* SetResources; source selection may resolve to several players. */
        if (a->location || a->flags != 0 || a->group2 > 65535u ||
            (a->modifier != SC64_RESOURCE_MOD_SET &&
             a->modifier != SC64_RESOURCE_MOD_ADD &&
             a->modifier != SC64_RESOURCE_MOD_SUBTRACT)) return 0;
        return sc64_p5_world_selector_resource(ctx, a->group1, a->group2,
                                               a->unit, a->modifier);
    case 43: return sc64_p5_world_set_invincible(ctx, a);
    default: return 0; /* Unknown/opaque actions must not silently succeed. */
    }
}

#endif
