/* SPDX-License-Identifier: GPL-3.0-or-later */
static void observe_strike_hit(int target, int damage, int source_type, int owner);
#define SC64_TEST_HIT_OBSERVER observe_strike_hit
#define SC64_NUKE_JOURNAL_FIXTURE_NO_MAIN
#include "sc64_nuke_silo_journal_game.c"

enum { GHOST_X = 700, IMPACT_X = 820, IMPACT_Y = 480 };
static int watched = -1, hits;

static void observe_strike_hit(int target, int damage, int source_type, int owner)
{
    if (target == watched && damage > 0 && source_type == -1 && owner == SIDE) ++hits;
}

static int fixture_armed_silo(void)
{
    fixture_build(SIDE, 106, FIRST_SILO_X - 48, SILO_Y - 8);
    int silo = fixture_build(SIDE, 108, FIRST_SILO_X, SILO_Y);
    assert(race_nuke_arm(silo, SIDE) == 4);
    fixture_ticks(1875);
    assert(race_nuke_armed_slot(SIDE) == silo);
    return silo;
}

static int fixture_ghost(void)
{
    int ghost = spawn_unit(SIDE, sc64_race_unit_from_raw(1), GHOST_X, IMPACT_Y);
    assert(ghost >= 0 && sc64_race_ordinary_ghost(u[ghost].type));
    return ghost;
}

static void write_strike(const char *path, int descending)
{
    fixture_start();
    assert(race_save_game());
    GbaSaveSelection previous = fixture_select(base_tag());
    fixture_armed_silo();
    int ghost = fixture_ghost();
    fixture_build(ENEMY, 154, IMPACT_X, IMPACT_Y);
    /* Create a lower slot that can be freed after launch. Saving must
     * retain the missile's actual slot and track index exactly. */
    int lower = find_unit_slot();
    assert(lower >= 0);
    assert(spawn_unit(SIDE, U_MARINE, 500, 500) == lower);
    /* Hold track zero briefly so the actual missile uses track one. This is
     * an already-resolved visual effect, which needs no journal row. */
    race_nuke_flights[0].impact_tick = 1; ++race_nuke_active_flights;
    assert(race_nuke_issue(ghost, SIDE, IMPACT_X, IMPACT_Y));
    Sc64NukeWorldFlight *flight = race_nuke_track(ghost);
    assert(flight == &race_nuke_flights[1] && flight->actor_slot_plus1);
    int slot = flight->actor_slot_plus1 - 1;
    assert(slot > lower && !u[slot].alive && !sc64_unit_slot_available(slot));
    u[lower].alive = 0;
    assert(find_unit_slot() == lower);
    fixture_ticks(8);
    if (descending) {
        for (int i = 0; i < 500 && flight->strike.phase == SC64_NUKE_PAINTING; ++i)
            fixture_ticks(1);
        assert(flight->strike.phase == SC64_NUKE_DESCENDING &&
               flight->strike.spotter == SC64_NUKE_SAVE_NO_GHOST);
        hit_target_impl_ex(ghost, 10000, -1, ENEMY, -1, 0);
        assert(!u[ghost].alive);
    }
    assert(race_save_game());
    RaceLoadPlan plan;
    GbaSaveSelection stage;
    assert(race_worker_cargo_select(&plan, &stage) && stage.tag == SC64_NUKE_SILO_TRANSPORT_TAG);
    assert(stage.sequence == 2 && stage.length == previous.length);
    assert(plan.nuke_strikes.actor_plus1[1] == slot + 1 &&
           plan.nuke_strikes.missile[1].strike.age == flight->strike.age);
    fixture_write(path);
}

static void read_strike(const char *path, int descending, int abort)
{
    gba_init(); game_init(); fixture_read(path);
    RaceLoadPlan before;
    GbaSaveSelection stage;
    assert(race_worker_cargo_select(&before, &stage) && stage.tag == SC64_NUKE_SILO_TRANSPORT_TAG);
    assert(!before.nuke_strikes.actor_plus1[0] && before.nuke_strikes.actor_plus1[1]);
    int slot = before.nuke_strikes.actor_plus1[1] - 1;
    Sc64NukeSaveMissile *saved = &before.nuke_strikes.missile[1];
    assert(race_load_game());
    fixture_assert_research();
    Sc64NukeWorldFlight *flight = &race_nuke_flights[1];
    assert(race_nuke_active_flights == 1 && race_nuke_reserved_supply(SIDE) == 16 &&
           flight->strike.phase == (descending ? SC64_NUKE_DESCENDING : SC64_NUKE_PAINTING) &&
           flight->strike.age == saved->strike.age && flight->actor_slot_plus1 == slot + 1 &&
           !u[slot].alive && !sc64_unit_slot_available(slot));
    assert(race_save_game() && fixture_select(SC64_NUKE_SILO_TRANSPORT_TAG).sequence == 3);
    int nexus = fixture_find(154, IMPACT_X, IMPACT_Y);
    int hp = b[nexus].hp, shield = b[nexus].shield;
    watched = MAX_UNITS + nexus; hits = 0;
    if (abort) {
        assert(!descending && race_nuke_ghost_locked(saved->strike.spotter));
        hit_target_impl_ex(saved->strike.spotter, 10000, -1, ENEMY, -1, 0);
        assert(!race_nuke_active_flights && sc64_unit_slot_available(slot));
    } else {
        unsigned ticks_left = saved->strike.cadence.paint + saved->strike.cadence.descent -
                              saved->strike.age;
        for (unsigned elapsed = 0; elapsed < ticks_left - 1; ) {
            fixture_ticks(1);
            elapsed += (unsigned)frame % 5u != 0;
        }
        assert(!hits && race_nuke_reserved_supply(SIDE) == 16 && !sc64_unit_slot_available(slot));
        for (int i = 0; i < 2 && !hits; ++i) fixture_ticks(1);
        assert(hits == 1 && sc64_unit_slot_available(slot) && !race_nuke_reserved_supply(SIDE));
    }
    fixture_ticks(640);
    assert(!race_nuke_active_flights && !race_nuke_reserved_supply(SIDE));
    if (abort) assert(!hits && b[nexus].hp == hp && b[nexus].shield == shield);
    else assert(hits == 1 && b[nexus].alive && b[nexus].hp < hp && b[nexus].shield < shield);
    assert(race_save_game()); /* no stale missile row replays an impact */
    assert(race_load_game() && !race_nuke_active_flights && sc64_unit_slot_available(slot));
    printf("%s restored at exact reserved slot; %d nuclear impacts\n",
           abort ? "painting abort" : descending ? "independent descent" : "painting", hits);
}

