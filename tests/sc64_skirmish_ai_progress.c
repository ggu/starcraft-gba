/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Canonical race-skirmish AI progression, capability and resume harness.
 * Progression uses game_update() with ordinary inputs and a passive human.
 * Named scenario fixtures inject setup and failures explicitly; snapshots
 * only read actor storage. Build once per canonical race profile. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_UNIFIED) || !defined(SC64_IMPORTED_SKIRMISH)
#error "Build the AI progress harness with a canonical race or Showcase profile"
#endif

static u16 previous;
static void input(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}
static void tap(u16 key) { input(key); input(0); }
static void to_row(int row)
{
    for (int step = 0; race_front.row != row && step < 16; ++step)
        tap(race_front.row < row ? KEY_DOWN : KEY_UP);
    assert(race_front.row == row);
}
static void set_field(int *field, int value)
{
    for (int i = 0; *field != value && i < 3; ++i) tap(KEY_RIGHT);
    assert(*field == value);
}

static void launch(int map, int human, int bots, const int *bot_races)
{
    game_init(); previous = 0;
    tap(KEY_DOWN); tap(KEY_A);
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_SKIRMISH);
    to_row(0); set_field(&race_front.map, map);
    to_row(1); set_field(&race_front.race, human);
    to_row(2); set_field(&race_front.bots, bots);
    for (int i = 0; i < bots; ++i) { to_row(3 + i); set_field(&race_front.bot_races[i], bot_races[i]); }
    to_row(3 + bots); tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
}

/* ---- Capability audit: what the catalog rows map to in this profile. ---- */
static void audit(void)
{
    printf("{\"audit\":[\n");
    int first = 1;
    for (unsigned i = 0; i < sc64_skirmish_record_count(); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_record_at(i);
        int type = -1, producer = -1, damage = 0, air = 0, flying = 0, minerals = 0, gas = 0, frames = 0, supply = 0;
        if (r->kind == SC64_SK_TRAIN) {
            type = sc64_race_unit_from_raw(r->raw_id);
            const Sc64RaceUnitRule *ur = type >= 0 ? sc64_race_unit_rule(type) : 0;
            if (ur) {
                producer = ur->producer; damage = sc64_race_unit_damage(type);
                air = sc64_race_unit_can_attack_air(type); flying = sc64_race_unit_flying(type);
                minerals = ur->minerals; gas = ur->gas; frames = ur->build_frames; supply = ur->supply;
            }
        } else if (r->kind == SC64_SK_WORKER_BUILD || r->kind == SC64_SK_ADDON ||
                   r->kind == SC64_SK_BUILDING_MORPH) {
            type = sc64_race_building_from_raw(r->raw_id);
            const Sc64RaceBuildingRule *br = type >= 0 ? sc64_race_building_rule(type) : 0;
            if (br) { minerals = br->minerals; gas = br->gas; frames = br->build_frames; supply = br->supply_provided; }
        } else continue;
        printf("%s{\"raw\":%u,\"race\":%u,\"kind\":%u,\"name\":\"%s\",\"type\":%d,\"producer\":%d,"
               "\"damage\":%d,\"air\":%d,\"flying\":%d,\"minerals\":%d,\"gas\":%d,\"frames\":%d,\"supply\":%d}",
               first ? "" : ",\n", r->raw_id, r->race, r->kind, r->name, type, producer,
               damage, air, flying, minerals, gas, frames, supply);
        first = 0;
    }
    printf("\n]}\n");
}

/* ---- Snapshots. ---- */
typedef struct {
    int workers, mining, gas_workers, idle_workers, building_workers;
    int army, army_supply, air_army, anti_air, units;
    int buildings, complete, depots, refineries, producers, queued;
    int supply, cap;
    int raw_units[256 / 8 * 8];
} OwnerStats;

static void stats_for(int owner, OwnerStats *s)
{
    memset(s, 0, sizeof *s);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) {
        int raw = sc64_race_raw_unit(u[i].type);
        ++s->units;
        if (raw >= 0 && raw < 256) ++s->raw_units[raw];
        if (sc64_race_is_worker(u[i].type)) {
            ++s->workers;
            if (u[i].order == ORDER_MINE) ++s->mining;
            else if (u[i].order == ORDER_GAS) ++s->gas_workers;
            else if (u[i].order == ORDER_IDLE) ++s->idle_workers;
            else if (SC64_BUILD_WORKER_ORDER(u[i].order)) ++s->building_workers;
        } else if (sc64_race_unit_damage(u[i].type) > 0 || sc64_race_unit_can_attack_air(u[i].type)) {
            const Sc64RaceUnitRule *r = sc64_race_unit_rule(u[i].type);
            ++s->army; s->army_supply += r ? r->supply : 0;
            if (sc64_race_unit_flying(u[i].type)) ++s->air_army;
            if (sc64_race_unit_can_attack_air(u[i].type)) ++s->anti_air;
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) {
        ++s->buildings;
        if (!b[i].under_construction) ++s->complete;
        if (b[i].type == B_COMMAND || b[i].type == B_NEXUS || sc64_race_is_zerg_townhall(b[i].type)) ++s->depots;
        if (b[i].type == B_REFINERY || b[i].type == B_EXTRACTOR || b[i].type == B_ASSIMILATOR) ++s->refineries;
        s->queued += b[i].queue_count;
    }
    s->supply = race_owner_supply(owner);
    s->cap = race_owner_supply_cap(owner);
}

static void print_owner(FILE *out, int owner)
{
    OwnerStats s;
    stats_for(owner, &s);
    fprintf(out, "{\"owner\":%d,\"race\":%d,\"minerals\":%d,\"gas\":%d,\"supply\":%d,\"cap\":%d,"
                 "\"workers\":%d,\"mining\":%d,\"gas_workers\":%d,\"idle_workers\":%d,\"building_workers\":%d,"
                 "\"army\":%d,\"army_supply\":%d,\"air_army\":%d,\"anti_air\":%d,\"depots\":%d,\"refineries\":%d,"
                 "\"buildings\":%d,\"complete\":%d,\"queued\":%d,\"unit_raw\":{",
            owner, race_owner_identity(owner), race_owner_minerals(owner), race_owner_gas(owner),
            s.supply, s.cap, s.workers, s.mining, s.gas_workers, s.idle_workers, s.building_workers,
            s.army, s.army_supply, s.air_army, s.anti_air, s.depots, s.refineries,
            s.buildings, s.complete, s.queued);
    int first = 1;
    for (int raw = 0; raw < 256; ++raw) if (s.raw_units[raw]) {
        fprintf(out, "%s\"%d\":%d", first ? "" : ",", raw, s.raw_units[raw]); first = 0;
    }
    fprintf(out, "},\"building_raw\":{");
    first = 1;
    int counted[256] = {0}, pending[256] = {0};
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) {
        int raw = sc64_race_raw_building(b[i].type);
        if (raw >= 0 && raw < 256) { if (b[i].under_construction) ++pending[raw]; else ++counted[raw]; }
    }
    for (int raw = 0; raw < 256; ++raw) if (counted[raw] || pending[raw]) {
        fprintf(out, "%s\"%d\":[%d,%d]", first ? "" : ",", raw, counted[raw], pending[raw]); first = 0;
    }
    fprintf(out, "}}");
}

