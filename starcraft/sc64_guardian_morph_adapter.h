/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_MORPH_ADAPTER_H
#define SC64_GUARDIAN_MORPH_ADAPTER_H
/* HOST-only staged service. Mission permission and UI admission are separate. */
#include "sc64_guardian_cocoon.h"

static int race_guardian_product_type(void)
{
#ifdef SC64_Z6_ACTORS
    return U_Z6_GUARDIAN;
#elif defined(SC64_SKIRMISH_ROSTER)
    return U_SK_GUARDIAN;
#else
    return U_GUARDIAN_STAGE;
#endif
}

static void race_guardian_transform(int slot, int type, int hp, int maxhp,
                                    int raw, int target, int timer)
{
    Unit *q=&u[slot];
    q->type=type; q->raw_type=raw; q->hp=hp; q->maxhp=maxhp;
    q->armor=sc64_race_unit_armor(type); q->shield=0; q->shield_remainder=0;
    q->order=ORDER_IDLE; q->target=target; q->tx=q->x; q->ty=q->y;
    q->carry=0; q->build=-1; q->work_timer=timer; q->flags=0; q->cd=0;
    q->path_timer=0; q->path_retry=0; q->path_x=q->x; q->path_y=q->y;
    /* Same owner and position keep the spatial and target indexes valid.
     * Incoming attacks remain bound to this slot; only animation state resets. */
    sc64_walk_reset_slot(slot);
#ifdef SC64_UNIT_FACING
    sc64_facing_reset_slot(slot);
#endif
#ifdef SC64_COLLISION_CLASS_CACHE
    sc64_unit_collision_class_invalidate(slot);
#endif
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif
}

static int race_guardian_morph_start(int slot)
{
    if (slot<0 || slot>=MAX_UNITS) return 0;
    Unit *q=&u[slot];
    if (!q->alive || (unsigned)q->owner>=5u || race_owner_identity(q->owner)<0 ||
        q->type!=U_MUTALISK ||
        (q->raw_type!=-1 && q->raw_type!=43) || q->flags ||
        SC64_TRANSPORT_LOADED(q) || q->hp<=0 || q->maxhp!=120 || q->hp>120 ||
        race_owner_minerals(q->owner)<50 || race_owner_gas(q->owner)<100) return 0;
    int has_spire=0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].owner==q->owner && b[i].type==B_GREATER_SPIRE && building_complete(i)) {
            has_spire=1;break;
        }
    if (!has_spire) return 0;
    int hp=sc64_guardian_scaled_hp(q->hp,120,200);
    if (!hp) return 0;
    race_add_owner_resources(q->owner,-50,-100);
    race_guardian_transform(slot,U_GUARDIAN_COCOON,hp,200,59,44,SC64_GUARDIAN_TICKS);
    return 1;
}

static int race_guardian_morph_cancel(int slot)
{
    if (slot<0 || slot>=MAX_UNITS) return 0;
    Unit *q=&u[slot];
    if (!q->alive || (unsigned)q->owner>=5u || race_owner_identity(q->owner)<0 ||
        q->type!=U_GUARDIAN_COCOON ||
        q->raw_type!=59 || q->target!=44 || q->maxhp!=200 || q->hp<=0 || q->hp>200 ||
        q->work_timer<1 || q->work_timer>SC64_GUARDIAN_TICKS) return 0;
    int hp=sc64_guardian_scaled_hp(q->hp,200,120);
    if (!hp) return 0;
    int owner=q->owner;
    race_guardian_transform(slot,U_MUTALISK,hp,120,43,-1,0);
    race_add_owner_resources(owner,50,100);
    return 1;
}

static int race_guardian_morph_tick(int slot)
{
    if (slot<0 || slot>=MAX_UNITS) return 0;
    Unit *q=&u[slot];
    if (!q->alive || q->type!=U_GUARDIAN_COCOON || q->raw_type!=59 ||
        q->target!=44 || q->maxhp!=200 || q->hp<=0 || q->hp>200 ||
        q->work_timer<1 || q->work_timer>SC64_GUARDIAN_TICKS) return 0;
    if (--q->work_timer) return 1;
    int hp=sc64_guardian_scaled_hp(q->hp,200,150);
    race_guardian_transform(slot,race_guardian_product_type(),hp,150,44,-1,0);
    return 1;
}
#endif
