/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_bunker_rules.h"
int main(void)
{
    const int passengers[]={0,1,7,10,15,16,20,32,34,99,100};
    const int shooters[]={0,1,10,16,20,32,99,100};
    for(int raw=-1;raw<=255;++raw) {
        int passenger=0, shooter=0;
        for(unsigned i=0;i<sizeof(passengers)/sizeof(*passengers);++i) passenger|=raw==passengers[i];
        for(unsigned i=0;i<sizeof(shooters)/sizeof(*shooters);++i) shooter|=raw==shooters[i];
        assert(sc64_bunker_passenger_raw(raw)==passenger);
        assert(sc64_bunker_firing_raw(raw)==shooter);
    }
    for(int owner=-1;owner<=5;++owner) for(int carrier=-1;carrier<=5;++carrier)
        for(int raw=-2;raw<=8;++raw) for(int other=-2;other<=8;++other)
            for(int custom=0;custom<=1;++custom) {
                int valid=owner>=0 && owner<5 && owner==carrier && raw>=-1 && raw<8 && other>=-1 && other<8;
                int expected=valid && (custom || owner==0 || (raw>=0 && other>=0 && raw==other));
                assert(sc64_bunker_owner_matches(owner,raw,carrier,other,custom,0)==expected);
            }
    const int profiles[][2]={{192,96},{240,160}};
    for(unsigned p=0;p<sizeof(profiles)/sizeof(*profiles);++p) {
        int u=profiles[p][0], b=profiles[p][1];
        for(int i=0;i<b;++i) assert(sc64_bunker_target_decode(u,b,sc64_bunker_target_encode(u,b,i))==i);
        assert(sc64_bunker_target_encode(u,b,-1)==-1);
        assert(sc64_bunker_target_encode(u,b,b)==-1);
        assert(sc64_bunker_target_decode(u,b,u-1)==-1);
        assert(sc64_bunker_target_decode(u,b,u+b)==-1);
    }
    assert(sc64_bunker_target_encode(240,272,0)==-1);
    assert(sc64_bunker_target_decode(240,272,240)==-1);
    puts("Bunker source identity and target namespace rules PASS");
    return 0;
}
