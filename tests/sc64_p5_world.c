#include <assert.h>
#include <string.h>
#include "../.local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h"
#include "../starcraft/sc64_p5.h"
#include "../starcraft/sc64_p5_adapter.h"

enum { MAX_UNITS = 16, MAX_BUILDINGS = 8, SIDE = 0, ENEMY = 1,
       NEUTRAL = 2, RESCUABLE = 3 };
enum { ORDER_IDLE = 0 };
typedef struct {
    int x, y, type, owner, raw_type, raw_owner, alive, selected, target, order;
} Unit;
typedef struct {
    int x, y, type, owner, raw_type, raw_owner, alive;
} Building;
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
static u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
static int race_cols = 128, race_rows = 128;
static int minerals, gas, race_minerals[5], race_gas[5];
static int target_lists_valid, sc64_unit_bucket_active;
static int supply_used, supply_cap;
static int race_owner_supply(int owner) { (void)owner; return 0; }
static int race_owner_supply_cap(int owner) { (void)owner; return 0; }
static void sc64_target_index_end(void) { }
static int sc64_race_unit_from_raw(int raw) { return raw < 228 ? raw : -1; }
static int sc64_race_building_from_raw(int raw) { return raw < 228 ? raw : -1; }
static int sc64_race_raw_unit(int type) { return type; }
static int sc64_race_raw_building(int type) { return type; }
static int spawn_unit(int owner, int type, int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (!u[i].alive) {
        memset(&u[i], 0, sizeof(u[i]));
        u[i].alive = 1; u[i].owner = owner; u[i].type = type;
        u[i].raw_type = type; u[i].x = x; u[i].y = y;
        return i;
    }
    return -1;
}

#include "../starcraft/sc64_p5_world_adapter.h"

static void init_context(Sc64P5WorldContext *ctx, u32 deaths[12])
{
    memset(ctx, 0, sizeof(*ctx));
    memset(deaths, 0, sizeof(u32) * 12u);
    ctx->source_current_player = 7;
    ctx->deaths_by_source = deaths;
    for (unsigned i = 0; i < 8; ++i) {
        ctx->source_slot_active[i] = 1;
        ctx->live_owner_for_source[i] = i == 7 ? SIDE :
            (i == 2 ? RESCUABLE : ENEMY);
    }
    ctx->source_force_for_slot[0] = 3;
    ctx->source_force_for_slot[1] = 3;
    ctx->source_force_for_slot[2] = 0;
    ctx->source_force_for_slot[3] = 3;
    ctx->source_force_for_slot[4] = 2;
    ctx->source_force_for_slot[5] = 3;
    ctx->source_force_for_slot[6] = 1;
    ctx->source_force_for_slot[7] = 0;
}

static void add_unit(int slot, int raw, int raw_owner, int owner, int x, int y)
{
    memset(&u[slot], 0, sizeof(u[slot]));
    u[slot].alive = 1; u[slot].raw_type = raw; u[slot].raw_owner = raw_owner;
    u[slot].type = raw; u[slot].owner = owner; u[slot].x = x; u[slot].y = y;
    u[slot].target = -1;
}

int main(void)
{
    Sc64P5WorldContext ctx;
    Sc64P5RuntimeState state;
    u32 deaths[12];
    init_context(&ctx, deaths);
    (void)sc64_p5_runtime_init(&state);
    assert(sc64_p5_source_model_valid());

    /* Current Player predicates follow the transferred live owner while
     * All Players and direct selectors retain source-slot identity. */
    add_unit(0, 79, 2, SIDE, 1740, 1720);
    add_unit(1, 65, 7, SIDE, 1742, 1720);
    add_unit(2, 65, 7, SIDE, 1744, 1720);
    assert(sc64_p5_world_condition(&ctx, &state, &sc64_p5_source_conditions[4]) == 1);
    assert(sc64_p5_world_condition(&ctx, &state, &sc64_p5_source_conditions[5]) == 1);
    assert(sc64_p5_world_condition(&ctx, &state, &sc64_p5_source_conditions[0]) == 0);
    assert(sc64_p5_world_condition(&ctx, &state, &sc64_p5_source_conditions[1]) == 0);
    deaths[6] = 1;
    assert(sc64_p5_world_condition(&ctx, &state, &sc64_p5_source_conditions[6]) == 1);

    /* Opcode 43 targets live units and buildings in the exact selected region. */
    memset(&b[0], 0, sizeof(b[0]));
    b[0].alive = 1; b[0].raw_type = 154; b[0].raw_owner = 2;
    b[0].owner = SIDE; b[0].x = 1740; b[0].y = 1720;
    memset(&b[1], 0, sizeof(b[1]));
    b[1].alive = 1; b[1].raw_type = 154; b[1].raw_owner = 2;
    b[1].owner = SIDE; b[1].x = 1850; b[1].y = 1720;
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[26]) == 1);
    assert(invincible_units[0] && invincible_units[1] && invincible_units[2]);
    assert(invincible_buildings[0] && !invincible_buildings[1]);
    {
        Sc64P5SourceAction disable = sc64_p5_source_actions[26];
        disable.modifier = 5;
        assert(sc64_p5_world_action(&ctx, &disable) == 1);
        assert(!invincible_units[0] && !invincible_buildings[0]);
    }

    /* Opcode 11 honors zero quantity as one and resolves direct source owner. */
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[8]) == 1);
    assert(u[3].alive && u[3].raw_type == 8 && u[3].raw_owner == 6 &&
           u[3].owner == ENEMY);

    /* Opcode 26 SetResources handles Current Player and multi-slot forces. */
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[7]) == 1);
    assert(minerals == 150 && race_minerals[SIDE] == 150);
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[33]) == 1);
    assert(race_minerals[ENEMY] == 5000 && race_gas[ENEMY] == 5000);
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[35]) == 1);
    assert(race_minerals[ENEMY] == 5000); /* Same live owner applied once. */

    /* Terminal actions and source kill-unit action affect only requested state. */
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[0]) == 1);
    assert(ctx.defeat_requested);
    add_unit(4, 101, 7, SIDE, 1740, 1720);
    assert(sc64_p5_world_action(&ctx, &sc64_p5_source_actions[25]) == 1);
    assert(!u[4].alive);
    assert(deaths[7] == 1); /* Kill Unit increments Current Player deaths. */
    assert(!sc64_p5_world_action(&ctx, &sc64_p5_source_actions[34]));
    return 0;
}
