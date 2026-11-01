/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P10_AI_H
#define SC64_P10_AI_H

/* Bounded authored backend for P10's five RunAIScriptAtLocation rows, built
 * like the P9 backend (sc64_p9_ai.h). The native SC64 AI script programs are
 * undecoded: composition, cadence and targeting below are authored. Requests
 * go through the native-shaped request heap and the native cap/count
 * admission gate; production uses each town's Hatcheries and Hives and is
 * paid from the source owner's wallet through race_queue_unit(). The heap is
 * rebuilt on every town step and never saved.
 *
 * Included from the P10 runtime adapter. */
#include "sc64_p8_request_queue.h"
#include "sc64_p8_ai_admission.h"

enum {
    SC64_P10_AI_STEP_UPDATES = 120,
    SC64_P10_AI_FIRST_WAVE = 9000,
    SC64_P10_AI_WAVE_PERIOD = 4500,
    SC64_P10_AI_TOWN_RADIUS = 1024, /* Source pixels. */
    SC64_P10_AI_RAW_LIMIT = 228,
    SC64_P10_AI_REQUESTS = 3
};

typedef struct {
    u32 fourcc;
    u8 trigger, action, slot, location, attacks;
    u8 raw[SC64_P10_AI_REQUESTS], priority[SC64_P10_AI_REQUESTS], cap[SC64_P10_AI_REQUESTS];
} Sc64P10AiScript;

/* P10o runs for source slot 3 (force 2, the Overmind's brood) and holds its
 * ground. Slot 0 (force 3) runs P10C at location 6 and Pr10 at locations 4,
 * 5 and 3; those four towns release attack waves at the nearest human
 * structure. One script ID may run at several locations, so rows match on
 * the authenticated trigger, action, owner and location as well. Caps count
 * the whole source slot. */
static const Sc64P10AiScript sc64_p10_ai_scripts[SC64_P10_TOWN_COUNT] = {
    { 0x6f303150u, 8, 1, 3, 2, 0, { 38, 37, 43 }, { 60, 50, 40 }, { 12, 12, 4 } },
    { 0x43303150u, 9, 1, 0, 6, 1, { 37, 38, 43 }, { 60, 50, 40 }, { 24, 18, 8 } },
    { 0x30317250u, 9, 2, 0, 4, 1, { 37, 38, 43 }, { 60, 50, 40 }, { 24, 18, 8 } },
    { 0x30317250u, 9, 3, 0, 5, 1, { 37, 38, 43 }, { 60, 50, 40 }, { 24, 18, 8 } },
    { 0x30317250u, 9, 4, 0, 3, 1, { 37, 38, 43 }, { 60, 50, 40 }, { 24, 18, 8 } }
};

static int sc64_p10_ai_start(Sc64P10PersistentState *state,
                             const Sc64P10SourceAction *action,
                             unsigned trigger_owner)
{
    if (!state || !action || action->opcode != 16u || action->group1 ||
        action->unit || action->time || action->modifier || action->flags ||
        action->string_id || action->wav_id) return 0;
    for (unsigned i = 0; i < SC64_P10_TOWN_COUNT; ++i) {
        const Sc64P10AiScript *s = &sc64_p10_ai_scripts[i];
        if (s->fourcc != action->group2 || action->trigger != s->trigger ||
            action->index != s->action) continue;
        if (action->location != s->location || trigger_owner != s->slot ||
            state->towns[i].script) return 0;
        state->towns[i].script = (u8)(i + 1u);
        state->towns[i].wave = 0;
        state->towns[i].timer = SC64_P10_AI_STEP_UPDATES;
        return 1;
    }
    return 0;
}

static void sc64_p10_ai_town_center(const Sc64P10AiScript *s, int *x, int *y)
{
    const Sc64P10SourceLocation *l = &sc64_p10_source_locations[s->location - 1u];
    *x = (int)((l->left + l->right) / 4u);
    *y = (int)((l->top + l->bottom) / 4u);
}

static int sc64_p10_ai_building_town(const Sc64P10PersistentState *state, int slot)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < SC64_P10_TOWN_COUNT; ++i) {
        const Sc64P10AiScript *s = &sc64_p10_ai_scripts[i];
        int x, y, d;
        if (!state->towns[i].script || b[slot].raw_owner != s->slot) continue;
        sc64_p10_ai_town_center(s, &x, &y);
        d = dist2(b[slot].x, b[slot].y, x, y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static void sc64_p10_ai_census(unsigned source_slot, u16 counts[SC64_P10_AI_RAW_LIMIT])
{
    for (int i = 0; i < SC64_P10_AI_RAW_LIMIT; ++i) counts[i] = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == (int)source_slot &&
            u[i].raw_type >= 0 && u[i].raw_type < SC64_P10_AI_RAW_LIMIT &&
            counts[u[i].raw_type] < 0xffffu) ++counts[u[i].raw_type];
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == (int)source_slot)
            for (int q = 0; q < b[i].queue_count; ++q) {
                int raw = sc64_race_raw_unit(b[i].queue[(b[i].queue_head + q) % 5]);
                if (raw >= 0 && raw < SC64_P10_AI_RAW_LIMIT && counts[raw] < 0xffffu)
                    ++counts[raw];
            }
}

