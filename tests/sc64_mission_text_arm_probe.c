/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe of the modal mission text box (replaces common/main.c).
 *
 * For each mission in PROBE_MISSIONS, it starts the mission and waits with no
 * input until the first row posts. Then it checks through the shipped
 * game_update()/game_render() that the world stays frozen, that only A advances,
 * and that the world resumes after the last row. It reports over the mGBA debug
 * log and dumps the first frame of each box as hex for
 * tools/sc64_mission_text_arm_probe.py. No debugger reads or writes game state.
 * The missions are entered directly, so this is presentation evidence, not
 * evidence that a mission can be reached or won.
 */
#include "../starcraft/game.c"

/* The fullest candidates have 8 bytes of working RAM to spare, so the probe's
 * own storage lives beside the box's in IWRAM. */
volatile u16 demo_keys SC64_MISSION_TEXT_STATE;

#ifndef PROBE_MISSIONS
#error "Define PROBE_MISSIONS as a comma list of global mission IDs"
#endif
#ifndef PROBE_IDLE_FRAMES
#define PROBE_IDLE_FRAMES 30000
#endif
#ifndef PROBE_BOXES
#define PROBE_BOXES 2
#endif

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[240] SC64_MISSION_TEXT_STATE;
static unsigned line_len SC64_MISSION_TEXT_STATE;
static u16 probe_previous SC64_MISSION_TEXT_STATE;
static int probe_failed SC64_MISSION_TEXT_STATE;

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
    DBG_FLAGS = 0x100 | 2;
    line_len = 0;
}
static void out_hex_bytes(const volatile u8 *p, unsigned count)
{
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < count; ++i) {
        char c[3] = { digits[p[i] >> 4], digits[p[i] & 15u], 0 };
        out_text(c);
    }
}

static void check(int ok, int mission, const char *what)
{
    out_text(ok ? "MT PASS " : "MT FAIL ");
    out_text(what); out_text(" mission="); out_num(mission);
    out_text(" frame="); out_num((long)frame);
    out_flush();
    if (!ok) probe_failed = 1;
}

static void frame_with(u16 held, int render)
{
    demo_keys = held;
    game_update(held, held & (u16)~probe_previous);
    if (render) {
        audio_update();
        game_render();
    }
    probe_previous = held;
}

/* The world a box must not disturb. */
static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) (h = (h ^ (u32)(v)) * 16777619u)
    MIX(frame); MIX(minerals); MIX(gas); MIX(cx); MIX(cy); MIX(won); MIX(lost);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) { MIX(i); MIX(u[i].type); MIX(u[i].owner); MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) { MIX(i); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].hp); }
#undef MIX
    return h;
}

/* Mode 4: the page on screen follows DISPCNT bit 4. */
static void dump_frame(int mission, int box)
{
    const volatile u8 *page = (const volatile u8 *)
        ((*(volatile u16 *)0x04000000 & 0x10) ? 0x0600A000 : 0x06000000);
    const volatile u8 *palette = (const volatile u8 *)0x05000000;
    out_text("MT IMAGE "); out_num(mission); out_text(" "); out_num(box); out_flush();
    for (unsigned i = 0; i < 512u; i += 100u) {
        out_text("MTP "); out_hex_bytes(palette + i, 512u - i < 100u ? 512u - i : 100u); out_flush();
    }
    for (unsigned i = 0; i < 38400u; i += 100u) {
        out_text("MTI "); out_hex_bytes(page + i, 100u); out_flush();
    }
    out_text("MT IMAGE END"); out_flush();
}

static void report_row(int mission)
{
    const Sc64MissionTextRow *row = &sc64_mission_text_rows[sc64_mission_text_row(&race_mission_text)];
    out_text("MT BOX mission="); out_num(mission);
    out_text(" frame="); out_num((long)frame);
    out_text(" trigger="); out_num(row->trigger); out_text(" action="); out_num(row->action);
    out_text(" opcode="); out_num(row->opcode);
    out_text(" speaker="); out_text(sc64_mission_text_speakers[row->speaker].name);
    out_flush();
}

