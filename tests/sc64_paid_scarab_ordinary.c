/* SPDX-License-Identifier: GPL-3.0-or-later
 * Paid Scarabs through ordinary controls only, in the P10 product vector.
 *
 * From the title screen, with pad input through game_update() alone (the
 * helpers of the skirmish controller): launch a Protoss custom game, build
 * Pylon, Gateway, Cybernetics Core, Robotics Facility and Robotics Support
 * Bay with the Probe's order list, train two Reavers (each must arrive
 * empty), buy Scarabs for one of them with the real "BUILD SCARAB" row of the
 * order list, order both to attack the enemy base, and watch launches, flight
 * and splash. When the first Scarab is in flight the real pause menu writes
 * SRAM; the same process continues, and `resume FILE` in a fresh process loads
 * it from the title screen and must match the continuation update by update.
 *
 *   run OUT [TRACE]   whole scenario; writes OUT (SRAM) and OUT.state
 *   resume OUT        fresh process: load OUT, continue, compare
 *
 * The only fixture is the starting bank (set once, after launch, so a short
 * run can afford the whole tech chain). Everything else is pad input. State
 * is read, never written, except the clearly marked UI normalisation before
 * the save (selection flags and the remembered menu row are UI, not world). */
#include "../common/gba.c"
#define main skirmish_controller_main
#define SC64_TEST_HIT_OBSERVER psc_hit
static void psc_hit(int target, int damage, int source, int owner);
#include "sc64_skirmish_campaign_run.c"
#undef main

#ifndef SC64_PAID_SCARAB
#error "build with the P10 product flags"
#endif

enum { MAX_LOG = 256 };
static struct { unsigned at; int target, damage; } hit_log[MAX_LOG];
static unsigned hit_count, hit_epoch;       /* epoch: updates since the save */
static void psc_hit(int target, int damage, int source, int owner)
{
    (void)owner;
    if (source == U_REAVER && hit_count < MAX_LOG) {
        hit_log[hit_count].at = host_ticks - hit_epoch;
        hit_log[hit_count].target = target; hit_log[hit_count].damage = damage;
        ++hit_count;
    }
}

static u32 hash_u32(u32 h, u32 v)
{
    for (int i = 0; i < 4; ++i) { h ^= (u8)(v >> (8 * i)); h *= 16777619u; }
    return h;
}
#define H(v) (h = hash_u32(h, (u32)(v)))
static u32 state_digest(void)
{
    u32 h = 2166136261u;
    for (int i = 0; i < MAX_UNITS; i++) {
        const Unit *q = &u[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->x); H(q->y); H(q->tx); H(q->ty);
        H(q->target); H(q->raw_type); H(q->raw_owner); H(q->cd);
        H(q->owner); H(q->type); H(q->order); H(q->carry); H(q->flags);
        H(q->build); H(q->work_timer); H(q->path_retry); H(q->path_timer);
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->build_progress); H(q->x); H(q->y);
        H(q->queue_progress); H(q->owner); H(q->type); H(q->queue_count);
    }
    H(minerals); H(gas); H(frame);
    for (int o = 0; o < 5; ++o) { H(race_minerals[o]); H(race_gas[o]); }
    return h;
}
#undef H

static int scarabs_alive(void)
{ int n = 0; for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].type == U_SCARAB; return n; }
static int scarab_of(int parent)
{ for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type == U_SCARAB && u[i].build == parent) return i; return -1; }

static int complete_count(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        n += b[i].alive && b[i].owner == SIDE && b[i].type == type && building_complete(i);
    return n;
}


/* The A press that pays, observed exactly: the bank must change by the worker
 * deliveries that landed in the same update, minus the price. */
