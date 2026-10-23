#include <assert.h>
#include <limits.h>
#include <stdio.h>

#define SC64_T4_RUNTIME 1
#include "starcraft/sc64_t4_queries.h"

static void test_rectangles(void)
{
    const Sc64T4QueryRect location = {100, 200, 140, 240};
    const Sc64T4QueryRect inside = {100, 200, 101, 201};
    const Sc64T4QueryRect overlap_left = {90, 210, 100, 220};
    const Sc64T4QueryRect actor_center_outside_but_bounds_overlap = {90, 210, 105, 220};
    const Sc64T4QueryRect touching_right = {140, 210, 150, 220};
    const Sc64T4QueryRect touching_bottom = {110, 240, 120, 250};
    const Sc64T4QueryRect reversed = {130, 220, 120, 230};
    const Sc64T4QueryRect empty = {110, 220, 110, 230};
    assert(sc64_t4_query_rect_overlap(&location, &inside));
    assert(sc64_t4_query_rect_overlap(&location, &actor_center_outside_but_bounds_overlap));
    assert(!sc64_t4_query_rect_overlap(&location, &overlap_left));
    assert(!sc64_t4_query_rect_overlap(&location, &touching_right));
    assert(!sc64_t4_query_rect_overlap(&location, &touching_bottom));
    assert(!sc64_t4_query_rect_overlap(&location, &reversed));
    assert(!sc64_t4_query_rect_overlap(&location, &empty));
    assert(!sc64_t4_query_rect_overlap(NULL, &inside));
    assert(!sc64_t4_query_rect_overlap(&location, NULL));
}

static void test_actor_rectangles(void)
{
    static const struct { unsigned raw; int l, u, r, d; } expected[] = {
        {0,8,9,8,10}, {1,7,10,7,11}, {2,16,16,15,15},
        {3,16,16,15,15}, {7,11,11,11,11}, {15,8,9,8,10},
        {20,8,9,8,10}, {32,11,7,11,14}, {37,8,4,7,11},
        {89,16,16,15,15}, {95,16,16,15,15}, {101,13,13,13,17},
        {195,48,32,47,31}, {218,16,16,15,15}
    };
    Sc64T4QueryRect r;
    for (unsigned i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) {
        assert(sc64_t4_query_actor_rect(expected[i].raw, 100, 200, &r));
        assert(r.left == 100 - expected[i].l);
        assert(r.top == 200 - expected[i].u);
        assert(r.right == 100 + expected[i].r + 1);
        assert(r.bottom == 200 + expected[i].d + 1);
    }

    /* Marine and Ghost asymmetry must survive conversion to half-open bounds. */
    assert(sc64_t4_query_actor_rect(0, 100, 100, &r));
    assert(r.left == 92 && r.top == 91 && r.right == 109 && r.bottom == 111);
    assert(sc64_t4_query_actor_rect(1, 100, 100, &r));
    assert(r.left == 93 && r.top == 90 && r.right == 108 && r.bottom == 112);
    /* Beacon is asymmetric and much wider; no generic building fallback. */
    assert(sc64_t4_query_actor_rect(195, 100, 100, &r));
    assert(r.left == 52 && r.top == 68 && r.right == 148 && r.bottom == 132);

    assert(sc64_t4_query_actor_rect(0, 100, 100, &r));
    {
        Sc64T4QueryRect center_outside = {90, 95, 93, 96};
        Sc64T4QueryRect exact_left_edge = {80, 95, r.left, 96};
        assert(sc64_t4_query_rect_overlap(&center_outside, &r));
        assert(!sc64_t4_query_rect_overlap(&exact_left_edge, &r));
    }
    assert(!sc64_t4_query_actor_rect(194, 100, 100, &r));
    assert(!sc64_t4_query_actor_rect(219, 100, 100, &r));
    assert(!sc64_t4_query_actor_rect(0, INT_MIN, 100, &r));
    assert(!sc64_t4_query_actor_rect(0, INT_MAX, 100, &r));
    assert(!sc64_t4_query_actor_rect(0, 100, INT_MAX, &r));
    assert(!sc64_t4_query_actor_rect(0, 100, 100, NULL));
}

