/* Source-pinned P5 Shuttle and Protoss passenger space facts. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_transport_rules.h"

static void test_p5_space_facts(void)
{
    unsigned space = 0;
#ifdef SC64_P5_SHUTTLE_ENABLED
    static const unsigned short full[] = { 66, 65 }; /* 4 + 2 */
    static const unsigned short partial[] = { 64, 65, 79 }; /* 1 + 2 + 2 */
#endif

#ifdef SC64_P5_SHUTTLE_ENABLED
    assert(sc64_transport_space_required_raw(64, &space) && space == 1); /* Probe */
    assert(sc64_transport_space_required_raw(65, &space) && space == 2); /* Zealot */
    assert(sc64_transport_space_required_raw(66, &space) && space == 4); /* Dragoon */
    assert(sc64_transport_space_required_raw(67, &space) && space == 2); /* High Templar */
    assert(sc64_transport_space_required_raw(79, &space) && space == 2); /* Tassadar */
    assert(sc64_transport_space_required_raw(69, &space) &&
           space == SC64_TRANSPORT_SPACE_UNAVAILABLE); /* Shuttle is not cargo */
#else
    assert(!sc64_transport_space_required_raw(64, &space));
    assert(!sc64_transport_space_required_raw(65, &space));
    assert(!sc64_transport_space_required_raw(66, &space));
    assert(!sc64_transport_space_required_raw(67, &space));
    assert(!sc64_transport_space_required_raw(79, &space));
    assert(!sc64_transport_space_required_raw(69, &space));
#endif

#ifdef SC64_P5_SHUTTLE_ENABLED
    assert(sc64_transport_space_provided_raw(69) == 8);
    assert(sc64_transport_load_fits_raw(69, 0, 0, 79));
    assert(sc64_transport_load_fits_raw(69, full, 2, 79)); /* exactly 8 */
    assert(sc64_transport_load_fits_raw(69, full, 2, 64)); /* 7 <= 8 */
    assert(!sc64_transport_load_fits_raw(69, full, 2, 66)); /* 10 > 8 */
    assert(sc64_transport_load_fits_raw(69, partial, 3, 65)); /* exactly 7 */
    assert(!sc64_transport_load_fits_raw(69, partial, 3, 66)); /* 5 + 4 > 8 */
#else
    assert(sc64_transport_space_provided_raw(69) == 0);
    assert(!sc64_transport_load_fits_raw(69, 0, 0, 79));
#endif
}

static void test_unknown_and_nontransportable_rejected(void)
{
    unsigned space = 0;
    static const unsigned short invalid[] = { 65, 13 };
    assert(!sc64_transport_space_required_raw(250, &space));
    assert(sc64_transport_space_required_raw(12, &space) &&
           space == SC64_TRANSPORT_SPACE_UNAVAILABLE);
    assert(!sc64_transport_load_fits_raw(69, invalid, 2, 64));
}

int main(void)
{
    test_p5_space_facts();
    test_unknown_and_nontransportable_rejected();
    puts("P5 Shuttle transport facts: ok");
    return 0;
}
