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
        if(sc64_defiler_projectile_at(i)) return 1;
    return 0;
}
#ifdef SC64_SHARED_DEFILER_APPROACH_PROBE
/* Source-Q8 direct, unobstructed stationary-start slice. The source walking
 * IScript supplies four source pixels per update; the spell order rechecks
 * its range every nine source updates. Collision/pathing outside this direct
 * slice is deliberately left pending, not approximated with stock steering. */
static int sc64_defiler_clear_approach(Sc64DefilerStageActor *a,Unit *q)
{
    if(a->blocked)return 0;
    if(!a->motion_age) {
        if(q->x<0 || q->y<0 || q->x>=WORLD_W || q->y>=WORLD_H) {a->blocked=1;return 0;}
        if(a->target_x!=q->x && a->target_y!=q->y) {
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
            int dx=a->target_x-q->x,dy=a->target_y-q->y;
            int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
            if((raw!=46
#ifdef SC64_SHARED_DEFILER_SE_HERO_PROBE
                && raw!=52
#endif
               ) || dx<140 || dx>224 || dx!=dy ||
               (q->x&31) || (q->y&31) || a->heading!=104) {a->blocked=1;return 0;}
            a->diagonal=1;a->turning=1;a->velocity=a->heading;
            a->waypoint_x=(s16)(q->x*2+64);a->waypoint_y=(s16)(q->y*2+64);
#else
            a->blocked=1;return 0;
#endif
        }
        if(!direct_path_clear(q->x,q->y,a->target_x,a->target_y)) {a->blocked=1;return 0;}
        a->x_q8=(s32)q->x*512;a->y_q8=(s32)q->y*512;a->moving=1;
    }
    ++a->motion_age;
    s32 dx=(s32)a->target_x*512-a->x_q8,dy=(s32)a->target_y*512-a->y_q8;
    u8 desired=(dx||dy)?sc64_interceptor_dir(dx,dy):a->heading;
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
    if(a->diagonal) {
        int sx=sc64_interceptor_integer_part(a->x_q8),sy=sc64_interceptor_integer_part(a->y_q8);
        if(a->motion_age>3 && a->x_q8==(s32)a->waypoint_x*256 && a->y_q8==(s32)a->waypoint_y*256) {
            a->waypoint_x+=64;a->waypoint_y+=64;
        }
        desired=sc64_interceptor_dir(a->waypoint_x-sx,a->waypoint_y-sy);
    }
#endif
    int order_due=(a->motion_age%9u)==1u;
    if(a->stopped) {
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
        if(a->diagonal) {
            int sx=sc64_interceptor_integer_part(a->x_q8),sy=sc64_interceptor_integer_part(a->y_q8);
            int tx=(int)a->target_x*2-sx,ty=(int)a->target_y*2-sy;
            if(tx||ty)sc64_interceptor_toward(&a->heading,sc64_interceptor_dir(tx,ty),27);
        }
#endif
        return order_due;
    }
    int stop=order_due && sc64_interceptor_len_raw(dx,dy)<=288*256;
    if(a->motion_age>=3) {
        int can_move=a->motion_age>=4 && sc64_interceptor_abs32(sc64_interceptor_turn_delta(a->heading,desired))<32;
        u8 motion_direction;
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
        if(a->diagonal) {
            if(!a->turning)can_move=a->motion_age>=4;
            sc64_interceptor_toward(&a->velocity,desired,27);
            if(a->turning) {
                sc64_interceptor_toward(&a->heading,desired,27);
                if(a->heading==desired && a->velocity==desired)a->turning=0;
            }
            motion_direction=a->velocity;
        } else
#endif
        { sc64_interceptor_toward(&a->heading,desired,27);motion_direction=a->heading; }
        if(can_move && (dx||dy)) {
            Sc64InterceptorDirectionVector v=sc64_interceptor_direction_vector[motion_direction];
            s32 nx=a->x_q8+sc64_interceptor_integer_part((s32)v.x*1024);
            s32 ny=a->y_q8+sc64_interceptor_integer_part((s32)v.y*1024);
#ifdef SC64_SHARED_DEFILER_SE_APPROACH_PROBE
            if(a->diagonal && sc64_interceptor_len_raw((s32)a->waypoint_x*256-a->x_q8,
                (s32)a->waypoint_y*256-a->y_q8)<=1024) {
                nx=(s32)a->waypoint_x*256;ny=(s32)a->waypoint_y*256;
            }
#endif
            int gx=sc64_interceptor_integer_part(nx)/2,gy=sc64_interceptor_integer_part(ny)/2;
            if(!is_traversable(gx/16,gy/16,q->x/16+(q->y/16)*MAP_COLS) ||
               !unit_step_clear(q,gx,gy)) {a->blocked=1;return 0;}
            a->x_q8=nx;a->y_q8=ny;q->x=gx;q->y=gy;
            sc64_unit_bucket_move(q);sc64_target_index_move(q);
        }
    }
    if(stop) {a->moving=0;a->stopped=1;}
    return 0;
}
#endif
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
        int canonical_raw=sc64_race_raw_unit(q->type);
        int actual_raw=q->raw_type>=0?q->raw_type:canonical_raw;
        if(actual_raw!=canonical_raw || (actual_raw!=46 && actual_raw!=52)) {
            a->intent=0;a->phase=0;continue;
        }
        unsigned max_energy=(actual_raw==52 ? 250u : 200u)*256u;
        if(a->energy_q8<max_energy) {
            unsigned e=a->energy_q8+8u;a->energy_q8=(u16)(e>max_energy?max_energy:e);
        }
        if(!a->intent)continue;
        if(a->phase==0) {
            /* Reject corrupt noncompact coordinates before subtraction,
             * doubling or squared-distance arithmetic. */
            if(MAP_COLS<=0 || MAP_ROWS<=0 || MAP_COLS>1024 || MAP_ROWS>1024 ||
               q->x<0 || q->y<0 || q->x>16383 || q->y>16383 ||
               q->x>=WORLD_W || q->y>=WORLD_H || a->target_x<0 || a->target_y<0 ||
               a->target_x>16383 || a->target_y>16383 ||
               a->target_x>=WORLD_W || a->target_y>=WORLD_H) {
                a->intent=0;a->phase=0;continue;
            }
            int dx=(a->target_x-q->x)*2,dy=(a->target_y-q->y)*2;
            /* The rectangle/halt boundary and visibility remain open. */
#ifdef SC64_SHARED_DEFILER_APPROACH_PROBE
            if(a->motion_age || (s32)dx*dx+(s32)dy*dy>288*288) {
                if(!sc64_defiler_clear_approach(a,q))continue;
                dx=(a->target_x-q->x)*2;dy=(a->target_y-q->y)*2;
            } else
#endif
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
                sc64_shared_payloads[slot].swarm=(Sc64DefilerProjectile){handle,a->target_x,a->target_y,255};
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
        Sc64DefilerProjectile *p=sc64_defiler_projectile_at(i);
        if(!p)continue;
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
