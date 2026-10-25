/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sc64_interceptor_motion.h"

typedef struct { uint16_t words[SC64_ARENA_WORDS]; } ArenaStore;
static void arena_bind(Sc64Arena *a, ArenaStore *s) { a->words=s->words; a->word_count=SC64_ARENA_WORDS; }
static uint32_t rng_state=0x6d2b79f5u;
static uint32_t rnd(void) { rng_state^=rng_state<<13; rng_state^=rng_state>>17; rng_state^=rng_state<<5; return rng_state; }
static Sc64ProjectileRow row_default(void) {
    Sc64ProjectileRow r; memset(&r,0,sizeof r); r.target_ref=511; r.owner_ref=255; r.state=3; return r;
}
static void acquire_n(Sc64Arena *a, unsigned n, uint8_t *ids) {
    for (unsigned i=0;i<n;i++) { uint8_t native; assert(sc64_pool_acquire(a,&ids[i],&native)); assert(native==sc64_native_index(ids[i])); }
}
static void init_row(Sc64Arena *a, uint8_t id, Sc64ProjectileRow r) { assert(sc64_pool_set_row(a,id,&r)); assert(sc64_pool_validate(a)); }
static int same_row(const Sc64ProjectileRow *a,const Sc64ProjectileRow *b) { return memcmp(a,b,sizeof *a)==0; }
static void checkpoint_tick(Sc64Arena *a, Sc64Arena *branch, uint8_t id,
                           Sc64InterceptorMotionContext c) {
    uint8_t packet[SC64_CODEC_BYTES]; Sc64ProjectileRow before_a,before_b,after_a,after_b;
    assert(sc64_pool_validate(a));
    assert(sc64_codec_encode(a,packet,sizeof packet));
    assert(sc64_codec_decode(branch,packet,sizeof packet));
    assert(sc64_pool_get_row(a,id,&before_a)); assert(sc64_pool_get_row(branch,id,&before_b));
    assert(same_row(&before_a,&before_b));
    after_a=before_a; after_b=before_b;
    assert(sc64_interceptor_motion_tick(&after_a,&c));
    assert(sc64_interceptor_motion_tick(&after_b,&c));
    if (after_a.state==3 && before_a.state!=3) {
        assert(sc64_pool_retire(a,id)); assert(sc64_pool_retire(branch,id));
    }
    assert(sc64_pool_set_row(a,id,&after_a)); assert(sc64_pool_set_row(branch,id,&after_b));
    assert(sc64_pool_get_row(a,id,&after_a)); assert(sc64_pool_get_row(branch,id,&after_b));
    assert(same_row(&after_a,&after_b));
    assert(sc64_pool_validate(a)); assert(sc64_pool_validate(branch));
}
static void targeted(void) {
    ArenaStore sa,sb; Sc64Arena a,b; uint8_t ids[10]; arena_bind(&a,&sa); arena_bind(&b,&sb);
    assert(sc64_pool_init(&a)); assert(sc64_pool_init(&b)); acquire_n(&a,1,ids);
    Sc64ProjectileRow r=row_default(); r.state=0; r.script_phase=1; r.x_q8_source=20*256; r.y_q8_source=20*256;
    r.destination_x_source=25; r.destination_y_source=20; r.next_speed=0; r.turn_rate_raw=127;
    r.heading=64; r.next_velocity_direction=64; r.target_ref=1; r.owner_ref=2; r.remaining_time=20;
    init_row(&a,ids[0],r); assert(sc64_pool_validate(&a));
    Sc64InterceptorMotionContext c={128,128,1,1}; uint8_t phase=0; unsigned ticks=0;
    while (ticks<32) {
       if (ticks==2) c.target_alive=0; /* reference clears before impact */
       if (ticks==4) { c.target_alive=1; c.owner_alive=0; }
       if (sc64_interceptor_source_frame_due(&phase)) { checkpoint_tick(&a,&b,ids[0],c); ticks++; }
       else { uint8_t p[SC64_CODEC_BYTES]; assert(sc64_codec_encode(&a,p,sizeof p)); assert(sc64_codec_decode(&b,p,sizeof p)); }
       Sc64ProjectileRow now; assert(sc64_pool_get_row(&a,ids[0],&now));
       if (now.state==3) break;
    }
    assert(ticks>0 && ticks<32);
}
static void lifetime_expiry(void) {
    ArenaStore sa,sb; Sc64Arena a,b; uint8_t id; arena_bind(&a,&sa); arena_bind(&b,&sb);
    assert(sc64_pool_init(&a)); assert(sc64_pool_init(&b)); acquire_n(&a,1,&id);
    Sc64ProjectileRow r=row_default(); r.state=1; r.script_phase=2; r.movement_flags_low4=3;
    r.x_q8_source=5*256; r.y_q8_source=5*256; r.destination_x_source=100; r.destination_y_source=5;
    r.next_speed=32; r.turn_rate_raw=127; r.heading=64; r.next_velocity_direction=64;
    r.target_ref=1; r.owner_ref=2; r.remaining_time=1; init_row(&a,id,r);
    Sc64InterceptorMotionContext c={128,128,1,1};
    for (unsigned i=0;i<5;i++) { checkpoint_tick(&a,&b,id,c); assert(sc64_pool_get_row(&a,id,&r)); if(r.state==3) break; }
    assert(r.state==3);
}
static Sc64ProjectileRow one_projectile(int state,int phase,int x,int dest,int timer) {
    Sc64ProjectileRow r=row_default(); r.state=(uint8_t)state; r.script_phase=(uint8_t)phase;
    r.x_q8_source=x*256; r.y_q8_source=10*256; r.destination_x_source=(int16_t)dest; r.destination_y_source=10;
    r.turn_rate_raw=127; r.heading=64; r.next_velocity_direction=64; r.next_speed=0;
    r.target_ref=1; r.owner_ref=2; r.remaining_time=(int16_t)timer; r.hit_flags_low2=0;
    return r;
}
static void lifecycle_case(int dead_target,int dead_owner,int expiry) {
    ArenaStore sa,sb; Sc64Arena a,b; uint8_t id; arena_bind(&a,&sa); arena_bind(&b,&sb);
    assert(sc64_pool_init(&a)); assert(sc64_pool_init(&b)); acquire_n(&a,1,&id);
    Sc64ProjectileRow r=one_projectile(expiry?1:0,expiry?2:1,expiry?5:10,expiry?100:10,expiry?1:20);
    if(expiry) { r.movement_flags_low4=3; r.next_speed=32; }
    init_row(&a,id,r); Sc64InterceptorMotionContext c={128,128,(uint8_t)!dead_target,(uint8_t)!dead_owner};
    checkpoint_tick(&a,&b,id,c); assert(sc64_pool_get_row(&a,id,&r));
    if(expiry) {
        assert(r.state==1 && r.pending_damage_event==0);
        checkpoint_tick(&a,&b,id,c); assert(sc64_pool_get_row(&a,id,&r));
        assert(r.state==2 && r.pending_damage_event==1); /* timer 0 -> -1 */
    } else {
        assert(r.state==2);
        assert(r.pending_damage_event==(uint8_t)!dead_target);
        if(dead_target) assert(r.target_ref==511);
        if(dead_owner) assert(r.owner_ref==255 && r.pending_damage_event==1);
    }
    checkpoint_tick(&a,&b,id,c); assert(sc64_pool_get_row(&a,id,&r));
    assert(r.state==3 && r.pending_damage_event==0);
}
static void lifecycle_matrix(void) {
    lifecycle_case(0,0,0); /* init / immediate endpoint / pulse / free */
    lifecycle_case(1,0,0); /* target removed / no damage pulse */
    lifecycle_case(0,1,0); /* owner removed / target impact remains */
    lifecycle_case(0,0,1); /* signed timer expiry */
}
static void hundred_active_reuse(void) {
    ArenaStore s; Sc64Arena a; uint8_t ids[100]; arena_bind(&a,&s); assert(sc64_pool_init(&a));
    acquire_n(&a,100,ids); assert(sc64_pool_validate(&a));
    for(unsigned i=0;i<100;i++) { Sc64ProjectileRow r=row_default(); r.state=1; r.remaining_time=10; r.target_ref=511; r.owner_ref=255; init_row(&a,ids[i],r); }
    assert(sc64_pool_validate(&a));
    ArenaStore branch_store; Sc64Arena branch; arena_bind(&branch,&branch_store); assert(sc64_pool_init(&branch));
    Sc64InterceptorMotionContext context={64,64,1,1};
    for(unsigned i=0;i<100;i++) { Sc64ProjectileRow row; assert(sc64_pool_get_row(&a,ids[i],&row)); row.x_q8_source=row.y_q8_source=20*256; row.destination_x_source=row.destination_y_source=20; row.target_ref=1; row.owner_ref=2; init_row(&a,ids[i],row); checkpoint_tick(&a,&branch,ids[i],context); }
    for(unsigned i=0;i<100;i++) { Sc64ProjectileRow row; assert(sc64_pool_get_row(&a,ids[i],&row)); assert(row.state==2 && row.pending_damage_event==1); checkpoint_tick(&a,&branch,ids[i],context); }
    assert(sc64_pool_validate(&a));
    uint8_t reused,native; assert(sc64_pool_acquire(&a,&reused,&native));
    assert(reused==ids[0] && native==sc64_native_index(ids[0])); assert(sc64_pool_validate(&a));
}
static void randomized(void) {
    for(unsigned trial=0;trial<128;trial++) {
       ArenaStore sa,sb; Sc64Arena a,b; uint8_t id; arena_bind(&a,&sa); arena_bind(&b,&sb);
       assert(sc64_pool_init(&a)); assert(sc64_pool_init(&b)); acquire_n(&a,1,&id);
       Sc64ProjectileRow r=row_default(); r.state=(trial%3==0)?0:1; r.script_phase=(r.state==0)?1:2;
       r.x_q8_source=(int32_t)(16+(rnd()%96))*256; r.y_q8_source=(int32_t)(16+(rnd()%96))*256;
       r.destination_x_source=(int16_t)(16+(rnd()%96)); r.destination_y_source=(int16_t)(16+(rnd()%96));
       r.next_speed=(uint16_t)(rnd()%20000); r.turn_rate_raw=(uint16_t)(rnd()%512);
       r.heading=(uint8_t)rnd(); r.next_velocity_direction=(uint8_t)rnd(); r.target_ref=3; r.owner_ref=4;
       r.remaining_time=(int16_t)(1+(rnd()%20)); r.movement_flags_low4=(uint8_t)(rnd()%16); r.hit_flags_low2=(uint8_t)(rnd()%4);
       init_row(&a,id,r); Sc64InterceptorMotionContext c={128,128,1,1}; uint8_t phase=(uint8_t)(rnd()%5);
       for(unsigned step=0;step<24;step++) {
          if(step==5 && (trial&1)) c.target_alive=0;
          if(step==7 && (trial&2)) c.owner_alive=0;
          if(sc64_interceptor_source_frame_due(&phase)) checkpoint_tick(&a,&b,id,c);
          assert(sc64_pool_get_row(&a,id,&r)); if(r.state==3) break;
       }
    }
}
static void launch_helpers(void) {
    uint8_t d=0; int32_t ox=0,oy=0; Sc64ProjectileRow r=row_default();
    assert(sc64_interceptor_direction_for_delta(1,0,&d) && d==64);
    assert(sc64_interceptor_direction_for_delta(0,1,&d) && d==128);
    assert(sc64_interceptor_direction_for_delta(-1,0,&d) && d==192);
    assert(!sc64_interceptor_direction_for_delta(0,0,&d));
    assert(!sc64_interceptor_direction_for_delta(16384,1,&d));
    assert(sc64_interceptor_forward_offset(0,&ox,&oy) && ox==0 && oy==-5120);
    assert(sc64_interceptor_forward_offset(64,&ox,&oy) && ox==5120 && oy==0);
    assert(sc64_interceptor_forward_offset(128,&ox,&oy) && ox==0 && oy==5120);
    assert(sc64_interceptor_forward_offset(192,&ox,&oy) && ox==-5120 && oy==0);
    assert(sc64_interceptor_forward_offset(3,&ox,&oy) && oy==-5120);
    assert(sc64_interceptor_projectile_launch(&r,100*256+255,100*256+128,130,100,64,7,2,3));
    assert(r.x_q8_source==120*256 && r.y_q8_source==100*256);
    assert(sc64_interceptor_projectile_launch(&r,-1,-1,30,30,64,7,2,3));
    assert(r.x_q8_source==19*256 && r.y_q8_source==-256);
    assert(sc64_interceptor_projectile_launch(&r,100*256,100*256,130,100,64,7,2,3));
    assert(r.x_q8_source==120*256 && r.y_q8_source==100*256);
    assert(r.destination_x_source==130 && r.destination_y_source==100);
    assert(r.state==0 && r.script_phase==1 && r.remaining_time==255);
    assert(r.heading==64 && r.next_velocity_direction==64 && r.next_speed==0);
    assert(r.target_ref==7 && r.owner_ref==2 && r.raw_source_owner==3);
    r.x_q8_source=77;
    assert(!sc64_interceptor_projectile_launch(&r,2097151,0,1,1,64,7,2,3));
    assert(r.x_q8_source==77); /* failure is atomic */
    assert(!sc64_interceptor_projectile_launch(&r,0,0,1,1,0,400,2,3));
    assert(!sc64_interceptor_projectile_launch(&r,0,0,1,1,0,7,240,3));
}
static void invalid_inputs(void) {
    Sc64ProjectileRow r=row_default(); Sc64InterceptorMotionContext c={64,64,1,1};
    assert(sc64_interceptor_motion_tick(&r,&c));
    r=row_default(); r.state=1; r.order_signal_bit0=2; assert(!sc64_interceptor_motion_tick(&r,&c));
    r=row_default(); r.state=1; r.x_q8_source=INT32_MIN; assert(!sc64_interceptor_motion_tick(&r,&c));
    r=row_default(); r.state=1; c.map_width_px=0; assert(!sc64_interceptor_motion_tick(&r,&c));
    c.map_width_px=64; c.target_alive=2; r=row_default(); r.state=1; assert(!sc64_interceptor_motion_tick(&r,&c));
    c.target_alive=1; r=one_projectile(1,2,10,10,10); r.target_ref=511;
    assert(sc64_interceptor_motion_tick(&r,&c));
    assert(r.state==2 && r.pending_damage_event==0);
    c.map_width_px=8193; r=one_projectile(1,2,10,20,10); assert(!sc64_interceptor_motion_tick(&r,&c));
    c.map_width_px=8192; r=one_projectile(1,2,10,20,-1); assert(!sc64_interceptor_motion_tick(&r,&c));
    r=one_projectile(1,2,10,20,10); r.turn_rate_raw=511; r.movement_flags_low4=1;
    Sc64ProjectileRow before=r; assert(!sc64_interceptor_motion_tick(&r,&c));
    assert(memcmp(&r,&before,sizeof r)==0); /* post-tick turn rate 512 rejected atomically */
}
int main(void) { uint8_t clock=0; unsigned due=0; for(unsigned i=0;i<30;++i) due+=(unsigned)sc64_interceptor_source_frame_due(&clock); assert(due==24 && clock==0); targeted(); lifetime_expiry(); lifecycle_matrix(); hundred_active_reuse(); randomized(); launch_helpers(); invalid_inputs(); puts("PASS lifecycle matrix, 100 active/reuse, 128 seeded cases, launch bounds and row/context rejection"); return 0; }
