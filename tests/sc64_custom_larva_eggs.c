/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Custom Game Larva and Eggs on the real engine, for a Zerg human and a Zerg
 * computer opponent. The human's orders, cancel, save and load go through
 * game_update() as controller input. Three things are HOST fixtures and are
 * labelled where they happen: bank and Larva top-ups (so a case need not wait
 * for income), prerequisite structures placed complete, and an Egg killed
 * through the engine's hit_target death path.
 *
 *   lifecycle            every rule below, in one process
 *   save PATH [MAP BOTS] play to several Eggs on both sides, pause-save,
 *                        write the SRAM image to PATH, continue, print digest.
 *                        MAP and BOTS (default 0 and 1) choose the lobby: 1 3
 *                        is Blood Bath against three Zerg opponents, whose
 *                        record packs its seats and start order differently
 *   resume PATH          fresh process: read PATH, HOME load, continue the
 *                        same number of updates, print digest
 *   old-write PATH       (built with the capability off) the serial queue
 *                        mid-production, pause-save, write the SRAM image
 *   old-fixture PATH     the same record from a build with the capability,
 *                        for products whose generated tables cannot be built
 *                        without it: the queue entries are placed as the
 *                        earlier producer stored them
 *   old-read PATH        load that record: the queued orders still complete,
 *                        and every new order is an Egg
 *   old-reject PATH      (capability off) a record holding Eggs is refused
 */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <unistd.h>

extern int sc64_test_sram_transfer(const char *path, int write_mode);

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s (frame %d)\n", __FILE__, __LINE__, #c, frame); exit(1); } } while (0)

static int is_egg(const Unit *q)
{
#ifdef SC64_EGG_RUNTIME
    return q->alive && q->type == SC64_EGG_ACTOR;
#else
    (void)q;
    return 0;
#endif
}
#define CONTINUE_UPDATES 900
static char lifecycle_sram[512];
#define LIFECYCLE_SRAM lifecycle_sram

static int bot(void) { return race_bot_owner_ids[0]; }

static int hall_of(int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && !b[i].under_construction &&
            sc64_race_is_zerg_townhall(b[i].type)) return i;
    return -1;
}

static int eggs_of(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].owner == owner && is_egg(&u[i]);
    return n;
}

static int units_of(int owner, int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].owner == owner && u[i].type == type;
    return n;
}

static int queued_of(int owner)
{
    int n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner) n += b[i].queue_count;
    return n;
}

/* The newest Egg of an owner: the one with the most time left. */
static int newest_egg(int owner)
{
    int best = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].owner == owner && is_egg(&u[i]) &&
            (best < 0 || u[i].work_timer > u[best].work_timer)) best = i;
    return best;
}

static void wait_updates(int n) { for (int i = 0; i < n; ++i) input(0); }

/* Neutral input until the match clock reads the given update. */
static void wait_until_frame(int target)
{
    for (int i = 0; i < 400000 && frame < target; ++i) input(0);
    CHECK(frame == target && screen == SCREEN_PLAY);
}

static int lobby_map, lobby_bots = 1;

static void start(int bot_race)
{
    option_map = lobby_map; option_human = R_ZERG; option_bots = lobby_bots;
    for (int i = 0; i < 3; ++i) option_bot_races[i] = bot_race;
    match_limit = 400000;
    CHECK(launch_match());
    CHECK(screen == SCREEN_PLAY && race_mode);
}

/* The human's own controls: the selected hall is the Larva source, R picks
 * the product and A orders one. */
static int pick_product(int hall, int type)
{
    if (selected_building != hall && !select_building(hall)) return 0;
    for (int i = 0; i < 24 && train_type != type; ++i) tap(KEY_R);
    return train_type == type && selected_building == hall;
}

static int order(int hall, int type)
{
    int before = eggs_of(SIDE) + queued_of(SIDE);
    if (!pick_product(hall, type)) return 0;
    tap(KEY_A);
    return eggs_of(SIDE) + queued_of(SIDE) == before + 1;
}

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

