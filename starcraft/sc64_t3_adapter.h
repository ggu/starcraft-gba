#ifndef SC64_T3_ADAPTER_H
#define SC64_T3_ADAPTER_H

/* Mission-specific controller bridge for 008/00C. The trigger state and
 * source-buildings-killed counter live in game.c so race_save_adapter.h can
 * persist them. This adapter approximates T3's opening AI with the imported
 * guard force and a timed all-combat-unit assault; it does not execute the
 * original AI scripts or create trigger-authored reinforcement units. */
#include "sc64_t3.h"

enum {
    /* Hold sixteen of the 160 global unit slots open for player production. */
    SC64_T3_PLAYER_UNIT_RESERVE = 16,
    SC64_T3_PRODUCTION_PERIOD = 300 * 30,
    SC64_T3_PRESSURE_PERIOD = 180 * 30
};

static void
race_t3_init(void)
{
    (void)sc64_t3_init(&race_t3);
    race_t3_buildings_killed = 0;
    race_campaign.objective_stage = 0;
}

static const char *
race_t3_objective_text(void)
{
    return race_t3.objective ?
        "Destroy the Zerg Hatchery before time expires, or survive the countdown." :
        "Survive 30 minutes. Kill an enemy building with at least 2 minutes left to unlock the Hatchery win.";
}

static const char *
race_t3_victory_text(void)
{
    return "THE DESPERATE ALLIANCE WON";
}

static int
race_t3_total_units(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) ++count;
    return count;
}

static int
race_t3_pending_enemy_units(void)
{
    int i, j, count = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == ENEMY)
        for (j = 0; j < b[i].queue_count; ++j) {
            int type = b[i].queue[(b[i].queue_head + j) % 5];
            count += type == U_ZERGLING ? 2 : 1;
        }
    return count;
}

static int
race_t3_queue_paid_zerglings(void)
{
    int i;
    if (race_t3_total_units() + race_t3_pending_enemy_units() + 2 >
        MAX_UNITS - SC64_T3_PLAYER_UNIT_RESERVE) return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        b[i].owner == ENEMY && b[i].type == B_HATCHERY && b[i].queue_count < 2)
        return race_queue_unit(ENEMY, i, U_ZERGLING);
    return 0;
}

static void
race_t3_release_attack_force(int limit)
{
    int i, sent = 0, target_x = cx, target_y = cy, found = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        target_x = u[i].x;
        target_y = u[i].y;
        found = 1;
        break;
    }
    if (!found) {
        for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE) {
            target_x = b[i].x;
            target_y = b[i].y;
            break;
        }
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY &&
        sc64_race_unit_damage(u[i].type) > 0 &&
        (limit <= 0 || (sent < limit && u[i].order == ORDER_IDLE))) {
        u[i].order = ORDER_ATTACK_MOVE;
        u[i].tx = target_x;
        u[i].ty = target_y;
        ++race_campaign.enemy_units_sent;
        ++sent;
    }
    if (sent) ++race_campaign.enemy_wave;
}

static void
race_t3_campaign_tick(void)
{
    Sc64T3Input input;
    u16 events;
    int i;
    input.force1_buildings_killed = race_t3_buildings_killed;
    input.human_buildings = input.force2_hatcheries = input.force2_any_units = 0;
    /* Derive all building predicates in one pass. Any Unit includes buildings,
     * so the unit pass is only needed after the last enemy building is gone. */
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == SIDE) input.human_buildings = 1;
        else if (b[i].owner == ENEMY) {
            input.force2_any_units = 1;
            if (b[i].type == B_HATCHERY) ++input.force2_hatcheries;
        }
    }
    if (!input.force2_any_units)
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
            input.force2_any_units = 1;
            break;
        }
    input.paused = 0;
    events = sc64_t3_step(&race_t3, &input);

    if (events & SC64_T3_EVENT_FORCE2_AI_AND_RESOURCES) {
        /* T3 TRIG 5 sets Force 2's raw Ore+Gas amount to 5000. */
        race_minerals[ENEMY] = 5000;
        race_gas[ENEMY] = 5000;
    }
    if (events & SC64_T3_EVENT_ALTERNATE_OBJECTIVE) {
        race_campaign.objective_stage = 1;
        race_campaign_set_message(19);
    }
    /* The Force 2 opening grant supplies the authored Ore+Gas budget. After
     * that, queue one paid Zergling pair per five minutes while preserving
     * sixteen global unit slots for player production. */
    if (race_campaign.elapsed_frames % SC64_T3_PRODUCTION_PERIOD == 0)
        (void)race_t3_queue_paid_zerglings();
    /* A deliberately bounded pressure model: every three minutes, send up to
     * two existing damaging units that are still guarding the base. */
    if (race_campaign.elapsed_frames % SC64_T3_PRESSURE_PERIOD == 0)
        race_t3_release_attack_force(2);
    if (events & SC64_T3_EVENT_AI_START) {
        /* T3 TRIG 8 invokes an authored AI script at <=120 seconds. Approximate
         * that late pressure by releasing the existing combat roster only. */
        race_t3_release_attack_force(0);
        race_campaign_set_message(15);
    }
    if (events & SC64_T3_EVENT_REMINDER_10_MIN) race_campaign_set_message(16);
    if (events & SC64_T3_EVENT_REMINDER_5_MIN) race_campaign_set_message(17);
    if (events & SC64_T3_EVENT_REMINDER_1_MIN) race_campaign_set_message(18);

    if (events & SC64_T3_EVENT_DEFEAT) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        lost = 1;
        won = 0;
        return;
    }
    if (events & SC64_T3_EVENT_VICTORY) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << race_campaign.mission;
        won = 1;
    }
    /* game_update recomputes supply in race_tick_state after production. */
}

#endif /* SC64_T3_ADAPTER_H */