static void snapshot(FILE *out, const char *label)
{
    fprintf(out, "{\"kind\":\"%s\",\"frame\":%d,\"won\":%d,\"lost\":%d,\"max_units\":%d,\"max_buildings\":%d,\"owners\":[",
            label, frame, won, lost, MAX_UNITS, MAX_BUILDINGS);
    print_owner(out, SIDE);
    for (int i = 0; i < race_bots; ++i) { fputc(',', out); print_owner(out, race_bot_owner_ids[i]); }
    fprintf(out, "]}\n");
}

/* ---- Deterministic world digest for save/resume continuation. ---- */
static uint32_t hash_word(uint32_t h, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) { h ^= (uint8_t)v; h *= 16777619u; v >>= 8; }
    return h;
}
static uint32_t world_hash(void)
{
    uint32_t h = 2166136261u;
#define MIX(v) (h = hash_word(h, (uint32_t)(v)))
    MIX(frame); MIX(seed); MIX(minerals); MIX(gas); MIX(won); MIX(lost);
    for (int p = 0; p < 5; ++p) { MIX(race_player[p]); MIX(race_owner_minerals(p)); MIX(race_owner_gas(p)); }
    for (int i = 0; i < MAX_MINERALS; ++i) { MIX(m[i].x); MIX(m[i].y); MIX(m[i].amount); }
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y); MIX(q->hp); MIX(q->order);
        MIX(q->tx); MIX(q->ty); MIX(q->carry); MIX(q->target); MIX(q->work_timer); MIX(q->shield);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y); MIX(q->hp); MIX(q->under_construction);
        MIX(q->maxhp); MIX(q->raw_type); MIX(q->build_progress); MIX(q->queue_count); MIX(q->queue_progress); MIX(q->larva);
        for (int j = 0; j < q->queue_count && j < 5; ++j) MIX(q->queue[(q->queue_head + j) % 5]);
    }
#undef MIX
    return h;
}

static void run_ticks(int ticks, int every, FILE *out)
{
    for (int t = 0; t < ticks && screen == SCREEN_PLAY && !won && !lost; ++t) {
        input(0);
        if (every > 0 && frame % every == 0) snapshot(out, "tick");
    }
}

/* ---- Recovery fixtures. Losses are injected through the engine's own
 * hit_target death path (as if the human's Marine killed the actor); the
 * AI must recover using only its ordinary paid services. ---- */
static int first_bot(void) { return race_bot_owner_ids[0]; }

static int run_until(int limit, int (*ready)(void))
{
    for (int t = 0; t < limit && screen == SCREEN_PLAY && !won && !lost; ++t) {
        input(0);
        if (ready()) return 1;
    }
    return 0;
}

static int pending_builder(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == first_bot() && u[i].order == ORDER_BUILD_PENDING &&
            dist2(u[i].x, u[i].y, u[i].tx, u[i].ty) > 64 * 64) return i;
    return -1;
}
static int ready_pending_builder(void) { return pending_builder() >= 0; }

static int scv_constructing(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == first_bot() && u[i].order == ORDER_BUILD &&
            u[i].target >= 0 && u[i].target < MAX_BUILDINGS && b[u[i].target].alive &&
            b[u[i].target].under_construction && b[u[i].target].build_progress > 30 &&
            b[u[i].target].type != B_DEPOT) return i;
    return -1;
}
static int ready_scv_constructing(void) { return scv_constructing() >= 0; }

static int foundation(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == first_bot() && b[i].under_construction &&
            b[i].type != B_DEPOT && b[i].type != B_PYLON && !sc64_race_is_zerg_townhall(b[i].type) &&
            b[i].type != B_COMMAND && b[i].type != B_NEXUS && b[i].build_progress > 30) return i;
    return -1;
}
static int ready_foundation(void) { return foundation() >= 0; }

static int bot_depot(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == first_bot() && !b[i].under_construction &&
            (b[i].type == B_COMMAND || b[i].type == B_NEXUS || sc64_race_is_zerg_townhall(b[i].type)))
            return i;
    return -1;
}
static int ready_depot_loss(void)
{
    int workers = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == first_bot() && sc64_race_is_worker(u[i].type)) ++workers;
    int other = 0;
    /* Another completed structure keeps the match from ending at the loss. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == first_bot() && !b[i].under_construction &&
            b[i].type != B_COMMAND && b[i].type != B_NEXUS && !sc64_race_is_zerg_townhall(b[i].type))
            ++other;
    return workers >= 8 && other && race_owner_minerals(first_bot()) >= 450 && bot_depot() >= 0;
}

static int count_type(int owner, int type, int complete_only)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type &&
            (!complete_only || !b[i].under_construction)) ++n;
    for (int i = 0; !complete_only && i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].order == ORDER_BUILD_PENDING && u[i].target == type) ++n;
    return n;
}

static int sunken_morph(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner != SIDE &&
            race_construction_deferred_morph(&b[i]) == B_SUNKEN_COLONY &&
            b[i].build_progress > 30) return i;
    return -1;
}
static int ready_sunken_morph(void) { return sunken_morph() >= 0; }

static void kill_unit(int slot) { hit_target(slot, 10000, U_MARINE, SIDE); }
static void kill_building(int slot) { hit_target(MAX_UNITS + slot, 100000, U_MARINE, SIDE); }

static int save_through_pause(void)
{
#ifdef SC64_IMPACT_EVENTS
    for (int t = 0; t < 120 && race_impact_count > RACE_IMPACT_SAVE_CAPACITY; ++t) input(0);
    if (race_impact_count > RACE_IMPACT_SAVE_CAPACITY) return 0;
#endif
    if (screen != SCREEN_PLAY || won || lost) return 0;
    tap(KEY_START); if (screen != SCREEN_PAUSE) return 0;
    input(KEY_B);
    input(KEY_A); input(0);
    return screen == SCREEN_PLAY;
}

static int reload_saved(void)
{
    game_init(); previous = 0;
    input(KEY_L);
    return screen == SCREEN_PLAY && race_mode;
}

/* ---- Walking builders. Construction pays on arrival; a walking builder's
 * work_timer counts planning passes (already saved by every unit codec) and
 * the planner releases it after RACE_AI_STALL_PASSES. ---- */
static int built_since(int owner, int type, int x, int y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type &&
            dist2(b[i].x, b[i].y, x, y) <= 32 * 32) return i;
    return -1;
}

/* Run until the builder arrives (a structure of its type stands at its
 * site), is released (idle), or the limit. jam != 0 returns it to (jx,jy)
 * every update, as a unit stuck in a crowd. Returns the event frame. */
