/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHARED_ORDINARY_STAGE_H
#define SC64_SHARED_ORDINARY_STAGE_H
/* Experimental source-profile slice for selected common weapons. Pending attack wind-up is
 * separate from live bullets: only actual release attempts claim arena slots.
 * Other weapons keep their existing path and do not yet contend here. */
_Static_assert(MAX_UNITS <= 256, "ordinary reservation owner fits the arena byte");
typedef struct {
    int target, damage;
    s16 order, actor_target;
    u8 active, raw, owner, source_type, damage_class, shots;
    u8 wait[2], impact[2], life[2];
} Sc64OrdinaryPending;
typedef struct {
    u32 handle;
    int target, damage;
    u8 owner, source_type, damage_class, impact, life, damaged, reservation_owner;
} Sc64OrdinaryBullet;
static Sc64OrdinaryPending sc64_ordinary_pending[MAX_UNITS];
static Sc64OrdinaryBullet sc64_ordinary_bullets[SC64_SHARED_PROJECTILE_CAPACITY];

static void race_shared_ordinary_cancel(int slot)
{
    if(slot>=0 && slot<MAX_UNITS) memset(&sc64_ordinary_pending[slot],0,sizeof(sc64_ordinary_pending[slot]));
}
static void race_shared_ordinary_forget_target(int ref)
{
    for(int i=0;i<MAX_UNITS;i++)
        if(sc64_ordinary_pending[i].active && sc64_ordinary_pending[i].target==ref)
            race_shared_ordinary_cancel(i);
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++)
        if(sc64_ordinary_bullets[i].handle && sc64_ordinary_bullets[i].target==ref)
            sc64_ordinary_bullets[i].target=-1;
}
static void race_shared_ordinary_reset(void)
{
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) {
        Sc64OrdinaryBullet *b=&sc64_ordinary_bullets[i];
        if(b->handle) (void)sc64_shared_projectile_release(&sc64_shared_projectiles,b->handle,
                                      SC64_SHARED_PROJECTILE_IMPACT,b->reservation_owner);
    }
    memset(sc64_ordinary_pending,0,sizeof(sc64_ordinary_pending));
    memset(sc64_ordinary_bullets,0,sizeof(sc64_ordinary_bullets));
}
static int race_shared_ordinary_pending_any(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(sc64_ordinary_pending[i].active)return 1;
    return 0;
}
/* Only explicitly source-tested actor/weapon/mode pairs participate.
 * Scout and Goliath turret each release two independent bullets; no generic
 * factor-to-bullet inference is made for untested weapons. */
