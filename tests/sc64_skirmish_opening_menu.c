/* Normal-menu skirmish openings in each product and every lobby combination. */
#ifndef SC64_IMPORTED_SKIRMISH
#define SC64_SKIRMISH_IMPORTED_COUNT 0
#endif
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <assert.h>

static void check_owner(int owner, int race)
{
    int workers=0, halls=0, extra=0, overlords=0;
    int worker=race==R_TER?U_SCV:race==R_ZERG?U_DRONE:U_PROBE;
    int hall=race==R_TER?B_COMMAND:race==R_ZERG?B_HATCHERY:B_NEXUS;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==owner) {
        if(u[i].type==worker) { ++workers; assert(u[i].order==ORDER_MINE); }
        if(u[i].type==U_OVERLORD) ++overlords;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==owner) {
        if(b[i].type==hall) ++halls;
        if(b[i].type==B_DEPOT || b[i].type==B_PYLON) ++extra;
    }
    assert(workers==4 && halls==1 && extra==0 && overlords==(race==R_ZERG));
    assert(race_owner_supply(owner)==8);
    assert(race_owner_supply_cap(owner)==(race==R_TER?20:18));
    assert((owner==SIDE?minerals:race_minerals[owner])==50);
    assert((owner==SIDE?gas:race_gas[owner])==0);
}

#ifdef SC64_IMPORTED_SKIRMISH
static void check_seat(int owner, int base)
{
    assert(race_ai_owner_base(owner) == base);
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) {
            assert(b[i].x == race_base_x[base] && b[i].y == race_base_y[base]);
            ++count;
        }
    assert(count == 1);
}

static void check_fallback(int owner, int base)
{
    int home = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) { home = i; break; }
    assert(home >= 0);
    b[home].alive = 0;
    RaceAiCensus census;
    race_ai_census(owner, &census);
    b[home].alive = 1;
    assert(census.home == -1 && census.base == base &&
           census.hx == race_base_x[base] && census.hy == race_base_y[base]);
}

#ifdef SC64_NATIVE_MELEE_STARTS
static void check_match_hostility(int melee, unsigned order)
{
    assert(race_skirmish_melee == (unsigned)melee);
    for (int i = 0; i < race_bots; ++i) {
        int owner = race_bot_owner_ids[i];
        assert(!owners_hostile(owner, owner) && !owners_hostile(owner, NEUTRAL));
        assert(owners_hostile(owner, SIDE) && owners_hostile(SIDE, owner));
        for (int j = 0; j < race_bots; ++j) if (i != j) {
            int other = race_bot_owner_ids[j];
            assert(owners_hostile(owner, other) == !melee);
            if (order) {
                RaceAiCensus census = {.owner = owner};
                int base = sc64_skirmish_start_base(order, j + 1);
                int human = sc64_skirmish_start_base(order, 0);
                int x = race_base_x[base], y = race_base_y[base], tx = -1, ty = -1;
                assert(race_ai_hostile_near(&census, x, y, 16) == !melee);
                assert(race_ai_target(&census, x, y, &tx, &ty));
                assert(tx == (melee ? race_base_x[human] : x));
                assert(ty == (melee ? race_base_y[human] : y));
            }
        }
    }
}

static void check_match_words(void)
{
    unsigned order = SC64_SKIRMISH_BLOOD_BATH_DENSE_ORDER;
    int race = -1;
    unsigned sparse = 99, decoded = 99, mode = 99;
    int melee = sc64_skirmish_melee_word(R_PROTOSS, 7, order);
    assert(!sc64_skirmish_read_start_word(1, 1, 3, melee, &race, &sparse, &decoded));
    assert(!sc64_skirmish_read_match_word(0, 1, 3, melee, &race, &sparse, &decoded, &mode));
    assert(!sc64_skirmish_read_match_word(1, 0, 3, melee, &race, &sparse, &decoded, &mode));
    assert(!sc64_skirmish_read_match_word(1, 1, 2, melee, &race, &sparse, &decoded, &mode));
    assert(!sc64_skirmish_read_match_word(1, 1, 3, melee | 0x2000, &race, &sparse, &decoded, &mode));
    assert(race == -1 && sparse == 99 && decoded == 99 && mode == 99);
    assert(sc64_skirmish_read_match_word(1, 1, 3, melee, &race, &sparse, &decoded, &mode));
    assert(race == R_PROTOSS && !sparse && decoded == order && mode == 1);
    int legacy = sc64_skirmish_start_word(R_PROTOSS, 7, order);
    assert(sc64_skirmish_read_match_word(1, 1, 3, legacy, &race, &sparse, &decoded, &mode));
    assert(race == R_PROTOSS && !sparse && decoded == order && mode == 0);
}
#endif

