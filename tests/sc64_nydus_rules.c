/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free Nydus link, eligibility, ownership and exit-search rules. The
 * record shapes mirror Z8's two reciprocal pairs without carrying source
 * coordinates or any other extracted scenario values. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_nydus_rules.h"

static Sc64NydusSourceRecord rec(unsigned index, unsigned linked, int x, int y)
{
    Sc64NydusSourceRecord r;
    memset(&r, 0, sizeof(r));
    r.record_index = (unsigned short)index;
    r.linked_record_index = (unsigned short)linked;
    r.raw_type = SC64_NYDUS_RAW;
    r.relation = SC64_NYDUS_RELATION_LINK;
    r.raw_owner = 3;
    r.x = (short)x;
    r.y = (short)y;
    return r;
}

static void fixture(Sc64NydusSourceRecord *r)
{
    /* Deliberately unsorted so pair ordering is exercised. */
    r[0] = rec(20, 22, 300, 300);
    r[1] = rec(10, 12, 100, 100);
    r[2] = rec(22, 20, 400, 300);
    r[3] = rec(12, 10, 200, 100);
}

static void source_pairs_validate_and_order(void)
{
    Sc64NydusSourceRecord r[4], bad[4];
    Sc64NydusSourcePair p[4];
    unsigned n = 99;
    fixture(r);
    assert(sc64_nydus_source_pairs(r, 4, p, &n) && n == 2);
    assert(r[p[0].a].record_index == 10 && r[p[0].b].record_index == 12);
    assert(r[p[1].a].record_index == 20 && r[p[1].b].record_index == 22);

    assert(!sc64_nydus_source_pairs(r, 0, p, &n) && n == 0);
    assert(!sc64_nydus_source_pairs(r, 3, p, &n));
    assert(!sc64_nydus_source_pairs(NULL, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[1].raw_type = 131;
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[1].relation = 0x0200; /* add-on link */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[1].linked_record_index = 99; /* absent */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[1].linked_record_index = 10; /* self */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[3].linked_record_index = 20; /* one-way */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[3].raw_owner = 4;
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[3].record_index = 20; /* duplicate id */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[3].x = 100; /* duplicate position */
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    memcpy(bad, r, sizeof(bad)); bad[3].linked_record_index = SC64_NYDUS_NO_RECORD;
    assert(!sc64_nydus_source_pairs(bad, 4, p, &n));
    {
        /* A three-way chain cannot form a reciprocal pair table. */
        Sc64NydusSourceRecord chain[4];
        chain[0] = rec(1, 2, 10, 10); chain[1] = rec(2, 1, 20, 10);
        chain[2] = rec(3, 2, 30, 10); chain[3] = rec(4, 3, 40, 10);
        assert(!sc64_nydus_source_pairs(chain, 4, p, &n));
    }
}

static Sc64NydusEndpointView view(int x, int y, int owner)
{
    Sc64NydusEndpointView v;
    memset(&v, 0, sizeof(v));
    v.x = (short)x; v.y = (short)y; v.raw_type = SC64_NYDUS_RAW;
    v.alive = v.complete = 1; v.owner = (unsigned char)owner;
    v.slot = SC64_NYDUS_NO_SLOT;
    return v;
}

static void binding_is_stable_and_fail_closed(void)
{
    Sc64NydusSourceRecord r[4];
    Sc64NydusSourcePair p[2];
    Sc64NydusEndpointView v[8];
    Sc64NydusState s;
    unsigned n;
    fixture(r);
    assert(sc64_nydus_source_pairs(r, 4, p, &n) && n == 2);
    memset(v, 0, sizeof(v));
    /* Unrelated building, endpoints in shuffled slots. */
    v[0] = view(100, 100, 0); v[0].raw_type = 131;
    v[5] = view(100, 100, 1); v[2] = view(200, 100, 1);
    v[7] = view(300, 300, 1); v[1] = view(400, 300, 1);
    for (unsigned i = 0; i < 8; ++i) v[i].slot = (unsigned char)i;
    assert(sc64_nydus_bind(r, p, n, v, 8, 1, &s));
    assert(s.count == 2 && sc64_nydus_state_valid(&s, 8));
    assert(s.link[0].active && s.link[0].slot[0] == 5 && s.link[0].slot[1] == 2);
    assert(s.link[1].active && s.link[1].slot[0] == 7 && s.link[1].slot[1] == 1);
    assert(sc64_nydus_partner(&s, 5) == 2 && sc64_nydus_partner(&s, 2) == 5);
    assert(sc64_nydus_partner(&s, 7) == 1 && sc64_nydus_partner(&s, 0) == -1);

    /* Mixed owners or a duplicate endpoint identity reject the binding. */
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[2].owner = 2;
      assert(!sc64_nydus_bind(r, p, n, w, 8, 1, &s)); }
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[3] = view(400, 300, 1);
      w[3].slot = 3; assert(!sc64_nydus_bind(r, p, n, w, 8, 1, &s)); }
    /* Views must name distinct, real slots; the list is a compact subset. */
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[3].slot = 5;
      assert(!sc64_nydus_bind(r, p, n, w, 8, 1, &s)); }
    { Sc64NydusEndpointView w[4] = { v[5], v[2], v[7], v[1] };
      assert(sc64_nydus_bind(r, p, n, w, 4, 1, &s) &&
             s.link[0].slot[0] == 5 && s.link[1].slot[1] == 1); }
    assert(!sc64_nydus_bind(r, p, n, v, SC64_NYDUS_MAX_VIEWS + 1, 1, &s));
    /* A whole destroyed pair is valid after start, never at start. */
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[7].alive = w[1].alive = 0;
      assert(!sc64_nydus_bind(r, p, n, w, 8, 1, &s));
      assert(sc64_nydus_bind(r, p, n, w, 8, 0, &s));
      assert(s.link[0].active && !s.link[1].active &&
             s.link[1].slot[0] == SC64_NYDUS_NO_SLOT && sc64_nydus_state_valid(&s, 8)); }
    /* A lone survivor is an impossible saved state. */
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[1].alive = 0;
      assert(!sc64_nydus_bind(r, p, n, w, 8, 0, &s)); }
    /* An incomplete endpoint keeps its identity; readiness is separate. */
    { Sc64NydusEndpointView w[8]; memcpy(w, v, sizeof(w)); w[2].complete = 0;
      assert(sc64_nydus_bind(r, p, n, w, 8, 1, &s)); }
    assert(!sc64_nydus_bind(r, p, n, v, 8, 2, &s));
    assert(!sc64_nydus_bind(r, p, 5, v, 8, 1, &s));
}

