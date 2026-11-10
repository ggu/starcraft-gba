/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_research_campaign_policy.h"
static unsigned char rows[10][8][6][2], costs[10][6];
int main(void) {
    int8_t roles[8]={1,1,1,1,1,1,1,0};
    Sc64PrCampaignPolicyView v={rows,costs}, missing=sc64_pr_campaign_generated_view();
    Sc64PrCampaignRule r; Sc64ProtossResearchQuote q;
    assert(!sc64_pr_campaign_rule(&missing,0,"P1",7,roles,0,0,&r));
    for (int ch=0;ch<10;ch++) for (int f=0;f<6;f++) {
        unsigned slot=ch==9?1:7; rows[ch][slot][f][0]=f==5?1:3;
        rows[ch][slot][f][1]=f==5?0:1; costs[ch][f]=1;
    }
    for (int ch=1;ch<=10;ch++) {
        char mission[4]; unsigned slot=ch==10?1:7; snprintf(mission,sizeof mission,"P%d",ch);
        for(int i=0;i<8;i++) roles[i]=1; roles[slot]=0;
        assert(sc64_pr_campaign_rule(&v,0,mission,slot,roles,0,0,&r));
        assert(r.binding.current_controller_key==slot && r.binding.wallet_slot==0 && r.initial==1 && r.maximum==3);
        assert(!sc64_pr_campaign_quote(&r,0,0,&q));
        assert(sc64_pr_campaign_quote(&r,0,1,&q) && q.mineral==175 && q.gas==175 && q.max_level==3);
        assert(!sc64_pr_campaign_quote(&r,0,3,&q));
        assert(!sc64_pr_campaign_rule(&v,0,mission,slot,roles,1,0,&r));
    }
    roles[1]=1; roles[7]=0;
    assert(!sc64_pr_campaign_rule(&v,1,"P1",7,roles,0,0,&r));
    assert(!sc64_pr_campaign_rule(&v,0,"P10",7,roles,0,0,&r));
    assert(!sc64_pr_campaign_rule(&v,0,"P1",7,roles,0,6,&r));
    roles[3]=0; assert(!sc64_pr_campaign_rule(&v,0,"P1",7,roles,0,0,&r)); roles[3]=1;
    costs[0][0]=0; assert(sc64_pr_campaign_rule(&v,0,"P1",7,roles,0,0,&r)); assert(!sc64_pr_campaign_quote(&r,0,1,&q));
    costs[0][0]=2; assert(!sc64_pr_campaign_rule(&v,0,"P1",7,roles,0,0,&r));
    costs[0][0]=1; rows[0][7][0][1]=4; assert(!sc64_pr_campaign_rule(&v,0,"P1",7,roles,0,0,&r));
    puts("campaign policy fail-closed binding and quote: PASS"); return 0;
}
