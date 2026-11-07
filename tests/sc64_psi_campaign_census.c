/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only census of imported campaign starts under shared Pylon policies. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
static int circular(int owner,int x,int y)
{
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (building_complete(i) && b[i].type==B_PYLON && b[i].owner==owner &&
            dist2(x,y,b[i].x,b[i].y)<=144*144) return 1;
    return 0;
}
static int raw_match(int owner,int raw,int x,int y)
{
    if (raw<0) return -1;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (building_complete(i) && b[i].type==B_PYLON && b[i].owner==owner &&
            b[i].raw_owner==raw && sc64_p7_in_psi_range(x-b[i].x,y-b[i].y)) return 1;
    return 0;
}
int main(void)
{
    const int missions[]={SC64_P3_GLOBAL_ID,SC64_P7_GLOBAL_ID,SC64_P10_GLOBAL_ID};
    gba_init();
    for (unsigned m=0;m<sizeof(missions)/sizeof(missions[0]);++m) {
        int count[5]={0}, powered[5]={0}, oldonly[5]={0}, newonly[5]={0}, rawdiff=0,unknown=0;
        game_init(); race_mode=0; race_selected_mission=missions[m]; init_state();
        assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==missions[m]);
        for (int i=0;i<MAX_BUILDINGS;++i) if (building_complete(i)) {
            const Sc64RaceBuildingRule *r=sc64_race_building_rule(b[i].type);
            if (!r || !(r->flags & SC_RACE_F_REQUIRES_POWER)) continue;
            int owner=b[i].owner;
            assert(owner>=0 && owner<5);
            int now=race_site_powered(owner,b[i].x,b[i].y);
            int old=circular(owner,b[i].x,b[i].y);
            int raw=raw_match(owner,b[i].raw_owner,b[i].x,b[i].y);
            ++count[owner]; powered[owner]+=now; oldonly[owner]+=(old&&!now);
            newonly[owner]+=(now&&!old);
            if (raw<0) ++unknown; else rawdiff+=(now&&!raw);
        }
        printf("mission=%d",missions[m]);
        for (int i=0;i<5;++i) printf(" owner%d=%d/%d oldonly=%d newonly=%d",i,powered[i],count[i],oldonly[i],newonly[i]);
        printf(" merged_owner_extra=%d raw_unknown=%d\n",rawdiff,unknown);
    }
    return 0;
}
