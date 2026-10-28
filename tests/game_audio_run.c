/* Presentation-only audio hooks. "hooks" checks event detection on fixtures;
 * without arguments a long controller-driven run prints "state <hash>", which
 * must match a build compiled with -DGAME_AUDIO_OFF. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

extern u32 audio_host_tones, audio_host_noises, audio_host_samples;
extern int audio_host_last_sample, audio_host_music;

static u16 previous_held;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL game audio: %s\n", message);
    return 1;
}

static void tick(u16 held)
{
    game_audio_host_held = held;
    game_update(held, held & (u16)~previous_held);
    previous_held = held;
    game_render();
}

static u32 events(int kind) { return game_audio_events[kind]; }

static void clear_events(void)
{
    for (int i = 0; i < GAME_AUDIO_EVENT_KINDS; i++) game_audio_events[i] = 0;
}

/* Advance one frame without simulation, so fixture edits are the only change. */
static void observe_next(void)
{
    ++frame;
    game_render();
}

static void put_unit(int slot, int owner, int type, int x, int y)
{
    memset(&u[slot], 0, sizeof u[slot]);
    u[slot].owner = owner;
    u[slot].type = type;
    u[slot].x = u[slot].tx = x;
    u[slot].y = u[slot].ty = y;
    u[slot].hp = u[slot].maxhp = 40;
    u[slot].alive = 1;
    u[slot].order = ORDER_IDLE;
    u[slot].target = -1;
}

static void fixture(void)
{
    previous_held = 0;
    init_state();
    memset(u, 0, sizeof u);
    memset(b, 0, sizeof b);
    selected_building = -1;
    menu_open = help = 0;
    mode = MODE_NORMAL;
    screen = SCREEN_PLAY;
    camx = camy = 0;
    cx = cy = 60;
    enemy_timer = -100000;
    /* Distant, completed bases keep the mission from ending. */
    b[MAX_BUILDINGS - 1].alive = 1; b[MAX_BUILDINGS - 1].owner = SIDE;
    b[MAX_BUILDINGS - 1].type = B_COMMAND; b[MAX_BUILDINGS - 1].x = WORLD_W - 48;
    b[MAX_BUILDINGS - 1].y = WORLD_H - 48; b[MAX_BUILDINGS - 1].hp = b[MAX_BUILDINGS - 1].maxhp = 1500;
    b[MAX_BUILDINGS - 2] = b[MAX_BUILDINGS - 1];
    b[MAX_BUILDINGS - 2].owner = ENEMY; b[MAX_BUILDINGS - 2].type = B_HATCHERY;
    b[MAX_BUILDINGS - 2].x = WORLD_W - 48; b[MAX_BUILDINGS - 2].y = WORLD_H - 120;
    tick(0);
    /* Fixtures advance by observe_next() only; race profiles may judge the
     * synthetic map finished, which the observer must not see. */
    screen = SCREEN_PLAY;
    won = lost = 0;
    camx = camy = 0;
    observe_next();
    clear_events();
}

static u32 state_hash(void)
{
    u32 h = 2166136261u;
#define MIX(p, n) do { const u8 *q = (const u8 *)(p); for (size_t k = 0; k < (n); k++) h = (h ^ q[k]) * 16777619u; } while (0)
    MIX(u, sizeof u); MIX(b, sizeof b); MIX(m, sizeof m);
    int scalars[] = { minerals, gas, supply_used, supply_cap, frame, screen, won, lost,
                      cx, cy, camx, camy, selected_building, mode, wave, enemy_timer, menu_open };
    MIX(scalars, sizeof scalars);
    MIX(&seed, sizeof seed);
#undef MIX
    return h;
}

