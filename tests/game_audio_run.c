/* Presentation-only audio hooks. "hooks" checks event detection on fixtures;
 * without arguments a long controller-driven run prints "state <hash>", which
 * must match a build compiled with -DGAME_AUDIO_OFF. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

extern u32 audio_host_tones, audio_host_noises, audio_host_samples;
extern int audio_host_last_sample, audio_host_music;
extern int audio_host_last_kind, audio_host_last_priority, audio_host_advisor_busy;
extern u32 audio_host_advisor_samples;

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
static int empty_slot_transitions(void)
{
    fixture();
    put_unit(MAX_UNITS - 1, ENEMY, U_ZERGLING, 60, 60);
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_BARRACKS;
    b[0].x = b[0].y = 60;
    b[0].under_construction = 1;
    observe_next();
    clear_events();

    /* Skip only dead slots whose old snapshot is already zero. Other dead
     * snapshots must still be cleared, and observed deaths emitted once. */
    u[MAX_UNITS - 1].alive = 0;
    b[0].alive = 0;
    game_audio_snapshot[0] = 0xffu;
    game_audio_snapshot[MAX_UNITS + 1] = GAME_AUDIO_SNAPSHOT_CONSTRUCTING;
    observe_next();
    if (events(GAME_AUDIO_DEATH) != 2) return fail("empty-slot scan lost a death");
    for (int i = 0; i < MAX_UNITS; ++i)
        if (!u[i].alive && game_audio_snapshot[i]) {
            fprintf(stderr, "slot %d snapshot=%u frame=%d observed=%d\n",
                    i, game_audio_snapshot[i], frame, game_audio_last_frame);
            return fail("dead unit snapshot was retained");
        }
    if (game_audio_snapshot[MAX_UNITS] || game_audio_snapshot[MAX_UNITS + 1])
        return fail("dead building snapshot was retained");
    observe_next();
    if (events(GAME_AUDIO_DEATH) != 2) return fail("empty slot replayed a death");

    put_unit(MAX_UNITS - 1, SIDE, U_MARINE, 60, 60);
    b[0].alive = 1;
    observe_next();
    if (events(GAME_AUDIO_READY) != 1) return fail("reused slot lost ready cue");
    b[0].under_construction = 0;
    observe_next();
    if (events(GAME_AUDIO_BUILT) != 1) return fail("reused building lost completion cue");

    clear_events();
    u[MAX_UNITS - 1].alive = 0;
    b[0].alive = 0;
    frame += 3;
    game_render();
    observe_next();
    for (int kind = 0; kind < GAME_AUDIO_EVENT_KINDS; ++kind)
        if (events(kind)) return fail("reused slots replayed discontinuous history");
    return 0;
}

#ifdef SC64_RACES
static int warp_load_prime(void)
{
    gba_init(); game_init(); race_mode = 1; race_human = R_PROTOSS;
    race_bots = 1; race_bot_races[0] = R_TER; race_skirmish_map = 0;
    init_state(); screen = SCREEN_PLAY; game_render();
    int slot = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            sc64_race_building_rule(b[i].type) &&
            sc64_race_building_rule(b[i].type)->race == R_PROTOSS) { slot = i; break; }
    if (slot < 0 || !save_game()) return fail("load prime setup save");
    int worker = -1, sx, sy;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE) { worker = i; break; }
    if (worker < 0 || !race_skirmish_site_for(SIDE, B_PYLON, &sx, &sy)) return fail("failed build setup");
    minerals = 5000; gas = 1000;
    if (!race_start_build(SIDE, worker, B_PYLON, sx, sy)) return fail("failed build admission setup");
    u[worker].x = sx; u[worker].y = sy; minerals = 0; clear_events();
    race_pending_build_tick(worker);
    if (game_audio_pending_warp >= 0 || events(GAME_AUDIO_WARP_START))
        return fail("unfunded foundation queued cue");
    game_audio_snapshot[MAX_UNITS + slot] = GAME_AUDIO_SNAPSHOT_ALIVE | GAME_AUDIO_SNAPSHOT_CONSTRUCTING;
    game_audio_pending_warp = 156; /* queued before a successful load */
    game_audio_last_frame = frame - 1; clear_events();
    if (!load_game()) return fail("load prime real load");
    game_render();
    if (events(GAME_AUDIO_WARP_DONE) || events(GAME_AUDIO_WARP_START)) return fail("coincident load replayed cue");
    clear_events();
    game_audio_snapshot[MAX_UNITS + slot] = GAME_AUDIO_SNAPSHOT_ALIVE | GAME_AUDIO_SNAPSHOT_CONSTRUCTING;
    game_audio_last_frame = frame; game_audio_prime(); game_render();
    if (events(GAME_AUDIO_WARP_DONE)) return fail("same-frame prime replayed cue");
    game_audio_pending_warp = 156; clear_events(); init_state();
    screen = SCREEN_PLAY; game_render();
    if (events(GAME_AUDIO_WARP_START) || events(GAME_AUDIO_WARP_DONE)) return fail("new match replayed cue");
    return 0;
}

