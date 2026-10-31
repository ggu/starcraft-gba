/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_IMPACT_PACKET_H
#define SC64_SCARAB_IMPACT_PACKET_H
/* In-memory only; never admitted by the impact save codec. The five payload
 * bytes hold x12,y12,target9,owner3,source-ticks3 and one reserved zero bit. */
#define SC64_SCARAB_IMPACT_MARKER 0xfeu
#define SC64_SCARAB_IMPACT_NO_TARGET 511u
_Static_assert(SC64_IMPACT_TYPE_LIMIT <= SC64_SCARAB_IMPACT_MARKER,
               "Scarab marker overlaps ordinary source types");
_Static_assert(MAX_UNITS + MAX_BUILDINGS < SC64_SCARAB_IMPACT_NO_TARGET,
               "Scarab target sentinel overlaps actor slots");
typedef struct { int x,y,target,owner;unsigned ticks; } Sc64ScarabImpact;
static int sc64_scarab_impact_pack(Sc64ImpactEvent *e,const Sc64ScarabImpact *p)
{
    unsigned t;
    if(!e || !p || p->x<0 || p->x>4095 || p->y<0 || p->y>4095 ||
       p->target< -1 || p->target>=MAX_UNITS+MAX_BUILDINGS ||
       p->owner<0 || p->owner>=5 || p->ticks<1 || p->ticks>6) return 0;
    t=p->target<0?SC64_SCARAB_IMPACT_NO_TARGET:(unsigned)p->target;
    e->target_meta=(uint16_t)((unsigned)p->x | (((unsigned)p->y&15u)<<12));
    e->damage_hits=(uint16_t)(((unsigned)p->y>>4) | ((t&255u)<<8));
    e->steps=(uint8_t)((t>>8) | ((unsigned)p->owner<<1) | (p->ticks<<4));
    e->source_type=SC64_SCARAB_IMPACT_MARKER;return 1;
}
static int sc64_scarab_impact_unpack(const Sc64ImpactEvent *e,Sc64ScarabImpact *p)
{
    unsigned t,owner,ticks;
    if(!e || !p || e->source_type!=SC64_SCARAB_IMPACT_MARKER || (e->steps&128u)) return 0;
    t=((unsigned)e->damage_hits>>8) | (((unsigned)e->steps&1u)<<8);
    owner=((unsigned)e->steps>>1)&7u;ticks=((unsigned)e->steps>>4)&7u;
    if ((t>=MAX_UNITS+MAX_BUILDINGS && t!=SC64_SCARAB_IMPACT_NO_TARGET) || owner>=5 || ticks>6) return 0;
    p->x=(int)(e->target_meta&4095u);
    p->y=(int)(((unsigned)e->target_meta>>12) | (((unsigned)e->damage_hits&255u)<<4));
    p->target=t==SC64_SCARAB_IMPACT_NO_TARGET ? -1 : (int)t;
    p->owner=(int)owner;p->ticks=ticks;return 1;
}
static void sc64_scarab_impact_forget_target(Sc64ImpactEvent *e)
{
    e->damage_hits=(uint16_t)(e->damage_hits | 0xff00u);
    e->steps=(uint8_t)(e->steps | 1u);
}
#endif
