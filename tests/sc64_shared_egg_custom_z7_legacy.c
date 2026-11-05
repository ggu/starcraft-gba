/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only constructed Z7 producer; exercises the ordinary legacy envelope. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
static unsigned held_keys;

static void press(unsigned key)
{
    game_update((u16)key, (u16)(key & ~held_keys));
    held_keys = key;
    game_update(0, 0);
    held_keys = 0;
}

static void launch_z7(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                            SC64_Z7_GLOBAL_ID);
    assert(ordinal == 6 && race_front.page == SC_FRONT_HOME);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z7_GLOBAL_ID);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && race_z7_active() &&
           race_campaign.mission == SC64_Z7_GLOBAL_ID);
    assert(SC64_RACE_SAVE_GENERATION ==
#ifdef SC64_Z7_DERIVED_SAVE
           38u
#else
           36u
#endif
    );
}

static int add_source_hatchery(int owner, int raw_owner)
{
    int slot = spawn_build(owner, B_HATCHERY, owner == SIDE ? 420 : 1810,
                           owner == SIDE ? 420 : 1820, 0);
    assert(slot >= 0);
    b[slot].raw_owner = raw_owner;
    b[slot].raw_type = 131;
    b[slot].larva = 1;
    return slot;
}

static int find_egg(int owner, int product)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_Z7_EGG && u[i].owner == owner &&
            u[i].target == product) return i;
    return -1;
}

static void make_egg(int owner, int producer, int product)
{
    int slot;
    assert(race_queue_unit(owner, producer, product));
    slot = find_egg(owner, product);
    assert(slot >= 0 && u[slot].raw_type == 36 &&
           u[slot].raw_owner == (owner == SIDE ? 3 : 4));
    assert(u[slot].work_timer == sc64_z7_egg_duration(product));
}

int main(void)
{
    launch_z7();int hall=add_source_hatchery(SIDE,3);
    race_minerals[SIDE]=minerals=1000;race_gas[SIDE]=gas=1000;
    make_egg(SIDE,hall,U_DRONE);int egg=find_egg(SIDE,U_DRONE);assert(egg>=0);
    for(int i=0;i<17;i++)game_update(0,0);
    int clock=u[egg].work_timer;assert(race_save_game());
    RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION && plan.egg_auth.features==0);
    u[egg].work_timer=1;assert(race_load_game());
    assert(u[egg].alive && u[egg].type==U_Z7_EGG && u[egg].work_timer==clock);
    puts("Z7 legacy campaign Egg and ordinary envelope PASS");return 0;
}
