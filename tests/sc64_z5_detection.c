#include <assert.h>
#include "../starcraft/sc64_z5_detection.h"

static Sc64Z5TrapDetector detector(int raw, int x, int y)
{
    const Sc64Z5BuildingFact *fact=sc64_z5_building_fact(raw);
    Sc64Z5TrapDetector t={0};
    assert(fact);
    t.raw_type=raw;
    t.source_flags=fact->flags;
    t.box=(Sc64Z5DetectBox){x,y,fact->left,fact->up,fact->right,fact->down};
    t.owner=6;
    t.alive=t.enabled=t.completed=1;
    return t;
}

int main(void)
{
    /* Kerrigan's source bounding extents are 7/10/7/11 pixels. */
    Sc64Z5DetectBox kerrigan={1000,1000,7,10,7,11};
    Sc64Z5TrapDetector large=detector(203,1232,1000);
    Sc64Z5TrapDetector small=detector(211,1120,1000);
    Sc64Z5TrapDetector door=detector(208,1000,1000);

    /* Bbox gap is exactly the inclusive 192px source range at center delta
     * 232. One pixel farther exceeds range. */
    assert(sc64_z5_box_distance(&kerrigan,&large.box)==192u);
    assert(sc64_z5_trap_detects(&kerrigan,0,&large));
    large.box.x++;
    assert(sc64_z5_box_distance(&kerrigan,&large.box)==193u);
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));

    /* Small trap range is 96px; touching boxes at the exact outer threshold
     * is detected, then moving one pixel beyond the threshold is not. */
    assert(sc64_z5_box_distance(&kerrigan,&small.box)==96u);
    assert(sc64_z5_trap_detects(&kerrigan,0,&small));
    small.box.x++;
    assert(sc64_z5_box_distance(&kerrigan,&small.box)==97u);
    assert(!sc64_z5_trap_detects(&kerrigan,0,&small));

    /* Inclusive bbox edges overlap at equality. The octagonal metric's
     * diagonal branch is bounded and deterministic. */
    {
        Sc64Z5DetectBox a={100,100,0,0,0,0};
        Sc64Z5DetectBox b={101,101,0,0,0,0};
        assert(sc64_z5_box_distance(&a,&b)==0u);
        b.x=200; b.y=200;
        assert(sc64_z5_box_distance(&a,&b)==130u);
    }

    /* Owner, life, activation, completion, blindness, classification, and
     * source type all gate detection; doors are native buildings, not traps. */
    large=detector(209,1000,1000);
    assert(sc64_z5_trap_detects(&kerrigan,0,&large));
    assert(!sc64_z5_trap_detects(&kerrigan,6,&large));
    large.alive=0; assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    large=detector(209,1000,1000); large.enabled=0;
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    large=detector(209,1000,1000); large.completed=0;
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    large=detector(209,1000,1000); large.blinded=1;
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    large=detector(209,1000,1000); large.source_flags|=SC64_Z5_SOURCE_BUILDING;
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    assert(!sc64_z5_trap_detects(&kerrigan,0,&door));
    large=detector(203,1000,1000); large.raw_type=205;
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));

    /* Invalid boxes and out-of-range owners fail closed. */
    kerrigan.x=-1;
    large=detector(203,1000,1000);
    assert(!sc64_z5_trap_detects(&kerrigan,0,&large));
    kerrigan.x=1000;
    assert(!sc64_z5_trap_detects(&kerrigan,8,&large));
    return 0;
}
