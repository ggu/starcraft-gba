/* Internal T4 scenario save validator. This fixture exercises the authored
 * save schema; it is not a T4 controller-win or load/replay proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifdef SC64_TERRAN_LATE_RUNTIME
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.terran_late)
#elif defined(SC64_T6_RUNTIME)
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.t6)
#elif defined(SC64_T5_RUNTIME)
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.t5)
#else
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.t4)
#endif

static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Sc64T4RuntimeState saved_t4;
static RaceSave good_save;

static int saved_building_word(const RaceSave *save, unsigned slot, unsigned word)
{
#ifdef SC64_T6_STORAGE
    Building row;
    int result;
    assert(sc64_building_save_decode(&row, &save->buildings[slot]));
    memcpy(&result, (const u8 *)&row + word * sizeof(int), sizeof(result));
    return result;
#else
    return save->buildings[slot][word];
#endif
}

static void corrupt_building_word(RaceSave *save, unsigned slot, unsigned word,
                                  int value)
{
#ifdef SC64_T6_STORAGE
    Building row;
    assert(sc64_building_save_decode(&row, &save->buildings[slot]));
    memcpy((u8 *)&row + word * sizeof(int), &value, sizeof(value));
    assert(sc64_building_save_encode(&save->buildings[slot], &row,
                                     race_cols * 16u, race_rows * 16u));
#else
    save->buildings[slot][word] = (s16)value;
#endif
}

static void make_runtime_state(void)
{
    memset(&race_t4, 0, sizeof(race_t4));
    race_t4.active_mask = 0xdf;
    for (unsigned p = 0; p < 8; ++p)
        assert(sc64_t4_trigger_init(&race_t4.player[p], &sc64_t4_scenario,
                                    p, sc64_t4_source_force((int)p)));
    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        const Sc64T4Doodad *row = &sc64_t4_scenario.doodads[d];
        unsigned slot = 255, enabled = 0;
        if (!(row->flags & SC64_T4_THG2_SPRITE_FLAG)) {
            int type;
            slot = 8;
            for (unsigned j = 0; j < d; ++j)
                if (!(sc64_t4_scenario.doodads[j].flags & SC64_T4_THG2_SPRITE_FLAG))
                    ++slot;
            assert(slot < MAX_BUILDINGS);
            type = sc64_race_building_from_raw(row->type);
            assert(type >= 0);
            memset(&b[slot], 0, sizeof(b[slot]));
            b[slot].alive = 1;
            b[slot].raw_type = row->type;
            b[slot].raw_owner = row->owner;
            b[slot].type = type;
            b[slot].owner = sc64_race_t4_is_door(type) ? NEUTRAL :
                            sc64_t4_runtime_owner(row->owner);
            b[slot].x = row->x / 2;
            b[slot].y = row->y / 2;
            b[slot].hp = b[slot].maxhp = sc64_race_t4_building_intrinsically_invincible(type) ?
                          100000 : sc64_race_building_rules[type].hp;
            invincible_buildings[slot] =
                (u8)sc64_race_t4_building_intrinsically_invincible(type);
            assert(sc64_t4_doodad_initial_enabled(row->flags, &enabled));
        }
        race_t4.doodad_slot[d] = (u8)slot;
        race_t4.doodad_enabled[d] = (u8)enabled;
    }
}

static void start_fixture(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    init_state();
    assert(race_cols == 128 && race_rows == 128);
    frame = 3;
    race_campaign.mission = SC64_T4_GLOBAL_ID;
    race_campaign.elapsed_frames = (u32)frame;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    make_runtime_state();
}

/* Write a candidate through the same record envelope as the game, then
 * prove the envelope decodes before testing T4 semantic rejection. */
