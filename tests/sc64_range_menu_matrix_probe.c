/* SPDX-License-Identifier: GPL-3.0-or-later
 * Prepared-input host probe: real frontend input, ordinary skirmish startup,
 * 60 game updates and same-process save/load/save, for every lineup. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RANGE_WORK) || !defined(SC64_UNIFIED) || !defined(SC64_IMPORTED_SKIRMISH)
#error "Use an ON current production profile with imported skirmish"
#endif
static u16 previous_input;
static void input(u16 held) { game_update(held, held & (u16)~previous_input); previous_input = held; }
static void tap(u16 key) { input(key); input(0); }
static void row(int target)
{
    for (int n = 0; race_front.row != target && n < 16; ++n)
        tap(race_front.row < target ? KEY_DOWN : KEY_UP);
    assert(race_front.row == target);
}
static void field(int *value, int target)
{
    for (int n = 0; *value != target && n < 3; ++n) tap(KEY_RIGHT);
    assert(*value == target);
}
static int units(int owner)
{
    int n = 0; for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].owner == owner; return n;
}
static int buildings(int owner)
{
    int n = 0; for (int i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive && b[i].owner == owner; return n;
}
static void lineup(int map, int human, int bots, const int ai[3])
{
    const int owners[3] = {ENEMY, RESCUABLE, 4};
    game_init(); previous_input = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    tap(KEY_DOWN); tap(KEY_A);
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_SKIRMISH);
    row(0); field(&race_front.map, map);
    row(1); field(&race_front.race, human);
    row(2); field(&race_front.bots, bots);
    for (int i = 0; i < bots; ++i) { row(3 + i); field(&race_front.bot_races[i], ai[i]); }
    row(3 + bots); tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && !won && !lost);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
    assert(race_player[SIDE] == human && units(SIDE) && buildings(SIDE));
    if (map == 0) assert(race_cols == 64 && race_rows == 64);
    else assert(race_cols == sc64_skirmish_maps[map - 1].width && race_rows == sc64_skirmish_maps[map - 1].height);
    for (int i = 0; i < 3; ++i) {
        if (i < bots) assert(race_player[owners[i]] == ai[i] && units(owners[i]) && buildings(owners[i]));
        else assert(!units(owners[i]) && !buildings(owners[i]));
    }
    for (int tick = 0; tick < 60; ++tick) input(0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_save_game());
    RaceSave saved = race_data;
    assert(race_load_game());
    assert(screen == SCREEN_PLAY && race_mode && race_human == human && race_bots == bots && race_skirmish_map == map);
    for (int i = 0; i < bots; ++i) assert(race_bot_races[i] == ai[i]);
    assert(race_save_game());
    assert(!memcmp(&saved, &race_data, sizeof(saved)));
}
int main(void)
{
    int cases = 0; gba_init(); assert(SC64_FRONT_MAP_COUNT == 2);
    for (int map = 0; map < 2; ++map)
        for (int human = R_TER; human <= R_PROTOSS; ++human)
            for (int bots = 1; bots <= 3; ++bots) {
                int combinations = bots == 1 ? 3 : bots == 2 ? 9 : 27;
                for (int config = 0; config < combinations; ++config) {
                    int ai[3] = {R_TER, R_TER, R_TER}, n = config;
                    for (int i = 0; i < bots; ++i) { ai[i] = n % 3; n /= 3; }
                    lineup(map, human, bots, ai); ++cases;
                }
            }
    assert(cases == 234);
    printf("normal menu and same-process skirmish save matrix: %d lineups passed\n", cases);
    return 0;
}