static void pause_save(void)
{
    if (menu_open) tap(KEY_B);
    tap(KEY_SELECT);
    tap(KEY_START); CHECK(screen == SCREEN_PAUSE);
    tap(KEY_B); CHECK((pause_notice & 0xc0) == UI_NOTICE_SAVED);
    tap(KEY_START); CHECK(screen == SCREEN_PLAY);
}

static uint32_t digest(void)
{
    uint32_t h = 2166136261u;
#define MIX(v) do { h ^= (uint32_t)(v); h *= 16777619u; } while (0)
    MIX(frame); MIX(seed); MIX(minerals); MIX(gas); MIX(supply_used); MIX(supply_cap);
    for (int i = 0; i < 5; ++i) { MIX(race_owner_minerals(i)); MIX(race_owner_gas(i)); MIX(race_owner_supply(i)); }
    for (int i = 0; i < MAX_MINERALS; ++i) { MIX(m[i].x); MIX(m[i].y); MIX(m[i].amount); }
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->raw_type); MIX(q->x); MIX(q->y); MIX(q->hp); MIX(q->maxhp);
        MIX(q->order); MIX(q->tx); MIX(q->ty); MIX(q->carry); MIX(q->target); MIX(q->work_timer);
        MIX(q->shield); MIX(q->cd); MIX(q->build);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y); MIX(q->hp); MIX(q->maxhp);
        MIX(q->under_construction); MIX(q->build_progress); MIX(q->larva); MIX(q->larva_timer);
        MIX(q->queue_count); MIX(q->queue_progress);
        for (int j = 0; j < q->queue_count && j < 5; ++j) MIX(q->queue[(q->queue_head + j) % 5]);
    }
#undef MIX
    return h;
}

static void print_state(const char *kind)
{
    printf("{\"kind\":\"%s\",\"frame\":%d,\"digest\":\"%08x\",\"human_eggs\":%d,\"bot_eggs\":%d,"
           "\"human_queued\":%d,\"bot_queued\":%d,\"human_larva\":%d,\"bot_larva\":%d}\n",
           kind, frame, digest(), eggs_of(SIDE), eggs_of(bot()), queued_of(SIDE), queued_of(bot()),
           hall_of(SIDE) >= 0 ? b[hall_of(SIDE)].larva : -1, hall_of(bot()) >= 0 ? b[hall_of(bot())].larva : -1);
}

#ifdef SC64_CUSTOM_EGGS_ACTIVE
/* HOST fixture: a prerequisite structure placed complete beside the hall. */
static int place_complete(int owner, int type, int hall, int dx, int dy)
{
    int slot = spawn_build(owner, type, b[hall].x + dx, b[hall].y + dy, 0);
    CHECK(slot >= 0);
    invalidate_path_grid();
    return slot;
}

