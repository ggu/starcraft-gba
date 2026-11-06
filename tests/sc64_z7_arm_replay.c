/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Input-only replay of the ordinary-input Z7 controller trace (replaces
 * common/main.c on ARM).
 *
 * The program boots normally and plays an embedded key tape generated from
 * a host controller trace by tools/sc64_z7_arm_replay.py: one entry per
 * held-key transition, starting at the title screen, through the shipped
 * game_update()/audio_update()/game_render() loop. The tape includes the
 * ordinary campaign-menu route to Zerg mission 7. Nothing writes game
 * state. At every host trace checkpoint tick it reports the same fields as
 * the controller's trace rows.
 *
 * Z7_ARM_MODE selects the run:
 *   0  full tape to the recorded terminal tick;
 *   1  prefix: at the first idle tick at or after Z7_ARM_SAVE_MIN_TICK,
 *      press START then B (the ordinary pause-menu save) and stop;
 *   2  resume: on a fresh boot, press L on the title screen to load the
 *      saved game, then continue the tape after Z7_ARM_CUT_TICK.
 *
 * On ARM, records go to the mGBA debug log with guest cycle counts from the
 * GBA timers. Built with GBA_HOST, the same program prints records to
 * stdout; the prefix writes its save payload to argv[1] and the resume reads
 * argv[1] into the empty host SRAM before booting. Emulator evidence only;
 * no physical-hardware claim.
 */
#if defined(Z7_PEAK_DETAILED_STAGES) && !defined(Z7_PEAK_DIAGNOSTIC)
#error "detailed peak stages require Z7_PEAK_DIAGNOSTIC"
#endif
#if defined(Z7_PEAK_DETAILED_STAGES) && !defined(SC64_DIAGNOSTIC_UPDATE)
#define SC64_DIAGNOSTIC_UPDATE
#endif
#if defined(GBA_HOST) && defined(Z7_CARGO_DIGEST)
#include "../common/gba.c"
#endif
#include "../starcraft/game.c"
#include "sc64_z7_tape_generated.h"
#ifdef GBA_HOST
#include <stdio.h>
#endif

#ifndef Z7_ARM_MODE
#define Z7_ARM_MODE 0
#endif
#if Z7_ARM_MODE == 1 && !defined(Z7_ARM_SAVE_MIN_TICK)
#error "the prefix replay needs Z7_ARM_SAVE_MIN_TICK"
#endif
#if Z7_ARM_MODE == 2 && !defined(Z7_ARM_CUT_TICK)
#error "the resume replay needs Z7_ARM_CUT_TICK"
#endif

volatile u16 demo_keys;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[240];
static unsigned line_len;
static u16 previous_held;
static u32 frames_run;
static uint64_t cycles_total, cycles_update;
static u32 cycles_peak, update_peak;
#ifdef Z7_PEAK_DIAGNOSTIC
/* Probe context only; never serialized or used to choose gameplay input.
 * 0=boot, 1=recorded tape, 2=HOME load, 3=pause-menu save. */