static void state_validation_rejects_stale_or_duplicate_slots(void)
{
    Sc64NydusState s;
    memset(&s, 0, sizeof(s));
    s.count = 2;
    s.link[0] = (Sc64NydusLink){ { 1, 2 }, 0, 1 };
    s.link[1] = (Sc64NydusLink){ { SC64_NYDUS_NO_SLOT, SC64_NYDUS_NO_SLOT }, 0, 0 };
    assert(sc64_nydus_state_valid(&s, 8));
    s.link[1] = (Sc64NydusLink){ { 2, 3 }, 0, 1 };
    assert(!sc64_nydus_state_valid(&s, 8)); /* slot 2 in two links */
    s.link[1] = (Sc64NydusLink){ { 3, 9 }, 0, 1 };
    assert(!sc64_nydus_state_valid(&s, 8)); /* out of range */
    s.link[1] = (Sc64NydusLink){ { 3, SC64_NYDUS_NO_SLOT }, 0, 0 };
    assert(!sc64_nydus_state_valid(&s, 8)); /* inactive with stale slot */
    s.link[1] = (Sc64NydusLink){ { 3, 4 }, 0, 2 };
    assert(!sc64_nydus_state_valid(&s, 8));
    s.count = SC64_NYDUS_MAX_LINKS + 1;
    assert(!sc64_nydus_state_valid(&s, 8));
}

static void unlink_returns_partner_once(void)
{
    Sc64NydusState s;
    memset(&s, 0, sizeof(s));
    s.count = 2;
    s.link[0] = (Sc64NydusLink){ { 4, 6 }, 1, 1 };
    s.link[1] = (Sc64NydusLink){ { 7, 3 }, 1, 1 };
    assert(sc64_nydus_unlink(&s, 6) == 4);
    assert(!s.link[0].active && sc64_nydus_partner(&s, 4) == -1);
    assert(sc64_nydus_unlink(&s, 4) == -1); /* partner death is not recursive */
    assert(sc64_nydus_partner(&s, 7) == 3 && sc64_nydus_state_valid(&s, 8));
}

static void ownership_follows_the_transferred_endpoint(void)
{
    Sc64NydusLink l = { { 1, 2 }, 0, 1 };
    assert(sc64_nydus_owner_source(&l, 0, 0) == -1);
    assert(sc64_nydus_owner_source(&l, 3, 0) == 0);
    assert(sc64_nydus_owner_source(&l, 0, 3) == 1);
    assert(sc64_nydus_owner_source(&l, 3, 4) == 0);
    l.active = 0;
    assert(sc64_nydus_owner_source(&l, 3, 0) == -1);
}

