/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe for the optional Nydus runtime (replaces common/main.c).
 *
 * The probe boots the private Z7 controller profile with SC64_NYDUS_ENABLED,
 * reaches Z7 through the normal HOME/menu path using an embedded key tape,
 * then places two linked raw-134 endpoint pairs exactly as the host fixture
 * does (the job a Z8 mission adapter will own). Passengers move only through
 * the A command and game_update(). Results go to the mGBA debug log; no
 * debugger reads or writes game state. Emulator evidence only.
 */
#include "../starcraft/game.c"

volatile u16 demo_keys;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[200];
static unsigned line_len;
static u16 probe_previous;
static u32 probe_frame_cycles, probe_nydus_frame, probe_search_frame;

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[12]; int n = 0; unsigned long m;
    if (v < 0) { out_text("-"); m = (unsigned long)(-v); } else m = (unsigned long)v;
    do { tmp[n++] = (char)('0' + m % 10u); m /= 10u; } while (m && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
    line_len = 0;
}

/* One rendered frame with the shipped main-loop work; returns its cycles. */
static u32 frame_with(u16 held)
{
    u32 start = gba_clock(), nydus0 = race_nydus_probe_cycles, search0 = race_nydus_probe_searches;
    demo_keys = held;
    game_update(held, held & (u16)~probe_previous);
    audio_update();
    game_render();
    probe_previous = held;
    probe_frame_cycles = gba_clock() - start;
    probe_nydus_frame = race_nydus_probe_cycles - nydus0;
    probe_search_frame = race_nydus_probe_searches - search0;
    return probe_frame_cycles;
}
static void press(u16 key) { frame_with(key); frame_with(0); }

typedef struct { u32 frames; unsigned long long total, nydus, searches; u32 peak, nydus_peak; } Stat;
static void stat_add(Stat *s, u32 c)
{
    s->frames++; s->total += c; if (c > s->peak) s->peak = c;
    s->nydus += probe_nydus_frame; s->searches += probe_search_frame;
    if (probe_nydus_frame > s->nydus_peak) s->nydus_peak = probe_nydus_frame;
}
static void report(const char *name, const Stat *s)
{
    out_text("NYDUS "); out_text(name);
    out_text(" frames="); out_num((long)s->frames);
    out_text(" avg="); out_num(s->frames ? (long)(s->total / s->frames) : 0);
    out_text(" peak="); out_num((long)s->peak);
    out_text(" nydus_avg="); out_num(s->frames ? (long)(s->nydus / s->frames) : 0);
    out_text(" nydus_peak="); out_num((long)s->nydus_peak);
    out_text(" searches/frame="); out_num(s->frames ? (long)(s->searches / s->frames) : 0);
    out_flush();
}

static int probe_failed;
static void check(int ok, const char *what)
{
    if (ok) return;
    probe_failed = 1;
    out_text("NYDUS FAIL "); out_text(what); out_flush();
}

static int open_site(int near_x, int near_y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int y = near_y - 320; y <= near_y + 320; y += 16)
        for (int x = near_x - 320; x <= near_x + 320; x += 16) {
            int d = dist2(x, y, near_x, near_y), clear = 1;
            if (x < 96 || y < 96 || x >= WORLD_W - 96 || y >= WORLD_H - 96 ||
                d >= best_d || !can_place(B_NYDUS_CANAL, x, y)) continue;
            for (int oy = -48; oy <= 48 && clear; oy += 16)
                for (int ox = -48; ox <= 48 && clear; ox += 16)
                    if ((ox < -16 || ox > 16 || oy < -16 || oy > 16) &&
                        !is_traversable((x + ox) / 16, (y + oy) / 16, -1)) clear = 0;
            for (int i = 0; i < MAX_UNITS && clear; ++i)
                if (u[i].alive && !sc64_race_unit_flying(u[i].type) &&
                    FAST_ABS(u[i].x - x) < 72 && FAST_ABS(u[i].y - y) < 72) clear = 0;
            if (clear) { best = x + y * WORLD_W; best_d = d; }
        }
    return best;
}

static int place_endpoint(int x, int y)
{
    int site = open_site(x, y), slot;
    if (site < 0) return -1;
    slot = spawn_build(SIDE, B_NYDUS_CANAL, site % WORLD_W, site / WORLD_W, 0);
    invalidate_path_grid();
    return slot;
}

static Sc64NydusSourceRecord records[4];
static Sc64NydusSourceRecord record_for(int slot, unsigned index, unsigned linked)
{
    Sc64NydusSourceRecord r;
    memset(&r, 0, sizeof(r));
    r.record_index = (unsigned short)index;
    r.linked_record_index = (unsigned short)linked;
    r.raw_type = SC64_NYDUS_RAW;
    r.relation = SC64_NYDUS_RELATION_LINK;
    r.raw_owner = (unsigned char)b[slot].raw_owner;
    r.x = (short)b[slot].x; r.y = (short)b[slot].y;
    return r;
}

static int busy(const int *p, int n)
{
    for (int i = 0; i < n; ++i) if (u[p[i]].alive && u[p[i]].order == ORDER_NYDUS_ENTER) return 1;
    return 0;
}

