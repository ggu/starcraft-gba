/* Internal T5 engine fixture. Direct world edits below test trigger outcomes;
 * they are not a controller completion or hardware-performance proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && screen == SCREEN_PLAY);
    assert(!sc64_catalog_mission_allowed(SC64_T5_GLOBAL_ID, R_TER));
    assert(!mission_unsupported_count && !won && !lost);
    assert(race_cols == 96 && race_rows == 96);
    assert(race_t5.active_mask == SC64_T5_ACTIVE_SOURCE_MASK);
}

static void roster(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_T5_GLOBAL_ID];
    unsigned char used_u[MAX_UNITS] = {0}, used_b[MAX_BUILDINGS] = {0};
    unsigned char used_m[MAX_MINERALS] = {0};
    int units = 0, buildings = 0, fields = 0, owners[8] = {0};
    assert(map->unit_count == 168);
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        int found = 0;
        if (src->source_type >= 176 && src->source_type <= 178) {
            for (int i = 0; i < MAX_MINERALS; ++i)
                if (!used_m[i] && m[i].amount > 0 &&
                    m[i].x == src->x_px && m[i].y == src->y_px &&
                    m[i].style == src->source_type) {
                    assert(m[i].amount == (src->resources ? src->resources : 1500));
                    used_m[i] = 1; found = 1; ++fields; break;
                }
        } else {
            for (int i = 0; i < MAX_UNITS; ++i)
                if (!used_u[i] && u[i].alive && u[i].raw_type == src->source_type &&
                    u[i].raw_owner == src->owner && u[i].x == src->x_px && u[i].y == src->y_px) {
                    used_u[i] = 1; found = 1; ++units; ++owners[src->owner];
                    assert(u[i].flags == src->flags);
                    assert(u[i].owner == race_campaign_owner(map, src->owner));
                    break;
                }
            if (!found) for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (!used_b[i] && b[i].alive && b[i].raw_type == src->source_type &&
                    b[i].raw_owner == src->owner && b[i].x == src->x_px && b[i].y == src->y_px) {
                    used_b[i] = 1; found = 1; ++buildings; ++owners[src->owner];
                    assert(b[i].flags == src->flags);
                    assert(b[i].owner == race_campaign_owner(map, src->owner));
                    break;
                }
        }
        assert(found);
    }
    assert(units == 79 && buildings == 48 && fields == 41);
    assert(owners[0] == 8 && owners[2] == 2 && owners[3] == 39 && owners[6] == 78);
    for (int i = 0; i < MAX_UNITS; ++i) assert(!u[i].alive || used_u[i]);
    for (int i = 0; i < MAX_BUILDINGS; ++i) assert(!b[i].alive || used_b[i]);
    assert(minerals == 0 && gas == 0);
    /* The compact mission payload must preserve this initial engine state. */
    assert(race_save_game());
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    Sc64T5RuntimeState before_runtime = race_t5;
    memcpy(before_units, u, sizeof(u)); memcpy(before_buildings, b, sizeof(b));
    RaceSave rejected = {0};
    rejected.version = SAVE_VERSION; rejected.state[2] = SC64_T5_GLOBAL_ID;
    save_write(&rejected, sizeof(rejected), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)) && !memcmp(before_buildings, b, sizeof(b)));
    assert(!memcmp(&before_runtime, &race_t5, sizeof(race_t5)));
}

static void initial_pass(void)
{
    for (int tick = 0; tick < 2; ++tick) race_campaign_tick();
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(minerals == 300 && gas == 300);
    assert(race_minerals[ENEMY] == 0 && race_gas[ENEMY] == 0);
    assert(race_t5.resources[0].total_minerals_gathered == 300);
    assert(race_t5.resources[6].total_minerals_gathered == 0);
    assert(!(race_t5.switches.bits[0] & (1u << 6)));
    /* Directly place a human ground actor inside source location 18 to test
     * the original Bring -> Switch 6 -> Force 2 grant chain. */
    int actor = -1, x, y;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) { actor = i; break; }
    assert(actor >= 0);
    const Sc64T4Location *area = &sc64_t5_scenario.locations[17];
    x = (int)(area->left + area->right) / 4;
    y = (int)(area->top + area->bottom) / 4;
    u[actor].x = x; u[actor].y = y;
    for (int tick = 0; tick < 35; ++tick) race_campaign_tick();
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_t5.switches.bits[0] & (1u << 6));
    assert(race_minerals[ENEMY] == 6000 && race_gas[ENEMY] == 6000);
    assert(race_t5.resources[6].total_minerals_gathered == 6000);
    for (int p = 0; p < 8; ++p) assert(!race_t5.player[p].fault);
}

