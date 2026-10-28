/* Controller-only live run of Desperate Alliance. Build with the unified,
 * imported T3 profile; this test never writes mission/game state directly. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

enum { T3_RUN_LIMIT = SC64_T3_DURATION_TICKS + 300u };
static u16 previous_keys;
static u32 ticks;
static const char *trace_path;

typedef struct {
    u32 tick, elapsed, remaining;
    u16 killed;
    u8 outcome, objective, phase, won, lost, screen;
    u8 side_units, enemy_units, side_buildings, enemy_buildings;
    u16 side_ore, side_gas, enemy_ore, enemy_gas;
    u8 sent, waves;
} T3Checkpoint;
typedef struct { u32 tick; u16 held; } T3InputEvent;

static T3Checkpoint checkpoints[64];
static unsigned checkpoint_count;
static T3InputEvent input_events[16];
static unsigned input_event_count;
static int peak_live_units;
static int live_units(void);

static void controller_frame(u16 held)
{
    if (held != previous_keys && input_event_count <
        sizeof(input_events) / sizeof(input_events[0])) {
        input_events[input_event_count].tick = ticks;
        input_events[input_event_count++].held = held;
    }
    game_update(held, held & (u16)~previous_keys);
    previous_keys = held;
    ++ticks;
    if (screen == SCREEN_PLAY && race_campaign.mission == SC64_RACE_MISSION_T3) {
        int current = live_units();
        if (current > peak_live_units) peak_live_units = current;
        assert(current <= MAX_UNITS);
        assert(current <= MAX_UNITS - SC64_T3_PLAYER_UNIT_RESERVE);
    }
}

static void controller_press(u16 key)
{
    controller_frame(key);
    controller_frame(0);
}

static int counts_for(int owner, int buildings)
{
    int i, count = 0;
    if (buildings) {
        for (i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == owner) ++count;
    } else {
        for (i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == owner) ++count;
    }
    return count;
}

static int live_units(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive) ++count;
    return count;
}

static void checkpoint(void)
{
    T3Checkpoint *c;
    if (ticks % 1000u || checkpoint_count >=
        sizeof(checkpoints) / sizeof(checkpoints[0])) return;
    /* Rendering reuses the common post-production supply totals. Check their
     * consistency through production and the late loss of units/buildings. */
    assert(supply_used == race_owner_supply(SIDE));
    assert(supply_cap == race_owner_supply_cap(SIDE));
    c = &checkpoints[checkpoint_count++];
    c->tick = ticks;
    c->elapsed = race_t3.elapsed_ticks;
    c->remaining = race_t3.remaining_ticks;
    c->killed = race_t3_buildings_killed;
    c->outcome = race_t3.outcome;
    c->objective = race_t3.objective;
    c->phase = race_campaign.phase;
    c->won = won;
    c->lost = lost;
    c->screen = screen;
    c->side_units = (u8)counts_for(SIDE, 0);
    c->enemy_units = (u8)counts_for(ENEMY, 0);
    c->side_buildings = (u8)counts_for(SIDE, 1);
    c->enemy_buildings = (u8)counts_for(ENEMY, 1);
    c->side_ore = (u16)race_minerals[SIDE];
    c->side_gas = (u16)race_gas[SIDE];
    c->enemy_ore = (u16)race_minerals[ENEMY];
    c->enemy_gas = (u16)race_gas[ENEMY];
    c->sent = race_campaign.enemy_units_sent;
    c->waves = race_campaign.enemy_wave;
    game_render();
}

