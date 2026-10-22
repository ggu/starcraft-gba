/* Actual-engine campaign query fixtures. Direct world edits are test setup,
 * not a controller-driven gameplay or mission-completion proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_T5_RUNTIME
#error "T5 transport query fixture requires the T5 runtime"
#endif

static void
start_world(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active());
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
}

static Unit *
unit_at(int slot, int raw, int owner, int raw_owner, int x, int y)
{
    Unit *q = &u[slot];
    int type = sc64_race_unit_from_raw(raw);
    assert(type >= 0);
    memset(q, 0, sizeof(*q));
    q->alive = 1;
    q->type = type;
    q->raw_type = raw;
    q->owner = owner;
    q->raw_owner = raw_owner;
    q->x = x;
    q->y = y;
    q->target = -1;
    return q;
}

static void
set_location_condition(Sc64T4Condition *c, unsigned raw_type,
                       unsigned comparison, unsigned location)
{
    memset(c, 0, sizeof(*c));
    c->opcode = 3; /* Bring */
    c->player = 0; /* source player 0 / current SIDE */
    c->unit = (u16)raw_type;
    c->comparison = (u8)comparison;
    c->location = location;
}

static u32
query(Sc64T4Condition *c)
{
    u32 count = 0xdeadbeefu;
    assert(sc64_t5_world_count(&race_t5, c, 0, &count) == 1);
    return count;
}

static void
test_source_bounds_and_flying_class(void)
{
    int l, u0, r, d;
    assert(sc64_t5_raw_extents(11, 0, &l, &u0, &r, &d));
    assert(l == 24 && u0 == 16 && r == 24 && d == 20);
    assert(sc64_t5_raw_extents(115, 1, &l, &u0, &r, &d));
    assert(l == 47 && u0 == 24 && r == 28 && d == 22);
    assert(sc64_t5_source_is_flying(11));
    assert(sc64_t5_source_is_flying(8));
    assert(!sc64_t5_source_is_flying(19));
}

static void
test_command_includes_cargo_and_bring_at_most_excludes_it(void)
{
    start_world();
    Unit *ship = unit_at(100, 11, SIDE, 0, 1299, 240);
    Unit *marine = unit_at(0, 0, SIDE, 0, 1400, 300);
    Sc64T4Condition c;
    u32 count;
    assert(ship && marine);
    marine->order = ORDER_TRANSPORTED;
    marine->target = 100;

    memset(&c, 0, sizeof(c));
    c.opcode = 2; /* Command: global unit count includes loaded units. */
    c.player = 0;
    c.unit = 0;
    c.comparison = 1;
    count = query(&c);
    assert(count == 1);

    c.unit = 230; /* Men includes physical Dropships and ordinary passengers. */
    count = query(&c);
    assert(count == 2);

    /* The passenger itself is deliberately inside location 1, while its
       carrier is just outside the boundary. AtMost Bring counts physical
       actors only and must ignore cargo entirely. */
    set_location_condition(&c, 229, 1, 1);
    count = query(&c);
    assert(count == 0);
    ship->x = 1300; /* the Dropship's source-pixel rectangle now overlaps */
    count = query(&c);
    assert(count == 1); /* carrier, not passenger, is the physical actor */
}

static void
test_completed_bring_uses_carrier_rect_and_elevation(void)
{
    start_world();
    Unit *ship = unit_at(100, 11, SIDE, 0, 1299, 240);
    Unit *kerrigan = unit_at(0, 16, SIDE, 0, 1400, 300);
    Sc64T4Condition c;
    assert(ship && kerrigan);
    kerrigan->order = ORDER_TRANSPORTED;
    kerrigan->target = 100;

    set_location_condition(&c, 16, 0, 1); /* comparison != AtMost */
    assert(query(&c) == 0);
    ship->x = 1300;
    assert(query(&c) == 1); /* passenger center may be anywhere */

    /* Location 18 uses native elevation mask 7 (ground-only). Even when its
       rectangle overlaps, a loaded passenger is excluded by the flying
       carrier's elevation, rather than by the passenger's own position. */
    set_location_condition(&c, 16, 0, 18);
    assert(sc64_t5_scenario.locations[17].elevation == 7);
    ship->x = 100;
    ship->y = 1000;
    kerrigan->x = 100;
    kerrigan->y = 1000;
    assert(query(&c) == 0);
}

