/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host P10 "Eye of the Storm" mission checks on the live game.
 *
 * 1. Start-up: P10 is started through the ordinary campaign menu. The first
 *    source pass must run the authenticated start-up trigger: Raynor's
 *    Terran base for the human, the rescuable Hyperion, both computer
 *    wallets and all five town scripts. Zeratul and the Gantrithor are
 *    rescued on contact, and the opening transmissions set switch 8.
 * 2. Save continuation: with no further input the mission runs to a fixed
 *    frame, saves through the pause menu (START, B, START) and keeps
 *    running, recording a state digest every 500 updates. A fresh process
 *    (this program, re-executed with --resume) quick-loads that SRAM image
 *    from the title screen and must reproduce every digest.
 * 3. Outcomes: the source victory and defeat triggers fire on the live
 *    world. These runs kill the Overmind or a hero directly; that is a test
 *    fixture, not ordinary input. An in-process save/load taken during the
 *    victory epilogue (a pending Transmission) must continue identically.
 *
 * These are host checks with no win by ordinary input; see the P10 audit. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define P10_SAVE_FRAME 8000u
#define P10_CONTINUE_UPDATES 10000u
#define P10_DIGEST_PERIOD 500u
#define KEY_TAP(k) do { input((k)); input(0); } while (0)

static u16 prior;

static void input(u16 held)
{
    game_update(held, (u16)(held & (u16)~prior));
    prior = held;
}

static u32 hash_u32(u32 h, u32 v)
{
    for (int i = 0; i < 4; ++i) { h ^= (u8)(v >> (8 * i)); h *= 16777619u; }
    return h;
}
#define H(v) (h = hash_u32(h, (u32)(v)))
/* Gameplay state only: actors, resources and mission/runtime state. */
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
    const u8 *p10 = (const u8 *)&race_p10;
    for (unsigned i = 0; i < sizeof(race_p10); ++i) H(p10[i]);
    return h;
}
#undef H

static int count_raw(int raw, int raw_owner, int live_owner)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw && u[i].raw_owner == raw_owner &&
            (live_owner < 0 || u[i].owner == live_owner)) ++n;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw && b[i].raw_owner == raw_owner &&
            (live_owner < 0 || b[i].owner == live_owner)) ++n;
    return n;
}

static int find_raw(int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_type == raw) return i;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw) return MAX_UNITS + i;
    return -1;
}

static void census(const char *label)
{
    int side = 0, enemy = 0, rescuable = 0, side_b = 0, enemy_b = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        side += u[i].owner == SIDE; enemy += u[i].owner == ENEMY;
        rescuable += u[i].owner == RESCUABLE;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        side_b += b[i].owner == SIDE; enemy_b += b[i].owner == ENEMY;
    }
    printf("p10 %s: frame=%d screen=%d won=%d lost=%d faults=%d side=%d/%d enemy=%d/%d "
           "rescuable=%d minerals=%d gas=%d switches=0x%04x wait=0x%x digest=%u\n",
           label, frame, screen, won, lost, mission_unsupported_count, side, side_b,
           enemy, enemy_b, rescuable, minerals, gas, race_p10.switches,
           race_p10.waiting_mask, state_digest());
}

static void start_p10(void)
{
    int ordinal, page;
    game_init();
    prior = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    KEY_TAP(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_PROTOSS);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS, SC64_P10_GLOBAL_ID);
    assert(ordinal == 9);
    page = ordinal / 2;
    for (int i = 0; i < page; ++i) KEY_TAP(KEY_RIGHT);
    if (ordinal & 1) KEY_TAP(KEY_DOWN);
    KEY_TAP(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_P10_GLOBAL_ID);
    input(KEY_START);
    if (!(screen == SCREEN_PLAY && race_p10_active() && race_p10_map)) {
        census("start-failed");
        exit(3);
    }
}

static void run_updates(unsigned n)
{
    for (unsigned i = 0; i < n; ++i) {
        input(0);
        if (screen != SCREEN_PLAY && !won && !lost) {
            census("fault");
            exit(4);
        }
    }
}

