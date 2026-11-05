/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_CAMPAIGN_CONSTRUCTION_H
#define SC64_GUARDIAN_CAMPAIGN_CONSTRUCTION_H
/* HOST construction experiment. The paid marker is not yet authenticated by
 * the selected-journal codecs; do not enable this as a guest save service. */
static int race_guardian_campaign_puni(const unsigned char *puni,int source,int raw)
{
    if (!puni || (unsigned)source>=12u || (unsigned)raw>=228u) return 0;
    unsigned cell=(unsigned)source*228u+(unsigned)raw;
    return puni[2964u+cell] ? puni[2736u+(unsigned)raw]!=0 : puni[cell]!=0;
}

static int race_guardian_campaign_allowed(int owner,int source,int raw)
{
    if (race_mode || owner!=SIDE || source<0 || source>=8) return 0;
#ifdef SC64_Z8_RUNTIME
    if (race_z8_active()) {
        int type=sc64_race_building_from_raw(raw);
        return type>=0 && race_z8_colony_product_available(owner,source,type);
    }
#endif
#ifdef SC64_Z9_RUNTIME
    if (race_zl_active()) {
        int type=sc64_race_building_from_raw(raw);
        return type>=0 && race_zl_colony_product_available(owner,source,type);
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) return race_z7_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z7_source_puni,source,raw);
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) return race_z6_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z6_source_puni,source,raw);
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active()) return race_z5_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z5_source_puni,source,raw);
#endif
    return 0; /* Earlier campaign PUNI has not been authenticated here. */
}

static int race_guardian_campaign_morph(int building,int target)
{
    if ((unsigned)building>=MAX_BUILDINGS ||
        (target!=B_HIVE && target!=B_GREATER_SPIRE)) return 0;
    Building *q=&b[building];
    int source=target==B_HIVE ? B_LAIR : B_SPIRE;
    int prerequisite=target==B_HIVE ? B_ZP_QUEENS_NEST : B_HIVE;
    int found=0;
    if (!q->alive || q->owner!=SIDE || !building_complete(building) ||
        q->type!=source || q->raw_type!=sc64_race_raw_building(source) ||
        q->maxhp!=sc64_race_building_max_hp(source) || q->hp<=0 || q->hp>q->maxhp ||
        q->queue_count || (q->flags & SC64_GUARDIAN_PAID_BUILDING_MORPH) ||
        !race_guardian_campaign_allowed(q->owner,q->raw_owner,sc64_race_raw_building(target))) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].owner==q->owner && b[i].type==prerequisite && building_complete(i)) {
            found=1; break;
        }
    if (!found) return 0;
    Sc64RaceBank bank={(unsigned short)race_owner_minerals(q->owner),
                       (unsigned short)race_owner_gas(q->owner)};
    if (!sc64_race_pay_building(&bank,target)) return 0;
    q->type=target; q->raw_type=sc64_race_raw_building(target);
    q->maxhp=sc64_race_building_max_hp(target);
    q->under_construction=1; q->build_progress=0;
    q->flags |= SC64_GUARDIAN_PAID_BUILDING_MORPH;
    minerals=bank.minerals; gas=bank.gas;
    invalidate_path_grid();
    return 1;
}

static int race_guardian_campaign_cancel(int building)
{
    if ((unsigned)building>=MAX_BUILDINGS) return 0;
    Building *q=&b[building];
    if (!q->alive || q->owner!=SIDE || !q->under_construction ||
        !(q->flags & SC64_GUARDIAN_PAID_BUILDING_MORPH) ||
        (q->type!=B_HIVE && q->type!=B_GREATER_SPIRE) ||
        q->raw_type!=sc64_race_raw_building(q->type) || q->hp<=0 ||
        q->maxhp!=sc64_race_building_max_hp(q->type) || q->hp>q->maxhp ||
        q->build_progress<0 || q->build_progress>building_build_time(q->type)) return 0;
    const Sc64RaceBuildingRule *product=sc64_race_building_rule(q->type);
    int source=q->type==B_HIVE ? B_LAIR : B_SPIRE;
    int source_hp=sc64_race_building_max_hp(source);
    q->type=source; q->raw_type=sc64_race_raw_building(source);
    q->maxhp=source_hp; if (q->hp>source_hp) q->hp=source_hp;
    q->flags &= (u16)~SC64_GUARDIAN_PAID_BUILDING_MORPH;
    q->under_construction=0; q->build_progress=0;
    race_add_owner_resources(q->owner,(product->minerals*3)/4,(product->gas*3)/4);
    invalidate_path_grid();
    return 1;
}
#endif
