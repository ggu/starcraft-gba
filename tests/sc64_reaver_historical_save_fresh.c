/* SPDX-License-Identifier: GPL-3.0-or-later
 * Gate-off production writer versus gated reader, separate host processes. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc,char **argv)
{
    assert(argc==3); setup();
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
        assert(!strcmp(argv[1],"reject"));
        Unit before_units[MAX_UNITS]; Building before_buildings[MAX_BUILDINGS];
        int before_minerals=minerals,before_gas=gas,before_frame=frame; u32 before_seed=seed;
        memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
        assert(!race_load_game());
        assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
        assert(before_minerals==minerals && before_gas==gas && before_frame==frame && before_seed==seed);
        assert(sc64_test_sram_transfer(argv[2],1));
        puts("gated reader rejected unconverted parent; sampled live state unchanged");
#else
        assert(!strcmp(argv[1],"read") && race_load_game());
        assert(u[0].alive && u[0].type==U_REAVER && u[0].carry==0 && u[0].work_timer==0);
        puts("gate-off reader restored historical parent");
#endif
    }
    return 0;
}
