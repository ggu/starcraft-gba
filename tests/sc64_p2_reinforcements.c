/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P2 source trigger capacity and retry regression. */
#include <stdio.h>
#include "../starcraft/game.c"

static int fail(const char *what)
{
    fprintf(stderr, "P2 reinforcement fixture failed: %s\n", what);
    return 1;
}

static void start_p2(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_RACE_MISSION_P2;
    race_completed = 0;
    init_state();
}

static int free_unit_slots(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) n += !u[i].alive;
    return n;
}

static int check_arrival(void)
{
    int i, dragoons = 0, zealots = 0, reavers = 0, fenix = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == 66 && u[i].raw_owner == 13 && u[i].owner == SIDE) ++dragoons;
        if (u[i].raw_type == 65 && u[i].raw_owner == 13 && u[i].owner == SIDE) ++zealots;
        if (u[i].raw_type == 83 && u[i].raw_owner == 13 && u[i].owner == SIDE) ++reavers;
        if (u[i].raw_type == 77 && u[i].raw_owner == 2 && u[i].owner == RESCUABLE) ++fenix;
    }
    return dragoons == 5 && zealots == 5 && reavers == 3 && fenix == 1;
}

static int p2_trigger_units(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_owner == 13) {
        if (u[i].raw_type == 66 && (u[i].x == 1400 || u[i].x == 1408) &&
            u[i].y >= 64 && u[i].y <= 80) ++count;
        else if (u[i].raw_type == 65 && (u[i].x == 1344 || u[i].x == 1352) &&
                 u[i].y >= 96 && u[i].y <= 112) ++count;
        else if (u[i].raw_type == 83 && (u[i].x == 1400 || u[i].x == 1408) &&
                 (u[i].y == 128 || u[i].y == 136)) ++count;
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_type == 77 &&
        u[i].raw_owner == 2 && u[i].owner == RESCUABLE && u[i].x == 1312 && u[i].y == 96)
        ++count;
    return count;
}

static int queued_actor_slots(void)
{
    int i, j, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        for (j = 0; j < b[i].queue_count; ++j) {
            int type = b[i].queue[(b[i].queue_head + j) % 5];
            if (sc64_race_unit_rule(type)) n += type == U_ZERGLING ? 2 : 1;
        }
    return n;
}

static int same_actor_identity(const Unit *a, const Unit *b)
{
    return a->alive == b->alive && a->type == b->type && a->owner == b->owner &&
        a->raw_type == b->raw_type && a->raw_owner == b->raw_owner &&
        a->x == b->x && a->y == b->y && a->hp == b->hp;
}

