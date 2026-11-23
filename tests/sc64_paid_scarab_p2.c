/* SPDX-License-Identifier: GPL-3.0-or-later
 * Paid Scarabs on the Protoss campaign, mission 2 (P2), with no fixture.
 *
 * P2 changes with the paid-Scarab rule because the human has Reavers there. The
 * P2 controller's recorded pad tape plays the whole mission through ordinary
 * input; text-box acknowledgements are the one thing the host controller adds
 * outside its tape, so `expand` writes them into a self-contained pad tape. This
 * program replays a pad tape (optionally with an ordinary pause-menu save inserted by
 * tools/sc64_paid_scarab_p2_tape.py) through game_update() and does:
 *
 *   expand TAPE OUT      write the controller tape with the text-box inputs added
 *   find TAPE            first call with a human-owned Scarab in flight (FIND_FRAME=n: not before frame n)
 *   trace TAPE           frame, minerals, gas and a digest every TRACE_EVERY calls (to compare with the ROM)
 *   ref  TAPE            play TAPE with no save; sample fixed frames from the save frame on
 *   run  TAPE SAV        play the tape with the save; the pause menu writes SAV; same samples
 *   resume TAPE SAV      fresh process: load SAV from the title with L, continue the tape after
 *                        the save inputs, same samples
 *
 * Samples are printed as "S frame=.. minerals=.. gas=.. stock=.. scarabs=.. sx=.. sy=.. units=..
 * allhp=.. allsh=.. hits=.. digest=.." and compared by tools/sc64_paid_scarab_p2_host.py. The same
 * tape is replayed on the ROM by
 * tools/sc64_paid_scarab_p2_arm.py. State is read, never written. */
#include "../common/gba.c"
#include "../common/gba.h"
#include "sc64_text_box_replay.h"
#define SC64_TEST_HIT_OBSERVER psc_hit
static void psc_hit(int target, int damage, int source, int owner);
#define SC64_TEST_DETONATION_OBSERVER psc_det
static void psc_det(int slot);
#include "../starcraft/game.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef SC64_PAID_SCARAB
#error "build with the P10 product flags"
#endif

volatile u16 demo_keys;
static u16 previous_held;

enum { MAX_EVENTS = 20000, MAX_LOG = 512, SAMPLES = 12 };
static struct { int tick; int held; } events[MAX_EVENTS];
static int event_count, total_ticks, save_start = -1;

static struct { unsigned frame; int target, damage; } hit_log[MAX_LOG];
static unsigned hit_count;
/* The Scarab followed from the save point: its slot, its target, the frame of its own detonation (-1 while it
 * flies, -2 if it ended without one) and the hits that detonation (and no other) caused. */
static int detonating = -1, track_slot = -1, track_target = -1, track_life, own_frame = -1, own_hits, own_printed;
static void psc_det(int slot)
{
    detonating = slot;
    if (slot >= 0 && slot == track_slot && own_frame == -1) own_frame = frame;
    if (slot < 0 && own_frame >= 0 && track_slot >= 0 && !own_printed && (own_printed = 1))
        printf("OWN-IMPACT frame=%d target=%d hits=%d\n", own_frame, track_target, own_hits);
}
static void psc_hit(int target, int damage, int source, int owner)
{
    (void)owner;
    if (source == U_REAVER && detonating >= 0 && detonating == track_slot && own_frame >= 0) ++own_hits;
    if (source == U_REAVER && hit_count < MAX_LOG) {
        hit_log[hit_count].frame = (unsigned)frame;
        hit_log[hit_count].target = target; hit_log[hit_count].damage = damage;
        ++hit_count;
    }
}

/* Every game_update() call, including the text-box acknowledgements a tape player must add. */
static u16 *call_log;
static unsigned call_count, call_capacity;

