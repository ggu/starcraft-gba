/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * No-GDB ARM replay of the T9 controller trace (replaces common/main.c).
 *
 * The ROM boots normally, taps through the ordinary Terran campaign menu to
 * mission 9 (New Gettysburg) and deploys from its briefing. It then plays an
 * embedded key tape generated from a host controller trace by
 * tools/sc64_t9_arm_replay.py, one entry per held-key transition, through the
 * shipped game_update()/audio_update()/game_render() loop. Nothing writes
 * game state; observations go to the mGBA debug log every 1000 mission ticks
 * and at the end, with guest cycle counts from the GBA timers. Emulator
 * evidence only; no physical-hardware claim.
 */
#include "../starcraft/game.c"
#include "sc64_t9_tape_generated.h"

volatile u16 demo_keys;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[240];
static unsigned line_len;
static u16 previous_held;
static u32 frames_run;
static uint64_t cycles_total, cycles_update;
static u32 cycles_peak;
static u32 cycle_buckets[8];

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[12]; int n = 0; unsigned long m;
    if (v < 0) { out_text("-"); m = (unsigned long)(-v); } else m = (unsigned long)v;
    do { tmp[n++] = (char)('0' + m % 10u); m /= 10u; } while (m && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_u64(uint64_t v)
{
    char tmp[21]; int n = 0;
    do { tmp[n++] = (char)('0' + (int)(v % 10u)); v /= 10u; } while (v && n < 20);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_field(const char *name, long v) { out_text(" "); out_text(name); out_text("="); out_num(v); }
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
    line_len = 0;
}

/* One frame of the shipped main-loop work, timed like common/main.c. */
static void frame_with(u16 held)
{
    u32 start = gba_clock();
    demo_keys = held;
    game_update(held, held & (u16)~previous_held);
    u32 update = gba_clock() - start;
    audio_update();
    game_render();
    u32 total = gba_clock() - start;
    previous_held = held;
    ++frames_run;
    cycles_total += total;
    cycles_update += update;
    if (total > cycles_peak) cycles_peak = total;
    u32 bucket = (total + 140448u) / 280896u;
    cycle_buckets[bucket < 7 ? bucket : 7]++;
}
static void press(u16 key) { frame_with(key); frame_with(0); }

static int count_force_buildings(int raw_owner)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_owner == raw_owner) ++n;
    return n;
}

/* Same fields and meaning as the host controller's trace observations. */
static void report(const char *kind, u32 tick)
{
    int raw16 = 0, side_units = 0, zerg_units = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == 16) ++raw16;
        if (u[i].owner == SIDE) ++side_units;
        if (u[i].owner == 4) ++zerg_units;
    }
    /* mGBA's debug text window is 256 bytes, so one observation is two
     * lines joined by host_tick. */
    out_text("T9 CHECK "); out_text(kind);
    out_field("host_tick", (long)tick);
    out_field("frame", (long)frame);
    out_field("screen", screen);
    out_field("won", won);
    out_field("lost", lost);
    out_field("mission", race_selected_mission);
    out_field("faults", mission_unsupported_count);
    out_field("phase", (long)race_campaign.phase);
    out_field("objective_flags", (long)race_campaign.objective_flags);
    out_field("race_completed", (long)race_completed);
    out_flush();
    out_text("T9 CHECK2");
    out_field("host_tick", (long)tick);
    out_field("protoss_force2_buildings", count_force_buildings(1));
    out_field("zerg_force3_buildings", count_force_buildings(3));
    out_field("raw16_alive", raw16);
    out_field("side_units", side_units);
    out_field("zerg_units", zerg_units);
    out_field("minerals", minerals);
    out_field("gas", gas);
    out_field("selection", selected_count());
    out_field("selected_building", selected_building);
    out_flush();
}

static void report_cycles(void)
{
    out_text("T9 CYCLES frames="); out_num((long)frames_run);
    out_text(" total="); out_u64(cycles_total);
    out_text(" update="); out_u64(cycles_update);
    out_text(" peak="); out_num((long)cycles_peak);
    out_text(" buckets=");
    for (int i = 0; i < 8; ++i) { if (i) out_text(","); out_num((long)cycle_buckets[i]); }
    out_flush();
}

int main(void)
{
    gba_init();
    DBG_ENABLE = 0xC0DE;
    game_init();
    out_text("T9 START debug="); out_num(DBG_ENABLE == 0x1DEA);
    out_field("tape", T9_TAPE_COUNT); out_field("stop", T9_TAPE_STOP_TICK);
    out_flush();
    /* Ordinary campaign route: chapter 9 is the first row of the fifth page. */
    static const u16 route[] = { KEY_A, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_RIGHT, KEY_A, KEY_START };
    for (unsigned i = 0; i < sizeof(route) / sizeof(route[0]); ++i) {
        press(route[i]);
        out_text("T9 MENU key="); out_num(route[i]);
        out_field("screen", screen); out_field("page", race_front.page);
        out_field("row", race_front.row); out_field("mission", race_front.mission);
        out_flush();
    }
    int launched = screen == SCREEN_PLAY && !race_mode &&
        race_selected_mission == 12 && !won && !lost;
    out_text(launched ? "T9 LAUNCH ok" : "T9 LAUNCH fail");
    out_field("frames", (long)frames_run); out_field("frame", (long)frame);
    out_field("screen", screen); out_field("mission", race_selected_mission);
    out_field("race_mode", race_mode);
    out_flush();
    /* The briefing's START frame ran mission tick 1; the host controller
     * leaves that tick neutral, so the tape continues from tick 2. */
    report("launch", 1);
    u32 cursor = 0;
    u16 held = 0;
    while (cursor < T9_TAPE_COUNT && (t9_tape[cursor] >> 10) <= 1u) {
        if ((t9_tape[cursor] & 1023u) != 0) launched = 0;
        held = (u16)(t9_tape[cursor++] & 1023u);
    }
    if (!launched) { out_text("T9 LAUNCH tape tick 1 is not neutral"); out_flush(); }
    for (u32 tick = 2; launched && tick <= T9_TAPE_STOP_TICK; ++tick) {
        while (cursor < T9_TAPE_COUNT && (t9_tape[cursor] >> 10) <= tick)
            held = (u16)(t9_tape[cursor++] & 1023u);
        if (screen != SCREEN_PLAY || won || lost) {
            out_text("T9 EARLY_END"); out_field("host_tick", (long)tick); out_flush();
            break;
        }
        frame_with(held);
        if (tick % 1000u == 0) report("periodic", tick);
        if (tick == T9_TAPE_STOP_TICK) report(won ? "won" : lost ? "lost" : "limit", tick);
    }
    report_cycles();
    out_text("T9 DONE"); out_field("cursor", (long)cursor); out_flush();
    for (;;) frame_with(0);
}
