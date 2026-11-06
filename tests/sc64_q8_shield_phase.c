/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_q8_shield_phase.h"
int main(void)
{
    for(unsigned s=0;s<=4;++s) for(unsigned p=0;p<1280;++p)
        for(unsigned d=0;d<=1025;++d) {
            unsigned a=s,b=p;uint32_t taken=999;
            uint32_t total=s*1280u+p;
            uint32_t exp=d<total/5u?d:total/5u;
            assert(sc64_q8_shield_damage(&a,&b,d,&taken));
            assert(taken==exp && a*1280u+b==total-exp*5u);
            assert((a*1280u+b)%5u==total%5u);
        }
    for(unsigned p=0;p<320;++p) {
        unsigned migrated=999; assert(sc64_q8_phase_320_to_1280(p,&migrated));
        assert(migrated==p*4u);
        /* One old +7/320 tick equals the new +28/1280 tick. */
        assert((migrated+28u)/1280u==(p+7u)/320u);
        assert((migrated+28u)%1280u==((p+7u)%320u)*4u);
    }
    {unsigned s=2,p=1280;uint32_t n=7;
     assert(!sc64_q8_shield_damage(&s,&p,1,&n));assert(s==2&&p==1280&&n==7);}
    puts("6566400 shield arithmetic cases and 320 migrations passed");
    return 0;
}
