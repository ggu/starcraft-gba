/* Focused P6 runtime-adapter checks. This fixture uses the real engine data
 * and actor arrays but calls only bounded adapter helpers; mission launch
 * remains fail-closed until the separately gated combat handlers are wired. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_p6_runtime_adapter.h"

#ifndef SC64_P6_RUNTIME
#error "Compile this fixture with the gated P6 runtime flags"
#endif

static void reset_runtime(void)
{
    race_mode = 0;
    race_selected_mission = SC64_P6_GLOBAL_ID;
    memset(&race_p6, 0, sizeof(race_p6));
    sc64_p6_runtime_init(&race_p6.runtime);
    (void)race_p6_world_context_init(&sc64_race_maps[SC64_P6_GLOBAL_ID]);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
}

static void persistent_state_and_reservations(void)
{
    Sc64P6PersistentState s;
    memset(&s, 0, sizeof(s));
    sc64_p6_runtime_init(&s.runtime);
    assert(sc64_p6_persistent_state_valid(&s));
    assert(sc64_p6_remaining_creations(&s.runtime) == SC64_P6_CREATE_ACTIONS);
    assert(sc64_p6_runtime_metadata_valid());
    s.doodad_enabled[4] = 0x20;
    assert(!sc64_p6_persistent_state_valid(&s));
    memset(&s, 0, sizeof(s)); sc64_p6_runtime_init(&s.runtime);
    for (unsigned i=0;i<SC64_P6_DOODAD_SOURCE_COUNT;++i)
        if (sc64_p6_doodads[i].sprite_only) {
            s.doodad_enabled[i>>3] = (u8)(1u << (i&7u));
            assert(!sc64_p6_persistent_state_valid(&s));
            s.doodad_enabled[i>>3] = 0;
            break;
        }
    s.reserved[1] = 1;
    assert(!sc64_p6_persistent_state_valid(&s));
}

static void owner_and_death_provenance(void)
{
    Unit q;
    reset_runtime();
    assert(race_p6_live_owner_for_source(7) == SIDE);
    /* Import may query ownership before runtime context binding. */
    assert(race_p6_source_owner(1) == RESCUABLE);
    assert(race_p6_source_owner(5) == RESCUABLE);
    assert(race_p6_live_owner_for_source(1) == RESCUABLE);
    assert(race_p6_live_owner_for_source(2) == RESCUABLE);
    assert(race_p6_live_owner_for_source(5) == RESCUABLE);
    assert(race_p6_live_owner_for_source(3) == ENEMY);
    assert(race_p6_live_owner_for_source(6) == 4);
    assert(race_p6_live_owner_for_source(0) == NEUTRAL);
    assert(!race_p6_owners_hostile(SIDE, RESCUABLE));
    assert(race_p6_owners_hostile(SIDE, ENEMY));
    assert(race_p6_owners_hostile(ENEMY, 4));
    assert(!race_p6_owners_hostile(NEUTRAL, ENEMY));
    memset(&q,0,sizeof(q)); q.owner=ENEMY; q.raw_owner=6;
    race_p6_note_unit_death(&q); assert(race_p6.deaths_by_source[6] == 1);
    q.owner=SIDE; q.raw_owner=1;
    race_p6_note_unit_death(&q); assert(race_p6.deaths_by_source[7] == 1);
}

static void door_binding_and_objective(void)
{
    int row = -1;
    reset_runtime();
    assert(strcmp(race_p6_objective_text(),
                  "Rescue the heroes; bring Zeratul to the extraction point.") == 0);
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        if (!sc64_p6_doodads[i].sprite_only &&
            sc64_p6_doodads[i].raw_type == 207) {
            row = (int)i;
            break;
        }
    assert(row >= 0);
    b[3].alive = 1;
    b[3].raw_type = 207;
    /* Unknown/unrebound doors fail closed. A rebound source-enabled door is
     * collidable; clearing its state bit opens it. */
    assert(race_p6_building_collidable(3));
    race_p6_doodad_bindings[row].building_slot = 3;
    race_p6.doodad_enabled[(unsigned)row >> 3] |=
        (u8)(1u << ((unsigned)row & 7u));
    assert(race_p6_building_collidable(3));
    race_p6.doodad_enabled[(unsigned)row >> 3] &=
        (u8)~(1u << ((unsigned)row & 7u));
    assert(!race_p6_building_collidable(3));
    b[4].alive = 1;
    b[4].raw_type = 195;
    b[4].type = B_Z5_TERRAN_BEACON;
    assert(!race_p6_building_collidable(4));
    b[5].alive = 1;
    b[5].raw_type = 205;
    b[5].type = sc64_race_building_from_raw(205);
    assert(!race_p6_building_collidable(5));
    b[5].raw_type = 206;
    b[5].type = sc64_race_building_from_raw(206);
    assert(!race_p6_building_collidable(5));
}

static void authored_ai_policy_has_real_effect(void)
{
    Sc64P6SourceAction a;
    int enemy=-1, ally=-1;
    reset_runtime();
    for (int i=0;i<MAX_UNITS;++i) {
        if (u[i].alive) continue;
        if (enemy<0) enemy=i; else { ally=i; break; }
    }
    assert(enemy>=0 && ally>=0);
    assert(sc64_p6_source_triggers[13].enabled_players[19] == 1);
    assert(sc64_p6_source_forces[3] == 1 && sc64_p6_source_forces[6] == 3);
    u[enemy].alive=1; u[enemy].owner=ENEMY; u[enemy].raw_owner=3;
    u[enemy].raw_type=37; u[enemy].x=20; u[enemy].y=30;
    u[ally].alive=1; u[ally].owner=SIDE; u[ally].raw_owner=7;
    u[ally].raw_type=65; u[ally].x=100; u[ally].y=200;
    u[2].alive=1; u[2].owner=4; u[2].raw_owner=6;
    u[2].raw_type=37; u[2].x=300; u[2].y=400;
    memset(&a,0,sizeof(a));
    a.trigger=13; a.index=0; a.opcode=SC64_P6_ACTION_RUN_AI;
    a.group2=0x63697553u;
    assert(race_p6_source_ai(0,&a));
    assert(u[enemy].order==ORDER_ATTACK_MOVE && u[enemy].tx==100 && u[enemy].ty==200);
    assert(u[2].order==ORDER_IDLE);
    u[enemy].alive=0;
    assert(!race_p6_source_ai(0,&a));
}

static void startup_is_still_fail_closed_without_actor_handlers(void)
{
    assert(race_p6_source_row_valid(&sc64_race_maps[SC64_P6_GLOBAL_ID]));
#if !defined(SC64_P6_COMBAT_ENABLED)
    assert(!race_p6_preflight(&sc64_race_maps[SC64_P6_GLOBAL_ID]));
#endif
}

int main(void)
{
    persistent_state_and_reservations();
    owner_and_death_provenance();
    door_binding_and_objective();
    authored_ai_policy_has_real_effect();
    startup_is_still_fail_closed_without_actor_handlers();
    return 0;
}
