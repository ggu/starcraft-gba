#include <assert.h>

#define SC64_Z5_RUNTIME 1
#include "../starcraft/sc64_z5_hp_regen.h"

static void test_exact_eighty_tick_cadence_and_resume(void)
{
    int hp=20;
    u16 phase=0;
    for (unsigned tick=0;tick<79;++tick) {
        assert(sc64_z5_hp_regen_tick(SC64_Z5_HP_REGEN_RAW_ZERGLING,
                                     &hp,100,&phase,0)==SC64_Z5_HP_REGEN_ADVANCED);
        assert(hp==20);
    }
    assert(phase==79);

    /* The existing persisted remainder alone carries the fractional phase. */
    int restored_hp=hp;
    u16 restored_phase=phase;
    assert(sc64_z5_hp_regen_tick(SC64_Z5_HP_REGEN_RAW_ZERGLING,
                                 &restored_hp,100,&restored_phase,0)==
           SC64_Z5_HP_REGEN_ADVANCED);
    assert(restored_hp==21 && restored_phase==0);
    assert(sc64_z5_hp_regen_tick(SC64_Z5_HP_REGEN_RAW_INFESTED_KERRIGAN,
                                 &hp,100,&phase,0)==SC64_Z5_HP_REGEN_ADVANCED);
    assert(hp==21 && phase==0);
}

static void test_pause_dead_and_full_hp(void)
{
    int hp=10;
    u16 phase=79;
    assert(sc64_z5_hp_regen_tick(37,&hp,100,&phase,1)==SC64_Z5_HP_REGEN_UNCHANGED);
    assert(hp==10 && phase==79);
    assert(sc64_z5_hp_regen_tick(37,&hp,100,&phase,0)==SC64_Z5_HP_REGEN_ADVANCED);
    assert(hp==11 && phase==0);

    hp=0; phase=45;
    assert(sc64_z5_hp_regen_tick(51,&hp,400,&phase,0)==SC64_Z5_HP_REGEN_UNCHANGED);
    assert(hp==0 && phase==45);
    hp=-1; phase=45;
    assert(sc64_z5_hp_regen_tick(37,&hp,35,&phase,0)==SC64_Z5_HP_REGEN_UNCHANGED);
    assert(hp==-1 && phase==45);

    hp=35; phase=12;
    assert(sc64_z5_hp_regen_tick(37,&hp,35,&phase,0)==SC64_Z5_HP_REGEN_ADVANCED);
    assert(hp==35 && phase==0);
    hp=35; phase=0;
    assert(sc64_z5_hp_regen_tick(37,&hp,35,&phase,0)==SC64_Z5_HP_REGEN_UNCHANGED);
    assert(hp==35 && phase==0);
}

static void test_invalid_state_is_transactional(void)
{
    int hp=20;
    u16 phase=80;
    assert(sc64_z5_hp_regen_tick(37,&hp,35,&phase,0)==SC64_Z5_HP_REGEN_ERROR);
    assert(hp==20 && phase==80);
    hp=36; phase=17;
    assert(sc64_z5_hp_regen_tick(51,&hp,35,&phase,0)==SC64_Z5_HP_REGEN_ERROR);
    assert(hp==36 && phase==17);
    hp=20; phase=17;
    assert(sc64_z5_hp_regen_tick(37,&hp,0,&phase,0)==SC64_Z5_HP_REGEN_ERROR);
    assert(hp==20 && phase==17);
    assert(sc64_z5_hp_regen_tick(37,&hp,35,&phase,2)==SC64_Z5_HP_REGEN_ERROR);
    assert(hp==20 && phase==17);
}

static void test_other_actor_is_untouched(void)
{
    int hp=20;
    u16 phase=1279; /* Range used by shields on unrelated actors. */
    assert(sc64_z5_hp_regen_tick(44,&hp,150,&phase,0)==SC64_Z5_HP_REGEN_UNCHANGED);
    assert(hp==20 && phase==1279);
}

int main(void)
{
    test_exact_eighty_tick_cadence_and_resume();
    test_pause_dead_and_full_hp();
    test_invalid_state_is_transactional();
    test_other_actor_is_untouched();
    return 0;
}