static void mixed_opening_and_save(int map, int human_choice, int expected_human,
                                   const int choices[3], unsigned order, const int *expected_races,
                                   int melee)
{
    unsigned seats = 0;
    for (int seat = 0; seat < 3; ++seat)
        if (choices[seat] != SC_FRONT_BOT_CLOSED) seats |= 1u << seat;
    int bots = sc64_skirmish_seat_count(seats);
    unsigned sparse = seats == sc64_skirmish_dense_seats(bots) ? 0 : seats;
#ifdef SC64_NATIVE_MELEE_STARTS
    gba_test_set_elapsed_milliseconds(0xf3ae);
#endif
    gba_init(); game_init(); prior_keys = 0;
    tap(KEY_DOWN); tap(KEY_A);
    assert(race_front.page == SC_FRONT_SKIRMISH);
    if (map) tap(KEY_RIGHT);
    tap(KEY_DOWN);
    for (int step = 0; race_front.race != human_choice && step < 4; ++step) tap(KEY_RIGHT);
    assert(race_front.race == human_choice);
    sc64_frontend_draw();
    while (race_front.row < SC_FRONT_BOT_FIRST_ROW) tap(KEY_DOWN);
    for (int seat = 0; seat < 3; ++seat, tap(KEY_DOWN)) {
        int choice = choices[seat];
        for (int step = 0; race_front.bot_races[seat] != choice && step < 5; ++step) tap(KEY_RIGHT);
        assert(race_front.bot_races[seat] == choice);
    }
#ifdef SC64_NATIVE_MELEE_STARTS
    assert(race_front.row == SC_FRONT_SKIRMISH_MODE_ROW);
    if (melee) tap(KEY_RIGHT);
    assert(race_front.skirmish_melee == (unsigned)melee);
    sc64_frontend_draw();
    tap(KEY_DOWN);
#else
    assert(!melee);
#endif
    assert(race_front.row == SC_FRONT_SKIRMISH_LAUNCH_ROW);
    tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_skirmish_map == map &&
           race_skirmish_slots == sparse && race_bots == bots && race_human == expected_human);
    check_owner(SIDE, expected_human);
    check_seat(SIDE, order ? sc64_skirmish_start_base(order, 0) : 0);
    int saved_races[3];
    for (int i = 0; i < 3; ++i) saved_races[i] = race_bot_races[i];
    assert(race_skirmish_start_order == order);
    for (int i = 0; i < race_bots; ++i) {
        int seat = sc64_skirmish_bot_seat(seats, i);
        assert(saved_races[i] >= 0 && saved_races[i] <= 2);
        if (choices[seat] != SC_FRONT_BOT_RANDOM) assert(saved_races[i] == choices[seat]);
        if (expected_races) assert(saved_races[i] == expected_races[i]);
        check_owner(race_bot_owner_ids[i], saved_races[i]);
        check_seat(race_bot_owner_ids[i], order ? sc64_skirmish_start_base(order, i + 1) : seat + 1);
    }
    for (int i = race_bots; i < 3; ++i)
        assert(race_player[race_bot_owner_ids[i]] == -1);

    assert(race_save_game());
    int word = order ? sc64_skirmish_start_word(saved_races[2], seats, order) :
                       sc64_skirmish_seat_word(saved_races[2], sparse);
