/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST wallets/actors. Exercise the actual source-tick caller;
 * this is not an AI build-order or ordinary-input match qualification. */
#ifndef SC64_SCARAB_AI_STOCK_PROBE
#error "Use the explicit paid AI stocking probe"
#endif
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main

static void check_custom_owners(int map, int bots)
{
    int parents[5];
    setup();
    race_skirmish_map=map; race_bots=bots;
    for (int k=0;k<3;++k) race_bot_races[k]=R_PROTOSS;
    race_skirmish_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    minerals=95;
    for (int owner=0;owner<5;++owner) {
        race_minerals[owner]=95;
        parents[owner]=spawn_unit(owner,U_REAVER,128+owner*64,128);
        assert(parents[owner]>=0);
    }
    for (int owner=0;owner<5;++owner) {
        int expected=0;
        /* Independent expected roster: do not call the eligibility helper. */
        for (int k=0;k<bots;++k)
            if (owner==(k==0 ? ENEMY : k==1 ? RESCUABLE : 4)) expected=1;
        race_scarab_human_parent_step(parents[owner]);
        Sc64ScarabParent p; Sc64ScarabCounts counts;
        assert(race_scarab_parent_read(parents[owner],0,&p,&counts));
        if ((p.queued!=expected) || race_minerals[owner]!=95-15*expected)
            fprintf(stderr,"AI stock mismatch map=%d bots=%d owner=%d queued=%u bank=%d expected=%d\n",
                    map,bots,owner,p.queued,race_minerals[owner],expected);
        assert(p.queued==expected && race_minerals[owner]==95-15*expected);
        assert(minerals==95); /* Human wallet is never stocked automatically. */
        if (expected) {
            race_minerals[owner]=0; /* Separate insufficient-funds fixture. */
            race_scarab_human_parent_step(parents[owner]);
            assert(race_scarab_parent_read(parents[owner],0,&p,&counts));
            assert(p.queued==1 && race_minerals[owner]==0);
        }
    }
}

int main(void)
{
    for (int map=0;map<=SC64_SKIRMISH_IMPORTED_COUNT;++map)
        for (int bots=1;bots<=3;++bots) check_custom_owners(map,bots);
    puts("paid stocking matches active 1-3 custom AI owners on every prepared map");
    return 0;
}
