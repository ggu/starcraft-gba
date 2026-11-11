/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST player controller for the common Egg release: orders go through the
 * ordinary Custom Game menu and game_update(); HOST setup only places the
 * prerequisite buildings, bank and pool pressure the scenario needs.
 *
 *   rules               in-process rules: price, larva, supply, timer, pair
 *                       births, cancel, kill, pool and supply caps, Zerg bot
 *   full                uninterrupted run; prints the hatch digest
 *   prefix <file>       same run, pause-menu save at SAVE_AT, SRAM image out
 *   resume <file>       fresh process: SRAM image in, title quick-load,
 *                       finish; prints the same digest
 */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <string.h>
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *dst);
extern void guardian_test_restore(const void *src);

#define SAVE_AT 90
#define HORIZON 900

static int menu_to(int command)
{
    if (menu_open) tap(KEY_B);
    tap(KEY_B);
    for (int n = 0; menu_open && menu_cursor != command && n < 64; n++) {
#ifdef SC64_COMMAND_ICONS
        if (card_fits()) {
            int slot = card_command_slot(menu_cursor), col = card_command_slot(command) % CARD_COLUMNS;
            int row_length = field_menu_count() - (slot / CARD_COLUMNS) * CARD_COLUMNS;
            if (row_length > CARD_COLUMNS) row_length = CARD_COLUMNS;
            if (slot % CARD_COLUMNS != col) tap(col >= row_length ? KEY_UP : KEY_RIGHT);
            else tap(KEY_DOWN);
        } else
#endif
            tap(KEY_DOWN);
    }
    return menu_open && menu_cursor == command;
}

static int own_hall(int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == owner && sc64_race_is_zerg_townhall(b[i].type)) return i;
    return -1;
}

static int egg_of(int owner, int product, int skip)
{
    for (int i = 0; i < MAX_UNITS; i++)
        if (i != skip && u[i].alive && u[i].owner == owner && u[i].type == SC64_EGG_ACTOR &&
            u[i].target == product) return i;
    return -1;
}

static int count_type(int owner, int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; i++) n += u[i].alive && u[i].owner == owner && u[i].type == type;
    return n;
}

static int free_unit_slots(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; i++) n += !u[i].alive;
    return n;
}

static int press_minerals, press_gas, press_free, press_used, press_larva;
static int free_unit_slots(void);
/* Ordinary input: select the Hatchery, cycle the train choice, press A. */
static int order_unit(int hall, int type)
{
    int found = 0;
    if (!select_building(hall)) return 0;
    for (int i = 0; i < 24; i++) {
        if (train_type == type) { found = 1; break; }
        tap(KEY_R);
    }
    if (!found) return 0;
    press_minerals = minerals; press_gas = gas;
    press_free = free_unit_slots(); press_used = supply_used; press_larva = b[hall].larva;
    tap(KEY_A);
    return 1;
}

static void setup_match(int bot_race)
{
    guardian_test_reset();
    option_map = 0; option_human = R_ZERG; option_bots = 1;
    option_bot_races[0] = bot_race; match_limit = 100000;
    host_ticks = 0;
    assert(launch_match());
}

static void setup_tech(int hall)
{
    /* HOST setup: a finished Spawning Pool and Spire beside the Hatchery. */
    assert(spawn_build(SIDE, B_SPAWNING_POOL, b[hall].x + 100, b[hall].y, 0) >= 0);
    assert(spawn_build(SIDE, B_SPIRE, b[hall].x + 100, b[hall].y + 70, 0) >= 0);
    minerals = 5000; gas = 5000; b[hall].larva = 3;
}

/* ----------------------------------------------------------------- rules */