static void frame_with(u16 held)
{
    if (call_log && call_count < call_capacity) call_log[call_count] = held;
    ++call_count;
    demo_keys = held;
    game_update(held, held & (u16)~previous_held);
    audio_update();
    game_render();
    previous_held = held;
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
        H(q->build); H(q->work_timer);
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->build_progress); H(q->x); H(q->y);
        H(q->raw_type); H(q->raw_owner); H(q->queue_progress);
        H(q->owner); H(q->type); H(q->queue_count); H(q->under_construction);
    }
    H(minerals); H(gas); H(frame);
    return h;
}
#undef H

static int human_scarab(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SCARAB && u[i].owner == SIDE) return i;
    return -1;
}

static void load_tape(const char *path)
{
    static char text[1 << 20];
    FILE *f = fopen(path, "rb");
    size_t n; char *p;
    if (!f) { perror(path); exit(2); }
    n = fread(text, 1, sizeof text - 1, f); text[n] = 0; fclose(f);
    p = text;
    while ((p = strstr(p, "{\"tick\":")) != NULL) {
        int tick, held;
        if (sscanf(p, "{\"tick\":%d,\"held\":%d}", &tick, &held) == 2 && event_count < MAX_EVENTS) {
            events[event_count].tick = tick; events[event_count].held = held; ++event_count;
        }
        p += 8;
    }
    p = strstr(text, "\"total_ticks\":"); total_ticks = p ? atoi(p + 14) : 0;
    p = strstr(text, "\"save_start\":"); if (p) save_start = atoi(p + 13);
    assert(event_count > 0 && total_ticks > 0);
}

/* Sample points: the save frame plus these offsets, in the world's frame counter. */
static const int sample_after[SAMPLES] = {0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144};
static int sample_base = -1, next_sample;

static int stock_of_first_reaver(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_REAVER) return race_paid_scarab_stock(i);
    return -1;
}

static void sample_now(void)
{
    int s = human_scarab(), nh = 0;
    long hp_sum = 0, sh_sum = 0;
    for (unsigned i = 0; i < hit_count; ++i) {
        int t = hit_log[i].target;
        if (t < MAX_UNITS ? u[t].alive : b[t - MAX_UNITS].alive) {
            ++nh;
            hp_sum += t < MAX_UNITS ? u[t].hp : b[t - MAX_UNITS].hp;
            sh_sum += t < MAX_UNITS ? u[t].shield : b[t - MAX_UNITS].shield;
        }
    }
    int alive = 0; long all_hp = 0, all_sh = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) { ++alive; all_hp += u[i].hp; all_sh += u[i].shield; }
    printf("S frame=%d minerals=%d gas=%d stock=%d scarabs=%d sx=%d sy=%d units=%d allhp=%ld allsh=%ld hits=%u hitlive=%d hitHP=%ld hitSH=%ld digest=%u\n",
           frame, minerals, gas, stock_of_first_reaver(), s >= 0 ? 1 : 0,
           s >= 0 ? u[s].x : -1, s >= 0 ? u[s].y : -1, alive, all_hp, all_sh, hit_count, nh, hp_sum, sh_sum, state_digest());
}

/* The Scarab that is in flight at the save point, followed to its own impact. Each continuation (the
 * uninterrupted tape, the tape with the save, the fresh process) must show that impact, and the same one. */
static void track_begin(void)
{
    int sc = human_scarab();
    assert(sc >= 0);
    track_slot = sc; track_target = u[sc].target; track_life = u[sc].work_timer;
    own_frame = -1; own_hits = 0; own_printed = 0;
    printf("TRACKED Scarab slot %d aimed at %d, %d ticks of flight left\n", track_slot, track_target, track_life);
}
/* The tracked Scarab keeps flying when its Reaver dies, so the parent slot proves nothing: it is the same
 * Scarab while its row is live and its life only counts down. It ends by its own detonation (psc_det) or, if
 * the row goes or is reborn first, without one, which the host check refuses. */
static void track_poll(void)
{
    if (track_slot < 0 || own_frame != -1) return;
    if (u[track_slot].alive && u[track_slot].type == U_SCARAB && u[track_slot].work_timer <= track_life) {
        track_life = u[track_slot].work_timer;
        return;
    }
    own_frame = -2;
    printf("OWN-GONE frame=%d: the tracked Scarab ended without detonating\n", frame);
}