static int paid_press(int mineral_price, int gas_price)
{
    int cargo[MAX_UNITS], m0 = minerals, g0 = gas, delivered = 0, exact;
    for (int i = 0; i < MAX_UNITS; ++i) cargo[i] = u[i].alive && u[i].owner == SIDE ? u[i].carry : 0;
    input(KEY_A);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type) &&
            cargo[i] > 0 && u[i].carry == 0) delivered += cargo[i];
    exact = minerals == m0 - mineral_price + delivered && g0 - gas == gas_price;
    fprintf(stderr, "paid press: bank %d -> %d (deliveries %d, price %d), gas %d -> %d\n",
            m0, minerals, delivered, mineral_price, g0, gas);
    input(0);
    return exact;
}

/* choose_menu_command without the final A. */
static int navigate_menu(int command)
{
    if (menu_open) tap(KEY_B);
    tap(KEY_B);
    for (int guard = 0; menu_open && menu_cursor != command && guard < 64; ++guard) tap(KEY_DOWN);
    return menu_open && menu_cursor == command;
}

/* Build one structure from the Probe's order list, as a player would. */
static int build_structure(int raw)
{
    int type = sc64_race_building_from_raw(raw), command = -1, worker = -1, x = 0, y = 0;
    for (unsigned row = 0; row < sc64_skirmish_menu_count(R_PROTOSS); ++row) {
        Sc64SkirmishMenuRow item;
        if (sc64_skirmish_menu_at(R_PROTOSS, row, &item) && item.kind == SC64_SK_MENU_BUILD &&
            item.record && item.record->raw_id == (unsigned)raw) { command = (int)row; break; }
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (build_worker_available(i, SIDE, U_PROBE)) { worker = i; break; }
    if (type < 0 || command < 0 || worker < 0 || !choose_build_site(type, &x, &y) ||
        !select_unit(worker) || !choose_menu_command(command) || mode != MODE_BUILD || menu != type) {
        fprintf(stderr, "build raw%d: setup failed (command%d worker%d)\n", raw, command, worker);
        return 0;
    }
    move_cursor(x, y);
    if (mode != MODE_BUILD) return 0;
    {
        int before = complete_count(type);
        tap(KEY_A);                                    /* the Probe walks over; the bank is charged when it starts */
        for (unsigned t = 0; t < 14000 && screen == SCREEN_PLAY; ++t) {
            input(0);
            if (complete_count(type) > before) return 1;
        }
    }
    fprintf(stderr, "build raw%d: never completed\n", raw);
    return 0;
}

static int select_group_of(int slot)
{ return select_unit(slot); }

static int buy_scarab(int slot)
{
    int base = race_carrier_command_base();
    if (!select_group_of(slot)) return 0;
    if (strcmp(race_menu_label(base), "BUILD SCARAB")) {
        fprintf(stderr, "order list row %d is '%s'\n", base, race_menu_label(base)); return 0;
    }
    {
        int queued = race_paid_scarab_queued(slot);
        if (!navigate_menu(base)) return 0;
        if (!paid_press((int)sc64_psc_price(), 0)) return 0;
        return race_paid_scarab_queued(slot) == queued + 1;
    }
}

static int attack_with(int slot, int x, int y)
{
    if (!select_unit(slot) || !choose_menu_command(MENU_ATTACK) || mode != MODE_ATTACK) return 0;
    move_cursor(x, y);
    if (mode != MODE_ATTACK) return 0;
    tap(KEY_A);
    return u[slot].order == ORDER_ATTACK_MOVE;
}

static void normalise_ui(void)
{
    /* UI, not world: close menus, drop the selection, return the remembered
     * order-list row to the top, so a fresh process starts the same way. */
    for (int k = 0; k < 4 && (menu_open || mode != MODE_NORMAL); ++k) tap(KEY_B);
    clear_building_selection();
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    menu_last = 0;
}

enum { CHECK_AT_COUNT = 10 };
static const unsigned check_at[CHECK_AT_COUNT] = {1, 2, 3, 5, 10, 20, 40, 80, 160, 320};

/* A few readable world values at each check, for comparison with the ARM replay. */
typedef struct { int frame, minerals, gas, scarabs, x, y, life; unsigned host_tick; } Sample;
static Sample take_sample(void)
{
    Sample v = {frame, minerals, gas, scarabs_alive(), -1, -1, -1, host_ticks};
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SCARAB) { v.x = u[i].x; v.y = u[i].y; v.life = u[i].work_timer; break; }
    return v;
}
static Sample samples[CHECK_AT_COUNT];

