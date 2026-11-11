/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P10 Gantrithor as a Carrier-class parent, on the live game.
 *
 * The Gantrithor (raw 82) uses the shared Carrier hangar service: it joins
 * with a full hangar of four Interceptors, releases them at an enemy, takes
 * them back, rebuilds lost ones for 25 minerals each and takes its children
 * with it when it dies or changes hands. These checks start P10 through the
 * ordinary campaign menu and then step the whole game with game_update().
 * Staged enemies are fixtures; everything that follows runs the live code.
 *
 *   ./sc64-p10-gantrithor OUTDIR            all checks, including the
 *                                           fresh-process save continuation
 *   ./sc64-p10-gantrithor --resume OUTDIR   (internal) fresh-process half
 *   ./sc64-p10-gantrithor --old-save FILE   load a SRAM image written by an
 *                                           older ROM and keep playing */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define KEY_TAP(k) do { input((k)); input(0); } while (0)
#define STAGE_UPDATES 700u
#define CONTINUE_UPDATES 1500u
#define DIGEST_PERIOD 100u

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
/* Gameplay state, including every Interceptor field and the projectile pool. */
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
        H(q->build); H(q->work_timer); H(q->path_retry); H(q->path_timer);
        H(q->shield_remainder);
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->build_progress); H(q->x); H(q->y);
        H(q->raw_type); H(q->raw_owner); H(q->queue_progress);
        H(q->owner); H(q->type); H(q->queue_count); H(q->under_construction);
    }
    for (unsigned i = 0; i < SC64_ARENA_WORDS; ++i) H(race_p7_projectile_host_words[i]);
    H(minerals); H(gas); H(frame); H(race_campaign.elapsed_frames);
    {
        const u8 *p10 = (const u8 *)&race_p10;
        for (unsigned i = 0; i < sizeof(race_p10); ++i) H(p10[i]);
    }
    return h;
}
#undef H

static void start_p10(void)
{
    int ordinal;
    game_init();
    prior = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    KEY_TAP(KEY_A);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS, SC64_P10_GLOBAL_ID);
    assert(ordinal == 9);
    for (int i = 0; i < ordinal / 2; ++i) KEY_TAP(KEY_RIGHT);
    if (ordinal & 1) KEY_TAP(KEY_DOWN);
    KEY_TAP(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_P10_GLOBAL_ID);
    input(KEY_START);
    assert(screen == SCREEN_PLAY && race_p10_active() && race_p10_map);
}

static void close_menus(void)
{
    if (menu_open) KEY_TAP(KEY_B);
    if (mode != MODE_NORMAL) KEY_TAP(KEY_B);
}

static void run(unsigned n)
{
    for (unsigned i = 0; i < n; ++i) {
        input(0);
        if (screen != SCREEN_PLAY || won) {
            printf("mission left play: screen=%d won=%d lost=%d frame=%d\n", screen, won, lost, frame);
            exit(4);
        }
    }
}

static int gantrithor(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P10_GANTRITHOR) return i;
    return -1;
}

static int count_state(int parent, unsigned state)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
            sc64_p7_carrier_child_state(&u[i]) == state &&
            (parent < 0 || u[i].build == parent)) ++n;
    return n;
}

static int count_children(int parent)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR && u[i].build == parent) ++n;
    return n;
}

static int find_state(int parent, unsigned state)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
            sc64_p7_carrier_child_state(&u[i]) == state &&
            (parent < 0 || u[i].build == parent)) return i;
    return -1;
}

static int live_units(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    return n;
}

static int pool_active(void)
{ return sc64_pool_active_head(&race_p7_projectiles) != SC64_END; }

/* Lethal damage through the ordinary hit path, like combat. */
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
}

static void place(Unit *q, int x, int y)
{
    q->x = x; q->y = y; q->tx = x; q->ty = y;
    sc64_unit_bucket_move(q);
    sc64_target_index_move(q);
}

/* The spot farthest from every other actor, for the arena: nothing else can
 * join the fight within a few hundred pixels. */
