/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compare reused motion with original separately retained fdba20f implementations. */
#define SC64_WEAPON_SOURCE_BANK_HEADER "../tests/fixtures/weapon_source_mock.h"
#include "../starcraft/sc64_weapon_source_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>

typedef struct {uint32_t flight,duration,has;int valid,x,y;} ProbeResult;
/* Original fdba20f implementations, retained as a separate reference. */
static inline uint32_t
reference_flight_ticks(const Sc64WeaponSourceAttackProfile *p,
                                      int x0, int y0, int x1, int y1)
{
    uint64_t remaining = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    int64_t top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint32_t limit = p->projectile_lifetime ? p->projectile_lifetime : 65535u;
    int64_t speed = 0;
    if (!remaining || sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type))
        return 0;
    if (top <= 0) return 0;
    if (accel <= 0) speed = top;
    /* Step tick by tick only while accelerating toward top speed. */
    uint32_t tick = 1;
    for (; tick <= limit && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        if ((uint64_t)speed >= remaining) return tick;
        remaining -= (uint64_t)speed;
    }
    if (tick > limit) return limit;
    /* At top speed the first covering tick is ceil(remaining / top) away. */
    uint64_t arrival = (uint64_t)tick - 1u + (remaining + (uint64_t)top - 1u) / (uint64_t)top;
    return arrival > limit ? limit : (uint32_t)arrival;
}

static inline int
reference_position(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1,
                                  uint32_t source_age, int *x, int *y)
{
    if (!p || !x || !y) return 0;
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_SOURCE ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_SELF_DESTRUCT) {
        *x = x0;
        *y = y0;
        return 1;
    }
    if (p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_PERSIST_TARGET_POS ||
        p->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_POS) {
        *x = x1;
        *y = y1;
        return 1;
    }
    int64_t dx = (int64_t)x1 - x0, dy = (int64_t)y1 - y0;
    uint64_t distance = sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1);
    uint32_t flight = reference_flight_ticks(p, x0, y0, x1, y1);
    if (!flight) {
        if (!sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1)) {
            *x = x1;
            *y = y1;
            return 1;
        }
        return 0;
    }
    if (source_age >= flight || !distance) {
        *x = x1;
        *y = y1;
        return 1;
    }
    int64_t speed = 0, top = p->speed_raw;
    int64_t accel = p->accel_raw;
    uint64_t travelled = 0;
    /* Step tick by tick only while accelerating; top > 0 since flight > 0. */
    uint32_t tick = 0;
    for (; tick < source_age && accel > 0 && speed < top; ++tick) {
        speed += accel;
        if (speed > top) speed = top;
        if (speed <= 0) speed = top;
        travelled += (uint64_t)speed;
        if (travelled >= distance) { travelled = distance; break; }
    }
    if (travelled < distance && tick < source_age) {
        /* The remaining ticks each cover top, capped at the distance. */
        uint64_t rest = source_age - tick, room = distance - travelled;
        travelled = rest >= (room + (uint64_t)top - 1u) / (uint64_t)top ?
                    distance : travelled + rest * (uint64_t)top;
    }
    *x = (int)((int64_t)x0 + dx * (int64_t)travelled / (int64_t)distance);
    *y = (int)((int64_t)y0 + dy * (int64_t)travelled / (int64_t)distance);
    return 1;
}