static void rule_price_larva_supply_timer_birth(void)
{
    setup_match(R_TER);
    int hall = own_hall(SIDE);
    assert(hall >= 0);
    setup_tech(hall);
    static const int products[] = { U_DRONE, U_ZERGLING, U_SCOURGE };
    for (unsigned k = 0; k < 3; k++) {
        int type = products[k];
        const Sc64RaceUnitRule *r = sc64_race_unit_rule(type);
        int n = sc64_z7_egg_product_count(type), duration = sc64_z7_egg_duration(type);
        b[hall].larva = 3; minerals = 5000; gas = 5000; /* HOST setup: refill */
        int larva = b[hall].larva, used = supply_used;
        int reserved = race_owner_reserved_supply(SIDE);
        int queue = b[hall].queue_count;
        int free0 = free_unit_slots();
        assert(order_unit(hall, type));
        int egg = egg_of(SIDE, type, -1);
        assert(egg >= 0);
        /* Price and larva are paid at once; no queue entry is made. */
        /* One update elapsed since the bank was read: income at most. */
        assert(minerals >= press_minerals - r->minerals && minerals <= press_minerals - r->minerals + 16);
        assert(gas == press_gas - r->gas);
        assert(b[hall].larva == press_larva - 1 && b[hall].queue_count == queue);
        /* The Egg is one live actor; a pair reserves supply for both bodies. */
        assert(free_unit_slots() == press_free - 1);
        assert(u[egg].raw_type == 36 && u[egg].maxhp == 200 && u[egg].hp == 200);
        assert(unit_slot_supply(&u[egg]) == r->supply * n);
        assert(supply_used == press_used + r->supply * n);
        assert(race_owner_reserved_supply(SIDE) == press_used + r->supply * n);
        assert(supply_used == race_owner_supply(SIDE));
        /* The ordering tap runs two updates; the timer started at the full duration. */
        assert(u[egg].work_timer <= duration && u[egg].work_timer >= duration - 3);
        int remaining = u[egg].work_timer;
        int born_at = host_ticks, supply_before = supply_used, owned = count_type(SIDE, type);
        int guard = 0;
        while (u[egg].alive && u[egg].type == SC64_EGG_ACTOR && guard++ < 2000) input(0);
        int took = (int)host_ticks - born_at;
        printf("%s egg: build %d frames -> hatched after %d ticks, %d bod%s\n",
               unit_name(type), duration, took, n, n == 1 ? "y" : "ies");
        assert(u[egg].alive && u[egg].type == type);
        assert(took >= remaining - 1 && took <= remaining + 1);
        assert(count_type(SIDE, type) == owned + n);
        assert(supply_used == supply_before);          /* reservation becomes the units */
        assert(supply_used == race_owner_supply(SIDE));
        assert(u[egg].hp > 0 && u[egg].hp <= u[egg].maxhp && u[egg].order == ORDER_IDLE);
    }
}

static void rule_cancel_refund_and_larva(void)
{
    setup_match(R_TER);
    int hall = own_hall(SIDE);
    assert(hall >= 0);
    setup_tech(hall);
    b[hall].larva = 2;
    int ore = minerals, vesp = gas, used = supply_used;
    assert(order_unit(hall, U_ZERGLING));
    int egg = egg_of(SIDE, U_ZERGLING, -1);
    assert(egg >= 0 && b[hall].larva == 1 && supply_used > used);
    tap(KEY_SELECT); assert(selected_building < 0);
    assert(select_unit(egg));
    int row = race_context_command_base(RACE_CONTEXT_EGG);
    assert(row < race_skirmish_command_count() && !strcmp(race_menu_label(row), "CANCEL EGG"));
    assert(menu_to(row));
    tap(KEY_A);
    assert(!u[egg].alive);
    /* The bank is whole again (ore may have grown from mining), the larva is
     * back, and the reserved supply is released. */
    assert(minerals >= ore && gas == vesp);
    assert(b[hall].larva == 2);
    assert(supply_used == used);
    assert(egg_of(SIDE, U_ZERGLING, -1) < 0);
    /* A full Hatchery cannot take a fourth larva; the next one is lost. */
    b[hall].larva = 3;
    assert(order_unit(hall, U_DRONE) && b[hall].larva == 2);
    int drone_egg = egg_of(SIDE, U_DRONE, -1);
    b[hall].larva = 3;
    tap(KEY_SELECT); assert(selected_building < 0);
    assert(select_unit(drone_egg));
    assert(menu_to(row));
    tap(KEY_A);
    assert(!u[drone_egg].alive && b[hall].larva == 3);
    puts("cancel: price refunded, larva returned, supply released");
}