/* Called after every update once the sampling base frame is known. */
static void maybe_sample(void)
{
    track_poll();
    while (sample_base >= 0 && next_sample < SAMPLES && frame >= sample_base + sample_after[next_sample]) {
        sample_now(); ++next_sample;
    }
}

/* Pad value for call number c (events are transitions; latest tick <= c wins). */
static int cursor_ev;
static u16 held_at(int c)
{
    static u16 held;
    while (cursor_ev < event_count && events[cursor_ev].tick <= c) held = (u16)events[cursor_ev++].held;
    return held;
}

enum { FIND_CALLS = 120000 };
static unsigned char fly[FIND_CALLS];
static int fly_frame[FIND_CALLS], fly_stock[FIND_CALLS];
static u16 fly_held[FIND_CALLS];

/* The first neutral call that lies inside a Scarab's flight with room to spare: the pause menu opens
 * on that call and the flight must still be on when the menu has written the save (a few calls later). */
static void pick_flight_call(int calls)
{
    int from = getenv("FIND_FRAME") ? atoi(getenv("FIND_FRAME")) : 0;
    for (int n = 1; n + 8 < calls && n + 8 < FIND_CALLS; ++n) {
        int all = fly_held[n] == 0 && fly_frame[n] >= from;
        for (int k = n - 1; all && k <= n + 8; ++k) all = fly[k];
        if (!all) continue;
        printf("FIRST human Scarab in flight after call %d: frame %d, Reaver stock %d, neutral call inside the flight\n",
               n, fly_frame[n], fly_stock[n]);
        return;
    }
}

enum { SAVE_LEN = 4 };   /* START, neutral, B (SRAM write), A (unpause): no world tick of their own */

static unsigned char before_save[sizeof host_save];

static int play(const char *mode, const char *sav, int first_call)
{
    int saved = 0;
    for (int c = first_call; c < total_ticks && screen != SCREEN_RESULT; ++c) {
        u16 held = held_at(c);
        frame_with(held);
        /* Only while expanding: the expanded tape carries the acknowledgements as ordinary inputs. */
        if (!strcmp(mode, "expand")) TEXT_BOXES_DISMISS(frame_with);
        if (!strcmp(mode, "trace") && (c + 1) % (getenv("TRACE_EVERY") ? atoi(getenv("TRACE_EVERY")) : 500) == 0) {
            int sc = 0, rv = 0;
            for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) { sc += u[i].type == U_SCARAB; rv += u[i].type == U_REAVER && u[i].owner == SIDE; }
            printf("C call=%d frame=%d screen=%d minerals=%d gas=%d reavers=%d scarabs=%d digest=%u\n", c + 1, frame, screen, minerals, gas, rv, sc, state_digest());
        }
        if (!strcmp(mode, "find")) {
            /* Record every call; the pick is made after the pass, once the whole flight is known. */
            if (c < FIND_CALLS) {
                fly[c] = (unsigned char)(human_scarab() >= 0);
                fly_frame[c] = frame; fly_stock[c] = stock_of_first_reaver(); fly_held[c] = held;
            }
            continue;
        }
        if (!strcmp(mode, "run")) {
            if (c == save_start + 1) memcpy(before_save, host_save, sizeof host_save);
            if (c == save_start + 2) {                 /* the B press: the pause menu has written SRAM */
                FILE *f;
                if (!(screen == SCREEN_PAUSE && human_scarab() >= 0)) {
                    printf("NOT SAVED: screen=%d scarab=%d frame=%d call=%d held=%u\n", screen, human_scarab(), frame, c, (unsigned)held);
                    return 4;
                }
                if (!memcmp(before_save, host_save, sizeof host_save)) {
                    /* save_game() refused this update (the campaign writer is picky about some updates). */
                    printf("REFUSED: the pause menu wrote nothing at call %d, frame %d\n", c, frame);
                    return 3;
                }
                f = fopen(sav, "wb");
                assert(f && fwrite(host_save, 1, sizeof host_save, f) == sizeof host_save); fclose(f);
                saved = 1;
                printf("SAVED by the pause menu at call %d, frame %d, with a Scarab in flight\n", c, frame);
            }
            if (c == save_start + SAVE_LEN - 1) {      /* the A press: unpaused, still the same world frame */
                assert(saved && screen == SCREEN_PLAY);
                sample_base = frame; next_sample = 0; hit_count = 0; track_begin();
            }
        }
        if (!strcmp(mode, "ref") && c == save_start - 1) {
            assert(human_scarab() >= 0);
            sample_base = frame; next_sample = 0; hit_count = 0; track_begin();
            printf("REF base frame %d (the uninterrupted tape, Scarab in flight)\n", frame);
        }
        if (!strcmp(mode, "resume")) { /* sample_base set by main */ }
        maybe_sample();
        if (next_sample >= SAMPLES && strcmp(mode, "expand")) break;
    }
    if (!strcmp(mode, "find")) pick_flight_call(total_ticks);
    return 0;
}