static void write_trace(void)
{
    FILE *f;
    unsigned i;
    if (!(f = fopen(trace_path, "w"))) return;
    fprintf(f, "{\"trace_schema\":1,\"mode\":\"controller-only-neutral-run\","
               "\"mission\":%d,\"campaign_mission\":%u,\"selected_mission\":%d,"
               "\"total_ticks\":%u,\"outcome\":%u,\"won\":%d,\"lost\":%d,"
               "\"elapsed_ticks\":%u,\"remaining_ticks\":%u,\"objective\":%u,"
               "\"buildings_killed\":%u,\"initial_units\":138,\"initial_buildings\":31,"
               "\"peak_live_units\":%d,"
               "\"events\":[",
            SC64_RACE_MISSION_T3, race_campaign.mission, race_selected_mission,
            ticks, race_t3.outcome, won, lost, race_t3.elapsed_ticks,
            race_t3.remaining_ticks, race_t3.objective, race_t3_buildings_killed,
            peak_live_units);
    for (i = 0; i < input_event_count; ++i)
        fprintf(f, "%s{\"tick\":%u,\"held\":%u}", i ? "," : "",
                input_events[i].tick, input_events[i].held);
    fprintf(f, "],\"checkpoints\":[");
    for (i = 0; i < checkpoint_count; ++i) {
        const T3Checkpoint *c = &checkpoints[i];
        fprintf(f, "%s{\"tick\":%u,\"elapsed\":%u,\"remaining\":%u,"
                   "\"killed\":%u,\"outcome\":%u,\"objective\":%u,\"phase\":%u,"
                   "\"won\":%u,\"lost\":%u,\"screen\":%u,"
                   "\"side_units\":%u,\"enemy_units\":%u,"
                   "\"side_buildings\":%u,\"enemy_buildings\":%u,"
                   "\"side_ore\":%u,\"side_gas\":%u,\"enemy_ore\":%u,"
                   "\"enemy_gas\":%u,\"sent\":%u,\"waves\":%u}",
                i ? "," : "", c->tick, c->elapsed, c->remaining, c->killed,
                c->outcome, c->objective, c->phase, c->won, c->lost, c->screen,
                c->side_units, c->enemy_units, c->side_buildings, c->enemy_buildings,
                c->side_ore, c->side_gas, c->enemy_ore, c->enemy_gas, c->sent, c->waves);
    }
    fprintf(f, "]}\n");
    fclose(f);
}

static void launch_t3_through_menus(void)
{
    game_init();
    previous_keys = 0;
    controller_press(KEY_A); /* HOME -> campaign mission list. */
    assert(race_front.page == SC_FRONT_MISSION);
    controller_press(KEY_RIGHT); /* T3 is on campaign page 2. */
    assert(race_front.mission_page == 1);
    controller_press(KEY_A); /* Select T3, opening the briefing. */
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 6);
    controller_press(KEY_START); /* Explicit deploy from the briefing. */
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 6);
    assert(race_campaign.mission == 6 && sc64_t3_state_valid(&race_t3));
    assert(mission_unsupported_count == 0);
    assert(counts_for(SIDE, 0) + counts_for(ENEMY, 0) == 138);
    assert(counts_for(SIDE, 1) + counts_for(ENEMY, 1) == 31);
    assert(race_minerals[SIDE] == 100 && race_gas[SIDE] == 0);
    game_render();
}

int main(void)
{
    u32 i;
    trace_path = getenv("SC64_T3_TRACE_PATH");
    if (!trace_path) trace_path = ".local/sc64/t3-controller-run.json";
    gba_init();
    launch_t3_through_menus();
    for (i = 0; i < T3_RUN_LIMIT && !won && !lost; ++i) {
        controller_frame(0);
        checkpoint();
    }
    game_render();
    write_trace();
    if (won && !lost) {
        assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
        assert(race_t3.outcome == SC64_T3_WON);
        printf("T3 controller run won: ticks=%u elapsed=%u remaining=%u objective=%u checkpoints=%u\n",
               ticks, race_t3.elapsed_ticks, race_t3.remaining_ticks,
               race_t3.objective, checkpoint_count);
        return 0;
    }
    fprintf(stderr, "T3 controller run did not win: ticks=%u elapsed=%u remaining=%u "
                    "side_buildings=%d enemy_buildings=%d killed=%u units=%d/%d trace=%s\n",
            ticks, race_t3.elapsed_ticks, race_t3.remaining_ticks,
            counts_for(SIDE, 1), counts_for(ENEMY, 1), race_t3_buildings_killed,
            counts_for(SIDE, 0), counts_for(ENEMY, 0), trace_path);
    return 1;
}
