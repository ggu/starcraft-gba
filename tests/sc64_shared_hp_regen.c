#include <assert.h>
#include "../starcraft/sc64_shared_hp_regen.h"

int main(void)
{
    int hp=10; unsigned phase=0;
    for (int i=0;i<79;i++) assert(sc64_shared_hp_regen_tick(&hp,20,&phase,1));
    assert(hp==10 && phase==79);
    assert(sc64_shared_hp_regen_tick(&hp,20,&phase,0) && hp==10 && phase==79);
    assert(sc64_shared_hp_regen_tick(&hp,20,&phase,1) && hp==11 && phase==0);
    hp=19; phase=79;
    assert(sc64_shared_hp_regen_tick(&hp,20,&phase,1) && hp==20 && phase==0);
    hp=20; phase=42;
    assert(sc64_shared_hp_regen_tick(&hp,20,&phase,1) && hp==20 && phase==0);
    hp=0; phase=79;
    assert(sc64_shared_hp_regen_tick(&hp,20,&phase,1) && hp==0 && phase==79);
    hp=10; phase=80;
    assert(!sc64_shared_hp_regen_tick(&hp,20,&phase,1) && hp==10 && phase==80);
    return 0;
}
