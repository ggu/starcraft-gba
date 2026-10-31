/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include "../starcraft/sc64_scarab_unit_adapter.h"
#include <stdlib.h>
extern int sc64_test_sram_transfer(const char *,int);
extern int sc64_test_replace_payload(u32,const void *,u32);
extern int sc64_test_remove_sequence(u32);
extern int sc64_test_set_sequence(u32,u32);
int main(int argc,char **argv) {
    assert(argc==4);setup();race_skirmish_map=atoi(argv[3]);race_skirmish_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));minerals=500;
    if (!strncmp(argv[1],"reject-write-",13)) {
        int p=spawn_unit(SIDE,U_REAVER,128,128);assert(p==0 && race_scarab_parent_init(p));
        assert(race_scarab_enqueue(p,0));assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));
        int c=parent.active_slot;assert(c>=0);
        if (!strcmp(argv[1],"reject-write-live-outside")) u[c].carry=(int)p|(2<<8);
        else if (!strcmp(argv[1],"reject-write-live-self")) u[c].carry=(int)p|(3<<8);
        else if (!strcmp(argv[1],"reject-write-dead-outside")) { u[c].alive=0;u[c].carry=(int)p|(2<<8); }
        else if (!strcmp(argv[1],"reject-write-dead-self")) { u[c].alive=0;u[c].carry=(int)p|(3<<8); }
        else if (!strcmp(argv[1],"reject-write-dead-mismatch")) { u[c].alive=0;u[c].raw_type=84; }
        else if (!strcmp(argv[1],"reject-write-dead-malformed")) { u[c].alive=0;u[c].carry=-1; }
        else assert(!"unknown writer rejection case");
        assert(!race_save_game());assert(sc64_test_sram_transfer(argv[2],1));
        puts("invalid child rejected by writer");return 0;
    } else if (!strcmp(argv[1],"write") || !strcmp(argv[1],"write-over-old")) {
        if (!strcmp(argv[1],"write-over-old")) assert(sc64_test_sram_transfer(argv[2],0));
        int p=spawn_unit(SIDE,U_REAVER,128,128);assert(p==0 && race_scarab_parent_init(p));
        assert(race_scarab_enqueue(p,0));assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
        for(int i=0;i<20;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
        for (int i=0;i<MAX_UNITS;i++) { Unit wire; Sc64PackedUnitSave row;
            assert(sc64_scarab_flight_to_wire(&u[i],&wire,1,0));
            assert(sc64_unit_save_encode(&row,&wire,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
        }
        assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(sc64_test_sram_transfer(argv[2],0));
        if (!strcmp(argv[1],"old-newer")) {
            assert(sc64_test_set_sequence(SAVE_VERSION,3) && sc64_test_sram_transfer(argv[2],1));return 0;
        }
        if (!strcmp(argv[1],"corrupt-old")) {
            u32 len;Unit q;
            assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SAVE_VERSION,&len));
            assert(sc64_unit_save_decode(&q,&race_data.units[0],race_cols*16u,race_rows*16u));
            assert(q.type==U_REAVER);q.carry=1;
            assert(sc64_unit_save_encode(&race_data.units[0],&q,race_cols*16u,race_rows*16u));
            assert(sc64_test_replace_payload(SAVE_VERSION,&race_impact_record,len));
            assert(sc64_test_sram_transfer(argv[2],1));return 0;
        }
        if (!strncmp(argv[1],"corrupt",7)) {
            u32 len; Unit q;
            assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SCARAB_SAVE_VERSION,&len));
            assert(sc64_unit_save_decode(&q,&race_data.units[1],race_cols*16u,race_rows*16u));
            assert(q.type==U_SCARAB);
            if (!strcmp(argv[1],"corrupt-orphan") || !strcmp(argv[1],"corrupt-parent-count")) {
                assert(sc64_unit_save_decode(&q,&race_data.units[0],race_cols*16u,race_rows*16u));
                if (!strcmp(argv[1],"corrupt-orphan")) q.alive=0;
                else q.carry+=6; /* Encodes an impossible six-entry queue. */
                assert(sc64_unit_save_encode(&race_data.units[0],&q,race_cols*16u,race_rows*16u));
                assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
                assert(sc64_test_sram_transfer(argv[2],1));return 0;
            }
            if (!strcmp(argv[1],"corrupt")) q.hp=19;
            else if (!strcmp(argv[1],"corrupt-target")) q.target=0;
            else if (!strcmp(argv[1],"corrupt-timer")) q.work_timer=1;
            else if (!strcmp(argv[1],"corrupt-owner")) q.owner=ENEMY;
            else if (!strcmp(argv[1],"corrupt-raw")) q.raw_type=84;
            else assert(!"unknown corrupt case");
            assert(sc64_unit_save_encode(&race_data.units[1],&q,race_cols*16u,race_rows*16u));
            assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
            assert(sc64_test_sram_transfer(argv[2],1));return 0;
        }
        if (!strcmp(argv[1],"remove-old")) {
            assert(sc64_test_remove_sequence(1) && sc64_test_sram_transfer(argv[2],1));return 0;
        }
        if (!strcmp(argv[1],"reject")) {
            Unit before_u[MAX_UNITS];Building before_b[MAX_BUILDINGS];int m=minerals,g=gas,n=frame;u32 s=seed;
            memcpy(before_u,u,sizeof(u));memcpy(before_b,b,sizeof(b));assert(!race_load_game());
            assert(!memcmp(before_u,u,sizeof(u)) && !memcmp(before_b,b,sizeof(b)) && m==minerals && g==gas && n==frame && s==seed);
            assert(sc64_test_sram_transfer(argv[2],1));return 0;
        }
        assert(race_load_game());
        Sc64ScarabParent p;Sc64ScarabCounts counts;
        assert(race_scarab_parent_read(0,0,&p,&counts));
        if (!strcmp(argv[1],"fallback-old") || !strcmp(argv[1],"old-write")) {
            assert(p.active_slot<0 && p.queued==0 && counts.inside==0);
            if (!strcmp(argv[1],"old-write")) assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else if (!strcmp(argv[1],"fallback-new")) {
            assert(p.active_slot>=0 && p.progress==20);
            assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else if (!strcmp(argv[1],"complete")) {
            assert(p.active_slot>=0 && p.progress==20);
            while (p.active_slot>=0) {
                int result=race_scarab_production_tick(0,0);
                assert(result==SC64_SCARAB_PROGRESS || result==SC64_SCARAB_COMPLETED);
                assert(race_scarab_parent_read(0,0,&p,&counts));
            }
            assert(counts.inside==1 && race_save_game() && sc64_test_sram_transfer(argv[2],1));
        } else if (!strcmp(argv[1],"fallback")) {
            assert(p.active_slot>=0 && p.progress==20);
        } else { assert(!strcmp(argv[1],"verify") && counts.inside==1); }
    }
    puts("hidden Scarab save stage passed");return 0;
}