static void quiet_spot(int *x, int *y)
{
    int best = -1;
    for (int py = 200; py < WORLD_H - 200; py += 40)
        for (int px = 200; px < WORLD_W - 200; px += 40) {
            int nearest = 0x7fffffff;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].type != U_P10_GANTRITHOR &&
                    u[i].type != U_P7_INTERCEPTOR) {
                    int d = dist2(px, py, u[i].x, u[i].y);
                    if (d < nearest) nearest = d;
                }
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive) {
                    int d = dist2(px, py, b[i].x, b[i].y);
                    if (d < nearest) nearest = d;
                }
            if (nearest > best) { best = nearest; *x = px; *y = py; }
        }
    assert(best >= 300 * 300);
}

/* An enemy Hatchery (it cannot shoot back) 110 px from the Gantrithor. */
static int stage_target(int g)
{
    int slot = spawn_build(ENEMY, B_HATCHERY, u[g].x + 110, u[g].y, 0);
    assert(slot >= 0);
    b[slot].hp = b[slot].maxhp; b[slot].under_construction = 0;
    /* A source-identified Zerg structure of the Overmind's brood (slot 3). */
    b[slot].raw_type = 131; b[slot].raw_owner = 3;
    sc64_visibility_reset(); update_exploration();
    return MAX_UNITS + slot;
}

/* A started P10 mission with the Gantrithor moved to a quiet spot and the
 * target staged next to it. */
static int arena(int *hatchery)
{
    int x, y, g;
    start_p10();
    run(3);
    g = gantrithor();
    assert(g >= 0 && u[g].owner == SIDE && u[g].raw_owner == SC64_P10_HUMAN_SLOT);
    quiet_spot(&x, &y);
    place(&u[g], x, y);
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    *hatchery = stage_target(g);
    return g;
}

/* Joins with a full hangar of four docked children in dense rank order,
 * owned like the parent. */
static void check_full_hangar(void)
{
    int g;
    start_p10();
    run(3);
    g = gantrithor();
    assert(g >= 0 && sc64_p7_carrier_parent_valid(g));
    assert(u[g].owner == SIDE && u[g].raw_owner == SC64_P10_HUMAN_SLOT);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    assert(count_children(g) == 4 && race_p7_carrier_training_child_count(g) == 4);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR) {
            assert(u[i].owner == SIDE && u[i].raw_owner == SC64_P10_HUMAN_SLOT);
            assert(u[i].hp == 40 && u[i].shield == 40 && u[i].build == g);
            assert(SC64_CARRIER_HANGAR_HIDDEN(&u[i]));
        }
    assert(race_p7_carrier_training_live_world_valid());
    printf("gantrithor: joined with a full hangar (4 docked), actors alive=%d\n",
           live_units());
}

/* Launch cadence, attack passes, projectile damage, target death, return,
 * docking, and the ordinary Interceptor rebuild. */
