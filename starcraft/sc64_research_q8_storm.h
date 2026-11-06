/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESEARCH_Q8_STORM_H
#define SC64_RESEARCH_Q8_STORM_H
#include "sc64_psionic_storm.h"
/* Keep the mission adapters' existing eligibility, overlap and death hooks.
 * Only their integer/Q8 state boundary changes in this fenced experiment. */
static int sc64_pr_q8_storm_read(int target,int *hp,int *shield)
{
    unsigned h,s;
    if(!hp||!shield||!sc64_pr_q8_actor_hp(target,&h)||
       !sc64_pr_q8_actor_shield(target,&s)||h>INT_MAX||s>INT_MAX)return 0;
    *hp=(int)h;*shield=(int)s;return 1;
}
static int sc64_pr_q8_storm_commit(int target,int hp,int shield)
{
    if(hp<0||shield<0)return 0;
    /* Snapshot validation and the bounded pure Storm subtraction establish
     * both commit preconditions before either live field is published. */
    return sc64_pr_q8_commit_hp(target,(unsigned)hp)&&
           sc64_pr_q8_commit_shield(target,(unsigned)shield);
}
/* Pinned weapon_deal_damage tests shields >= one whole point and doubles a
 * hallucinated target's packet before shield absorption. Two separate hits
 * would differ when the first leaves less than one shield point. */
static int sc64_pr_q8_storm_hit(u8 klass,int invincible,u8 *hit,
                              int *hp,int *shield,int hallucinated)
{
    Sc64ShieldDamageInput in={0};Sc64ShieldDamageResult out;
    if(!hit||!hp||!shield||*hp<=0||*shield<0||invincible||*hit||
       !(klass&(SC64_STORM_TARGET_AIR|SC64_STORM_TARGET_GROUND))||
       !(klass&SC64_STORM_TARGET_NONBUILDING))return 0;
    in.packet_q8=SC64_STORM_DAMAGE_PER_PULSE*256u;
    in.hp_q8=(unsigned)*hp;in.shield_q8=(unsigned)*shield;
    in.has_shields=*shield>0;in.hallucinated=(unsigned)hallucinated;
    in.damage_type=SC64_DAMAGE_IGNORE_ARMOR;in.divisor=1;
    if(!sc64_shield_damage_q8(&in,&out))return 0;
    *hp=(int)out.hp_after_q8;*shield=(int)out.shield_after_q8;*hit=1;return 1;
}
#endif
