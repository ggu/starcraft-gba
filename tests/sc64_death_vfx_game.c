/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Death effects are observed from real kills, expire, ignore scripted
 * removals and loads, and never change simulation or save bytes. Build once
 * normally and once with -DSC64_DEATH_VFX_DISABLED; with SC64_DEATH_VFX_HASH
 * set, both print the same state/save digest over a combat workload. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static u32 fnv(u32 h, const void *data, unsigned n)
{
    const u8 *p = data;
    while (n--) { h ^= *p++; h *= 16777619u; }
    return h;
}
#define MIX(h, v) do { int value_ = (int)(v); h = fnv(h, &value_, sizeof(value_)); } while (0)
static int pixel_hash_enabled;
static u32 pixel_hash = 2166136261u;
static void hash_frame_pixels(void)
{
    if (!pixel_hash_enabled) return;
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
        u16 rgb555 = host_palette[(u8)gfx_test_pixel(x, y)];
        pixel_hash ^= (u8)rgb555; pixel_hash *= 16777619u;
        pixel_hash ^= (u8)(rgb555 >> 8); pixel_hash *= 16777619u;
    }
}

/* Field-wise digest: struct padding is not simulation state. */
static u32 state_hash(void)
{
    u32 h = 2166136261u;
    for (int i = 0; i < MAX_UNITS; ++i) {
        MIX(h, u[i].alive);
        if (!u[i].alive) continue;
        MIX(h, u[i].x); MIX(h, u[i].y); MIX(h, u[i].hp); MIX(h, u[i].type);
        MIX(h, u[i].owner); MIX(h, u[i].order); MIX(h, u[i].target); MIX(h, u[i].cd);
        MIX(h, u[i].tx); MIX(h, u[i].ty);
#ifdef SC64_RACES
        MIX(h, u[i].shield);
#endif
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(h, b[i].alive);
        if (!b[i].alive) continue;
        MIX(h, b[i].x); MIX(h, b[i].y); MIX(h, b[i].hp); MIX(h, b[i].type);
        MIX(h, b[i].owner); MIX(h, b[i].build_progress);
    }
    MIX(h, minerals); MIX(h, gas); MIX(h, supply_used); MIX(h, supply_cap);
    MIX(h, frame); MIX(h, seed); MIX(h, won); MIX(h, lost);
    h = fnv(h, explored, sizeof(explored));
    for (int i = 0; i < MAX_TRACERS; ++i) {
        MIX(h, tracers[i].life); MIX(h, tracers[i].x0); MIX(h, tracers[i].y1);
    }
    return h;
}

static void setup_world(void)
{
    game_init();
#ifdef SC64_RACES
    race_mode = 1; race_human = R_ZERG; race_bots = 1;
    race_skirmish_init();
#else
    init_state();
#endif
    screen = SCREEN_PLAY;
}

static int base_x, base_y;

static void find_base(void)
{
    base_x = 160; base_y = 120;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) { base_x = b[i].x; base_y = b[i].y; return; }
}

#ifdef SC64_RACES
#define ALLY_TYPE U_ZERGLING
#define FOE_TYPE U_MARINE
#else
#define ALLY_TYPE U_MARINE
#define FOE_TYPE U_ZERGLING
#endif

static void spawn_skirmish(int count)
{
    for (int i = 0; i < count; ++i) {
        int a = spawn_unit(SIDE, ALLY_TYPE, base_x + 40 + (i % 4) * 6, base_y + 30 + (i / 4) * 6);
        int f = spawn_unit(ENEMY, FOE_TYPE, base_x + 70 + (i % 4) * 6, base_y + 30 + (i / 4) * 6);
        assert(a >= 0 && f >= 0);
        u[a].order = ORDER_ATTACK_TARGET; u[a].target = f;
        u[f].order = ORDER_ATTACK_TARGET; u[f].target = a;
    }
}

static int count_alive(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive != 0;
    return n;
}

/* Destroy every opposing building (and, in the original-art game, every
 * enemy unit) so the next update reaches the victory outcome. */
static void destroy_opposition(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        for (int guard = 0; b[i].alive && b[i].owner != SIDE && b[i].owner != NEUTRAL &&
             guard < 10000; ++guard)
            hit_target(MAX_UNITS + i, 400, ALLY_TYPE, SIDE);
#ifndef SC64_RACES
    for (int i = 0; i < MAX_UNITS; ++i)
        while (u[i].alive && u[i].owner == ENEMY) hit_target(i, 500, ALLY_TYPE, SIDE);
#endif
}

