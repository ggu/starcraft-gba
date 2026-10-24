#ifndef SC64_Z5_WORLD_ADAPTER_H
#define SC64_Z5_WORLD_ADAPTER_H

/* Z5's numeric trigger operands are resolved against the live engine arrays.
 * Scenario-only doodad state is supplied by the caller because THG2 is a
 * separately authenticated map section, not part of the TRIG scenario table. */
#include "sc64_z5_adapter.h"
#include "sc64_z5_state.h"
#ifdef SC64_Z5_COUNT_SNAPSHOT
#include "sc64_z5_count_snapshot.h"
#endif

enum { SC64_Z5_DOODAD_CAPACITY = SC64_Z5_UNIT_BACKED_DOODADS };

typedef struct {
    u16 raw_type;
    u16 x, y;                 /* source pixels */
    u16 raw_flags;
    u8 source_owner;
    u8 enabled;
    u8 building_slot;
} Sc64Z5DoodadState;

typedef struct {
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    Sc64Z5DoodadState *doodads;
    u8 doodad_count;
    u8 *pause_acknowledged;
    u8 presentation_pause_requested;
    u8 objective_refresh_requested;
    u8 victory_requested;
    u8 defeat_requested;
    u8 mute_speech_requested;
    u8 minimap_ping_requested;
    u16 center_x, center_y;
    u16 last_string_id, last_wav_id;
    u8 presentation_opcode;
    u8 fault;
#ifdef SC64_Z5_COUNT_SNAPSHOT
    Sc64Z5CountSnapshot count_snapshot;
#endif
} Sc64Z5WorldContext;

static int sc64_z5_world_location(unsigned id, int *left, int *top,
                                   int *right, int *bottom)
{
    const Sc64Z5SourceLocation *loc;
    if (!id || id > SC64_Z5_SOURCE_LOCATION_COUNT || !left || !top ||
        !right || !bottom) return 0;
    loc = &sc64_z5_source_locations[id - 1u];
    if (loc->left >= loc->right || loc->top >= loc->bottom ||
        loc->right > (unsigned)race_cols * 32u ||
        loc->bottom > (unsigned)race_rows * 32u || loc->flags) return 0;
    *left = (int)(loc->left / 2u); *top = (int)(loc->top / 2u);
    *right = (int)(loc->right / 2u); *bottom = (int)(loc->bottom / 2u);
    return *left < *right && *top < *bottom;
}

static int sc64_z5_world_owner_matches(const Sc64Z5WorldContext *ctx,
                                        unsigned selector, int raw_owner)
{
    unsigned slot;
    if (!ctx || raw_owner < 0 || raw_owner >= 12) return 0;
    slot = (unsigned)raw_owner;
    if (!ctx->source_slot_active[slot]) return 0;
    if (selector <= 11) return selector == slot;
    if (selector == 12) return 0; /* No Players. */
    if (selector == 13) return slot == ctx->source_current_player;
    if (selector == 17) return slot < 8;
    if (selector >= 18 && selector <= 21)
        return slot < 8 && sc64_z5_source_forces[slot] == selector - 18u;
    return -1;
}

static int sc64_z5_world_resolve_owner(const Sc64Z5WorldContext *ctx,
                                        unsigned selector, unsigned *slot)
{
    unsigned found = 12;
    if (!ctx || !slot) return 0;
    if (selector <= 11) found = selector;
    else if (selector == 13) found = ctx->source_current_player;
    else if (selector >= 18 && selector <= 21) {
        unsigned force = selector - 18u;
        for (unsigned i=0;i<8;i++) if(ctx->source_slot_active[i] &&
            sc64_z5_source_forces[i] == force) {
            if (found != 12) return 0;
            found=i;
        }
    } else return 0;
    if(found>=12 || !ctx->source_slot_active[found]) return 0;
    *slot=found;
    return 1;
}