static void rule_killed_egg(void)
{
    setup_match(R_TER);
    int hall = own_hall(SIDE);
    assert(hall >= 0);
    setup_tech(hall);
    assert(order_unit(hall, U_ZERGLING));
    int egg = egg_of(SIDE, U_ZERGLING, -1);
    assert(egg >= 0);
    int ore = minerals, larva = b[hall].larva, used = supply_used;
    int zerglings = count_type(SIDE, U_ZERGLING);
    assert(used > 0);
    int hits = 0;
    while (u[egg].alive && u[egg].type == SC64_EGG_ACTOR && hits < 40) {
        hit_target(egg, 30, U_MARINE, ENEMY);
        ++hits;
    }
    /* 200 HP, armor 10: a 30-point hit does 20. */
    assert(!u[egg].alive && hits >= 8 && hits <= 12);
    assert(supply_used == used - 2);                 /* the pair's supply is released */
    assert(supply_used == race_owner_supply(SIDE));
    assert(b[hall].larva == larva);                  /* no larva comes back */
    for (int t = 0; t < 40; t++) input(0);
    assert(minerals >= ore && minerals < ore + 80);  /* no refund (mining only) */
    assert(count_type(SIDE, U_ZERGLING) == zerglings);
    puts("killed Egg: nothing returned, supply released, no birth");
}

static void rule_caps(void)
{
    setup_match(R_TER);
    int hall = own_hall(SIDE);
    assert(hall >= 0);
    setup_tech(hall);
    /* Unit pool: fill all but two slots. A pair needs its Egg slot plus a
     * second body; with the last free slot it is refused unpaid. */
    int filler = 0;
    for (int i = 0; i < MAX_UNITS; i++) if (!u[i].alive && free_unit_slots() > 2) {
        u[i].alive = 1; u[i].owner = RESCUABLE; u[i].type = U_OVERLORD; ++filler;
        u[i].x = 4; u[i].y = 4; u[i].hp = u[i].maxhp = 100; u[i].order = ORDER_IDLE;
    }
    assert(free_unit_slots() == 2 && filler > 0);
    assert(race_queue_unit(SIDE, hall, U_ZERGLING));       /* Egg + second body */
    assert(free_unit_slots() == 1);
    int ore = minerals, larva = b[hall].larva, used = supply_used;
    assert(!race_queue_unit(SIDE, hall, U_ZERGLING));      /* both would need two more */
    assert(!race_queue_unit(SIDE, hall, U_DRONE));         /* the second body is committed */
    assert(minerals == ore && b[hall].larva == larva && supply_used == used);
    /* The pair still hatches into the reserved slots. */
    int egg = egg_of(SIDE, U_ZERGLING, -1);
    assert(egg >= 0);
    u[egg].work_timer = 1;
    assert(race_z7_egg_tick_unit(egg));
    assert(u[egg].type == U_ZERGLING && count_type(SIDE, U_ZERGLING) == 2);
    assert(free_unit_slots() == 0);

    /* Supply: the reservation counts against the cap until birth. */
    setup_match(R_TER);
    hall = own_hall(SIDE);
    setup_tech(hall);
    int n = 0;
    while (n < 60) {
        int cap = race_owner_supply_cap(SIDE), reserved = race_owner_reserved_supply(SIDE);
        b[hall].larva = 3; minerals = 5000;
        int ok = race_queue_unit(SIDE, hall, U_DRONE);
        if (!ok) { assert(reserved + 2 > cap); break; }
        assert(race_owner_reserved_supply(SIDE) <= cap);
        ++n;
    }
    assert(n > 0 && n < 60);
    puts("caps: unit pool and supply reservations hold");
}

static void rule_zerg_opponent(void)
{
    setup_match(R_ZERG);
    int hall = own_hall(ENEMY);
    assert(hall >= 0);
    int ever_egg = 0, max_eggs = 0, drones0 = count_type(ENEMY, U_DRONE);
    for (int t = 0; t < 6000 && !won && !lost; t++) {
        input(0);
        int eggs = 0;
        for (int i = 0; i < MAX_UNITS; i++)
            eggs += u[i].alive && u[i].owner == ENEMY && u[i].type == SC64_EGG_ACTOR;
        if (eggs > max_eggs) max_eggs = eggs;
        ever_egg |= eggs > 0;
        /* Held supply never exceeds the cap while the AI pays for Eggs. */
        assert(race_owner_reserved_supply(ENEMY) <= race_owner_supply_cap(ENEMY) + 0);
    }
    assert(ever_egg && count_type(ENEMY, U_DRONE) > drones0);
    printf("Zerg opponent: %d Eggs at once, Drones %d -> %d\n", max_eggs, drones0,
           count_type(ENEMY, U_DRONE));
}

/* ------------------------------------------------- persistence digests */

static unsigned long long digest = 1469598103934665603ull;
static void mix(unsigned long long v) { digest = (digest ^ v) * 1099511628211ull; }