static int run_hash_workload(void)
{
    int deaths = 0, peak = 0;
    setup_world();
    find_base();
    spawn_skirmish(8);
    memset(visible, 0xff, sizeof(visible));
    for (int step = 0; step < 900 && screen == SCREEN_PLAY; ++step) {
        int before = count_alive();
        game_update(0, 0);
        game_render();
        hash_frame_pixels();
        int after = count_alive();
        if (after < before) deaths += before - after;
#ifndef SC64_DEATH_VFX_DISABLED
        Sc64DeathVfx pool[SC64_DEATH_VFX_POOL];
        death_vfx_test_load_pool(pool);
        int active = sc64_death_vfx_active(pool, SC64_DEATH_VFX_POOL, (u32)frame);
        if (active > peak) peak = active;
#endif
    }
    /* End of mission: the outcome update and frozen result screen. */
    int outcome_frame = -1;
    if (screen == SCREEN_PLAY) {
        destroy_opposition();
        game_update(0, 0);
        game_render();
        hash_frame_pixels();
        outcome_frame = frame;
    }
    for (int step = 0; step < 120; ++step) { game_update(0, 0); game_render(); hash_frame_pixels(); }
    printf("OUTCOME screen %d won %d lost %d frame %d\n", screen, won, lost, outcome_frame);
    u32 state = state_hash();
    save_game();
    u32 save = fnv(2166136261u, host_save, sizeof(host_save));
    printf("STATE %08x SAVE %08x\n", (unsigned)state, (unsigned)save);
    if (pixel_hash_enabled) printf("RGB555 %08x\n", (unsigned)pixel_hash);
    fprintf(stderr, "death workload: %d actor deaths, peak %d active effects\n", deaths, peak);
#ifndef SC64_DEATH_VFX_DISABLED
    assert(deaths > 0 && peak > 0);
#endif
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    assert(death_vfx_test_storage_guards_ok());
#endif
    return 0;
}

#ifndef SC64_DEATH_VFX_DISABLED
static int active_now(void)
{
    Sc64DeathVfx pool[SC64_DEATH_VFX_POOL];
    death_vfx_test_load_pool(pool);
    return sc64_death_vfx_active(pool, SC64_DEATH_VFX_POOL, death_vfx_now());
}

static const Sc64DeathVfx *only_effect(void)
{
    static Sc64DeathVfx found;
    int count = 0;
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i)
    {
        Sc64DeathVfx e;
        death_vfx_test_load_row(i, &e);
        if (!sc64_death_vfx_expired(&e, death_vfx_now())) { assert(!count); found = e; ++count; }
    }
    assert(count == 1);
    return &found;
}

static int spawn_test_effect(int style, int raw, int x, int y, int extent,
                             const Sc64DeathVfxTiming *timing, int source, int color)
{
    Sc64DeathVfx pool[SC64_DEATH_VFX_POOL];
    death_vfx_test_load_pool(pool);
    int slot = sc64_death_vfx_spawn(pool, SC64_DEATH_VFX_POOL, (u32)frame,
                                    style, raw, x, y, extent, timing, source, color);
    death_vfx_test_store_pool(pool);
    return slot;
}

static int drawn_pixels(int under)
{
    int changed = 0;
    gfx_begin(C_PANEL);
    draw_death_vfx(under);
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
        int p = gfx_test_pixel(x, y);
        if (y < 12 || y > 119) assert(p == C_PANEL);
        else changed += p != C_PANEL;
    }
    return changed;
}

static void aim_camera(int x, int y)
{
    memset(visible, 0xff, sizeof(visible));
    camx = x - 120; camy = y - 54;
}