static void check_launch_return_rebuild(void)
{
    int hatchery, g = arena(&hatchery), target_hp, saw_projectile = 0;
    int outside_peak = 0;
    assert(u[g].order == ORDER_IDLE);
    assert(race_p7_flight_target_ok(&u[g], hatchery));
    target_hp = b[hatchery - MAX_UNITS].hp + b[hatchery - MAX_UNITS].shield;
    /* An idle Gantrithor with an enemy in 8 tiles holds and launches, one
     * child every 7 source ticks. */
    assert(race_p7_interceptor_flight_carrier_holds(g));
    for (unsigned n = 0; n < 200; ++n) {
        run(1);
        saw_projectile |= pool_active();
        if (count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) > outside_peak)
            outside_peak = count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE);
    }
    assert(outside_peak == 4);
    assert(saw_projectile);
    assert(b[hatchery - MAX_UNITS].hp + b[hatchery - MAX_UNITS].shield < target_hp);
    assert(race_p7_carrier_training_live_world_valid());
    /* Interceptors stay within their leash of the parent. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR &&
            sc64_p7_carrier_child_state(&u[i]) == SC64_P7_INTERCEPTOR_OUTSIDE)
            assert(dist2(u[i].x, u[i].y, u[g].x, u[g].y) <= 260 * 260);
    /* Target death: every launched child returns and docks, repaired. */
    kill_actor(hatchery);
    assert(!b[hatchery - MAX_UNITS].alive);
    for (unsigned n = 0; n < 600 && count_state(g, SC64_P7_INTERCEPTOR_INSIDE) != 4; ++n)
        run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) == 0);
    run(60);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    assert(race_p7_carrier_training_live_world_valid());
    printf("gantrithor: launch, projectile damage, target death, return and dock ok\n");

    /* Lose one Interceptor in flight (docked ones cannot be hit): the rest
     * return and the hangar holds three. A paid replacement is then built
     * for exactly 25 minerals through the shared queue. */
    hatchery = stage_target(g);
    for (unsigned n = 0; n < 300 && count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) < 2; ++n) run(1);
    {
        int child = find_state(g, SC64_P7_INTERCEPTOR_OUTSIDE);
        assert(child >= 0);
        kill_actor(child);
        assert(!u[child].alive);
    }
    assert(race_p7_carrier_training_live_world_valid());
    kill_actor(hatchery);
    for (unsigned n = 0; n < 600 && count_state(g, SC64_P7_INTERCEPTOR_INSIDE) != 3; ++n) run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 3);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) == 0 && count_children(g) == 3);
    assert(race_p7_carrier_training_live_world_valid());
    minerals = 100;
    /* The ordinary command row: select only the Gantrithor and ask. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    selected_building = -1;
    u[g].selected = 1;
    assert(race_p7_carrier_training_menu_available());
    choose_command(MENU_STOP + 1);
    assert(minerals == 75 && u[g].carry == 1);
    u[g].selected = 0;
    {
        int before = live_units();
        for (unsigned n = 0; n < 40 && count_state(g, SC64_P7_INTERCEPTOR_TRAINING) == 0; ++n)
            run(1);
        assert(count_state(g, SC64_P7_INTERCEPTOR_TRAINING) == 1);
        assert(live_units() == before + 1 && u[g].carry == 0);
    }
    for (unsigned n = 0; n < 600 && count_state(g, SC64_P7_INTERCEPTOR_INSIDE) != 4; ++n)
        run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 4);
    assert(minerals >= 75 && minerals < 400);
    assert(race_p7_carrier_training_live_world_valid());
    /* A full hangar refuses another order and charges nothing. */
    {
        int m = minerals;
        assert(!race_p7_carrier_training_queue(g));
        assert(minerals == m && u[g].carry == 0);
    }
    printf("gantrithor: lost child rebuilt for 25 minerals through the shared queue\n");
}

/* The Gantrithor's death: docked children vanish at once, launched ones
 * outlive it for 15 to 45 source ticks and then go. */
static void check_parent_death(void)
{
    int hatchery, g = arena(&hatchery), outside = 0;
    for (unsigned n = 0; n < 200 && (outside = count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE)) < 2; ++n)
        run(1);
    assert(outside >= 2);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) >= 1);
    {
        int launched = count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE);
        kill_actor(g);
        assert(!u[g].alive);
        /* At once: hangar children (docked and in production) are gone,
         * launched ones are orphaned with a 15-45 source tick life. */
        assert(count_state(-1, SC64_P7_INTERCEPTOR_INSIDE) == 0);
        assert(count_state(-1, SC64_P7_INTERCEPTOR_TRAINING) == 0);
        assert(count_state(-1, SC64_P7_INTERCEPTOR_DETACHED) == launched);
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].type == U_P7_INTERCEPTOR)
                assert(u[i].build == -1 && u[i].carry >= 15 && u[i].carry <= 45);
        assert(race_p7_carrier_training_live_world_valid());
    }
    /* The orphans survive a save and load. */
    close_menus();
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PAUSE);
    KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PLAY);
    assert(race_load_game());
    assert(count_state(-1, SC64_P7_INTERCEPTOR_DETACHED) >= 1);
    assert(race_p7_carrier_training_live_world_valid());
    for (unsigned n = 0; n < 80 && count_state(-1, SC64_P7_INTERCEPTOR_DETACHED); ++n) run(1);
    assert(!count_state(-1, SC64_P7_INTERCEPTOR_DETACHED));
    for (int i = 0; i < MAX_UNITS; ++i)
        assert(!(u[i].alive && u[i].type == U_P7_INTERCEPTOR));
    assert(race_p7_carrier_training_live_world_valid());
    printf("gantrithor: death removes docked children at once and launched ones after 15-45 ticks\n");
}

