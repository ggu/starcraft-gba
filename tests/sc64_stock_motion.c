/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_stock_motion.h"
int main(void)
{
    for (int raw=0;raw<228;++raw) {
        const Sc64StockUnitProfile *p=sc64_stock_unit_profile(raw);
        unsigned total=0,diagonal=0;
        for (unsigned tick=0;tick<1024;++tick) {
            total+=sc64_stock_motion_step(raw,tick,3,0);
            diagonal+=sc64_stock_motion_step(raw,tick,3,1);
            assert(sc64_stock_motion_step(raw,tick,3,0)==
                   sc64_stock_motion_step(raw,tick+1024,3,0));
        }
        assert(total==sc64_stock_runtime_speed_q10(p));
        assert(diagonal==(total*724u+512u)>>10);
    }
    assert(!sc64_stock_motion_step(30,8,0,0)); /* Sieged tank. */
    assert(sc64_stock_motion_axis(10,11,3)==11);
    assert(sc64_stock_motion_axis(10,9,3)==9);
    assert(sc64_stock_motion_axis(10,20,2)==12);
    puts("Stock nominal speed, diagonal normalization, save phase and endpoint checks passed");
    return 0;
}