static void
test_completed_bring_excludes_incomplete_building_but_at_most_keeps_it(void)
{
    start_world();
    Sc64T4Condition c;
    Building *tower = &b[0];
    int type = sc64_race_building_from_raw(115);
    assert(type >= 0);
    memset(tower, 0, sizeof(*tower));
    tower->alive = 1;
    tower->type = type;
    tower->raw_type = 115;
    tower->owner = SIDE;
    tower->raw_owner = 0;
    tower->x = 1300;
    tower->y = 240;
    tower->under_construction = 1;
    set_location_condition(&c, 229, 1, 1);
    assert(query(&c) == 1);
    c.comparison = 0;
    assert(query(&c) == 0);

    memset(&c, 0, sizeof(c));
    c.opcode = 2;
    c.player = 0;
    c.unit = 115;
    c.comparison = 0; /* completed Command excludes an unfinished building */
    assert(query(&c) == 0);
    c.comparison = 1; /* AtMost uses the ordinary, non-completed count */
    assert(query(&c) == 1);
}

static void
test_tower_is_known_nontransportable(void)
{
    unsigned space = 0;
    assert(sc64_transport_space_required_raw(115, &space));
    assert(space == SC64_TRANSPORT_SPACE_UNAVAILABLE);
    assert(!sc64_transport_passenger_eligible_raw(115));
}

static void
test_target_scan_skips_loaded_passenger(void)
{
    start_world();
    Unit *passenger = unit_at(0, 0, SIDE, 0, 500, 500);
    Unit *carrier = unit_at(100, 11, SIDE, 0, 500, 500);
    Unit *enemy = unit_at(1, 32, ENEMY, 6, 520, 500);
    assert(passenger && carrier && enemy);
    passenger->order = ORDER_TRANSPORTED;
    passenger->target = 100;

    /* Force the bounded linear fallback. A closer passenger is still alive
       for Command accounting, but is not a world target. */
    target_lists_valid = 0;
    sc64_target_index_end();
    int target = nearest_enemy_typed(500, 500, SIDE, 96, U_MARINE);
    assert(target == 1);
}

static void
test_source_remove_deletes_carrier_cargo_before_command(void)
{
    start_world();
    Unit *carrier = unit_at(100, 11, SIDE, 0, 500, 500);
    Unit *passenger = unit_at(0, 0, SIDE, 0, 900, 900);
    assert(carrier && passenger);
    passenger->order = ORDER_TRANSPORTED;
    passenger->target = 100;
    supply_used = unit_supply(U_MARINE);

    Sc64T4Action remove;
    memset(&remove, 0, sizeof(remove));
    remove.opcode = 24; /* RemoveUnit */
    remove.player = 0;
    remove.unit = 11; /* Dropship */
    remove.flags = 0x10;
    assert(race_t5_remove_action(&remove, 0) == 1);
    assert(!carrier->alive && !passenger->alive);
    assert(supply_used == 0);

    Sc64T4Condition command;
    memset(&command, 0, sizeof(command));
    command.opcode = 2;
    command.player = 0;
    command.unit = 0;
    command.comparison = 0;
    u32 count = 99;
    assert(sc64_t5_world_count(&race_t5, &command, 0, &count) == 1);
    assert(count == 0);
}

int
main(void)
{
    test_source_bounds_and_flying_class();
    test_command_includes_cargo_and_bring_at_most_excludes_it();
    test_completed_bring_uses_carrier_rect_and_elevation();
    test_completed_bring_excludes_incomplete_building_but_at_most_keeps_it();
    test_tower_is_known_nontransportable();
    test_target_scan_skips_loaded_passenger();
    test_source_remove_deletes_carrier_cargo_before_command();
    puts("sc64_transport_queries: ok");
    return 0;
}
