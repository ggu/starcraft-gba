/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESEARCH_Q8_LIVE_H
#define SC64_RESEARCH_Q8_LIVE_H
/* Included after the shared damage eligibility/lifetime helpers. This HOST
 * experiment cannot save: HP fractions have no qualified journal yet.
 * HP is displayed as ceil(Q8/256), with one debt byte per actor. Shield
 * remainder uses 1/1280 points, preserving the existing 30 Hz time carry.
 * Non-shield actors keep their aliased regeneration/trap/burn fields. */
#include "sc64_shield_damage_q8.h"
static void sc64_pr_q8_regen(unsigned short *shield,unsigned short maximum,unsigned short *phase)
{
    if(!shield||!phase||*shield>maximum||*phase>=1280u)return;
    if(*shield==maximum){*phase=0;return;}
    unsigned n=(unsigned)*phase+28u;
    *shield=(unsigned short)(*shield+n/1280u);
    *phase=*shield<maximum?(unsigned short)(n%1280u):0;
}
static int sc64_pr_q8_hp(int whole,unsigned debt,unsigned *out)
{
    if (!out || whole<0 || (unsigned)whole>UINT32_MAX/256u ||
        debt>255u || (!whole && debt)) return 0;
    *out=(unsigned)whole*256u-debt;return 1;
}
/* Eligible Terran buildings already carry their HP deficit in the low byte
 * of shield_remainder. Its upper bits remain the burn damage-credit owner. */
static int sc64_pr_q8_hp_fields(int target,int **whole,unsigned *debt)
{
    if (!whole || !debt || target<0 || target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    if (target<MAX_UNITS) {
        if (!u[target].alive) return 0;
        *whole=&u[target].hp; *debt=sc64_pr_hp_debt[target];
    } else {
        Building *q=&b[target-MAX_UNITS]; if (!q->alive) return 0;
        *whole=&q->hp;
        *debt=race_building_can_burn(q) ? (unsigned)q->shield_remainder&255u : sc64_pr_hp_debt[target];
    }
    return 1;
}
static int sc64_pr_q8_actor_hp(int target,unsigned *out)
{
    int *whole; unsigned debt;
    return sc64_pr_q8_hp_fields(target,&whole,&debt) && sc64_pr_q8_hp(*whole,debt,out);
}
/* Update only the fraction: hit_target owns the corresponding whole-HP
 * debit and its death callbacks. The caller must publish that debit next. */
static int sc64_pr_q8_store_hp_fraction(int target,unsigned value,int *whole_out)
{
    int *old; unsigned debt,whole=(value+255u)/256u;
    if (!whole_out || value>UINT32_MAX-255u || whole>(unsigned)INT_MAX ||
        !sc64_pr_q8_hp_fields(target,&old,&debt)) return 0;
    debt=whole*256u-value;
    if (target>=MAX_UNITS && race_building_can_burn(&b[target-MAX_UNITS])) {
        Building *q=&b[target-MAX_UNITS];
        q->shield_remainder=(q->shield_remainder & ~255) | (int)debt;
        sc64_pr_hp_debt[target]=0;
    } else sc64_pr_hp_debt[target]=(u8)debt;
    *whole_out=(int)whole; return 1;
}
static int sc64_pr_q8_commit_hp(int target,unsigned value)
{
    int *old,whole; unsigned debt;
    if (!sc64_pr_q8_hp_fields(target,&old,&debt) ||
        !sc64_pr_q8_store_hp_fraction(target,value,&whole)) return 0;
    *old=whole; return 1;
}
static const Sc64StockUnitProfile *sc64_pr_q8_actor_stock(int target)
{
    int raw;
    if (target<0 || target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    if (target<MAX_UNITS) {
        Unit *q=&u[target]; if (!q->alive) return 0;
        raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    } else {
        Building *q=&b[target-MAX_UNITS]; if (!q->alive) return 0;
        raw=q->raw_type>=0?q->raw_type:sc64_race_raw_building(q->type);
    }
    return sc64_stock_unit_profile(raw);
}
static int sc64_pr_q8_actor_shield(int target,unsigned *out)
{
    const Sc64StockUnitProfile *stock=sc64_pr_q8_actor_stock(target);
    int shield,phase;
    if (!stock || !out) return 0;
    if (!stock->shields) { *out=0; return 1; }
    if (target<MAX_UNITS) { shield=u[target].shield; phase=u[target].shield_remainder; }
    else { shield=b[target-MAX_UNITS].shield; phase=b[target-MAX_UNITS].shield_remainder; }
    if (shield<0 || (unsigned)shield>stock->shields || phase<0 || phase>=1280 ||
        ((unsigned)shield==stock->shields && phase)) return 0;
    *out=(unsigned)shield*256u+(unsigned)phase/5u; return 1;
}
static int sc64_pr_q8_commit_shield(int target,unsigned value)
{
    const Sc64StockUnitProfile *stock=sc64_pr_q8_actor_stock(target);
    unsigned previous,carry,whole,phase;
    if (!stock || !sc64_pr_q8_actor_shield(target,&previous)) return 0;
    if (!stock->shields) return value==0;
    if (value>(unsigned)stock->shields*256u) return 0;
    carry=target<MAX_UNITS ? u[target].shield_remainder%5u : (unsigned)b[target-MAX_UNITS].shield_remainder%5u;
    whole=value/256u; phase=(value%256u)*5u+carry;
    if (whole==(unsigned)stock->shields) phase=0;
    if (target<MAX_UNITS) {u[target].shield=(int)whole;u[target].shield_remainder=(u16)phase;}
    else {b[target-MAX_UNITS].shield=(int)whole;b[target-MAX_UNITS].shield_remainder=(int)phase;}
    return 1;
}
static unsigned sc64_pr_q8_shield_level(int owner)
{
    if (owner<0 || owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)owner) ||
        !sc64_pr_levels_valid(&sc64_experimental_research_levels,SC64_PR_CUSTOM_OWNERS)) return 0;
    return sc64_experimental_research_levels.level[owner][SC64_PR_SHIELDS];
}
static int sc64_pr_q8_damage_kind(int stock_class)
{
    switch(stock_class) {
    case SC64_STOCK_DMG_NORMAL:return SC64_DAMAGE_NORMAL;
    case SC64_STOCK_DMG_EXPLOSIVE:return SC64_DAMAGE_EXPLOSIVE;
    case SC64_STOCK_DMG_CONCUSSIVE:return SC64_DAMAGE_CONCUSSIVE;
    case SC64_STOCK_DMG_IGNORE_ARMOR:return SC64_DAMAGE_IGNORE_ARMOR;
    case SC64_STOCK_DMG_NONE:return SC64_DAMAGE_NONE;
    default:return -1;
    }
}
/* Common eligibility checks and incoming hallucination scaling have already
 * run. Publish only validated fractions/shields; the existing core consumes
 * hp_debit and owns all deaths, transport, supply and campaign callbacks. */
