/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic fresh-process skirmish: actual codec and journal, hidden phases. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include "../starcraft/sc64_scarab_unit_adapter.h"
#include <stdlib.h>
extern int sc64_test_sram_transfer(const char *,int);
extern int sc64_test_replace_payload(u32,const void *,u32);
int main(int argc,char **argv) {
    assert(argc==4);setup();race_skirmish_map=atoi(argv[3]);race_skirmish_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));minerals=500;
    if (!strcmp(argv[1],"reject-outside")) {
        assert(sc64_test_sram_transfer(argv[2],0));
        int p=spawn_unit(SIDE,U_REAVER,128,128);assert(p==0 && race_scarab_parent_init(p));
        assert(race_scarab_enqueue(p,0));assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));
        u[parent.active_slot].carry=(p | (2<<8));
        assert(!race_save_game() && sc64_test_sram_transfer(argv[2],1));
        puts("outside child rejected");return 0;
    } else if (!strcmp(argv[1],"write") || !strcmp(argv[1],"write-over-old")) {
        if (!strcmp(argv[1],"write-over-old")) assert(sc64_test_sram_transfer(argv[2],0));
        int p=spawn_unit(SIDE,U_REAVER,128,128);assert(p==0 && race_scarab_parent_init(p));
        assert(race_scarab_enqueue(p,0));assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        for(int i=0;i<20;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
        assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(sc64_test_sram_transfer(argv[2],0));
        if (!strcmp(argv[1],"corrupt")) {
            u32 len=0;Unit wire;
            assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SCARAB_SAVE_VERSION,&len));
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
            assert(sc64_p7_typed_unit_decode(&wire,&race_data.units[1],race_cols*16u,race_rows*16u));
#elif defined(SC64_Z7_DERIVED_SAVE)
            assert(sc64_derived_unit_decode(&wire,&race_data.units[1],race_cols*16u,race_rows*16u));
#else
            assert(sc64_zp_unit_save_decode(&wire,&race_data.units[1],race_cols*16u,race_rows*16u));
#endif
            assert(wire.type==U_SCARAB);wire.selected=1;
            assert(race_scarab_zp_encode(&race_data,1,&wire,race_cols*16u,race_rows*16u));
            assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
            assert(sc64_test_sram_transfer(argv[2],1));puts("checksum-valid bad child written");return 0;
        }
        assert(race_load_game());
        Sc64ScarabParent p;Sc64ScarabCounts counts;assert(race_scarab_parent_read(0,0,&p,&counts));
        if (!strcmp(argv[1],"old-verify")) {
            assert(p.active_slot<0 && !p.queued && !counts.inside && !counts.outside);
            assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else if (!strcmp(argv[1],"complete")) {
            assert(p.active_slot>=0 && p.progress==20);
            while(p.active_slot>=0) {
                int result=race_scarab_production_tick(0,0);
                assert(result==SC64_SCARAB_PROGRESS || result==SC64_SCARAB_COMPLETED);
                assert(race_scarab_parent_read(0,0,&p,&counts));
            }
            assert(counts.inside==1 && race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else if (!strcmp(argv[1],"fallback")) {
            assert(p.active_slot>=0 && p.progress==20 && counts.inside==0);
        } else if (!strcmp(argv[1],"fallback-old")) {
            assert(p.active_slot<0 && !p.queued && !counts.inside && !counts.outside);
        } else { assert(!strcmp(argv[1],"verify") && counts.inside==1); }
    }
    puts("cross-codec hidden save passed");return 0;
}