static int follow_builder(int slot, int type, int x, int y, int jam, int jx, int jy,
                          int limit, int *arrived, int *released)
{
    int owner = u[slot].owner;
    *arrived = *released = 0;
    for (int t = 0; t < limit && screen == SCREEN_PLAY && !won && !lost; ++t) {
        input(0);
        if (built_since(owner, type, x, y) >= 0) { *arrived = 1; return frame; }
        if (!u[slot].alive || u[slot].order != ORDER_BUILD_PENDING) {
            /* The same pass that releases a builder sends it back to work. */
            *released = u[slot].alive && u[slot].order != ORDER_BUILD;
            return frame;
        }
        if (jam) { u[slot].x = jx; u[slot].y = jy; }
    }
    return -1;
}

/* A walking builder already counted by a pass and still well short of its
 * site, so a save lands mid-walk. */
static int counted_builder(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == first_bot() && u[i].order == ORDER_BUILD_PENDING &&
            u[i].work_timer >= 1 && dist2(u[i].x, u[i].y, u[i].tx, u[i].ty) > 96 * 96) return i;
    return -1;
}
static int ready_counted_builder(void) { return counted_builder() >= 0; }

static int builder_walk(int scenario)
{
    static const char *names[] = { "walk-save-resume", "timeout-replan", "timeout-save-resume" };
    int owner = first_bot(), slot, type, x, y, jx, jy, ok = 0, arrived = 0, released = 0;
    int event = -1, ref_event = -1, timer_at_save = -1, replanned = -1, before, start = -1;
    uint32_t reference = 0, resumed = 0;
    if (scenario < 0 || scenario > 2) return 0;
    if (!run_until(20000, scenario == 0 ? ready_counted_builder : ready_pending_builder)) goto done;
    slot = scenario == 0 ? counted_builder() : pending_builder();
    type = u[slot].target; x = u[slot].tx; y = u[slot].ty;
    jx = u[slot].x; jy = u[slot].y; start = frame;
    before = count_type(owner, type, 0);
    if (scenario == 0) {
        int ref_arrived, ref_released, horizon;
        if (!save_through_pause()) goto done;
        timer_at_save = u[slot].work_timer;
        ref_event = follow_builder(slot, type, x, y, 0, 0, 0, 1800, &ref_arrived, &ref_released);
        horizon = frame + 60;
        while (frame < horizon) input(0);
        reference = world_hash();
        if (!reload_saved()) goto done;
        if (u[slot].order != ORDER_BUILD_PENDING || u[slot].work_timer != timer_at_save) goto done;
        event = follow_builder(slot, type, x, y, 0, 0, 0, 1800, &arrived, &released);
        while (frame < horizon) input(0);
        resumed = world_hash();
        ok = ref_arrived && arrived && event == ref_event && reference == resumed;
    } else {
        if (scenario == 2) {
            /* Jam until the pass counter is halfway, then save mid-wait. */
            for (int t = 0; t < 4000 && u[slot].alive && u[slot].order == ORDER_BUILD_PENDING &&
                 u[slot].work_timer < RACE_AI_STALL_PASSES / 2; ++t) {
                input(0); u[slot].x = jx; u[slot].y = jy;
            }
            if (!save_through_pause()) goto done;
            timer_at_save = u[slot].work_timer;
            ref_event = follow_builder(slot, type, x, y, 1, jx, jy, 4000, &arrived, &released);
            if (!released) goto done;
            reference = world_hash();
            if (!reload_saved() || u[slot].work_timer != timer_at_save) goto done;
        }
        event = follow_builder(slot, type, x, y, 1, jx, jy, 4000, &arrived, &released);
        if (scenario == 2) resumed = world_hash();
        /* Released, nothing paid; the structure is planned again. */
        for (int t = 0; t < 2400 && released && replanned < 0; ++t) {
            input(0);
            if (count_type(owner, type, 0) >= before) replanned = frame;
        }
        ok = released && !arrived && replanned >= 0 &&
            event - start <= (RACE_AI_STALL_PASSES + 2) * RACE_AI_PERIOD &&
            (scenario != 2 || (event == ref_event && reference == resumed));
    }
done:
    printf("{\"kind\":\"builder\",\"scenario\":\"%s\",\"start\":%d,\"timer_at_save\":%d,\"event\":%d,\"reference_event\":%d,"
           "\"arrived\":%d,\"released\":%d,\"replanned\":%d,\"stall_passes\":%d,\"reference\":\"%08x\","
           "\"resumed\":\"%08x\",\"ok\":%d}\n",
           scenario >= 0 && scenario <= 2 ? names[scenario] : "?", start, timer_at_save, event, ref_event,
           arrived, released, replanned, RACE_AI_STALL_PASSES, reference, resumed, ok);
    return ok;
}

/* ---- Workers after STOP, depletion and construction. The shared update
 * no longer sends idle workers to the nearest field; the planner sends its
 * own idle workers back, and never touches the human's. STOP is the
 * engine's stop_selected applied to one selected worker. ---- */
static void stop_one(int slot)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    u[slot].selected = 1;
    stop_selected();
    u[slot].selected = 0;
}

static int worker_with(int owner, int order)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && sc64_race_is_worker(u[i].type) &&
            u[i].order == order && !u[i].carry) return i;
    return -1;
}

static int local_patch(int owner, int patch)
{
    int base = race_ai_owner_base(owner);
    return patch >= 0 && patch < MAX_MINERALS && m[patch].amount > 0 &&
        dist2(m[patch].x, m[patch].y, race_base_x[base], race_base_y[base]) <= 320 * 320;
}

static int worker_jobs(int scenario)
{
    static const char *names[] = { "bot-stop-returns", "human-stop-stays", "depleted-patch-reassigned",
                                   "constructed-scv-returns" };
    int owner = scenario == 1 ? SIDE : first_bot(), slot = -1, ok = 0, back = -1, old = -1, idle_at = -1;
    if (scenario < 0 || scenario > 3) return 0;
    for (int t = 0; t < 900; ++t) input(0);
    if (scenario == 3) {
        if (race_owner_identity(owner) != R_TER || !run_until(20000, ready_scv_constructing)) goto done;
        slot = scv_constructing();
        while (screen == SCREEN_PLAY && u[slot].alive && u[slot].order == ORDER_BUILD) input(0);
        idle_at = frame;
    } else {
        slot = worker_with(owner, ORDER_MINE);
        if (slot < 0) goto done;
        old = u[slot].target;
        if (scenario == 2) {
            m[old].amount = 0;               /* the field this worker mines runs out */
            for (int t = 0; t < 600 && u[slot].order == ORDER_MINE && u[slot].target == old; ++t) input(0);
        } else stop_one(slot);
        idle_at = frame;
    }
    for (int t = 0; t < 600 && screen == SCREEN_PLAY; ++t) {
        input(0);
        if (back < 0 && u[slot].alive &&
            ((u[slot].order == ORDER_MINE && local_patch(owner, u[slot].target)) ||
             (scenario == 3 && (u[slot].order == ORDER_GAS || SC64_BUILD_WORKER_ORDER(u[slot].order)))))
            back = frame;
    }
    if (scenario == 1)
        ok = u[slot].alive && u[slot].order == ORDER_IDLE && back < 0;
    else
        ok = back >= 0 && back - idle_at <= 2 * RACE_AI_PERIOD + RACE_AI_PHASE &&
            (scenario != 2 || u[slot].target != old);
done:
    printf("{\"kind\":\"workers\",\"scenario\":\"%s\",\"slot\":%d,\"idle_at\":%d,\"back_at\":%d,"
           "\"old_patch\":%d,\"new_patch\":%d,\"order\":%d,\"ok\":%d}\n",
           scenario >= 0 && scenario <= 3 ? names[scenario] : "?", slot, idle_at, back, old,
           slot >= 0 ? u[slot].target : -1, slot >= 0 ? u[slot].order : -1, ok);
    return ok;
}

