/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Detached custom candidates only. No journal publication is enabled. */
#define main scarab_unused_egg_journal_main
#include "sc64_shared_egg_custom_journal.c"
#undef main
#include "../starcraft/sc64_scarab_unit_adapter.h"
static RaceSave candidate, snapshot_candidate;
static int encode_candidate(unsigned slot,const Unit *row,unsigned features)
{
#if defined(SC64_ZP_FULL_STORAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    return sc64_p7_typed_unit_encode_for(&candidate.units[slot],row,1024,1024,(int)features);
#elif defined(SC64_ZP_FULL_STORAGE) && !defined(SC64_Z7_DERIVED_SAVE)
    return sc64_zp_unit_save_encode_for(&candidate.units[slot],row,1024,1024,(int)features);
#elif defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_ZP_FULL_STORAGE)
    return sc64_unit_save_encode_for(&candidate.units[slot],row,1024,1024,(int)features);
#else
    (void)slot;(void)row;(void)features;return 0;
#endif
}
int main(void)
{
    start_custom();assert(race_save_game());candidate=race_data;
    memset(candidate.units,0,sizeof(candidate.units));
    Unit parent={0},child={0},wire,out,sentinel={0};
    parent.alive=1;parent.owner=SIDE;parent.type=U_REAVER;parent.raw_type=83;
    parent.raw_owner=-1;parent.x=parent.y=128;parent.maxhp=100;parent.hp=100;
    parent.build=-1;parent.target=-1;
    Sc64ScarabParentFields fields={1,1,0,0};int timer;
    assert(sc64_scarab_parent_fields_pack(&fields,&parent.carry,&timer));parent.work_timer=timer;
    child.alive=1;child.owner=SIDE;child.type=U_SCARAB;child.raw_type=85;child.raw_owner=-1;
    child.x=child.y=128;child.hp=2;child.maxhp=20;child.shield=10;child.build=-1;child.target=-1;
    Sc64ScarabChildFields cf={0,0,0,0};assert(sc64_scarab_child_fields_pack(&cf,&child.carry,&timer));child.work_timer=timer;
    assert(sc64_scarab_flight_to_wire(&child,&wire,1,0));
    assert(encode_candidate(0,&parent,SC64_STAGE_SAVE_EGG|SC64_STAGE_SAVE_SCARAB));
    assert(!encode_candidate(1,&wire,SC64_STAGE_SAVE_EGG));
    assert(encode_candidate(1,&wire,SC64_STAGE_SAVE_EGG|SC64_STAGE_SAVE_SCARAB));
    const Sc64StageTagPair pair={0x12345678u,SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG|SC64_STAGE_SAVE_SCARAB};
    Sc64StageSaveAuth auth;assert(sc64_stage_save_authenticate(&auth,&candidate,pair.outer,candidate.version,&pair,1));
    snapshot_candidate=candidate;sentinel.hp=123;out=sentinel;
    assert(race_save_decode_unit_authenticated(&candidate,1,1024,1024,&out,&auth));
    assert(out.type==U_SCARAB && out.carry==child.carry && out.raw_type==85);
    assert(race_save_decode_unit_authenticated(&candidate,0,1024,1024,&out,&auth));
    assert(out.type==U_REAVER && out.carry==parent.carry);
    assert(!memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    Sc64StageSaveAuth wrong=auth;wrong.features=SC64_STAGE_SAVE_EGG;out=sentinel;
    assert(!race_save_decode_unit_authenticated(&candidate,1,1024,1024,&out,&wrong));
    assert(!memcmp(&out,&sentinel,sizeof(out)) && !memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    candidate.version^=1;out=sentinel;
    assert(!race_save_decode_unit_authenticated(&candidate,0,1024,1024,&out,&auth));
    assert(!memcmp(&out,&sentinel,sizeof(out)));
    puts("detached Egg/Scarab feature binding and immutable candidate passed");return 0;
}