/* A parent that changes hands takes its children along. */
static void check_owner_change(void)
{
    int g, marine, before;
    start_p10();
    run(3);
    g = gantrithor();
    assert(g >= 0 && count_children(g) == 4);
    /* Back to a rescuable actor with a full hangar, then rescued again by
     * contact with a human unit: the children follow and none are added. */
    u[g].owner = RESCUABLE; u[g].raw_owner = 2;
    race_p7_carrier_training_owner_changed(g);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR) {
            assert(u[i].owner == RESCUABLE && u[i].raw_owner == 2);
        }
    assert(race_p7_carrier_training_live_world_valid());
    {
        int x, y;
        quiet_spot(&x, &y);
        place(&u[g], x, y);
        marine = spawn_unit(SIDE, U_MARINE, x + 8, y);
        assert(marine >= 0);
        u[marine].raw_owner = SC64_P10_HUMAN_SLOT;
    }
    before = live_units();
    run(3);
    assert(u[g].owner == SIDE && u[g].raw_owner == SC64_P10_HUMAN_SLOT);
    assert(count_children(g) == 4);
    assert(live_units() == before);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR)
            assert(u[i].owner == SIDE && u[i].raw_owner == SC64_P10_HUMAN_SLOT);
    assert(race_p7_carrier_training_live_world_valid());
    printf("gantrithor: rescue moves the hangar with its parent\n");
}

/* Children occupy unit slots: with the pool full a paid order waits, stock
 * adds no more than fit, and nothing overflows. */
static void check_actor_cap(void)
{
    int g, free_slots = 0, filler[MAX_UNITS], fills = 0, minerals_before;
    start_p10();
    run(3);
    g = gantrithor();
    assert(g >= 0 && count_children(g) == 4);
    /* Lose three children (removed as the service does when one is shot
     * down), then fill the pool leaving two free. */
    for (int k = 0; k < 3; ++k)
        sc64_p7_carrier_child_remove(find_state(g, SC64_P7_INTERCEPTOR_INSIDE),
                                     race_p7_projectile_clear_references);
    assert(count_children(g) == 1);
    for (int i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
    assert(free_slots > 8);
    while (free_slots > 2) {
        int s = spawn_unit(ENEMY, U_ZERGLING, 40, 40);
        assert(s >= 0);
        /* Source-identified brood units (slot 3), as the mission's own. */
        u[s].raw_type = 37; u[s].raw_owner = 3;
        filler[fills++] = s; --free_slots;
    }
    /* Pool pressure: stock adds only what fits. */
    assert(race_p7_carrier_training_stock(g, 3) == 2);
    assert(count_children(g) == 3);
    /* No free slot left: a paid order is refused and costs nothing. */
    minerals = 200; minerals_before = minerals;
    assert(!race_p7_carrier_training_queue(g));
    assert(minerals == minerals_before && u[g].carry == 0);
    /* One slot back: queued and built; the pool never overflows. */
    u[filler[--fills]].alive = 0;
    assert(race_p7_carrier_training_queue(g));
    assert(minerals == minerals_before - 25 && u[g].carry == 1);
    for (unsigned n = 0; n < 600 && count_state(g, SC64_P7_INTERCEPTOR_INSIDE) != 4; ++n)
        run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 4 && u[g].carry == 0);
    assert(race_p7_carrier_training_live_world_valid());
    printf("gantrithor: actor cap respected (stock and paid queue wait for free slots)\n");
}