static void lifecycle(void)
{
    const Sc64RaceUnitRule *drone = sc64_race_unit_rule(U_DRONE);
    const Sc64RaceUnitRule *overlord = sc64_race_unit_rule(U_OVERLORD);
    const Sc64RaceUnitRule *ling = sc64_race_unit_rule(U_ZERGLING);
    const Sc64RaceUnitRule *scourge = sc64_race_unit_rule(U_SCOURGE);
    int hall, bank, used, eggs[3], drones, born_at[3], ordered_at[3], n;
    start(R_ZERG);
    hall = hall_of(SIDE);
    CHECK(hall >= 0 && SC64_EGG_CONTEXT());

    /* The match's starting hall holds three Larva (OpenBW 21171-21172 and a
     * zero timer); none of its orders is a queue entry. */
    CHECK(b[hall].larva == SC64_LARVA_CAP && b[hall_of(bot())].larva == SC64_LARVA_CAP);

    /* Every Zerg product the Custom Game catalog trains from Larva is an Egg
     * product, so nothing can fall back to the serial queue. */
    n = 0;
    for (unsigned p = 0; p < SC64_SKIRMISH_UNIT_COUNT; ++p) {
        const Sc64SkirmishRecord *r = &sc64_skirmish_unit_catalog[p];
        int type = sc64_race_unit_from_raw(r->raw_id);
        if (r->kind != SC64_SK_TRAIN || r->race != R_ZERG || r->producer_raw != 35 || type < 0 ||
            !sc64_race_unit_rule(type) || race_skirmish_product_producer(type) == 255) continue;
        CHECK(sc64_z7_egg_product_valid(type));
        ++n;
    }
    CHECK(n >= 6);

    /* Order: three Drones from one hall, through the controls. Each order
     * takes one Larva, its price and its supply at once, and is its own Egg. */
    minerals = 600; /* HOST fixture: bank */
    bank = minerals; used = supply_used; drones = units_of(SIDE, U_DRONE);
    for (int k = 0; k < 3; ++k) {
        CHECK(order(hall, U_DRONE));
        eggs[k] = newest_egg(SIDE); born_at[k] = -1;
        CHECK(eggs[k] >= 0 && u[eggs[k]].target == U_DRONE && u[eggs[k]].raw_type == 36);
        /* The key's press and release are two updates the Egg already ran. */
        CHECK(u[eggs[k]].work_timer >= sc64_z7_egg_duration(U_DRONE) - 2 &&
              u[eggs[k]].work_timer <= sc64_z7_egg_duration(U_DRONE));
        ordered_at[k] = frame + u[eggs[k]].work_timer; /* the update it must hatch on */
        CHECK(u[eggs[k]].hp == 200 && u[eggs[k]].maxhp == 200 && u[eggs[k]].armor == 10);
        CHECK(b[hall].larva == SC64_LARVA_CAP - 1 - k && b[hall].queue_count == 0);
        CHECK(minerals == bank - (k + 1) * drone->minerals);
        CHECK(supply_used == used + (k + 1) * drone->supply);
    }
    CHECK(eggs[0] != eggs[1] && eggs[1] != eggs[2] && eggs[0] != eggs[2]);
    CHECK(u[eggs[0]].x != u[eggs[1]].x && u[eggs[1]].x != u[eggs[2]].x && u[eggs[0]].x != u[eggs[2]].x);
    /* No Larva left: the same control orders nothing and charges nothing. */
    bank = minerals;
    CHECK(!order(hall, U_DRONE) && minerals == bank && eggs_of(SIDE) == 3 && b[hall].larva == 0);

    /* Simultaneous: all three are in progress together and each counts down
     * every update on its own timer; each hatches on exactly the update its
     * own timer runs out, sc64_z7_egg_duration() updates after its order. */
    CHECK(u[eggs[0]].work_timer > 0 && u[eggs[1]].work_timer > 0 && u[eggs[2]].work_timer > 0);
    {
        int t0 = u[eggs[0]].work_timer, t1 = u[eggs[1]].work_timer, t2 = u[eggs[2]].work_timer;
        input(0);
        CHECK(u[eggs[0]].work_timer == t0 - 1 && u[eggs[1]].work_timer == t1 - 1 &&
              u[eggs[2]].work_timer == t2 - 1);
    }
    used = supply_used;
    for (int t = 0; t < 2000 && (born_at[0] < 0 || born_at[1] < 0 || born_at[2] < 0); ++t) {
        input(0);
        for (int k = 0; k < 3; ++k)
            if (born_at[k] < 0 && u[eggs[k]].alive && u[eggs[k]].type == U_DRONE) born_at[k] = frame;
    }
    for (int k = 0; k < 3; ++k) {
        CHECK(born_at[k] > 0);
        CHECK(born_at[k] == ordered_at[k]);
    }
    CHECK(born_at[2] - born_at[0] < sc64_z7_egg_duration(U_DRONE) / 4);
    /* Each newborn stands where a ground unit can walk away. */
    for (int k = 0; k < 3; ++k) CHECK(race_egg_exit_open(u[eggs[k]].x, u[eggs[k]].y));
    CHECK(units_of(SIDE, U_DRONE) == drones + 3 && eggs_of(SIDE) == 0 && supply_used == used);
    printf("order and simultaneous hatch: three Drone Eggs born at %d, %d, %d\n",
           born_at[0], born_at[1], born_at[2]);

    /* Larva: one more every SC64_LARVA_PERIOD updates while below the cap. */
    {
        int larva = b[hall].larva, timer = b[hall].larva_timer;
        CHECK(larva < SC64_LARVA_CAP);
        wait_updates(SC64_LARVA_PERIOD - timer - 1);
        CHECK(b[hall].larva == larva);
        input(0);
        CHECK(b[hall].larva == larva + 1);
        wait_updates(SC64_LARVA_PERIOD * (SC64_LARVA_CAP + 1));
        CHECK(b[hall].larva == SC64_LARVA_CAP);
    }
    printf("larva: one per %d updates up to %d\n", SC64_LARVA_PERIOD, SC64_LARVA_CAP);

    /* Pairs: one Zergling Egg holds both Zerglings' supply from the order and
     * hatches two; one Scourge Egg hatches two. */
    (void)place_complete(SIDE, B_SPAWNING_POOL, hall, 96, -40);   /* HOST fixture */
    (void)place_complete(SIDE, B_SPIRE, hall, 96, 40);            /* HOST fixture */
    CHECK(spawn_unit(SIDE, U_OVERLORD, b[hall].x - 30, b[hall].y - 60) >= 0); /* HOST fixture: supply */
    input(0);
    minerals = 600; gas = 300;                                   /* HOST fixture: bank */
    bank = minerals; used = supply_used;
    CHECK(order(hall, U_ZERGLING));
    eggs[0] = newest_egg(SIDE);
    CHECK(u[eggs[0]].target == U_ZERGLING && minerals == bank - ling->minerals);
    CHECK(supply_used == used + 2 * ling->supply);
    n = units_of(SIDE, U_ZERGLING);
    wait_updates(sc64_z7_egg_duration(U_ZERGLING) + 4);
    CHECK(units_of(SIDE, U_ZERGLING) == n + 2 && supply_used == used + 2 * ling->supply);
    bank = minerals; used = supply_used;
    {
        int vespene = gas;
        CHECK(order(hall, U_SCOURGE));
        eggs[0] = newest_egg(SIDE);
        CHECK(u[eggs[0]].target == U_SCOURGE && minerals == bank - scourge->minerals &&
              gas == vespene - scourge->gas && supply_used == used + 2 * scourge->supply);
    }
    n = units_of(SIDE, U_SCOURGE);
    wait_updates(sc64_z7_egg_duration(U_SCOURGE) + 4);
    CHECK(units_of(SIDE, U_SCOURGE) == n + 2);
    printf("pairs: a Zergling Egg and a Scourge Egg each hatched two\n");

    /* Cancel: the whole price back, the Egg and its Larva gone, its supply
     * released (OpenBW cancel_building_unit, 2383-2427). */
    wait_updates(SC64_LARVA_PERIOD * SC64_LARVA_CAP);
    minerals = 600; gas = 300;                                   /* HOST fixture: bank */
    CHECK(order(hall, U_SCOURGE));
    eggs[0] = newest_egg(SIDE);
    {
        int larva = b[hall].larva, vespene = gas, row;
        bank = minerals; used = supply_used;
        tap(KEY_SELECT);
        CHECK(select_unit(eggs[0]));
        row = race_context_command_base(RACE_CONTEXT_EGG);
        CHECK(row < race_skirmish_command_count() && !strcmp(race_menu_label(row), "CANCEL EGG"));
        CHECK(menu_to(row));
        tap(KEY_A);
        CHECK(!u[eggs[0]].alive && minerals == bank + scourge->minerals && gas == vespene + scourge->gas);
        CHECK(supply_used == used - 2 * scourge->supply && b[hall].larva <= larva + 1);
        n = units_of(SIDE, U_SCOURGE);
        wait_updates(sc64_z7_egg_duration(U_SCOURGE) + 4);
        CHECK(units_of(SIDE, U_SCOURGE) == n);
    }
    printf("cancel: full refund, no unit\n");

    /* Death: a killed Egg is the unit lost. Nothing is refunded and nothing
     * hatches. */
    minerals = 600;                                              /* HOST fixture: bank */
    CHECK(order(hall, U_DRONE));
    eggs[0] = newest_egg(SIDE);
    bank = minerals; used = supply_used; drones = units_of(SIDE, U_DRONE);
    hit_target(eggs[0], 100000, U_MARINE, ENEMY);                /* HOST fixture: the kill */
    input(0);
    CHECK(!is_egg(&u[eggs[0]]));
    CHECK(eggs_of(SIDE) == 0 && supply_used == used - drone->supply);
    /* Workers keep delivering; one update's cargo is far below the price. */
    CHECK(minerals >= bank && minerals < bank + drone->minerals);
    wait_updates(sc64_z7_egg_duration(U_DRONE) + 4);
    CHECK(units_of(SIDE, U_DRONE) == drones);
    printf("death: the unit is lost, no refund\n");

    /* Saves: after a cancel and a death, with an Egg in progress, the record
     * is written, and it is a Custom Game record. The same rows in a campaign
     * record are refused. */
    minerals = 600; b[hall].larva = SC64_LARVA_CAP;              /* HOST fixture */
    CHECK(order(hall, U_DRONE));
    pause_save();
    CHECK(sc64_test_sram_transfer(LIFECYCLE_SRAM, 1));
    {
        /* Load it in this process, through HOME, and find the Eggs again.
         * The loader leaves the record it read and checked in race_data:
         * the same Egg rows marked as a campaign record are refused. */
        static RaceSave copy;
        int frame_saved = frame, eggs_saved = eggs_of(SIDE);
        unsigned w, h;
        CHECK(sc64_test_sram_transfer(LIFECYCLE_SRAM, 0));
        game_init(); prior_keys = 0;
        tap(KEY_L);
        CHECK(screen == SCREEN_PLAY && race_mode && eggs_of(SIDE) == eggs_saved && frame >= frame_saved - 4);
        remove(LIFECYCLE_SRAM);
        w = (unsigned)race_cols * 16u; h = (unsigned)race_rows * 16u;
        memcpy(&copy, &race_data, sizeof copy);
        {
            Unit row0;
            /* Some products keep race_data transformed after a load; the
             * check needs the plain row bank. */
            if (copy.state[0] == 1 && race_save_decode_unit(&copy, 0, w, h, &row0)) {
                CHECK(race_custom_egg_rows_valid(&copy, w, h));
                copy.state[0] = 0;
#ifdef SC64_Z7_EGG_PRODUCTION
                copy.state[2] = SC64_Z7_GLOBAL_ID + 1;
#endif
                CHECK(!race_custom_egg_rows_valid(&copy, w, h));
                printf("saves: the same Egg rows in a campaign record are refused\n");
            } else printf("saves: campaign-record refusal not checked here (loaded record is not a plain row bank)\n");
        }
    }
    printf("saves: written after cancel and death and loaded\n");

    /* Supply at order time. With no supply left a Drone order is refused and
     * costs nothing; an Overlord, which needs none, is still accepted. */
    minerals = 5000;                                             /* HOST fixture: bank */
    for (int k = 0; k < 64; ++k) {
        int cap = race_owner_supply_cap(SIDE), reserved = race_owner_reserved_supply(SIDE);
        b[hall].larva = SC64_LARVA_CAP;                          /* HOST fixture: Larva */
        bank = minerals;
        race_refusal = 0;
        if (race_queue_unit(SIDE, hall, U_DRONE)) {
            CHECK(reserved + drone->supply <= cap && minerals == bank - drone->minerals);
            continue;
        }
        CHECK(reserved + drone->supply > cap && race_refusal == 2 && minerals == bank);
        CHECK(b[hall].larva == SC64_LARVA_CAP);
        n = eggs_of(SIDE);
        CHECK(race_queue_unit(SIDE, hall, U_OVERLORD) && eggs_of(SIDE) == n + 1);
        CHECK(minerals == bank - overlord->minerals);
        break;
    }
    CHECK(race_owner_reserved_supply(SIDE) + drone->supply > race_owner_supply_cap(SIDE));
    printf("supply: refused at the cap at order time; an Overlord is still accepted\n");

    /* A Hatchery finished during the match starts with one Larva, not three. */
    {
        int site = spawn_build(SIDE, B_HATCHERY, b[hall].x - 120 > 40 ? b[hall].x - 120 : b[hall].x + 160,
                               b[hall].y, 1);                   /* HOST fixture */
        CHECK(site >= 0);
        race_complete_building(site);
        CHECK(!b[site].under_construction && b[site].larva == 1 && b[site].larva_timer == 0);
    }
    printf("new Hatchery: one Larva at completion\n");

    /* The computer opponent. Its halls never hold a queued unit, it has
     * several Eggs at once, and what it orders is born. */
    {
        int foe = bot(), most = 0, born = units_of(foe, U_DRONE);
        for (int t = 0; t < 9000 && screen == SCREEN_PLAY && !won && !lost; ++t) {
            input(0);
            CHECK(queued_of(foe) == 0 && queued_of(SIDE) == 0);
            if (eggs_of(foe) > most) most = eggs_of(foe);
        }
        CHECK(most >= 2 && units_of(foe, U_DRONE) + units_of(foe, U_ZERGLING) > born);
        printf("computer opponent: no queued unit in %d updates, up to %d Eggs at once\n", 9000, most);
    }
    puts("custom larva and eggs lifecycle: PASS");
}