#ifdef SC64_NATIVE_MELEE_STARTS
    check_match_hostility(melee, order);
    if (melee) word = sc64_skirmish_melee_word(saved_races[2], seats, order);
    gba_test_set_elapsed_milliseconds(0xf6e0);
#endif
    assert(race_data.state[23] == word);
    game_init(); prior_keys = 0;
    assert(race_load_game());
    assert(race_skirmish_map == map && race_skirmish_slots == sparse &&
           race_skirmish_start_order == order && race_bots == bots && race_human == expected_human);
#ifdef SC64_NATIVE_MELEE_STARTS
    check_match_hostility(melee, order);
#endif
    check_seat(SIDE, order ? sc64_skirmish_start_base(order, 0) : 0);
    for (int i = 0; i < race_bots; ++i) {
        assert(race_bot_races[i] == saved_races[i]);
        int base = order ? sc64_skirmish_start_base(order, i + 1) : sc64_skirmish_bot_seat(seats, i) + 1;
        check_seat(race_bot_owner_ids[i], base);
        check_fallback(race_bot_owner_ids[i], base);
    }
    if (order) {
        tap(KEY_START); assert(screen == SCREEN_PAUSE);
        tap(KEY_R); assert(screen == SCREEN_PLAY && race_skirmish_start_order == order &&
                           race_human == expected_human);
#ifdef SC64_NATIVE_MELEE_STARTS
        check_match_hostility(melee, order);
#endif
        check_owner(SIDE, expected_human);
        check_seat(SIDE, sc64_skirmish_start_base(order, 0));
        for (int i = 0; i < race_bots; ++i) {
            assert(race_bot_races[i] == saved_races[i]);
            check_owner(race_bot_owner_ids[i], saved_races[i]);
            check_seat(race_bot_owner_ids[i], sc64_skirmish_start_base(order, i + 1));
        }
    }
    race_frontend_reset();
    assert(race_front.race == expected_human);
#ifdef SC64_NATIVE_MELEE_STARTS
    assert(race_front.skirmish_melee == (unsigned)melee);
#endif
    for (int i = 0, rank = 0; i < 3; ++i)
        assert(race_front.bot_races[i] == ((seats & (1u << i)) ?
               saved_races[rank++] : SC_FRONT_BOT_CLOSED));
}

static void sparse_opening_and_save(int map, unsigned seats)
{
    int choices[3];
    for (int seat = 0; seat < 3; ++seat)
        choices[seat] = (seats & (1u << seat)) ?
            (seat == 1 ? R_TER : SC_FRONT_BOT_RANDOM) : SC_FRONT_BOT_CLOSED;
#ifdef SC64_NATIVE_MELEE_STARTS
    if (map == SC64_MAP_BLOOD_BATH) {
        const unsigned order_by_mask[] = {[2]=0x0c, [4]=0x0d, [5]=0x18, [6]=0x36};
        const int races[2] = {seats == 2 || seats == 6 ? R_TER : R_ZERG,
                              seats == 6 ? R_ZERG : R_TER};
        mixed_opening_and_save(map, R_TER, R_TER, choices, order_by_mask[seats], races, 0);
        return;
    }
#endif
    mixed_opening_and_save(map, R_TER, R_TER, choices, 0, NULL, 0);
}

