/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST fixture: source campaign state and the actual product-picker helper.
 * Resources and tech are constructed here; this is not a mission playthrough. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#if !defined(SC64_Z9_RUNTIME) || !defined(SC64_SHARED_SCOURGE_COMBAT)
#error "Use the ordinary Z9 or Z10 shared-combat vector"
#endif

static void check_picker(int mission)
{
    game_init();
    race_mode = 0;
    race_human = R_ZERG;
    race_selected_mission = mission;
    init_state();
    assert(!race_mode && !race_z7_active() && screen == SCREEN_PLAY);
    int hatch = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && building_complete(i) &&
            sc64_race_is_zerg_townhall(b[i].type)) { hatch = i; break; }
    assert(hatch >= 0);
    const int raw_owner = b[hatch].raw_owner;
    assert(raw_owner >= 0 && raw_owner < 12);
    const Sc64Z8PermissionTable *permissions = mission == SC64_Z8_GLOBAL_ID ?
        &sc64_z8_permission_table : &sc64_z9_program_permissions;
#ifdef SC64_Z10_RUNTIME
    if (mission == SC64_Z10_GLOBAL_ID) permissions = &sc64_z10_program_permissions;
#endif
    assert(sc64_z8_permission_available(permissions, (unsigned)raw_owner, 47));
    selected_building = hatch;
    train_type = U_DRONE;
    const int ore = minerals, vespene = gas, larvae = b[hatch].larva;
    const int queues = b[hatch].queue_count;
    int found = 0;
    for (int n = 0; n < 12; ++n) {
        select_next_train_type();
        if (train_type == U_SCOURGE) found = 1;
    }
    assert(found && selected_building == hatch);
    assert(minerals == ore && gas == vespene && b[hatch].larva == larvae &&
           b[hatch].queue_count == queues);
    for (int n = 0; n < 6 && train_type != U_SCOURGE; ++n)
        select_next_train_type();
    assert(train_type == U_SCOURGE);
    assert(race_save_game());
    train_type = U_DRONE;
    assert(race_load_game() && train_type == U_SCOURGE);
    printf("campaign %d: source-permitted Scourge appears without spending\n", mission);
}

static void check_denied(int mission)
{
    game_init(); race_mode = 0; race_human = R_ZERG;
    race_selected_mission = mission; init_state();
    assert(screen == SCREEN_PLAY);
    int hatch = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            sc64_race_is_zerg_townhall(b[i].type)) { hatch = i; break; }
    /* Z5 starts without a human town hall; exercise future production. */
    if (hatch < 0) {
        for (int i = 0; i < MAX_BUILDINGS; ++i) if (!b[i].alive) { hatch = i; break; }
        assert(hatch >= 0);
        b[hatch].alive = 1; b[hatch].owner = SIDE;
        b[hatch].raw_owner = 3; b[hatch].type = B_HATCHERY;
    }
    assert(spawn_build(SIDE, B_SPIRE, 400, 400, 0) >= 0);
    assert(spawn_unit(SIDE, U_OVERLORD, 450, 450) >= 0);
    minerals = gas = 5000; b[hatch].larva = 3;
    b[hatch].under_construction = 0;
    selected_building = hatch; train_type = U_DRONE;
    const int ore = minerals, vespene = gas, larvae = b[hatch].larva;
    const int queues = b[hatch].queue_count;
    for (int n = 0; n < 12; ++n) {
        select_next_train_type(); assert(train_type != U_SCOURGE);
    }
    assert(!race_queue_unit(SIDE, hatch, U_SCOURGE));
    assert(minerals == ore && gas == vespene && b[hatch].larva == larvae &&
           b[hatch].queue_count == queues);
    assert(ui_missing_tech(B_SPIRE, -1) != 0);
    int x = 0, y = 0;
    assert(!race_build_request_valid(SIDE, -1, B_SPIRE, &x, &y, 0));
    /* Captured actors preserve source provenance but obey the live controller. */
    b[hatch].raw_owner = 4;
    assert(!race_queue_unit(SIDE, hatch, U_SCOURGE));
    assert(!race_zerg_campaign_product_available(SIDE, 47));
    assert(race_zerg_campaign_product_available(ENEMY, 47));
    assert(race_zerg_campaign_product_available(SIDE, 43));
    /* Same viable producer, captured provenance, prerequisites and resources:
     * a permitted mission must accept and charge for the product. */
    race_selected_mission = SC64_RACE_MISSION_Z2;
    assert(race_queue_unit(SIDE, hatch, U_SCOURGE));
    assert(minerals == ore - 25 && gas == vespene - 75 &&
           b[hatch].larva == larvae - 1);
    race_mode = 1;
    assert(race_zerg_campaign_product_available(SIDE, 47));
    assert(race_zerg_campaign_product_available(SIDE, 141));
    printf("campaign %d: denied products do not spend; custom unaffected\n", mission);
}

int main(void)
{
    check_denied(SC64_RACE_MISSION_Z1);
    check_denied(SC64_Z5_GLOBAL_ID);
    check_picker(SC64_Z8_GLOBAL_ID);
    check_picker(SC64_Z9_GLOBAL_ID);
#ifdef SC64_Z10_RUNTIME
    check_picker(SC64_Z10_GLOBAL_ID);
#endif
    return 0;
}