/* Every page of the box on screen, through ordinary A presses. */
static void read_box(int mission, int box)
{
    u32 digest = world_digest();
    int pages = 0;
    report_row(mission);
    for (int i = 0; i < 30; ++i) frame_with(0, 1);
    dump_frame(mission, box);
    /* Field input must not reach the frozen world. */
    frame_with(KEY_START, 1); frame_with(0, 1);
    frame_with(KEY_B, 1); frame_with(0, 1);
    frame_with(KEY_LEFT, 1); frame_with(0, 1);
    check(screen == SCREEN_PLAY && !menu_open && sc64_mission_text_showing(&race_mission_text),
          mission, "field input stays out");
    check(world_digest() == digest, mission, "world frozen under the box");
    int row = sc64_mission_text_row(&race_mission_text);
    while (sc64_mission_text_row(&race_mission_text) == row && pages < 8) {
        for (int i = 0; i <= SC64_MISSION_TEXT_SETTLE_FRAMES; ++i) frame_with(0, 1);
        frame_with(KEY_A, 1);
        frame_with(0, 1);
        ++pages;
    }
    check(pages < 8, mission, "A advances the row");
    check(world_digest() == digest, mission, "world frozen until the queue is read");
}

static void probe_mission(int mission)
{
    int boxes = 0;
    race_mode = 0;
    race_selected_mission = mission;
    race_human = sc64_race_maps[mission].owner_races[sc64_race_maps[mission].human_slot];
    race_bots = 0;
    sc64_display_palette(sc64_palette + 32);
    init_state();
    probe_previous = 0;
    sc64_mission_text_interactive = 1;
    out_text("MT MISSION "); out_num(mission); out_text(" rows=");
    out_num(sc64_mission_text_first[mission + 1] - sc64_mission_text_first[mission]);
    out_flush();
    for (long idle = 0; idle < PROBE_IDLE_FRAMES && boxes < PROBE_BOXES && screen == SCREEN_PLAY; ++idle) {
        if (sc64_mission_text_showing(&race_mission_text)) {
            read_box(mission, boxes++);
            /* A row chained behind this one stays on the same frozen tick. */
            while (sc64_mission_text_showing(&race_mission_text) && boxes < PROBE_BOXES)
                read_box(mission, boxes++);
            while (sc64_mission_text_showing(&race_mission_text)) {
                for (int i = 0; i <= SC64_MISSION_TEXT_SETTLE_FRAMES; ++i) frame_with(0, 0);
                frame_with(KEY_A, 0);
                frame_with(0, 0);
            }
            u32 before = frame;
            frame_with(0, 0); /* The frame the box still holds, on release. */
            for (int i = 0; i < 4 && frame == before; ++i) frame_with(0, 0);
            check(frame > before || screen == SCREEN_RESULT, mission, "world resumes after A");
            continue;
        }
        frame_with(0, 0);
    }
    out_text("MT SUMMARY mission="); out_num(mission);
    out_text(" boxes="); out_num(boxes);
    out_text(" posted="); out_num(race_mission_text.posted);
    out_text(" dropped="); out_num(race_mission_text.dropped);
    out_text(" frame="); out_num((long)frame);
    out_text(" won="); out_num(won); out_text(" lost="); out_num(lost);
    out_text(" screen="); out_num(screen);
    out_flush();
}

int main(void)
{
    static const short missions[] = { PROBE_MISSIONS };
    DBG_ENABLE = 0xC0DE;
    gba_init();
    game_init();
    for (unsigned i = 0; i < sizeof(missions) / sizeof(missions[0]); ++i)
        probe_mission(missions[i]);
    out_text(probe_failed ? "MT DONE fail" : "MT DONE ok");
    out_flush();
    for (;;) {}
}