static int warp_lifecycle(void)
{
    fixture();
    game_audio_effects = GAME_AUDIO_EFFECTS_PER_FRAME; /* previous frame was saturated */
#ifdef SC64_AUDIO_IMPORTED
    u32 samples_before = audio_host_samples;
#endif
    int slot = spawn_build(SIDE, B_PYLON, 60, 60, 1);
    if (slot < 0 || events(GAME_AUDIO_WARP_START)) return fail("foundation cue did not queue");
    observe_next();
    if (events(GAME_AUDIO_WARP_START) != 1) return fail("previous budget suppressed warp start");
#if defined(SC64_AUDIO_IMPORTED) && !defined(SC64_AUDIO_WARP_CUES)
    if (game_audio_effects != 0) return fail("silent legacy warp spent effect budget");
    game_audio_emit(GAME_AUDIO_DEATH, 156);
    game_audio_emit(GAME_AUDIO_DEATH, 156);
    if (events(GAME_AUDIO_DEATH) != 2) return fail("legacy warp suppressed another effect");
#endif
#ifdef SC64_AUDIO_WARP_CUES
    if (audio_host_samples != samples_before + 1 || audio_host_last_sample != sc64_audio_warp[0])
        return fail("warp start sample");
    samples_before = audio_host_samples;
#endif
    observe_next();
    if (events(GAME_AUDIO_WARP_START) != 1) return fail("warp start repeated");
#ifdef SC64_AUDIO_IMPORTED
    samples_before = audio_host_samples;
#endif
    b[slot].under_construction = 0; observe_next();
    if (events(GAME_AUDIO_WARP_DONE) != 1 || events(GAME_AUDIO_BUILT)) return fail("warp completion");
#ifdef SC64_AUDIO_WARP_CUES
    if (audio_host_samples != samples_before + 1 || audio_host_last_sample != sc64_audio_warp[1])
        return fail("warp completion sample");
#elif defined(SC64_AUDIO_IMPORTED)
    int row = 156 * SC64_AUDIO_KIND_COUNT + SC64_AUDIO_WHAT;
    if (!sc64_audio_unit_count[row] || audio_host_samples != samples_before + 1 ||
        audio_host_last_sample != sc64_audio_unit_sound[sc64_audio_unit_first[row]])
        return fail("legacy warp completion fallback sample");
#endif
    observe_next();
    if (events(GAME_AUDIO_WARP_DONE) != 1) return fail("warp completion repeated");
    b[slot].alive = 0; clear_events();
    if (spawn_build(SIDE, B_PYLON, 60, 60, 1) != slot) return fail("same-frame reused slot");
    observe_next();
    if (events(GAME_AUDIO_WARP_START) != 1) return fail("same-frame reused slot start");
    b[slot].alive = 0; observe_next();
    if (events(GAME_AUDIO_WARP_DONE)) return fail("cancel generated completion");
    clear_events();
    slot = spawn_build(ENEMY, B_PYLON, 60, 60, 1);
    if (slot < 0 || events(GAME_AUDIO_WARP_START)) return fail("enemy start");
    b[slot].alive = 0; observe_next(); clear_events();
    slot = spawn_build(SIDE, B_PYLON, WORLD_W - 48, 60, 1);
    if (slot < 0 || events(GAME_AUDIO_WARP_START)) return fail("offscreen start");
    b[slot].alive = 0; observe_next(); clear_events();
    slot = spawn_build(SIDE, B_DEPOT, 60, 60, 1); observe_next();
    b[slot].under_construction = 0; observe_next();
    if (events(GAME_AUDIO_WARP_START) || events(GAME_AUDIO_WARP_DONE) || events(GAME_AUDIO_BUILT) != 1)
        return fail("Terran completion changed");
    b[slot].alive = 0; observe_next(); clear_events();
    slot = spawn_build(SIDE, B_HATCHERY, 60, 60, 1); observe_next();
    b[slot].under_construction = 0; observe_next();
    if (events(GAME_AUDIO_WARP_START) || events(GAME_AUDIO_WARP_DONE) || events(GAME_AUDIO_BUILT) != 1)
        return fail("Zerg completion changed");
    b[slot].alive = 0; observe_next(); clear_events();
    if (spawn_build(SIDE, B_PYLON, 60, 60, 1) < 0 ||
        spawn_build(SIDE, B_PYLON, 80, 60, 1) < 0) return fail("simultaneous setup");
    observe_next();
    if (events(GAME_AUDIO_WARP_START) != 1) return fail("simultaneous warp starts not bounded");
    return 0;
}