static int race_shared_ordinary_prepare(Unit *q,int target,int flying,int damage,
                                        int hits,const Sc64StockAttack *attack)
{
    int slot=(int)(q-u),raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    if(slot<0||slot>=MAX_UNITS||!attack||!attack->weapon) return 0;
    if(!((raw==0 && attack->weapon_id==0 && hits==1) ||
         (raw==70 && flying && attack->weapon_id==74 && hits==2) ||
         (raw==3 && flying && attack->weapon_id==8 && hits==2) ||
         (raw==2 && !flying && attack->weapon_id==4 && hits==1) ||
         (raw==8 && !flying && attack->weapon_id==16 && hits==1) ||
         (raw==38 && !flying && attack->weapon_id==38 && hits==1) ||
         (raw==66 && !flying && attack->weapon_id==66 && hits==1))) return 0;
    unsigned shots=(raw==70 || raw==3)?2u:1u;
    Sc64OrdinaryPending p={0};
    if(sc64_ordinary_pending[slot].active)return 1;
    p.target=target;p.damage=damage;p.order=q->order;p.actor_target=q->target;
    p.raw=(u8)raw;p.owner=(u8)q->owner;p.source_type=(u8)q->type;
    p.damage_class=(u8)(attack->weapon->damage_class+1);p.shots=(u8)shots;
    for(unsigned i=0;i<shots;i++) {
        const Sc64WeaponSourceAttackProfile *profile=sc64_weapon_source_profile_lookup((u8)raw,attack->weapon_id,(u8)flying,(u8)i);
        if(!profile || profile->weapon_count!=shots)return 0;
        unsigned release=sc64_weapon_source_vfx_first_gba_age(sc64_weapon_source_trace_release_ticks(profile));
        unsigned impact=race_impact_profile_steps(profile,q->x,q->y,target_x(target),target_y(target));
        Sc64WeaponSourceMotion motion=sc64_weapon_source_motion_prepare(profile,q->x,q->y,target_x(target),target_y(target));
        const Sc64WeaponSourceAnimation *death=sc64_weapon_source_animation_lookup(profile->projectile_image,1u);
        if(!death || (!sc64_weapon_source_bullet_is_stationary(profile->weapon_bullet_type) && motion.distance && !motion.flight))return 0;
        unsigned life_source=sc64_weapon_source_trace_startup_ticks(profile)+motion.flight+
                             sc64_weapon_source_animation_visual_ticks(death);
        unsigned life=sc64_weapon_source_vfx_gba_duration(life_source);
        if(release>=254 || life>=254 || impact>=254)return 0;
        p.wait[i]=(u8)(release+1u);
        /* Appear-at-target source bullet applies its first impact at release. */
        p.impact[i]=impact>release ? (u8)(impact-release) : 0;
        /* Only the projectile's own Init/flight/Death contribute: the
         * attacker's visual animation must not keep an arena slot occupied. */
        p.life[i]=(u8)(life?life:1u);
        if(p.life[i]<p.impact[i])p.life[i]=p.impact[i];
    }
    p.active=1;sc64_ordinary_pending[slot]=p;return 1;
}
static void race_shared_ordinary_release_tick(void)
{
    for(int i=0;i<MAX_UNITS;i++) {
        Sc64OrdinaryPending *p=&sc64_ordinary_pending[i];Unit *q=&u[i];
        if(!p->active)continue;
        int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
        if(!q->alive || raw!=p->raw || q->owner!=p->owner || q->order!=p->order ||
           q->target!=p->actor_target || !target_alive(p->target)) {
            race_shared_ordinary_cancel(i);continue;
        }
        int waiting=0;
        for(unsigned shot=0;shot<p->shots;shot++) {
            if(!p->wait[shot])continue;
            if(--p->wait[shot]) {waiting=1;continue;}
#ifdef SC64_SHARED_DARK_SWARM_PROBE
            /* These source-tested normal-hit weapons receive the guaranteed
             * field miss. Air-only staged pairs cannot hit a ground target. */
            int swarm=(p->raw==0 || p->raw==2 || p->raw==8 || p->raw==38 || p->raw==66)
                ?sc64_world_dark_swarm_affects_target(p->target):0;
            if(swarm<0)continue; /* unknown geometry: no unsafe staged hit */
#endif
            u32 handle=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_IMPACT,(unsigned)i);
            if(!handle)continue; /* source allocation failure: cooldown was already spent */
            unsigned slot=(handle&255u)-1u;
            Sc64OrdinaryBullet *b=&sc64_ordinary_bullets[slot];
            b->handle=handle;b->target=p->target;b->damage=p->damage;b->owner=p->owner;
#ifdef SC64_SHARED_DARK_SWARM_PROBE
            if(swarm>0)b->target=-1; /* source threshold 255 misses every roll */
#endif
            b->source_type=p->source_type;b->damage_class=p->damage_class;
            b->impact=p->impact[shot];b->life=p->life[shot];b->damaged=0;b->reservation_owner=(u8)i;
        }
        if(!waiting)race_shared_ordinary_cancel(i);
    }
}
static void race_shared_ordinary_bullet_tick(void)
{
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) {
        Sc64OrdinaryBullet *b=&sc64_ordinary_bullets[i];if(!b->handle)continue;
        /* Match the existing impact path: loss of targetability permanently
         * clears the reference, even if that slot later becomes visible. */
        if(b->target>=0 && !target_alive(b->target)) b->target=-1;
        if(!b->damaged && (!b->impact || !--b->impact)) {
            if(target_alive(b->target))hit_target_impl(b->target,b->damage,b->source_type,b->owner,(int)b->damage_class-1);
            b->damaged=1;
        }
        if(b->life && !--b->life) {
            (void)sc64_shared_projectile_release(&sc64_shared_projectiles,b->handle,SC64_SHARED_PROJECTILE_IMPACT,b->reservation_owner);
            memset(b,0,sizeof(*b));
        }
    }
}
static void race_shared_ordinary_tick(void)
{ race_shared_ordinary_release_tick(); race_shared_ordinary_bullet_tick(); }
#endif
