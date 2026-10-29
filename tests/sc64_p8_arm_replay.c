/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Input-only P8 replay probe (replaces common/main.c on ARM).
 *
 * The program boots normally and plays an embedded key tape generated from a
 * host P8 controller trace by tools/sc64_p8_arm_replay.py, one entry per
 * held-key transition, through the shipped game_update()/audio_update()/
 * game_render() loop. Tape tick N is the controller's input call N, from the
 * title screen onward, so the ordinary campaign menu, the briefing and the
 * pause-menu save are all part of the tape. Nothing writes game state.
 *
 * P8_REPLAY_PREFIX stops after the tape's pause-menu save sequence, leaving
 * that save in SRAM. P8_REPLAY_RESUME boots a fresh process, presses L on
 * the title screen to load it, and continues the tape from the call after the
 * save sequence. Observations go to stdout on the host and to the mGBA debug
 * log on ARM, every 1000 tape ticks and at the end; the same source builds
 * both, so their lines can be compared directly. Emulator evidence only; no
 * physical-hardware claim.
 */
#ifdef GBA_HOST
#include "../common/gba.c"
#endif
#include "../starcraft/game.c"
#include "sc64_p8_tape_generated.h"
#ifdef GBA_HOST
#include <stdio.h>
#include <stdlib.h>
#endif

volatile u16 demo_keys;

static char line[240];
static unsigned line_len;
static u16 previous_held;
static u32 frames_run;
static uint64_t cycles_total, cycles_update;
static u32 cycles_peak;

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_unsigned(unsigned long v)
{
    char tmp[21]; int n = 0;
    do { tmp[n++] = (char)('0' + (int)(v % 10u)); v /= 10u; } while (v && n < 20);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_num(long v)
{
    if (v < 0) { out_text("-"); out_unsigned((unsigned long)(-v)); }
    else out_unsigned((unsigned long)v);
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
    line[line_len] = 0;
#ifdef GBA_HOST
    puts(line); fflush(stdout);
#else
    for (unsigned i = 0; i <= line_len; ++i) ((volatile char *)0x04FFF600)[i] = line[i];
    *(volatile u16 *)0x04FFF700 = 0x100 | 2; /* warn: visible with log mask 7 */
#endif
    line_len = 0;
}

static u32 hash_u32(u32 h, u32 v)
{
    for (int i = 0; i < 4; ++i) { h ^= (u8)(v >> (8 * i)); h *= 16777619u; }
    return h;
}
#define H(v) (h = hash_u32(h, (u32)(v)))
/* Gameplay state only: actors, resources, mission/runtime state. */
static u32 state_digest(void)
{
    u32 h = 2166136261u;
    for (int i = 0; i < MAX_UNITS; i++) {
        const Unit *q = &u[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->x); H(q->y); H(q->tx); H(q->ty);
        H(q->target); H(q->raw_type); H(q->raw_owner); H(q->cd);
        H(q->owner); H(q->type); H(q->order); H(q->carry);
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->build_progress); H(q->x); H(q->y);
        H(q->raw_type); H(q->raw_owner); H(q->queue_progress);
        H(q->owner); H(q->type); H(q->queue_count); H(q->under_construction);
    }
    for (int i = 0; i < MAX_MINERALS; i++) H(m[i].amount);
    H(minerals); H(gas); H(frame); H(race_campaign.elapsed_frames);
    H(race_campaign.phase);
    const u8 *p8 = (const u8 *)&race_p8;
    for (unsigned i = 0; i < sizeof(race_p8); ++i) H(p8[i]);
    return h;
}
#undef H

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
}