#endif

/* ---- Advisor alerts: what reaches the observer, and when it speaks. ---- */

#ifdef SC64_RACES
static void set_race(int race) { race_human = race; }
#else
static void set_race(int race) { (void)race; }
#endif

/* A hit through the real damage path; the owner is the attacker. */
static void hostile_hit(int target, int owner) { hit_target(target, 3, U_ZERGLING, owner); }

static void quiet(int ticks) { for (int k = 0; k < ticks; k++) observe_next(); }

static void quiet_past_cooldown(void) { quiet(GAME_AUDIO_ATTACK_ALERT_TICKS + 1); clear_events(); }

#define ATTACK_TICK GAME_AUDIO_STATE[GAME_AUDIO_STATE_ATTACK_TICK]

#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ALERT_LINES)
#define ALERT_SAMPLE(line) sc64_audio_alert[GAME_AUDIO_PLAYER_RACE * SC64_AUDIO_ALERT_LINES + (line)]
/* An alert is one advisor-class request at priority zero. */
static int advisor_request(int line)
{
    return audio_host_last_sample == ALERT_SAMPLE(line) && audio_host_last_kind == AUDIO_CLASS_ADVISOR &&
           audio_host_last_priority == 0;
}
#endif

static void off_screen_player(void)
{
    put_unit(10, SIDE, U_MARINE, 900, 900);
    put_unit(11, SIDE, U_MARINE, 60, 60);
    u[10].hp = u[10].maxhp = u[11].hp = u[11].maxhp = 200;
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_DEPOT; b[0].x = 900; b[0].y = 700;
    b[0].hp = 300; b[0].maxhp = 500;
}

/* Only a hostile hit raises an under-attack alert: not burn, not friendly or
 * ownerless damage, not an on-screen fight, and a one-shot kill still counts. */