static int sc64_z5_world_actor_rect(unsigned raw, int x, int y,
                                     int *left, int *top,
                                     int *right, int *bottom)
{
    int l, u, r, d, type;
    if (x < 0 || y < 0) return 0;
    type = sc64_race_unit_from_raw((int)raw);
    if (type < 0) return 0;
    if (!sc64_zp_actor_bounds_source(type, &l, &u, &r, &d)) {
        /* Source-pixel extents for common actors used by this scenario. */
        switch (raw) {
        case 0: case 20: l=8;u=9;r=8;d=10;break;
        case 1: l=7;u=10;r=7;d=11;break;
        case 2: case 3: l=16;u=16;r=15;d=15;break;
        case 7: l=11;u=11;r=11;d=11;break;
        case 32: l=11;u=7;r=11;d=14;break;
        case 37: l=8;u=4;r=7;d=11;break;
        case 41: l=11;u=11;r=11;d=11;break;
        case 43: l=22;u=22;r=21;d=21;break;
        case 53: l=10;u=10;r=10;d=12;break;
        default: return 0;
        }
    }
    /* Runtime coordinates use half the CHK source-pixel scale. */
    *left = x - (l + 1) / 2; *top = y - (u + 1) / 2;
    *right = x + (r + 1) / 2 + 1; *bottom = y + (d + 1) / 2 + 1;
    return 1;
}

/* Source-pixel bounds for StarCraft structures that can be present in Z5's
 * query regions. Values are from the pinned BWAPI UnitType dimensions table. */
static int sc64_z5_world_building_bounds(unsigned raw,
                                          int *left,int *up,int *right,int *down)
{
    int l,u,r,d;
    switch(raw) {
    case 106: l=58;u=41;r=58;d=41;break;
    case 109: l=38;u=22;r=38;d=26;break;
    case 110: l=56;u=32;r=56;d=31;break;
    case 111: l=48;u=40;r=56;d=32;break;
    case 112: l=40;u=32;r=44;d=24;break;
    case 113: l=56;u=40;r=56;d=40;break;
    case 114: l=48;u=40;r=48;d=38;break;
    case 115: l=47;u=24;r=28;d=22;break;
    case 120: l=39;u=24;r=31;d=24;break;
    case 122: l=48;u=32;r=48;d=28;break;
    case 123: l=48;u=32;r=47;d=22;break;
    case 124: l=16;u=32;r=16;d=16;break;
    case 125: l=32;u=24;r=32;d=16;break;
    case 130: l=58;u=41;r=58;d=41;break;
    case 131: case 132: case 133: l=49;u=32;r=49;d=32;break;
    case 135: l=40;u=32;r=40;d=24;break;
    case 137: case 141: l=28;u=32;r=28;d=24;break;
    case 138: l=38;u=28;r=32;d=28;break;
    case 139: l=44;u=32;r=32;d=20;break;
    case 140: l=40;u=32;r=32;d=31;break;
    case 142: l=36;u=28;r=40;d=18;break;
    case 143: case 144: case 146: l=24;u=24;r=23;d=23;break;
    case 149: l=64;u=32;r=63;d=31;break;
    case 150: l=32;u=32;r=31;d=31;break;
    case 155: l=36;u=16;r=40;d=20;break;
    case 151: l=40;u=32;r=32;d=31;break;
    case 154: l=56;u=39;r=56;d=39;break;
    case 156: l=16;u=12;r=16;d=20;break;
    case 157: l=48;u=32;r=48;d=24;break;
    case 160: l=48;u=32;r=48;d=40;break;
    case 162: l=20;u=16;r=20;d=16;break;
    case 164: l=40;u=24;r=40;d=24;break;
    case 166: l=36;u=24;r=36;d=20;break;
    case 167: l=48;u=40;r=48;d=32;break;
    case 194: case 195: case 196: l=48;u=32;r=47;d=31;break;
    case 203: case 209: l=32;u=32;r=31;d=31;break;
    case 205: l=25;u=17;r=44;d=20;break;
    case 206: l=44;u=17;r=25;d=20;break;
    case 207: l=41;u=17;r=28;d=20;break;
    case 208: l=28;u=17;r=41;d=20;break;
    case 211: case 213: l=16;u=16;r=15;d=15;break;
    default: return 0;
    }
    if(left)*left=l;
    if(up)*up=u;
    if(right)*right=r;
    if(down)*down=d;
    return 1;
}