static void test_owner_groups(void)
{
    /* Ordinary enemy: raw slot 3 is still current owner 3 / Force 2. */
    assert(sc64_t4_query_owner_matches(3, 0, SC64_T4_OWNER_ENEMY, 3) == 1);
    assert(sc64_t4_query_owner_matches(13, 3, SC64_T4_OWNER_ENEMY, 3) == 1);
    assert(sc64_t4_query_owner_matches(17, 0, SC64_T4_OWNER_ENEMY, 3) == 1);
    assert(sc64_t4_query_owner_matches(19, 0, SC64_T4_OWNER_ENEMY, 3) == 1);
    assert(sc64_t4_query_owner_matches(18, 0, SC64_T4_OWNER_ENEMY, 3) == 0);

    /* A rescued slot-1 actor is counted as the current SIDE/slot-0 actor;
     * its raw slot remains provenance and no longer matches Force 3. */
    assert(sc64_t4_query_owner_matches(0, 0, SC64_T4_OWNER_SIDE, 1) == 1);
    assert(sc64_t4_query_owner_matches(1, 0, SC64_T4_OWNER_SIDE, 1) == 0);
    assert(sc64_t4_query_owner_matches(13, 0, SC64_T4_OWNER_SIDE, 1) == 1);
    assert(sc64_t4_query_owner_matches(17, 0, SC64_T4_OWNER_SIDE, 1) == 1);
    assert(sc64_t4_query_owner_matches(18, 0, SC64_T4_OWNER_SIDE, 1) == 1);
    assert(sc64_t4_query_owner_matches(20, 0, SC64_T4_OWNER_SIDE, 1) == 0);

    /* A still-rescuable slot-7 actor is force 3, not All Players excluded. */
    assert(sc64_t4_query_owner_matches(7, 0, SC64_T4_OWNER_RESCUABLE, 7) == 1);
    assert(sc64_t4_query_owner_matches(17, 0, SC64_T4_OWNER_RESCUABLE, 7) == 1);
    assert(sc64_t4_query_owner_matches(20, 0, SC64_T4_OWNER_RESCUABLE, 7) == 1);

    /* Trigger selection first gates through OpenBW's active player slots,
     * which rejects every slot >=8, including map-neutral slot 11. */
    assert(sc64_t4_query_owner_matches(11, 0, SC64_T4_OWNER_NEUTRAL, 11) == 0);
    assert(sc64_t4_query_owner_matches(17, 0, SC64_T4_OWNER_NEUTRAL, 11) == 0);
    assert(sc64_t4_query_owner_matches(18, 0, SC64_T4_OWNER_NEUTRAL, 11) == 0);

    assert(sc64_t4_query_slot_active(0));
    assert(sc64_t4_query_slot_active(4));
    assert(!sc64_t4_query_slot_active(5));
    assert(sc64_t4_query_slot_active(6));
    assert(sc64_t4_query_slot_active(7));
    assert(!sc64_t4_query_slot_active(8));
    assert(!sc64_t4_query_slot_active(11));
    assert(sc64_t4_query_owner_matches(5, 0, SC64_T4_OWNER_FORCE4, 6) == 0);
    assert(sc64_t4_query_owner_matches(21, 0, SC64_T4_OWNER_FORCE4, 6) == 1);
    assert(sc64_t4_query_owner_matches(13, 5, SC64_T4_OWNER_ENEMY, 3) == 0);

    /* Unsupported raw selectors must be visible to the caller, not silently
     * evaluated as false. */
    assert(sc64_t4_query_owner_matches(12, 0, SC64_T4_OWNER_SIDE, 0) == 0);
    assert(sc64_t4_query_owner_matches(14, 0, SC64_T4_OWNER_SIDE, 0) == -1);
    assert(sc64_t4_query_owner_matches(22, 0, SC64_T4_OWNER_SIDE, 0) == -1);
}

int main(void)
{
    test_rectangles();
    test_actor_rectangles();
    test_owner_groups();
    puts("T4 query helpers: ok");
    return 0;
}
