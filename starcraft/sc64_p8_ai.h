/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_AI_H
#define SC64_P8_AI_H

/* Bounded authored backend for P8's three RunAIScriptAtLocation IDs.
 *
 * The native SC64 AI script programs remain undecoded, so this is not a
 * script interpreter and does not claim native composition, cadence or
 * targeting. It reuses the native-backed pieces that are known: requests are
 * ordered through the native-shaped 63-entry production heap and filtered by
 * the native cap/count admission gate before insertion. Production is paid
 * from the source owner's own wallet through the engine's ordinary
 * race_queue_unit() path. The heap is rebuilt on every town step and never
 * persists, so saves store only each town's schedule.
 *
 * Included from the P8 runtime adapter, after the engine and race adapters. */
#include "sc64_p8_request_queue.h"
#include "sc64_p8_ai_admission.h"

enum {
    SC64_P8_AI_STEP_UPDATES = 120,
    SC64_P8_AI_FIRST_WAVE = 9000,
    SC64_P8_AI_WAVE_PERIOD = 4500,
    SC64_P8_AI_TOWN_RADIUS = 1024, /* Source pixels. */
    SC64_P8_AI_RAW_LIMIT = 228,
    SC64_P8_AI_REQUESTS = 3
};

typedef struct {
    u32 fourcc;
    u8 trigger, action, slot, location, attacks;
    /* Authored composition: source raw ids, heap priorities and per-owner caps. */
    u8 raw[SC64_P8_AI_REQUESTS], priority[SC64_P8_AI_REQUESTS], cap[SC64_P8_AI_REQUESTS];
} Sc64P8AiScript;

/* Script IDs, trigger rows, owners and locations are authenticated against the
 * P8 source actions. Pro8 and Pr8B run for source slot 3 (force 3) and Pr8D
 * for slot 0 (force 2, the Stasis Cell defenders). */
static const Sc64P8AiScript sc64_p8_ai_scripts[SC64_P8_AI_TOWN_COUNT] = {
    { 0x386f7250u, 5, 1, 3, 2, 1, { 65, 66, 83 }, { 60, 50, 40 }, { 16, 10, 4 } },
    { 0x42387250u, 5, 2, 3, 3, 1, { 65, 66, 83 }, { 60, 50, 40 }, { 16, 10, 4 } },
    { 0x44387250u, 6, 1, 0, 4, 0, { 66, 65, 70 }, { 60, 50, 40 }, { 24, 12, 12 } }
};

static int sc64_p8_ai_wallet_index(unsigned slot)
{ return slot == 0u ? 0 : slot == 3u ? 1 : -1; }

/* RunAIScriptAtLocation: accept only the three authenticated rows. */
static int sc64_p8_ai_start(Sc64P8PersistentState *state,
                            const Sc64P8SourceAction *action,
                            unsigned trigger_owner)
{
    if (!state || !action || action->opcode != 16u || action->group1 ||
        action->unit || action->time || action->modifier || action->flags ||
        action->string_id || action->wav_id) return 0;
    for (unsigned i = 0; i < SC64_P8_AI_TOWN_COUNT; ++i) {
        const Sc64P8AiScript *s = &sc64_p8_ai_scripts[i];
        if (s->fourcc != action->group2) continue;
        if (action->trigger != s->trigger || action->index != s->action ||
            action->location != s->location || trigger_owner != s->slot ||
            state->towns[i].script) return 0;
        state->towns[i].script = (u8)(i + 1u);
        state->towns[i].wave = 0;
        state->towns[i].timer = SC64_P8_AI_STEP_UPDATES;
        return 1;
    }
    return 0;
}

static void sc64_p8_ai_town_center(const Sc64P8AiScript *s, int *x, int *y)
{
    const Sc64P8SourceLocation *l = &sc64_p8_source_locations[s->location - 1u];
    *x = (int)((l->left + l->right) / 4u); /* Engine pixels are source / 2. */
    *y = (int)((l->top + l->bottom) / 4u);
}

/* A producer belongs to the nearest active town of its own source owner. */
static int sc64_p8_ai_building_town(const Sc64P8PersistentState *state, int slot)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < SC64_P8_AI_TOWN_COUNT; ++i) {
        const Sc64P8AiScript *s = &sc64_p8_ai_scripts[i];
        int x, y, d;
        if (!state->towns[i].script || b[slot].raw_owner != s->slot) continue;
        sc64_p8_ai_town_center(s, &x, &y);
        d = dist2(b[slot].x, b[slot].y, x, y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

/* Live plus queued products of one source owner, by source raw id. */
static void sc64_p8_ai_census(unsigned source_slot, u16 counts[SC64_P8_AI_RAW_LIMIT])
{
    for (int i = 0; i < SC64_P8_AI_RAW_LIMIT; ++i) counts[i] = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == (int)source_slot &&
            u[i].raw_type >= 0 && u[i].raw_type < SC64_P8_AI_RAW_LIMIT &&
            counts[u[i].raw_type] < 0xffffu) ++counts[u[i].raw_type];
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_owner == (int)source_slot)
            for (int q = 0; q < b[i].queue_count; ++q) {
                int raw = sc64_race_raw_unit(b[i].queue[(b[i].queue_head + q) % 5]);
                if (raw >= 0 && raw < SC64_P8_AI_RAW_LIMIT && counts[raw] < 0xffffu)
                    ++counts[raw];
            }
}