/* A save written before this change has a Gantrithor and no children. It
 * loads, plays on, and can rebuild a hangar for money. */
static void check_hangarless_legacy_state(void)
{
    int g;
    start_p10();
    run(3);
    g = gantrithor();
    assert(g >= 0 && count_children(g) == 4);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_P7_INTERCEPTOR) {
            u[i].alive = 0; u[i].build = -1; u[i].flags = 0; u[i].target = -1;
        }
    assert(count_children(g) == 0);
    /* Stand where no one else is, so the rest of the run is quiet. */
    run(2);
    assert(race_p7_carrier_training_live_world_valid());
    close_menus();
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PAUSE);
    KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PLAY);
    assert(race_load_game());
    g = gantrithor();
    assert(g >= 0 && sc64_p7_carrier_parent_valid(g) && count_children(g) == 0);
    run(300);
    /* No automatic restock: only the player's order builds children. */
    assert(count_children(g) == 0);
    minerals = 100;
    assert(race_p7_carrier_training_queue(g));
    for (unsigned n = 0; n < 700 && count_state(g, SC64_P7_INTERCEPTOR_INSIDE) != 1; ++n)
        run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_INSIDE) == 1);
    printf("gantrithor: a hangarless Gantrithor (older save state) loads and can rebuild\n");
}

/* In-process save/load with Interceptors docked, in flight and in
 * production resumes bit for bit. */
static void stage_fight(int *g_out)
{
    int hatchery, g = arena(&hatchery);
    /* Out for the fight, with the hangar part-emptied and one in build. */
    for (unsigned n = 0; n < 400 && count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) < 3; ++n) run(1);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) >= 2);
    minerals = 300;
    u[g].selected = 1;
    for (int i = 0; i < MAX_UNITS; ++i) if (i != g) u[i].selected = 0;
    selected_building = -1;
    choose_command(MENU_STOP + 1);
    u[g].selected = 0;
    close_menus();
    for (unsigned n = 0; n < 60 && !count_state(g, SC64_P7_INTERCEPTOR_TRAINING); ++n) run(1);
    *g_out = g;
}

static void check_in_process_save(void)
{
    int g;
    u32 a, c;
    stage_fight(&g);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) >= 2);
    assert(race_p7_carrier_training_live_world_valid());
    assert(race_save_game());
    run(400);
    a = state_digest();
    assert(race_load_game());
    run(400);
    c = state_digest();
    if (a != c) { printf("in-process save continuation diverged: %u vs %u\n", a, c); exit(6); }
    printf("gantrithor: in-process save/load with children docked, in flight and building matches (digest %u)\n", a);
}

static void write_digests(FILE *out)
{
    unsigned stop = (unsigned)frame + CONTINUE_UPDATES;
    while ((unsigned)frame < stop && screen != SCREEN_RESULT) {
        input(0);
        if ((unsigned)frame % DIGEST_PERIOD == 0)
            fprintf(out, "frame=%d screen=%d won=%d lost=%d outside=%d inside=%d digest=%u\n",
                    frame, screen, won, lost, count_state(-1, SC64_P7_INTERCEPTOR_OUTSIDE),
                    count_state(-1, SC64_P7_INTERCEPTOR_INSIDE), state_digest());
    }
    fprintf(out, "final frame=%d screen=%d digest=%u\n", frame, screen, state_digest());
}

