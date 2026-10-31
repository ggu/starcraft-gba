/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Seeded campaign economy fixture; not mission or ordinary-menu proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"

static int reaver(int owner, int source)
{
    int slot = spawn_unit(owner, U_REAVER, 128, 128);
    assert(slot >= 0);
    u[slot].raw_owner = source;
    assert(race_scarab_parent_init(slot));
    return slot;
}

static void campaign_wallets(int mission, uint32_t *first, uint32_t *second,
                             int first_source, int second_source)
{
    memset(u, 0, sizeof(u));
    race_mode = 0; race_selected_mission = mission;
    minerals = 100; race_minerals[ENEMY] = 900;
    *first = 200; *second = 300;
    int a = reaver(ENEMY, first_source), b = reaver(ENEMY, second_source);
    int h = reaver(SIDE, 7);
    assert(race_scarab_enqueue(a, 0) && *first == 185 && *second == 300);
    assert(race_scarab_enqueue(b, 0) && *first == 185 && *second == 285);
    assert(race_scarab_enqueue(h, 0) && minerals == 85);
    assert(race_minerals[ENEMY] == 900);
    assert(race_scarab_cancel(a, 0, 0) && *first == 200 && *second == 285);
    assert(!race_scarab_cancel(a, 0, 0) && *first == 200);
    assert(race_scarab_enqueue(a, 0) &&
           race_scarab_production_tick(a, 0) == SC64_SCARAB_ALLOCATED);
    *first = 65530;
    Unit overflow_parent = u[a];
    assert(!race_scarab_cancel_all(a, 0) && *first == 65530 &&
           !memcmp(&overflow_parent, &u[a], sizeof(overflow_parent)));
    *first = 185;
    assert(race_scarab_cancel_all(a, 0) && *first == 200);
    assert(race_scarab_enqueue(a, 0) && race_scarab_cancel_all(a, 0));
    assert(*first == 200 && *second == 285);
    /* Transfer contract: refund before changing provenance, even when the
     * engine combat owner remains the same. No GiveUnit hook is claimed. */
    u[a].raw_owner = second_source;
    assert(race_scarab_enqueue(a, 0) && *first == 200 && *second == 270);
    assert(race_scarab_cancel_all(a, 0) && *second == 285);
    *first = 65535;
    assert(race_scarab_enqueue(a, 0) && *first == 65535 && *second == 270);
    assert(race_scarab_cancel_all(a, 0) && *second == 285);
    u[a].raw_owner = first_source;
    *first = 14;
    Unit before = u[a];
    assert(!race_scarab_enqueue(a, 0) && !memcmp(&before, &u[a], sizeof(before)));
    assert(*first == 14 && *second == 285 && race_minerals[ENEMY] == 900);
    /* A human combat owner with enemy source provenance cannot charge either
     * a source wallet or the human bank. */
    u[a].owner = SIDE;
    before = u[a];
    assert(!race_scarab_enqueue(a, 0) && !memcmp(&before, &u[a], sizeof(before)));
    assert(minerals == 85 && *first == 14 && *second == 285);
}

int main(void)
{
    gba_init();
#ifdef SC64_P7_RUNTIME
    campaign_wallets(SC64_P7_GLOBAL_ID, &race_p7.source_minerals[0],
                     &race_p7.source_minerals[4], 0, 4);
#endif
#ifdef SC64_P8_RUNTIME
    campaign_wallets(SC64_P8_GLOBAL_ID, &race_p8.ai_minerals[0],
                     &race_p8.ai_minerals[1], 0, 3);
#endif
    memset(u, 0, sizeof(u));
    race_mode = 1; minerals = 100; race_minerals[ENEMY] = 200;
    int a = reaver(ENEMY, 0), b = reaver(SIDE, 7);
    assert(race_scarab_enqueue(a, 0) && race_minerals[ENEMY] == 185);
    assert(race_scarab_enqueue(b, 0) && minerals == 85);
    puts("Scarab source-owner campaign and custom wallet transactions passed");
    return 0;
}