static void write_state(const char *path, const u32 *digests, unsigned frame_at_save, u32 d0)
{
    char side[512]; FILE *f;
    snprintf(side, sizeof side, "%s.state", path);
    f = fopen(side, "w"); assert(f);
    fprintf(f, "%u %u\n", frame_at_save, d0);
    for (int k = 0; k < CHECK_AT_COUNT; ++k) fprintf(f, "%u\n", digests[k]);
    fprintf(f, "%u\n", hit_count);
    for (unsigned i = 0; i < hit_count; ++i)
        fprintf(f, "%u %d %d\n", hit_log[i].at, hit_log[i].target, hit_log[i].damage);
    fprintf(f, "U_SCARAB %d\n", (int)U_SCARAB);
    for (int k = 0; k < CHECK_AT_COUNT; ++k)
        fprintf(f, "S %u %d %d %d %d %d %d %d %u\n", check_at[k], samples[k].frame, samples[k].minerals, samples[k].gas,
                samples[k].scarabs, samples[k].x, samples[k].y, samples[k].life, samples[k].host_tick);
    assert(!fclose(f));
}

static int run_scenario(const char *out, const char *trace_path)
{
    int robotics = -1, reaver[2] = {-1, -1}, target_building;
    unsigned flights = 0, best_splash = 0, empty_reach_ticks = 0;
    int launched_from_empty = 0, saved = 0;
    u32 digests[CHECK_AT_COUNT]; u32 d0 = 0; unsigned frame_at_save = 0;
    option_map = 1; option_human = R_PROTOSS; option_bots = 1; option_bot_races[0] = R_PROTOSS;
    match_limit = 1000000u;
    if (trace_path) {
        trace = fopen(trace_path, "w"); assert(trace);
        fputs("{\"profile\":\"unified-race-skirmish\",\"events\":[\n", trace);
    }
    gba_init();
    if (!launch_match()) { fprintf(stderr, "normal-menu launch failed\n"); return 1; }
    minerals = gas = 4000;       /* the one fixture: a bank that affords the tech chain */
    for (int i = 0; i < 600; ++i) input(0);
    assert(build_structure(156));      /* Pylon */
    assert(build_structure(160));      /* Gateway */
    assert(build_structure(164));      /* Cybernetics Core */
    assert(build_structure(155));      /* Robotics Facility */
    assert(build_structure(171));      /* Robotics Support Bay */
    printf("chain built at frame %d: pylon, gateway, core, robotics, support bay\n", frame);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_ROBOTICS_FACILITY) robotics = i;
    assert(robotics >= 0);
    /* Train two Reavers from the ordinary production menu. */
    for (int n = 0; n < 2; ++n) {
        int before = minerals;
        assert(select_building(robotics));
        for (int step = 0; train_type != U_REAVER && step < 8; ++step) tap(KEY_R);
        assert(train_type == U_REAVER);
        assert(paid_press((int)sc64_race_unit_rule(U_REAVER)->minerals,
                          (int)sc64_race_unit_rule(U_REAVER)->gas));
    }
    for (unsigned t = 0; t < 6000 && count_units(SIDE, U_REAVER) < 2; ++t) input(0);
    assert(count_units(SIDE, U_REAVER) == 2);
    {
        int k = 0;
        for (int i = 0; i < MAX_UNITS && k < 2; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_REAVER) reaver[k++] = i;
    }
    assert(race_paid_scarab_stock(reaver[0]) == 0 && race_paid_scarab_stock(reaver[1]) == 0);
    printf("two Reavers trained at frame %d, both empty\n", frame);
    for (int n = 0; n < 3; ++n) assert(buy_scarab(reaver[0]));
    assert(race_paid_scarab_queued(reaver[0]) + race_paid_scarab_stock(reaver[0]) == 3);
    for (unsigned t = 0; t < 600 && race_paid_scarab_stock(reaver[0]) < 3; ++t) input(0);
    assert(race_paid_scarab_stock(reaver[0]) == 3 && race_paid_scarab_stock(reaver[1]) == 0);
    printf("Reaver %d bought 3 Scarabs at 15 each; Reaver %d none (frame %d)\n", reaver[0], reaver[1], frame);
    target_building = closest_hostile_building();
    assert(target_building >= 0);
    assert(attack_with(reaver[0], b[target_building].x, b[target_building].y));
    assert(attack_with(reaver[1], b[target_building].x, b[target_building].y));
    for (unsigned t = 0; t < 30000 && screen == SCREEN_PLAY && !won && !lost; ++t) {
        int s0 = scarab_of(reaver[0]);
        if (scarab_of(reaver[1]) >= 0) launched_from_empty = 1;
        if (u[reaver[1]].alive && race_paid_scarab_stock(reaver[1]) == 0) {
            int in_reach = 0;
            for (int i = 0; i < MAX_BUILDINGS && !in_reach; ++i)
                in_reach = b[i].alive && owners_hostile(SIDE, b[i].owner) &&
                    dist2(u[reaver[1]].x, u[reaver[1]].y, b[i].x, b[i].y) < 140 * 140;
            for (int i = 0; i < MAX_UNITS && !in_reach; ++i)
                in_reach = u[i].alive && !SC64_SCARAB_HIDDEN(&u[i]) && owners_hostile(SIDE, u[i].owner) &&
                    dist2(u[reaver[1]].x, u[reaver[1]].y, u[i].x, u[i].y) < 128 * 128;
            empty_reach_ticks += in_reach;
        }
        if (s0 >= 0 && !saved) {
            /* A Scarab is in flight: save through the real pause menu. */
            unsigned before_hits = hit_count; (void)before_hits;
            normalise_ui();
            tap(KEY_START); assert(screen == SCREEN_PAUSE);
            input(KEY_B);                                  /* SRAM write */
            d0 = state_digest(); frame_at_save = (unsigned)frame;
            assert(scarab_of(reaver[0]) >= 0);
            { FILE *f = fopen(out, "wb"); assert(f && fwrite(host_save, 1, sizeof host_save, f) == sizeof host_save); fclose(f); }
            hit_count = 0; hit_epoch = host_ticks;
            input(KEY_A); input(0);                        /* unpause, first play tick */
            {
                unsigned done = 1; int ci = 0;
                while (ci < CHECK_AT_COUNT) {
                    if (done == check_at[ci]) { samples[ci] = take_sample(); digests[ci++] = state_digest(); }
                    if (ci < CHECK_AT_COUNT) { input(0); ++done; }
                }
            }
            saved = 1; ++flights;
            write_state(out, digests, frame_at_save, d0);
            printf("saved with a Scarab in flight at frame %u; continued %u updates, %u Scarab hits logged\n",
                   frame_at_save, check_at[CHECK_AT_COUNT - 1], hit_count);
            continue;
        }
        input(0);
        /* Splash: distinct targets hit by Scarabs in one update. */
        {
            static unsigned last_at = ~0u; unsigned n = 0, at = 0;
            for (unsigned i = 0; i < hit_count; ++i) at = hit_log[i].at;
            (void)last_at; (void)at; (void)n;
        }
        if (saved && s0 < 0 && scarab_of(reaver[0]) < 0 && flights) { /* nothing */ }
        if (s0 >= 0) ++flights;
    }
    /* Splash across the whole log: the most distinct targets in one update. */
    for (unsigned i = 0; i < hit_count; ++i) {
        unsigned n = 0; int seen[32] = {0};
        for (unsigned j = 0; j < hit_count && n < 32; ++j) if (hit_log[j].at == hit_log[i].at) {
            int dup = 0; for (unsigned k = 0; k < n; ++k) dup |= seen[k] == hit_log[j].target;
            if (!dup) seen[n++] = hit_log[j].target;
        }
        if (n > best_splash) best_splash = n;
    }
    printf("end: frame %d won %d lost %d; Reaver A %s, Reaver B %s (order %d, at %d,%d, stock %d)\n", frame, won, lost,
           u[reaver[0]].alive ? "alive" : "dead", u[reaver[1]].alive && u[reaver[1]].type == U_REAVER ? "alive" : "dead",
           u[reaver[1]].order, u[reaver[1]].x, u[reaver[1]].y, race_paid_scarab_stock(reaver[1]));
    printf("reaver-empty fired: %s; empty Reaver was within reach of an enemy for %u updates\n",
           launched_from_empty ? "YES" : "never", empty_reach_ticks);
    printf("Scarab in flight updates: %u; most distinct targets hit by one explosion: %u; hits logged: %u\n",
           flights, best_splash, hit_count);
    assert(saved && !launched_from_empty && flights > 0 && best_splash >= 2);
    if (trace) fclose(trace);
    return 0;
}

