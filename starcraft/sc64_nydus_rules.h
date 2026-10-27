/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Freestanding Nydus Canal link and transit rules.
 *
 * Source-data facts (authenticated Z8 CHK UNIT rows; see docs/NYDUS-CANAL.md):
 * - Nydus Canal is raw UnitType 134. Each preplaced endpoint row carries the
 *   relation bit 0x0400 and a 32-bit serial naming exactly one partner row;
 *   the links are reciprocal. Every Z8 endpoint is owned by one source slot.
 *
 * Pinned PC reference policy (OpenBW bwgame.h; not native N64 code):
 * - unit_can_enter_nydus(): the entrance and its exit must both be complete;
 *   the passenger has the entrance's owner, is not flying and is Zerg.
 * - order_EnterNydusCanal(): approach, then hide, place near the exit and
 *   finish in the same order step. Failed placement restores the passenger at
 *   its previous position and retries on its next order step. A passenger is
 *   therefore never "inside" a canal across frames.
 * - Nydus death kills the linked partner; ownership transfer carries it.
 *
 * GBA-specific policies are labelled where they occur. This header owns no
 * engine state and performs no allocation; every loop has a fixed bound.
 */
#ifndef SC64_NYDUS_RULES_H
#define SC64_NYDUS_RULES_H

enum {
    SC64_NYDUS_RAW = 134,
    /* CHK UNIT relation flag for a Nydus partner link. */
    SC64_NYDUS_RELATION_LINK = 0x0400,
    /* Pinned BWAPI UnitType tables: 250 HP, 1 armor, 150 minerals, 0 gas,
     * 600 build frames, 8-tile sight, 2x2 tiles, 32/32/31/31 dimensions,
     * Large size, and 255 (not transportable) space required. */
    SC64_NYDUS_HP = 250,
    SC64_NYDUS_ARMOR = 1,
    SC64_NYDUS_MINERALS = 150,
    SC64_NYDUS_BUILD_FRAMES = 600,
    SC64_NYDUS_SIGHT_PX = 256,
    SC64_NYDUS_MAX_SOURCE_RECORDS = 8,
    SC64_NYDUS_MAX_LINKS = SC64_NYDUS_MAX_SOURCE_RECORDS / 2,
    /* Candidate endpoints a caller may collect for one bind: every source
     * position plus one duplicate each, enough to prove ambiguity. */
    SC64_NYDUS_MAX_VIEWS = 2 * SC64_NYDUS_MAX_SOURCE_RECORDS,
    SC64_NYDUS_NO_RECORD = 0xffff,
    SC64_NYDUS_NO_SLOT = 0xff,
    /* OpenBW find_unit_placement(u, pos, false) searches a 256x256 source
     * pixel box centred on the exit: +/-64 GBA pixels at the half scale. */
    SC64_NYDUS_EXIT_SEARCH_PX = 64,
    /* GBA policy: candidate spacing and ring order are authored; OpenBW's
     * own search order is not reproduced. */
    SC64_NYDUS_EXIT_STEP_PX = 8,
    SC64_NYDUS_EXIT_RINGS = SC64_NYDUS_EXIT_SEARCH_PX / SC64_NYDUS_EXIT_STEP_PX,
    SC64_NYDUS_EXIT_CANDIDATES = 4 * SC64_NYDUS_EXIT_RINGS *
                                 (SC64_NYDUS_EXIT_RINGS + 1),
    /* GBA policy: OpenBW enters when its path reaches the target. The GBA
     * router stops beside the inflated 2x2 footprint, so contact is a box of
     * half-size footprint (16) plus this slack around the endpoint centre. */
    SC64_NYDUS_HALF_FOOTPRINT_PX = 16,
    SC64_NYDUS_CONTACT_SLACK_PX = 16
};

_Static_assert(SC64_NYDUS_EXIT_CANDIDATES == 288,
               "Nydus exit search must stay bounded");

/* One preplaced source endpoint. x/y are the runtime world-pixel centre at
 * which the caller spawned that endpoint; record indexes are source UNIT row
 * indexes, giving stable identity independent of runtime slots. */
typedef struct {
    unsigned short record_index, linked_record_index;
    unsigned short raw_type, relation;
    unsigned char raw_owner, reserved;
    short x, y;
} Sc64NydusSourceRecord;

