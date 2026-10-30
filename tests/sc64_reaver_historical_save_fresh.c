/* SPDX-License-Identifier: GPL-3.0-or-later
 * Gate-off production writer versus gated reader, separate host processes. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include <stdlib.h>
extern int sc64_test_sram_transfer(const char *,int);
extern int sc64_test_replace_payload(u32,const void *,u32);
extern int sc64_test_remove_tag(u32);
int main(int argc,char **argv)
{
    assert(argc==3 || argc==6); setup();
    if (argc==6) {
        race_skirmish_map=atoi(argv[3]);race_human=atoi(argv[4]);race_bots=atoi(argv[5]);
        assert(race_skirmish_map>=0 && race_skirmish_map<=SC64_SKIRMISH_IMPORTED_COUNT &&
               race_human>=0 && race_human<3 && race_bots>=1 && race_bots<=3);
        for(int i=0;i<3;i++) race_bot_races[i]=(race_human+i+1)%3;
        race_skirmish_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    }
    if (!strcmp(argv[1],"write")) {
#ifdef SC64_SCARAB_RUNTIME
        assert(!"writer must use the historical gate-off layout");
#else
        int parent=add_unit(SIDE,U_REAVER,128,128);
        assert(sc64_race_raw_unit(u[parent].type)==83);
        assert(u[parent].carry==0 && u[parent].work_timer==0);
        assert(race_save_game());
        printf("tag=%08x base=%08x raw=%d mapped=%d carry=%d timer=%d slot=%d\n",
               (unsigned)SAVE_VERSION,(unsigned)SC64_LEGACY_SAVE_VERSION,
               u[parent].raw_type,sc64_race_raw_unit(u[parent].type),u[parent].carry,u[parent].work_timer,parent);
        assert(sc64_test_sram_transfer(argv[2],1));
#endif
    } else {
        assert(sc64_test_sram_transfer(argv[2],0));
#ifdef SC64_SCARAB_RUNTIME
#ifdef SC64_SCARAB_NAMESPACE_STAGE
        if (!strcmp(argv[1],"corrupt")) {
            u32 len; Unit q;
            assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SCARAB_SAVE_VERSION,&len));
            assert(race_save_decode_unit(&race_data,0,1024,1024,&q) && q.type==U_REAVER);
            q.carry=0; q.work_timer=0; /* Not a valid packed parent. */
            assert(sc64_unit_save_encode(&race_data.units[0],&q,1024,1024));
            assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
            assert(sc64_test_sram_transfer(argv[2],1));
            puts("wrote checksummed invalid packed parent"); return 0;
        }
        if (!strcmp(argv[1],"remove-old")) {
            assert(sc64_test_remove_tag(SAVE_VERSION));
            assert(sc64_test_sram_transfer(argv[2],1));return 0;
        }
        if (!strcmp(argv[1],"reject")) {
            Unit before_units[MAX_UNITS]; Building before_buildings[MAX_BUILDINGS];
            int old_minerals=minerals,old_gas=gas,old_frame=frame;u32 old_seed=seed;
            memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));
            assert(!race_load_game());
            assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
            assert(old_minerals==minerals && old_gas==gas && old_frame==frame && old_seed==seed);
            assert(sc64_test_sram_transfer(argv[2],1));
            puts("invalid only record rejected; sampled state unchanged");return 0;
        }
        assert(!strcmp(argv[1],"migrate") || !strcmp(argv[1],"verify") || !strcmp(argv[1],"fallback"));
        {
            RaceLoadPlan candidate; GbaSaveSelection chosen;
            assert(race_scarab_stage_select(&candidate,&chosen));
            assert(chosen.tag==(!strcmp(argv[1],"verify") ? SC64_SCARAB_SAVE_VERSION : SAVE_VERSION));
        }
        assert(race_load_game());
        Sc64ScarabParentFields f;
        assert(u[0].alive && u[0].type==U_REAVER && u[0].raw_type==-1);
        assert(sc64_scarab_parent_fields_unpack(u[0].carry,u[0].work_timer,&f));
        assert(f.queued==0 && f.active_slot==SC64_SCARAB_FIELD_NO_SLOT && !f.cooldown && !f.progress);
        if (!strcmp(argv[1],"migrate") || !strcmp(argv[1],"fallback")) assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
        puts("gated reader migrated gate-off empty Reaver parent");
#else
        assert(!strcmp(argv[1],"reject"));
        Unit before_units[MAX_UNITS]; Building before_buildings[MAX_BUILDINGS];
        int before_minerals=minerals,before_gas=gas,before_frame=frame; u32 before_seed=seed;
        memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
        assert(!race_load_game());
        assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
        assert(before_minerals==minerals && before_gas==gas && before_frame==frame && before_seed==seed);
        assert(sc64_test_sram_transfer(argv[2],1));
        puts("gated reader rejected unconverted parent; sampled live state unchanged");
#endif
#else
        assert(!strcmp(argv[1],"read") && race_load_game());
        assert(u[0].alive && u[0].type==U_REAVER && u[0].carry==0 && u[0].work_timer==0);
        puts("gate-off reader restored historical parent");
#endif
    }
    return 0;
}
