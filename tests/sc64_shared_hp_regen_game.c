/* Prepared-profile fixture: common actor service, not a mission replay. */
#include <assert.h>
#include "../starcraft/game.c"
#if !defined(SC64_SHARED_HP_REGEN) || !defined(SC64_RACES)
#error "requires shared HP regeneration in a race-engine profile"
#endif
/* Actor-owned phases must not share a global clock or owner restriction.
 * This is controlled fixture setup, not ordinary-input play. */
static void test_actor_phase_owners_and_reuse(void)
{
    int slots[5];
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    for (int owner=0;owner<5;++owner) {
        int slot=spawn_unit(owner,U_ZERGLING,100+owner*20,100);
        assert(slot>=0);slots[owner]=slot;
        u[slot].hp=u[slot].maxhp-2;
        u[slot].shield_remainder=(unsigned)owner;
    }
    for (int tick=0;tick<79;++tick) race_shared_hp_regen_tick();
    for (int owner=0;owner<5;++owner) {
        Unit *q=&u[slots[owner]];
        assert(q->hp==q->maxhp-2+(owner>0));
        assert(q->shield_remainder==(unsigned)((owner+79)%80));
    }
    race_shared_hp_regen_tick();
    for (int owner=0;owner<5;++owner) {
        Unit *q=&u[slots[owner]];
        assert(q->hp==q->maxhp-1 && q->shield_remainder==(unsigned)owner);
    }
    /* A dead slot must not inherit the old occupant's phase on spawn. */
    int slot=slots[0];u[slot].alive=0;u[slot].shield_remainder=63;
    assert(spawn_unit(SIDE,U_ZERGLING,200,200)==slot);
    assert(u[slot].shield_remainder==0);
    u[slot].hp=u[slot].maxhp-1;
    for(int tick=0;tick<79;++tick) race_shared_hp_regen_tick();
    assert(u[slot].hp==u[slot].maxhp-1 && u[slot].shield_remainder==79);
    race_shared_hp_regen_tick();
    assert(u[slot].hp==u[slot].maxhp && u[slot].shield_remainder==0);
}
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
    test_actor_phase_owners_and_reuse();
    return 0;
}
