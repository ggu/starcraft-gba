/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P9_AI_H
#define SC64_P9_AI_H

/* Bounded authored backend for P9's three RunAIScriptAtLocation IDs, built
 * like the P8 backend (sc64_p8_ai.h). The native SC64 AI script programs are
 * undecoded: composition, cadence and targeting below are authored. Requests
 * go through the native-shaped request heap and the native cap/count
 * admission gate; production uses each Hatchery's larva and is paid from the
 * source owner's wallet through race_queue_unit(). The heap is rebuilt on
 * every town step and never saved.
 *
 * Included from the P9 runtime adapter. */
#include "sc64_p8_request_queue.h"
#include "sc64_p8_ai_admission.h"

enum {
    SC64_P9_AI_STEP_UPDATES = 120,
    SC64_P9_AI_FIRST_WAVE = 9000,
    SC64_P9_AI_WAVE_PERIOD = 4500,
    SC64_P9_AI_TOWN_RADIUS = 1024, /* Source pixels. */
    SC64_P9_AI_RAW_LIMIT = 228,
    SC64_P9_AI_REQUESTS = 3
};

typedef struct {
    u32 fourcc;
    u8 trigger, action, slot, location, attacks;
    u8 raw[SC64_P9_AI_REQUESTS], priority[SC64_P9_AI_REQUESTS], cap[SC64_P9_AI_REQUESTS];
} Sc64P9AiScript;

/* Pro9 runs for source slot 0 (force 2), Pr9W for slot 6 (force 3, the
 * Baelrog Cerebrate) and Pr9Y for slot 5 (force 4, the Leviathan Cerebrate).
 * Only Pro9 releases attack waves; the Cerebrate broods hold their ground. */
static const Sc64P9AiScript sc64_p9_ai_scripts[SC64_P9_TOWN_COUNT] = {
    { 0x396f7250u, 9, 1, 0, 3, 1, { 37, 38, 43 }, { 60, 50, 40 }, { 16, 12, 6 } },
    { 0x57397250u, 10, 1, 6, 5, 0, { 38, 37, 43 }, { 60, 50, 40 }, { 12, 12, 4 } },
    { 0x59397250u, 11, 1, 5, 4, 0, { 38, 37, 43 }, { 60, 50, 40 }, { 12, 12, 4 } }
};

static int sc64_p9_ai_start(Sc64P9PersistentState *state,
                            const Sc64P9SourceAction *action,
                            unsigned trigger_owner)
{
    if (!state || !action || action->opcode != 16u || action->group1 ||
        action->unit || action->time || action->modifier || action->flags ||
        action->string_id || action->wav_id) return 0;
    for (unsigned i = 0; i < SC64_P9_TOWN_COUNT; ++i) {
        const Sc64P9AiScript *s = &sc64_p9_ai_scripts[i];
        if (s->fourcc != action->group2) continue;
        if (action->trigger != s->trigger || action->index != s->action ||
            action->location != s->location || trigger_owner != s->slot ||
            state->towns[i].script) return 0;
        state->towns[i].script = (u8)(i + 1u);
        state->towns[i].wave = 0;
        state->towns[i].timer = SC64_P9_AI_STEP_UPDATES;
        return 1;
    }
    return 0;
}

static void sc64_p9_ai_town_center(const Sc64P9AiScript *s, int *x, int *y)
{
    const Sc64P9SourceLocation *l = &sc64_p9_source_locations[s->location - 1u];
    *x = (int)((l->left + l->right) / 4u);
    *y = (int)((l->top + l->bottom) / 4u);
}

static int sc64_p9_ai_building_town(const Sc64P9PersistentState *state, int slot)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < SC64_P9_TOWN_COUNT; ++i) {
        const Sc64P9AiScript *s = &sc64_p9_ai_scripts[i];
        int x, y, d;
        if (!state->towns[i].script || b[slot].raw_owner != s->slot) continue;
        sc64_p9_ai_town_center(s, &x, &y);
        d = dist2(b[slot].x, b[slot].y, x, y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static void sc64_p9_ai_census(unsigned source_slot, u16 counts[SC64_P9_AI_RAW_LIMIT])
{
    for (int i = 0; i < SC64_P9_AI_RAW_LIMIT; ++i) counts[i] = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == (int)source_slot &&
            u[i].raw_type >= 0 && u[i].raw_type < SC64_P9_AI_RAW_LIMIT &&
            counts[u[i].raw_type] < 0xffffu) ++counts[u[i].raw_type];
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == (int)source_slot)
            for (int q = 0; q < b[i].queue_count; ++q) {
                int raw = sc64_race_raw_unit(b[i].queue[(b[i].queue_head + q) % 5]);
                if (raw >= 0 && raw < SC64_P9_AI_RAW_LIMIT && counts[raw] < 0xffffu)
                    ++counts[raw];
            }
}

