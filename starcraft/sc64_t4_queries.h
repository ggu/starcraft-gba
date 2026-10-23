#ifndef SC64_T4_QUERIES_H
#define SC64_T4_QUERIES_H

#ifdef SC64_T4_RUNTIME

#include "sc64_trigger_types.h"
#include "sc64_t4_world.h"

/* OpenBW's Command/Bring player filtering rejects slots >=8. T4 source
 * controllers make slots 0..4, 6, and 7 active; slot 5 is unused.
 * THG2 doodad targeting has separate rules and must not use this filter. */
static int
sc64_t4_query_slot_active(unsigned slot)
{
    return slot <= 4 || slot == 6 || slot == 7;
}

typedef struct {
    s32 left, top, right, bottom;
} Sc64T4QueryRect;

/* BWAPI UnitType.cpp unitDimensions rows use tile width/height followed by
 * source-pixel left/up/right/down extents. Right and bottom are converted to
 * exclusive edges (+1). These are the T4 initial/trigger-created types whose
 * footprints were verified against that pinned table. No guess is made for
 * any other type. */
static int
sc64_t4_query_actor_rect(unsigned raw_type, s32 center_x, s32 center_y,
                         Sc64T4QueryRect *out)
{
    static const struct { u8 left, up, right, down; } extents[] = {
        {8, 9, 8, 10},   /* 0 Marine */
        {7, 10, 7, 11},  /* 1 Ghost */
        {16, 16, 15, 15},/* 2 Vulture */
        {16, 16, 15, 15},/* 3 Goliath */
        {11, 11, 11, 11},/* 7 SCV */
        {8, 9, 8, 10},   /* 15 Civilian */
        {8, 9, 8, 10},   /* 20 Jim Raynor (Marine) */
        {11, 7, 11, 14}, /* 32 Firebat */
        {8, 4, 7, 11},   /* 37 Zergling */
        {16, 16, 15, 15},/* 89 Rhynadon */
        {16, 16, 15, 15},/* 95 Ragnasaur */
        {13, 13, 13, 17},/* 101 Map Revealer */
        {48, 32, 47, 31},/* 195 Terran Beacon */
        {16, 16, 15, 15} /* 218 Data Disk */
    };
    static const u8 raw_types[] = {0, 1, 2, 3, 7, 15, 20, 32, 37,
                                   89, 95, 101, 195, 218};
    unsigned i;
    s32 left, top, right, bottom;
    if (!out) return 0;
    for (i = 0; i < sizeof(raw_types); ++i)
        if (raw_types[i] == raw_type) break;
    if (i == sizeof(raw_types)) return 0;
    /* Guard signed arithmetic explicitly before constructing the rectangle. */
    if (center_x < (-2147483647 - 1) + extents[i].left ||
        center_y < (-2147483647 - 1) + extents[i].up ||
        center_x > 2147483647 - extents[i].right - 1 ||
        center_y > 2147483647 - extents[i].down - 1) return 0;
    left = center_x - extents[i].left;
    top = center_y - extents[i].up;
    right = center_x + extents[i].right + 1;
    bottom = center_y + extents[i].down + 1;
    out->left = left; out->top = top; out->right = right; out->bottom = bottom;
    return 1;
}

/* CHK location bounds and the actor bounds supplied by the caller both use
 * source pixels with right/bottom exclusive edges. Invalid/reversed or empty
 * rectangles do not overlap; operands are deliberately not normalized. */
static int
sc64_t4_query_rect_overlap(const Sc64T4QueryRect *location,
                           const Sc64T4QueryRect *actor)
{
    if (!location || !actor || location->left >= location->right ||
        location->top >= location->bottom || actor->left >= actor->right ||
        actor->top >= actor->bottom) return 0;
    return location->left < actor->right && actor->left < location->right &&
           location->top < actor->bottom && actor->top < location->bottom;
}

/* Resolve the CHK player-group selector against the actor's current runtime
 * owner, while preserving raw_owner as provenance for rescues. Supported
 * selectors are direct slots 0..11, Current Player (13), All Players (17),
 * and Force 1..4 (18..21). The known No Players selector (12) never matches.
 * Return -1 for an unsupported selector so callers can
 * reject an unsupported condition/action instead of treating it as false.
 * Neutral slot 11 is inactive for trigger queries (OpenBW rejects every
 * slot >=8); unused/inactive slots never match. */
static int
sc64_t4_query_owner_matches(unsigned selector, unsigned current_player,
                            int live_owner, int raw_owner)
{
    int current_slot = sc64_t4_current_source_owner(live_owner, raw_owner);
    if (selector <= 11) {
        if (!sc64_t4_query_slot_active(selector)) return 0;
        return current_slot == (int)selector;
    }
    if (selector == 12) return 0; /* No Players. */
    if (selector == 13) {
        if (current_player > 11 || !sc64_t4_query_slot_active(current_player))
            return 0;
        return current_slot == (int)current_player;
    }
    if (selector == 17) {
        return current_slot >= 0 && current_slot < 8 &&
               sc64_t4_query_slot_active((unsigned)current_slot);
    }
    if (selector >= 18 && selector <= 21) {
        if (current_slot < 0 || current_slot >= 8 ||
            !sc64_t4_query_slot_active((unsigned)current_slot)) return 0;
        return sc64_t4_force_contains((int)(selector - 17), current_slot);
    }
    return -1;
}

#endif /* SC64_T4_RUNTIME */

#endif