static int recovery(int scenario)
{
    int owner = first_bot(), ok = 0, type = -1, slot = -1, bank = 0, f0;
    const char *name = "";
    if (scenario == 0) {
        name = "pending-builder-killed";
        if (!run_until(20000, ready_pending_builder)) goto done;
        slot = pending_builder(); type = u[slot].target; bank = race_owner_minerals(owner);
        int before = count_type(owner, type, 0);
        kill_unit(slot);
        /* Pay-on-arrival: losing a walking builder costs no resources. */
        if (u[slot].alive || race_owner_minerals(owner) != bank) goto done;
        if (count_type(owner, type, 0) != before - 1) goto done;
        f0 = frame;
        for (int t = 0; t < 4000 && !ok; ++t) { input(0); ok = count_type(owner, type, 0) >= before; }
        printf("{\"kind\":\"recovery\",\"scenario\":\"%s\",\"type\":%d,\"lost_at\":%d,\"replanned_at\":%d,\"ok\":%d}\n",
               name, sc64_race_raw_building(type), f0, frame, ok);
        return ok;
    }
    if (scenario == 1) {
        name = "constructing-scv-killed";
        if (!run_until(20000, ready_scv_constructing)) goto done;
        slot = scv_constructing(); int site = u[slot].target; int progress = b[site].build_progress;
        kill_unit(slot);
        f0 = frame;
        int resumed = -1;
        for (int t = 0; t < 6000 && !ok; ++t) {
            input(0);
            if (!b[site].alive) break;
            if (resumed < 0)
                for (int i = 0; i < MAX_UNITS; ++i)
                    if (u[i].alive && u[i].owner == owner && u[i].order == ORDER_BUILD && u[i].target == site)
                        resumed = frame;
            ok = resumed >= 0 && !b[site].under_construction;
        }
        printf("{\"kind\":\"recovery\",\"scenario\":\"%s\",\"type\":%d,\"progress_at_loss\":%d,\"lost_at\":%d,"
               "\"resumed_at\":%d,\"completed_at\":%d,\"ok\":%d}\n",
               name, sc64_race_raw_building(b[site].type), progress, f0, resumed, ok ? frame : -1, ok);
        return ok;
    }
    if (scenario == 2) {
        name = "foundation-destroyed";
        if (!run_until(20000, ready_foundation)) goto done;
        slot = foundation(); type = b[slot].type;
        /* Structures of this type that existed at the loss, in any state.
         * A walking builder's plan may never complete (e.g. a second gas
         * structure on a one-geyser base), so it is not counted. */
        int before = 0;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            before += b[i].alive && b[i].owner == owner && b[i].type == type;
        kill_building(slot);
        if (b[slot].alive && b[slot].type == type) goto done;
        f0 = frame;
        for (int t = 0; t < 6000 && !ok; ++t) { input(0); ok = count_type(owner, type, 1) >= before; }
        printf("{\"kind\":\"recovery\",\"scenario\":\"%s\",\"type\":%d,\"lost_at\":%d,\"rebuilt_at\":%d,\"ok\":%d}\n",
               name, sc64_race_raw_building(type), f0, ok ? frame : -1, ok);
        return ok;
    }
    if (scenario == 3) {
        name = "depot-destroyed";
        if (!run_until(20000, ready_depot_loss)) goto done;
        slot = bot_depot(); type = b[slot].type;
        /* Destroy every depot, including Zerg macro Hatcheries and any
         * unfinished one, so the owner has nowhere to return cargo. */
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == owner &&
                (b[i].type == B_COMMAND || b[i].type == B_NEXUS || sc64_race_is_zerg_townhall(b[i].type)))
                kill_building(i);
        if (bot_depot() >= 0) goto done;
        f0 = frame;
        int rebuilt = -1;
        static int prior_carry[MAX_UNITS];
        /* Complete a paid replacement depot, then deliver new cargo to it:
         * a loaded miner unloads beside it. Spending can hide the bank rise. */
        for (int t = 0; t < 9000 && !ok && screen == SCREEN_PLAY && !won && !lost; ++t) {
            for (int i = 0; i < MAX_UNITS; ++i) prior_carry[i] = u[i].alive && u[i].owner == owner ? u[i].carry : 0;
            input(0);
            if (rebuilt < 0 && bot_depot() >= 0) rebuilt = frame;
            if (rebuilt >= 0) {
                int d = bot_depot();
                for (int i = 0; i < MAX_UNITS && !ok; ++i)
                    ok = u[i].alive && u[i].owner == owner && u[i].order == ORDER_MINE &&
                         prior_carry[i] > 0 && u[i].carry == 0 &&
                         dist2(u[i].x, u[i].y, b[d].x, b[d].y) < 48 * 48;
            }
            if (getenv("AI_DEBUG") && t % 600 == 0) {
                fprintf(stderr, "t%d bank%d ", t, race_owner_minerals(owner));
                for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner)
                    fprintf(stderr, "[%d r%d o%d tg%d %d,%d>%d,%d]", i, sc64_race_raw_unit(u[i].type), u[i].order, u[i].target, u[i].x, u[i].y, u[i].tx, u[i].ty);
                fprintf(stderr, "\n");
            }
        }
        printf("{\"kind\":\"recovery\",\"scenario\":\"%s\",\"type\":%d,\"lost_at\":%d,\"rebuilt_at\":%d,\"ok\":%d}\n",
               name, sc64_race_raw_building(type), f0, rebuilt, ok);
        return ok;
    }
done:
    printf("{\"kind\":\"recovery\",\"scenario\":\"%s\",\"frame\":%d,\"ok\":0,\"setup\":0}\n", name, frame);
    return 0;
}

/* ---- Creep Colony to Sunken Colony morph hit points. BW keeps the
 * colony's hit points while it morphs and applies the maximum-HP
 * difference (300 - 400) on completion, clamped to 1..300. The human's
 * colony is used so the planner never touches it; it is started through
 * the same paid service as the command row. ---- */