static void test_unit_death_and_expiry(void)
{
    setup_world(); find_base();
    death_vfx_scan(); /* prime */
    int x = base_x + 50, y = base_y + 40;
    int victim = spawn_unit(ENEMY, FOE_TYPE, x, y);
    assert(victim >= 0);
    death_vfx_scan();
    assert(active_now() == 0);
    u32 before = state_hash();
    while (u[victim].alive) hit_target(victim, 500, ALLY_TYPE, SIDE);
    assert(active_now() == 0);          /* the simulation itself spawns nothing */
    u32 killed = state_hash();
    death_vfx_scan();
    assert(state_hash() == killed && killed != before);
    const Sc64DeathVfx *e = only_effect();
    int raw = death_vfx_raw(victim);
    assert(e->x == x && e->y == y && e->raw == raw);
    assert(e->style == sc64_death_vfx_style_for_raw(raw, 0));
    aim_camera(x, y);
    u32 born = (u32)frame;
    int any_burst = 0, any_under = 0;
    while (sc64_death_vfx_age_su(born, (u32)frame) < e->total_su) {
        any_burst |= drawn_pixels(0) > 0;
        any_under |= drawn_pixels(1) > 0;
        ++frame;
        death_vfx_scan();            /* a dead row never re-triggers */
        assert(active_now() <= 1);
    }
    assert(any_burst || any_under);
    assert(active_now() == 0);
    assert(!drawn_pixels(0) && !drawn_pixels(1));
    /* Fogged cells hide the effect, as they hide units. */
    int again = spawn_unit(ENEMY, FOE_TYPE, x, y);
    death_vfx_scan();
    while (u[again].alive) hit_target(again, 500, ALLY_TYPE, SIDE);
    death_vfx_scan();
    assert(active_now() == 1);
    memset(visible, 0, sizeof(visible));
    assert(!drawn_pixels(0) && !drawn_pixels(1));
}

static void test_removal_and_buildings(void)
{
    setup_world(); find_base();
    death_vfx_scan();
    int removed = spawn_unit(ENEMY, FOE_TYPE, base_x + 30, base_y + 30);
    death_vfx_scan();
    u[removed].alive = 0;               /* scripted removal keeps its HP */
    death_vfx_scan();
    assert(active_now() == 0);
    int target = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type != B_COMMAND && !b[i].under_construction) { target = i; break; }
    if (target < 0)
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) { target = i; break; }
    assert(target >= 0);
    int bx = b[target].x, by = b[target].y;
    for (int guard = 0; b[target].alive && guard < 10000; ++guard)
        hit_target(MAX_UNITS + target, 400, ALLY_TYPE, ENEMY);
    assert(!b[target].alive);
    death_vfx_scan();
    const Sc64DeathVfx *e = only_effect();
    int raw = death_vfx_raw(MAX_UNITS + target);
    assert(e->x == bx && e->y == by);
    assert(e->style == sc64_death_vfx_style_for_raw(raw, 1));
    assert(e->style >= SC64_DEATH_VFX_BUILDING_TERRAN);
    aim_camera(bx, by);
    assert(drawn_pixels(0) > 0);
    frame += 60;                        /* past the burst: rubble remains */
    assert(active_now() == 1 && drawn_pixels(0) == 0 && drawn_pixels(1) > 0);
}

static void test_loads_reprime(void)
{
    setup_world(); find_base();
    int victim = spawn_unit(ENEMY, FOE_TYPE, base_x + 40, base_y + 40);
    death_vfx_scan(); death_vfx_scan();
    assert(save_game());
    u32 saved = state_hash();
    while (u[victim].alive) hit_target(victim, 500, ALLY_TYPE, SIDE);
    death_vfx_scan();
    assert(active_now() == 1);
    assert(load_game());                /* restores the victim; clears effects */
    assert(state_hash() == saved && u[victim].alive);
    assert(active_now() == 0);
    death_vfx_scan();
    assert(active_now() == 0);
    /* A loaded world with dead rows does not replay their deaths. */
    for (int i = 0; i < MAX_UNITS; ++i) if (!u[i].alive) u[i].hp = 0;
    death_vfx_scan();
    assert(active_now() == 0);
}

static void test_pool_overflow(void)
{
    setup_world(); find_base();
    death_vfx_scan();
    int slots[SC64_DEATH_VFX_POOL + 4];
    for (int i = 0; i < SC64_DEATH_VFX_POOL + 4; ++i) {
        slots[i] = spawn_unit(ENEMY, FOE_TYPE, base_x + (i % 8) * 8, base_y + 40 + (i / 8) * 8);
        assert(slots[i] >= 0);
    }
    death_vfx_scan();
    for (int i = 0; i < SC64_DEATH_VFX_POOL + 4; ++i) {
        while (u[slots[i]].alive) hit_target(slots[i], 500, ALLY_TYPE, SIDE);
        death_vfx_scan(); ++frame;
    }
    assert(active_now() == SC64_DEATH_VFX_POOL);
}

static int panel_pixels(void)
{
    int n = 0;
    for (int y = 42; y < 104; ++y) for (int x = 30; x < 210; ++x) n += gfx_test_pixel(x, y) == C_PANEL;
    return n;
}

