/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_scarab_order_state.h"
int main(void) {
    const unsigned native[]={0,2,3,6},bridge[]={1,2,3,4};
    for(unsigned i=0;i<4;i++) { unsigned a=99,b=99;
        assert(sc64_scarab_native_to_bridge(native[i],&a) && a==bridge[i]);
        assert(sc64_scarab_bridge_to_native(a,&b) && b==native[i]);
    }
    for(unsigned i=0;i<256;i++) { unsigned b=99,n=99;
        if(i!=0 && i!=2 && i!=3 && i!=6) assert(!sc64_scarab_native_to_bridge(i,&b) && b==99);
        if(i<1 || i>4) assert(!sc64_scarab_bridge_to_native(i,&n) && n==99);
    }
    assert(!sc64_scarab_native_to_bridge(0,0) && !sc64_scarab_bridge_to_native(1,0));
    puts("explicit Scarab native/bridge state mapping passed");return 0;
}
