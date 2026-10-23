/* Internal late-Terran world/runtime fixture. This checks initial import and
 * bounded trigger execution; it is not a campaign completion proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static Unit full_unit_snapshot[MAX_UNITS];

static unsigned live_units(void)
{
    unsigned n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    return n;
}

static unsigned live_buildings(void)
{
    unsigned n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive != 0;
    return n;
}

static void mapping_and_diplomacy(void)
{
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T7;
    assert(sc64_terran_late_runtime_owner(0) == SIDE);
    assert(sc64_terran_late_runtime_owner(4) == ENEMY);
    assert(sc64_terran_late_runtime_owner(2) == RESCUABLE);
    assert(sc64_terran_late_runtime_owner(11) == NEUTRAL);
    assert(sc64_terran_late_current_source_owner(SIDE, -1) == 0);
    assert(sc64_terran_late_product_source_owner(ENEMY) == 4);
    assert(sc64_terran_late_owners_hostile(SIDE, ENEMY));
    assert(!sc64_terran_late_owners_hostile(ENEMY, 4));
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T9;
    assert(sc64_terran_late_runtime_owner(1) == ENEMY);
    assert(sc64_terran_late_runtime_owner(3) == 4);
    assert(sc64_terran_late_owners_hostile(ENEMY, 4));
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T10;
    assert(sc64_terran_late_runtime_owner(1) == SIDE);
    assert(sc64_terran_late_current_source_owner(SIDE, 6) == 1);
}

static void mission_start_and_live_trigger_passes(void)
{
    const unsigned missions[] = {
        SC64_TERRAN_LATE_GLOBAL_T7,
        SC64_TERRAN_LATE_GLOBAL_T8,
        SC64_TERRAN_LATE_GLOBAL_T9,
        SC64_TERRAN_LATE_GLOBAL_T10
    };
    for (unsigned m = 0; m < sizeof(missions) / sizeof(missions[0]); ++m) {
        game_init();
        race_mode = 0;
        race_selected_mission = (u8)missions[m];
        /* Load the retained source roster through ordinary mission startup
         * so opening triggers see their authored actors. */
        init_state();
        assert(race_late_active());
        const Sc64RaceMap *map = &sc64_race_maps[missions[m]];
        assert(race_cols == map->width && race_rows == map->height);
        assert(!mission_unsupported_count);
        assert(live_units() > 0 && live_buildings() > 0);
        if (missions[m] == SC64_TERRAN_LATE_GLOBAL_T7) {
            /* The source creates its emitter in the first trigger pass. */
            assert(race_late.emitter_unit == SC64_TERRAN_LATE_NO_SLOT);
        }
        assert(!mission_unsupported_count);
        unsigned tick = 0;
        while (tick < 3000 && screen == SCREEN_PLAY) {
            game_update(0, 0);
            ++tick;
        }
        assert(tick == 3000 || race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
               race_campaign.phase == SC64_RACE_PHASE_FAILED);
        assert(screen == SCREEN_PLAY ||
               race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
               race_campaign.phase == SC64_RACE_PHASE_FAILED);
        assert(race_campaign.elapsed_frames == tick);
        for (unsigned owner = 0; owner < 8; ++owner) {
            assert(!race_late.triggers.player[owner].fault);
            assert(race_late.triggers.player[owner].elapsed_ticks == tick);
        }
        if (missions[m] == SC64_TERRAN_LATE_GLOBAL_T7) {
            assert(race_late.emitter_unit < MAX_UNITS);
            assert(u[race_late.emitter_unit].alive &&
                   sc64_terran_late_raw_unit(&u[race_late.emitter_unit]) == 217);
            if (race_late.emitter_carrier != SC64_TERRAN_LATE_NO_SLOT) {
                assert(race_late.emitter_carrier < MAX_UNITS);
                assert(u[race_late.emitter_carrier].alive &&
                       u[race_late.emitter_carrier].owner == SIDE);
            }
        }
    }
}

