/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Detached candidates using the real profile codec; no release feature gate. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include "../starcraft/sc64_scarab_unit_adapter.h"
static RaceSave staged,legacy;
static int encode(RaceSave *candidate,unsigned slot,const Unit *q)
{
#if defined(SC64_ZP_FULL_STORAGE)
    return race_scarab_zp_encode(candidate,slot,q,(unsigned)race_cols*16u,(unsigned)race_rows*16u);
#else
    return sc64_unit_save_encode(&candidate->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u);
#endif
}
int main(void)
{
    setup();race_skirmish_map=0;race_skirmish_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));minerals=500;screen=SCREEN_PLAY;
    int p=spawn_unit(SIDE,U_REAVER,128,128);
    assert(p>=0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;
    assert(c>=0 && race_save_game());
    staged=race_data;legacy=staged;legacy.version=SAVE_VERSION;
    Unit old_parent=u[p];old_parent.carry=0;old_parent.work_timer=0;
    assert(encode(&legacy,(unsigned)p,&old_parent));
    memset(&legacy.units[c],0,sizeof(legacy.units[c]));
    const Sc64StageTagPair pairs[]={
        {SAVE_VERSION,SAVE_VERSION,0},
        {SC64_SCARAB_SAVE_VERSION,SC64_SCARAB_SAVE_VERSION,SC64_STAGE_SAVE_SCARAB}};
    Sc64StageSaveAuth a,z;
    assert(sc64_stage_save_authenticate(&a,&staged,SC64_SCARAB_SAVE_VERSION,staged.version,pairs,2));
    assert(sc64_stage_save_authenticate(&z,&legacy,SAVE_VERSION,legacy.version,pairs,2));
    const unsigned ww=(unsigned)race_cols*16u,wh=(unsigned)race_rows*16u;
    Unit q,guard={0};guard.hp=123; q=guard;
    assert(race_scarab_decode_authenticated(&staged,(unsigned)c,ww,wh,&q,&a));
    assert(q.alive && q.type==U_SCARAB && q.raw_type==85);
    assert(race_scarab_decode_authenticated(&legacy,(unsigned)p,ww,wh,&q,&z));
    Sc64ScarabParentFields empty;
    assert(q.type==U_REAVER && sc64_scarab_parent_fields_unpack(q.carry,q.work_timer,&empty));
    assert(empty.active_slot==SC64_SCARAB_FIELD_NO_SLOT && empty.queued==0);
    /* An authentic legacy candidate must not gain access to a staged child
     * just because another candidate was decoded with the staged token. */
    memcpy(&legacy.units[c],&staged.units[c],sizeof(legacy.units[c]));q=guard;
    assert(!race_scarab_decode_authenticated(&legacy,(unsigned)c,ww,wh,&q,&z));
    assert(!memcmp(&q,&guard,sizeof(q)));
    memset(&legacy.units[c],0,sizeof(legacy.units[c]));
    q=guard;assert(!race_scarab_decode_authenticated(&legacy,(unsigned)p,ww,wh,&q,&a));
    assert(!memcmp(&q,&guard,sizeof(q)));
    q=guard;assert(!race_scarab_decode_authenticated(&staged,(unsigned)c,ww,wh,&q,&z));
    assert(!memcmp(&q,&guard,sizeof(q)));
    assert(race_scarab_save_candidate_valid(&staged,ww,wh,&a));
    assert(race_scarab_decode_authenticated(&staged,(unsigned)c,ww,wh,&q,&a));
    assert(race_scarab_decode_authenticated(&legacy,(unsigned)p,ww,wh,&q,&z));
    assert(sc64_scarab_parent_fields_unpack(q.carry,q.work_timer,&empty) && empty.active_slot==SC64_SCARAB_FIELD_NO_SLOT);
    RaceSave before=legacy;
    assert(race_scarab_decode_authenticated(&legacy,(unsigned)p,ww,wh,&q,&z));
    assert(!memcmp(&legacy,&before,sizeof(legacy)));
    u32 original=staged.version;staged.version=SAVE_VERSION;q=guard;
    assert(!race_scarab_decode_authenticated(&staged,(unsigned)c,ww,wh,&q,&a));
    assert(!memcmp(&q,&guard,sizeof(q)));staged.version=original;
    assert(!sc64_stage_save_authenticate(&a,&staged,SC64_SCARAB_SAVE_VERSION,SAVE_VERSION,pairs,2));
    assert(!race_scarab_decode_authenticated(&staged,(unsigned)c,ww,wh,&q,&a));
    puts("Scarab candidate contexts bind codec, candidate and version");
    return 0;
}
