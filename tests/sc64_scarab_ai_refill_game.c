/* SPDX-License-Identifier: GPL-3.0-or-later
 * Seeded owner/payment checks for the disabled Scarab AI enqueue service. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"

static int parent(int owner,int source)
{
    int slot=spawn_unit(owner,U_REAVER,128,128);assert(slot>=0);
    u[slot].raw_owner=source;assert(race_scarab_parent_init(slot));return slot;
}
static void custom_bots(void)
{
    const int owners[]={ENEMY,RESCUABLE,4};int slots[3];
    memset(u,0,sizeof(u));race_mode=1;race_bots=3;minerals=101;
    for(int i=0;i<3;i++) {race_minerals[owners[i]]=200+i*100;slots[i]=parent(owners[i],-1);}
    for(int i=0;i<3;i++) {
        assert(race_scarab_ai_enqueue_one(slots[i],owners[i],-1));
        for(int j=0;j<3;j++)assert(race_minerals[owners[j]]==200+j*100-(j<=i?15:0));
    }
    int human=parent(SIDE,-1);Unit before=u[human];
    assert(!race_scarab_ai_enqueue_one(human,SIDE,-1));
    assert(!memcmp(&before,&u[human],sizeof(before))&&minerals==101);
    before=u[slots[0]];
    assert(!race_scarab_ai_enqueue_one(slots[0],RESCUABLE,-1));
    assert(!race_scarab_ai_enqueue_one(slots[0],ENEMY,7));
    assert(!memcmp(&before,&u[slots[0]],sizeof(before))&&race_minerals[ENEMY]==185);
    race_bots=1;before=u[slots[1]];
    assert(!race_scarab_ai_enqueue_one(slots[1],RESCUABLE,-1));
    assert(!memcmp(&before,&u[slots[1]],sizeof(before))&&race_minerals[RESCUABLE]==285);
    race_bots=3;race_minerals[ENEMY]=14;before=u[slots[0]];
    assert(!race_scarab_ai_enqueue_one(slots[0],ENEMY,-1));
    assert(!memcmp(&before,&u[slots[0]],sizeof(before))&&race_minerals[ENEMY]==14);
    race_minerals[ENEMY]=300;
    for(int i=0;i<4;i++)assert(race_scarab_ai_enqueue_one(slots[0],ENEMY,-1));
    before=u[slots[0]];int bank=race_minerals[ENEMY];
    assert(!race_scarab_ai_enqueue_one(slots[0],ENEMY,-1));
    assert(!memcmp(&before,&u[slots[0]],sizeof(before))&&race_minerals[ENEMY]==bank);
}
#ifdef SC64_IMPORTED_SKIRMISH
/* Initialize both actual imported maps, every human race and each bot count.
 * Parents are seeded after setup; this is not natural Reaver production. */
static void imported_map_owner_matrix(void)
{
    const int owners[]={ENEMY,RESCUABLE,4};
    assert(SC64_SKIRMISH_IMPORTED_COUNT>=2);
    for(int map=1;map<=2;map++)for(int human=0;human<3;human++)for(int bots=1;bots<=3;bots++) {
        race_mode=1;race_skirmish_map=map;race_human=human;race_bots=bots;
        for(int i=0;i<3;i++)race_bot_races[i]=(human+i)%3;
        race_skirmish_init();
        assert(race_mode&&race_skirmish_map==map&&race_human==human&&race_bots==bots);
        int human_bank=minerals,slots[3];
        for(int i=0;i<bots;i++) {
            race_minerals[owners[i]]=200+i*100;
            slots[i]=parent(owners[i],-1);
        }
        for(int i=0;i<bots;i++) {
            assert(race_scarab_ai_enqueue_one(slots[i],owners[i],-1));
            for(int j=0;j<bots;j++)
                assert(race_minerals[owners[j]]==200+j*100-(j<=i?15:0));
        }
        assert(minerals==human_bank);
    }
}
#endif
static void campaign_banks(int mission,int source_a,int source_b,uint32_t *a,uint32_t *b)
{
    memset(u,0,sizeof(u));race_mode=0;race_selected_mission=mission;
    minerals=101;race_minerals[ENEMY]=900;*a=200;*b=300;
    int one=parent(ENEMY,source_a),two=parent(ENEMY,source_b),human=parent(SIDE,7);
    assert(race_scarab_ai_enqueue_one(one,ENEMY,source_a));
    assert(*a==185&&*b==300&&race_minerals[ENEMY]==900);
    assert(race_scarab_ai_enqueue_one(two,ENEMY,source_b));
    assert(*a==185&&*b==285&&race_minerals[ENEMY]==900);
    assert(!race_scarab_ai_enqueue_one(human,SIDE,7)&&minerals==101);
    Unit before=u[one];assert(!race_scarab_ai_enqueue_one(one,ENEMY,source_b));
    assert(!memcmp(&before,&u[one],sizeof(before))&&*a==185&&*b==285);
    u[one].raw_owner=6;before=u[one];
    assert(!race_scarab_ai_enqueue_one(one,ENEMY,6));
    assert(!memcmp(&before,&u[one],sizeof(before))&&race_minerals[ENEMY]==900);
}
int main(void)
{
    gba_init();custom_bots();
#ifdef SC64_IMPORTED_SKIRMISH
    imported_map_owner_matrix();
#endif
#ifdef SC64_P7_RUNTIME
    campaign_banks(SC64_P7_GLOBAL_ID,0,4,&race_p7.source_minerals[0],&race_p7.source_minerals[4]);
#endif
#ifdef SC64_P8_RUNTIME
    campaign_banks(SC64_P8_GLOBAL_ID,0,3,&race_p8.ai_minerals[0],&race_p8.ai_minerals[1]);
#endif
    puts("Scarab AI one-entry owner/payment checks passed");return 0;
}