static int sc64_z5_world_type_match(unsigned wanted, unsigned raw, int building)
{
    if (wanted == 229) return 1;             /* Any unit, includes buildings. */
    if (wanted == 230) {
        int type = building ? -1 : sc64_race_unit_from_raw((int)raw);
        return !building && raw != 101 && raw != 218 && type >= 0;
    }
    if (wanted >= 228 || raw != wanted) return 0;
    return building ? sc64_race_building_from_raw((int)raw) >= 0 :
                      sc64_race_unit_from_raw((int)raw) >= 0;
}

static int sc64_z5_world_compare(unsigned comparison, unsigned value,
                                  unsigned amount)
{
    switch (comparison) {
    case 0: return value >= amount; /* At least. */
    case 1: return value <= amount; /* At most. */
    case 10: return value == amount; /* CHK Exactly. */
    default: return -1;
    }
}

static int sc64_z5_world_count_one(const Sc64Z5WorldContext *ctx,
                                    unsigned selector, unsigned wanted,
                                    unsigned location, u32 *count)
{
    int left = 0, top = 0, right = race_cols * 32, bottom = race_rows * 32;
    u32 n = 0;
    if (!ctx || !count || selector > 21 ||
        (selector > 13 && selector < 17) || selector > 21 ||
        wanted > 230) return 0;
    if (location && !sc64_z5_world_location(location, &left, &top, &right, &bottom)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int owner = sc64_z5_world_owner_matches(ctx, selector, u[i].raw_owner);
        int al, at, ar, ab;
        if (owner < 0) return 0;
        if (!owner || raw < 0 || !sc64_z5_world_type_match(wanted, (unsigned)raw, 0)) continue;
        if (location) {
            if (!sc64_z5_world_actor_rect((unsigned)raw, u[i].x, u[i].y, &al,&at,&ar,&ab)) return 0;
            if (left >= ar || al >= right || top >= ab || at >= bottom) continue;
        }
        ++n;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int owner = sc64_z5_world_owner_matches(ctx, selector, b[i].raw_owner);
        int l, t, r, d, al, at, ar, ab;
        if (owner < 0) return 0;
        if (!owner || raw < 0 || !sc64_z5_world_type_match(wanted, (unsigned)raw, 1)) continue;
        if (location) {
            if (!sc64_z5_world_building_bounds((unsigned)raw,&l,&t,&r,&d)) return 0;
            al = b[i].x - (l + 1) / 2; at = b[i].y - (t + 1) / 2;
            ar = b[i].x + (r + 1) / 2 + 1; ab = b[i].y + (d + 1) / 2 + 1;
            if (left >= ar || al >= right || top >= ab || at >= bottom) continue;
        }
        ++n;
    }
    *count = n;
    return 1;
}

#ifdef SC64_Z5_COUNT_SNAPSHOT
typedef struct {
    s16 left, top, right, bottom;
    u8 row, query_flags; /* bit 0 Force1; bit 1 raw-51 rather than Any. */
} Sc64Z5CountQuery;
_Static_assert(sizeof(Sc64Z5CountQuery) == 10,
               "Z5 count query scratch layout bound");

/* Only table-owned numeric count rows using the selectors and actor kinds
 * handled by this snapshot can hit the cache. Copies and other source rows
 * always use the retained complete scan. */
