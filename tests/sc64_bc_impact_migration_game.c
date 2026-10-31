/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual engine: legacy impact record migrates without dropping its queue. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
int main(void)
{
    setup();
    int a=add_unit(SIDE,U_DRAGOON,100,100);
    int t=add_unit(ENEMY,U_ZERGLING,156,100);
    (void)launch_dragoon(a,t);
    assert(pending()==1 && race_save_game());
    u32 len=latest_record();
    const u32 base=(u32)sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES;
    assert(len==base+sc64_impact_save_bytes(1));
    Sc64ImpactEvent before=race_impact_rows[0];
    record_copy.save.version=SC64_LEGACY_SAVE_VERSION;
    record_copy.save.state[27]=0;
    record_copy.save.state[28]=0;
    save_write(&record_copy,len,SC64_LEGACY_SAVE_VERSION);
    save_test_corrupt_latest(0,SAVE_VERSION);
    race_impact_reset();
    assert(race_load_game() && pending()==1);
    assert(!memcmp(&before,&race_impact_rows[0],sizeof(before)));
    assert(race_save_game());
    assert(latest_record()==base+sc64_impact_save_bytes(1));
    /* Old experimental BC marker is never accepted as the new format. */
    record_copy.save.state[27]=0x42430000;
    save_write(&record_copy,len,SAVE_VERSION);
    int hp=u[t].hp; unsigned count=pending();
    assert(!race_load_game() && u[t].hp==hp && pending()==count);
    puts("legacy impact migration and old BC marker rejection passed");
    return 0;
}