static int attack_alerts(void)
{
    fixture();
    set_race(0);
    off_screen_player();
    observe_next();
    quiet_past_cooldown();

    hostile_hit(11, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("on-screen hit alerted");
    hostile_hit(10, SIDE);
    hit_target(10, 3, U_ZERGLING, -1);
    hostile_hit(10, NEUTRAL);
    hit_target(MAX_UNITS + 0, 20, U_ZERGLING, -1);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("friendly or ownerless damage alerted");
    put_unit(5, ENEMY, U_ZERGLING, 900, 900);
    hostile_hit(5, SIDE);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("damage to an enemy alerted");

    hostile_hit(10, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_FORCES)
        return fail("off-screen unit hit");
#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ALERT_LINES)
    if (!advisor_request(GAME_AUDIO_ALERT_FORCES)) return fail("forces alert is not an advisor request");
#endif
    observe_next();
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("one hit alerted twice");

    /* The wait is shared by base and forces, and runs from the alert. */
    int alerted = ATTACK_TICK;
    hostile_hit(10, ENEMY);
    hit_target(MAX_UNITS + 0, 20, U_ZERGLING, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("a base hit right after a unit alert");
    while ((u16)(frame - alerted) < GAME_AUDIO_ATTACK_ALERT_TICKS - 2) observe_next();
    hit_target(MAX_UNITS + 0, 20, U_ZERGLING, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("sustained attack repeated inside the wait");
    hit_target(MAX_UNITS + 0, 20, U_ZERGLING, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2 || game_audio_last_alert != GAME_AUDIO_ALERT_BASE)
        return fail("off-screen building hit");
    /* A unit and a building hit in the same update make one alert, base first. */
    quiet_past_cooldown();
    hostile_hit(10, ENEMY);
    hostile_hit(MAX_UNITS + 0, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_BASE)
        return fail("simultaneous hits");

    /* A unit killed outright between two looks still alerts. */
    quiet_past_cooldown();
    u[10].hp = 4;
    hit_target(10, 50, U_ZERGLING, ENEMY);
    if (u[10].alive) return fail("fixture unit survived");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_FORCES)
        return fail("one-shot kill");
    quiet_past_cooldown();
    b[0].hp = 5;
    hit_target(MAX_UNITS + 0, 500, U_ZERGLING, ENEMY);
    if (b[0].alive) return fail("fixture building survived");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_BASE)
        return fail("one-shot building kill");

    /* A discontinuity drops a hit from before it. */
    quiet_past_cooldown();
    put_unit(10, SIDE, U_MARINE, 900, 900);
    hostile_hit(10, ENEMY);
    frame += 40;
    game_render();
    quiet(2);
    if (events(GAME_AUDIO_ALERT)) return fail("hit across a discontinuity alerted");

    /* Zerg and Protoss attack alerts have no N64 evidence: silent, and no work. */
#ifdef SC64_RACES
    for (int race = 1; race <= 2; race++) {
        fixture();
        set_race(race);
        off_screen_player();
        observe_next();
        quiet_past_cooldown();
        hostile_hit(10, ENEMY);
        hit_target(MAX_UNITS + 0, 20, U_ZERGLING, ENEMY);
        observe_next();
        if (events(GAME_AUDIO_ALERT) || GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING])
            return fail("attack alert for a race without evidence");
    }
    set_race(0);
#endif
    return 0;
}

#ifdef SC64_RACES
/* Terran buildings below a third of their hit points burn without any enemy. */
static int burning_building(void)
{
    fixture();
    set_race(0);
    off_screen_player();
    b[1].alive = 1; b[1].owner = SIDE; b[1].type = B_BARRACKS; b[1].raw_type = -1;
    b[1].x = 900; b[1].y = 800; b[1].maxhp = 1000;
    b[1].shield_remainder = NEUTRAL * 256;
    if (!race_building_can_burn(&b[1])) return fail("fixture building cannot burn");
    /* 336 of 1000 burns (a third is 33.4): one point lost crosses a step of 16. */
    b[1].hp = 336;
    observe_next();
    quiet_past_cooldown();
    for (int k = 0; k < 600; k++) {
        ++frame;
        race_tick_building_damage(1);
        game_render();
    }
    if (b[1].hp >= 336 - 5) return fail("fixture building did not burn");
    if (events(GAME_AUDIO_ALERT)) return fail("burn alone alerted");
    /* The same building taking an enemy hit while it burns is under attack. */
    ++frame;
    hit_target(MAX_UNITS + 1, 15, U_MARINE, ENEMY);
    race_tick_building_damage(1);
    game_render();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_BASE)
        return fail("hit on a burning building");
    /* Burn that kills it is still no attack. */
    quiet_past_cooldown();
    b[1].hp = 1; b[1].shield_remainder = NEUTRAL * 256;
    for (int k = 0; k < 40 && b[1].alive; k++) {
        ++frame;
        race_tick_building_damage(1);
        game_render();
    }
    if (b[1].alive) return fail("fixture building did not burn down");
    if (events(GAME_AUDIO_ALERT)) return fail("burn kill alerted");
    return 0;
}
#endif