/* Indexes into the caller's record array, lower record index first. */
typedef struct {
    unsigned char a, b;
} Sc64NydusSourcePair;

/* A live raw-134 building, identified by its runtime building slot. */
typedef struct {
    short x, y;
    unsigned short raw_type;
    unsigned char alive, complete, owner, slot;
} Sc64NydusEndpointView;

/* Runtime link: two building slots and the owner both endpoints share. */
typedef struct {
    unsigned char slot[2];
    unsigned char owner, active;
} Sc64NydusLink;

typedef struct {
    Sc64NydusLink link[SC64_NYDUS_MAX_LINKS];
    unsigned char count, reserved[3];
} Sc64NydusState;

typedef struct {
    unsigned char alive, owner, flying, zerg, movable, loaded, burrowed;
} Sc64NydusPassengerView;

/* Validate the complete source link table and emit its pairs. Any malformed
 * row rejects the whole table: wrong type or relation flag, missing, self or
 * non-reciprocal partner, duplicate identity/position, or mixed owners. */
static inline int sc64_nydus_source_pairs(const Sc64NydusSourceRecord *rec,
                                          unsigned count,
                                          Sc64NydusSourcePair *out,
                                          unsigned *out_count)
{
    unsigned pairs = 0, used = 0;
    if (!out_count) return 0;
    *out_count = 0;
    if (!rec || !out || count == 0 || (count & 1u) ||
        count > SC64_NYDUS_MAX_SOURCE_RECORDS) return 0;
    for (unsigned i = 0; i < count; ++i) {
        if (rec[i].raw_type != SC64_NYDUS_RAW ||
            !(rec[i].relation & SC64_NYDUS_RELATION_LINK) ||
            rec[i].record_index == SC64_NYDUS_NO_RECORD ||
            rec[i].linked_record_index == SC64_NYDUS_NO_RECORD ||
            rec[i].linked_record_index == rec[i].record_index ||
            rec[i].x < 0 || rec[i].y < 0) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (rec[j].record_index == rec[i].record_index ||
                (rec[j].x == rec[i].x && rec[j].y == rec[i].y)) return 0;
    }
    for (unsigned i = 0; i < count; ++i) {
        unsigned partner = count;
        if (used & (1u << i)) continue;
        for (unsigned j = 0; j < count; ++j)
            if (rec[j].record_index == rec[i].linked_record_index) partner = j;
        if (partner == count || (used & (1u << partner)) ||
            rec[partner].linked_record_index != rec[i].record_index ||
            rec[partner].raw_owner != rec[i].raw_owner) return 0;
        used |= (1u << i) | (1u << partner);
        if (rec[i].record_index < rec[partner].record_index) {
            out[pairs].a = (unsigned char)i; out[pairs].b = (unsigned char)partner;
        } else {
            out[pairs].a = (unsigned char)partner; out[pairs].b = (unsigned char)i;
        }
        ++pairs;
    }
    /* Deterministic order: ascending lower record index. */
    for (unsigned i = 1; i < pairs; ++i)
        for (unsigned j = i; j > 0 &&
             rec[out[j - 1].a].record_index > rec[out[j].a].record_index; --j) {
            Sc64NydusSourcePair t = out[j - 1]; out[j - 1] = out[j]; out[j] = t;
        }
    *out_count = pairs;
    return 1;
}

static inline int sc64_nydus_find_endpoint(const Sc64NydusSourceRecord *want,
                                           const Sc64NydusEndpointView *view,
                                           unsigned view_count)
{
    int found = -1;
    for (unsigned i = 0; i < view_count; ++i)
        if (view[i].alive && view[i].raw_type == SC64_NYDUS_RAW &&
            view[i].x == want->x && view[i].y == want->y) {
            if (found >= 0) return -2;
            found = (int)i;
        }
    return found;
}

/* Bind validated source pairs to live building slots. The view list may be
 * any subset of buildings that includes every raw-134 building standing on a
 * source position; distinct slots are required. With require_all, both
 * endpoints of every pair must exist (mission start). Otherwise a pair may be
 * absent as a whole (destroyed). One surviving endpoint is never valid: the
 * partner kill is immediate, so a save cannot legitimately contain it. */