static int sc64_z5_count_query(unsigned row_index, Sc64Z5CountQuery *q)
{
    const Sc64Z5SourceCondition *c;
    int left = 0, top = 0, right = 0, bottom = 0;
    if (!q || row_index >= SC64_Z5_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_z5_source_conditions[row_index];
    if (c->index >= sc64_z5_condition_count(c->trigger) ||
        &sc64_z5_source_conditions[row_index] != c ||
        (c->opcode != 2 && c->opcode != 3) || c->flags != 0x10u ||
        c->mask || (c->player != 13 && c->player != 18) ||
        (c->unit != 229 && c->unit != 51) ||
        (c->comparison != 0 && c->comparison != 1 && c->comparison != 10) ||
        (c->opcode == 2 && c->location) ||
        (c->opcode == 3 && !c->location)) return 0;
    if (c->opcode == 3 && !sc64_z5_world_location(c->location,
                                                  &left, &top, &right, &bottom))
        return 0;
    q->left = (s16)left; q->top = (s16)top;
    q->right = (s16)right; q->bottom = (s16)bottom;
    q->row = (u8)row_index;
    q->query_flags = (u8)((c->player == 18 ? 1u : 0u) |
                          (c->unit == 51 ? 2u : 0u));
    return 1;
}

static int sc64_z5_count_snapshot_lookup(const Sc64Z5WorldContext *ctx,
                                          const Sc64Z5SourceCondition *c,
                                          u32 *count)
{
    unsigned row;
    if (!ctx || !count || !ctx->count_snapshot.active || !c) return 0;
    for (row = 0; row < SC64_Z5_SOURCE_CONDITION_COUNT; ++row) {
        Sc64Z5CountQuery query;
        if (c != &sc64_z5_source_conditions[row]) continue;
        if (!sc64_z5_count_query(row, &query)) return 0;
        if (!(ctx->count_snapshot.valid[row >> 3] &
              (u8)(1u << (row & 7u)))) return 0;
        *count = ctx->count_snapshot.count[row];
        return 1;
    }
    return 0;
}

static int sc64_z5_count_snapshot_add_actor(
    const Sc64Z5WorldContext *ctx, Sc64Z5CountSnapshot *snapshot,
    const Sc64Z5CountQuery *queries, unsigned query_count, int building,
    int raw, int raw_owner, int x, int y)
{
    int current, force1, any, type51, rect_ready = 0;
    int al = 0, at = 0, ar = 0, ab = 0;
    if (raw < 0 || raw_owner < 0 || raw_owner >= 12 ||
        !ctx->source_slot_active[(unsigned)raw_owner]) return 1;
    current = (unsigned)raw_owner == ctx->source_current_player;
    force1 = raw_owner < 8 && sc64_z5_source_forces[(unsigned)raw_owner] == 0;
    if (!current && !force1) return 1;
    any = 1; /* CHK Any Unit includes structures and unmapped raw IDs. */
    type51 = !building && raw == 51 && sc64_race_unit_from_raw(51) >= 0;
    for (unsigned i = 0; i < query_count; ++i) {
        const Sc64Z5CountQuery *q = &queries[i];
        int owner_match = (q->query_flags & 1u) ? force1 : current;
        if (!owner_match || ((q->query_flags & 2u) ? !type51 : !any)) continue;
        if (q->right) {
            if (!rect_ready) {
                if (building) {
                    int l, u, r, d;
                    if (!sc64_z5_world_building_bounds((unsigned)raw,
                                                        &l, &u, &r, &d)) return 0;
                    al = x - (l + 1) / 2; at = y - (u + 1) / 2;
                    ar = x + (r + 1) / 2 + 1; ab = y + (d + 1) / 2 + 1;
                } else if (!sc64_z5_world_actor_rect((unsigned)raw, x, y,
                                                     &al, &at, &ar, &ab)) return 0;
                rect_ready = 1;
            }
            if (q->left >= ar || al >= q->right ||
                q->top >= ab || at >= q->bottom) continue;
        }
        if (snapshot->count[q->row] == 0xffffu) return 0;
        ++snapshot->count[q->row];
    }
    return 1;
}

/* Build after all pre-condition world changes and before scheduler actions.
 * Any unsupported query geometry disables the snapshot; callers then retain
 * the original full-scan error/success behavior. */
static int sc64_z5_count_snapshot_begin(Sc64Z5WorldContext *ctx,
                                         const Sc64Z5RuntimeState *runtime)
{
    Sc64Z5CountQuery queries[SC64_Z5_SOURCE_CONDITION_COUNT];
    Sc64Z5CountSnapshot next;
    unsigned query_count = 0;
    if (!ctx) return 0;
#ifdef SC64_Z5_PRUNE_STARTED
    if (!runtime || !sc64_z5_runtime_state_valid(runtime)) return 0;
#else
    (void)runtime;
#endif
    ctx->count_snapshot.active = 0;
    memset(ctx->count_snapshot.count, 0, sizeof(ctx->count_snapshot.count));
    memset(&next, 0, sizeof(next));
    for (unsigned row = 0; row < SC64_Z5_SOURCE_CONDITION_COUNT; ++row) {
        Sc64Z5CountQuery query;
        const Sc64Z5SourceCondition *c = &sc64_z5_source_conditions[row];
        if (c->opcode != 2 && c->opcode != 3) continue;
#ifdef SC64_Z5_PRUNE_STARTED
        if (!sc64_z5_trigger_preserved(c->trigger) &&
            sc64_z5_mask_test(runtime->model.started_lo,
                              runtime->model.started_hi, c->trigger)) continue;
#endif
        if (sc64_z5_count_query(row, &query)) {
            next.valid[row >> 3] |= (u8)(1u << (row & 7u));
            queries[query_count++] = query;
        }
    }
    if (!query_count) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (!sc64_z5_count_snapshot_add_actor(ctx, &next, queries, query_count,
                                               0, raw, u[i].raw_owner,
                                               u[i].x, u[i].y)) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int raw = b[i].raw_type >= 0 ? b[i].raw_type :
                  sc64_race_raw_building(b[i].type);
        if (!sc64_z5_count_snapshot_add_actor(ctx, &next, queries, query_count,
                                               1, raw, b[i].raw_owner,
                                               b[i].x, b[i].y)) return 0;
    }
    memcpy(ctx->count_snapshot.count, next.count, sizeof(next.count));
    memcpy(ctx->count_snapshot.valid, next.valid, sizeof(next.valid));
    ctx->count_snapshot.active = 1;
    return 1;
}

