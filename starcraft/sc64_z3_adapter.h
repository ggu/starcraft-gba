#ifndef SC64_Z3_ADAPTER_H
#define SC64_Z3_ADAPTER_H

/* Bounded authored adapter for Zerg 3 / The New Dominion. The trigger gates
 * and their raw action sequence come from the verified local SC64 CHK audit;
 * enemy production and presentation duration below are explicitly engine
 * policies, not claims to execute the original opaque Force 2 AI script. */
#include "sc64_z3.h"

enum {
    SC64_Z3_GLOBAL_MISSION_ID = 14,
    SC64_Z3_TERRAN_MARINE_PERIOD = 120 * 30,
    SC64_Z3_TERRAN_ATTACK_PERIOD = 180 * 30,
    SC64_Z3_TERRAN_WAVE_SIZE = 2,
    SC64_Z3_PLAYER_UNIT_RESERVE = 16,
    SC64_Z3_TX_HOLD_TICKS = 90,
    SC64_Z3_LOSS_TEXT_HOLD_TICKS = 90,
    SC64_Z3_POLICY_TICKS_PER_SECOND = 30
};

/* Runtime state is declared by game.c and persisted by the parent save
 * adapter. The caller invokes race_z3_campaign_tick once per unpaused 30 Hz
 * campaign update; pause is therefore handled by the outer update loop. */
static u16
race_z3_action_duration(const Sc64Z3State *s)
{
    if (!s) return 0;
    switch (s->sequence) {
    case SC64_Z3_SEQ_WIN_WAIT_1000:
    case SC64_Z3_SEQ_LOSS_WAIT_1000:
        return 30; /* adapter policy: 1000 raw wait units -> 30 ticks */
    case SC64_Z3_SEQ_WIN_TX_1:
    case SC64_Z3_SEQ_WIN_TX_2:
    case SC64_Z3_SEQ_WIN_TX_3:
    case SC64_Z3_SEQ_LOSS_MESSAGE:
        return SC64_Z3_TX_HOLD_TICKS;
    case SC64_Z3_SEQ_WIN_WAIT_500:
    case SC64_Z3_SEQ_LOSS_WAIT_500:
        return 15; /* adapter policy: 500 raw wait units -> 15 ticks */
    case SC64_Z3_SEQ_LOSS_WAV:
        return 13; /* ceil(403 * 30 / 1000); adapter timing policy */
    default:
        return 0;
    }
}

static void
race_z3_init(void)
{
    (void)sc64_z3_init(&race_z3);
    race_z3_action_ticks = 0;
    minerals = SC64_Z3_MISSION_ORE;
    gas = SC64_Z3_MISSION_GAS;
    race_minerals[SIDE] = minerals;
    race_gas[SIDE] = gas;
    race_campaign.objective_stage = 0;
}

static const char *
race_z3_objective_text(void)
{
    return "Protect the Chrysalis. Eliminate the Terran presence.";
}

static const char *
race_z3_victory_text(void)
{
    return "THE NEW DOMINION SECURED";
}

static void
race_z3_count_trigger_inputs(Sc64Z3Input *in)
{
    int i;
    in->foes_any_units = 0;
    in->foes_buildings = 0;
    in->force2_any_units = 0;
    in->human_chrysalis = 0;
    /* Source Force2/Foes Any Unit selector 229 includes buildings. This map's
     * Force 2 is imported as ENEMY, opposed to human SIDE. */
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY) {
        ++in->foes_any_units;
        ++in->foes_buildings;
        ++in->force2_any_units;
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == ENEMY) {
            ++in->foes_any_units;
            ++in->force2_any_units;
        } else if (u[i].owner == SIDE &&
                   u[i].raw_type == SC64_Z3_UNIT_MATURE_CHRYSALIS) {
            ++in->human_chrysalis;
        }
    }
    in->paused = 0;
}

static int
race_z3_total_units_and_queued(void)
{
    int i, j, count = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) ++count;
    /* Training already queued consumes future global slots too. */
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        for (j = 0; j < b[i].queue_count; ++j)
            count += b[i].queue[(b[i].queue_head + j) % 5] == U_ZERGLING ? 2 : 1;
    return count;
}