static void run_digest(unsigned from, unsigned to)
{
    /* Eggs are tracked by slot; a hatch is recorded with its tick since the
     * orders and the bodies it made. */
    static int egg_slot[3];
    static int egg_type[3] = { U_DRONE, U_ZERGLING, U_SCOURGE };
    static int hatched[3];
    for (int k = 0; k < 3; k++) {
        egg_slot[k] = -1;
        for (int i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == SC64_EGG_ACTOR &&
                u[i].target == egg_type[k]) egg_slot[k] = i;
    }
    for (unsigned t = from; t < to; t++) {
        int before[3];
        for (int k = 0; k < 3; k++) before[k] = egg_slot[k] >= 0 && u[egg_slot[k]].alive &&
                                                 u[egg_slot[k]].type == SC64_EGG_ACTOR;
        input(0);
        for (int k = 0; k < 3; k++) {
            int now = egg_slot[k] >= 0 && u[egg_slot[k]].alive &&
                      u[egg_slot[k]].type == SC64_EGG_ACTOR;
            if (before[k] && !now && !hatched[k]) {
                hatched[k] = 1;
                mix(t + 1); mix((unsigned)egg_type[k]);
                for (int i = 0; i < MAX_UNITS; i++)
                    if (u[i].alive && u[i].owner == SIDE && u[i].type == egg_type[k]) {
                        mix((unsigned)i); mix((unsigned)u[i].x); mix((unsigned)u[i].y);
                        mix((unsigned)u[i].hp);
                    }
                printf("hatch %s at +%u\n", unit_name(egg_type[k]), t + 1);
            }
        }
    }
    mix((unsigned)supply_used);
}

static void order_all(void)
{
    int hall;
    setup_match(R_TER);
    hall = own_hall(SIDE);
    setup_tech(hall);
    /* One selection, then cycle the train choice and press A for each. */
    assert(select_building(hall));
    static const int products[3] = { U_DRONE, U_ZERGLING, U_SCOURGE };
    for (int k = 0; k < 3; k++) {
        int found = 0;
        for (int i = 0; i < 24 && !found; i++) { if (train_type == products[k]) found = 1; else tap(KEY_R); }
        assert(found);
        tap(KEY_A);
    }
    assert(egg_of(SIDE, U_DRONE, -1) >= 0 && egg_of(SIDE, U_ZERGLING, -1) >= 0 &&
           egg_of(SIDE, U_SCOURGE, -1) >= 0);
}

static int save_via_pause(void)
{
    tap(KEY_START);
    if (screen != SCREEN_PAUSE) return 0;
    tap(KEY_B);
    if ((pause_notice & 0xc0) != UI_NOTICE_SAVED) return 0;
    tap(KEY_START);
    return screen == SCREEN_PLAY;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "rules";
    if (!strcmp(mode, "rules")) {
        rule_price_larva_supply_timer_birth();
        rule_cancel_refund_and_larva();
        rule_killed_egg();
        rule_caps();
        rule_zerg_opponent();
        puts("shared Egg rules PASS");
        return 0;
    }
    if (!strcmp(mode, "full")) {
        order_all();
        run_digest(0, HORIZON);
        printf("DIGEST %016llx\n", digest);
        return 0;
    }
    if (!strcmp(mode, "prefix") && argc > 2) {
        FILE *f;
        static u8 image[32768];
        order_all();
        run_digest(0, SAVE_AT);
        assert(save_via_pause());
        guardian_test_snapshot(image);
        f = fopen(argv[2], "wb");
        assert(f && fwrite(image, 1, sizeof image, f) == sizeof image);
        fclose(f);
        puts("PREFIX saved");
        return 0;
    }
    if (!strcmp(mode, "resume") && argc > 2) {
        FILE *f = fopen(argv[2], "rb");
        static u8 image[32768];
        assert(f && fread(image, 1, sizeof image, f) == sizeof image);
        fclose(f);
        game_init();
        guardian_test_restore(image);
        prior_keys = 0;
        tap(KEY_L);                       /* title quick-load */
        assert(screen == SCREEN_PLAY && race_mode);
        assert(egg_of(SIDE, U_DRONE, -1) >= 0 && egg_of(SIDE, U_ZERGLING, -1) >= 0 &&
               egg_of(SIDE, U_SCOURGE, -1) >= 0);
        /* The save point is tick SAVE_AT; the quick-load tap ran one update. */
        run_digest(SAVE_AT + 1, HORIZON);
        printf("DIGEST %016llx\n", digest);
        return 0;
    }
    fprintf(stderr, "usage: %s rules|full|prefix FILE|resume FILE\n", argv[0]);
    return 2;
}
