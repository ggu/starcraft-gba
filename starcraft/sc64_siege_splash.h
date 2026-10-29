/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Stock Siege Tank radial splash, enabled with SC64_SIEGE_SPLASH.
 * Source timing remains the engine's immediate-hit weapon model. */
#ifndef SC64_SIEGE_SPLASH_H
#define SC64_SIEGE_SPLASH_H
#if defined(SC64_SIEGE_SPLASH) && !defined(SC64_RACES)
#error "Siege splash requires race combat"
#endif
#ifdef SC64_SIEGE_SPLASH
#include <limits.h>
#include "sc64_siege_source_bounds.h"
enum { SC64_SIEGE_SPLASH_INNER=1, SC64_SIEGE_SPLASH_MIDDLE=2,
       SC64_SIEGE_SPLASH_OUTER=3 };
static unsigned sc64_siege_xy_length(int dx,int dy) {
 unsigned horizontal=dx<0?0u-(unsigned)dx:(unsigned)dx;
 unsigned vertical=dy<0?0u-(unsigned)dy:(unsigned)dy;
 unsigned major=horizontal>vertical?horizontal:vertical;
 unsigned minor=horizontal>vertical?vertical:horizontal;
 if(minor<=major/4u)return major;
 /* Preserve each term's integer rounding in the stock diagonal estimate. */
 unsigned reduction=(major>>4)+(major>>6);
 unsigned diagonal=((minor*3u)>>3)+((minor*3u)>>8);
 return major-reduction+diagonal;
}
/* Raw BWAPI bounds are inclusive edge offsets; GBA centers are doubled into
 * source pixels. Distance matches pinned OpenBW rect_difference/xy_length. */
static int sc64_siege_distance(int raw,int x,int y,int px,int py) {
 int l,t,r,b,cx,cy,dx,dy;
 if(raw<0||raw>=228||x<0||y<0||x>(INT_MAX-256)/2||y>(INT_MAX-256)/2)return -1;
 l=x*2-siege_bounds[raw][2]; t=y*2-siege_bounds[raw][3];
 r=x*2+siege_bounds[raw][4]; b=y*2+siege_bounds[raw][5];
 cx=px*2; cy=py*2;
 dx=cx<l?l-cx:cx>r?cx-r:0; dy=cy<t?t-cy:cy>b?cy-b:0;
 return (int)sc64_siege_xy_length(dx,dy);
}
static unsigned sc64_siege_packet_q8(int base,int distance,int *tier) {
 *tier=0; if(distance<0||distance>40)return 0;
 if(distance<=10){*tier=SC64_SIEGE_SPLASH_INNER;return (unsigned)base*256u;}
 if(distance<=25){*tier=SC64_SIEGE_SPLASH_MIDDLE;return (unsigned)base*128u;}
 *tier=SC64_SIEGE_SPLASH_OUTER;return (unsigned)base*64u;
}
static int sc64_siege_burrowed(int slot) {
 int burrowed=0;
 if(slot<0||slot>=MAX_UNITS||!u[slot].alive)return 0;
 if(u[slot].flags&2u)return 1; /* Source instance Burrowed property. */
#ifdef SC64_P6_COMBAT_ENABLED
 burrowed|=race_p6_combat_unit_burrowed(slot);
#endif
#ifdef SC64_T6_RUNTIME
 burrowed|=race_t6_unit_burrowed(slot);
#endif
 return burrowed;
}
static int sc64_siege_target_alive(int target) {
 if(target<0||target>=MAX_UNITS+MAX_BUILDINGS)return 0;
 if(target<MAX_UNITS)return u[target].alive&&u[target].hp>0;
 return b[target-MAX_UNITS].alive&&b[target-MAX_UNITS].hp>0;
}
static int sc64_siege_hidden_or_immune(int target) {
 if(target<MAX_UNITS) {
  if(u[target].flags&16u
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
     ||invincible_units[target]
#endif
    )return 1;
#if defined(SC64_ZP_CAMPAIGN_ACTORS) || defined(SC64_T4_RUNTIME)
  if(sc64_game_unit_has_flag(u[target].type,SC64_GAME_ACTOR_HIDDEN)||
     sc64_game_unit_has_flag(u[target].type,SC64_GAME_ACTOR_INVINCIBLE))return 1;
#endif
 } else {
  int bi=target-MAX_UNITS;
  if(b[bi].flags&16u
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
     ||invincible_buildings[bi]
#endif
    )return 1;
#if defined(SC64_ZP_CAMPAIGN_ACTORS) || defined(SC64_T4_RUNTIME)
  if(sc64_game_building_has_flag(b[bi].type,SC64_GAME_ACTOR_INVINCIBLE))return 1;
#endif
 }
 return 0;
}
/* Arclite Shock Cannon is Explosive. Split its packet before armor, then apply
 * the Explosive size ratio in Q8 before integer HP quantization. */