static int human_site(int type, int *x, int *y)
{
    for (int r = 48; r <= 160; r += 16)
        for (int dy = -r; dy <= r; dy += 16)
            for (int dx = -r; dx <= r; dx += 16) {
                int px = race_base_x[0] + dx, py = race_base_y[0] + dy;
                if ((dx == -r || dx == r || dy == -r || dy == r) &&
                    race_can_site(SIDE, type, px, py) && can_place(type, px, py)) {
                    *x = px; *y = py; return 1;
                }
            }
    return 0;
}

static int spawn_complete(int type)
{
    int x, y, slot;
    if (!human_site(type, &x, &y)) return -1;
    slot = spawn_build(SIDE, type, x, y, 0);
    if (slot >= 0) invalidate_path_grid();
    return slot;
}

static int damage_building(int slot, int amount)
{
    hit_target(MAX_UNITS + slot, amount, U_MARINE, ENEMY);
    return b[slot].alive ? b[slot].hp : 0;
}

static int morph_expected(int hp)
{
    hp += 300 - 400;
    return hp < 1 ? 1 : hp > 300 ? 300 : hp;
}

static int morph_hp(int scenario)
{
    static const char *names[] = { "full", "damaged-during", "low-hp", "low-hp-dies",
                                   "save-resume", "save-resume-low-hp" };
    int creep, pool, ok = 0, start_hp = -1, during_hp = -1, during_max = -1, final_hp = -1;
    int final_max = -1, final_type = -1, alive = 0, charged = 0, completed_at = -1;
    uint32_t reference = 0, resumed = 0;
    const Sc64SkirmishRecord *r;
    if (scenario < 0 || scenario > 5 || race_human != R_ZERG) return 0;
    for (int t = 0; t < 60; ++t) input(0);
    pool = spawn_complete(B_SPAWNING_POOL);
    creep = spawn_complete(B_CREEP_COLONY);
    r = race_skirmish_build_record(SIDE, sc64_race_raw_building(B_SUNKEN_COLONY));
    if (pool < 0 || creep < 0 || !r || r->kind != SC64_SK_BUILDING_MORPH ||
        b[creep].hp != 400 || b[creep].maxhp != 400) goto done;
    if (scenario == 2 || scenario == 3 || scenario == 5)
        damage_building(creep, 350);               /* damaged before the morph */
    start_hp = b[creep].hp;
    minerals += 50; charged = minerals;
    if (!race_skirmish_start_record(SIDE, creep, r)) goto done;
    charged -= minerals;                           /* paid from the human's bank */
    /* While incomplete the colony keeps its type, maximum and hit points. */
    if (b[creep].type != B_CREEP_COLONY || b[creep].maxhp != 400 ||
        b[creep].hp != start_hp || race_construction_deferred_morph(&b[creep]) != B_SUNKEN_COLONY)
        goto done;
    for (int t = 0; t < 120; ++t) input(0);
    if (scenario == 1) damage_building(creep, 350); /* BW: survives with 50 */
    if (scenario == 3) damage_building(creep, 60);  /* 50 hit points: destroyed */
    if (scenario == 4) damage_building(creep, 30);
    during_hp = b[creep].alive ? b[creep].hp : 0; during_max = b[creep].maxhp;
    if (scenario == 4 || scenario == 5) {
        int len;
        if (!b[creep].alive || !b[creep].under_construction || !save_through_pause()) goto done;
        len = 400;
        for (int t = 1; t < len; ++t) input(0);
        reference = world_hash();
        final_hp = b[creep].hp;
        game_init(); previous = 0;
        input(KEY_L);
        if (screen != SCREEN_PLAY || !race_mode) goto done;
        /* The reloaded morph still holds its source maximum and hit points. */
        if (b[creep].type != B_CREEP_COLONY || b[creep].maxhp != 400 ||
            b[creep].hp != during_hp || !b[creep].under_construction) goto done;
        for (int t = 0; t < len; ++t) input(0);
        resumed = world_hash();
        if (resumed != reference || b[creep].hp != final_hp) goto done;
    } else {
        for (int t = 0; t < 400 && b[creep].alive && b[creep].under_construction; ++t) input(0);
    }
    alive = b[creep].alive;
    if (alive && !b[creep].under_construction) completed_at = frame;
    final_hp = alive ? b[creep].hp : 0; final_max = b[creep].maxhp; final_type = b[creep].type;
    if (scenario == 3)
        ok = !alive && during_hp == 0 && charged == 50 && sc64_race_raw_building(final_type) == 143;
    else
        ok = alive && completed_at >= 0 && charged == 50 && final_type == B_SUNKEN_COLONY &&
            final_max == 300 && final_hp == morph_expected(during_hp) && during_max == 400 &&
            (scenario != 1 || (during_hp > 0 && during_hp <= 100)) &&
            (scenario != 2 || during_hp == start_hp);
done:
    printf("{\"kind\":\"morph-hp\",\"scenario\":\"%s\",\"start_hp\":%d,\"during_hp\":%d,\"during_max\":%d,"
           "\"final_hp\":%d,\"final_max\":%d,\"final_raw\":%d,\"alive\":%d,\"charged\":%d,"
           "\"completed_at\":%d,\"reference\":\"%08x\",\"resumed\":\"%08x\",\"ok\":%d}\n",
           scenario >= 0 && scenario <= 5 ? names[scenario] : "?", creep >= 0 ? start_hp : -1,
           during_hp, during_max, final_hp, final_max,
           final_type >= 0 ? sc64_race_raw_building(final_type) : -1, alive, charged,
           completed_at, reference, resumed, ok);
    return ok;
}

/* ---- Addon feasibility. A Terran opponent with every step before the
 * Machine Shop satisfied and an empty bank either reserves the Machine
 * Shop's price (some Factory can take it) or skips the step without
 * reserving (no Factory can). With the price available, the addon starts
 * on the first feasible Factory. Fixture structures are spawned complete;
 * the planner itself pays for the addon. ---- */
/* Machine Shop exists in every canonical profile through its raw ID. */
#define SHOP_TYPE sc64_race_building_from_raw(120)
/* Spawn a complete fixture structure near the owner's start. With an
 * addon type, keep only a site whose addon footprint is free and would
 * leave the producer's unit exit open. */
static int owner_spawn(int owner, int type, int addon)
{
    int base = race_ai_owner_base(owner);
    for (int r = 64; r <= 224; r += 16)
        for (int dy = -r; dy <= r; dy += 16)
            for (int dx = -r; dx <= r; dx += 16) {
                int px = race_base_x[base] + dx, py = race_base_y[base] + dy, slot;
                if ((dx != -r && dx != r && dy != -r && dy != r) ||
                    !race_can_site(owner, type, px, py) || !can_place(type, px, py) ||
                    (addon >= 0 && !can_place(addon, px + 48, py + 8))) continue;
                slot = spawn_build(owner, type, px, py, 0);
                if (slot < 0) return -1;
                invalidate_path_grid();
                if (addon < 0 || race_ai_exit_escapes(slot, addon, px + 48, py + 8)) return slot;
                b[slot].alive = 0;
                invalidate_path_grid();
            }
    return -1;
}

