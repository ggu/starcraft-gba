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

static void sparse_opening_and_save(int map, unsigned seats)
{
    gba_init(); game_init(); prior_keys = 0;
    tap(KEY_DOWN); tap(KEY_A);
    assert(race_front.page == SC_FRONT_SKIRMISH);
    if (map) tap(KEY_RIGHT);
    tap(KEY_DOWN);
    for (int step = 0; race_front.race != R_TER && step < 3; ++step) tap(KEY_RIGHT);
    assert(race_front.race == R_TER);
    while (race_front.row < SC_FRONT_BOT_FIRST_ROW) tap(KEY_DOWN);
    for (int seat = 0; seat < 3; ++seat, tap(KEY_DOWN)) {
        int choice = (seats & (1u << seat)) ?
            (seat == 1 ? R_TER : SC_FRONT_BOT_RANDOM) : SC_FRONT_BOT_CLOSED;
        for (int step = 0; race_front.bot_races[seat] != choice && step < 5; ++step) tap(KEY_RIGHT);
        assert(race_front.bot_races[seat] == choice);
    }
    assert(race_front.row == SC_FRONT_SKIRMISH_LAUNCH_ROW);
    tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_skirmish_map == map &&
           race_skirmish_slots == seats && race_bots == sc64_skirmish_seat_count(seats));
    int saved_races[3];
    for (int i = 0; i < 3; ++i) saved_races[i] = race_bot_races[i];
    unsigned order = 0;
#ifdef SC64_NATIVE_MELEE_STARTS
    if (map == SC64_MAP_BLOOD_BATH)
        order = race_bots == 1 ? SC64_SKIRMISH_BLOOD_BATH_SOLO_ORDER : 0x31u;
#endif
    assert(race_skirmish_start_order == order);
    for (int i = 0; i < race_bots; ++i) {
        int seat = sc64_skirmish_bot_seat(seats, i);
        assert(saved_races[i] >= 0 && saved_races[i] <= 2);
        if (seat == 1) assert(saved_races[i] == R_TER);
        check_owner(race_bot_owner_ids[i], saved_races[i]);
        check_seat(race_bot_owner_ids[i], order ? sc64_skirmish_start_base(order, i + 1) : seat + 1);
    }
    for (int i = race_bots; i < 3; ++i)
        assert(race_player[race_bot_owner_ids[i]] == -1);

    assert(race_save_game());
    assert(race_data.state[23] == (order ? sc64_skirmish_start_word(saved_races[2], seats, order) :
           sc64_skirmish_seat_word(saved_races[2], seats)));
    game_init(); prior_keys = 0;
    assert(race_load_game());
    assert(race_skirmish_map == map && race_skirmish_slots == seats &&
           race_skirmish_start_order == order && race_bots == sc64_skirmish_seat_count(seats));
    for (int i = 0; i < race_bots; ++i) {
        assert(race_bot_races[i] == saved_races[i]);
        int base = order ? sc64_skirmish_start_base(order, i + 1) : sc64_skirmish_bot_seat(seats, i) + 1;
        check_seat(race_bot_owner_ids[i], base);
        check_fallback(race_bot_owner_ids[i], base);
    }
    if (order) {
        tap(KEY_START); assert(screen == SCREEN_PAUSE);
        tap(KEY_R); assert(screen == SCREEN_PLAY && race_skirmish_start_order == order);
        check_seat(SIDE, sc64_skirmish_start_base(order, 0));
        for (int i = 0; i < race_bots; ++i)
            check_seat(race_bot_owner_ids[i], sc64_skirmish_start_base(order, i + 1));
    }
    race_frontend_reset();
    for (int i = 0, rank = 0; i < 3; ++i)
        assert(race_front.bot_races[i] == ((seats & (1u << i)) ?
               saved_races[rank++] : SC_FRONT_BOT_CLOSED));
}

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
    puts("Native Blood Bath draws survive SRAM load and restart with a changed clock seed");
#else
    dense_blood_bath_start_and_save(0, 0);
    puts("Default dense and sparse Blood Bath starts survive SRAM load and restart");
#endif
#endif
    return 0;
}