/* Queue one product on an idle producer of this town, paying from the source
 * wallet through the engine's normal production rules. */
static int sc64_p8_ai_try_train(Sc64P8PersistentState *state, int town,
                                unsigned raw)
{
    const Sc64P8AiScript *s = &sc64_p8_ai_scripts[town];
    int wallet = sc64_p8_ai_wallet_index(s->slot);
    int type = sc64_race_unit_from_raw((int)raw);
    const Sc64RaceUnitRule *rule = type >= 0 ? sc64_race_unit_rule(type) : 0;
    if (wallet < 0 || !rule || rule->producer == 255) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int saved_minerals, saved_gas, queued;
        if (!b[i].alive || b[i].owner != ENEMY || b[i].raw_owner != s->slot ||
            b[i].queue_count || !building_complete(i) ||
            !sc64_race_producer_compatible(b[i].type, rule->producer) ||
            sc64_p8_ai_building_town(state, i) != town) continue;
        saved_minerals = race_minerals[ENEMY];
        saved_gas = race_gas[ENEMY];
        race_minerals[ENEMY] = (int)state->ai_minerals[wallet];
        race_gas[ENEMY] = (int)state->ai_gas[wallet];
        queued = race_queue_unit(ENEMY, i, type);
        state->ai_minerals[wallet] = (u32)race_minerals[ENEMY];
        state->ai_gas[wallet] = (u32)race_gas[ENEMY];
        race_minerals[ENEMY] = saved_minerals;
        race_gas[ENEMY] = saved_gas;
        if (queued) return 1;
    }
    return 0;
}

static void sc64_p8_ai_produce(Sc64P8PersistentState *state, int town)
{
    const Sc64P8AiScript *s = &sc64_p8_ai_scripts[town];
    static u16 counts[SC64_P8_AI_RAW_LIMIT];
    static u8 caps[SC64_P8_AI_RAW_LIMIT];
    sc64_p8_request_queue heap;
    sc64_p8_request request;
    sc64_p8_ai_census(s->slot, counts);
    for (int i = 0; i < SC64_P8_AI_RAW_LIMIT; ++i) caps[i] = 0;
    for (int i = 0; i < SC64_P8_AI_REQUESTS; ++i) caps[s->raw[i]] = s->cap[i];
    sc64_p8_request_queue_init(&heap);
    for (int i = 0; i < SC64_P8_AI_REQUESTS; ++i) {
        sc64_p8_ai_admission_result gate = sc64_p8_ai_admission_blocked(
            caps, SC64_P8_AI_RAW_LIMIT, counts, SC64_P8_AI_RAW_LIMIT, s->raw[i]);
        if (gate == SC64_P8_AI_ADMISSION_ERROR) continue;
        request.priority = s->priority[i];
        request.category = 1u; /* Native unit-request category. */
        request.raw_unit = s->raw[i];
        request.payload = (u32)town;
        (void)sc64_p8_request_enqueue(&heap, request,
                                      gate == SC64_P8_AI_ADMISSION_NO_BLOCK);
    }
    /* Highest priority first; a request no producer can take ends the step. */
    while (sc64_p8_request_pop(&heap, &request) == SC64_P8_QUEUE_OK)
        if (!sc64_p8_ai_try_train(state, town, request.raw_unit)) break;
}

static int sc64_p8_ai_wave_target(int *x, int *y)
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

/* Release idle army units of the town owner near the town toward the human
 * base. The cadence and size are authored, not native-qualified. */
static void sc64_p8_ai_wave(Sc64P8PersistentState *state, int town)
{
    const Sc64P8AiScript *s = &sc64_p8_ai_scripts[town];
    int cx, cy, tx, ty, size, sent = 0;
    int radius = SC64_P8_AI_TOWN_RADIUS / 2;
    if (!sc64_p8_ai_wave_target(&tx, &ty)) return;
    sc64_p8_ai_town_center(s, &cx, &cy);
    size = 4 + 2 * state->towns[town].wave;
    if (size > 12) size = 12;
    for (int i = 0; i < MAX_UNITS && sent < size; ++i) {
        if (!u[i].alive || u[i].owner != ENEMY || u[i].raw_owner != s->slot ||
            u[i].order != ORDER_IDLE || sc64_race_is_worker(u[i].type) ||
            u[i].raw_type == 67 || u[i].raw_type == 71 || u[i].raw_type == 84 ||
            dist2(u[i].x, u[i].y, cx, cy) > radius * radius) continue;
        u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
        u[i].tx = tx; u[i].ty = ty;
        u[i].path_timer = 0; u[i].path_retry = 0;
        ++sent;
    }
    if (state->towns[town].wave < 255u) ++state->towns[town].wave;
}

static void sc64_p8_ai_tick(Sc64P8PersistentState *state, u32 elapsed_updates)
{
    for (int i = 0; i < SC64_P8_AI_TOWN_COUNT; ++i) {
        Sc64P8AiTownState *t = &state->towns[i];
        if (!t->script) continue;
        if (t->timer > 1u) { --t->timer; continue; }
        t->timer = SC64_P8_AI_STEP_UPDATES;
        sc64_p8_ai_produce(state, i);
        if (sc64_p8_ai_scripts[i].attacks &&
            elapsed_updates >= (u32)SC64_P8_AI_FIRST_WAVE +
                (u32)t->wave * SC64_P8_AI_WAVE_PERIOD)
            sc64_p8_ai_wave(state, i);
    }
}

#endif /* SC64_P8_AI_H */