static void check_startup(void)
{
    start_p10();
    census("launch");
    run_updates(1);
    census("first-pass");
    /* Start-up trigger 1 completed for the human runner. */
    assert(race_p10.runner[SC64_P10_RUNNER_HUMAN].done & (1u << 1));
    assert(race_p10.switches & 1u);
    assert(minerals == 100 && gas == 100);
    for (int w = 0; w < SC64_P10_WALLET_COUNT; ++w)
        assert(race_p10.wallet_minerals[w] == 8000 && race_p10.wallet_gas[w] == 8000);
    for (int t = 0; t < SC64_P10_TOWN_COUNT; ++t) assert(race_p10.towns[t].script == t + 1);
    /* Raynor's base: created for source slot 1 on the human side. */
    assert(count_raw(106, 1, SIDE) == 1 && count_raw(109, 1, SIDE) == 3);
    assert(count_raw(7, 1, SIDE) == 4 && count_raw(32, 1, SIDE) == 2);
    assert(count_raw(0, 1, SIDE) == 12 && count_raw(111, 1, SIDE) == 1);
    assert(count_raw(125, 1, SIDE) == 2 && count_raw(122, 1, SIDE) == 1);
    assert(count_raw(124, 1, SIDE) == 1);
    /* The Hyperion starts rescuable; Zeratul and the Gantrithor start next to
     * human units and are rescued on contact into the human slot. */
    assert(count_raw(28, 2, RESCUABLE) == 1);
    assert(count_raw(75, 1, SIDE) == 1 && count_raw(82, 1, SIDE) == 1);
    assert(count_raw(147, 3, ENEMY) == 1);
    /* Opening transmissions and wait, then switch 8 (trigger 2, then 6). */
    for (unsigned n = 0; n < 1200 && !(race_p10.switches & (1u << 8)); ++n) run_updates(1);
    census("opening-done");
    assert(race_p10.switches & (1u << 8));
    assert(race_p10.runner[SC64_P10_RUNNER_HUMAN].done & (1u << 2));
    /* Trigger 6 starts in the same pass and plays one Transmission. */
    for (unsigned n = 0; n < 200 && !(race_p10.runner[SC64_P10_RUNNER_HUMAN].done & (1u << 6)); ++n)
        run_updates(1);
    assert(race_p10.runner[SC64_P10_RUNNER_HUMAN].done & (1u << 6));
    assert(!race_p10.waiting_mask);
    assert(race_p10_outcome() == SC64_P10_OUTCOME_RUNNING && !won && !lost);
}

/* Both runs stop at the same frame; the pause-menu resume and the title
 * quick-load each advance one frame on their final key release. */
static void write_digests(FILE *out)
{
    while ((unsigned)frame < P10_SAVE_FRAME + P10_CONTINUE_UPDATES &&
           screen != SCREEN_RESULT) {
        input(0);
        if ((unsigned)frame % P10_DIGEST_PERIOD == 0)
            fprintf(out, "frame=%d screen=%d won=%d lost=%d digest=%u\n",
                    frame, screen, won, lost, state_digest());
    }
    fprintf(out, "final frame=%d screen=%d won=%d lost=%d digest=%u\n",
            frame, screen, won, lost, state_digest());
}

static int resume_main(const char *dir)
{
    char path[512];
    FILE *f;
    snprintf(path, sizeof(path), "%s/p10-save.sav", dir);
    f = fopen(path, "rb");
    if (!f || fread(host_save, 1, sizeof(host_save), f) != sizeof(host_save)) {
        perror(path); return 2;
    }
    fclose(f);
    game_init();
    prior = 0;
    KEY_TAP(KEY_L);
    if (screen != SCREEN_PLAY || !race_p10_active() || (unsigned)frame != P10_SAVE_FRAME + 1u) {
        census("resume-failed");
        return 3;
    }
    census("resumed");
    snprintf(path, sizeof(path), "%s/p10-resume.txt", dir);
    f = fopen(path, "w");
    if (!f) { perror(path); return 2; }
    write_digests(f);
    fclose(f);
    census("resume-end");
    return 0;
}

static int files_equal(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int ca, cb, same = fa && fb;
    while (same) {
        ca = fgetc(fa); cb = fgetc(fb);
        if (ca != cb) same = 0;
        if (ca == EOF || cb == EOF) break;
    }
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return same;
}