static void test_result_outro(void)
{
    setup_world(); find_base();
    for (int i = 0; i < 3; ++i) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    destroy_opposition();
    int before = frame;
    game_update(0, 0);
    assert(screen == SCREEN_RESULT && won && !lost);
    int outcome = frame;
    assert(outcome == before + 1);
    u32 frozen = state_hash();
    game_update(0, 0);   /* first result update: final deaths are scanned */
    assert(death_vfx_bursting(death_vfx_now()));   /* final buildings exploding */
    assert(!result_outro_end_get());
    memset(visible, 0xff, sizeof(visible));
    int full = 180 * 62;
    game_render();
    assert(panel_pixels() < full / 2);              /* battlefield, not the panel */
    int ticks = 1;
    while (!result_outro_end_get()) { game_update(0, 0); ++ticks; assert(ticks <= (int)SC64_RESULT_OUTRO_MAX); }
    assert(ticks >= (int)SC64_RESULT_OUTRO_MIN);
    assert(ticks == (int)SC64_RESULT_OUTRO_MAX || !death_vfx_bursting(death_vfx_now()));
    for (unsigned i = 0; i < SC64_RESULT_PANEL_OPEN; ++i) {
        game_render();
        int opening = panel_pixels();
        assert(opening > 0 && opening < full);
        game_update(0, 0);
    }
    game_render();
    assert(panel_pixels() > full * 3 / 4);           /* full result panel */
    assert(state_hash() == frozen && frame == outcome && screen == SCREEN_RESULT && won);
    fprintf(stderr, "result outro: %d updates before the panel\n", ticks);
    /* Input is unchanged: A during the outro acts at once. */
    setup_world(); find_base();
    game_update(0, 0); destroy_opposition(); game_update(0, 0);
    assert(screen == SCREEN_RESULT && !result_outro_end_get());
    game_update(KEY_A, KEY_A);
    assert(screen == SCREEN_PLAY && !won);
    /* A world loaded on its result screen shows the panel at once. */
    death_vfx_reset();
    assert(result_outro_end_get() && result_outro_get() >= result_outro_end_get() + SC64_RESULT_PANEL_OPEN);
    screen = SCREEN_RESULT; won = 1;
    game_render();
    assert(panel_pixels() > full * 3 / 4);
}

static void dump_gallery(void)
{
    const char *out = getenv("SC64_DEATH_GALLERY");
    if (!out) return;
    setup_world(); find_base();
    death_vfx_reset(); death_vfx_scan();
    for (int style = 1; style < SC64_DEATH_VFX_STYLE_COUNT; ++style) {
        Sc64DeathVfxTiming t = sc64_death_vfx_timing(style);
        int col = (style - 1) % 4, row = (style - 1) / 4;
        static const int raws[] = { 0, 0, 7, 5, 37, 39, 43, 65, 83, 111, 142, 160 };
        assert(spawn_test_effect(style, raws[style], camx + 30 + col * 60,
                                 camy - 12 + 30 + row * 36,
                                 style >= SC64_DEATH_VFX_BUILDING_TERRAN ? 18 : 6,
                                 &t, 0, C_GREEN) >= 0);
    }
    memset(visible, 0xff, sizeof(visible));
    u32 born = (u32)frame;
    for (int shot = 0; shot < 12; ++shot) {
        frame = (int)(born + (u32)shot * 5);
        gfx_begin(C_DIRT);
        draw_death_vfx(1); draw_death_vfx(0);
        char path[1024];
        snprintf(path, sizeof(path), "%s/death-%02d.ppm", out, shot);
        gfx_end(); gfx_dump_ppm(path);
    }
}
#endif

int main(void)
{
    gba_init();
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    death_vfx_test_storage_guards_init();
#endif
    pixel_hash_enabled = getenv("SC64_DEATH_VFX_PIXEL_HASH") != 0;
    if (getenv("SC64_DEATH_VFX_HASH")) return run_hash_workload();
#ifdef SC64_DEATH_VFX_DISABLED
    puts("death VFX disabled build: run with SC64_DEATH_VFX_HASH=1");
    return 0;
#else
    test_unit_death_and_expiry();
    test_removal_and_buildings();
    test_loads_reprime();
    test_pool_overflow();
    test_result_outro();
    dump_gallery();
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    assert(death_vfx_test_storage_guards_ok());
#endif
    puts("PASS observed unit/building deaths, expiry, fog, removals, load re-prime, pool overflow and result outro");
    return 0;
#endif
}
