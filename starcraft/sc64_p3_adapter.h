#ifndef SC64_P3_ADAPTER_H
#define SC64_P3_ADAPTER_H

#if !defined(SC64_P3_RUNTIME) || !defined(SC64_P3_ENABLED) || !defined(SC64_UNIFIED)
#error "Higher Ground runtime requires SC64_P3_RUNTIME, SC64_P3_ENABLED and SC64_UNIFIED"
#endif

#include "sc64_p3_model.h"

/* Engine-policy timings, in the engine's 30 Hz simulation ticks. The source
 * CHK stores millisecond waits; transmission audio is absent from this build. */
enum {
    SC64_P3_TICKS_PER_SECOND = 30,
    SC64_P3_TX_HOLD_TICKS = 90,
    SC64_P3_PLAYER_UNIT_RESERVE = 16,
    SC64_P3_FORCE_PRODUCTION_PERIOD = 90 * 30,
    SC64_P3_FORCE_ATTACK_PERIOD = 180 * 30,
    SC64_P3_FORCE_WAVE_SIZE = 2
};

static void race_p3_assign_idle_workers(void);

static u16
race_p3_action_duration(const Sc64P3State *s)
{
    if (!s) return 0;
    switch (s->sequence) {
    case SC64_P3_SEQ_WIN_WAIT_2000: return 60;
    case SC64_P3_SEQ_WIN_TX_1:
    case SC64_P3_SEQ_WIN_TX_2: return SC64_P3_TX_HOLD_TICKS;
    case SC64_P3_SEQ_WIN_WAIT_3000: return 90;
    default: return 0;
    }
}

static void
race_p3_init(void)
{
    (void)sc64_p3_model_init(&race_p3);
    race_p3_action_ticks = 0;
    minerals = SC64_P3_HUMAN_MINERALS;
    gas = SC64_P3_HUMAN_GAS;
    race_minerals[SIDE] = minerals;
    race_gas[SIDE] = gas;
    race_campaign.objective_stage = race_p3.sequence;
    race_p3_assign_idle_workers();
}

static const char *
race_p3_objective_text(void)
{
    return "Destroy both Zerg colonies.";
}

static const char *
race_p3_victory_text(void)
{
    return "HIGHER GROUND SECURED";
}

static void
race_p3_count_triggers(Sc64P3Input *in)
{
    int i;
    in->human_any_units = 0;
    in->foes_buildings = 0;
    in->force2_any_units = 0;
    in->force3_any_units = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == SIDE) ++in->human_any_units;
        if (b[i].owner == ENEMY || b[i].owner == 4) {
            ++in->foes_buildings;
            if (b[i].owner == ENEMY) ++in->force2_any_units;
            else ++in->force3_any_units;
        }
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == SIDE) ++in->human_any_units;
        else if (u[i].owner == ENEMY) ++in->force2_any_units;
        else if (u[i].owner == 4) ++in->force3_any_units;
    }
    in->paused = 0; /* The outer game update does not call this while paused. */
    in->complete_action = 0;
}

static int
race_p3_live_and_queued(void)
{
    int i, j, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) ++n;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        for (j = 0; j < b[i].queue_count; ++j) {
            int type = b[i].queue[(b[i].queue_head + j) % 5];
            n += type == U_ZERGLING ? 2 : 1;
        }
    return n;
}

static void
race_p3_assign_idle_workers(void)
{
    static const int owners[2] = { ENEMY, 4 };
    int k, i;
    for (k = 0; k < 2; ++k) for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owners[k] &&
            sc64_race_is_worker(u[i].type) && u[i].order == ORDER_IDLE) {
            int patch = nearest_mineral(u[i].x, u[i].y);
            if (patch >= 0) {
                u[i].order = ORDER_MINE;
                u[i].target = patch;
                u[i].work_timer = 0;
            }
        }
}

static void
race_p3_paid_production(void)
{
    static const int owners[2] = { ENEMY, 4 };
    int k, i;
    if (race_p3_live_and_queued() + 2 > MAX_UNITS - SC64_P3_PLAYER_UNIT_RESERVE)
        return;
    for (k = 0; k < 2; ++k) {
        int owner = owners[k];
        if (race_p3_live_and_queued() + 2 >
            MAX_UNITS - SC64_P3_PLAYER_UNIT_RESERVE) return;
        /* Only queue paid Zerglings from already-imported town halls. Each
         * force keeps its own wallet and producer queue. */
        for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
            b[i].owner == owner && sc64_race_is_zerg_townhall(b[i].type) &&
            b[i].queue_count < 2) {
            if (race_queue_unit(owner, i, U_ZERGLING)) break;
        }
    }
}