static int sc64_siege_resolve(int target,unsigned packet_q8,int *shield_loss) {
 unsigned damage=packet_q8,armor_q8; int shields=0,armor,size,shield_contact=0;
 *shield_loss=0;
#ifdef SC64_Z6_RUNTIME
 if(target<MAX_UNITS)damage=(unsigned)race_z6_hallucination_incoming_damage(target,(int)damage);
#endif
 if(damage<128u)damage=128u;
#ifdef SC64_RACES
 if(target<MAX_UNITS) {
  Unit *v=&u[target];
  const Sc64StockUnitProfile *sp;
  shields=v->shield;
  armor=v->armor+(v->owner==SIDE?armor_level:0);
  sp=sc64_stock_unit_profile(v->raw_type>=0?v->raw_type:sc64_race_raw_unit(v->type));
  size=sp?sp->size:SC64_STOCK_SIZE_LARGE;
 } else {
  Building *v=&b[target-MAX_UNITS]; shields=v->shield;
  armor=sc64_race_building_armor(v->type);
  const Sc64StockUnitProfile *sp=sc64_stock_unit_profile(v->raw_type>=0?v->raw_type:sc64_race_raw_building(v->type));
  size=sp?sp->size:SC64_STOCK_SIZE_LARGE;
 }
#else
 armor=0;size=SC64_STOCK_SIZE_LARGE;
#endif

 if(shields>0) {
  unsigned absorbed=damage<(unsigned)shields*256u?damage:(unsigned)shields*256u;
  shield_contact=absorbed!=0;
  *shield_loss=(int)(absorbed/256u); damage-=absorbed;
 }
 armor_q8=(unsigned)(armor<0?0:armor)*256u;
 if(damage>armor_q8)damage-=armor_q8;else damage=0;
 /* Weapon 27/28 are Explosive: size scaling follows armor. Keep it Q8. */
 if(size==SC64_STOCK_SIZE_SMALL)damage/=2u;
 else if(size==SC64_STOCK_SIZE_MEDIUM)damage=damage*3u/4u;
 if(!shield_contact && damage<128u)damage=128u;
 return (int)(damage/256u);
}
static void race_siege_splash_attack(Unit *source,int primary,int base_damage) {
 int impact_x=target_x(primary),impact_y=target_y(primary),source_slot=(int)(source-u);
 /* Bounds exist for every public raw type. If an otherwise eligible actor
  * with an unknown identity is close enough that its box could touch the
  * blast, apply the primary packet only; never cancel the shot or guess its
  * collateral. Unrelated air/loaded/hidden/immune actors cannot block it. */
 int unknown_candidate=0;
 for(int target=0;target<MAX_UNITS+MAX_BUILDINGS;target++) {
  int raw,x,y;
  if(target==source_slot||!sc64_siege_target_alive(target)||sc64_siege_hidden_or_immune(target))continue;
  if(target<MAX_UNITS) {
   if(SC64_TRANSPORT_LOADED(&u[target])||sc64_race_unit_flying(u[target].type))continue;
   raw=u[target].raw_type>=0?u[target].raw_type:sc64_race_raw_unit(u[target].type);x=u[target].x;y=u[target].y;
  } else {raw=b[target-MAX_UNITS].raw_type>=0?b[target-MAX_UNITS].raw_type:sc64_race_raw_building(b[target-MAX_UNITS].type);x=b[target-MAX_UNITS].x;y=b[target-MAX_UNITS].y;}
  if(raw<0||raw>=228) {
   int dx=x-impact_x,dy=y-impact_y;
   /* Largest pinned half-extent is 128 source pixels; outer radius is 40. */
   if(dx>=-84&&dx<=84&&dy>=-84&&dy<=84)unknown_candidate=1;
  }
 }
 if(unknown_candidate) {
  int flags=SC64_HIT_PRECOMPUTED_HP|SC64_HIT_SKIP_HALLUCINATION_SCALE;
  int shield_loss=0,hp_damage;
  if(primary<MAX_UNITS&&sc64_siege_burrowed(primary))flags|=SC64_HIT_ALLOW_SPLASH_BURROW;
  hp_damage=sc64_siege_resolve(primary,(unsigned)base_damage*256u,&shield_loss);
#ifdef SC64_RACES
  if(primary<MAX_UNITS)u[primary].shield-=shield_loss;else b[primary-MAX_UNITS].shield-=shield_loss;
#endif
  hit_target_impl_ex(primary,hp_damage,source->type,source->owner,-1,flags);
  return;
 }
 for(int target=0;target<MAX_UNITS+MAX_BUILDINGS;target++) {
  int raw,distance,tier=0,shield_loss=0,hp_damage,flags=SC64_HIT_PRECOMPUTED_HP|SC64_HIT_SKIP_HALLUCINATION_SCALE;
  unsigned packet;
  if(target==source_slot||!sc64_siege_target_alive(target)||sc64_siege_hidden_or_immune(target))continue;
  if(target<MAX_UNITS) {
   if(SC64_TRANSPORT_LOADED(&u[target])||sc64_race_unit_flying(u[target].type))continue;
   raw=u[target].raw_type>=0?u[target].raw_type:sc64_race_raw_unit(u[target].type);
   distance=sc64_siege_distance(raw,u[target].x,u[target].y,impact_x,impact_y);
  } else {
   raw=b[target-MAX_UNITS].raw_type>=0?b[target-MAX_UNITS].raw_type:sc64_race_raw_building(b[target-MAX_UNITS].type);
   distance=sc64_siege_distance(raw,b[target-MAX_UNITS].x,b[target-MAX_UNITS].y,impact_x,impact_y);
  }
  if(distance<0)continue; /* out-of-range unknown actor was screened above */
  packet=sc64_siege_packet_q8(base_damage,distance,&tier); if(!packet)continue;
  if(target<MAX_UNITS&&sc64_siege_burrowed(target)) {
   if(tier!=SC64_SIEGE_SPLASH_INNER)continue;
   flags|=SC64_HIT_ALLOW_SPLASH_BURROW;
  }
  hp_damage=sc64_siege_resolve(target,packet,&shield_loss);
#ifdef SC64_RACES
  if(target<MAX_UNITS)u[target].shield-=shield_loss;
  else b[target-MAX_UNITS].shield-=shield_loss;
#endif
  hit_target_impl_ex(target,hp_damage,source->type,source->owner,-1,flags);
 }
}
#endif
#endif