int main(int argc, char **argv)
{
    const char *mode;
    if (argc < 3) { fprintf(stderr, "usage: %s expand|find|trace|ref|run|resume TAPE [SAV|OUT]\n", argv[0]); return 2; }
    mode = argv[1]; load_tape(argv[2]);
    printf("T U_SCARAB=%d U_REAVER=%d SIDE=%d\n", (int)U_SCARAB, (int)U_REAVER, (int)SIDE);
    gba_init();
    if (!strcmp(mode, "resume")) {                /* the cartridge's SRAM is in place before boot */
        FILE *f = fopen(argv[3], "rb");
        assert(f && fread(host_save, 1, sizeof host_save, f) == sizeof host_save); fclose(f);
    }
    game_init();
    if (!strcmp(mode, "resume")) {
        frame_with(KEY_L);                    /* the real load action from the title */
        assert(screen == SCREEN_PLAY && human_scarab() >= 0);
        sample_base = frame; next_sample = 0; hit_count = 0; track_begin();
        printf("RESUMED at frame %d with a Scarab in flight\n", frame);
        maybe_sample();                       /* offset 0: the loaded state itself */
        play("resume", argv[3], save_start + SAVE_LEN);
        return next_sample >= SAMPLES ? 0 : 1;
    }
    if (!strcmp(mode, "expand")) {
        /* Play the controller tape and write every call, with the text-box acknowledgements
         * inserted, as a self-contained pad tape (same format as the controller's). */
        FILE *f;
        unsigned prior = 0, first = 1;
        call_capacity = 200000; call_log = calloc(call_capacity, sizeof *call_log);
        play(mode, NULL, 0);
        f = fopen(argv[3], "w");
        assert(f);
        fprintf(f, "{\"total_ticks\":%u,\"events\":[", call_count);
        for (unsigned c = 0; c < call_count; ++c)
            if (call_log[c] != prior || (c == 0 && call_log[c])) {
                fprintf(f, "%s{\"tick\":%u,\"held\":%u}", first ? "" : ",", c, (unsigned)call_log[c]);
                prior = call_log[c]; first = 0;
            }
        fprintf(f, "]}\n");
        fclose(f);
        printf("expanded: %u controller calls -> %u calls (%u text-box frames added)\n", total_ticks, call_count,
               call_count - (unsigned)total_ticks);
        return 0;
    }
    {
        int played = play(mode, argc > 3 ? argv[3] : NULL, 0);
        if (played == 3 || played == 4) return played;
    }
    if (!strcmp(mode, "find") && getenv("FINDALL")) {
        printf("Scarab hits in the whole mission: %u\n", hit_count);
        for (unsigned i = 0; i < hit_count && i < 40; ++i)
            printf("  frame %u target %d damage %d\n", hit_log[i].frame, hit_log[i].target, hit_log[i].damage);
    }
    if (strcmp(mode, "find") && next_sample < SAMPLES) { fprintf(stderr, "ended before all samples\n"); return 1; }
    return 0;
}