static void sc64_z5_count_snapshot_end(Sc64Z5WorldContext *ctx)
{
    if (!ctx) return;
    ctx->count_snapshot.active = 0;
    memset(ctx->count_snapshot.count, 0, sizeof(ctx->count_snapshot.count));
    memset(ctx->count_snapshot.valid, 0, sizeof(ctx->count_snapshot.valid));
}
#endif

static int sc64_z5_world_condition(void *context,
                                    const Sc64Z5RuntimeState *state,
                                    const Sc64Z5SourceCondition *c)
{
    Sc64Z5WorldContext *ctx = context;
    u32 count = 0;
    int result;
    if (!ctx || !state || !c || (c->flags != 0x10u && c->flags != 0) || c->mask) return -1;
    switch (c->opcode) {
    case 2: case 3: /* Command, Bring */
#ifdef SC64_Z5_COUNT_SNAPSHOT
        if (sc64_z5_count_snapshot_lookup(ctx, c, &count))
            return sc64_z5_world_compare(c->comparison, count, c->amount);
#endif
        if (!sc64_z5_world_count_one(ctx, c->player, c->unit,
                                     c->opcode == 3 ? c->location : 0, &count)) return -1;
        return sc64_z5_world_compare(c->comparison, count, c->amount);
    case 7: case 17: { /* CommandsTheMostAt / CommandTheLeastAt */
        u32 own = 0;
        unsigned subject;
        if (!sc64_z5_world_resolve_owner(ctx,c->player,&subject) || subject>=8 ||
            !sc64_z5_world_count_one(ctx, c->player, c->unit, c->location, &own)) return -1;
        for (unsigned slot = 0; slot < 8; ++slot) if (slot != subject &&
                                                        ctx->source_slot_active[slot]) {
            u32 other = 0;
            if (!sc64_z5_world_count_one(ctx, slot, c->unit, c->location, &other)) return -1;
            if ((c->opcode == 7 && own <= other) || (c->opcode == 17 && own >= other)) return 0;
        }
        return 1;
    }
    case 11: /* Switch: comparison 2=set, 3=cleared. */
        if (c->flags || c->resource > 255 || c->comparison < 2 || c->comparison > 3) return -1;
        result = sc64_z5_switch_is_set(state, c->resource);
        return c->comparison == 2 ? result : !result;
    case 12: /* ElapsedTime is whole mission seconds. */
        return sc64_z5_world_compare(c->comparison,
               (unsigned)(race_campaign.elapsed_frames / 30u), c->amount);
    case 22: /* Always. */
        return 1;
    default: return -1;
    }
}