static void check_save_continuation(const char *self, const char *dir)
{
    char path[512], full[512], resumed[512];
    FILE *f;
    pid_t pid;
    int status = 0;
    while ((unsigned)frame < P10_SAVE_FRAME) run_updates(1);
    /* No selection, open menu or build mode is held while idle. */
    assert(!menu_open && mode == MODE_NORMAL);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PAUSE && (unsigned)frame == P10_SAVE_FRAME);
    KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PLAY && (unsigned)frame == P10_SAVE_FRAME + 1u);
    census("saved");
    snprintf(path, sizeof(path), "%s/p10-save.sav", dir);
    f = fopen(path, "wb");
    assert(f && fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    fclose(f);
    snprintf(full, sizeof(full), "%s/p10-full.txt", dir);
    f = fopen(full, "w");
    assert(f);
    write_digests(f);
    fclose(f);
    census("continued");
    assert(race_p10_outcome() == SC64_P10_OUTCOME_RUNNING || lost);

    fflush(stdout);
    pid = fork();
    assert(pid >= 0);
    if (!pid) {
        execl(self, self, "--resume", dir, (char *)0);
        _exit(127);
    }
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    snprintf(resumed, sizeof(resumed), "%s/p10-resume.txt", dir);
    if (!files_equal(full, resumed)) {
        printf("p10 resume diverged: compare %s and %s\n", full, resumed);
        exit(5);
    }
    printf("p10 resume matched the uninterrupted run (%u updates)\n",
           P10_CONTINUE_UPDATES);
}

/* Lethal damage only, so the dead row stays inside the save codecs' HP
 * range as it would after ordinary combat. */
static void kill_actor(int index)
{
    int damage;
    assert(index >= 0);
    if (index < MAX_UNITS) {
        invincible_units[index] = 0;
        damage = u[index].hp + u[index].shield + 1;
    } else {
        invincible_buildings[index - MAX_UNITS] = 0;
        damage = b[index - MAX_UNITS].hp + b[index - MAX_UNITS].shield + 1;
    }
    hit_target_impl_ex(index, damage, -1, -1, -1,
                       SC64_HIT_PRECOMPUTED_HP | SC64_HIT_SKIP_HALLUCINATION_SCALE);
    assert(index < MAX_UNITS ? !u[index].alive : !b[index - MAX_UNITS].alive);
}

static void check_victory(void)
{
    u32 a, bdig;
    start_p10();
    run_updates(1);
    kill_actor(find_raw(147));
    /* Run into the epilogue until a Transmission is pending. */
    for (unsigned n = 0; n < 400 && !(race_p10.runner[SC64_P10_RUNNER_HUMAN].running & (1u << 7)); ++n)
        run_updates(1);
    for (unsigned n = 0; n < 400 && !(race_p10.waiting_mask & 1u); ++n) run_updates(1);
    assert(race_p10.runner[SC64_P10_RUNNER_HUMAN].running & (1u << 7));
    assert(race_p10.waiting_mask & 1u);
    census("epilogue");
    assert(race_save_game());
    run_updates(300);
    a = state_digest();
    assert(race_load_game());
    run_updates(300);
    bdig = state_digest();
    assert(a == bdig);
    for (unsigned n = 0; n < 2000 && !won; ++n) run_updates(1);
    census("victory");
    assert(won && !lost && race_p10_outcome() == SC64_P10_OUTCOME_WON);
    assert(race_completed & (1 << SC64_P10_GLOBAL_ID));
}

static void check_defeat(int raw)
{
    char label[32];
    start_p10();
    run_updates(1);
    kill_actor(find_raw(raw));
    /* The defeat trigger's Wait queues behind the opening transmissions on
     * the human runner's shared wait timer, as native waits do. */
    for (unsigned n = 0; n < 1200 && !lost; ++n) run_updates(1);
    snprintf(label, sizeof(label), "defeat-raw%d", raw);
    census(label);
    assert(lost && !won && race_p10_outcome() == SC64_P10_OUTCOME_LOST);
}

int main(int argc, char **argv)
{
    gba_init();
    if (argc == 3 && !strcmp(argv[1], "--resume")) return resume_main(argv[2]);
    if (argc != 2) { fprintf(stderr, "usage: %s OUTDIR\n", argv[0]); return 2; }
    check_startup();
    check_save_continuation(argv[0], argv[1]);
    check_victory();
    check_defeat(75);
    check_defeat(82);
    check_defeat(28);
    printf("p10 mission host checks passed\n");
    return 0;
}
