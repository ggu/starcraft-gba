/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The plain unified profile has no extended actor-facts API or Battery type. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_UNIFIED) || !defined(SC64_RACES) || !defined(SC64_SHARED_BATTERY)
#error "plain unified shared Battery profile required"
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "fixture must exercise the native actor fallback"
#endif
static RaceSave candidate;
int main(void)
{
    Unit actor = {0};
    Building target = {0};
    assert(!SC64_BATTERY_UNIT_IMMOVABLE(U_ZEALOT));
    assert(sc64_race_raw_unit(U_ZEALOT) >= 0);
    /* A forged source identity cannot turn a native building into a Battery. */
    actor.alive=1; actor.type=U_ZEALOT; actor.owner=SIDE;
    actor.raw_type=-1; actor.raw_owner=-1; actor.shield=1;
    actor.order=ORDER_RECHARGE; actor.target=0;
    target.alive=1; target.type=B_NEXUS; target.owner=SIDE;
    target.raw_type=172; target.raw_owner=-1;
    assert(sc64_race_raw_building(target.type)!=172);
    u[0]=actor; b[0]=target;
    assert(!race_battery_unit_valid(0,0));
    for (unsigned i=0;i<RACE_BUILD_WORDS;++i) {
        int n; memcpy(&n,(const u8 *)&target+i*sizeof(int),sizeof(n));
        candidate.buildings[0][i]=(s16)n;
    }
    candidate.state[0]=1;
    assert(!race_battery_saved_recharge_valid(&candidate,&actor));
    puts("plain unified Battery native mobility and forged target rejection: ok");
    return 0;
}