/* Synthetic AI setup; the actual defense planner starts the paid morph. */
static int ai_sunken_save(void)
{
    int owner = first_bot(), creep, pool, hp, ok = 0;
    uint32_t reference, resumed;
    RaceAiCensus c;
    if (race_owner_identity(owner) != R_ZERG) return 0;
    for (int t = 0; t < 60; ++t) input(0);
    pool = owner_spawn(owner, B_SPAWNING_POOL, -1);
    creep = owner_spawn(owner, B_CREEP_COLONY, -1);
    if (pool < 0 || creep < 0) return 0;
    race_minerals[owner] += 50;
    race_ai_census(owner, &c);
    if (!race_ai_start_defense(&c, 146) || !b[creep].under_construction ||
        race_construction_deferred_morph(&b[creep]) != B_SUNKEN_COLONY) return 0;
    for (int t = 0; t < 120; ++t) input(0);
    hit_target(MAX_UNITS + creep, 30, U_MARINE, SIDE);
    hp = b[creep].hp;
    if (!b[creep].alive || !b[creep].under_construction || !save_through_pause()) return 0;
    for (int t = 1; t < 400; ++t) input(0);
    reference = world_hash();
    game_init(); previous = 0; input(KEY_L);
    if (screen != SCREEN_PLAY || !race_mode || !b[creep].alive ||
        b[creep].owner != owner || b[creep].type != B_CREEP_COLONY ||
        b[creep].maxhp != 400 || b[creep].hp != hp ||
        race_construction_deferred_morph(&b[creep]) != B_SUNKEN_COLONY) return 0;
    for (int t = 0; t < 400; ++t) input(0);
    resumed = world_hash();
    ok = reference == resumed && b[creep].alive && b[creep].type == B_SUNKEN_COLONY &&
        b[creep].hp == morph_expected(hp);
    printf("{\"kind\":\"ai-sunken-save\",\"hp\":%d,\"reference\":\"%08x\",\"resumed\":\"%08x\",\"ok\":%d}\n", hp, reference, resumed, ok);
    return ok;
}

static int addon_at(int owner, int parent)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == SHOP_TYPE &&
            b[i].x == b[parent].x + 48 && b[i].y == b[parent].y + 8) return i;
    return -1;
}

static int addon_feasibility(int scenario)
{
    static const char *names[] = { "first-blocked-second-free", "no-feasible-parent" };
    int owner = first_bot(), f[2] = { -1, -1 }, blockers = 0, parent = -2, ok = 0;
    int reserve_m = -1, reserve_g = -1, started_on = -1, charged_m = -1, charged_g = -1;
    const Sc64SkirmishRecord *r;
    RaceAiCensus c;
    if ((scenario != 0 && scenario != 1) || race_owner_identity(owner) != R_TER) return 0;
    for (int t = 0; t < 30; ++t) input(0);
    /* Satisfy the steps before the Machine Shop. */
    if (owner_spawn(owner, B_BARRACKS, -1) < 0 || owner_spawn(owner, B_BARRACKS, -1) < 0) goto done;
    {
        int base = race_ai_owner_base(owner), g = -1, best = 0x7fffffff;
        for (int i = 0; i < geyser_count; ++i) {
            int d = dist2(geysers[i].x, geysers[i].y, race_base_x[base], race_base_y[base]);
            if (d < best && can_place(B_REFINERY, geysers[i].x, geysers[i].y)) { best = d; g = i; }
        }
        if (g < 0 || spawn_build(owner, B_REFINERY, geysers[g].x, geysers[g].y, 0) < 0) goto done;
    }
    for (int i = 0; i < 2; ++i) if ((f[i] = owner_spawn(owner, B_FACTORY, SHOP_TYPE)) < 0) goto done;
    for (int n = 0; n < 12; ++n)
        if (spawn_unit(owner, U_SCV, race_base_x[race_ai_owner_base(owner)], race_base_y[race_ai_owner_base(owner)] + 48) < 0) goto done;
    /* Occupy the addon footprint of the first Factory (both in scenario 1). */
    for (int i = 0; i <= scenario; ++i) {
        if (spawn_build(owner, B_DEPOT, b[f[i]].x + 48, b[f[i]].y + 8, 0) < 0) goto done;
        ++blockers;
    }
    invalidate_path_grid();
    if (can_place(SHOP_TYPE, b[f[0]].x + 48, b[f[0]].y + 8) ||
        can_place(SHOP_TYPE, b[f[1]].x + 48, b[f[1]].y + 8) != (scenario == 0)) goto done;
    r = race_skirmish_build_record(owner, sc64_race_raw_building(SHOP_TYPE));
    if (!r || r->kind != SC64_SK_ADDON) goto done;
    /* Empty bank: what does one development pass reserve? */
    race_minerals[owner] = 0; race_gas[owner] = 0;
    race_ai_census(owner, &c);
    parent = race_ai_attach_parent(&c, r);
    if (getenv("AI_DEBUG"))
        for (int i = 0; i < 2; ++i)
            fprintf(stderr, "factory %d at %d,%d ready %d place %d escapes %d escapes-now %d\n", f[i], b[f[i]].x, b[f[i]].y,
                    race_skirmish_record_ready(owner, f[i], r), can_place(SHOP_TYPE, b[f[i]].x + 48, b[f[i]].y + 8),
                    race_ai_exit_escapes(f[i], SHOP_TYPE, b[f[i]].x + 48, b[f[i]].y + 8), race_ai_exit_escapes(f[i], -1, 0, 0));
    reserve_m = c.reserve_minerals; reserve_g = c.reserve_gas;
    race_ai_develop(&c);
    reserve_m = c.reserve_minerals - reserve_m; reserve_g = c.reserve_gas - reserve_g;
    /* Exactly the Machine Shop's price: does a pass start it? */
    race_minerals[owner] = 50; race_gas[owner] = 50;
    race_ai_census(owner, &c);
    race_ai_develop(&c);
    charged_m = 50 - race_minerals[owner]; charged_g = 50 - race_gas[owner];
    for (int i = 0; i < 2; ++i) if (addon_at(owner, f[i]) >= 0) started_on = started_on < 0 ? i : 9;
    if (scenario == 0)
        ok = parent == f[1] && reserve_m == 50 && reserve_g == 50 && started_on == 1 &&
            charged_m == 50 && charged_g == 50 && b[addon_at(owner, f[1])].under_construction;
    else
        ok = parent == -1 && reserve_g == 0 && reserve_m != 50 && started_on == -1 &&
            charged_g == 0 && charged_m == 0;
done:
    printf("{\"kind\":\"addon\",\"scenario\":\"%s\",\"factories\":[%d,%d],\"blocked\":%d,\"parent\":%d,"
           "\"reserved\":[%d,%d],\"started_on\":%d,\"charged\":[%d,%d],\"ok\":%d}\n",
           scenario == 0 || scenario == 1 ? names[scenario] : "?", f[0], f[1], blockers,
           parent, reserve_m, reserve_g, started_on, charged_m, charged_g, ok);
    return ok;
}

