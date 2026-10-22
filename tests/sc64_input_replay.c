/* Replay a bounded text controller trace through the real game_update path. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

#define REPLAY_MAX_EVENTS 2048
#define REPLAY_MAX_TICKS 200000UL
#define REPLAY_MAX_CHECKPOINTS (REPLAY_MAX_TICKS / 1000UL + 1UL)

typedef struct { unsigned long tick; u16 held; } ReplayEvent;
typedef struct {
    unsigned long tick;
    int frame, minerals, gas, screen, won, lost;
    int marines, firebats, scvs, zerglings, infested_cc;
    int raynor_alive, raynor_owner, objective;
#ifdef SC64_TERRAN_CAMPAIGN
    int campaign_mission, campaign_completed;
#endif
} ReplayCheckpoint;

static ReplayEvent replay_events[REPLAY_MAX_EVENTS];
static ReplayCheckpoint replay_checkpoints[REPLAY_MAX_CHECKPOINTS];
static u16 previous_held;

static int
replay_count_units(int owner, int type)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) ++count;
    return count;
}

static int
replay_count_buildings(int owner, int type)
{
    int i, count = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) ++count;
    return count;
}

static ReplayCheckpoint
replay_state(unsigned long tick)
{
    ReplayCheckpoint state;
    int i;
    state.tick = tick;
    state.frame = frame;
    state.minerals = minerals;
    state.gas = gas;
    state.screen = screen;
    state.won = won;
    state.lost = lost;
    state.marines = replay_count_units(SIDE, U_MARINE);
    state.firebats = replay_count_units(SIDE, U_FIREBAT);
    state.scvs = replay_count_units(SIDE, U_SCV);
    state.zerglings = replay_count_units(ENEMY, U_ZERGLING);
    state.infested_cc = replay_count_buildings(ENEMY, B_INFESTED_COMMAND);
    state.raynor_alive = 0;
    state.raynor_owner = -1;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_RAYNOR) {
            state.raynor_alive = 1;
            state.raynor_owner = u[i].owner;
            break;
        }
#ifdef SC64_BACKWATER
    state.objective = sc64_backwater_objective(&backwater);
#else
    state.objective = mission_objective_stage;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    state.campaign_mission = campaign_mission;
    state.campaign_completed = campaign_completed;
#endif
    return state;
}

static void
replay_write_state(FILE *out, const ReplayCheckpoint *s)
{
    fprintf(out,
        "{\"tick\":%lu,\"frame\":%d,\"minerals\":%d,\"gas\":%d,\"screen\":%d,"
        "\"won\":%d,\"lost\":%d,\"marines\":%d,\"firebats\":%d,\"scvs\":%d,"
        "\"zerglings\":%d,\"infested_cc\":%d,\"raynor_alive\":%d,"
        "\"raynor_owner\":%d,\"objective\":%d",
        s->tick, s->frame, s->minerals, s->gas, s->screen, s->won, s->lost,
        s->marines, s->firebats, s->scvs, s->zerglings, s->infested_cc,
        s->raynor_alive, s->raynor_owner, s->objective);
#ifdef SC64_TERRAN_CAMPAIGN
    fprintf(out, ",\"campaign_mission\":%d,\"campaign_completed\":%d",
        s->campaign_mission, s->campaign_completed);
#endif
    fputc('}', out);
}

int
main(int argc, char **argv)
{
    FILE *input, *output;
    unsigned long total_ticks, tick, previous_tick = 0;
    unsigned int event_count, i, event_index = 0;
    u16 held = 0;
    int have_previous = 0;
    unsigned int checkpoint_count = 0;
    ReplayCheckpoint final_state;
    if (argc != 3) {
        fprintf(stderr, "usage: %s INPUTTEXT OUTPUTJSON\n", argv[0]);
        return 64;
    }
    input = fopen(argv[1], "r");
    if (!input) { perror(argv[1]); return 65; }
    if (fscanf(input, "%lu %u", &total_ticks, &event_count) != 2 ||
        total_ticks == 0 || total_ticks > REPLAY_MAX_TICKS ||
        event_count > REPLAY_MAX_EVENTS) {
        fclose(input);
        fprintf(stderr, "invalid replay header (maximum %lu ticks, %d events)\n",
            REPLAY_MAX_TICKS, REPLAY_MAX_EVENTS);
        return 66;
    }
    for (i = 0; i < event_count; ++i) {
        unsigned int keys;
        if (fscanf(input, "%lu %u", &replay_events[i].tick, &keys) != 2 ||
            replay_events[i].tick >= total_ticks || keys > 0xffffu ||
            (have_previous && replay_events[i].tick <= previous_tick)) {
            fclose(input);
            fprintf(stderr, "invalid replay event %u\n", i);
            return 67;
        }
        replay_events[i].held = (u16)keys;
        previous_tick = replay_events[i].tick;
        have_previous = 1;
    }
    if (fclose(input) != 0) { perror(argv[1]); return 65; }
    output = fopen(argv[2], "w");
    if (!output) { perror(argv[2]); return 65; }

    game_init();
    for (tick = 0; tick < total_ticks; ++tick) {
        u16 pressed;
        if (event_index < event_count && replay_events[event_index].tick == tick)
            held = replay_events[event_index++].held;
        pressed = held & (u16)~previous_held;
        game_update(held, pressed);
        previous_held = held;
        if ((tick + 1) % 1000 == 0 && checkpoint_count < REPLAY_MAX_CHECKPOINTS)
            replay_checkpoints[checkpoint_count++] = replay_state(tick + 1);
    }
    if (event_index != event_count) {
        fclose(output);
        fprintf(stderr, "replay did not consume every event\n");
        return 68;
    }
    final_state = replay_state(total_ticks);
    fprintf(output, "{\"total_ticks\":%lu,\"events\":[", total_ticks);
    for (i = 0; i < event_count; ++i)
        fprintf(output, "%s{\"tick\":%lu,\"held\":%u}", i ? "," : "",
            replay_events[i].tick, replay_events[i].held);
    fprintf(output, "],\"checkpoints\":[");
    for (i = 0; i < checkpoint_count; ++i) {
        if (i) fputc(',', output);
        replay_write_state(output, &replay_checkpoints[i]);
    }
    fprintf(output, "],\"expected\":");
    replay_write_state(output, &final_state);
    fprintf(output, "}\n");
    if (fclose(output) != 0) { perror(argv[2]); return 65; }

#ifdef SC64_TERRAN_CAMPAIGN
    if (!final_state.won || final_state.lost || final_state.campaign_mission != 1 ||
        final_state.campaign_completed != 3 || !final_state.raynor_alive ||
        final_state.raynor_owner != SIDE) {
        fprintf(stderr, "campaign replay incomplete: frame=%d mission=%d completed=%d won=%d lost=%d Raynor=%d/%d\n",
            final_state.frame, final_state.campaign_mission, final_state.campaign_completed,
            final_state.won, final_state.lost, final_state.raynor_alive, final_state.raynor_owner);
        return 1;
    }
#endif
    printf("Replay passed: ticks=%lu frame=%d won=%d lost=%d\n",
        total_ticks, final_state.frame, final_state.won, final_state.lost);
    return 0;
}
