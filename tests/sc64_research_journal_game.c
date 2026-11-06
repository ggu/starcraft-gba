/* SPDX-License-Identifier: GPL-3.0-or-later
 * Experimental synthetic research levels through actual game save/load. */
#define main sc64_existing_mission_main
#ifdef SC64_RESEARCH_TEST_Z9
#include "../common/gba.c"
#include "sc64_z9_game.c"
#else
#include "sc64_p10_mission.c"
#endif
#undef main
static u8 prior_sram[32768];
static void start_campaign(void) {
#ifdef SC64_RESEARCH_TEST_Z9
    launch_z9_from_normal_menu();
#else
    start_p10();
#endif
    game_update(0,0);
}
static void check_campaign(void) {
    start_campaign(); memset(host_save,0,sizeof(host_save));
    memset(&sc64_experimental_research_levels,0,sizeof(sc64_experimental_research_levels));
    assert(race_save_game());
    /* Zero-level research wire is byte-identical to the old normal wire.
     * Retag this synthetic slot to exercise the real legacy reader. */
    assert(sram_get32(host_save)==SC64_RESEARCH_TRANSPORT_SAVE_VERSION);
    sram_put32(host_save,SC64_TRANSPORT_SAVE_VERSION);
    memset(&sc64_experimental_research_levels,0xa5,sizeof(sc64_experimental_research_levels));
    assert(race_load_game());
    Sc64ProtossResearchLevels zero={0};
    assert(!memcmp(&sc64_experimental_research_levels,&zero,sizeof(zero)));
    sc64_experimental_research_levels.level[0][0]=3;
    sc64_experimental_research_levels.level[7][4]=2;
    memcpy(prior_sram,host_save,sizeof(prior_sram));
    assert(race_save_game());
    assert(!memcmp(prior_sram,host_save,SAVE_SLOT_SIZE));
    assert(sram_get32(host_save+SAVE_SLOT_SIZE)==SC64_RESEARCH_TRANSPORT_SAVE_VERSION);
    memset(&sc64_experimental_research_levels,0,sizeof(sc64_experimental_research_levels));
    assert(race_load_game());
    assert(sc64_experimental_research_levels.level[0][0]==3 && sc64_experimental_research_levels.level[7][4]==2);
    /* Corrupt a new-format unused padding bit, retaining a valid checksum.
     * Its predecessor remains intact and must win selection. */
    volatile u8 *p=host_save+SAVE_SLOT_SIZE; u32 len=sram_get32(p+4);
    size_t bitrow=96/(8-SC64_PR_USED_BITS), bit=SC64_PR_USED_BITS+96%(8-SC64_PR_USED_BITS);
    size_t offset=offsetof(RaceSave,buildings)+bitrow*SC64_PR_ROW_WIDTH+SC64_PR_ROW_WIDTH-1;
    assert(offset<len); p[16+offset]|=(u8)(1u<<bit);
    u8 payload[SAVE_PAYLOAD_MAX]; for(u32 i=0;i<len;i++)payload[i]=p[16+i];
    sram_put32(p+8,hash_bytes(payload,len)); memcpy(prior_sram,host_save,sizeof(prior_sram));
    assert(race_load_game()); assert(!memcmp(&sc64_experimental_research_levels,&zero,sizeof(zero)));
    assert(!memcmp(prior_sram,host_save,sizeof(prior_sram)));
}
static void check_custom(void) {
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init(); screen=SCREEN_PLAY; memset(host_save,0,sizeof(host_save));
    memset(&sc64_experimental_research_levels,0,sizeof(sc64_experimental_research_levels));
    sc64_experimental_research_levels.level[4][2]=3;
    int target=-1; for(int i=0;i<MAX_UNITS;i++) if(u[i].alive) { target=i; break; }
    assert(target>=0); race_impact_reset();
    assert(RACE_IMPACT_SAVE_CAPACITY>=8);
    for(unsigned i=0;i<8;i++) race_impact_launch(target,1,1,SIDE,0,U_DRAGOON,5+i);
    assert(race_impact_count==8);
    Sc64ImpactEvent impacts[8]; memcpy(impacts,race_impact_rows,sizeof(impacts));
    assert(race_save_game());
    race_impact_reset(); memset(&sc64_experimental_research_levels,0,sizeof(sc64_experimental_research_levels));
    assert(race_load_game()); assert(race_impact_count==8);
    assert(!memcmp(impacts,race_impact_rows,sizeof(impacts)));
    assert(sc64_experimental_research_levels.level[4][2]==3);
    memcpy(prior_sram,host_save,sizeof(prior_sram));
    sc64_experimental_research_levels.level[5][0]=1;
    assert(!race_save_game()); assert(!memcmp(prior_sram,host_save,sizeof(prior_sram)));
}
int main(void) { gba_init(); check_campaign(); check_custom(); puts("experimental research campaign and custom journal: PASS"); return 0; }
