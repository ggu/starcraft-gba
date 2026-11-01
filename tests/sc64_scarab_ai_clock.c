/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_scarab_ai_dispatch.h"
typedef struct { unsigned count[SC64_SCARAB_AI_OWNER_LIMIT]; } Calls;
static void decision(void *context,unsigned slot,unsigned owner,unsigned source)
{
    Calls *calls=context;
    assert(owner==1 && (source==1 || source==2));
    ++calls->count[slot];
}
int main(void)
{
    Sc64ScarabAiClock clock={0}; Calls calls={0};
    clock.owner[0]=(Sc64ScarabAiDecision){1,1,1,1};
    clock.owner[1]=(Sc64ScarabAiDecision){1,1,2,2};
    Sc64ScarabAiClock original=clock;
    assert(sc64_scarab_ai_clock_tick(&clock,1,decision,&calls));
    assert(!memcmp(&original,&clock,sizeof(clock)));
    assert(sc64_scarab_ai_clock_tick(&clock,0,decision,&calls));
    assert(calls.count[0]==1 && calls.count[1]==0);
    assert(sc64_scarab_ai_clock_tick(&clock,0,decision,&calls));
    assert(calls.count[0]==1 && calls.count[1]==1);
    original=clock; clock.owner[1].source_owner=1;
    assert(!sc64_scarab_ai_clock_tick(&clock,0,decision,&calls));
    assert(calls.count[0]==1 && calls.count[1]==1);
    clock=original; clock.owner[0].remaining=0;
    assert(!sc64_scarab_ai_clock_valid(&clock));
    clock=original; clock.owner[2].source_owner=1;
    assert(!sc64_scarab_ai_clock_valid(&clock));
    return 0;
}
