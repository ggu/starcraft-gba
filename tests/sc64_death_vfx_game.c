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
        int after = count_alive();
        if (after < before) deaths += before - after;
#ifndef SC64_DEATH_VFX_DISABLED
        int active = sc64_death_vfx_active(death_vfx, SC64_DEATH_VFX_POOL, (u32)frame);
        if (active > peak) peak = active;
#endif
    }
    u32 state = state_hash();
    save_game();
    u32 save = fnv(2166136261u, host_save, sizeof(host_save));
    printf("STATE %08x SAVE %08x\n", (unsigned)state, (unsigned)save);
    fprintf(stderr, "death workload: %d actor deaths, peak %d active effects\n", deaths, peak);
#ifndef SC64_DEATH_VFX_DISABLED
    assert(deaths > 0 && peak > 0);
#endif
    return 0;
}

#ifndef SC64_DEATH_VFX_DISABLED
static int active_now(void)
{
    return sc64_death_vfx_active(death_vfx, SC64_DEATH_VFX_POOL, (u32)frame);
}

static const Sc64DeathVfx *only_effect(void)
{
    const Sc64DeathVfx *found = 0;
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i)
        if (!sc64_death_vfx_expired(&death_vfx[i], (u32)frame)) { assert(!found); found = &death_vfx[i]; }
    assert(found);
    return found;
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
        sc64_death_vfx_spawn(death_vfx, SC64_DEATH_VFX_POOL, (u32)frame, style, raws[style],
                             camx + 30 + col * 60, camy - 12 + 30 + row * 36,
                             style >= SC64_DEATH_VFX_BUILDING_TERRAN ? 18 : 6, &t, 0, C_GREEN);
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
    if (getenv("SC64_DEATH_VFX_HASH")) return run_hash_workload();
#ifdef SC64_DEATH_VFX_DISABLED
    puts("death VFX disabled build: run with SC64_DEATH_VFX_HASH=1");
    return 0;
#else
    test_unit_death_and_expiry();
    test_removal_and_buildings();
    test_loads_reprime();
    test_pool_overflow();
    dump_gallery();
    puts("PASS observed unit/building deaths, expiry, fog, removals, load re-prime and pool overflow");
    return 0;
#endif
}
