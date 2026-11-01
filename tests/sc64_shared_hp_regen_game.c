/* Prepared-profile fixture: common actor service, not a mission replay. */
#include <assert.h>
#include "../starcraft/game.c"
#if !defined(SC64_SHARED_HP_REGEN) || !defined(SC64_RACES)
#error "requires shared HP regeneration in a race-engine profile"
#endif
int main(void)
{
    game_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    int z=spawn_unit(SIDE,U_ZERGLING,100,100);
    int enemy=spawn_unit(ENEMY,U_ZERGLING,200,200);
    int marine=spawn_unit(SIDE,U_MARINE,300,300);
    int colony=spawn_build(SIDE,B_CREEP_COLONY,400,400,0);
    assert(z>=0 && enemy>=0 && marine>=0 && colony>=0);
    u[z].hp--;u[enemy].hp--;u[marine].hp--;b[colony].hp--;
    u[z].shield_remainder=79;u[enemy].shield_remainder=79;
    u[marine].shield_remainder=79;b[colony].shield_remainder=79;
    assert(race_shared_hp_regen_unit_phase_valid(&u[z]));
    u[z].shield_remainder=80;
    assert(!race_shared_hp_regen_unit_phase_valid(&u[z]));
    u[z].shield_remainder=79;
#ifdef SC64_TRANSPORT_RUNTIME
    u[z].order=ORDER_TRANSPORTED; /* hidden completed passenger remains eligible */
#endif
    int zh=u[z].hp,eh=u[enemy].hp,mh=u[marine].hp,bh=b[colony].hp;
    b[colony].under_construction=1;
    b[colony].raw_type=sc64_race_raw_building(B_SUNKEN_COLONY);
    assert(race_shared_hp_regen_building_phase_valid(&b[colony],1));
    assert(!race_shared_hp_regen_building_phase_valid(&b[colony],0));
    race_shared_hp_regen_tick();
    assert(u[z].hp==zh+1 && u[enemy].hp==eh+1 && u[marine].hp==mh);
    assert(b[colony].hp==bh && b[colony].shield_remainder==79);
    b[colony].under_construction=0;
    b[colony].raw_type=sc64_race_raw_building(B_CREEP_COLONY);
    race_shared_hp_regen_tick();
    assert(b[colony].hp==bh+1 && b[colony].shield_remainder==0);
    assert(u[marine].shield_remainder==79);
    return 0;
}
