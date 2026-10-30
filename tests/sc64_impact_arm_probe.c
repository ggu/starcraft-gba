/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * No-GDB ARM timing probe for a projectile-heavy battle (replaces
 * common/main.c). Dragoons and Goliaths attack-move into Mutalisks and
 * Hydralisks; the shipped game_update()/game_render() run for a fixed number
 * of updates while the cycle timer measures update and drawing work (the
 * present step's deliberate VBlank pacing is excluded). Results, including
 * the pending-impact high-water mark and deferred launches when the queue is
 * compiled in, go to the mGBA debug log. Emulator evidence only: it is not a
 * hardware frame-rate measurement.
 */
#include "../starcraft/game.c"

volatile u16 demo_keys;
extern volatile u32 gfx_draw_cycles; /* drawing only; gfx_end then paces to VBlank */

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)
#define PROBE_UPDATES 1200
#define PROBE_BUDGET 561792u /* two 60 Hz frames: one 30 Hz update + render */

static char line[200];
static unsigned line_len;

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(unsigned long v)
{
    char tmp[12]; int n = 0;
    do { tmp[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_hex(u32 v)
{
    static const char digits[] = "0123456789abcdef";
    char c[2] = { 0, 0 };
    for (int s = 28; s >= 0; s -= 4) { c[0] = digits[v >> s & 15u]; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
    line_len = 0;
}

static void spawn_group(int owner, int type, int count, int x0, int y0, int tx, int ty)
{
    for (int i = 0; i < count; ++i) {
        int slot = spawn_unit(owner, type, x0 + (i % 4) * 14, y0 + (i / 4) * 14);
        if (slot < 0) continue;
        u[slot].raw_owner = owner == SIDE ? 0 : 1;
        u[slot].order = ORDER_ATTACK_MOVE;
        u[slot].target = -1;
        u[slot].tx = tx; u[slot].ty = ty;
    }
}

static u32 world_digest(void)
{
    u32 h = 2166136261u;
    for (int i = 0; i < MAX_UNITS; ++i) {
        u32 v[5] = { (u32)u[i].alive, (u32)u[i].hp, (u32)u[i].shield,
                     (u32)u[i].x, (u32)u[i].y };
        for (int k = 0; k < 5; ++k) { h ^= v[k]; h *= 16777619u; }
    }
    return h;
}

int main(void)
{
    gba_init();
    DBG_ENABLE = 0xC0DE;
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    race_player[SIDE] = R_PROTOSS; race_player[ENEMY] = R_ZERG;
    selected_building = -1; menu_open = 0; mode = MODE_NORMAL;
    clear_tracers();
    /* Distant structures keep both players in the match for every update. */
    (void)spawn_build(SIDE, B_NEXUS, 64, 64, 0);
    (void)spawn_build(ENEMY, B_HATCHERY, 64, 560, 0);
    spawn_group(SIDE, U_DRAGOON, 16, 220, 260, 420, 290);
    spawn_group(SIDE, U_GOLIATH, 8, 220, 330, 420, 290);
    spawn_group(ENEMY, U_MUTALISK, 14, 400, 250, 220, 290);
    spawn_group(ENEMY, U_HYDRALISK, 10, 400, 320, 220, 290);
    camx = 200; camy = 230; cx = 320; cy = 290;

    unsigned long update_sum = 0, render_sum = 0;
    unsigned long window_update = 0, window_draw = 0, window_tracers = 0;
    u32 update_max = 0, render_max = 0, over = 0, first_alive = 0, ticks = 0, max_tick = 0;
    for (int tick = 0; tick < PROBE_UPDATES && screen == SCREEN_PLAY; ++tick) {
        demo_keys = 0;
        u32 t0 = gba_clock();
        game_update(0, 0);
        u32 t1 = gba_clock();
        game_render();
        u32 du = t1 - t0, dr = gfx_draw_cycles;
        update_sum += du; render_sum += dr;
        if (du > update_max) { update_max = du; max_tick = (u32)tick; }
        if (dr > render_max) render_max = dr;
        if (du + dr > PROBE_BUDGET) ++over;
        ++ticks;
        window_update += du; window_draw += dr;
        for (int i = 0; i < MAX_TRACERS; ++i) window_tracers += tracers[i].life != 0;
        if (tick % 100 == 99) {
            unsigned alive = 0;
            for (int i = 0; i < MAX_UNITS; ++i) alive += (unsigned)u[i].alive;
            out_text("IMPACT WINDOW end="); out_num((unsigned long)tick + 1u);
            out_text(" update="); out_num(window_update / 100u);
            out_text(" draw="); out_num(window_draw / 100u);
            out_text(" tracers="); out_num(window_tracers / 100u);
            out_text(" alive="); out_num(alive);
            out_flush();
            window_update = window_draw = window_tracers = 0;
        }
        if (tick == 0)
            for (int i = 0; i < MAX_UNITS; ++i) first_alive += (u32)u[i].alive;
    }
    unsigned alive_side = 0, alive_enemy = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) { if (u[i].owner == SIDE) ++alive_side; else ++alive_enemy; }
    if (!ticks) ticks = 1;
    out_text("IMPACT TIMING updates="); out_num(ticks);
    out_text(" update_avg="); out_num(update_sum / ticks);
    out_text(" update_max="); out_num(update_max);
    out_text(" max_tick="); out_num(max_tick);
    out_text(" draw_avg="); out_num(render_sum / ticks);
    out_text(" draw_max="); out_num(render_max);
    out_text(" over_budget="); out_num(over);
    out_flush();
    out_text("IMPACT WORLD units="); out_num(first_alive);
    out_text(" screen="); out_num((unsigned long)screen);
    out_text(" side="); out_num(alive_side); out_text(" enemy="); out_num(alive_enemy);
    out_text(" digest="); out_hex(world_digest());
#ifdef SC64_IMPACT_EVENTS
    out_text(" capacity="); out_num(RACE_IMPACT_CAPACITY);
    out_text(" high_water="); out_num(race_impact_high_water);
    out_text(" deferred="); out_num(race_impact_deferred);
#else
    out_text(" capacity=0");
#endif
    out_flush();
#ifdef SC64_IMPACT_EVENTS
    /* The queue's own cost in isolation: launch timing for one Dragoon shot,
     * pushing a row, and one end-of-update step over a full queue. */
    clear_tracers();
    int a = spawn_unit(SIDE, U_DRAGOON, 300, 300);
    int t = spawn_unit(ENEMY, U_ZERGLING, 356, 300);
    unsigned steps = 0;
    u32 c0 = gba_clock();
    for (int i = 0; i < 64; ++i) steps += race_impact_unit_steps(&u[a], t, 0);
    u32 c1 = gba_clock();
    for (int i = 0; i < RACE_IMPACT_CAPACITY; ++i)
        race_impact_launch(t, 1, 1, ENEMY, 0, U_MARINE, 200u);
    u32 c2 = gba_clock();
    race_impact_tick();
    u32 c3 = gba_clock();
    out_text("IMPACT MICRO steps="); out_num(steps / 64u);
    out_text(" timing_cycles="); out_num((c1 - c0) / 64u);
    out_text(" push_cycles="); out_num((c2 - c1) / RACE_IMPACT_CAPACITY);
    out_text(" full_step_cycles="); out_num(c3 - c2);
    out_flush();
#endif
    out_text("IMPACT DONE"); out_flush();
    for (;;) __asm__ volatile ("" ::: "memory");
}