static void duplicate_track(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    RaceLoadPlan plan;
    GbaSaveSelection active;
    assert(race_worker_cargo_select(&plan, &active) && active.tag == SC64_NUKE_SILO_TRANSPORT_TAG);
    unsigned source = (unsigned)plan.nuke_strikes.actor_plus1[1] - 1u;
    const unsigned empty = MAX_UNITS - 1u;
    assert(source < empty && plan.nuke_strikes.missile[1].strike.phase == SC64_NUKE_DESCENDING);
    u8 *record = host_save + active.slot * SAVE_SLOT_SIZE;
    u8 *unit_rows = record + 16 + offsetof(RaceSave, units);
    size_t row_bytes = sizeof race_data.units[0];
    memcpy(unit_rows + empty * row_bytes, unit_rows + source * row_bytes, row_bytes);
    sram_put32(record + 8, hash_bytes(record + 16, active.length));
    assert(valid_record(record, active.length, active.tag));
    assert(fixture_select(base_tag()).sequence == 1);
    assert(race_load_game() && !race_nuke_active_flights && minerals == STOCK_BANK);
    puts("authenticated duplicate runtime flight rejected; predecessor preserved");
}

static void capacity(void)
{
    fixture_start();
    int silo = fixture_armed_silo(), ghost = fixture_ghost();
    assert(race_save_game());
    static u8 original[sizeof host_save];
    memcpy(original, host_save, sizeof original);
    assert(race_nuke_issue(ghost, SIDE, GHOST_X - 220, IMPACT_Y));
    assert(race_nuke_track(ghost)->approaching &&
           !race_save_game() && !memcmp(original, host_save, sizeof original));
    assert(race_nuke_abort(ghost) && race_nuke_armed_slot(SIDE) == silo);
    int available = find_unit_slot();
    assert(available >= 0);
    for (int i = 0; i < MAX_UNITS; ++i) if (!u[i].alive) u[i].alive = 1;
    assert(find_unit_slot() < 0 && !race_nuke_issue(ghost, SIDE, IMPACT_X, IMPACT_Y));
    assert(race_nuke_armed_slot(SIDE) == silo && !race_nuke_active_flights);
    u[available].alive = 0;
    assert(race_nuke_issue(ghost, SIDE, IMPACT_X, IMPACT_Y));
    assert(!u[available].alive && find_unit_slot() < 0 && !race_product_slots_available(1));
    assert(race_nuke_abort(ghost) && find_unit_slot() == available && race_product_slots_available(1));
    assert(spawn_unit(SIDE, U_MARINE, 500, 500) == available);
    puts("real slot exhaustion keeps armed ammo; abort releases the exact slot");
}

static void prior_silo(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    assert(race_load_game() && !race_nuke_active_flights && race_nuke_paid_silos == 2);
    int armed = fixture_find(108, FIRST_SILO_X, SILO_Y);
    int queued = fixture_find(108, SECOND_SILO_X, SILO_Y);
    assert(b[armed].owner == NEUTRAL && race_nuke_silos[armed].ammo.armed &&
           race_nuke_silos[queued].ammo.remaining == 1480 && race_nuke_reserved_supply(SIDE) == 32);
    fixture_assert_research();
    puts("earlier authenticated Silo-only record resumed with no invented flight");
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "capacity")) { capacity(); return 0; }
    assert(argc == 3);
    if (!strcmp(argv[1], "write-paint")) write_strike(argv[2], 0);
    else if (!strcmp(argv[1], "write-descent")) write_strike(argv[2], 1);
    else if (!strcmp(argv[1], "read-paint")) read_strike(argv[2], 0, 0);
    else if (!strcmp(argv[1], "read-descent")) read_strike(argv[2], 1, 0);
    else if (!strcmp(argv[1], "abort-paint")) read_strike(argv[2], 0, 1);
    else if (!strcmp(argv[1], "duplicate")) duplicate_track(argv[2]);
    else if (!strcmp(argv[1], "legacy")) legacy(argv[2]);
    else if (!strcmp(argv[1], "prior-silo")) prior_silo(argv[2]);
    else assert(!"unknown test operation");
    return 0;
}
