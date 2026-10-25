#include "../common/gba.h"

typedef struct {
    s16 x, y, raw_type;
    u16 flags;
    s16 target;
    u8 owner, alive, order;
} Sc64P6BurrowActor;

/* Independent full scan. Deliberately contains no center-axis rejection. */
unsigned sc64_p6_burrow_reference(const Sc64P6BurrowActor *units,
                                  unsigned capacity,
                                  int (*owners_hostile)(int, int),
                                  unsigned *released, unsigned released_capacity)
{
    unsigned count=0;
    if (!units || !owners_hostile || (!released && released_capacity)) return 0;
    for (unsigned i=0;i<capacity;++i) {
        const Sc64P6BurrowActor *q=&units[i];
        if (!q->alive || !(q->flags&2u) ||
            (q->raw_type!=37 && q->raw_type!=38 && q->raw_type!=50)) continue;
        for (unsigned j=0;j<capacity;++j) {
            int dx,dy;
            if (i==j || !units[j].alive ||
                !owners_hostile(q->owner,units[j].owner)) continue;
            dx=(int)q->x-(int)units[j].x;
            dy=(int)q->y-(int)units[j].y;
            if (dx*dx+dy*dy<=96*96) {
                if (count<released_capacity) released[count]=i;
                ++count;
                break;
            }
        }
    }
    return count;
}