static void
race_z3_paid_reinforcement(void)
{
    int i, used = race_z3_total_units_and_queued();
    if (used >= MAX_UNITS - SC64_Z3_PLAYER_UNIT_RESERVE) return;
    /* New Dominion already imports one Terran Barracks. Queue only paid
     * Marines from that authored producer; do not call the generic AI, which
     * can add unbounded buildings and workers. This is bounded pressure
     * authored for this engine, not the source RunAIScriptAtLocation script. */
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        b[i].owner == ENEMY && b[i].type == B_BARRACKS && b[i].queue_count < 2) {
        if (race_queue_unit(ENEMY, i, U_MARINE)) return;
    }
}

static void
race_z3_release_small_wave(void)
{
    int i, target_x = cx, target_y = cy, sent = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        target_x = u[i].x; target_y = u[i].y; break;
    }
    if (i == MAX_UNITS)
        for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE) {
            target_x = b[i].x; target_y = b[i].y; break;
        }
    for (i = 0; i < MAX_UNITS && sent < SC64_Z3_TERRAN_WAVE_SIZE; ++i)
        if (u[i].alive && u[i].owner == ENEMY && u[i].order == ORDER_IDLE &&
            !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0) {
            u[i].order = ORDER_ATTACK_MOVE;
            u[i].tx = target_x; u[i].ty = target_y;
            ++sent;
            ++race_campaign.enemy_units_sent;
        }
    if (sent) ++race_campaign.enemy_wave;
}

static void
race_z3_apply_result(u16 events)
{
    if (events & SC64_Z3_EVENT_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << SC64_Z3_GLOBAL_MISSION_ID;
        won = 1;
        lost = 0;
    } else if (events & SC64_Z3_EVENT_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        won = 0;
        lost = 1;
    }
}

static void
race_z3_campaign_tick(void)
{
    u16 events = 0;
    if (race_z3.outcome == SC64_Z3_RUNNING) {
        if (race_z3.sequence != SC64_Z3_SEQ_NONE) {
            if (race_z3_action_ticks) --race_z3_action_ticks;
            if (!race_z3_action_ticks) {
                events = sc64_z3_complete_action(&race_z3, 0);
                race_z3_action_ticks = race_z3_action_duration(&race_z3);
            }
        } else {
            Sc64Z3Input input;
            u8 switch7_before = race_z3.switch7;
            race_z3_count_trigger_inputs(&input);
            events = sc64_z3_step(&race_z3, &input);
            if (!switch7_before && race_z3.switch7)
                race_campaign.objective_stage = 1;
            if (events & SC64_Z3_EVENT_FORCE2_START) {
                /* TRIG 1 sets resource selector 2 (Ore and Gas) to 8000, once. */
                race_minerals[ENEMY] = SC64_Z3_FORCE2_RESOURCES;
                race_gas[ENEMY] = SC64_Z3_FORCE2_RESOURCES;
            }
            if (race_z3.sequence != SC64_Z3_SEQ_NONE)
                race_z3_action_ticks = race_z3_action_duration(&race_z3);
        }
    } else {
        /* Terminal result states never retain a running action timer. */
        race_z3_action_ticks = 0;
    }

    /* Bounded generic-pressure substitute: paid Marine training every two
     * minutes, up to MAX_UNITS minus 16 live+queued units; this leaves slots for
     * human production. Periodically release two existing idle combat guards.
     * The unknown source AI script may behave differently. */
    if (race_campaign.elapsed_frames &&
        race_campaign.elapsed_frames % SC64_Z3_TERRAN_MARINE_PERIOD == 0)
        race_z3_paid_reinforcement();
    if (race_campaign.elapsed_frames &&
        race_campaign.elapsed_frames % SC64_Z3_TERRAN_ATTACK_PERIOD == 0)
        race_z3_release_small_wave();

    race_z3_apply_result(events);
}

#endif /* SC64_Z3_ADAPTER_H */
