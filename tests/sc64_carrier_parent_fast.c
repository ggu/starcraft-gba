/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_P7_CARRIER_TRAINING_ENABLED
#error "Requires an actual Carrier training product vector"
#endif

/* Frozen eager reference: evaluate binding and mapping before alive/type. */
static int eager_parent(const Unit *actors,int count,int slot)
{
    int mapped,source;
    if (!actors || count<0 || count>MAX_UNITS || slot<0 || slot>=count) return 0;
    source=RACE_PROTOSS_SLOT(&actors[slot]);
    mapped=source>=0 && source<8 ? race_protoss_source_owner((unsigned)source) : -1;
    return actors[slot].alive && mapped>=0 && actors[slot].type==U_P7_CARRIER &&
        RACE_PROTOSS_RAW(&actors[slot])==SC64_P7_CARRIER_RAW && actors[slot].owner==mapped;
}

int main(void)
{
    Unit q={0};
    const int raws[]={-32768,-1,0,71,72,73,255,32767};
    const int owners[]={-1,0,1,2,3,4,5,7,8,127};
    const int lives[]={0,1,2,255};
    for (int mode=0;mode<3;++mode) {
        race_mode=mode==0;
        race_selected_mission=SC64_P7_GLOBAL_ID;
#ifdef SC64_P8_RUNTIME
        if(mode==2) race_selected_mission=SC64_P8_GLOBAL_ID;
#endif
        for (unsigned ov=0;ov<2;++ov) {
            race_protoss_binding_override=ov ? SC64_PROTOSS_BIND_GENERIC : -1;
            for (int type=-1;type<=256;++type) for(unsigned a=0;a<4;++a)
            for(unsigned o=0;o<sizeof(owners)/sizeof(owners[0]);++o)
            for(unsigned r=0;r<sizeof(owners)/sizeof(owners[0]);++r)
            for(unsigned t=0;t<sizeof(raws)/sizeof(raws[0]);++t) {
                q.type=type;q.alive=lives[a];q.owner=owners[o];q.raw_owner=owners[r];q.raw_type=raws[t];
                assert(sc64_p7_carrier_parent_valid_in(&q,1,0)==eager_parent(&q,1,0));
            }
        }
    }
    assert(!sc64_p7_carrier_parent_valid_in(NULL,1,0));
    assert(!sc64_p7_carrier_parent_valid_in(&q,1,-1));
    assert(!sc64_p7_carrier_parent_valid_in(&q,1,1));
    assert(!sc64_p7_carrier_parent_valid_in(&q,MAX_UNITS+1,0));
    puts("Carrier parent early rejection matches eager reference");
    return 0;
}
