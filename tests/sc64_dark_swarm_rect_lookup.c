/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include "sc64_dark_swarm_rect_lookup.h"
int main(void)
{
    int l,t,r,d;
    assert(sc64_dark_swarm_source_rect(0,400,320,2048,2048,&l,&t,&r,&d));
    assert(l==792 && t==631 && r==809 && d==651);
    assert(!sc64_dark_swarm_source_rect(228,1,1,2048,2048,&l,&t,&r,&d));
    assert(!sc64_dark_swarm_source_rect(0,-1,1,2048,2048,&l,&t,&r,&d));
    assert(!sc64_dark_swarm_source_rect(0,2048,1,2048,2048,&l,&t,&r,&d));
    assert(!sc64_dark_swarm_source_rect(0,INT_MAX-1,1,INT_MAX,2048,&l,&t,&r,&d));
    assert(!sc64_dark_swarm_source_rect(0,1,INT_MAX-1,2048,INT_MAX,&l,&t,&r,&d));
    return 0;
}