/* The attack wait starts with the match or the load, from any old state. */
static int cooldown_start(void)
{
    /* A new match with leftover observer words from some earlier game. */
    previous_held = 0;
    init_state();
    set_race(0);
    GAME_AUDIO_STATE[GAME_AUDIO_STATE_ATTACK_TICK] = 0x7000;
    GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING] = 0xf;
    /* The opening position stays as it is, so a save of it is valid. */
    int k = 10;
    for (int i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == SIDE) { k = i; break; }
    if (!u[k].alive) return fail("no player unit to attack");
    u[k].x = u[k].tx = 330; u[k].y = u[k].ty = 200; /* in the world, off the first screen */
    u[k].hp = u[k].maxhp = 400;
    screen = SCREEN_PLAY;
    camx = camy = 0;
    observe_next();
    clear_events();
    if (GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING]) return fail("a new match kept pending alerts");
    int start = ATTACK_TICK;
    if ((u16)(frame - start) > 3) return fail("the wait did not start with the match");
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("alert in the first seconds of a match");
    while ((u16)(frame - start) < GAME_AUDIO_ATTACK_ALERT_TICKS - 2) observe_next();
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("alert one tick early");
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("no alert once the wait is over");

    /* A load: the saved game's tick is far from the one in the observer's state. */
    quiet(300);
    if (!save_game()) return fail("save for the load check");
    int saved = frame;
    quiet(GAME_AUDIO_ATTACK_ALERT_TICKS * 3);
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2) return fail("alert before the load");
    if (!load_game() || frame != saved) return fail("load for the cooldown check");
    game_render();
    clear_events();
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT)) return fail("alert right after a load");
    start = ATTACK_TICK;
    while ((u16)(frame - start) < GAME_AUDIO_ATTACK_ALERT_TICKS) observe_next();
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("no alert after the load wait");

    /* Pausing keeps the wait where it was; it is not a restart. */
    quiet_past_cooldown();
    screen = SCREEN_PAUSE;
    observe_next();
    screen = SCREEN_PLAY;
    observe_next();
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("a pause restarted the wait");

    /* A long quiet spell never wraps the 16-bit age into a false wait. */
    quiet_past_cooldown();
    u[k].hp = 400;
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("alert before the long quiet spell");
    clear_events();
    for (int k = 0; k < 8; k++) { frame += 8192; game_render(); }
    frame += 100;
    game_render();
    quiet(2);
    hostile_hit(k, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1) return fail("an old alert read as recent after the age wrapped");
    return 0;
}

/* Research and upgrades speak once per completion, however many finish at once. */
static int one_shots(void)
{
    fixture();
    set_race(0);
    off_screen_player();
    observe_next();
    quiet_past_cooldown();

    /* Both upgrades finishing in one update: one announcement. */
    weapons_research = armor_research = 600;
    observe_next();
    tick_research();
    if (weapons_level != 1 || armor_level != 1) return fail("fixture upgrades did not finish");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_UPGRADE)
        return fail("two upgrades in one update");
    quiet(10);
    if (events(GAME_AUDIO_ALERT) != 1) return fail("upgrade alert repeated");
    /* A level that is not a finished upgrade stays silent. */
    ++weapons_level;
    quiet(2);
    if (events(GAME_AUDIO_ALERT) != 1) return fail("granted upgrade alerted");
    /* The next one is a new completion. */
    armor_research = 600;
    observe_next();
    tick_research();
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2) return fail("a later upgrade stayed silent");
    clear_events();

    /* Research finishing twice in one update is one announcement, a later one another. */
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_RESEARCH)
        return fail("research alert");
    quiet(5);
    if (events(GAME_AUDIO_ALERT) != 1) return fail("research alert repeated");
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2) return fail("a later research stayed silent");
    clear_events();

    /* A load or restart forgets what finished before it. */
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    game_audio_cue(GAME_AUDIO_CUE_UPGRADE);
    game_audio_prime();
    quiet(3);
    if (events(GAME_AUDIO_ALERT)) return fail("research replayed across a prime");

    /* Research races are told apart: Zerg has research only, Protoss nothing. */
