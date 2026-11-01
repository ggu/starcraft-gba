/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Bucket membership remains correct when a newly-created or reused slot moves. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"

static void
set_actor(int slot, int type, int x, int y, int alive)
{
    memset(&u[slot], 0, sizeof(u[slot]));
    u[slot].type = type;
    u[slot].x = x;
    u[slot].y = y;
    u[slot].alive = alive;
    u[slot].order = ORDER_IDLE;
}

static void
reset_world(int peer)
{
    memset(u, 0, sizeof(u));
    set_actor(0, U_MARINE, 10, 10, 1);
    if (peer) set_actor(1, U_MARINE, 10, 10, 1);
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
}

static void
test_birth_move_preserves_old_cell(void)
{
    int old_cell;
    reset_world(1);
    old_cell = sc64_unit_bucket_cell_at(10, 10);
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);

    /* The slot is born after begin in the same cell, then moves. */
    set_actor(2, U_MARINE, 11, 10, 1);
    sc64_unit_bucket_move(&u[2]);
    assert(sc64_unit_bucket_head[old_cell] != 0xff);
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);

    /* A post-begin birth in a different cell is indexed at that new cell. */
    set_actor(3, U_MARINE, 100, 10, 1);
    sc64_unit_bucket_move(&u[3]);
    assert(sc64_unit_bucket_head[old_cell] != 0xff);
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);
}

static void
test_reused_linked_slot_move(void)
{
    int old_cell;
    reset_world(0);
    set_actor(1, U_MARINE, 10, 10, 1);
    sc64_unit_buckets_begin();
    old_cell = sc64_unit_bucket_cell_at(10, 10);

    /* Death leaves its linked node in place; slot reuse changes its actor. */
    u[1].alive = 0;
    set_actor(1, U_ZERGLING, 10, 10, 1);
#ifdef SC64_CONTACT_GROUP_INDEX
    sc64_unit_bucket_move(&u[1]);
#endif
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);

    /* Moving the reused slot unlinks it from its recorded old cell only. */
    u[1].x = 100;
    sc64_unit_bucket_move(&u[1]);
    assert(sc64_unit_bucket_head[old_cell] == 0);
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0u);
}

static void
test_linked_death_and_load_are_dynamic(void)
{
    reset_world(1);
    u[1].alive = 0;
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0u);
    u[1].alive = 1;
    u[1].order = ORDER_TRANSPORTED;
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0u);
    u[1].order = ORDER_IDLE;
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);

    /* Loading at begin omits the slot; unloading later does not insert it. */
    reset_world(1);
    u[1].order = ORDER_TRANSPORTED;
    sc64_unit_buckets_begin();
    u[1].order = ORDER_IDLE;
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0u);
}

static void
test_type_morph_rekey(void)
{
    reset_world(0);
    set_actor(1, U_Z7_EGG, 10, 10, 1);
    sc64_unit_buckets_begin();
    u[1].type = U_ZERGLING;
#ifdef SC64_CONTACT_GROUP_INDEX
    sc64_unit_bucket_group_rekey(1, sc64_unit_bucket_cell[1]);
#endif
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);
}

#ifdef SC64_CONTACT_GROUP_INDEX
static int
group_contains(int cell, int wanted)
{
    int rep;
    for (rep = sc64_unit_bucket_head[cell]; rep != 0xff;
         rep = sc64_unit_bucket_group_next[rep]) {
        int member;
        for (member = rep; member != 0xff; member = sc64_unit_bucket_next[member])
            if (member == wanted) return 1;
    }
    return 0;
}
#endif

static void
test_representative_promotion_and_bucket_edges(void)
{
#ifdef SC64_CONTACT_GROUP_INDEX
    int old_cell, new_cell;
    reset_world(0);
    set_actor(0, U_ZERGLING, 15, 15, 1);
    set_actor(1, U_MARINE, 15, 15, 1);
    set_actor(2, U_MARINE, 15, 15, 1);
    set_actor(3, U_MARINE, 14, 15, 1); /* same cell, distinct exact key */
    sc64_unit_buckets_begin();
    old_cell = sc64_unit_bucket_cell_at(15, 15);
    assert(sc64_unit_bucket_prev[1] == 1);
    assert(sc64_unit_bucket_prev[2] == 1);
    assert(group_contains(old_cell, 1) && group_contains(old_cell, 2));
    assert(group_contains(old_cell, 3));

    /* Moving the representative promotes its identical peer in place. */
    u[1].x = 16;
    sc64_unit_bucket_move(&u[1]);
    new_cell = sc64_unit_bucket_cell_at(16, 15);
    assert(sc64_unit_bucket_prev[2] == 2);
    assert(group_contains(old_cell, 2) && group_contains(old_cell, 3));
    assert(group_contains(new_cell, 1));
    assert(sc64_contact_blocked_mask(&u[0], 0) == 0xffu);

    /* Both adjacent in-map edge cells are valid index locations. */
    set_actor(4, U_MARINE, WORLD_W - 1, WORLD_H - 1, 1);
    sc64_unit_bucket_move(&u[4]);
    assert(sc64_unit_bucket_cell[4] ==
           sc64_unit_bucket_cell_at(WORLD_W - 1, WORLD_H - 1));
#endif
}

static unsigned
reference_mask(const Unit *q, int speed)
{
    static const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    unsigned mask = 0;
    int k;
    for (k = 0; k < 8; ++k)
        if (!sc64_unit_bucket_step_clear_reference(
                q, q->x + dx[k] * speed, q->y + dy[k] * speed, 0))
            mask |= 1u << k;
    return mask;
}

static void
test_stale_snapshot_group_falls_back_per_member(void)
{
#ifdef SC64_CONTACT_GROUP_INDEX
    unsigned want, got;
    reset_world(0);
    set_actor(0, U_ZERGLING, 15, 15, 1);
    set_actor(1, U_MARINE, 15, 15, 1);
    set_actor(2, U_MARINE, 15, 15, 1);
    sc64_unit_buckets_begin();

    /* A post-snapshot mission callback can move one actor without rekeying.
     * The base index still visits its old cell, but its member geometry is
     * now different; the grouped query must inspect both current positions. */
    u[1].x = 31;
    want = reference_mask(&u[0], 0);
    got = sc64_contact_blocked_mask(&u[0], 0);
    assert(want == 0xffu);
    assert(got == want);

    /* The same fallback applies to an unhooked in-place type morph. */
    u[1].x = 15;
    u[1].type = U_DRONE;
    want = reference_mask(&u[0], 0);
    got = sc64_contact_blocked_mask(&u[0], 0);
    assert(got == want);
#endif
}

int
main(void)
{
    test_birth_move_preserves_old_cell();
    test_reused_linked_slot_move();
    test_linked_death_and_load_are_dynamic();
    test_type_morph_rekey();
    test_representative_promotion_and_bucket_edges();
    test_stale_snapshot_group_falls_back_per_member();
    return 0;
}
