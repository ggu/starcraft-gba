/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe for imported unit facing and walk poses (replaces common/main.c).
 *
 * Starts a race skirmish, adds a player Zergling, and walks it east and then
 * north-west with the shipped game_update()/game_render(). Every frame it
 * reports the unit's facing byte, which walk pose it should show, how many of
 * that pose's pixels are on the shown page, and dumps the 40x40 area around
 * the unit so the frames can be inspected. Then an idle Mutalisk must keep
 * beating its wings in place. Results go to the mGBA debug log;
 * no debugger reads or writes game state. Emulator evidence only.
 */
#include "../starcraft/game.c"
#if !defined(SC64_UNIT_FACING) || !defined(SC64_UNIT_WALK_FRAMES)
#error "The walk probe needs the facing and walk banks"
#endif

volatile u16 demo_keys;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)
#define CROP 40

static char line[240];
static unsigned line_len;
static int probe_failed;

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[12]; int n = 0; unsigned long m;
    if (v < 0) { out_text("-"); m = (unsigned long)(-v); } else m = (unsigned long)v;
    do { tmp[n++] = (char)('0' + m % 10u); m /= 10u; } while (m && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_hex(u32 v, int digits)
{
    static const char hex[] = "0123456789abcdef";
    char c[2] = { 0, 0 };
    for (int s = (digits - 1) * 4; s >= 0; s -= 4) { c[0] = hex[v >> s & 15u]; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
    line_len = 0;
}
static void check(int ok, const char *what)
{
    out_text(ok ? "WALK PASS " : "WALK FAIL "); out_text(what); out_flush();
    if (!ok) probe_failed = 1;
}

static const volatile u8 *shown(void)
{
    return (const volatile u8 *)((*(volatile u16 *)0x04000000 & 0x10) ? 0x0600A000 : 0x06000000);
}

/* Dump the shown area centred on screen point (x, y). */
static void dump_crop(int x, int y)
{
    const volatile u8 *p = shown();
    for (int row = 0; row < CROP; ++row) {
        out_text("WALK ROW "); out_num(row); out_text(" ");
        for (int col = 0; col < CROP; ++col) {
            int sx = x - CROP / 2 + col, sy = y - CROP / 2 + row;
            out_hex(sx >= 0 && sx < 240 && sy >= 0 && sy < 160 ? p[sy * 240 + sx] : 0, 2);
        }
        out_flush();
    }
}

/* Count the opaque pixels of one directional frame drawn at (x, y) that the
 * shown page holds. Facing banks store 17 headings and mirror the rest;
 * walk poses store all 32. */
static int frame_on_screen(const Sc64SpriteFrame *frames, int headings, int dir,
                           int x, int y, int *total)
{
    const volatile u8 *p = shown();
    int mirror = headings == 17 && dir > 16;
    const Sc64SpriteFrame *s = &frames[mirror ? 32 - dir : dir];
    int left = mirror ? x + s->canvas_w / 2 - s->offset_x - s->w
                      : x - s->canvas_w / 2 + s->offset_x;
    int top = y - s->canvas_h / 2 + s->offset_y, hits = 0;
    *total = 0;
    for (int row = 0; row < s->h; ++row)
        for (int col = 0; col < s->w; ++col) {
            u8 v = s->pixels[row * s->w + (mirror ? s->w - 1 - col : col)];
            int sx = left + col, sy = top + row;
            if (!v || sx < 0 || sx >= 240 || sy < 12 || sy >= 120) continue;
            ++*total;
            hits += p[sy * 240 + sx] == v;
        }
    return hits;
}

static void dump_palette(void)
{
    const volatile u16 *pal = (const volatile u16 *)0x05000000;
    for (int i = 0; i < 256; i += 32) {
        out_text("WALK PAL "); out_num(i); out_text(" ");
        for (int k = 0; k < 32; ++k) out_hex(pal[i + k], 4);
        out_flush();
    }
}

static void frame_tick(void)
{
    demo_keys = 0;
    game_update(0, 0);
    audio_update();
    game_render();
}

/* Walk to (tx, ty) for up to `ticks` frames with the camera on the unit.
 * Returns how many distinct walk poses were seen fully on screen; *last_dir
 * receives the final facing. */
static int leg(int slot, const char *name, int tx, int ty, int ticks, int *last_dir)
{
    const Sc64SpriteFrame *seen[SC64_UNIT_WALK_MAX_STEPS];
    int distinct = 0, moved = 0, facing_ok = 1, drawn_ok = 1;
    unsigned raw = (unsigned)sc64_race_raw_unit(u[slot].type);
    u[slot].selected = 1;
    issue_order(ORDER_MOVE, tx, ty);
    u[slot].selected = 0;
    for (int t = 0; t < ticks; ++t) {
        int x = u[slot].x, y = u[slot].y;
        cx = x + 64; cy = y + 32; /* the camera follows the cursor, kept off the unit */
        frame_tick();
        int sx = u[slot].x - camx, sy = u[slot].y - camy + 12;
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
        int moving = u[slot].x != x || u[slot].y != y;
        const Sc64SpriteFrame *pose = moving ? sc64_walk_pose(raw, slot) : 0;
        int total = 0, hits = pose ? frame_on_screen(pose, 32, dir, sx, sy, &total) : 0;
        int step = -1;
        const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
        for (unsigned k = 0; pose && k < cycle->count; ++k) if (cycle->steps[k].frames == pose) step = (int)k;
        out_text("WALK FRAME leg="); out_text(name); out_text(" t="); out_num(t);
        out_text(" x="); out_num(u[slot].x); out_text(" y="); out_num(u[slot].y);
        out_text(" moving="); out_num(moving); out_text(" facing="); out_num(dir);
        out_text(" waypoint="); out_num(u[slot].path_x); out_text(","); out_num(u[slot].path_y);
        out_text(" step="); out_num(step); out_text(" pixels="); out_num(hits);
        out_text("/"); out_num(total); out_flush();
        dump_crop(sx, sy);
        if (!moving) continue;
        ++moved;
        if (FAST_ABS(u[slot].x - tx) + FAST_ABS(u[slot].y - ty) <= 8) continue;
        /* Ground units face the path waypoint they are walking to. */
        int wx = u[slot].path_x - u[slot].x, wy = u[slot].path_y - u[slot].y;
        if (FAST_ABS(wx) >= SC64_FACING_STEP || FAST_ABS(wy) >= SC64_FACING_STEP)
            facing_ok &= dir == sc64_facing_direction(wx, wy);
        if (!pose || !total || hits != total) { drawn_ok = 0; continue; }
        int known = 0;
        for (int i = 0; i < distinct; ++i) known |= seen[i] == pose;
        if (!known && distinct < (int)SC64_UNIT_WALK_MAX_STEPS) seen[distinct++] = pose;
    }
    out_text("WALK LEG "); out_text(name); out_text(" moved="); out_num(moved);
    out_text(" poses="); out_num(distinct); out_text(" facing_ok="); out_num(facing_ok);
    out_text(" drawn_ok="); out_num(drawn_ok); out_flush();
    *last_dir = sc64_facing[slot] & SC64_FACING_MASK;
    check(moved > 0 && facing_ok && drawn_ok, name);
    return distinct;
}

/* After arriving, the unit shows its facing frame and holds still. */
static void stand(int slot)
{
    int ok = 1;
    for (int t = 0; t < 60 && u[slot].order != ORDER_IDLE; ++t) {
        cx = u[slot].x + 64; cy = u[slot].y + 32;
        frame_tick();
    }
    /* The tracker bridges one drawn tick after the last step. */
    for (int t = 0; t < 2; ++t) frame_tick();
    for (int t = 0; t < 6; ++t) {
        cx = u[slot].x + 64; cy = u[slot].y + 32;
        frame_tick();
        int sx = u[slot].x - camx, sy = u[slot].y - camy + 12, total;
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
        int hits = frame_on_screen(sc64_unit_facing_frames((unsigned short)sc64_race_raw_unit(u[slot].type)),
                                   17, dir, sx, sy, &total);
        out_text("WALK FRAME leg=stand t="); out_num(t); out_text(" facing="); out_num(dir);
        out_text(" order="); out_num(u[slot].order); out_text(" pixels="); out_num(hits);
        out_text("/"); out_num(total); out_flush();
        dump_crop(sx, sy);
        ok &= u[slot].order == ORDER_IDLE && total && hits == total;
    }
    check(ok, "standing unit shows its facing frame");
}

/* An idle flyer with a source cycle keeps playing it while it hovers. */
static void hover(int slot)
{
    unsigned raw = (unsigned)sc64_race_raw_unit(u[slot].type);
    const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
    const Sc64SpriteFrame *seen[SC64_UNIT_WALK_MAX_STEPS];
    int distinct = 0, drawn_ok = 1, x = u[slot].x, y = u[slot].y;
    check(cycle != 0, "the Mutalisk has a source wing cycle");
    if (!cycle) return;
    for (int t = 0; t < (int)cycle->cycle_ticks * 5 / 4 + 8; ++t) {
        cx = u[slot].x + 64; cy = u[slot].y + 32;
        frame_tick();
        int sx = u[slot].x - camx, sy = u[slot].y - camy + 12, total;
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
        const Sc64SpriteFrame *pose = sc64_walk_pose(raw, slot);
        int hits = frame_on_screen(pose, 32, dir, sx, sy, &total), step = -1;
        for (unsigned k = 0; k < cycle->count; ++k) if (cycle->steps[k].frames == pose) step = (int)k;
        out_text("WALK FRAME leg=hover t="); out_num(t); out_text(" facing="); out_num(dir);
        out_text(" order="); out_num(u[slot].order); out_text(" step="); out_num(step);
        out_text(" pixels="); out_num(hits); out_text("/"); out_num(total); out_flush();
        dump_crop(sx, sy);
        if (!total || hits != total) { drawn_ok = 0; continue; }
        int known = 0;
        for (int i = 0; i < distinct; ++i) known |= seen[i] == pose;
        if (!known && distinct < (int)SC64_UNIT_WALK_MAX_STEPS) seen[distinct++] = pose;
    }
    out_text("WALK HOVER poses="); out_num(distinct); out_text("/"); out_num(cycle->count);
    out_text(" drawn_ok="); out_num(drawn_ok); out_flush();
    check(drawn_ok && distinct == (int)cycle->count && u[slot].order == ORDER_IDLE &&
          u[slot].x == x && u[slot].y == y, "an idle Mutalisk shows every wing pose in place");
}

int main(void)
{
    gba_init();
    DBG_ENABLE = 0xC0DE;
    game_init();
    out_text("WALK START debug="); out_num(DBG_ENABLE == 0x1DEA); out_flush();
    race_mode = 1; race_human = R_TER; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    for (int i = 0; i < 4; ++i) frame_tick();
    int home = -1;
    for (int i = 0; i < MAX_UNITS && home < 0; ++i)
        if (u[i].alive && u[i].owner == SIDE) home = i;
    check(home >= 0, "skirmish has a player unit");
    int slot = spawn_unit(SIDE, U_ZERGLING, u[home].x + 24, u[home].y + 24);
    check(slot >= 0, "spawned a Zergling");
    if (slot >= 0) {
        int raw = sc64_race_raw_unit(U_ZERGLING);
        const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
        out_text("WALK UNIT raw="); out_num(raw); out_text(" poses=");
        out_num(cycle ? cycle->count : 0); out_text(" cycle_ticks=");
        out_num(cycle ? cycle->cycle_ticks : 0); out_flush();
        for (int i = 0; i < 3; ++i) frame_tick();
        dump_palette();
        int east_dir, north_west_dir;
        int east = leg(slot, "east", u[slot].x + 120, u[slot].y, 30, &east_dir);
        int nw_x = u[slot].x - 80, nw_y = u[slot].y - 80;
        int north_west = leg(slot, "north-west", nw_x, nw_y, 40, &north_west_dir);
        check(east_dir == 8, "the east leg ends facing east");
        check(north_west_dir == sc64_facing_direction(nw_x - u[slot].x, nw_y - u[slot].y),
              "the north-west leg ends facing its target");
        check(cycle && east == cycle->count && north_west == cycle->count,
              "each leg draws every walk pose");
        stand(slot);
    }
    /* Beside the Zergling's open ground, clear of the base. */
    int flyer = slot >= 0 ? spawn_unit(SIDE, U_MUTALISK, u[slot].x + 40, u[slot].y) : -1;
    check(flyer >= 0, "spawned a Mutalisk");
    if (flyer >= 0) hover(flyer);
    out_text(probe_failed ? "WALK DONE fail" : "WALK DONE ok"); out_flush();
    for (;;) frame_tick();
}