static inline int sc64_nydus_bind(const Sc64NydusSourceRecord *rec,
                                  const Sc64NydusSourcePair *pair,
                                  unsigned pair_count,
                                  const Sc64NydusEndpointView *view,
                                  unsigned view_count, int require_all,
                                  Sc64NydusState *out)
{
    Sc64NydusState next;
    if (!out || !rec || !pair || (!view && view_count) ||
        pair_count > SC64_NYDUS_MAX_LINKS || view_count > SC64_NYDUS_MAX_VIEWS ||
        (require_all != 0 && require_all != 1))
        return 0;
    for (unsigned i = 0; i < view_count; ++i)
        for (unsigned j = 0; j < i; ++j)
            if (view[i].slot == view[j].slot || view[i].slot == SC64_NYDUS_NO_SLOT)
                return 0;
    for (unsigned i = 0; i < sizeof(next); ++i) ((unsigned char *)&next)[i] = 0;
    for (unsigned p = 0; p < pair_count; ++p) {
        int a = sc64_nydus_find_endpoint(&rec[pair[p].a], view, view_count);
        int b = sc64_nydus_find_endpoint(&rec[pair[p].b], view, view_count);
        Sc64NydusLink *link = &next.link[p];
        if (a == -2 || b == -2) return 0;
        link->slot[0] = link->slot[1] = SC64_NYDUS_NO_SLOT;
        if (a < 0 && b < 0) {
            if (require_all) return 0;
            continue;
        }
        if (a < 0 || b < 0 || view[a].owner != view[b].owner) return 0;
        link->slot[0] = view[a].slot;
        link->slot[1] = view[b].slot;
        link->owner = view[a].owner;
        link->active = 1;
    }
    next.count = (unsigned char)pair_count;
    *out = next;
    return 1;
}

static inline int sc64_nydus_state_valid(const Sc64NydusState *s,
                                         unsigned slot_limit)
{
    unsigned seen[(SC64_NYDUS_NO_SLOT + 32) / 32] = { 0 };
    if (!s || s->count > SC64_NYDUS_MAX_LINKS || slot_limit > SC64_NYDUS_NO_SLOT)
        return 0;
    for (unsigned i = 0; i < s->count; ++i) {
        const Sc64NydusLink *l = &s->link[i];
        if (l->active > 1) return 0;
        if (!l->active) {
            if (l->slot[0] != SC64_NYDUS_NO_SLOT || l->slot[1] != SC64_NYDUS_NO_SLOT)
                return 0;
            continue;
        }
        for (unsigned e = 0; e < 2; ++e) {
            unsigned slot = l->slot[e];
            if (slot >= slot_limit || (seen[slot / 32] & (1u << (slot % 32))))
                return 0;
            seen[slot / 32] |= 1u << (slot % 32);
        }
    }
    return 1;
}

/* Return the partner slot of an active endpoint, or -1. */
static inline int sc64_nydus_partner(const Sc64NydusState *s, unsigned slot)
{
    if (!s || slot >= SC64_NYDUS_NO_SLOT) return -1;
    for (unsigned i = 0; i < s->count && i < SC64_NYDUS_MAX_LINKS; ++i) {
        const Sc64NydusLink *l = &s->link[i];
        if (!l->active) continue;
        if (l->slot[0] == slot) return l->slot[1];
        if (l->slot[1] == slot) return l->slot[0];
    }
    return -1;
}

/* Break the link that contains slot. Return the partner that the caller must
 * kill (OpenBW: kill_unit(exit) after clearing both links), or -1. */
static inline int sc64_nydus_unlink(Sc64NydusState *s, unsigned slot)
{
    if (!s || slot >= SC64_NYDUS_NO_SLOT) return -1;
    for (unsigned i = 0; i < s->count && i < SC64_NYDUS_MAX_LINKS; ++i) {
        Sc64NydusLink *l = &s->link[i];
        int partner;
        if (!l->active || (l->slot[0] != slot && l->slot[1] != slot)) continue;
        partner = l->slot[0] == slot ? l->slot[1] : l->slot[0];
        l->slot[0] = l->slot[1] = SC64_NYDUS_NO_SLOT;
        l->active = 0;
        l->owner = 0;
        return partner;
    }
    return -1;
}

/* Ownership follows a transferred endpoint (OpenBW give_unit_to carries the
 * linked exit). Given each endpoint's current owner, return the endpoint
 * whose owner must be copied to its partner, or -1 when consistent. If both
 * changed to different owners, endpoint 0 is the deterministic GBA choice. */