#ifndef GAME_AUDIO_OFF
static int hooks(void)
{
#ifndef SC64_RACES
    /* The opening skirmish brings the enemy squad on screen: its shots and
     * deaths are heard. */
    previous_held = 0;
    init_state();
    screen = SCREEN_PLAY;
    camx = camy = 0;
    tick(0);
    clear_events();
    for (int t = 0; t < 4000 && !(events(GAME_AUDIO_ATTACK) && events(GAME_AUDIO_DEATH)); t++) tick(0);
    if (!events(GAME_AUDIO_ATTACK) || !events(GAME_AUDIO_DEATH)) return fail("combat events");
#endif

    /* The observer's shot contract: a raised cooldown on a visible unit. */
    fixture();
    put_unit(0, SIDE, U_MARINE, 60, 60);
    put_unit(1, ENEMY, U_ZERGLING, WORLD_W - 20, WORLD_H - 20);
    observe_next();
    clear_events();
    u[0].cd = 15;
    u[1].cd = 15;
    observe_next();
    if (events(GAME_AUDIO_ATTACK) != 1) return fail("visible shot");
    u[0].cd = 14;
    observe_next();
    if (events(GAME_AUDIO_ATTACK) != 1) return fail("cooling down is not a shot");

    /* Off-screen deaths stay silent; on-screen ones are capped per frame. */
    fixture();
    for (int i = 0; i < 6; i++) put_unit(i, ENEMY, U_ZERGLING, 40 + i * 8, 40);
    put_unit(10, ENEMY, U_ZERGLING, WORLD_W - 20, WORLD_H - 20);
    observe_next();
    clear_events();
    u[10].alive = 0;
    observe_next();
    if (events(GAME_AUDIO_DEATH)) return fail("off-screen death was audible");
    for (int i = 0; i < 6; i++) u[i].alive = 0;
#ifdef SC64_AUDIO_IMPORTED
#define AUDIO_OUTPUTS audio_host_samples
#else
#define AUDIO_OUTPUTS audio_host_noises
#endif
    u32 outputs = AUDIO_OUTPUTS;
    observe_next();
    if (events(GAME_AUDIO_DEATH) != GAME_AUDIO_EFFECTS_PER_FRAME ||
        AUDIO_OUTPUTS - outputs != GAME_AUDIO_EFFECTS_PER_FRAME)
        return fail("per-frame effect cap");

    /* Selection, command response, new unit and completed building. */
    fixture();
    put_unit(0, SIDE, U_MARINE, 60, 60);
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_BARRACKS; b[0].x = 100; b[0].y = 80;
    b[0].under_construction = 1;
    observe_next();
    clear_events();
    u[0].selected = 1;
    observe_next();
    if (events(GAME_AUDIO_WHAT) != 1) return fail("selection response");
#ifdef SC64_AUDIO_IMPORTED
    {
        /* Imported banks answer with one of the unit's own response samples. */
        int raw = game_audio_unit_raw(&u[0]), row = raw * SC64_AUDIO_KIND_COUNT + SC64_AUDIO_WHAT;
        int first = sc64_audio_unit_first[row], count = sc64_audio_unit_count[row], found = 0;
        for (int k = 0; k < count; k++) found |= sc64_audio_unit_sound[first + k] == audio_host_last_sample;
        if (raw < 0 || !count || !found) return fail("imported selection sample");
        if (audio_host_music != 4 + 4 * GAME_AUDIO_PLAYER_RACE) return fail("race theme");
    }
#endif
    observe_next();
    if (events(GAME_AUDIO_WHAT) != 1) return fail("unchanged selection repeated");
    u[0].tx = 90;
    observe_next();
    if (events(GAME_AUDIO_YES)) return fail("order change without input answered");
    u[0].tx = 95;
    game_audio_host_held = KEY_A;
    observe_next();
    if (events(GAME_AUDIO_YES) != 1) return fail("command response");
    u[0].tx = 100;
    observe_next();
    if (events(GAME_AUDIO_YES) != 1) return fail("held key is not a new command");
    game_audio_host_held = 0;
    put_unit(5, SIDE, U_SCV, 300, 300);
    observe_next();
    if (events(GAME_AUDIO_READY) != 1) return fail("new unit ready response");
    put_unit(6, ENEMY, U_SCV, 300, 300);
    observe_next();
    if (events(GAME_AUDIO_READY) != 1) return fail("enemy unit answered");
    b[0].under_construction = 0;
    observe_next();
    if (events(GAME_AUDIO_BUILT) != 1) return fail("construction complete");

    /* A refused train order cues the player race's advisor once per press. */
    fixture();
#ifdef SC64_RACES
    static const int hall[3] = { B_COMMAND, B_HATCHERY, B_NEXUS }, worker[3] = { U_SCV, U_DRONE, U_PROBE };
    int hall_type = hall[race_human], trainee = worker[race_human];
#else
    int hall_type = B_COMMAND, trainee = U_SCV;
#endif
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = hall_type; b[0].x = 100; b[0].y = 80;
    b[0].hp = b[0].maxhp = 1500;
#ifdef SC64_RACES
    b[0].larva = 3;
#endif
    minerals = gas = 0;
    observe_next();
    clear_events();
    queue_unit(0, trainee);
    if (toast_id != 1 || b[0].queue_count) return fail("train refusal toast");
    observe_next();
    if (events(GAME_AUDIO_ADVISOR) != 1) return fail("minerals advisor");
#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ADVISOR_LINES)
    if (audio_host_last_sample != sc64_audio_advisor[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ADVISOR_LINES])
        return fail("race minerals line");
#endif
    observe_next();
    if (events(GAME_AUDIO_ADVISOR) != 1) return fail("advisor cue repeated without a press");
    queue_unit(0, trainee);
    observe_next();
    if (events(GAME_AUDIO_ADVISOR) != 2) return fail("repeated refusal replays the advisor");
#ifndef SC64_RACES
    minerals = 1000;
    supply_used = supply_cap;
    queue_unit(0, trainee);
    observe_next();
    if (toast_id != 2 || events(GAME_AUDIO_ADVISOR) != 3) return fail("supply advisor");
#endif
    toast_refusal(12);
    observe_next();
#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ADVISOR_LINES)
    if (audio_host_last_sample != sc64_audio_advisor[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ADVISOR_LINES + 1])
        return fail("race gas line");
#endif
    clear_events();
    toast_refusal(1);
    frame += 40;
    game_render();
    observe_next();
    if (events(GAME_AUDIO_ADVISOR) || toast_advisor) return fail("discontinuity kept an advisor cue");

    /* A tick discontinuity (load or restart) and pauses never replay history. */
    put_unit(7, ENEMY, U_ZERGLING, 50, 50);
    observe_next();
    clear_events();
    u[7].alive = 0;
    frame += 40;
    game_render();
    screen = SCREEN_PAUSE;
    put_unit(8, SIDE, U_MARINE, 50, 50);
    observe_next();
    screen = SCREEN_PLAY;
    observe_next();
    observe_next();
    for (int k = 0; k < GAME_AUDIO_EVENT_KINDS; k++)
        if (events(k)) return fail("discontinuity produced an event");
#ifdef SC64_AUDIO_IMPORTED
    if (audio_host_samples == 0) return fail("imported samples");
    screen = SCREEN_TITLE;
    game_render();
    if (audio_host_music != 0) return fail("menu theme");
    screen = SCREEN_PLAY;
#else
    if (audio_host_tones == 0) return fail("placeholder tones");
#endif
    return 0;
}
#endif

