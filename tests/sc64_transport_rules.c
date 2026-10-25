/* Asset-free checks for the verified raw-type cargo-space facts. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_transport_rules.h"

static void test_capacity_and_required_space(void)
{
    unsigned space = 0x1234u;
    assert(sc64_transport_space_provided_raw(11) == 8);
    assert(sc64_transport_space_provided_raw(0) == 0);
    assert(sc64_transport_space_provided_raw(109) == 0);

    assert(sc64_transport_space_required_raw(0, &space) && space == 1); /* Marine */
    assert(sc64_transport_space_required_raw(1, &space) && space == 1); /* Ghost */
    assert(sc64_transport_space_required_raw(2, &space) && space == 2); /* Vulture */
    assert(sc64_transport_space_required_raw(3, &space) && space == 2); /* Goliath */
    assert(sc64_transport_space_required_raw(5, &space) && space == 4); /* Tank mode */
    assert(sc64_transport_space_required_raw(7, &space) && space == 1); /* SCV */
    assert(sc64_transport_space_required_raw(15, &space) && space == 1); /* Civilian */
    assert(sc64_transport_space_required_raw(16, &space) && space == 1); /* Kerrigan */
    assert(sc64_transport_space_required_raw(19, &space) && space == 2); /* Raynor Vulture */
    assert(sc64_transport_space_required_raw(20, &space) && space == 1); /* Raynor Marine */
    assert(sc64_transport_space_required_raw(23, &space) && space == 4); /* Duke tank */
    assert(sc64_transport_space_required_raw(32, &space) && space == 1); /* Firebat */
    assert(sc64_transport_space_required_raw(34, &space) && space == 1); /* Medic */
    assert(sc64_transport_space_required_raw(37, &space) && space == 1); /* Zergling */
    assert(sc64_transport_space_required_raw(38, &space) && space == 2); /* Hydralisk */
    assert(sc64_transport_space_required_raw(41, &space) && space == 1); /* Drone */
    assert(!sc64_transport_space_required_raw(250, &space) && space == 1);
#ifdef SC64_Z6_TRANSPORT_ENABLED
    assert(sc64_transport_space_provided_raw(42) == 8);
    assert(sc64_transport_space_required_raw(51, &space) && space == 1); /* Kerrigan */
    assert(sc64_transport_space_required_raw(39, &space) && space == 4); /* Ultralisk */
    assert(sc64_transport_space_required_raw(42, &space) &&
           space == SC64_TRANSPORT_SPACE_UNAVAILABLE); /* carrier, not cargo */
    assert(!sc64_transport_passenger_eligible_raw(42));
    {
        static const unsigned short zerg_capacity[] = { 39 };
        static const unsigned short zerg_overflow[] = { 39, 39 };
        assert(sc64_transport_load_fits_raw(42, zerg_capacity, 1, 51));
        assert(!sc64_transport_load_fits_raw(42, zerg_overflow, 2, 51));
    }
#endif
}

static void test_nontransportable_types(void)
{
    static const unsigned short nontransportable[] = {
        8, 9, 11, 12, 13, 14, 25, 30, 42, 43, 45, 47,
        106, 109, 110, 111, 112, 113, 114, 122, 124, 125, 130, 131,
        132, 133, 135, 137, 139, 141, 142, 143, 144, 146, 149, 150,
        151, 154, 155, 156, 157, 160, 162, 164, 166, 167, 194, 195,
        203, 205, 206, 207, 208, 209, 211, 212
    };
    for (unsigned i = 0; i < sizeof(nontransportable) / sizeof(nontransportable[0]); ++i) {
        unsigned space = 0;
        unsigned raw = nontransportable[i];
        assert(sc64_transport_space_required_raw(raw, &space));
        assert(space == SC64_TRANSPORT_SPACE_UNAVAILABLE);
        assert(!sc64_transport_passenger_eligible_raw(raw));
        assert(!sc64_transport_load_fits_raw(11, 0, 0, raw));
    }
    /* Raw106/195 are building types; their source spaceRequired is 255. */
    assert(!sc64_transport_passenger_eligible_raw(195));
}

static void test_mixed_cargo_accounting(void)
{
    static const unsigned short full_mixed[] = {5, 2, 3}; /* 4 + 2 + 2 */
    static const unsigned short partial_mixed[] = {5, 2}; /* 4 + 2 */
    static const unsigned short invalid_manifest[] = {0, 13};
    assert(sc64_transport_load_fits_raw(11, 0, 0, 0));
    assert(sc64_transport_load_fits_raw(11, full_mixed, 3, 0) == 0);
    assert(!sc64_transport_load_fits_raw(11, full_mixed, 3, 7));
    assert(sc64_transport_load_fits_raw(11, partial_mixed, 2, 3));
    assert(!sc64_transport_load_fits_raw(11, partial_mixed, 2, 5)); /* 6 + 4 */
    assert(!sc64_transport_load_fits_raw(11, invalid_manifest, 2, 0));
    assert(!sc64_transport_load_fits_raw(11, 0, 1, 0));
    assert(!sc64_transport_load_fits_raw(0, 0, 0, 0));
    assert(!sc64_transport_load_fits_raw(11, partial_mixed,
                                          SC64_TRANSPORT_MAX_PASSENGERS + 1u, 0));
}

int main(void)
{
    test_capacity_and_required_space();
    test_nontransportable_types();
    test_mixed_cargo_accounting();
    puts("transport type rules: ok");
    return 0;
}