static void report(const char *kind, u32 tick)
{
    int side_units = 0, enemy_units = 0, enemy_buildings = 0;
    int stasis = -1, fenix = -1, hyperion = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == SIDE) ++side_units;
        if (u[i].owner == ENEMY) ++enemy_units;
        if (u[i].raw_type == 78) fenix = u[i].hp;
        if (u[i].raw_type == 28) hyperion = u[i].hp;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == ENEMY) ++enemy_buildings;
        if (b[i].raw_type == 168) stasis = b[i].hp;
    }
    out_text("P8 CHECK "); out_text(kind);
    out_field("tick", (long)tick);
    out_field("frame", (long)frame);
    out_field("screen", screen);
    out_field("won", won);
    out_field("lost", lost);
    out_field("mission", race_selected_mission);
    out_field("faults", mission_unsupported_count);
    out_field("side_units", side_units);
    out_field("enemy_units", enemy_units);
    out_field("enemy_buildings", enemy_buildings);
    out_field("stasis_hp", stasis);
    out_field("fenix_hp", fenix);
    out_field("hyperion_hp", hyperion);
    out_field("minerals", minerals);
    out_field("gas", gas);
    out_field("done", (long)race_p8.runtime[0].done);
    out_text(" digest="); out_unsigned(state_digest());
    out_flush();
}

static void report_cycles(void)
{
    out_text("P8 CYCLES frames="); out_num((long)frames_run);
    out_text(" total="); out_u64(cycles_total);
    out_text(" update="); out_u64(cycles_update);
    out_text(" peak="); out_num((long)cycles_peak);
    out_flush();
}

#ifdef GBA_HOST
/* The host stands in for the cartridge: load/store the SRAM image. */
static void host_sram_io(const char *path, int write)
{
    FILE *f;
    if (!path) return;
    f = fopen(path, write ? "wb" : "rb");
    if (!f) { if (write) perror(path); return; }
    if (write) fwrite(host_save, 1, sizeof(host_save), f);
    else if (fread(host_save, 1, sizeof(host_save), f) != sizeof(host_save))
        fprintf(stderr, "short SRAM image %s\n", path);
    fclose(f);
}
#endif

int main(int argc, char **argv)
{
#ifdef GBA_HOST
    const char *sram_path = argc > 1 ? argv[1] : 0;
#else
    (void)argc; (void)argv;
    *(volatile u16 *)0x04FFF780 = 0xC0DE;
#endif
    gba_init();
#if defined(GBA_HOST) && defined(P8_REPLAY_RESUME)
    host_sram_io(sram_path, 0);
#endif
    game_init();
    u32 first = 0, stop = P8_TAPE_STOP_TICK, cursor = 0;
    u16 held = 0;
#ifdef P8_REPLAY_PREFIX
    stop = P8_TAPE_RESUME_TICK;
#endif
#ifdef P8_REPLAY_RESUME
    /* Fresh process: quick-load from the title screen, then continue. */
    frame_with(KEY_L);
    frame_with(0);
    int loaded = screen == SCREEN_PLAY && race_selected_mission == 27;
    out_text(loaded ? "P8 RESUME ok" : "P8 RESUME fail");
    out_field("frame", (long)frame); out_field("screen", screen);
    out_flush();
    if (!loaded) { out_text("P8 DONE"); out_flush(); for (;;) frame_with(0); }
    first = P8_TAPE_RESUME_TICK;
    report("resume", first);
#endif
    out_text("P8 START tape="); out_num(P8_TAPE_COUNT);
    out_field("first", (long)first); out_field("stop", (long)stop);
    out_flush();
    for (u32 tick = first; tick < stop; ++tick) {
        while (cursor < P8_TAPE_COUNT && (p8_tape[cursor] >> 10) <= tick)
            held = (u16)(p8_tape[cursor++] & 1023u);
        frame_with(held);
        if ((tick + 1u) % 1000u == 0) report("periodic", tick + 1u);
    }
    report(won ? "won" : lost ? "lost" : "limit", stop);
    report_cycles();
#ifdef GBA_HOST
#ifdef P8_REPLAY_PREFIX
    host_sram_io(sram_path, 1);
#endif
#endif
    out_text("P8 DONE"); out_field("cursor", (long)cursor); out_flush();
#ifdef GBA_HOST
    return won ? 0 : 1;
#else
    for (;;) frame_with(0);
#endif
}
