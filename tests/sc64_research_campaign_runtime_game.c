/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST scenes on actual P1-P10 initialization and producer paths.
 * No ordinary campaign playthrough, guest or persistence qualification. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
#error "requires the isolated campaign HOST stage"
#endif
static void seed_all_chapters(void)
{
    unsigned chapters=0;
    for (int id=0;id<SC64_RACE_MAP_COUNT;++id) {
        const Sc64RaceMap *map=&sc64_race_maps[id];
        if (!map->mission_id || map->mission_id[0]!='P') continue;
        game_init(); race_mode=0; race_selected_mission=id; init_state();
        assert(sc64_pr_stage_owner_active(SIDE));
        for (unsigned owner=1;owner<5;++owner) assert(!sc64_pr_stage_owner_active(owner));
        for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family) {
            Sc64PrCampaignRule rule;
            assert(sc64_pr_stage_campaign_rule(family,&rule));
            assert(rule.binding.current_controller_key==map->human_slot);
            assert(sc64_experimental_research_levels.level[SIDE][family]==rule.initial);
            for (unsigned owner=1;owner<SC64_PR_CAMPAIGN_OWNERS;++owner)
                assert(!sc64_experimental_research_levels.level[owner][family]);
        }
        ++chapters;
    }
    assert(chapters==10);
}
static int authored_producer(int type)
{
    int pylon=spawn_build(SIDE,B_PYLON,240,240,0);
    int forge=spawn_build(SIDE,type,240,240,0);
    assert(pylon>=0 && forge>=0);
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    selected_building=forge; minerals=gas=1000; menu_open=0;
#ifdef SC_FIELD_FEEDBACK
    menu_last=0;
#endif
    assert(sc64_pr_stage_menu_selected());
    return forge;
}
static void real_menu_and_lifecycle(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_RACE_MISSION_P1;init_state();
    int visitor=-1,nexus=-1;
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==SIDE){visitor=i;break;}
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].type==B_NEXUS&&
        b[i].owner==RESCUABLE&&b[i].raw_owner==1){nexus=i;break;}
    assert(visitor>=0&&nexus>=0);
    u[visitor].x=400;u[visitor].y=900;race_campaign_tick();
    assert(b[nexus].owner==SIDE&&b[nexus].raw_owner==1);
    Sc64PrCampaignRule rule;Sc64ProtossResearchQuote quote;
    assert(sc64_pr_stage_campaign_rule(SC64_PR_GROUND_WEAPONS,&rule));
    unsigned initial=sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS];
    assert(sc64_pr_campaign_quote(&rule,SC64_PR_GROUND_WEAPONS,initial,&quote));
    int forge=authored_producer(B_FORGE);
    assert(sc64_pr_stage_menu_quote(0,&quote));
    game_update(KEY_B,KEY_B);assert(menu_open&&field_menu_count()==4);
    game_update(0,0);game_update(KEY_A,KEY_A);
    Sc64PrPaidReceipt paid;
    assert(sc64_pr_stage_receipt(forge,&paid)&&paid.live&&paid.payer==SIDE);
    assert(minerals==1000-quote.mineral&&gas==1000-quote.gas);
    unsigned remaining=b[forge].queue_progress;
    assert(remaining>1);game_update(0,0);assert(b[forge].queue_progress==remaining-1);
    /* The old generic job cannot run alongside the typed family. */
    int bank=minerals;research(0);assert(!weapons_research&&minerals==bank);
    assert(sc64_pr_stage_cancel_slot(forge,0));assert(minerals==1000&&gas==1000);
    assert(sc64_pr_stage_begin_slot(forge,SC64_PR_GROUND_WEAPONS));
    remaining=b[forge].queue_progress;
    for(int i=0;i<MAX_BUILDINGS;++i)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_PYLON)
        b[i].alive=0;
    sc64_pr_stage_tick();assert(b[forge].queue_progress==remaining);
    assert(spawn_build(SIDE,B_PYLON,240,240,0)>=0);
    b[forge].queue_progress=1;game_update(0,0);
    assert(sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==initial+1);
    assert(!b[forge].level&&!b[forge].queue_progress);
    assert(sc64_pr_stage_weapon_level(SIDE,70)==(int)(initial+1));
    assert(sc64_pr_stage_weapon_level(ENEMY,70)==0);
    /* The real source cap is enforced without charging or changing a row. */
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]=rule.maximum;
    Building old=b[forge];bank=minerals;
    assert(!sc64_pr_stage_begin_slot(forge,SC64_PR_GROUND_WEAPONS));
    assert(!memcmp(&old,&b[forge],sizeof old)&&bank==minerals);
    /* No campaign active-job save format is allocated by this stage. */
    memset(host_save,0xa5,sizeof host_save);static unsigned char prior[32768];
    memcpy(prior,host_save,sizeof prior);
    assert(!save_game()&&!race_save_game()&&!load_game()&&!race_load_game());
    assert(!memcmp(prior,host_save,sizeof prior));
}
static void resets_and_rejections(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_P10_GLOBAL_ID;init_state();
    Sc64PrCampaignRule rule;assert(sc64_pr_stage_campaign_rule(0,&rule));
    assert(rule.binding.current_controller_key==1);
    int forge=authored_producer(B_FORGE);Building prior=b[forge];
    b[forge].owner=ENEMY;int bank=minerals;
    assert(!sc64_pr_stage_begin_slot(forge,SC64_PR_GROUND_WEAPONS));
    assert(minerals==bank);b[forge]=prior;
    race_mode=2;assert(!sc64_pr_stage_owner_active(SIDE));
    assert(!sc64_pr_stage_begin_slot(forge,SC64_PR_GROUND_WEAPONS));
    race_mode=0;
    /* Starting a custom game clears campaign initial/completed levels. */
    race_mode=1;race_human=R_PROTOSS;race_bots=1;race_bot_races[0]=R_TER;init_state();
    assert(sc64_pr_stage_owner_active(SIDE));
    for(unsigned o=0;o<SC64_PR_CAMPAIGN_OWNERS;++o)for(unsigned f=0;f<SC64_PR_FAMILY_COUNT;++f)
        assert(!sc64_experimental_research_levels.level[o][f]);
}
int main(void)
{
    gba_init();seed_all_chapters();real_menu_and_lifecycle();resets_and_rejections();
    puts("campaign research actual initialization, menu, payment, tick and policy: PASS");
    return 0;
}