static inline int sc64_nydus_owner_source(const Sc64NydusLink *l,
                                          unsigned owner0, unsigned owner1)
{
    if (!l || !l->active || owner0 == owner1) return -1;
    if (owner0 != l->owner) return 0;
    return 1;
}

static inline int sc64_nydus_endpoint_ready(const Sc64NydusEndpointView *e)
{
    return e && e->alive && e->complete && e->raw_type == SC64_NYDUS_RAW;
}

/* OpenBW unit_can_enter_nydus(), plus GBA state that the reference expresses
 * through orders: immovable, already-carried or burrowed actors cannot move. */
static inline int sc64_nydus_can_enter(const Sc64NydusPassengerView *p,
                                       const Sc64NydusEndpointView *entrance,
                                       const Sc64NydusEndpointView *exit)
{
    return p && p->alive && sc64_nydus_endpoint_ready(entrance) &&
        sc64_nydus_endpoint_ready(exit) && p->owner == entrance->owner &&
        !p->flying && p->zerg && p->movable && !p->loaded && !p->burrowed;
}

static inline int sc64_nydus_contact(int ux, int uy, int bx, int by)
{
    int dx = ux - bx, dy = uy - by, reach = SC64_NYDUS_HALF_FOOTPRINT_PX +
                                            SC64_NYDUS_CONTACT_SLACK_PX;
    return dx <= reach && dx >= -reach && dy <= reach && dy >= -reach;
}

/* Deterministic exit candidate n (0..SC64_NYDUS_EXIT_CANDIDATES-1): square
 * rings at 8-pixel spacing, nearest ring first; within a ring, start at the
 * bottom-centre and walk counter-clockwise on screen. */
static inline int sc64_nydus_exit_candidate(unsigned n, int cx, int cy,
                                            int *x, int *y)
{
    unsigned ring = 1, side, pos, len;
    int r, ox, oy;
    if (!x || !y || n >= SC64_NYDUS_EXIT_CANDIDATES) return 0;
    while (n >= 8u * ring) { n -= 8u * ring; ++ring; }
    r = (int)ring * SC64_NYDUS_EXIT_STEP_PX;
    len = 2u * ring;
    pos = (n + ring) % (8u * ring);
    side = pos / len;
    pos %= len;
    switch (side) {
    case 0: ox = -r + (int)pos * SC64_NYDUS_EXIT_STEP_PX; oy = r; break;
    case 1: ox = r; oy = r - (int)pos * SC64_NYDUS_EXIT_STEP_PX; break;
    case 2: ox = r - (int)pos * SC64_NYDUS_EXIT_STEP_PX; oy = -r; break;
    default: ox = -r; oy = -r + (int)pos * SC64_NYDUS_EXIT_STEP_PX; break;
    }
    *x = cx + ox;
    *y = cy + oy;
    return 1;
}

/* The same candidate order without division (Thumb has no divider): call
 * accept() per candidate until it returns nonzero. Returns the accepted
 * candidate index, or -1 after all SC64_NYDUS_EXIT_CANDIDATES fail. */
static inline int sc64_nydus_exit_scan(int cx, int cy,
                                       int (*accept)(void *, int, int),
                                       void *ctx, int *x, int *y)
{
    int n = 0;
    if (!accept || !x || !y) return -1;
    for (int ring = 1; ring <= SC64_NYDUS_EXIT_RINGS; ++ring) {
        int r = ring * SC64_NYDUS_EXIT_STEP_PX, len = 2 * ring;
        int side = 0, pos = ring;
        for (int t = 0; t < 8 * ring; ++t, ++n) {
            int ox, oy, step = pos * SC64_NYDUS_EXIT_STEP_PX;
            switch (side) {
            case 0: ox = -r + step; oy = r; break;
            case 1: ox = r; oy = r - step; break;
            case 2: ox = r - step; oy = -r; break;
            default: ox = -r; oy = -r + step; break;
            }
            if (accept(ctx, cx + ox, cy + oy)) {
                *x = cx + ox; *y = cy + oy;
                return n;
            }
            if (++pos == len) { pos = 0; if (++side == 4) side = 0; }
        }
    }
    return -1;
}

#endif /* SC64_NYDUS_RULES_H */