#ifdef SC64_NATIVE_MELEE_STARTS
static void check_native_random_openings(void)
{
    const int sparse[] = {SC_FRONT_BOT_CLOSED, SC_FRONT_BOT_RANDOM, SC_FRONT_BOT_CLOSED};
    const int dense[] = {SC_FRONT_BOT_RANDOM, SC_FRONT_BOT_RANDOM, R_PROTOSS};
    const int races[] = {R_ZERG, R_TER, R_PROTOSS};
    const int joint_races[] = {R_TER, R_ZERG, R_PROTOSS};
    mixed_opening_and_save(SC64_MAP_BLOOD_BATH, R_TER, R_TER, sparse, 0x0d, races, 0);
    mixed_opening_and_save(SC64_MAP_BLOOD_BATH, R_TER, R_TER, sparse, 0x0d, races, 1);
    mixed_opening_and_save(SC64_MAP_BLOOD_BATH, R_TER, R_TER, dense, 0x63, races, 0);
    mixed_opening_and_save(SC64_MAP_BLOOD_BATH, R_TER, R_TER, dense, 0x63, races, 1);
    /* The load/restart seed in mixed_opening_and_save would instead roll Protoss. */
    mixed_opening_and_save(SC64_MAP_BLOOD_BATH, SC_FRONT_HUMAN_RANDOM, R_ZERG,
                           dense, 0xb4, joint_races, 0);

    Sc64Frontend picker;
    sc64_frontend_reset(&picker, 0, R_TER, 1, -1);
    picker.page = SC_FRONT_SKIRMISH;
    picker.map = SC64_MAP_BLOOD_BATH;
    picker.row = 1;
    sc64_frontend_update(&picker, KEY_RIGHT);
    assert(picker.race == R_PROTOSS);
    sc64_frontend_update(&picker, 0);
    sc64_frontend_update(&picker, KEY_RIGHT);
    assert(picker.race == SC_FRONT_HUMAN_RANDOM);
    sc64_frontend_update(&picker, 0);
    sc64_frontend_update(&picker, KEY_UP);
    picker.skirmish_melee = 1;
    sc64_frontend_update(&picker, KEY_LEFT);
    assert(picker.map == SC64_MAP_OUTPOST && picker.race == R_PROTOSS && !picker.skirmish_melee);
    sc64_frontend_update(&picker, KEY_DOWN);
    sc64_frontend_update(&picker, KEY_RIGHT);
    assert(picker.race == R_TER);

    Sc64Frontend malformed = {.map=SC64_MAP_BLOOD_BATH, .race=R_TER, .bots=1};
    int human = -1, result[3] = {-1, -2, -3};
    unsigned order = 0x123u;
    malformed.bot_races[0] = SC_FRONT_BOT_CLOSED;
    malformed.bot_races[1] = SC_FRONT_BOT_CLOSED;
    malformed.bot_races[2] = SC_FRONT_BOT_CLOSED;
    assert(!sc64_front_skirmish_native_resolve(&malformed, 0xf3ae, &human, result, &order));
    malformed.bot_races[1] = -1;
    assert(!sc64_front_skirmish_native_resolve(&malformed, 0xf3ae, &human, result, &order));
    malformed.bot_races[1] = SC_FRONT_BOT_RANDOM;
    malformed.bots = 2;
    assert(!sc64_front_skirmish_native_resolve(&malformed, 0xf3ae, &human, result, &order));
    malformed.bots = 1;
    malformed.map = SC64_MAP_OUTPOST;
    assert(!sc64_front_skirmish_native_resolve(&malformed, 0xf3ae, &human, result, &order));
    malformed.map = SC64_MAP_BLOOD_BATH;
    malformed.race = SC_FRONT_BOT_CLOSED;
    assert(!sc64_front_skirmish_native_resolve(&malformed, 0xf3ae, &human, result, &order));
    assert(human == -1 && result[0] == -1 && result[1] == -2 && result[2] == -3 && order == 0x123u);
}
#endif

