/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compile this same actual-bank render probe with and without the legacy
 * identity expression; compare every emitted facing and framebuffer hash. */
#include "../common/gba.h"
#include "../starcraft/race_rules.h"
#ifdef SC64_TEST_LEGACY_BC_RENDER_IDENTITY
#define sc64_race_is_battlecruiser(type) (sc64_race_raw_unit(type)==12)
#endif
#define main sc64_bc_unused_skirmish_main
#include "sc64_skirmish_runtime.c"
#undef main
#include <stdint.h>
int main(void)
{
    unsigned cases=0,bc_cases=0,nonempty=0;
    gba_init();game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    camx=camy=0;memset(visible,0xff,sizeof(visible));
    assert(sc64_unit_facing_frames(12));
    for(int type=-2;type<=SC_RU_COUNT+2;++type)
        assert(sc64_race_is_battlecruiser(type)==(sc64_race_raw_unit(type)==12));
    for(int type=0;type<SC_RU_COUNT;++type) {
        int bc=sc64_race_raw_unit(type)==12;
        if(!bc && type!=U_MARINE && type!=U_ZEALOT) continue;
        for(int binding=0;binding<2;++binding) for(int mode=0;mode<4;++mode)
          for(int heading=0;heading<256;++heading) {
            memset(u,0,sizeof(u));memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));clear_tracers();
            Unit *q=&u[0];q->alive=1;q->owner=SIDE;q->raw_owner=-1;q->type=type;
            q->raw_type=binding?sc64_race_raw_unit(type):-1;q->x=128;q->y=64;q->tx=160;q->ty=80;
            q->path_x=160;q->path_y=80;q->path_timer=9;q->hp=q->maxhp=100;
            q->order=mode==0?ORDER_IDLE:mode==1?ORDER_MOVE:ORDER_ATTACK_TARGET;
            q->cd=mode==2?7:0;q->work_timer=(heading<<3)|(mode==3?3:0);
            sc64_facing[0]=0xa5;frame=17;gfx_begin(0);
            draw_unit(q);draw_tracers();
            uint64_t hash=UINT64_C(1469598103934665603);unsigned changed=0;
            for(int y=0;y<160;++y)for(int x=0;x<240;++x) {
                unsigned px=gfx_test_pixel(x,y);hash=(hash^px)*UINT64_C(1099511628211);changed|=px;
            }
            if(bc) {++bc_cases;if(changed)++nonempty;}
            printf("%d %d %d %d %u %016llx\n",type,binding,mode,heading,
                   (unsigned)sc64_facing[0],(unsigned long long)hash);
            ++cases;
          }
    }
    assert(bc_cases && nonempty==bc_cases);
    fprintf(stderr,"actual-bank renderer cases=%u BC=%u nonempty=%u\n",cases,bc_cases,nonempty);
    return 0;
}