static void outcomes(void)
{
    start_t5();
    /* Original victory queries Force 2 Buildings. Enemy units alone cannot
     * prevent victory; a single surviving building must prevent it. */
    int last = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        if (last >= 0) b[last].alive = 0;
        last = i;
    }
    assert(last >= 0);
    for (int tick = 0; tick < 35; ++tick) race_campaign_tick();
    assert(!won && !lost && screen == SCREEN_PLAY);
    b[last].alive = 0;
    for (int tick = 0; tick < 35 && !won; ++tick) race_campaign_tick();
    assert(won && !lost && race_campaign.phase == SC64_RACE_PHASE_COMPLETE);

    start_t5();
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].owner == SIDE) u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].owner == SIDE) b[i].alive = 0;
    for (int tick = 0; tick < 35 && !lost; ++tick) race_campaign_tick();
    assert(lost && !won && race_campaign.phase == SC64_RACE_PHASE_FAILED);

    /* Either hero's death must fail the mission even with surviving human
     * troops. Run the retained wait/presentation sequence to its defeat. */
    const int heroes[] = {16, 19};
    for (unsigned hero = 0; hero < sizeof(heroes) / sizeof(heroes[0]); ++hero) {
        start_t5();
        int removed = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].raw_type == heroes[hero]) { u[i].alive = 0; ++removed; }
        assert(removed == 1);
        for (int tick = 0; tick < 2400 && !lost; ++tick) {
            race_campaign_tick();
            assert(screen == SCREEN_PLAY);
        }
        assert(lost && !won && race_campaign.phase == SC64_RACE_PHASE_FAILED);
    }
}

static void rebel_sequence(void)
{
    start_t5();
    int kerrigan = -1, before = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].type == U_KERRIGAN) kerrigan = i;
        if (u[i].raw_owner == 3) ++before;
        /* Isolate scripted effects from the separately tested mine combat. */
        if (u[i].type == U_SPIDER_MINE) u[i].alive = 0;
    }
    assert(kerrigan >= 0);
    /* Kerrigan starts under source rescue ownership. Contact must transfer
     * her before Current Player's base-entry condition can match her. */
    int escort = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) { escort = i; break; }
    assert(escort >= 0);
    u[escort].x = u[kerrigan].x; u[escort].y = u[kerrigan].y;
    race_t5_world_tick();
    assert(u[kerrigan].owner == SIDE);
    const Sc64T4Location *beacon = &sc64_t5_scenario.locations[0];
    u[kerrigan].x = (int)(beacon->left + beacon->right) / 4;
    u[kerrigan].y = (int)(beacon->top + beacon->bottom) / 4;
    for (int tick = 0; tick < 2400; ++tick) {
        race_campaign_tick();
        assert(screen == SCREEN_PLAY && !won && !lost);
        for (int p = 0; p < 8; ++p) assert(!race_t5.player[p].fault);
    }
    assert(race_t5.player[0].flags[10] == SC64_T4_TR_DONE);
    assert(race_t5.ai.requested_scripts[3] & SC64_T5_SCRIPT_BIT_RSCU);
    int after = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_owner == 3) {
        ++after;
        assert(u[i].owner == SIDE || u[i].owner == RESCUABLE);
    }
    assert(after == before + 3 && u[kerrigan].alive);
}

int main(void)
{
    start_t5(); roster(); initial_pass(); outcomes(); rebel_sequence();
    puts("T5 roster, gated grants, rebel sequence, outcomes and save rejection pass; menu gated");
    return 0;
}