static int sc64_p9_ai_try_train(Sc64P9PersistentState *state, int town, unsigned raw)
{
    const Sc64P9AiScript *s = &sc64_p9_ai_scripts[town];
    int wallet = sc64_p9_wallet_index(s->slot);
    int type = sc64_race_unit_from_raw((int)raw);
    const Sc64RaceUnitRule *rule = type >= 0 ? sc64_race_unit_rule(type) : 0;
    if (wallet < 0 || !rule || rule->producer == 255) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int saved_minerals, saved_gas, queued;
        if (!b[i].alive || b[i].owner != ENEMY || b[i].raw_owner != s->slot ||
            b[i].queue_count || !building_complete(i) ||
            !sc64_race_producer_compatible(b[i].type, rule->producer) ||
            sc64_p9_ai_building_town(state, i) != town) continue;
        saved_minerals = race_minerals[ENEMY];
        saved_gas = race_gas[ENEMY];
        race_minerals[ENEMY] = state->wallet_minerals[wallet];
        race_gas[ENEMY] = state->wallet_gas[wallet];
        queued = race_queue_unit(ENEMY, i, type);
        state->wallet_minerals[wallet] = (u16)race_minerals[ENEMY];
        state->wallet_gas[wallet] = (u16)race_gas[ENEMY];
        race_minerals[ENEMY] = saved_minerals;
        race_gas[ENEMY] = saved_gas;
        if (queued) return 1;
    }
    return 0;
}

static void sc64_p9_ai_produce(Sc64P9PersistentState *state, int town)
{
    const Sc64P9AiScript *s = &sc64_p9_ai_scripts[town];
    static u16 counts[SC64_P9_AI_RAW_LIMIT];
    static u8 caps[SC64_P9_AI_RAW_LIMIT];
    sc64_p8_request_queue heap;
    sc64_p8_request request;
    sc64_p9_ai_census(s->slot, counts);
    for (int i = 0; i < SC64_P9_AI_RAW_LIMIT; ++i) caps[i] = 0;
    for (int i = 0; i < SC64_P9_AI_REQUESTS; ++i) caps[s->raw[i]] = s->cap[i];
    sc64_p8_request_queue_init(&heap);
    for (int i = 0; i < SC64_P9_AI_REQUESTS; ++i) {
        sc64_p8_ai_admission_result gate = sc64_p8_ai_admission_blocked(
            caps, SC64_P9_AI_RAW_LIMIT, counts, SC64_P9_AI_RAW_LIMIT, s->raw[i]);
        if (gate == SC64_P8_AI_ADMISSION_ERROR) continue;
        request.priority = s->priority[i];
        request.category = 1u;
        request.raw_unit = s->raw[i];
        request.payload = (u32)town;
        (void)sc64_p8_request_enqueue(&heap, request,
                                      gate == SC64_P8_AI_ADMISSION_NO_BLOCK);
    }
    while (sc64_p8_request_pop(&heap, &request) == SC64_P8_QUEUE_OK)
        if (!sc64_p9_ai_try_train(state, town, request.raw_unit)) break;
}

static int sc64_p9_ai_wave_target(int *x, int *y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS) {
            *x = b[i].x; *y = b[i].y; return 1;
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) { *x = b[i].x; *y = b[i].y; return 1; }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { *x = u[i].x; *y = u[i].y; return 1; }
    return 0;
}

static void sc64_p9_ai_wave(Sc64P9PersistentState *state, int town)
{
    const Sc64P9AiScript *s = &sc64_p9_ai_scripts[town];
    int cx, cy, tx, ty, size, sent = 0;
    int radius = SC64_P9_AI_TOWN_RADIUS / 2;
    if (!sc64_p9_ai_wave_target(&tx, &ty)) return;
    sc64_p9_ai_town_center(s, &cx, &cy);
    size = 4 + 2 * state->towns[town].wave;
    if (size > 12) size = 12;
    for (int i = 0; i < MAX_UNITS && sent < size; ++i) {
        if (!u[i].alive || u[i].owner != ENEMY || u[i].raw_owner != s->slot ||
            u[i].order != ORDER_IDLE || sc64_race_is_worker(u[i].type) ||
            u[i].raw_type == 42 || u[i].raw_type == 45 || u[i].raw_type == 46 ||
            dist2(u[i].x, u[i].y, cx, cy) > radius * radius) continue;
        u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
        u[i].tx = tx; u[i].ty = ty;
        u[i].path_timer = 0; u[i].path_retry = 0;
        ++sent;
    }
    if (state->towns[town].wave < 255u) ++state->towns[town].wave;
}

static void sc64_p9_ai_tick(Sc64P9PersistentState *state, u32 elapsed_updates)
{
    for (int i = 0; i < SC64_P9_TOWN_COUNT; ++i) {
        Sc64P9TownState *t = &state->towns[i];
        if (!t->script) continue;
        if (t->timer > 1u) { --t->timer; continue; }
        t->timer = SC64_P9_AI_STEP_UPDATES;
        sc64_p9_ai_produce(state, i);
        if (sc64_p9_ai_scripts[i].attacks &&
            elapsed_updates >= (u32)SC64_P9_AI_FIRST_WAVE +
                (u32)t->wave * SC64_P9_AI_WAVE_PERIOD)
            sc64_p9_ai_wave(state, i);
    }
}

#endif /* SC64_P9_AI_H */