/* Both sides with Eggs in progress and Larva timers running. */
static void play_to_eggs(void)
{
    int hall, foe;
    start(R_ZERG);
    hall = hall_of(SIDE); foe = bot();
    CHECK(pick_product(hall, U_OVERLORD));
    /* Wait, with the hall selected, until the opponent has two Eggs at once
     * that both still have time left. */
    for (int t = 0; t < 30000; ++t) {
        int young = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            young += u[i].owner == foe && is_egg(&u[i]) && u[i].work_timer > 150;
        if (young >= 2) break;
        input(0);
        CHECK(screen == SCREEN_PLAY && !won && !lost);
    }
    CHECK(eggs_of(foe) >= 2);
    minerals = 400;                                              /* HOST fixture: bank */
    CHECK(order(hall, U_OVERLORD));
    wait_updates(40);
    CHECK(order(hall, U_DRONE));
    wait_updates(25);
    CHECK(order(hall, U_DRONE));
    wait_updates(60);
    CHECK(eggs_of(SIDE) == 3 && eggs_of(foe) >= 2);
    CHECK(b[hall].larva == 0 && b[hall].larva_timer > 0 && b[hall].larva_timer < SC64_LARVA_PERIOD);
}
#endif

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "", *path = argc > 2 ? argv[2] : 0;
    setvbuf(stdout, 0, _IOLBF, 0);
