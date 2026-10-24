#include "../starcraft/sc64_z5_detection.h"

typedef struct {
    int target;
    Sc64Z5DetectBox box;
    unsigned eligible;
} Sc64P6TrapCandidate;

static unsigned reference_box_distance(const Sc64Z5DetectBox *a,
                                       const Sc64Z5DetectBox *b)
{
    int ax0=a->x-a->left, ay0=a->y-a->up;
    int ax1=a->x+a->right+1, ay1=a->y+a->down+1;
    int bx0=b->x-b->left, by0=b->y-b->up;
    int bx1=b->x+b->right+1, by1=b->y+b->down+1;
    unsigned dx=0,dy=0,swap;
    if(ax0>bx1)dx=(unsigned)(ax0-bx1);else if(bx0>ax1)dx=(unsigned)(bx0-ax1);
    if(ay0>by1)dy=(unsigned)(ay0-by1);else if(by0>ay1)dy=(unsigned)(by0-ay1);
    if(dx<dy){swap=dx;dx=dy;dy=swap;}
    if(dx/4u<dy)dx=dx-dx/16u+dy*3u/8u-dx/64u+dy*3u/256u;
    return dx;
}

unsigned sc64_p6_trap_reference_box_distance(const Sc64Z5DetectBox *a,
                                               const Sc64Z5DetectBox *b)
{ return reference_box_distance(a,b); }

/* Independent exhaustive oracle: it does not apply the center-distance
 * rejection and evaluates the inclusive source box distance for every row. */
int sc64_p6_trap_reference_target(const Sc64Z5DetectBox *trap,
                                  const Sc64P6TrapCandidate *rows,
                                  unsigned count, unsigned range)
{
    int best=-1;
    unsigned best_dist=range+1u;
    if (!sc64_z5_detect_box_valid(trap) || (!rows && count)) return -1;
    for (unsigned i=0;i<count;++i) {
        unsigned dist;
        if (!rows[i].eligible || !sc64_z5_detect_box_valid(&rows[i].box)) continue;
        dist=reference_box_distance(trap,&rows[i].box);
        if (dist>range) continue;
        if (dist<best_dist || (dist==best_dist && best>=0 && rows[i].target<best)) {
            best=rows[i].target;
            best_dist=dist;
        }
    }
    return best;
}