static int sc64_p10_ai_try_train(Sc64P10PersistentState *state, int town, unsigned raw)
{
    const Sc64P10AiScript *s = &sc64_p10_ai_scripts[town];
    int wallet = sc64_p10_wallet_index(s->slot);
    int type = sc64_race_unit_from_raw((int)raw);
    const Sc64RaceUnitRule *rule = type >= 0 ? sc64_race_unit_rule(type) : 0;
    if (wallet < 0 || !rule || rule->producer == 255) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int saved_minerals, saved_gas, queued;
        if (!b[i].alive || b[i].owner != ENEMY || b[i].raw_owner != s->slot ||
            b[i].queue_count || !building_complete(i) ||
            !sc64_race_producer_compatible(b[i].type, rule->producer) ||
            sc64_p10_ai_building_town(state, i) != town) continue;
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

static void sc64_p10_ai_produce(Sc64P10PersistentState *state, int town)
{
    const Sc64P10AiScript *s = &sc64_p10_ai_scripts[town];
    static u16 counts[SC64_P10_AI_RAW_LIMIT];
    static u8 caps[SC64_P10_AI_RAW_LIMIT];
    sc64_p8_request_queue heap;
    sc64_p8_request request;
    sc64_p10_ai_census(s->slot, counts);
    for (int i = 0; i < SC64_P10_AI_RAW_LIMIT; ++i) caps[i] = 0;
    for (int i = 0; i < SC64_P10_AI_REQUESTS; ++i) caps[s->raw[i]] = s->cap[i];
    sc64_p8_request_queue_init(&heap);
    for (int i = 0; i < SC64_P10_AI_REQUESTS; ++i) {
        sc64_p8_ai_admission_result gate = sc64_p8_ai_admission_blocked(
            caps, SC64_P10_AI_RAW_LIMIT, counts, SC64_P10_AI_RAW_LIMIT, s->raw[i]);
        if (gate == SC64_P8_AI_ADMISSION_ERROR) continue;
        request.priority = s->priority[i];
        request.category = 1u;
        request.raw_unit = s->raw[i];
        request.payload = (u32)town;
        (void)sc64_p8_request_enqueue(&heap, request,
                                      gate == SC64_P8_AI_ADMISSION_NO_BLOCK);
    }
    while (sc64_p8_request_pop(&heap, &request) == SC64_P8_QUEUE_OK)
        if (!sc64_p10_ai_try_train(state, town, request.raw_unit)) break;
}

/* The nearest human structure to the town, else the nearest human unit. */
static int sc64_p10_ai_wave_target(int cx, int cy, int *x, int *y)
{
    int best = 0x7fffffff, found = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) {
            int d = dist2(b[i].x, b[i].y, cx, cy);
            if (d < best) { best = d; *x = b[i].x; *y = b[i].y; found = 1; }
        }
    if (found) return 1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) {
            int d = dist2(u[i].x, u[i].y, cx, cy);
            if (d < best) { best = d; *x = u[i].x; *y = u[i].y; found = 1; }
        }
    return found;
}

static void sc64_p10_ai_wave(Sc64P10PersistentState *state, int town)
{
    const Sc64P10AiScript *s = &sc64_p10_ai_scripts[town];
    int cx, cy, tx, ty, size, sent = 0;
    int radius = SC64_P10_AI_TOWN_RADIUS / 2;
    sc64_p10_ai_town_center(s, &cx, &cy);
    if (!sc64_p10_ai_wave_target(cx, cy, &tx, &ty)) return;
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

static void sc64_p10_ai_tick(Sc64P10PersistentState *state, u32 elapsed_updates)
{
    for (int i = 0; i < SC64_P10_TOWN_COUNT; ++i) {
        Sc64P10TownState *t = &state->towns[i];
        if (!t->script) continue;
        if (t->timer > 1u) { --t->timer; continue; }
        t->timer = SC64_P10_AI_STEP_UPDATES;
        sc64_p10_ai_produce(state, i);
        if (sc64_p10_ai_scripts[i].attacks &&
            elapsed_updates >= (u32)SC64_P10_AI_FIRST_WAVE +
                (u32)t->wave * SC64_P10_AI_WAVE_PERIOD)
            sc64_p10_ai_wave(state, i);
    }
}

#endif /* SC64_P10_AI_H */