static u32 diagnostic_tick;
static unsigned diagnostic_phase;
#endif

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_u64(uint64_t v)
{
    char tmp[21]; int n = 0;
    do { tmp[n++] = (char)('0' + (int)(v % 10u)); v /= 10u; } while (v && n < 20);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_num(long v)
{
    if (v < 0) { out_text("-"); out_u64((uint64_t)(-(int64_t)v)); }
    else out_u64((uint64_t)v);
}
static void out_field(const char *name, long v) { out_text(" "); out_text(name); out_text("="); out_num(v); }
static void out_flush(void)
{
    line[line_len] = 0;
#ifdef GBA_HOST
    puts(line);
#else
    for (unsigned i = 0; i <= line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
#endif
    line_len = 0;
}

#ifdef SC64_DIAGNOSTIC_SAVE_VALIDATE
void sc64_diagnostic_save_validate(unsigned layer,int slot,u32 cycles,
                                    u32 tag,u32 length,int accepted)
{
    /* Layer 0 wraps the generic selector callback; layer 1 times the nested
     * preflight, including its final direct call on the reloaded winner.
     * Nested scopes must not be summed. No payload or live state is changed. */
    out_text("Z7S VALIDATE"); out_field("layer",(long)layer);
    out_field("slot",slot); out_text(" cycles="); out_u64(cycles);
    out_text(" tag="); out_u64(tag); out_text(" length="); out_u64(length);
    out_field("accepted",accepted); out_text(" end=1"); out_flush();
}
#endif

#ifndef GBA_HOST
/* Untouched-stack high-water mark: paint the unused IWRAM stack once. */
enum { STACK_LOW = 0x03005000u, STACK_LIMIT = 0x03007e00u, STACK_GAP = 256u };
#define STACK_PATTERN 0xa55a3cc3u
static u32 stack_paint_end;
static void stack_paint(void)
{
    u32 sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    stack_paint_end = sp > STACK_LOW + STACK_GAP ? sp - STACK_GAP : STACK_LOW;
    if (stack_paint_end > STACK_LIMIT) stack_paint_end = STACK_LIMIT;
    for (volatile u32 *p = (volatile u32 *)STACK_LOW; (u32)p < stack_paint_end; ++p) *p = STACK_PATTERN;
}
static u32 stack_untouched(void)
{
    u32 n = 0;
    for (volatile u32 *p = (volatile u32 *)STACK_LOW; (u32)p < stack_paint_end && *p == STACK_PATTERN; ++p) n += 4;
    return n;
}
#endif

#if defined(Z7_PEAK_DIAGNOSTIC) && !defined(GBA_HOST)
static void peak_field(const char *name, u32 value)
{
    out_text(" "); out_text(name); out_text("="); out_u64(value);
}
static void report_peak(u16 held, u32 before_frame, int before_screen,
                        u32 total, u32 update)
{
    int advancing = before_screen == SCREEN_PLAY && (u32)frame == before_frame + 1u;
    out_text("Z7D PEAK");
    out_field("call", (long)frames_run); out_field("tick", (long)diagnostic_tick);
    out_field("phase", (long)diagnostic_phase);
    out_field("before", (long)before_frame); out_field("frame", (long)frame);
    out_field("screen0", before_screen); out_field("screen", screen);
    out_field("held", held); peak_field("total", total);
    peak_field("update", update); out_field("advancing", advancing);
    out_text(" end=1"); out_flush();
    out_text("Z7D STAGES");
    out_field("call", (long)frames_run); out_field("tick", (long)diagnostic_tick);
#ifdef SC64_DIAGNOSTIC_UPDATE
    out_field("detailed", 1);
#else
    out_field("detailed", 0);
#endif
    peak_field("orders", advancing ? sc64_orders_cycles : 0);
    peak_field("sight", advancing ? sc64_sight_cycles : 0);
#ifdef SC64_DIAGNOSTIC_UPDATE
    peak_field("mission", sc64_update_mission_cycles);
    peak_field("production", sc64_update_production_cycles);
    peak_field("state", sc64_update_race_state_cycles);
    peak_field("defenses", sc64_update_defenses_cycles);
    peak_field("research", sc64_update_research_cycles);
    peak_field("remainder", sc64_update_remainder_cycles);
#else
    peak_field("mission", 0); peak_field("production", 0);
    peak_field("state", 0); peak_field("defenses", 0);
    peak_field("research", 0); peak_field("remainder", 0);
#endif
    out_text(" end=1"); out_flush();
}
#endif

#ifdef Z7_BUSY_DIAGNOSTIC
#include "sc64_z7_busy_probe.h"
#endif
#ifdef Z7_BUSY_ATTRIBUTION
#include "sc64_z7_busy_attr.h"
#endif

/* One frame of the shipped main-loop work, timed like common/main.c. */
static void frame_with(u16 held)
{
#ifdef GBA_HOST
    demo_keys = held;
    game_update(held, held & (u16)~previous_held);
    audio_update();
    game_render();
#else
#ifdef Z7_PEAK_DIAGNOSTIC
    u32 before_frame = (u32)frame;
    int before_screen = screen;
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_update_mission_cycles = sc64_update_production_cycles = 0;
    sc64_update_race_state_cycles = sc64_update_defenses_cycles = 0;
    sc64_update_research_cycles = sc64_update_remainder_cycles = 0;
#endif
#endif
#ifdef Z7_BUSY_DIAGNOSTIC
    if (busy_tick) busy_before();
#endif
#ifdef Z7_BUSY_ATTRIBUTION
    if (busy_tick) attr_before();
#endif
    u32 start = gba_clock();
    demo_keys = held;
    game_update(held, held & (u16)~previous_held);
    u32 update = gba_clock() - start;
    audio_update();
#ifdef Z7_BUSY_ATTRIBUTION
    u32 render_start = gba_clock();
#endif
    game_render();
    u32 finish = gba_clock();
    u32 total = finish - start;
#ifdef Z7_BUSY_DIAGNOSTIC
    if (busy_tick) busy_after(total); else busy_load_cycles=total;
#endif
#ifdef Z7_BUSY_ATTRIBUTION
    if (busy_tick) attr_after(total,update,render_start-start-update,finish-render_start);
#endif
    cycles_total += total;
    cycles_update += update;
#ifdef Z7_PEAK_DIAGNOSTIC
    if ((total > cycles_peak || update > update_peak) &&
        (diagnostic_phase == 2 || total >= 1000000u))
        report_peak(held, before_frame, before_screen, total, update);
#endif
    if (total > cycles_peak) cycles_peak = total;
    if (update > update_peak) update_peak = update;
#endif
    previous_held = held;
    ++frames_run;
}

#ifdef Z7_CARGO_DIGEST
static u32 z7_cargo_digest(void)
{
    u32 h=2166136261u;
    for(int i=0;i<MAX_UNITS;++i) {
        const Unit *q=&u[i];
        h=(h^(u32)q->alive)*16777619u;
        if(!q->alive) continue;
#define Z7_HASH(v) do { u32 v_=(u32)(v); for(int j=0;j<4;++j) { h=(h^(v_&255u))*16777619u; v_>>=8; } } while(0)
        Z7_HASH(i); Z7_HASH(q->owner); Z7_HASH(q->type); Z7_HASH(q->raw_type);
        Z7_HASH(q->x); Z7_HASH(q->y); Z7_HASH(q->order); Z7_HASH(q->carry);
        Z7_HASH(q->flags); Z7_HASH(q->work_timer); Z7_HASH(q->build);
#undef Z7_HASH
    }
    return h;
}
#endif

/* Same fields and meaning as the controller's source_checkpoint() rows. */
static void report(const char *kind, u32 tick)
{
    unsigned side_units = 0, side_buildings = 0, enemy_units = 0, enemy_buildings = 0, workers = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        enemy_units += u[i].raw_owner == 4;
        side_units += u[i].owner == SIDE;
        workers += u[i].owner == SIDE && u[i].raw_type == 41;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        enemy_buildings += b[i].raw_owner == 4;
        side_buildings += b[i].owner == SIDE;
    }
    /* mGBA's debug text window is 256 bytes, so one observation is three
     * lines joined by host_tick. */
    out_text("Z7 CHECK "); out_text(kind);
    out_field("host_tick", (long)tick);
    out_field("frame", (long)frame);
    out_field("screen", screen);
    out_field("won", won);
    out_field("lost", lost);
    out_field("outcome", (long)race_z7.runtime.model.outcome);
    out_field("started", (long)race_z7.runtime.model.started);
    out_field("switch10", (long)race_z7.runtime.switch10);
    out_field("ai_script_mask", (long)race_z7.runtime.ai_script_mask);
    out_field("ai_plan_ticks", (long)race_z7.runtime.ai_plan_ticks);
    out_field("ai_plan_phase", (long)race_z7.runtime.ai_plan_phase);
    out_field("fault", (long)race_z7.runtime.fault);
    out_flush();
    out_text("Z7 CHECK2");
    out_field("host_tick", (long)tick);
    out_text(" action_cursors=");
    for (unsigned i = 0; i < SC64_Z7_TRIGGER_COUNT; ++i) {
        if (i) out_text(",");
        out_num((long)race_z7.runtime.model.action_cursor[i]);
    }
    out_text(" action_ticks=");
    for (unsigned i = 0; i < SC64_Z7_TRIGGER_COUNT; ++i) {
        if (i) out_text(",");
        out_num((long)race_z7.runtime.action_ticks[i]);
    }
    out_flush();
    out_text("Z7 CHECK3");
    out_field("host_tick", (long)tick);
    out_field("side_units", (long)side_units);
    out_field("side_buildings", (long)side_buildings);
    out_field("enemy_units", (long)enemy_units);
    out_field("enemy_buildings", (long)enemy_buildings);
    out_field("workers", (long)workers);
    out_field("minerals", minerals);
    out_field("gas", gas);
    out_flush();
#ifdef Z7_CARGO_DIGEST
    out_text("CARGO7 "); out_text(kind); out_field("tick", (long)tick);
    out_text(" digest="); out_u64(z7_cargo_digest()); out_text(" end=1"); out_flush();
#endif
}

static void report_cycles(const char *kind, u32 tick)
{
#ifdef GBA_HOST
    /* The host does not measure guest cycles. Do not emit timing records. */
    (void)kind; (void)tick;
#else
    out_text("Z7 CYCLES "); out_text(kind);
    out_field("host_tick", (long)tick);
    out_field("frames", (long)frames_run);
    out_text(" total="); out_u64(cycles_total);
    out_text(" update="); out_u64(cycles_update);
    out_field("peak", (long)cycles_peak);
    out_field("update_peak", (long)update_peak);
    out_field("stack_painted", (long)(stack_paint_end - STACK_LOW));
    out_field("stack_untouched", (long)stack_untouched());
    out_flush();
#endif
}

static u32 tape_cursor, check_cursor;
static u16 tape_held;

/* Advance the tape to host tick `tick` and return the held keys. */
static u16 tape_keys(u32 tick)
{
    while (tape_cursor < Z7_TAPE_COUNT && (z7_tape[tape_cursor] >> 10) <= tick)
        tape_held = (u16)(z7_tape[tape_cursor++] & 1023u);
    return tape_held;
}
static u16 tape_peek(u32 tick)
{
    u32 c = tape_cursor; u16 held = tape_held;
    while (c < Z7_TAPE_COUNT && (z7_tape[c] >> 10) <= tick) held = (u16)(z7_tape[c++] & 1023u);
    return held;
}
static void report_due(u32 tick)
{
    while (check_cursor < Z7_CHECK_COUNT && z7_checks[check_cursor] <= tick) {
        if (z7_checks[check_cursor] == tick) report("checkpoint", tick);
        ++check_cursor;
    }
}

#if Z7_ARM_MODE == 1
/* Ordinary save point: nothing held now or next tick, normal field mode.
 * The save deliberately omits transient UI memory (the remembered command
 * row, tap and box gestures). A loaded game starts with those at their
 * defaults, so a blind key tape stays meaningful only from a tick where the
 * uninterrupted run also has them at their defaults. */
static int idle_for_save(u32 tick)
{
    return tape_held == 0 && tape_peek(tick + 1) == 0 && screen == SCREEN_PLAY &&
        mode == MODE_NORMAL && !menu_open && !menu_cursor && !help &&
        selected_building < 0 && !box_state && !box_suppress_a && unit_tap < 0 &&
#ifdef SC64_FIELD_CONTROLS
        !menu_last && !r_tap_state &&
#endif
        !won && !lost && race_z7.runtime.fault == 0;
}
#endif

#ifdef GBA_HOST
static int host_payload(const char *path, int write)
{
#ifdef Z7_CARGO_DIGEST
    FILE *f = fopen(path, write ? "wb" : "rb");
    if (!f) return 0;
    int ok = write ? fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save) :
                     fread(host_save, 1, sizeof(host_save), f) == sizeof(host_save);
    return fclose(f) == 0 && ok;
#else
    static RaceSave data;
    FILE *f = fopen(path, write ? "wb" : "rb");
    if (!f) return 0;
    int ok;
    if (write) ok = save_read(&data, sizeof(data), SAVE_VERSION) && fwrite(&data, 1, sizeof(data), f) == sizeof(data);
    else {
        ok = fread(&data, 1, sizeof(data), f) == sizeof(data);
        if (ok) save_write(&data, sizeof(data), SAVE_VERSION);
    }
    return fclose(f) == 0 && ok;
#endif
}
int main(int argc, char **argv)
{
#if Z7_ARM_MODE != 0
    if (argc < 2) { fputs("usage: replay SAVE_PAYLOAD\n", stderr); return 2; }
#endif
    (void)argc; (void)argv;
#else
int main(void)
{
    stack_paint();
#endif
    gba_init();
#ifndef GBA_HOST
    DBG_ENABLE = 0xC0DE;
#endif
#if defined(GBA_HOST) && Z7_ARM_MODE == 2
    if (!host_payload(argv[1], 0)) { fputs("cannot stage save payload\n", stderr); return 2; }
#endif
    game_init();
    out_text("Z7 START"); out_field("mode", Z7_ARM_MODE);
    out_field("tape", Z7_TAPE_COUNT); out_field("checks", Z7_CHECK_COUNT);
    out_field("stop", Z7_TAPE_STOP_TICK);
    out_flush();
    u32 tick = 1;
#if Z7_ARM_MODE == 2
    /* HOME's L loads the newest valid record; the tape resumes after the cut. */
#ifdef Z7_PEAK_DIAGNOSTIC
    diagnostic_phase = 2; diagnostic_tick = Z7_ARM_CUT_TICK;
#endif
    frame_with(KEY_L);
    out_text(screen == SCREEN_PLAY && race_z7_active() && !race_z7.runtime.fault ?
             "Z7 LOADED ok" : "Z7 LOADED fail");
    out_field("frame", (long)frame); out_field("screen", screen);
    out_field("started", (long)race_z7.runtime.model.started);
    out_field("minerals", minerals); out_field("gas", gas);
    out_flush();
    if (screen != SCREEN_PLAY) for (;;) {}
    tape_keys(Z7_ARM_CUT_TICK);
    while (check_cursor < Z7_CHECK_COUNT && z7_checks[check_cursor] <= Z7_ARM_CUT_TICK) ++check_cursor;
    tick = Z7_ARM_CUT_TICK + 1;
#endif
    for (; tick <= Z7_TAPE_STOP_TICK; ++tick) {
#ifdef Z7_PEAK_DIAGNOSTIC
        diagnostic_phase = 1; diagnostic_tick = tick;
#ifdef Z7_PEAK_FRAME_LIMIT
        if (frames_run >= Z7_PEAK_FRAME_LIMIT) {
            out_text("Z7D STOP"); out_field("calls", (long)frames_run);
            out_field("tick", (long)tick); out_text(" end=1"); out_flush();
            /* This diagnostic stop is deliberately not a qualifying DONE. */
#ifdef GBA_HOST
            return 0;
#else
            for (;;) {}
#endif
        }
#endif
#endif
#ifdef Z7_BUSY_DIAGNOSTIC
        busy_tick = tick;
#endif
        u16 held = tape_keys(tick);
        if (screen == SCREEN_RESULT || won || lost) {
            out_text("Z7 EARLY_END"); out_field("host_tick", (long)tick); out_flush();
            break;
        }
        frame_with(held);
        report_due(tick);
#ifdef Z7_BUSY_DIAGNOSTIC
        if (busy_calls == Z7_BUSY_CALLS) {
#ifdef Z7_BUSY_ATTRIBUTION
            attr_report();
#endif
            busy_report(); for (;;) {}
        }
#endif
        if (tick % 50000u == 0) report_cycles("periodic", tick);
#if Z7_ARM_MODE == 1
        if (tick >= Z7_ARM_SAVE_MIN_TICK && idle_for_save(tick)) {
            report("cut", tick);
            report_cycles("cut", tick);
#ifdef Z7_PEAK_DIAGNOSTIC
            diagnostic_phase = 3;
#endif
            frame_with(KEY_START); frame_with(0); frame_with(KEY_B); frame_with(0);
            out_text(screen == SCREEN_PAUSE && !race_z7.runtime.fault ? "Z7 SAVED ok" : "Z7 SAVED fail");
            out_field("cut", (long)tick); out_field("frame", (long)frame);
            out_field("payload_bytes", (long)sizeof(RaceSave));
            out_field("tag", (long)SAVE_VERSION);
            out_flush();
#ifdef GBA_HOST
            return host_payload(argv[1], 1) ? 0 : 1;
#else
            for (;;) {}
#endif
        }
#endif
    }
    if (tick > Z7_TAPE_STOP_TICK) tick = Z7_TAPE_STOP_TICK;
    report(won ? "won" : lost ? "lost" : "limit", tick);
    report_cycles("terminal", tick);
    out_text("Z7 DONE"); out_field("cursor", (long)tape_cursor); out_flush();
#ifdef GBA_HOST
#if defined(Z7_CARGO_DIGEST) && Z7_ARM_MODE == 2
    if (argc > 2 && !host_payload(argv[2], 1)) return 2;
#endif
    return won ? 0 : 1;
#else
    for (;;) frame_with(0);
#endif
}