#ifdef SC64_RACES
    fixture();
    set_race(1);
    quiet_past_cooldown();
    game_audio_cue(GAME_AUDIO_CUE_UPGRADE);
    observe_next();
    quiet(2);
    if (events(GAME_AUDIO_ALERT)) return fail("Zerg upgrade alert has no evidence");
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_RESEARCH)
        return fail("Zerg research alert");
    fixture();
    set_race(2);
    quiet_past_cooldown();
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    game_audio_cue(GAME_AUDIO_CUE_UPGRADE);
    observe_next();
    quiet(2);
    if (events(GAME_AUDIO_ALERT) || GAME_AUDIO_STATE[GAME_AUDIO_STATE_PENDING])
        return fail("Protoss alert has no evidence");
    set_race(0);
#endif
    return 0;
}

#if defined(SC64_RACES) && defined(SC64_SHARED_TRANSPORT)
/* The skirmish transport research announces itself when its timer ends, for
 * the player's job only. */
static int zerg_research_job(void)
{
    set_race(1);
    gba_init(); game_init();
    race_mode = 1; race_human = R_ZERG; race_bots = 1; race_bot_races[0] = R_TER; race_skirmish_map = 0;
    init_state(); screen = SCREEN_PLAY; game_render();
    int slot = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_HATCHERY) { slot = i; break; }
    if (slot < 0) return fail("no Zerg hall for the research job");
    b[slot].type = B_LAIR; b[slot].raw_type = -1;
    minerals = gas = 5000;
    if (!race_sk_transport_start_owner(SIDE, slot)) return fail("transport research did not start");
    clear_events();
    int guard = 0;
    while (race_skirmish_transport_tech.owner[sc64_sk_transport_owner_index(SIDE)].remaining && guard++ < 20000) {
        ++frame;
        race_sk_transport_research_tick();
        if (events(GAME_AUDIO_ALERT)) break;
        game_render();
        if (!race_skirmish_transport_tech.owner[sc64_sk_transport_owner_index(SIDE)].remaining) break;
    }
    game_render();
    if (!race_skirmish_transport_tech.owner[sc64_sk_transport_owner_index(SIDE)].researched)
        return fail("transport research did not finish");
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_RESEARCH)
        return fail("transport research did not announce itself once");
    quiet(10);
    if (events(GAME_AUDIO_ALERT) != 1) return fail("finished transport research repeated");
    set_race(0);
    return 0;
}
#endif

/* The advisor voice: one request per frame, held back while it is taken. */
static int voice_policy(void)
{
    fixture();
    set_race(0);
    off_screen_player();
    observe_next();
    quiet_past_cooldown();

    /* Taken by another line: an attack is dropped, a one-shot waits. */
    audio_host_advisor_busy = 1;
    hostile_hit(10, ENEMY);
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    observe_next();
    quiet(3);
    if (events(GAME_AUDIO_ALERT)) return fail("alert while the advisor voice was taken");
    audio_host_advisor_busy = 0;
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_RESEARCH)
        return fail("held research did not play when the voice freed");
    quiet(5);
    if (events(GAME_AUDIO_ALERT) != 1) return fail("a dropped attack came back or research repeated");
    /* The dropped attack did not use up the wait. */
    hostile_hit(10, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2 || game_audio_last_alert != GAME_AUDIO_ALERT_FORCES)
        return fail("a dropped attack consumed the wait");
    clear_events();

    /* Everything at once: one per frame, attack, research, upgrade in that order. */
    quiet_past_cooldown();
    hostile_hit(10, ENEMY);
    game_audio_cue(GAME_AUDIO_CUE_UPGRADE);
    game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_FORCES)
        return fail("attack first");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2 || game_audio_last_alert != GAME_AUDIO_ALERT_RESEARCH)
        return fail("research second");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 3 || game_audio_last_alert != GAME_AUDIO_ALERT_UPGRADE)
        return fail("upgrade third");
    quiet(5);
    if (events(GAME_AUDIO_ALERT) != 3) return fail("held alerts repeated");
    clear_events();

    /* The player's own refusal takes the voice before an alert in the same frame. */
    quiet_past_cooldown();
    toast_advisor = TOAST_CUE_NO_BUILD;
    hostile_hit(10, ENEMY);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_NO_BUILD)
        return fail("refusal before attack");
    clear_events();

