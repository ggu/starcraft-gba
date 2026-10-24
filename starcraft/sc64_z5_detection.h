#ifndef SC64_Z5_DETECTION_H
#define SC64_Z5_DETECTION_H

#include "sc64_z5_actor_rules.h"

/* Geometry is supplied as sprite-center coordinates plus source-relative
 * collision extents. Runtime callers must provide bounded map coordinates
 * and source extents, not the building-pool policy extents. */
typedef struct {
    int x, y;
    int left, up, right, down;
} Sc64Z5DetectBox;

typedef struct {
    int raw_type;
    unsigned source_flags;
    Sc64Z5DetectBox box;
    unsigned owner;
    unsigned char alive, enabled, completed, blinded;
} Sc64Z5TrapDetector;

static inline int sc64_z5_detect_box_valid(const Sc64Z5DetectBox *box)
{
    return box && box->x >= 0 && box->x < 4096 &&
        box->y >= 0 && box->y < 4096 &&
        box->left >= 0 && box->left <= 128 &&
        box->up >= 0 && box->up <= 128 &&
        box->right >= 0 && box->right <= 128 &&
        box->down >= 0 && box->down <= 128;
}

static inline int sc64_z5_detector_raw(int raw)
{
    return raw == 203 || raw == 209 || raw == 211 || raw == 213;
}

/* OpenBW's unit distance compares sprite bounding boxes, then applies its
 * integer octagonal length approximation and an inclusive <= range test
 * (bwgame.h:720-722, 1587-1610, 1626-1629, 1864-1866). Keep this equivalent
 * calculation local and authored; do not depend on OpenBW implementation. */
static inline unsigned sc64_z5_box_distance(const Sc64Z5DetectBox *a,
                                             const Sc64Z5DetectBox *b)
{
    int ax0=a->x-a->left, ay0=a->y-a->up;
    int ax1=a->x+a->right+1, ay1=a->y+a->down+1;
    int bx0=b->x-b->left, by0=b->y-b->up;
    int bx1=b->x+b->right+1, by1=b->y+b->down+1;
    unsigned dx=0, dy=0, t;
    if (ax0 > bx1) dx=(unsigned)(ax0-bx1);
    else if (bx0 > ax1) dx=(unsigned)(bx0-ax1);
    if (ay0 > by1) dy=(unsigned)(ay0-by1);
    else if (by0 > ay1) dy=(unsigned)(by0-ay1);
    if (dx < dy) { t=dx; dx=dy; dy=t; }
    if (dx / 4u < dy)
        dx=dx-dx/16u+dy*3u/8u-dx/64u+dy*3u/256u;
    return dx;
}

/* This is an explicit Z5 runtime policy predicate, not a claim that OpenBW
 * SetDoodadState maps to its unit-disabled status. Caller-provided enabled
 * expresses this adapter's trap activation state. Native traps are
 * UnitType detectors but not buildings, so use their source sight (192px
 * for 203/209; 96px for 211/213), never the grounded-building 7-tile rule. */
static inline int sc64_z5_trap_detects(const Sc64Z5DetectBox *target,
                                        unsigned target_owner,
                                        const Sc64Z5TrapDetector *trap)
{
    const Sc64Z5BuildingFact *fact;
    unsigned range;
    if (!sc64_z5_detect_box_valid(target) || !trap || !trap->alive ||
        !trap->enabled || !trap->completed || trap->blinded ||
        target_owner >= 8u || trap->owner >= 8u || trap->owner == target_owner ||
        !sc64_z5_detector_raw(trap->raw_type) ||
        !(trap->source_flags & SC64_Z5_SOURCE_DETECTOR) ||
        (trap->source_flags & SC64_Z5_SOURCE_BUILDING) ||
        !sc64_z5_detect_box_valid(&trap->box))
        return 0;
    fact=sc64_z5_building_fact(trap->raw_type);
    if (!fact || fact->sight_px <= 0) return 0;
    range=(unsigned)fact->sight_px;
    return sc64_z5_box_distance(target,&trap->box) <= range;
}

#endif