static void dense_blood_bath_start_and_save(unsigned seed, unsigned order)
{
#ifdef SC64_NATIVE_MELEE_STARTS
    gba_test_set_elapsed_milliseconds(seed);
#else
    (void)seed;
#endif
    option_map = SC64_MAP_BLOOD_BATH;
    option_human = R_TER; option_bots = 3;
    option_bot_races[0] = R_ZERG; option_bot_races[1] = R_TER; option_bot_races[2] = R_PROTOSS;
    const int owners[] = {SIDE, ENEMY, RESCUABLE, 4};
    assert(launch_match() && !race_skirmish_slots && race_skirmish_start_order == order);
    assert(race_base_x[0] == 64 && race_base_y[0] == 120 &&
           race_base_x[1] == 880 && race_base_y[1] == 904 &&
           race_base_x[2] == 880 && race_base_y[2] == 120 &&
           race_base_x[3] == 144 && race_base_y[3] == 904);
    int human_base = order ? sc64_skirmish_start_base(order, 0) : 0;
    assert(cx == race_base_x[human_base] && cy == race_base_y[human_base]);
    for (int i = 0; i < 4; ++i) check_seat(owners[i], order ? sc64_skirmish_start_base(order, i) : i);
    assert(race_save_game());
    assert(race_data.state[23] == (order ? sc64_skirmish_start_word(R_PROTOSS, 7, order) : R_PROTOSS));
#ifdef SC64_NATIVE_MELEE_STARTS
    gba_test_set_elapsed_milliseconds(seed == 0xf3ae ? 0xf6e0 : 0xf3ae);
#endif
    game_init(); prior_keys = 0;
    assert(race_load_game() && !race_skirmish_slots && race_skirmish_start_order == order);
    for (int i = 0; i < 4; ++i) {
        int base = order ? sc64_skirmish_start_base(order, i) : i;
        check_seat(owners[i], base);
        if (i) check_fallback(owners[i], base);
    }
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    tap(KEY_R); assert(screen == SCREEN_PLAY && race_skirmish_start_order == order);
    assert(cx == race_base_x[human_base] && cy == race_base_y[human_base]);
    for (int i = 0; i < 4; ++i) check_seat(owners[i], order ? sc64_skirmish_start_base(order, i) : i);
}

#endif

int main(void)
{
#ifdef SC64_NATIVE_MELEE_STARTS
    gba_test_set_elapsed_milliseconds(0xf3ae);
#endif
    int cases=0;
    for(option_map=0;option_map<SC64_FRONT_MAP_COUNT;++option_map)
      for(option_human=R_TER;option_human<=R_PROTOSS;++option_human)
       for(option_bots=1;option_bots<=3;++option_bots) {
        int combinations=option_bots==1?3:option_bots==2?9:27;
        for(int c=0;c<combinations;++c) {
            int value=c;for(int i=0;i<option_bots;++i){option_bot_races[i]=value%3;value/=3;}
            gba_init();assert(launch_match());check_owner(SIDE,option_human);
            for(int i=0;i<option_bots;++i)check_owner(race_bot_owner_ids[i],option_bot_races[i]);
            ++cases;
        }
       }
    assert(cases==117 * SC64_FRONT_MAP_COUNT);printf("Normal-menu skirmish openings: %d configurations passed\n",cases);
#ifdef SC64_IMPORTED_SKIRMISH
    /* A dense launch keeps the former slot field, including the otherwise
     * inactive third race, and still round-trips with implicit seating. */
    option_map = 0; option_human = R_TER; option_bots = 1; option_bot_races[0] = R_ZERG;
    assert(launch_match() && !race_skirmish_slots && race_save_game());
    assert(race_data.state[23] == race_bot_races[2]);
    game_init();
    assert(race_load_game() && !race_skirmish_slots && race_bots == 1);
    for (int map = 0; map < SC64_FRONT_MAP_COUNT; ++map)
        for (unsigned seats = 1; seats <= SC64_SKIRMISH_SEAT_MASK; ++seats)
            if (sc64_skirmish_sparse_valid(sc64_skirmish_seat_count(seats), seats))
                sparse_opening_and_save(map, seats);
#ifdef SC64_NATIVE_MELEE_STARTS
    dense_blood_bath_start_and_save(0xf3ae, SC64_SKIRMISH_BLOOD_BATH_DENSE_ORDER);
    dense_blood_bath_start_and_save(0xf6e0, 0x9cu);
    check_native_random_openings();
    check_match_words();
    puts("Native Blood Bath races, positions and CPU diplomacy survive save and restart");
#else
    dense_blood_bath_start_and_save(0, 0);
    puts("Default dense and sparse Blood Bath starts survive SRAM load and restart");
#endif
#endif
    return 0;
}