static void write_candidate_record(const RaceSave *candidate)
{
#ifdef SC64_IMPACT_EVENTS
    Sc64ImpactEvent impacts[RACE_IMPACT_CAPACITY];
    int count = -1;
    race_data = *candidate;
    assert(race_impact_save_write());
    assert(race_impact_save_read(impacts, &count));
    assert(count == race_impact_count);
    assert(memcmp(&race_data, candidate, sizeof(*candidate)) == 0);
#else
    static RaceSave readback;
    save_write(candidate, sizeof(*candidate), SAVE_VERSION);
    assert(save_read(&readback, sizeof(readback), SAVE_VERSION));
    assert(memcmp(&readback, candidate, sizeof(*candidate)) == 0);
    race_data = readback;
#endif
}

static void round_trip_and_atomic_validation(void)
{
    int changed_building = 8;
    race_t4.switches.bits[31] = 0x80; /* Highest shared switch index. */
    for (unsigned p = 0; p < 8; ++p)
        if (p != 5) race_t4.player[p].elapsed_ticks = frame;
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    build_path_grid();
    int route_goal = -1;
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        if (SC64_GRID_GET(path_walkable, cell)) { route_goal = cell; break; }
    assert(route_goal >= 0 && sc64_route_slot(route_goal) >= 0);
    assert(sc64_route_builds > 0); /* The shared scratch has held a BFS queue. */
#endif
    assert(race_save_game());
    assert(race_t4_save_candidate_valid(&race_data, race_cols, race_rows));
    good_save = race_data;
    assert(race_data.t4.switches.bits[31] == 0x80);
    assert(race_data.t4.player[0].elapsed_ticks == (u32)frame);
    assert(race_data.t4.active_mask == 0xdf);
    assert(saved_building_word(&race_data, changed_building,
                              offsetof(Building,hp)/sizeof(int)) ==
#ifdef SC64_T6_STORAGE
           b[changed_building].hp);
#else
           (sc64_race_t4_building_intrinsically_invincible(b[changed_building].type) ?
            0 : b[changed_building].hp));
#endif

    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    saved_t4 = race_t4;
    memset(&race_t4, 0, sizeof(race_t4));
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    assert(race_load_game());
    assert(memcmp(saved_units, u, sizeof(u)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(b)) == 0);
    assert(memcmp(&saved_t4, &race_t4, sizeof(race_t4)) == 0);
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    /* Dropping the inactive instance is valid only for its exact canonical
     * source state. A rejected save must leave the preceding SRAM record. */
    ++race_t4.player[5].elapsed_ticks;
    assert(!race_save_game());
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(&race_t4, 0, sizeof(race_t4));
    assert(race_load_game());
    assert(memcmp(saved_units, u, sizeof(u)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(b)) == 0);
    assert(memcmp(&saved_t4, &race_t4, sizeof(race_t4)) == 0);
    build_path_grid();
    sc64_routes_reset();
    route_goal = -1;
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        if (SC64_GRID_GET(path_walkable, cell)) { route_goal = cell; break; }
    assert(route_goal >= 0 && sc64_route_slot(route_goal) >= 0);
#endif

    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        unsigned slot = race_t4.doodad_slot[d];
        if (slot != 255 && sc64_race_t4_is_trap(b[slot].type)) {
            b[slot].alive = 0;
            assert(race_save_game());
            good_save = race_data;
            assert(race_data.t4.doodad_slot[d] == slot);
            assert(race_load_game());
            assert(!b[slot].alive && race_t4.doodad_slot[d] == slot);
            break;
        }
    }
    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        unsigned slot = race_t4.doodad_slot[d];
        if (slot != 255 && sc64_race_t4_is_door(b[slot].type)) {
            b[slot].alive = 0;
            assert(race_save_game());
            assert(saved_building_word(&race_data, slot,
                                      offsetof(Building,hp)/sizeof(int)) ==
#ifdef SC64_T6_STORAGE
                   100000);
#else
                   0);