static inline uint32_t
reference_duration(const Sc64WeaponSourceAttackProfile *p,
                                  int x0, int y0, int x1, int y1)
{
    uint32_t flight, startup_ticks, death_ticks = 0;
    uint32_t source_ticks, actor_ticks = 0, release_ticks;
    const Sc64WeaponSourceAnimation *anim;
    if (!p || !p->projectile_image ||
        !sc64_weapon_source_image_lookup(p->projectile_image)) return 0;
    flight = sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) ? 0 :
             reference_flight_ticks(p, x0, y0, x1, y1);
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) &&
        sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1) && !flight)
        return 0;
    startup_ticks = sc64_weapon_source_trace_startup_ticks(p);
    anim = sc64_weapon_source_animation_lookup(p->projectile_image, 1u);
    if (anim) death_ticks = sc64_weapon_source_animation_visual_ticks(anim);
    /* The order signal enters GndAttkInit. For moving bullets the death script
     * starts at distance-derived arrival/expiry; stationary bullets transition
     * to Death immediately after the authored startup event. */
    uint32_t active_ticks = flight;
    release_ticks = sc64_weapon_source_trace_release_ticks(p);
    source_ticks = release_ticks + startup_ticks + active_ticks + death_ticks;
    if (!source_ticks) source_ticks = p->projectile_frame_count ? 1u : 0u;
    anim = sc64_weapon_source_animation_lookup(p->attack_image, p->attack_anim);
    if (anim && anim->loop_pc == UINT32_MAX) {
        uint32_t visual = sc64_weapon_source_animation_visual_ticks(anim);
        actor_ticks = visual;
    }
    if (actor_ticks > source_ticks) source_ticks = actor_ticks;
    return sc64_weapon_source_vfx_gba_duration(source_ticks);
}

static inline int
reference_has_duration(const Sc64WeaponSourceAttackProfile *p,
                                      int x0, int y0, int x1, int y1)
{
    if (!p || !p->projectile_image ||
        !sc64_weapon_source_image_lookup(p->projectile_image)) return 0;
    if (!sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type) &&
        sc64_weapon_source_trace_distance_raw(x0, y0, x1, y1) &&
        !reference_flight_ticks(p, x0, y0, x1, y1)) return 0;
    if (p->projectile_frame_count) return 1;
    return reference_duration(p, x0, y0, x1, y1) != 0;
}

static ProbeResult original_motion_probe(Sc64WeaponSourceAttackProfile p,int x0,int y0,int x1,int y1,uint32_t age,int null_profile,int null_x,int null_y)
{
 test_source_profile=p;test_source_image.image_id=531;
 test_source_init=(Sc64WeaponSourceAnimation){.event_count=1,.duration_ticks=9,.animation_id=0};
 test_source_death=(Sc64WeaponSourceAnimation){.duration_ticks=3,.animation_id=1};
 sc64_weapon_source_events[0]=(Sc64WeaponSourceEvent){.image_id=531,.source_tick=2,.opcode=36};
 const Sc64WeaponSourceAttackProfile *q=null_profile?0:&test_source_profile;ProbeResult r={0,0,0,0,123456,-654321};
 if(q)r.flight=reference_flight_ticks(q,x0,y0,x1,y1);
 r.duration=reference_duration(q,x0,y0,x1,y1);
 r.has=(uint32_t)reference_has_duration(q,x0,y0,x1,y1);
 r.valid=reference_position(q,x0,y0,x1,y1,age,null_x?0:&r.x,null_y?0:&r.y);return r;
}