static int sc64_z5_world_clear_selected(Sc64Z5WorldContext *ctx,
                                         unsigned selector, unsigned raw,
                                         unsigned location)
{
    int left = 0, top = 0, right = race_cols * 32, bottom = race_rows * 32;
    if (!ctx || (location && !sc64_z5_world_location(location,&left,&top,&right,&bottom))) return 0;
    /* Validate every target first so unknown geometry cannot partially apply. */
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int selected = sc64_z5_world_owner_matches(ctx, selector, u[i].raw_owner);
        int actual = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int l,t,r,d;
        if (selected < 0) return 0;
        if (selected && (raw == 229 || actual == (int)raw) && location &&
            !sc64_z5_world_actor_rect((unsigned)actual,u[i].x,u[i].y,&l,&t,&r,&d)) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int selected = sc64_z5_world_owner_matches(ctx, selector, b[i].raw_owner);
        int actual = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int l,t,r,d;
        if (selected < 0) return 0;
        if (selected && (raw == 229 || actual == (int)raw) && location &&
            !sc64_z5_world_building_bounds((unsigned)actual,&l,&t,&r,&d)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int selected = sc64_z5_world_owner_matches(ctx, selector, u[i].raw_owner);
        int actual = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int l,t,r,b;
        if (selected < 0) return 0;
        if (!selected || (raw != 229 && actual != (int)raw)) continue;
        if (location) {
            if (!sc64_z5_world_actor_rect((unsigned)actual,u[i].x,u[i].y,&l,&t,&r,&b)) return 0;
            if (left >= r || l >= right || top >= b || t >= bottom) continue;
        }
        u[i].alive=0; u[i].selected=0; u[i].target=-1; u[i].order=ORDER_IDLE;
        invincible_units[i]=0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int selected = sc64_z5_world_owner_matches(ctx, selector, b[i].raw_owner);
        int actual = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        int l,t,r,d,al,at,ar,ab;
        if (selected < 0) return 0;
        if (!selected || (raw != 229 && actual != (int)raw)) continue;
        if (location) {
            if (!sc64_z5_world_building_bounds((unsigned)actual,&l,&t,&r,&d)) return 0;
            al=b[i].x-(l+1)/2;at=b[i].y-(t+1)/2;
            ar=b[i].x+(r+1)/2+1;ab=b[i].y+(d+1)/2+1;
            if (left >= ar || al >= right || top >= ab || at >= bottom) continue;
        }
        b[i].alive=0; invincible_buildings[i]=0;
        if (selected_building == i) selected_building=-1;
    }
    supply_used = race_owner_supply(SIDE); supply_cap = race_owner_supply_cap(SIDE);
    target_lists_valid=0; sc64_unit_bucket_active=0; sc64_target_index_end();
    invalidate_path_grid();
    return 1;
}