static int resume_scenario(const char *out)
{
    char side[512]; FILE *f; unsigned frame_at_save, nhits, sh; u32 d0, want[CHECK_AT_COUNT];
    struct { unsigned at; int target, damage; } want_hits[MAX_LOG];
    snprintf(side, sizeof side, "%s.state", out);
    f = fopen(side, "r"); assert(f);
    assert(fscanf(f, "%u %u", &frame_at_save, &d0) == 2);
    for (int k = 0; k < CHECK_AT_COUNT; ++k) assert(fscanf(f, "%u", &want[k]) == 1);
    assert(fscanf(f, "%u", &nhits) == 1 && nhits <= MAX_LOG);
    for (unsigned i = 0; i < nhits; ++i)
        assert(fscanf(f, "%u %d %d", &want_hits[i].at, &want_hits[i].target, &want_hits[i].damage) == 3);
    fclose(f);
    gba_init(); game_init(); prior_keys = 0;
    f = fopen(out, "rb"); assert(f);
    assert(fread(host_save, 1, sizeof host_save, f) == sizeof host_save); fclose(f);
    input(KEY_L);                                      /* the real load action from the title */
    assert(screen == SCREEN_PLAY && race_mode && race_human == R_PROTOSS);
    assert((unsigned)frame == frame_at_save && state_digest() == d0);
    assert(scarabs_alive() >= 1);
    hit_count = 0; hit_epoch = host_ticks;
    sh = 0;
    for (int k = 0; k < CHECK_AT_COUNT; ++k) {
        while (sh < check_at[k]) { input(0); ++sh; }
        if (state_digest() != want[k]) {
            fprintf(stderr, "resume: digest mismatch after %u updates\n", check_at[k]); return 1;
        }
    }
    /* Hits: same targets and damage in the same order, including the impact. */
    if (hit_count != nhits) { fprintf(stderr, "resume: %u hits, expected %u\n", hit_count, nhits); return 1; }
    for (unsigned i = 0; i < nhits; ++i)
        if (hit_log[i].target != want_hits[i].target || hit_log[i].damage != want_hits[i].damage) {
            fprintf(stderr, "resume: hit %u differs\n", i); return 1;
        }
    printf("resume: fresh process loaded the in-flight save; %d digests and %u Scarab hits match through impact\n",
           CHECK_AT_COUNT, nhits);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "run")) return run_scenario(argv[2], argc > 3 ? argv[3] : NULL);
    if (argc == 3 && !strcmp(argv[1], "resume")) return resume_scenario(argv[2]);
    fprintf(stderr, "usage: %s run OUT [TRACE] | resume OUT\n", argv[0]);
    return 2;
}