static ProbeResult candidate(Sc64WeaponSourceAttackProfile p,int x0,int y0,int x1,int y1,uint32_t age,int null_profile,int null_x,int null_y)
{
    test_source_profile=p;
    test_source_image.image_id=531;
    test_source_init=(Sc64WeaponSourceAnimation){.event_count=1,.duration_ticks=9,.animation_id=0};
    test_source_death=(Sc64WeaponSourceAnimation){.duration_ticks=3,.animation_id=1};
    sc64_weapon_source_events[0]=(Sc64WeaponSourceEvent){.image_id=531,.source_tick=2,.opcode=36};
    const Sc64WeaponSourceAttackProfile *q=null_profile?0:&test_source_profile;
    Sc64WeaponSourceMotion prepared=sc64_weapon_source_motion_prepare(q,x0,y0,x1,y1);
    ProbeResult r={0,0,0,0,123456,-654321};
    if(q) r.flight=sc64_weapon_source_trace_flight_ticks(q,x0,y0,x1,y1);
    r.duration=sc64_weapon_source_trace_duration_motion(q,x0,y0,x1,y1,&prepared);
    r.has=(uint32_t)sc64_weapon_source_trace_has_duration_motion(q,x0,y0,x1,y1,&prepared);
    r.valid=sc64_weapon_source_trace_position_motion(q,x0,y0,x1,y1,age,null_x?0:&r.x,null_y?0:&r.y,&prepared);
    /* Pre-existing public wrappers must independently prepare fresh values. */
    int a=123456,b=-654321;
    assert(sc64_weapon_source_trace_duration(q,x0,y0,x1,y1)==r.duration);
    assert((uint32_t)sc64_weapon_source_trace_has_duration(q,x0,y0,x1,y1)==r.has);
    assert(sc64_weapon_source_trace_position(q,x0,y0,x1,y1,age,null_x?0:&a,null_y?0:&b)==r.valid);
    assert(a==r.x && b==r.y);
    return r;
}
static uint32_t state=0x91a43210u;
static uint32_t rnd(void){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static unsigned cases_checked;
static void compare(Sc64WeaponSourceAttackProfile p,int x0,int y0,int x1,int y1,uint32_t age,int np,int nx,int ny)
{
    ++cases_checked;
    ProbeResult a=original_motion_probe(p,x0,y0,x1,y1,age,np,nx,ny),b=candidate(p,x0,y0,x1,y1,age,np,nx,ny);
    if(a.flight!=b.flight||a.duration!=b.duration||a.has!=b.has||a.valid!=b.valid||a.x!=b.x||a.y!=b.y){
        fprintf(stderr,"mismatch type=%u speed=%d accel=%d age=%u xyz=%d,%d,%d,%d original=%u,%u,%u,%d,%d,%d new=%u,%u,%u,%d,%d,%d\n",p.weapon_bullet_type,p.speed_raw,p.accel_raw,age,x0,y0,x1,y1,a.flight,a.duration,a.has,a.valid,a.x,a.y,b.flight,b.duration,b.has,b.valid,b.x,b.y);assert(0);}
}
int main(void)
{
    Sc64WeaponSourceAttackProfile p={.projectile_image=531,.projectile_frame_count=3,.speed_raw=1024,.accel_raw=256,.projectile_lifetime=255};
    const uint32_t ages[]={0,1,2,7,255,65535,UINT32_MAX};
    for(int type=0;type<=11;type++)for(unsigned i=0;i<sizeof(ages)/sizeof(ages[0]);i++){
        p.weapon_bullet_type=(uint8_t)type;compare(p,15,-20,1024,800,ages[i],0,0,0);
        compare(p,INT_MIN,INT_MAX,INT_MAX,INT_MIN,ages[i]==0?0:UINT32_MAX,0,0,0);
    }
    /* Explicit zero-length, axis-aligned and tiny/zero/maximum-speed cases. */
    const int32_t speeds[]={-1,0,1,INT_MAX};
    const int16_t accelerations[]={-1,0,1,INT16_MAX};
    const int endpoints[][4]={{0,0,0,0},{5,-7,5,256},{-12,32,128,32}};
    const uint32_t edge_ages[]={0,1,UINT32_MAX};
    for(int type=0;type<=11;type++)for(unsigned v=0;v<4;v++)
        for(unsigned a=0;a<4;a++)for(unsigned e=0;e<3;e++)for(unsigned t=0;t<3;t++) {
            p.weapon_bullet_type=(uint8_t)type;p.speed_raw=speeds[v];
            p.accel_raw=accelerations[a];p.projectile_lifetime=7;
            compare(p,endpoints[e][0],endpoints[e][1],endpoints[e][2],endpoints[e][3],edge_ages[t],0,0,0);
        }
    for(unsigned i=0;i<25000;i++) {
        p.weapon_bullet_type=(uint8_t)(rnd()%12);p.speed_raw=(int32_t)(rnd()%40001)-10000;
        p.accel_raw=(int16_t)((int)(rnd()%2001)-1000);p.projectile_lifetime=(uint16_t)(rnd()%128);
        p.projectile_frame_count=(uint16_t)(rnd()%4);p.projectile_image=(rnd()%11)?531:777;
        int x0=(int)(rnd()%4097)-2048,y0=(int)(rnd()%4097)-2048,x1=(int)(rnd()%4097)-2048,y1=(int)(rnd()%4097)-2048;
        uint32_t age=i%10==0?UINT32_MAX:rnd()%300;
        compare(p,x0,y0,x1,y1,age,i%97==0,i%101==0,i%103==0);
    }
    printf("motion reuse matches original wrappers across %u cases\n",cases_checked);return 0;
}