static void eligibility_matches_the_reference_predicate(void)
{
    Sc64NydusEndpointView in = view(10, 10, 1), out = view(90, 10, 1);
    Sc64NydusPassengerView ling = { 1, 1, 0, 1, 1, 0, 0 }, p;
    assert(sc64_nydus_can_enter(&ling, &in, &out));
    p = ling; p.owner = 2; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.flying = 1; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.zerg = 0; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.movable = 0; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.loaded = 1; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.burrowed = 1; assert(!sc64_nydus_can_enter(&p, &in, &out));
    p = ling; p.alive = 0; assert(!sc64_nydus_can_enter(&p, &in, &out));
    { Sc64NydusEndpointView e = in; e.complete = 0; assert(!sc64_nydus_can_enter(&ling, &e, &out)); }
    { Sc64NydusEndpointView e = out; e.complete = 0; assert(!sc64_nydus_can_enter(&ling, &in, &e)); }
    { Sc64NydusEndpointView e = out; e.alive = 0; assert(!sc64_nydus_can_enter(&ling, &in, &e)); }
    { Sc64NydusEndpointView e = in; e.raw_type = 131; assert(!sc64_nydus_can_enter(&ling, &e, &out)); }
    assert(!sc64_nydus_can_enter(&ling, &in, NULL));
    assert(sc64_nydus_contact(10 + 32, 10 - 32, 10, 10));
    assert(!sc64_nydus_contact(10 + 33, 10, 10, 10));
}

static void exit_candidates_are_unique_bounded_and_nearest_first(void)
{
    static unsigned char seen[129][129];
    int prev_ring = 0, x, y;
    memset(seen, 0, sizeof(seen));
    for (unsigned n = 0; n < SC64_NYDUS_EXIT_CANDIDATES; ++n) {
        int dx, dy, ring;
        assert(sc64_nydus_exit_candidate(n, 500, 600, &x, &y));
        dx = x - 500; dy = y - 600;
        assert(dx % 8 == 0 && dy % 8 == 0);
        assert(dx >= -64 && dx <= 64 && dy >= -64 && dy <= 64);
        ring = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ?
               (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
        assert(ring > 0 && ring >= prev_ring);
        prev_ring = ring;
        assert(!seen[dx + 64][dy + 64]);
        seen[dx + 64][dy + 64] = 1;
    }
    assert(sc64_nydus_exit_candidate(0, 0, 0, &x, &y) && x == 0 && y == 8);
    assert(!sc64_nydus_exit_candidate(SC64_NYDUS_EXIT_CANDIDATES, 0, 0, &x, &y));
    assert(!sc64_nydus_exit_candidate(0, 0, 0, NULL, &y));
}

typedef struct { unsigned seen, stop; int cx, cy, ok; } ScanCheck;
static int scan_accept(void *ctx, int x, int y)
{
    ScanCheck *c = ctx;
    int ex, ey;
    if (!sc64_nydus_exit_candidate(c->seen, c->cx, c->cy, &ex, &ey) || ex != x || ey != y)
        c->ok = 0;
    return c->seen++ == c->stop;
}

static void division_free_scan_matches_indexed_order(void)
{
    ScanCheck c = { 0, SC64_NYDUS_EXIT_CANDIDATES, -40, 900, 1 };
    int x, y;
    assert(sc64_nydus_exit_scan(c.cx, c.cy, scan_accept, &c, &x, &y) == -1);
    assert(c.ok && c.seen == SC64_NYDUS_EXIT_CANDIDATES);
    c.seen = 0; c.stop = 37;
    assert(sc64_nydus_exit_scan(c.cx, c.cy, scan_accept, &c, &x, &y) == 37 && c.ok);
    assert(sc64_nydus_exit_candidate(37, c.cx, c.cy, &c.cx, &c.cy) && c.cx == x && c.cy == y);
    assert(sc64_nydus_exit_scan(0, 0, NULL, &c, &x, &y) == -1);
}

int main(void)
{
    source_pairs_validate_and_order();
    binding_is_stable_and_fail_closed();
    state_validation_rejects_stale_or_duplicate_slots();
    unlink_returns_partner_once();
    ownership_follows_the_transferred_endpoint();
    eligibility_matches_the_reference_predicate();
    exit_candidates_are_unique_bounded_and_nearest_first();
    division_free_scan_matches_indexed_order();
    puts("sc64 nydus rules: ok");
    return 0;
}
