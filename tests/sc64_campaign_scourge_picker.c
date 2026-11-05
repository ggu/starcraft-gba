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
    printf("campaign %d: source-permitted Scourge appears without spending\n", mission);
}

int main(void)
{
    check_picker(SC64_Z8_GLOBAL_ID);
    check_picker(SC64_Z9_GLOBAL_ID);
#ifdef SC64_Z10_RUNTIME
    check_picker(SC64_Z10_GLOBAL_ID);
#endif
    return 0;
}
