/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHARED_EGG_WIRE_HOST_H
#define SC64_SHARED_EGG_WIRE_HOST_H
#if !defined(SC64_SHARED_EGG_WIRE_HOST_STAGE) || \
    (!defined(GBA_HOST) && !defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE))
#error "Egg wire validation requires the isolated Egg candidate"
#endif
/* This is an admission primitive, not an installed journal decoder. The
 * caller must authenticate the new stage tag and the candidate's owner map;
 * Z7's existing legacy validator remains a separate, unchanged path. */
typedef int (*Sc64EggOwnerCheck)(int owner,int raw_owner,void *context);
static int sc64_shared_egg_wire_valid(const Unit *q,int stage_tag,
                                      Sc64EggOwnerCheck owner_check,void *context)
{
    const Sc64RaceUnitRule *product;
    int duration;
    if (!stage_tag || !q || !owner_check || q->alive!=1 || q->selected<0 ||
        q->selected>1 || q->type!=SC64_SHARED_EGG_TYPE || q->raw_type!=36 ||
        q->owner<0 || q->owner>=5 || !owner_check(q->owner,q->raw_owner,context) ||
        q->hp<1 || q->hp>200 || q->maxhp!=200 || q->armor!=10 ||
        q->shield!=0 || q->shield_remainder!=0 || q->flags!=0 || q->carry!=0 ||
        q->cd!=0 || q->order!=ORDER_IDLE || q->build!=-1 ||
        q->path_timer!=0 || q->path_retry!=0 || q->tx!=q->x || q->ty!=q->y)
        return 0;
    product=sc64_race_unit_rule(q->target);
    if (!product || product->race!=R_ZERG || !(product->flags & SC_RACE_F_LARVA) ||
        product->producer!=B_HATCHERY || product->build_frames<=0) return 0;
    duration=((int)product->build_frames*5+3)/4;
    return duration>0 && duration<=2047 && q->work_timer>=1 &&
           q->work_timer<=duration;
}
#endif