static void death_ledgers_use_source_slots(void)
{
    unsigned human = 0;
    int enemy;
    u32 count;
    Sc64T4Condition condition = {0};
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T7;
    init_state();
    enemy = sc64_terran_late_product_source_owner(ENEMY);
    assert(enemy >= 0 && enemy < 8 && enemy != 0);
    race_late_record_actor_death(ENEMY, -1, SIDE, -1, 0, 0);
    race_late_record_actor_death(ENEMY, -1, SIDE, -1, 0, 1);
    race_late_record_actor_death(SIDE, -1, ENEMY, -1, 0, 0);
    assert(race_late.building_deaths[human] == 1);
    assert(race_late.building_kills[enemy] == 1);
    assert(race_late.kills[enemy] == 2 && race_late.deaths[human] == 2);
    assert(race_late.kills[human] == 1 && race_late.deaths[enemy] == 1);

    condition.player = (u32)enemy;
    condition.unit = 229;
    condition.opcode = 5;
    count = 0;
    assert(race_late_trigger_count(&race_late, &condition, 0, &count) == 1);
    assert(count == 2);
    condition.unit = 231;
    count = 0;
    assert(race_late_trigger_count(&race_late, &condition, 0, &count) == 1);
    assert(count == 1);
    condition.player = human;
    condition.opcode = 15;
    condition.unit = 229;
    count = 0;
    assert(race_late_trigger_count(&race_late, &condition, 0, &count) == 1);
    assert(count == 2);
    condition.unit = 231;
    count = 0;
    assert(race_late_trigger_count(&race_late, &condition, 0, &count) == 1);
    assert(count == 1);
}

static void full_pool_creation_retries_atomically(void)
{
    Sc64T4Action create = {0};
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T7;
    init_state();
    assert(!mission_unsupported_count);
    for (int i = 0; i < MAX_UNITS; ++i) u[i].alive = 1;
    memcpy(full_unit_snapshot, u, sizeof(u));
    create.location = 1;
    create.player = 0;
    create.unit = 0; /* Source Marine */
    create.opcode = 11;
    create.flags = 0x10;
    /* A full unit pool must request a retry without changing any actor byte. */
    assert(sc64_terran_late_create(&create, 0) == 0);
    assert(memcmp(full_unit_snapshot, u, sizeof(u)) == 0);
    /* Once one slot is freed the same action can complete exactly once. */
    u[MAX_UNITS - 1].alive = 0;
    assert(sc64_terran_late_create(&create, 0) == 1);
    assert(u[MAX_UNITS - 1].alive &&
           sc64_terran_late_raw_unit(&u[MAX_UNITS - 1]) == 0 &&
           u[MAX_UNITS - 1].owner == SIDE && u[MAX_UNITS - 1].raw_owner == 0);
}

static void firebat_splash_respects_late_invincibility(void)
{
    int emitter = -1;
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T9;
    init_state();
    assert(race_late_active());

    /* Keep the actual imported raw-16 actor, but isolate the combat fixture. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 16) { emitter = i; break; }
    assert(emitter >= 0);
    memset(u, 0, sizeof(u));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    Unit *item = &u[emitter];
    item->alive = 1; item->type = U_KERRIGAN; item->raw_type = 16;
    item->raw_owner = 2; item->owner = SIDE;
    item->x = 200; item->y = 200; item->hp = item->maxhp = 250;
    invincible_units[emitter] = 1; /* TR0's live invincibility state. */

    u[0].alive = 1; u[0].type = U_SCV; u[0].raw_type = 7;
    u[0].raw_owner = 0; u[0].owner = SIDE;
    u[0].x = 201; u[0].y = 200; u[0].hp = u[0].maxhp = 100;
    u[1].alive = 1; u[1].type = U_SCV; u[1].raw_type = 7;
    u[1].raw_owner = 0; u[1].owner = SIDE;
    u[1].x = 202; u[1].y = 200; u[1].hp = u[1].maxhp = 100;

    /* An enemy Firebat's primary hit splashes nearby SIDE units. The human
     * hero must survive both the direct-invincibility case and the adjacent
     * splash path, while ordinary targets still receive splash damage. */
    hit_target(0, 10, U_FIREBAT, ENEMY);
    assert(item->alive && item->hp == 250);
    assert(u[0].hp == 87); /* primary 10, then radial splash 3 */
    assert(u[1].hp == 97);

    /* The actor's native invincibility flag is also honored by late splash. */
    invincible_units[emitter] = 0;
    item->flags = 16;
    item->hp = 250;
    u[0].hp = u[1].hp = 100;
    hit_target(0, 10, U_FIREBAT, ENEMY);
    assert(item->alive && item->hp == 250);
}

int main(void)
{
    mapping_and_diplomacy();
    mission_start_and_live_trigger_passes();
    death_ledgers_use_source_slots();
    full_pool_creation_retries_atomically();
    firebat_splash_respects_late_invincibility();
    puts("late Terran world checks passed");
    return 0;
}
