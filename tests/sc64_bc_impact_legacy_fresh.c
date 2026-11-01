/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc,char **argv)
{
    assert(argc==3); setup();
    if (!strcmp(argv[1],"write")) {
        int a=add_unit(SIDE,U_DRAGOON,100,100), t=add_unit(ENEMY,U_ZERGLING,156,100);
        (void)launch_dragoon(a,t); assert(pending()==1 && race_save_game());
        u32 len=latest_record(),base=sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES;
        assert(len==base+sc64_impact_save_bytes(1));
        record_copy.save.version=SC64_LEGACY_SAVE_VERSION;
        record_copy.save.state[27]=0;record_copy.save.state[28]=0;
        save_write(&record_copy,len,SC64_LEGACY_SAVE_VERSION);
        save_test_corrupt_latest(0,SAVE_VERSION);
        assert(sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(sc64_test_sram_transfer(argv[2],0) && race_load_game() && pending()==1);
        if (!strcmp(argv[1],"migrate")) {
            assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else assert(!strcmp(argv[1],"verify"));
        Sc64ImpactHit hit; sc64_impact_unpack(&race_impact_rows[0],&hit);
        assert(hit.target>=0 && target_alive(hit.target));
        printf("pending=%u target=%d damage=%d steps=%u\n",pending(),hit.target,hit.damage,(unsigned)race_impact_rows[0].steps);
    }
    return 0;
}