#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ALERT_LINES)
    /* Job's finished is an advisor line when the voice is free, and keeps the
     * building's own hum when it is not; two finishing together share one line. */
    quiet_past_cooldown();
    for (int i = 1; i <= 2; i++) {
        b[i].alive = 1; b[i].owner = SIDE; b[i].type = B_BARRACKS; b[i].x = 100 + i * 40; b[i].y = 80;
        b[i].hp = b[i].maxhp = 1000; b[i].under_construction = 1;
    }
    observe_next();
    clear_events();
    u32 samples = audio_host_samples, advisor = audio_host_advisor_samples;
    u32 lines = game_audio_alert_lines[GAME_AUDIO_ALERT_BUILT];
    b[1].under_construction = 0;
    b[2].under_construction = 0;
    observe_next();
    if (events(GAME_AUDIO_BUILT) != 2) return fail("both buildings should complete");
    if (game_audio_alert_lines[GAME_AUDIO_ALERT_BUILT] - lines != 1 || audio_host_advisor_samples - advisor != 1 ||
        audio_host_samples - samples != 2)
        return fail("job's finished was not one advisor request");
    b[1].under_construction = 1; b[2].under_construction = 1;
    observe_next();
    clear_events();
    audio_host_advisor_busy = 1;
    b[1].under_construction = 0;
    observe_next();
    audio_host_advisor_busy = 0;
    if (events(GAME_AUDIO_BUILT) != 1 || audio_host_last_kind == AUDIO_CLASS_ADVISOR ||
        game_audio_alert_lines[GAME_AUDIO_ALERT_BUILT] - lines != 1)
        return fail("job's finished took a taken advisor voice");
    /* An alert never takes the speech voice used for unit responses. */
    quiet_past_cooldown();
    hostile_hit(10, ENEMY);
    observe_next();
    if (audio_host_last_kind == AUDIO_CLASS_SPEECH) return fail("alert used the speech voice");
#endif
    return 0;
}

static int hooks(void)
{
#ifndef SC64_RACES
    /* The command menu pauses simulation; A must still play the immediate
     * insufficient-minerals advisor on the same rendered frame. */
    fixture();
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_ENG_BAY;
    minerals = 0; gas = 1000;
    tick(KEY_B); tick(0);
    if (!menu_open) return fail("research menu did not open");
    for (int i=0; menu_cursor != MENU_WEAPONS && i<field_menu_count(); ++i) {
        tick(KEY_DOWN); tick(0);
    }
    if (menu_cursor != MENU_WEAPONS) return fail("research row navigation");
    clear_events();
    tick(KEY_A);
    if (toast_id != 1 || events(GAME_AUDIO_ADVISOR) != 1)
        return fail("same-frame research refusal advisor");
    tick(0);
    if (events(GAME_AUDIO_ADVISOR) != 1) return fail("research refusal advisor repeated");
    minerals = 1000; gas = 0;
    tick(KEY_B); tick(0);
    for (int i=0; menu_cursor != MENU_WEAPONS && i<field_menu_count(); ++i) {
        tick(KEY_DOWN); tick(0);
    }
    if (menu_cursor != MENU_WEAPONS) return fail("research gas row navigation");
    tick(KEY_A);
    if (toast_id != 12 || events(GAME_AUDIO_ADVISOR) != 2) {
        fprintf(stderr, "gas refusal: toast=%d events=%u cursor=%d menu=%d screen=%d frame=%d\n",
                toast_id, events(GAME_AUDIO_ADVISOR), menu_cursor, menu_open, screen, frame);
        return fail("same-frame research gas advisor");
    }
    game_render();
    if (events(GAME_AUDIO_ADVISOR) != 2) return fail("same-frame redraw repeated advisor");
#endif
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

    if (empty_slot_transitions()) return 1;
#ifdef SC64_RACES
    if (warp_lifecycle() || warp_load_prime()) return 1;
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

    /* Advisor alerts: hostile hits, the attack wait, one-shots and the voice. */
    if (attack_alerts() || cooldown_start() || one_shots() || voice_policy()) return 1;
#ifdef SC64_RACES
    if (burning_building()) return 1;
#endif

    /* Refused sites and low energy use their cues, then clear. */
    fixture();
    set_race(0);
    quiet_past_cooldown();
    clear_events();
    toast_advisor = TOAST_CUE_NO_BUILD;
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_NO_BUILD)
        return fail("blocked placement");
    toast_advisor = TOAST_CUE_NO_LAND;
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 2 || game_audio_last_alert != GAME_AUDIO_ALERT_NO_LAND)
        return fail("blocked landing");
    toast_advisor = TOAST_CUE_ENERGY;
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 3 || game_audio_last_alert != GAME_AUDIO_ALERT_ENERGY)
        return fail("low energy");
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 3 || events(GAME_AUDIO_ADVISOR)) return fail("alert cue repeated");
#if defined(SC64_AUDIO_IMPORTED) && defined(SC64_AUDIO_ALERT_LINES)
    if (GAME_AUDIO_PLAYER_RACE == 0 &&
        audio_host_last_sample != sc64_audio_alert[GAME_AUDIO_ALERT_ENERGY])
        return fail("race energy line");