static int sc64_pr_q8_resolve(int target,unsigned packet,int stock_class,
                            int source_type,int source_owner,int *hp_debit)
{
    Sc64ShieldDamageInput in={0};Sc64ShieldDamageResult result;
    const Sc64StockUnitProfile *stock;
    int raw,owner,hp,shield,phase,armor,kind;
    unsigned hp_q8,shield_q8=0;
    if (!hp_debit || target<0 || target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    if (target<MAX_UNITS) {
        const Unit *q=&u[target];if(!q->alive)return 0;
        raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
        hp=q->hp;shield=q->shield;phase=q->shield_remainder;owner=q->owner;
        armor=q->armor+sc64_unit_armor_upgrade_level(q);
    } else {
        const Building *q=&b[target-MAX_UNITS];if(!q->alive)return 0;
        raw=q->raw_type>=0?q->raw_type:sc64_race_raw_building(q->type);
        hp=q->hp;shield=q->shield;phase=q->shield_remainder;owner=q->owner;
        armor=sc64_race_building_armor(q->type);
    }
    stock=sc64_stock_unit_profile(raw);
    if(!stock || armor<0 || (unsigned)armor>UINT32_MAX/256u ||
       !sc64_pr_q8_actor_hp(target,&hp_q8))return 0;
    if(stock_class<0) {
        stock_class=SC64_STOCK_DMG_NORMAL;
#ifdef SC64_T4_RUNTIME
        if(source_type==U_GHOST
#ifdef SC64_T5_RUNTIME
           ||source_type==U_KERRIGAN||source_type==U_RAYNOR_VULTURE
#endif
          )stock_class=SC64_STOCK_DMG_CONCUSSIVE;
#endif
#ifdef SC64_T5_RUNTIME
        if(source_type==U_SPIDER_MINE)stock_class=SC64_STOCK_DMG_EXPLOSIVE;
#endif
    }
    kind=sc64_pr_q8_damage_kind(stock_class);if(kind<0)return 0;
    if(stock->shields) {
        if(shield<0 || phase<0 || !sc64_pr_q8_actor_shield(target,&shield_q8))return 0;
    }
    in.packet_q8=packet;in.hp_q8=hp_q8;in.shield_q8=shield_q8;
    in.hp_armor_q8=(unsigned)armor*256u;
    in.shield_armor_q8=sc64_pr_q8_shield_level(owner)*256u;
    in.has_shields=stock->shields!=0;in.divisor=1;
    in.damage_type=(unsigned)kind;in.size=stock->size;
    if(!sc64_shield_damage_q8(&in,&result))return 0;
    int whole;
    if ((result.hp_after_q8+255u)/256u>(unsigned)hp ||
        !sc64_pr_q8_commit_shield(target,result.shield_after_q8) ||
        !sc64_pr_q8_store_hp_fraction(target,result.hp_after_q8,&whole)) return 0;
    *hp_debit=hp-whole;
    if(target>=MAX_UNITS && result.hp_damage_q8 &&
       race_building_can_burn(&b[target-MAX_UNITS]) && source_owner>=0 && source_owner<5)
        b[target-MAX_UNITS].shield_remainder=(int)sc64_building_burn_with_owner(
            (unsigned)b[target-MAX_UNITS].shield_remainder,(unsigned)source_owner);
    (void)source_type;return 1;
}
#endif