#endif
            good_save = race_data;
            assert(race_load_game());
            assert(!b[slot].alive && b[slot].hp == 100000 && b[slot].maxhp == 100000);
            break;
        }
    }
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    saved_t4 = race_t4;

    unsigned sprite_row = 32, trap_slot = MAX_BUILDINGS;
    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        if (sc64_t4_scenario.doodads[d].flags & SC64_T4_THG2_SPRITE_FLAG)
            sprite_row = d;
        else if (race_t4.doodad_slot[d] != 255 &&
                 sc64_race_t4_is_trap(b[race_t4.doodad_slot[d]].type))
            trap_slot = race_t4.doodad_slot[d];
    }
    assert(sprite_row < 32 && trap_slot < MAX_BUILDINGS);
    for (unsigned bad = 0; bad < 9; ++bad) {
        RaceSave corrupt = good_save;
        if (bad == 0) corrupt.t4.active_mask ^= 1;
        if (bad == 1) corrupt.t4.player[5].elapsed_ticks++;
        if (bad == 2) corrupt.t4.player[0].elapsed_ticks++;
        if (bad == 3) corrupt.t4.doodad_slot[0] ^= 1;
        if (bad == 4) corrupt.t4.location30_moved = 1; /* Empty extents. */
        if (bad == 5) corrupt.t4.ping_ticks = 91;
        if (bad == 6) corrupt_building_word(&corrupt, 8,
            offsetof(Building,raw_type)/sizeof(int),
            saved_building_word(&corrupt, 8, offsetof(Building,raw_type)/sizeof(int)) + 1);
        if (bad == 7) corrupt.t4.doodad_enabled[sprite_row] = 1;
        if (bad == 8) corrupt_building_word(&corrupt, trap_slot,
            offsetof(Building,queue_progress)/sizeof(int), 29);
        write_candidate_record(&corrupt);
        assert(!race_t4_save_candidate_valid(&race_data, race_cols, race_rows));
        assert(!race_load_game());
        assert(memcmp(saved_units, u, sizeof(u)) == 0);
        assert(memcmp(saved_buildings, b, sizeof(b)) == 0);
        assert(memcmp(&saved_t4, &race_t4, sizeof(race_t4)) == 0);
    }
    write_candidate_record(&good_save);
    assert(race_load_game());
}

static void non_t4_saves_zero_the_optional_state(void)
{
    race_mode = 1;
    race_skirmish_map = 0;
    init_state(); /* A real skirmish launch initializes its own map/entities. */
    memset(&race_t4, 0xa5, sizeof(race_t4));
    assert(race_save_game());
    for (unsigned i = 0; i < SC64_TEST_OPTIONAL_SAVE_BYTES; ++i)
        assert(((const u8 *)&race_data.t4)[i] == 0);
    assert(race_load_game());
    for (unsigned i = 0; i < sizeof(race_t4); ++i)
        assert(((const u8 *)&race_t4)[i] == 0);
}

int main(void)
{
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
#ifdef SC64_TERRAN_LATE_RUNTIME
    _Static_assert(SC64_RACE_SAVE_GENERATION == 18u,
                   "full Terran saves require their own generation");
#elif defined(SC64_T6_STORAGE)
    _Static_assert(SC64_RACE_SAVE_GENERATION == 15u,
                   "expanded combined storage requires a distinct generation");
#elif defined(SC64_T5_RUNTIME)
    _Static_assert(SC64_RACE_SAVE_GENERATION == 14u,
                   "combined T3/T4/T5 saves require a distinct generation");
#else
    _Static_assert(SC64_RACE_SAVE_GENERATION == 11u,
                   "combined T3/T4 saves require a distinct generation");
#endif
    _Static_assert(sizeof(RaceSave) <= 16368,
                   "combined payload must fit one SRAM record");
    _Static_assert(sizeof(Sc64CombinedRouteSaveScratch) >= sizeof(RaceSave),
                   "route BFS storage must cover the typed save scratch");
    assert((void *)&race_data == (void *)&sc64_combined_route_save_scratch);
#endif
    start_fixture();
    round_trip_and_atomic_validation();
    non_t4_saves_zero_the_optional_state();
    puts("T4 scenario save candidate validation, canonical prop health and atomic restore pass");
    return 0;
}
