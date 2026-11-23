/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include "../starcraft/sc64_stage_save_auth.h"
typedef uint8_t u8; typedef int32_t s32;
typedef struct { unsigned version; } RaceSave;
typedef struct { int alive,type,raw,slot,owner; } Unit;
typedef struct { int alive; } Building;
typedef struct { int pending_damage_event,state; int x_q8_source,y_q8_source,turn_rate_raw,order_signal_bit0,hit_flags_low2,movement_flags_low4,raw_source_owner,destination_x_source,destination_y_source,script_phase,remaining_time,next_speed,heading,next_velocity_direction,owner_ref,target_ref; } Sc64ProjectileRow;
#define SC64_P7_PROJECTILES_ENABLED 1
#define SC64_CUSTOM_CARRIER_PROBE 1
#define MAX_UNITS 10
#define U_P7_INTERCEPTOR 42
#define RACE_PROTOSS_RAW(q) ((q)->raw)
#define RACE_PROTOSS_SLOT(q) ((q)->slot)
#define SC64_TRANSPORT_LOADED(q) 0
static int race_protoss_services_active(void){return 1;}
static int race_protoss_source_owner(int owner){return owner;}
static int race_save_decode_building(const RaceSave *c,unsigned i,Building *b){(void)c;(void)i;b->alive=1;return 1;}
#ifdef TEST_EGG_CONTEXT
#define SC64_SHARED_EGG_CUSTOM_CANDIDATE 1
#define SC64_GCTX_DECL , const Sc64StageSaveAuth *guard_auth
#define SC64_GCTX_ARG , guard_auth
static int decode(const RaceSave *c,unsigned i,Unit *u,const Sc64StageSaveAuth *a){(void)i;if(!sc64_stage_save_auth_matches(a,c,c->version,c->version==12?SC64_STAGE_SAVE_EGG:0))return 0;*u=(Unit){1,42,73,1,1};return 1;}
#define SC64_GUARD_DECODE(c,s,w,h,o) decode(c,s,o,guard_auth)
#else
#define SC64_GCTX_DECL
#define SC64_GCTX_ARG
static int decode(const RaceSave *c,unsigned i,Unit *u){(void)c;(void)i;*u=(Unit){1,42,73,1,1};return 1;}
#define SC64_GUARD_DECODE(c,s,w,h,o) decode(c,s,o)
#endif
#include "../starcraft/sc64_p7_projectile_save_adapter.h"
int main(void){
 RaceSave candidate={12},other={12}; Sc64ProjectileRow row={0}; row.turn_rate_raw=127;row.movement_flags_low4=8;row.script_phase=1;row.remaining_time=255;row.raw_source_owner=1;row.owner_ref=0;row.target_ref=511;
#ifdef TEST_EGG_CONTEXT
 Sc64StageSaveAuth auth; const Sc64StageTagPair allowed[]={{2,12,SC64_STAGE_SAVE_EGG},{1,11,0}};
 assert(sc64_stage_save_authenticate(&auth,&candidate,2,12,allowed,2));
 Sc64ShowcaseProjectileCandidate view={&candidate,64,64,&auth};
 assert(race_showcase_projectile_row(&view,0,&row));
 view.candidate=&other;assert(!race_showcase_projectile_row(&view,0,&row));view.candidate=&candidate;
 auth.inner=11;assert(!race_showcase_projectile_row(&view,0,&row));auth.inner=12;
 auth.features=0;assert(!race_showcase_projectile_row(&view,0,&row));
 assert(!sc64_stage_save_authenticate(&auth,&candidate,2,11,allowed,2));assert(!race_showcase_projectile_row(&view,0,&row));
 candidate.version=11;assert(sc64_stage_save_authenticate(&auth,&candidate,1,11,allowed,2));assert(race_showcase_projectile_row(&view,0,&row));
#else
 (void)other;Sc64ShowcaseProjectileCandidate view={&candidate,64,64};assert(race_showcase_projectile_row(&view,0,&row));
#endif
 return 0;
}
