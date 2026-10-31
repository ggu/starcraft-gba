/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DEFILER_EXECUTOR_H
#define SC64_DEFILER_EXECUTOR_H
#define sc64_interceptor_source_frame_due sc64_defiler_unused_frame_due
#include "sc64_interceptor_motion_kernel.h"
#undef sc64_interceptor_source_frame_due
/* In-range stationary experimental cast. General movement, visibility,
 * rendering, miss rules, and a save codec are intentionally not admitted. */
static int sc64_defiler_executor_busy(void)
{
    for(int i=0;i<MAX_UNITS;i++)
        if(sc64_defiler_fields[i].remaining || sc64_defiler_stage[i].intent) return 1;
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++)
        if(sc64_defiler_projectiles[i].handle) return 1;
    return 0;
}
static void sc64_defiler_executor_unit_tick(void)
{
    if(!sc64_battlecruiser_source_tick_due()) return;
    /* Source units tick before bullets: new fields begin at 900. */
    for(int i=0;i<MAX_UNITS;i++) {
        Sc64DarkSwarmEffect *f=&sc64_defiler_fields[i];
        if(f->remaining>900) {
            if(f->remaining==901) f->remaining=0; else --f->remaining;
        } else if(f->remaining && sc64_dark_swarm_source_tick(f)) {
            /* Four further source samples in the dead-unit list retain the
             * actor slot; values above 900 are private inactive sentinels. */
            f->remaining=904;
        }
    }
    for(int i=0;i<MAX_UNITS;i++) {
        Sc64DefilerStageActor *a=&sc64_defiler_stage[i]; Unit *q=&u[i];
        if(!a->live)continue;
        if(!q->alive || !target_alive(i)) { a->intent=0; a->phase=0;continue; }
        if(a->energy_q8<200u*256u) {
            unsigned e=a->energy_q8+8u;a->energy_q8=(u16)(e>200u*256u?200u*256u:e);
        }
        if(!a->intent)continue;
        if(a->phase==0) {
            int dx=(a->target_x-q->x)*2,dy=(a->target_y-q->y)*2;
            /* Only the controlled interior is admitted. Source rectangle
             * range, halt distance, visibility and approach remain open. */
            if((s32)dx*dx+(s32)dy*dy>256*256)continue;
            u8 desired=(dx||dy)?sc64_interceptor_dir(dx,dy):a->heading;
            if(a->age)sc64_interceptor_toward(&a->heading,desired,27);
            else a->age=1;
            if(sc64_interceptor_abs32(sc64_interceptor_turn_delta(a->heading,desired))>16)continue;
            if(a->energy_q8<100u*256u) { a->intent=0;continue; }
            a->energy_q8-=100u*256u;a->phase=1;a->age=0;
        } else if(a->phase==1 && ++a->age==3) {
            u32 handle=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_SWARM,0);
            if(handle) {
                unsigned slot=(handle&255u)-1u;
                sc64_defiler_projectiles[slot]=(Sc64DefilerProjectile){handle,a->target_x,a->target_y,255};
            }
            a->phase=2;a->age=0;
        } else if(a->phase==2 && ++a->age==4) {
            a->intent=0;a->phase=0;
        }
    }
}
static void sc64_defiler_executor_bullet_tick(void)
{
    if(!sc64_battlecruiser_source_tick_due())return;
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) {
        Sc64DefilerProjectile *p=&sc64_defiler_projectiles[i];
        if(!p->handle)continue;
        if(p->age==255) {p->age=0;continue;}
        if(++p->age==5) {
            int slot=find_unit_slot();
            if(slot>=0) {
                int x=p->x*2,y=p->y*2,w=WORLD_W*2,h=WORLD_H*2;
                if(w>=160 && h>=192) {
                    sc64_unit_prepare_reuse(slot);
                    memset(&u[slot],0,sizeof(u[slot]));
                    memset(&sc64_defiler_stage[slot],0,sizeof(sc64_defiler_stage[slot]));
                    if(x<80)x=80;else if(x+80>=w)x=w-80;
                    if(y<80)y=80;else if(y+80>=h-32)y=h-112;
                    sc64_dark_swarm_created(&sc64_defiler_fields[slot],x,y);
                }
            }
        }
        if(p->age==6) {
            (void)sc64_shared_projectile_release(&sc64_shared_projectiles,p->handle,SC64_SHARED_PROJECTILE_SWARM,0);
            memset(p,0,sizeof(*p));
        }
    }
}
#endif