static int resume_main(const char *dir)
{
    char path[512];
    FILE *f;
    int g;
    snprintf(path, sizeof(path), "%s/gantrithor-save.sav", dir);
    f = fopen(path, "rb");
    if (!f || fread(host_save, 1, sizeof(host_save), f) != sizeof(host_save)) {
        perror(path); return 2;
    }
    fclose(f);
    game_init();
    prior = 0;
    KEY_TAP(KEY_L);
    if (screen != SCREEN_PLAY || !race_p10_active()) { printf("resume failed\n"); return 3; }
    g = gantrithor();
    assert(g >= 0 && count_children(g) >= 3);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) >= 1);
    snprintf(path, sizeof(path), "%s/gantrithor-resume.txt", dir);
    f = fopen(path, "w");
    if (!f) { perror(path); return 2; }
    write_digests(f);
    fclose(f);
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

/* A pause-menu save taken mid-fight reloads in a fresh process and
 * continues exactly as the uninterrupted run. */
static void check_fresh_process_save(const char *self, const char *dir)
{
    char path[512], full[512], resumed[512];
    FILE *f;
    pid_t pid;
    int status = 0, g;
    stage_fight(&g);
    close_menus();
    for (int k = 0; k < 4 && (menu_open || mode != MODE_NORMAL); ++k) KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PAUSE);
    KEY_TAP(KEY_B);
    KEY_TAP(KEY_START);
    assert(screen == SCREEN_PLAY);
    assert(count_state(g, SC64_P7_INTERCEPTOR_OUTSIDE) >= 1);
    snprintf(path, sizeof(path), "%s/gantrithor-save.sav", dir);
    f = fopen(path, "wb");
    assert(f && fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    fclose(f);
    snprintf(full, sizeof(full), "%s/gantrithor-full.txt", dir);
    f = fopen(full, "w");
    assert(f);
    write_digests(f);
    fclose(f);
    fflush(stdout);
    pid = fork();
    assert(pid >= 0);
    if (!pid) {
        execl(self, self, "--resume", dir, (char *)0);
        _exit(127);
    }
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    snprintf(resumed, sizeof(resumed), "%s/gantrithor-resume.txt", dir);
    if (!files_equal(full, resumed)) {
        printf("gantrithor resume diverged: compare %s and %s\n", full, resumed);
        exit(5);
    }
    printf("gantrithor: fresh-process save continuation matched the uninterrupted run (%u updates)\n",
           CONTINUE_UPDATES);
}

/* An SRAM image written by an older ROM: load it from the title screen,
 * check the Gantrithor and run on. */
static int old_save_main(const char *file)
{
    FILE *f = fopen(file, "rb");
    int g, mission_frame;
    if (!f || fread(host_save, 1, sizeof(host_save), f) != sizeof(host_save)) {
        perror(file); return 2;
    }
    fclose(f);
    game_init();
    prior = 0;
    KEY_TAP(KEY_L);
    if (screen != SCREEN_PLAY || !race_p10_active()) { printf("old save did not load\n"); return 3; }
    g = gantrithor();
    mission_frame = frame;
    assert(g >= 0 && sc64_p7_carrier_parent_valid(g));
    printf("old save loaded at frame %d: Gantrithor owner=%d hp=%d children=%d carry=%d\n",
           mission_frame, u[g].owner, u[g].hp, count_children(g), u[g].carry);
    run(2000);
    g = gantrithor();
    printf("old save continued to frame %d: Gantrithor %s, children=%d, live=%d\n",
           frame, g >= 0 ? "alive" : "gone", g >= 0 ? count_children(g) : 0,
           race_p7_carrier_training_live_world_valid());
    assert(race_p7_carrier_training_live_world_valid());
    printf("old save check passed\n");
    return 0;
}

int main(int argc, char **argv)
{
    gba_init();
    if (argc == 3 && !strcmp(argv[1], "--resume")) return resume_main(argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--old-save")) return old_save_main(argv[2]);
    if (argc != 2) { fprintf(stderr, "usage: %s OUTDIR\n", argv[0]); return 2; }
    check_full_hangar();
    check_launch_return_rebuild();
    check_parent_death();
    check_owner_change();
    check_actor_cap();
    check_hangarless_legacy_state();
    check_in_process_save();
    check_fresh_process_save(argv[0], argv[1]);
    printf("p10 gantrithor host checks passed\n");
    return 0;
}
