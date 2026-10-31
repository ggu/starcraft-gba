/* Shared colony admission preserves custom-game owner banks and save form. */
#define main sc64_skirmish_existing_fixture_main
#include "sc64_skirmish_runtime.c"
#undef main

static void check_owner(int owner,int target)
{
    int x=owner==SIDE ? 320 : 800, y=owner==SIDE ? 320 : 800;
    int parent=fixture_building_raw(owner,143,x,y);
    assert(parent>=0);
    assert(fixture_building_raw(owner,142,x+64,y)>=0);
    assert(fixture_building_raw(owner,139,x+128,y)>=0);
    const Sc64SkirmishRecord *record=build_record(R_ZERG,target);
    assert(record && race_skirmish_record_ready(owner,parent,record));
    int other_before=owner==SIDE ? race_minerals[ENEMY] : minerals;
    if (owner==SIDE) minerals=49; else race_minerals[owner]=49;
    assert(!race_skirmish_start_record(owner,parent,record));
    assert(!b[parent].under_construction);
    if (owner==SIDE) minerals=500; else race_minerals[owner]=500;
    assert(race_skirmish_start_record(owner,parent,record));
    assert((owner==SIDE ? minerals : race_minerals[owner])==450);
    assert((owner==SIDE ? race_minerals[ENEMY] : minerals)==other_before);
    assert(b[parent].raw_type==target && b[parent].under_construction);
    if (target==146) assert(b[parent].type==B_CREEP_COLONY && b[parent].maxhp==400);
    else assert(b[parent].type==B_SPORE_COLONY && b[parent].maxhp==400);
    for (int tick=0;tick<500 && b[parent].under_construction;++tick) race_tick_buildings();
    assert(building_complete(parent) && b[parent].type==building_raw(target));
}

int main(void)
{
    start_race(R_ZERG);
    race_player[ENEMY]=R_ZERG;
    check_owner(SIDE,146);
    check_owner(ENEMY,144);
    puts("skirmish colony owner banks and legacy morph representation passed");
    return 0;
}