int main(void)
{
    Stat baseline = { 0, 0, 0, 0, 0, 0 }, approach = { 0, 0, 0, 0, 0, 0 },
         transit = { 0, 0, 0, 0, 0, 0 };
    Stat blocked = { 0, 0, 0, 0, 0, 0 };
    int blocked_max = 0;
    int pass[6], n = 0, ent, ex, ent2, ex2, ordinal, transfers = 0, idle_before;
    gba_init();
    DBG_ENABLE = 0xC0DE;
    game_init();
    out_text("NYDUS START debug="); out_num(DBG_ENABLE == 0x1DEA); out_flush();
    press(KEY_A);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z7_GLOBAL_ID);
    for (int p = 0; p < ordinal / 2; ++p) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    press(KEY_START);
    check(screen == SCREEN_PLAY && race_z7_active(), "menu route");
    for (int f = 0; f < 4; ++f) frame_with(0);
    for (int f = 0; f < 240; ++f) stat_add(&baseline, frame_with(0));

    for (int i = 0; i < MAX_UNITS && n < 6; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_HYDRALISK) pass[n++] = i;
    check(n == 6, "six hydras");
    ent = place_endpoint(u[pass[0]].x, u[pass[0]].y - 96);
    ex = place_endpoint(u[pass[0]].x + 360, u[pass[0]].y - 260);
    ent2 = place_endpoint(u[pass[0]].x - 220, u[pass[0]].y - 120);
    ex2 = place_endpoint(u[pass[0]].x + 120, u[pass[0]].y - 420);
    check(ent >= 0 && ex >= 0 && ent2 >= 0 && ex2 >= 0, "sites");
    records[0] = record_for(ex2, 197, 195);
    records[1] = record_for(ent, 192, 194);
    records[2] = record_for(ent2, 195, 197);
    records[3] = record_for(ex, 194, 192);
    check(race_nydus_set_source(SC64_Z7_GLOBAL_ID, records, 4) &&
          race_nydus_bind_world(1), "bind");

    clear_selection();
    for (int i = 0; i < n; ++i) u[pass[i]].selected = 1;
    cx = b[ent].x; cy = b[ent].y;
    press(KEY_A);
    for (int i = 0; i < n; ++i) check(u[pass[i]].order == ORDER_NYDUS_ENTER, "order");
    for (int f = 0; f < 1200 && busy(pass, n); ++f) {
        int before = 0, after = 0;
        u32 c;
        for (int i = 0; i < n; ++i) before += u[pass[i]].order == ORDER_NYDUS_ENTER;
        c = frame_with(0);
        for (int i = 0; i < n; ++i) after += u[pass[i]].order == ORDER_NYDUS_ENTER;
        if (after < before) { stat_add(&transit, c); transfers += before - after; }
        else stat_add(&approach, c);
    }
    idle_before = 0;
    for (int i = 0; i < n; ++i) {
        idle_before += u[pass[i]].order == ORDER_IDLE;
        check(FAST_ABS(u[pass[i]].x - b[ex].x) <= 64 && FAST_ABS(u[pass[i]].y - b[ex].y) <= 64,
              "arrival");
    }
    check(transfers == n && idle_before == n, "transfers");

    /* Save, mutate, load: links rebind and passengers resume (in-process SRAM). */
    check(race_save_game(), "save");
    for (int f = 0; f < 30; ++f) frame_with(0);
    check(race_load_game() && sc64_nydus_partner(&race_nydus, (unsigned)ent) == ex &&
          sc64_nydus_partner(&race_nydus, (unsigned)ex2) == ent2, "load rebind");

    /* Worst case: the return trip (ex -> ent) with ent fully surrounded, so
     * every waiting passenger searches all 288 candidates each order step. */
    for (int oy = -72; oy <= 72; oy += 24)
        for (int ox = -72; ox <= 72; ox += 24)
            if (ox || oy) spawn_build(SIDE, B_EVOLUTION_CHAMBER, b[ent].x + ox, b[ent].y + oy, 0);
    invalidate_path_grid();
    {
        int bx, by, open = race_nydus_place(pass[0], ent, &bx, &by);
        out_text("NYDUS blockers open="); out_num(open);
        if (open) { out_text(" at "); out_num(bx - b[ent].x); out_text(","); out_num(by - b[ent].y); }
        out_flush();
    }
    clear_selection();
    for (int i = 0; i < n; ++i) u[pass[i]].selected = 1;
    cx = b[ex].x; cy = b[ex].y;
    press(KEY_A);
    {
        int issued = 0;
        for (int i = 0; i < n; ++i) issued += u[pass[i]].order == ORDER_NYDUS_ENTER;
        out_text("NYDUS return issued="); out_num(issued); out_flush();
        check(issued == n, "return order");
    }
    for (int f = 0; f < 900; ++f) {
        u32 c = frame_with(0);
        int waiting = 0;
        for (int i = 0; i < n; ++i)
            waiting += u[pass[i]].order == ORDER_NYDUS_ENTER &&
                       sc64_nydus_contact(u[pass[i]].x, u[pass[i]].y, b[ex].x, b[ex].y);
        if (waiting) {
            stat_add(&blocked, c);
            if (waiting > blocked_max) blocked_max = waiting;
        }
    }
    for (int i = 0; i < n; ++i)
        check(u[pass[i]].order == ORDER_NYDUS_ENTER &&
              !(FAST_ABS(u[pass[i]].x - b[ent].x) <= 64 && FAST_ABS(u[pass[i]].y - b[ent].y) <= 64),
              "blocked passengers hold");
    check(blocked.frames > 300, "blocked wait");

    report("baseline", &baseline);
    report("approach", &approach);
    report("transit", &transit);
    report("blocked6", &blocked);
    out_text("NYDUS blocked_max_waiting="); out_num(blocked_max);
    out_text(" cycles/search=");
    out_num(blocked.searches ? (long)(blocked.nydus / blocked.searches) : 0);
    out_flush();
    out_text("NYDUS transfers="); out_num(transfers);
    out_text(" ent="); out_num(ent); out_text(" ex="); out_num(ex);
    out_flush();
    out_text(probe_failed ? "NYDUS DONE FAIL" : "NYDUS DONE PASS"); out_flush();
    for (;;) frame_with(0);
}