int main(void)
{
    int i, filler = -1, original_free, expected_room, side_building = -1, enemy_building = -1;
    Unit before_wait[MAX_UNITS];

    start_p2();
    if (race_campaign.mission != SC64_RACE_MISSION_P2 || race_human != R_PROTOSS)
        return fail("loads the authored P2 Protoss mission");
    original_free = free_unit_slots();
    if (original_free < 14) return fail("source start leaves room for the trigger roster");
    expected_room = original_free - 14;
    if (race_campaign_p2_ai_queue_room() != expected_room)
        return fail("reserves fourteen slots for the source event");
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (side_building < 0 && b[i].owner == SIDE && b[i].type == B_NEXUS)
            side_building = i;
        if (enemy_building < 0 && b[i].owner == ENEMY && sc64_race_is_zerg_townhall(b[i].type))
            enemy_building = i;
    }
    if (side_building < 0 || enemy_building < 0)
        return fail("P2 fixture has player and enemy producers for queue accounting");
    b[side_building].queue_head = b[side_building].queue_count = 0;
    b[enemy_building].queue_head = b[enemy_building].queue_count = 0;
    b[side_building].queue[0] = U_PROBE;
    b[enemy_building].queue[0] = U_ZERGLING;
    b[side_building].queue_count = b[enemy_building].queue_count = 1;
    expected_room -= 3; /* paired Zerglings plus one queued Probe */
    if (race_campaign_p2_ai_queue_room() != (expected_room > 0 ? expected_room : 0))
        return fail("counts queued products from both owners and paired Zerglings");

    /* Fill naturally available actor slots to leave only thirteen. The
     * scripted 900-second event must wait intact rather than publish Fenix. */
    b[side_building].queue_count = b[enemy_building].queue_count = 0;
    while (free_unit_slots() > 13) {
        filler = spawn_unit(SIDE, U_PROBE, 160, 160);
        if (filler < 0) return fail("fills fixture slots with ordinary spawned units");
    }
    memcpy(before_wait, u, sizeof(before_wait));
    race_campaign.elapsed_frames = 900u * 30u - 1;
    race_campaign_tick();
    if (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY)
        return fail("does not publish Fenix ready when fewer than fourteen slots are free");
    if (race_campaign.rescue_unit < MAX_UNITS && u[race_campaign.rescue_unit].alive)
        return fail("does not partially deliver Fenix before the whole trigger fits");
    if (race_campaign.phase == SC64_RACE_PHASE_FAILED)
        return fail("does not falsely lose P2 while the arrival is waiting");
    if (free_unit_slots() != 13 || p2_trigger_units() != 0)
        return fail("does not partially add the 13 escort units when Fenix cannot fit");
    for (i = 0; i < MAX_UNITS; ++i)
        if (!same_actor_identity(&before_wait[i], &u[i]))
            return fail("leaves every preexisting actor intact while waiting");
    race_campaign_tick();
    if (free_unit_slots() != 13 || p2_trigger_units() != 0 ||
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED)
        return fail("repeated blocked update stays pending without duplicate arrivals");

    /* Preserve the pending timer and ordinary queues through the actual
     * SRAM codec, then prove the deadline event still retries after load. */
    b[side_building].queue[0] = U_PROBE;
    b[side_building].queue_head = 0; b[side_building].queue_count = 1;
    b[enemy_building].queue[0] = U_ZERGLING;
    b[enemy_building].queue_head = 0; b[enemy_building].queue_count = 1;
    if (!race_save_game()) return fail("saves a pending P2 event at the capacity boundary");
    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_READY;
    race_campaign.rescue_unit = 0;
    if (!race_load_game()) return fail("loads the pending P2 event from SRAM");
    if (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED || free_unit_slots() != 13)
        return fail("restores the pre-arrival deadline without false Fenix loss");
    if (b[side_building].queue_count != 1 || b[enemy_building].queue_count != 1 ||
        race_campaign_p2_ai_queue_room() != 0 || queued_actor_slots() != 3)
        return fail("restores queued-product budget and pending owner queues");
    b[side_building].queue_count = b[enemy_building].queue_count = 0;

    u[filler].alive = 0;
    race_campaign_tick();
    if (!(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) ||
        race_campaign.rescue_unit >= MAX_UNITS || !u[race_campaign.rescue_unit].alive)
        return fail("retries and publishes the event when fourteen slots become free");
    if (!check_arrival() || p2_trigger_units() != 14)
        return fail("delivers exactly five Dragoons, five Zealots, three Reavers, and Fenix");
    race_campaign_tick();
    if (p2_trigger_units() != 14)
        return fail("does not duplicate the source trigger after successful arrival");

    u[race_campaign.rescue_unit].alive = 0;
    race_campaign_tick();
    if (race_campaign.phase != SC64_RACE_PHASE_FAILED ||
        !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_LOST))
        return fail("keeps the original Fenix-death loss condition after arrival");

    start_p2();
    race_campaign.mission = SC64_RACE_MISSION_P1;
    if (race_campaign_p2_ai_queue_room() != MAX_UNITS)
        return fail("does not reserve slots for other campaign missions");
    race_campaign.mission = SC64_RACE_MISSION_P2;
    race_mode = 1;
    if (race_campaign_p2_ai_queue_room() != MAX_UNITS)
        return fail("does not reserve slots in skirmish mode");

    /* At near-full capacity the campaign AI may queue a worker, but must
     * recompute room before adding its army product. Its scheduled guard
     * dispatch still runs at the authored 180-second cadence. */
    start_p2();
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) b[i].queue_count = 0;
    while (free_unit_slots() > 15) {
        if (spawn_unit(SIDE, U_PROBE, 160, 160) < 0)
            return fail("prepares near-full capacity for actual campaign AI");
    }
    race_minerals[ENEMY] = 10000;
    race_gas[ENEMY] = 10000;
    race_campaign.elapsed_frames = 180u * 30u;
    {
        unsigned sent = race_campaign.enemy_units_sent;
        race_campaign_ai_tick();
        if (free_unit_slots() - queued_actor_slots() < 14)
            return fail("AI worker and army queues preserve the P2 trigger budget");
        if (race_campaign.enemy_units_sent <= sent)
            return fail("near-capacity budget leaves the scheduled enemy guard dispatch active");
    }
    puts("P2 capacity retry preserves the complete source reinforcement event and Fenix loss gate.");
    return 0;
}
