#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_p7_battery_rules.h"

int main(void)
{
    unsigned e=100*256, phase=0, sh=0, rem=0;
    for (int i=0;i<5;i++) sc64_p7_battery_regenerate(&e,&phase);
    assert(e==100*256+32 && phase==0);
    e=100*256; sh=0; rem=0;
    assert(sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(sh==4 && rem==0 && e==100*256-2*256);
    e=100*256; sh=199; rem=1275;
    assert(sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(sh==200 && rem==0 && e==100*256); /* one Q8-unit native floor edge */
    e=100*256; sh=200; rem=1;
    assert(!sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(e==100*256 && rem==1); /* malformed full+fraction never wraps */
    e=0; sh=0; rem=0;
    assert(!sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(e==0 && sh==0 && rem==0);
    e=SC64_P7_BATTERY_MAX_ENERGY_Q8; sh=0; rem=0;
    assert(sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(sh==4 && rem==0 && e==SC64_P7_BATTERY_MAX_ENERGY_Q8-512);
    e=100*256; sh=199; rem=1276;
    assert(sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(sh==200 && rem==0 && e==100*256); /* bounded non-Q8 final gap */
    e=1; sh=0; rem=0;
    assert(sc64_p7_battery_transfer(&e,&sh,200,&rem));
    assert(sh==0 && rem==10 && e==0);
    e=SC64_P7_BATTERY_MAX_ENERGY_Q8-1; phase=0;
    sc64_p7_battery_regenerate(&e,&phase);
    assert(e==SC64_P7_BATTERY_MAX_ENERGY_Q8 && phase==0);
    assert(!sc64_p7_battery_energy_valid(SC64_P7_BATTERY_MAX_ENERGY_Q8,1));
    puts("P7 battery fixed-point rules: ok");
    return 0;
}