int main(int argc, char **argv)
{
    gba_init();
    game_init();
#ifndef GAME_AUDIO_OFF
    if (argc > 1 && !strcmp(argv[1], "hooks")) {
        if (hooks()) return 1;
        puts("game audio hook checks passed");
        return 0;
    }
#else
    (void)argc; (void)argv;
#endif
    /* Long run from a fresh process: identical inputs from the title screen;
     * the hash must not depend on whether the observer runs. */
    previous_held = 0;
    clear_events();
    u32 lcg = 12345u, held = 0;
    int hold = 0;
    for (int t = 0; t < 12000; t++) {
        if (hold-- <= 0) {
            lcg = lcg * 1103515245u + 12345u;
            static const u16 keys[] = { 0, KEY_A, KEY_B, KEY_R, KEY_L, KEY_UP, KEY_DOWN, KEY_LEFT,
                                        KEY_RIGHT, KEY_A | KEY_R, KEY_L | KEY_RIGHT, KEY_START };
            held = keys[(lcg >> 16) % (sizeof keys / sizeof keys[0])];
            hold = (int)((lcg >> 8) % 24u);
        }
        tick((u16)held);
    }
    printf("state %08x\n", state_hash());
#ifndef GAME_AUDIO_OFF
    u32 total = 0;
    for (int k = 0; k < GAME_AUDIO_EVENT_KINDS; k++) total += events(k);
    printf("events attack=%u death=%u ready=%u what=%u yes=%u built=%u advisor=%u\n",
           events(GAME_AUDIO_ATTACK), events(GAME_AUDIO_DEATH), events(GAME_AUDIO_READY),
           events(GAME_AUDIO_WHAT), events(GAME_AUDIO_YES), events(GAME_AUDIO_BUILT),
           events(GAME_AUDIO_ADVISOR));
    if (!total) return fail("long run produced no audio events");
#endif
    return 0;
}