/* ---- Forced commit with a small army. Just before minute 6 the Terran
 * opponent's army is cut to three Marines (6 supply, below the normal
 * threshold of 24). An exhausted owner (no local minerals, bank under 100)
 * or an owner past 25 minutes must attack with them; an owner with
 * minerals must keep them home. Losses use the engine's hit_target path;
 * nothing is granted. ---- */
static int army_units(int owner, int *list, int max)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (race_ai_movable_army(&u[i], owner)) { if (n < max) list[n] = i; ++n; }
    return n;
}

static int cut_army(int owner, int keep, int *kept_slots)
{
    int kept = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!race_ai_movable_army(&u[i], owner)) continue;
        if (kept < keep && u[i].type == U_MARINE) { kept_slots[kept++] = i; continue; }
        kill_unit(i);
    }
    /* Cancel queued army products so the army cannot refill at once. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && !sc64_race_is_worker(b[i].queue[b[i].queue_head]))
            b[i].queue_count = 0, b[i].queue_progress = 0;
    return kept;
}

static void exhaust_minerals(int owner)
{
    for (int i = 0; i < MAX_MINERALS; ++i)
        for (int k = 0; k < MAX_BUILDINGS; ++k)
            if (b[k].alive && b[k].owner == owner && race_ai_is_depot(b[k].type) &&
                dist2(m[i].x, m[i].y, b[k].x, b[k].y) < 320 * 320) { m[i].amount = 0; break; }
    race_minerals[owner] = 0;
}

/* Frame at which every kept unit carries an attack-move order to a point
 * outside home range, or -1 within the limit. */
static int await_commit(int owner, const int *kept, int n, int limit, int *committed)
{
    RaceAiCensus c;
    for (int t = 0; t < limit && screen == SCREEN_PLAY && !won && !lost; ++t) {
        int out = 0;
        input(0);
        race_ai_census(owner, &c);
        for (int k = 0; k < n; ++k)
            if (u[kept[k]].alive && u[kept[k]].owner == owner && u[kept[k]].order == ORDER_ATTACK_MOVE &&
                dist2(u[kept[k]].tx, u[kept[k]].ty, c.hx, c.hy) >
                RACE_AI_DEPLOYED_RANGE * RACE_AI_DEPLOYED_RANGE) ++out;
        if (n > 0 && out == n) { *committed = n; return frame; }
    }
    return -1;
}

/* Fixture defense for the late case: opponent army units that reach the
 * passive human's base die through hit_target, so the match lasts. */
static void defend_human(int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && !sc64_race_is_worker(u[i].type) &&
            dist2(u[i].x, u[i].y, race_base_x[0], race_base_y[0]) < 640 * 640) kill_unit(i);
}

static int forced_commit(int scenario)
{
    static const char *names[] = { "exhausted-small-army", "late-small-army",
                                   "exhausted-small-army-resume", "minerals-small-army-stays" };
    int owner = first_bot(), kept = 0, committed = 0, at = -1, ok = 0, supply = -1, slots[3];
    int setup = scenario == 1 ? 45000 - 120 : 10800 - 120;
    uint32_t reference = 0, resumed = 0;
    RaceAiCensus c;
    if (scenario < 0 || scenario > 3 || race_owner_identity(owner) != R_TER) return 0;
    while (frame < setup && screen == SCREEN_PLAY && !won && !lost) {
        input(0);
        if (scenario == 1) defend_human(owner);
    }
    if (screen != SCREEN_PLAY || won || lost) goto done;
    kept = cut_army(owner, 3, slots);
    if (kept != 3) goto done;
    if (scenario == 0 || scenario == 2) exhaust_minerals(owner);
    if (scenario == 1) {
        /* Only the 25-minute rule may apply: local minerals remain. */
        for (int i = 0; i < MAX_MINERALS; ++i)
            if (dist2(m[i].x, m[i].y, race_base_x[race_ai_owner_base(owner)],
                      race_base_y[race_ai_owner_base(owner)]) < 320 * 320) m[i].amount = 1500;
    }
    race_ai_census(owner, &c);
    supply = c.army_supply;
    if (scenario == 2) {
        int ref_at, ref_n = 0;
        if (!save_through_pause()) goto done;
        ref_at = await_commit(owner, slots, 3, 600, &ref_n);
        while (frame < setup + 720 && screen == SCREEN_PLAY && !won && !lost) input(0);
        reference = world_hash();
        game_init(); previous = 0;
        input(KEY_L);
        if (screen != SCREEN_PLAY || !race_mode) goto done;
        at = await_commit(owner, slots, 3, 600, &committed);
        while (frame < setup + 720 && screen == SCREEN_PLAY && !won && !lost) input(0);
        resumed = world_hash();
        ok = ref_at >= 0 && at == ref_at && committed == ref_n && reference == resumed;
    } else if (scenario == 3) {
        at = await_commit(owner, slots, 3, 600, &committed);
        ok = at < 0;
    } else {
        at = await_commit(owner, slots, 3, 600, &committed);
        ok = at >= 0;
    }
    ok = ok && supply > 0 && supply < 12;
done:
    printf("{\"kind\":\"commit\",\"scenario\":\"%s\",\"kept\":%d,\"army_supply\":%d,\"committed\":%d,"
           "\"commit_frame\":%d,\"reference\":\"%08x\",\"resumed\":\"%08x\",\"ok\":%d}\n",
           scenario >= 0 && scenario <= 3 ? names[scenario] : "?", kept, supply, committed, at,
           reference, resumed, ok);
    return ok;
}

/* Synthetic policy regression: nearby depots do not multiply one attacker,
 * and an armed worker does not replace the documented non-worker threat. */
static int threat_census(void)
{
    int owner=first_bot();
    if (race_owner_identity(owner)!=R_TER) return 0;
    for (int scenario=0;scenario<4;scenario++) {
        RaceAiCensus c;
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
        assert(spawn_build(owner,B_COMMAND,320,320,0)>=0);
        assert(spawn_build(owner,B_COMMAND,384,320,0)>=0);
        int workers[2];
        for(int k=0;k<2;k++) {
            workers[k]=spawn_unit(owner,U_SCV,324+8*k,320);
            assert(workers[k]>=0);u[workers[k]].order=ORDER_MINE;
        }
        int enemy_x=scenario==3?384+RACE_AI_THREAT_RANGE:480;
        int enemy=spawn_unit(SIDE,U_MARINE,enemy_x,320);assert(enemy>=0);
        if(scenario==1)assert(spawn_unit(SIDE,U_MARINE,496,320)>=0);
        if(scenario==2)assert(spawn_unit(SIDE,U_SCV,332,320)>=0);
        race_ai_census(owner,&c);
        int expected=scenario==3?0:scenario==1?2:1;
        int ok=c.depots==2 && c.threat_count==expected &&
            (expected?c.threat==enemy && c.threat_x==enemy_x &&
                      c.threat_d==(enemy_x-384)*(enemy_x-384):c.threat<0);
        if(ok) {
            race_ai_military(&c);
            for(int k=0;k<2;k++)
                ok &= u[workers[k]].order==(scenario==1?ORDER_ATTACK_MOVE:ORDER_MINE);
        }
        printf("{\"kind\":\"threat-census\",\"scenario\":%d,\"expected\":%d,\"count\":%d,\"target\":%d,\"ok\":%d}\n",
               scenario,expected,c.threat_count,c.threat,ok);
        if(!ok)return 0;
    }
    return 1;
}