static void
race_p3_release_guard_waves(void)
{
    static const int owners[2] = { ENEMY, 4 };
    int target_x = cx, target_y = cy, found = 0, i, k, sent_total = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        target_x = u[i].x; target_y = u[i].y; found = 1; break;
    }
    if (!found) for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) {
            target_x = b[i].x; target_y = b[i].y; break;
        }
    for (k = 0; k < 2; ++k) {
        int sent = 0, owner = owners[k];
        for (i = 0; i < MAX_UNITS && sent < SC64_P3_FORCE_WAVE_SIZE; ++i)
            if (u[i].alive && u[i].owner == owner && u[i].order == ORDER_IDLE &&
                !sc64_race_is_worker(u[i].type) &&
                (sc64_race_unit_damage(u[i].type) > 0 ||
                 sc64_race_unit_damage_vs(u[i].type, 1) > 0)) {
                u[i].order = ORDER_ATTACK_MOVE;
                u[i].tx = target_x; u[i].ty = target_y;
                ++sent; ++sent_total;
                ++race_campaign.enemy_units_sent;
            }
        if (sent) ++race_campaign.enemy_wave;
    }
    (void)sent_total;
}

static void
race_p3_apply_result(u16 events)
{
    if (events & SC64_P3_EVENT_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << SC64_P3_GLOBAL_ID;
        won = 1; lost = 0;
    } else if (events & SC64_P3_EVENT_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        won = 0; lost = 1;
    }
}

static void
race_p3_campaign_tick(void)
{
    Sc64P3Input input;
    u16 events = 0;
    if (race_p3.outcome == SC64_P3_RUNNING) {
        race_p3_count_triggers(&input);
        if (race_p3.sequence != SC64_P3_SEQ_NONE) {
            if (race_p3_action_ticks) --race_p3_action_ticks;
            input.complete_action = race_p3_action_ticks == 0;
        }
        events = sc64_p3_model_step(&race_p3, &input);
        /* The model may advance an action only after the adapter's hold has
         * elapsed. A new action starts at its full duration this update. */
        if (race_p3.sequence != SC64_P3_SEQ_NONE && input.complete_action)
            race_p3_action_ticks = race_p3_action_duration(&race_p3);
        else if (events & SC64_P3_EVENT_WIN_WAIT)
            race_p3_action_ticks = race_p3_action_duration(&race_p3);
        if (events & SC64_P3_EVENT_FORCE2_START) {
            race_minerals[ENEMY] = SC64_P3_FORCE2_COMBINED_RESOURCES;
            race_gas[ENEMY] = SC64_P3_FORCE2_COMBINED_RESOURCES;
        }
        if (events & SC64_P3_EVENT_FORCE3_START) {
            race_minerals[4] = SC64_P3_FORCE3_COMBINED_RESOURCES;
            race_gas[4] = SC64_P3_FORCE3_COMBINED_RESOURCES;
        }
        if (race_p3.outcome != SC64_P3_RUNNING)
            race_p3_action_ticks = 0;
        race_campaign.objective_stage = race_p3.sequence;
        race_p3_apply_result(events);

        /* Authored substitute for opaque source RunAI scripts: independently
         * funded forces train modestly and dispatch existing guards. */
        if (race_p3.outcome == SC64_P3_RUNNING && race_campaign.elapsed_frames) {
            if (race_campaign.elapsed_frames % (10 * SC64_P3_TICKS_PER_SECOND) == 0)
                race_p3_assign_idle_workers();
            if (race_campaign.elapsed_frames % SC64_P3_FORCE_PRODUCTION_PERIOD == 0)
                race_p3_paid_production();
            if (race_campaign.elapsed_frames % SC64_P3_FORCE_ATTACK_PERIOD == 0)
                race_p3_release_guard_waves();
        }
    } else {
        race_p3_action_ticks = 0;
        race_campaign.objective_stage = race_p3.sequence;
    }
}

#endif /* SC64_P3_ADAPTER_H */