static int sc64_z5_world_action(void *context, const Sc64Z5SourceAction *a)
{
    Sc64Z5WorldContext *ctx = context;
    if (!ctx || !a) return 0;
    switch (a->opcode) {
    case 1: ctx->victory_requested=1; return 1;
    case 2: ctx->defeat_requested=1; return 1;
    case 3: case 4: case 5:
        if (a->opcode == 5) ctx->presentation_pause_requested=1;
        return 1; /* Preserve/wait handled by the source scheduler. */
    case 7: case 8: case 9:
        ctx->last_string_id=(u16)a->string_id;
        ctx->last_wav_id=(u16)a->wav_id;
        ctx->presentation_opcode=a->opcode;
        return 1; /* Asset playback/display is delegated to the presentation layer. */
    case 10: {
        int left,top,right,bottom;
        if(!sc64_z5_world_location(a->location,&left,&top,&right,&bottom)) return 0;
        ctx->center_x=(u16)((left+right)/2);ctx->center_y=(u16)((top+bottom)/2);
        return 1;
    }
    case 11: { /* CreateUnitWithProperties: Z5 source count modifier zero means one. */
        unsigned owner_slot;
        int type,left,top,right,bottom,owner,slot;
        if (a->modifier || a->group2 || a->unit >= 228 ||
            !sc64_z5_world_resolve_owner(ctx,a->group1,&owner_slot) ||
            !sc64_z5_world_location(a->location,&left,&top,&right,&bottom)) return 0;
        if (owner_slot>=12 || !ctx->source_slot_active[owner_slot]) return 0;
        type=sc64_race_unit_from_raw(a->unit);
        if (type<0) return 0;
        owner=ctx->live_owner_for_source[owner_slot];
        if(owner<0) return 0;
        slot=spawn_unit(owner,type,(left+right)/2,(top+bottom)/2);
        if (slot<0) return 0;
        u[slot].raw_type=a->unit; u[slot].raw_owner=(int)owner_slot;
        return 1;
    }
    case 12: ctx->objective_refresh_requested=1; return 1;
    case 13: return 1; /* Scheduler applies the exact switch operand. */
    case 25:
        if (a->modifier || a->group2) return 0;
        return sc64_z5_world_clear_selected(ctx,a->group1,a->unit,a->location);
    case 28: {
        int left,top,right,bottom;
        if(!sc64_z5_world_location(a->location,&left,&top,&right,&bottom)) return 0;
        ctx->center_x=(u16)((left+right)/2);ctx->center_y=(u16)((top+bottom)/2);
        ctx->minimap_ping_requested=1;return 1;
    }
    case 30: ctx->mute_speech_requested=1; return 1;
    case 31: ctx->mute_speech_requested=0; return 1;
    case 39: { /* MoveUnit from one source region to another. */
        int sl,st,sr,sb,dl,dt,dr,db;
        int changed=0;
        unsigned selector=a->group1;
        if (a->modifier || a->unit > 230 ||
            !sc64_z5_world_location(a->location,&sl,&st,&sr,&sb) ||
            !sc64_z5_world_location(a->group2,&dl,&dt,&dr,&db)) return 0;
        for (int i=0;i<MAX_UNITS;i++) if(u[i].alive) {
            int owner=sc64_z5_world_owner_matches(ctx,selector,u[i].raw_owner);
            int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
            if(owner<0)return 0;
            if(owner && sc64_z5_world_type_match(a->unit,(unsigned)raw,0) &&
               u[i].x>=sl && u[i].x<sr && u[i].y>=st && u[i].y<sb) {
                /* Selection pass validates every matching source actor before mutation. */
            }
        }
        /* Move every matching source actor in the source rectangle. */
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
            int owner=sc64_z5_world_owner_matches(ctx,selector,u[i].raw_owner);
            int raw=u[i].raw_type>=0?u[i].raw_type:sc64_race_raw_unit(u[i].type);
            if(owner<0) return 0;
            if(owner && sc64_z5_world_type_match(a->unit,(unsigned)raw,0) &&
               u[i].x>=sl && u[i].x<sr &&
               u[i].y>=st && u[i].y<sb) {
                u[i].x=(dl+dr)/2;u[i].y=(dt+db)/2;changed=1;
            }
        }
        if(changed) {
            target_lists_valid=0;
            sc64_unit_bucket_active=0;
            sc64_target_index_end();
        }
        return 1;
    }
    case 42: { /* SetDoodadState needs authenticated THG2 bindings. */
        unsigned found=0;
        u8 selected[SC64_Z5_DOODAD_CAPACITY]={0};
        u8 next_enabled[SC64_Z5_DOODAD_CAPACITY]={0};
        const Sc64Z5SourceLocation *loc;
        if (!ctx->doodads || ctx->doodad_count!=SC64_Z5_DOODAD_CAPACITY ||
            a->location == 0 || a->location > SC64_Z5_SOURCE_LOCATION_COUNT ||
            a->group1!=17) return 0;
        loc=&sc64_z5_source_locations[a->location-1u];
        if(loc->left>=loc->right || loc->top>=loc->bottom ||
           loc->right>(unsigned)race_cols*32u || loc->bottom>(unsigned)race_rows*32u) return 0;
        for(unsigned i=0;i<ctx->doodad_count;i++) {
            Sc64Z5DoodadState *d=&ctx->doodads[i];
            unsigned next; int owner;
            owner=sc64_z5_world_owner_matches(ctx,a->group1,d->source_owner);
            if(owner<0)return 0;
            if(owner && d->raw_type==a->unit && d->x>=loc->left && d->x<loc->right &&
               d->y>=loc->top && d->y<loc->bottom) {
                if(d->raw_flags & 0x1000u) return 0; /* Sprite-only THG2 row. */
                switch(a->modifier) {
                case 0: case 6: next=!d->enabled; break;
                case 4: next=1; break;
                case 5: next=0; break;
                default: return 0;
                }
                selected[i]=1;next_enabled[i]=(u8)next;++found;
            }
        }
        if(!found)return 0;
        for(unsigned i=0;i<ctx->doodad_count;i++) if(selected[i]) {
            Sc64Z5DoodadState *d=&ctx->doodads[i];
            d->enabled=next_enabled[i];
            if(d->building_slot<MAX_BUILDINGS && b[d->building_slot].alive) {
                /* Door toggles affect both unit pathfinding and collision.
                 * Retain the building identity even while its source state
                 * is disabled; only the path grid is rebuilt. */
                invalidate_path_grid();
                target_lists_valid=0;
                sc64_unit_bucket_active=0;
                sc64_target_index_end();
            }
        }
        return 1;
    }
    default: return 0;
    }
}

static int sc64_z5_world_pause(void *context, int paused)
{
    Sc64Z5WorldContext *ctx=context;
    if(!ctx || (paused!=0 && paused!=1)) return 0;
    ctx->presentation_pause_requested=(u8)paused;
    /* Any new source pause transition supersedes the user's previous
     * acknowledgment, including a later UnpauseGame. */
    if(ctx->pause_acknowledged) *ctx->pause_acknowledged=0;
    return 1;
}

static inline Sc64Z5WorldOps sc64_z5_world_ops(Sc64Z5WorldContext *context)
{
    Sc64Z5WorldOps ops;
    ops.context=context;
    ops.condition=sc64_z5_world_condition;
    ops.action=sc64_z5_world_action;
    ops.world_pause=sc64_z5_world_pause;
    return ops;
}

#endif