#endif

    /* A placement on impossible ground takes the cue through the real command path. */
    fixture();
    put_unit(12, SIDE, U_SCV, 60, 60);
    u[12].selected = 1;
    minerals = gas = 5000;
    observe_next();
    observe_next();
    clear_events();
    cx = cy = 2;
    if (can_place(B_DEPOT, cx, cy)) return fail("fixture site is placeable");
    start_build(B_DEPOT);
    observe_next();
    if (events(GAME_AUDIO_ALERT) != 1 || game_audio_last_alert != GAME_AUDIO_ALERT_NO_BUILD)
        return fail("blocked placement cue");

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
#if defined(SC64_RACES) && defined(SC64_SHARED_TRANSPORT)
    if (zerg_research_job()) return 1;
#endif
    return 0;
}

/* One alert check by name, for telling failures apart. */
static int alert_check(const char *name)
{
    set_race(0);
    if (!strcmp(name, "attack")) return attack_alerts();
    if (!strcmp(name, "cooldown")) return cooldown_start();
    if (!strcmp(name, "oneshot")) return one_shots();
    if (!strcmp(name, "voice")) return voice_policy();
#ifdef SC64_RACES
    if (!strcmp(name, "burn")) return burning_building();
#endif
    return fail("unknown alert check");
}
#endif

int main(int argc, char **argv)
{
    gba_init();
    game_init();
#ifndef GAME_AUDIO_OFF
    if (argc > 2 && !strcmp(argv[1], "alert")) {
        if (alert_check(argv[2])) return 1;
        printf("alert check %s passed\n", argv[2]);
        return 0;
    }
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
    printf("events attack=%u death=%u ready=%u what=%u yes=%u built=%u advisor=%u alert=%u\n",
           events(GAME_AUDIO_ATTACK), events(GAME_AUDIO_DEATH), events(GAME_AUDIO_READY),
           events(GAME_AUDIO_WHAT), events(GAME_AUDIO_YES), events(GAME_AUDIO_BUILT),
           events(GAME_AUDIO_ADVISOR), events(GAME_AUDIO_ALERT));
    printf("alerts base=%u forces=%u research=%u upgrade=%u energy=%u no_build=%u no_land=%u built=%u\n",
           game_audio_alert_lines[0], game_audio_alert_lines[1], game_audio_alert_lines[2],
           game_audio_alert_lines[3], game_audio_alert_lines[4], game_audio_alert_lines[5],
           game_audio_alert_lines[6], game_audio_alert_lines[7]);
    if (!total) return fail("long run produced no audio events");
#endif
    return 0;
}
