/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_bc_range_private.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(sc64_bc_source_range_distance(12,100,100,0,283,100)==320);
    assert(sc64_bc_source_range_distance(12,100,100,0,284,100)==322);
    assert(sc64_bc_source_range_distance(12,100,100,0,100,279)==319);
    assert(sc64_bc_source_range_distance(12,100,100,0,100,280)==321);
    assert(sc64_bc_source_range_distance(12,100,100,12,100,100)==0);
    assert(sc64_bc_source_range_distance(228,100,100,0,100,100)==-1);
    for (unsigned raw=0;raw<228;++raw) for (int x=0;x<2048;x+=37) {
        int a=sc64_bc_source_range_distance(12,100,100,raw,x,150);
        int b=sc64_bc_source_range_distance(raw,x,150,12,100,100);
        assert(a>=0 && a==b);
    }
    puts("BC stationary rectangle distance: source boundaries and symmetry pass");
}