static int parse_races(const char *text, int *races)
{
    int n = 0;
    for (const char *p = text; *p && n < 3; ++p) {
        if (*p == 't' || *p == 'T') races[n++] = R_TER;
        else if (*p == 'z' || *p == 'Z') races[n++] = R_ZERG;
        else if (*p == 'p' || *p == 'P') races[n++] = R_PROTOSS;
    }
    return n;
}

int main(int argc, char **argv)
{
    int map = 1, human = R_TER, ticks = 18000, every = 1800, resume_at = -1, resume_len = 1800;
    int races[3] = { R_TER, R_ZERG, R_PROTOSS }, bots = 3, recover = -1, resume_morph = 0, morph = -1, addon = -1, commit = -1;
    int builder = -1, workers = -1, ai_sunken = 0, threats = 0;
    gba_init();
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--audit")) { audit(); return 0; }
        else if (!strcmp(argv[i], "--map") && i + 1 < argc) map = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--human") && i + 1 < argc) { int r[3]; if (parse_races(argv[++i], r) != 1) return 2; human = r[0]; }
        else if (!strcmp(argv[i], "--bots") && i + 1 < argc) { bots = parse_races(argv[++i], races); if (bots < 1) return 2; }
        else if (!strcmp(argv[i], "--ticks") && i + 1 < argc) ticks = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--every") && i + 1 < argc) every = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--resume-at") && i + 1 < argc) resume_at = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--resume-len") && i + 1 < argc) resume_len = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--recovery") && i + 1 < argc) recover = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--resume-at-morph")) resume_morph = 1;
        else if (!strcmp(argv[i], "--morph-hp") && i + 1 < argc) morph = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--ai-sunken-save")) ai_sunken = 1;
        else if (!strcmp(argv[i], "--threat-census")) threats = 1;
        else if (!strcmp(argv[i], "--addon") && i + 1 < argc) addon = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--commit") && i + 1 < argc) commit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--builder") && i + 1 < argc) builder = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--workers") && i + 1 < argc) workers = atoi(argv[++i]);
        else { fprintf(stderr, "unknown argument %s\n", argv[i]); return 2; }
    }
    launch(map, human, bots, races);
    if (recover >= 0) return recovery(recover) ? 0 : 7;
    if (morph >= 0) return morph_hp(morph) ? 0 : 8;
    if (ai_sunken) return ai_sunken_save() ? 0 : 13;
    if (threats) return threat_census() ? 0 : 14;
    if (addon >= 0) return addon_feasibility(addon) ? 0 : 9;
    if (commit >= 0) return forced_commit(commit) ? 0 : 10;
    if (builder >= 0) return builder_walk(builder) ? 0 : 11;
    if (workers >= 0) return worker_jobs(workers) ? 0 : 12;
    if (resume_at < 0 && !resume_morph) {
        run_ticks(ticks, every, stdout);
        snapshot(stdout, "final");
        if (getenv("AI_DEBUG")) {
            for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
                fprintf(stderr, "B%d o%d raw%d %d,%d hp%d uc%d\n", i, b[i].owner, sc64_race_raw_building(b[i].type), b[i].x, b[i].y, b[i].hp, b[i].under_construction);
            for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && !sc64_race_is_worker(u[i].type))
                fprintf(stderr, "U%d o%d raw%d %d,%d ord%d t%d %d,%d\n", i, u[i].owner, sc64_race_raw_unit(u[i].type), u[i].x, u[i].y, u[i].order, u[i].target, u[i].tx, u[i].ty);
        }
        return 0;
    }
    /* Save through the ordinary pause menu after resume_at ticks, continue
     * the saving process, then reload SRAM into a reinitialized game and
     * require the same digest after the same neutral continuation. */
    if (resume_morph) {
        /* Save while an opponent's Creep Colony is morphing into a Sunken
         * Colony: the in-progress morph holds its source hit points. */
        if (!run_until(resume_at > 0 ? resume_at : 60000, ready_sunken_morph)) {
            snapshot(stdout, "no-morph-before-save"); return 4;
        }
        printf("{\"kind\":\"morph\",\"frame\":%d,\"slot\":%d,\"hp\":%d,\"maxhp\":%d}\n",
               frame, sunken_morph(), b[sunken_morph()].hp, b[sunken_morph()].maxhp);
    } else run_ticks(resume_at, 0, stdout);
    if (screen != SCREEN_PLAY || won || lost) { snapshot(stdout, "ended-before-save"); return 4; }
#ifdef SC64_IMPACT_EVENTS
    /* A save holding more pending impacts than the profile's record refuses
     * itself (SAVE FAILED); they land within a second. Save once they fit,
     * as a player would retry. */
    for (int t = 0; t < 120 && race_impact_count > RACE_IMPACT_SAVE_CAPACITY &&
         screen == SCREEN_PLAY && !won && !lost; ++t) input(0);
    if (race_impact_count > RACE_IMPACT_SAVE_CAPACITY || screen != SCREEN_PLAY || won || lost) {
        snapshot(stdout, "save-not-possible"); return 4;
    }
#endif
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    input(KEY_B);
    uint32_t saved = world_hash();
    int saved_frame = frame;
    snapshot(stdout, "saved");
    input(KEY_A); input(0);
    for (int t = 1; t < resume_len; ++t) input(0);
    uint32_t reference = world_hash();
    int reference_frame = frame;
    snapshot(stdout, "reference");
    game_init(); previous = 0;
    input(KEY_L);
    if (screen != SCREEN_PLAY || !race_mode) { puts("{\"kind\":\"load-failed\"}"); return 5; }
    uint32_t restored = world_hash();
    for (int t = 0; t < resume_len; ++t) input(0);
    uint32_t resumed = world_hash();
    snapshot(stdout, "resumed");
    printf("{\"kind\":\"resume\",\"saved_frame\":%d,\"reference_frame\":%d,\"resumed_frame\":%d,"
           "\"saved\":\"%08x\",\"restored\":\"%08x\",\"reference\":\"%08x\",\"resumed\":\"%08x\",\"match\":%d}\n",
           saved_frame, reference_frame, frame, saved, restored, reference, resumed,
           saved == restored && reference == resumed && reference_frame == frame);
    return saved == restored && reference == resumed && reference_frame == frame ? 0 : 6;
}
