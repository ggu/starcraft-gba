/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FRONTEND_STATE_H
#define SC64_FRONTEND_STATE_H

/* Title, setup and briefing state shared by the race cartridges. */
static const Sc64BriefingPage *race_front_briefing_page(void)
{
    const Sc64Briefing *briefing=sc64_briefing_get(race_front.mission);
    return briefing && race_front.briefing_page < briefing->page_count ?
        &briefing->pages[race_front.briefing_page] : NULL;
}

/* Generated text stays in ROM. Only a page cursor and reveal clock live in
 * RAM, and playback is configured in update rather than as a render effect. */
static void race_front_briefing_configure(void)
{
    if(race_front.page!=SC_FRONT_BRIEFING)return;
    const Sc64Briefing *briefing=sc64_briefing_get(race_front.mission);
    const Sc64BriefingPage *page=race_front_briefing_page();
    unsigned length=0;
    if(page)while(length<SC64_BRIEFING_MAX_TEXT_BYTES && page->text[length])length++;
    sc64_frontend_briefing_configure(&race_front,
        briefing?briefing->page_count:1,length);
}
static void race_frontend_reset(void)
{
    int completed=-1;
    for(int i=0;i<SC64_RACE_MAP_COUNT;i++)if(race_completed&(1<<i))completed=i;
    sc64_frontend_reset(&race_front,race_selected_mission,race_human,race_bots,completed);
    race_front.map=race_skirmish_map;
#ifdef SC64_IMPORTED_SKIRMISH
#ifdef SC64_NATIVE_MELEE_STARTS
    race_front.skirmish_melee=race_skirmish_melee && race_front.map==SC64_MAP_BLOOD_BATH;
#endif
    unsigned seats=race_skirmish_slots ? race_skirmish_slots : sc64_skirmish_dense_seats(race_front.bots);
    int rank=0;
    for(int i=0;i<3;i++){
        int race=race_bot_races[seats&(1u<<i) ? rank++ : i];
        race_front.bot_last_races[i]=race;
        race_front.bot_races[i]=seats&(1u<<i) ? race : SC_FRONT_BOT_CLOSED;
    }
#else
    for(int i=0;i<3;i++)race_front.bot_races[i]=race_bot_races[i];
#endif
    race_front_transition=0;race_front_load_error=0;
}

#endif