#ifdef SC64_CUSTOM_EGGS_ACTIVE
    if (!strcmp(mode, "lifecycle")) {
        const char *dir = getenv("TMPDIR");
        snprintf(lifecycle_sram, sizeof lifecycle_sram, "%s/sc64-custom-larva-eggs-%d.sav",
                 dir && *dir ? dir : "/tmp", (int)getpid());
        lifecycle(); return 0;
    }
    if (!strcmp(mode, "save") && path) {
        if (argc > 4) { lobby_map = atoi(argv[3]); lobby_bots = atoi(argv[4]); }
        CHECK(lobby_map >= 0 && lobby_map <= 1 && lobby_bots >= 1 && lobby_bots <= 3);
        play_to_eggs();
        pause_save();
        print_state("saved");
        CHECK(sc64_test_sram_transfer(path, 1));
        wait_until_frame(frame + CONTINUE_UPDATES);
        print_state("continued");
        return 0;
    }
    if (!strcmp(mode, "old-fixture") && path) {
        static const int orders[3] = { U_OVERLORD, U_DRONE, U_DRONE };
        int hall;
        start(R_TER);
        hall = hall_of(SIDE);
        CHECK(hall >= 0 && b[hall].larva == SC64_LARVA_CAP && !b[hall].queue_count);
        minerals = 400;
        /* HOST fixture: what the serial producer stored for three paid
         * orders. The record layout is the one earlier builds write. */
        for (int k = 0; k < 3; ++k) {
            const Sc64RaceUnitRule *r = sc64_race_unit_rule(orders[k]);
            b[hall].queue[(b[hall].queue_head + b[hall].queue_count++) % 5] = orders[k];
            --b[hall].larva;
            minerals -= r->minerals;
        }
        wait_updates(120);
        CHECK(queued_of(SIDE) == 3 && eggs_of(SIDE) == 0 && eggs_of(bot()) == 0);
        CHECK(select_building(hall));
        pause_save();
        print_state("old-saved");
        CHECK(sc64_test_sram_transfer(path, 1));
        return 0;
    }
    if ((!strcmp(mode, "resume") || !strcmp(mode, "old-read")) && path) {
        int old = !strcmp(mode, "old-read"), hall, pending, units;
        CHECK(sc64_test_sram_transfer(path, 0));
        option_map = 0; match_limit = 400000;
        game_init(); prior_keys = 0;
        tap(KEY_L);
        CHECK(screen == SCREEN_PLAY && race_mode);
        print_state(old ? "old-loaded" : "loaded");
        if (!old) {
            wait_until_frame(frame + CONTINUE_UPDATES);
            print_state("continued");
            return 0;
        }
        /* The record was written by the serial queue. Its queued orders are
         * still there and still complete; no Egg is in it. */
        hall = hall_of(SIDE); pending = queued_of(SIDE);
        CHECK(hall >= 0 && pending >= 2 && eggs_of(SIDE) == 0 && eggs_of(bot()) == 0);
        units = units_of(SIDE, U_DRONE) + units_of(SIDE, U_OVERLORD);
        /* A new order joins nothing: it is an Egg beside the old queue. */
        minerals = 400;                                          /* HOST fixture: bank */
        b[hall].larva = 1;                                       /* HOST fixture: Larva */
        CHECK(pick_product(hall, U_DRONE));
        tap(KEY_A);
        CHECK(eggs_of(SIDE) == 1 && queued_of(SIDE) == pending);
        for (int t = 0; t < 4000 && (queued_of(SIDE) || eggs_of(SIDE)); ++t) input(0);
        CHECK(queued_of(SIDE) == 0 && eggs_of(SIDE) == 0);
        CHECK(units_of(SIDE, U_DRONE) + units_of(SIDE, U_OVERLORD) == units + pending + 1);
        print_state("old-drained");
        puts("old save: loaded, queued orders completed, new order was an Egg: PASS");
        return 0;
    }
#else
    if (!strcmp(mode, "old-write") && path) {
        int hall;
        start(R_ZERG);
        hall = hall_of(SIDE);
        minerals = 400;                                          /* HOST fixture: bank */
        CHECK(order(hall, U_OVERLORD) && order(hall, U_DRONE) && order(hall, U_DRONE));
        CHECK(queued_of(SIDE) == 3 && eggs_of(SIDE) == 0);
        wait_updates(120);
        pause_save();
        print_state("old-saved");
        CHECK(sc64_test_sram_transfer(path, 1));
        return 0;
    }
    /* A build without the capability must refuse a record that holds Eggs
     * and leave the game where it was. */
    if (!strcmp(mode, "old-reject") && path) {
        CHECK(sc64_test_sram_transfer(path, 0));
        game_init(); prior_keys = 0;
        tap(KEY_L);
        CHECK(screen != SCREEN_PLAY && !race_mode);
        puts("a build without the capability refuses a record with Eggs: PASS");
        return 0;
    }
#endif
    fprintf(stderr, "usage: %s lifecycle | save PATH [MAP BOTS] | resume PATH | old-write PATH | old-fixture PATH | "
                    "old-read PATH | old-reject PATH\n", argv[0]);
    return 2;
}
