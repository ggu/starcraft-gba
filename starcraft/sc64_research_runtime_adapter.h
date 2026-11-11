/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESEARCH_RUNTIME_ADAPTER_H
#define SC64_RESEARCH_RUNTIME_ADAPTER_H
/* Included by game.c after the live actor, controller and wallet definitions. */
/* Default-off lifecycle bridge. The separate HOST campaign flag admits only
 * an authenticated current human controller; normal journals stay unchanged. */
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
/* This live context is derived from the canonical selected map and its runtime
 * role resolver. Actor raw origin is intentionally absent from payer identity.
 * Detached save contexts are not admitted by this HOST experiment. */
static int sc64_pr_stage_campaign_rule(unsigned family, Sc64PrCampaignRule *out)
{
    int8_t roles[8]; Sc64PrCampaignPolicyView view=sc64_pr_campaign_generated_view();
    if (race_mode || race_selected_mission<0 || race_selected_mission>=SC64_RACE_MAP_COUNT) return 0;
    const Sc64RaceMap *map=&sc64_race_maps[race_selected_mission];
    for (unsigned source=0;source<8;++source)
        roles[source]=(int8_t)race_campaign_owner(map,(int)source);
    return sc64_pr_campaign_rule(&view,0,map->mission_id,map->human_slot,roles,SIDE,family,out);
}
static int sc64_pr_stage_campaign_initialize(void)
{
    Sc64ProtossResearchLevels candidate={0}; Sc64PrCampaignRule rule;
    for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family) {
        if (!sc64_pr_stage_campaign_rule(family,&rule)) return 0;
        candidate.level[SIDE][family]=rule.initial;
    }
    sc64_experimental_research_levels=candidate;
    return 1;
}
#endif
static int sc64_pr_stage_job_marked(const Building *q)
{ return q && (q->type==B_FORGE || q->type==B_CYBERNETICS_CORE) && (q->level || q->queue_progress); }
#ifdef SC64_RESEARCH_SRAM_PRODUCT
/* Idle-cost caches of the product. `jobs` is the exact number of producers for
 * which sc64_pr_stage_job_marked() holds; `levels_any` is set while any
 * completed typed level is nonzero. Both are derived state: they are zeroed on
 * every reset, rebuilt from the loaded world by sc64_pr_stage_resync(), and
 * maintained where a job starts, completes, is cancelled or destroyed. With
 * both clear the per-update scan and every per-hit lookup reduce to one test. */
static uint16_t sc64_pr_stage_jobs;
static uint8_t sc64_pr_stage_levels_any;
#define SC64_PR_ANY_LEVEL (sc64_pr_stage_levels_any)
#define SC64_PR_STAGE_RESET() (sc64_pr_stage_jobs=0, sc64_pr_stage_levels_any=0)
#define SC64_PR_JOBS_ADJUST(n) (sc64_pr_stage_jobs=(uint16_t)(sc64_pr_stage_jobs+(n)))
static unsigned sc64_pr_stage_jobs_scan(void)
{
    unsigned n=0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (sc64_pr_stage_job_marked(&b[i])) ++n;
    return n;
}
static unsigned sc64_pr_stage_levels_scan(void)
{
    unsigned any=0;
    for (unsigned o=0;o<SC64_PR_CAMPAIGN_OWNERS;++o)
        for (unsigned f=0;f<SC64_PR_FAMILY_COUNT;++f) any|=sc64_experimental_research_levels.level[o][f];
    return any!=0;
}
static void sc64_pr_stage_resync(void)
{
    sc64_pr_stage_jobs=(uint16_t)sc64_pr_stage_jobs_scan();
    sc64_pr_stage_levels_any=(uint8_t)sc64_pr_stage_levels_scan();
}
#else
#define SC64_PR_ANY_LEVEL 1
#define SC64_PR_STAGE_RESET() ((void)0)
#define SC64_PR_JOBS_ADJUST(n) ((void)0)
#endif
static int sc64_pr_stage_receipt(int slot, Sc64PrPaidReceipt *paid)
{
    return slot>=0 && slot<MAX_BUILDINGS &&
           sc64_pr_job_reconstruct_v45(&b[slot],paid);
}
static int sc64_pr_stage_owner_active(unsigned owner)
{
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
    if (!race_mode) {
        Sc64PrCampaignRule rule;
        return owner==SIDE && sc64_pr_stage_campaign_rule(0,&rule);
    }
#endif
    if (race_mode!=1 || race_bots < 1 || race_bots > 3) return 0;
    if (!(owner==SIDE || (owner==ENEMY && race_bots>=1) ||
          (owner==RESCUABLE && race_bots>=2) || (owner==4 && race_bots>=3))) return 0;
    return race_player[owner]==R_PROTOSS;
}
/* Prerequisite structures need to be complete and owned by the same active
 * custom controller. Their own power status is not inferred as a requirement. */
/* HOST-only ordinary combat family selection. Untyped weapons and actors
 * retain the existing generic human bonus; it is never added twice. */
/* Completed typed level of one family, clamped to the representable maximum.
 * Levels reach this array only through a validated load or a job completion,
 * so the per-hit paths below skip the 48-entry validity scan. */
static inline unsigned sc64_pr_stage_typed(int owner,unsigned family)
{
    unsigned v=sc64_experimental_research_levels.level[owner][family];
    return v>3u ? 3u : v;
}
static int sc64_pr_stage_weapon_level_slow(int owner,int raw_weapon)
{
    int base=owner==SIDE ? weapons_level : 0, id=sc64_stock_weapon_upgrade_id(raw_weapon), family;
    if (base<0 || base>2 || owner<0 || owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)owner)) return base;
    if (id==13) family=SC64_PR_GROUND_WEAPONS;
    else if (id==14) family=SC64_PR_AIR_WEAPONS;
    else return base;
    return (int)sc64_pr_effective_level((unsigned)family,
        sc64_pr_stage_typed(owner,(unsigned)family),(unsigned)base,0);
}
static int sc64_pr_stage_unit_armor_level_slow(const Unit *q)
{
    int base=q->owner==SIDE ? armor_level : 0;
    int raw=q->raw_type>=0 ? q->raw_type : sc64_race_raw_unit(q->type);
    int id=sc64_stock_armor_upgrade_id(raw), family;
    if (base<0 || base>2 || q->owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)q->owner)) return base;
    if (id==5) family=SC64_PR_GROUND_ARMOR;
    else if (id==6) family=SC64_PR_AIR_ARMOR;
    else return base;
    return (int)sc64_pr_effective_level((unsigned)family,
        sc64_pr_stage_typed(q->owner,(unsigned)family),0,(unsigned)base);
}
/* Plasma Shields: one point of shield armor per completed level, owned by the
 * target's controller and used only while the target still has shields. The
 * integer engine floors a reduced hit at one point (the original game's floor
 * is half a point); hit_target_impl_ex() and the siege packet call this. */
static int sc64_pr_stage_shield_armor_slow(int target)
{
    int owner=target<MAX_UNITS ? u[target].owner : b[target-MAX_UNITS].owner;
    if (owner<0 || owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)owner)) return 0;
    return (int)sc64_pr_stage_typed(owner,SC64_PR_SHIELDS);
}
/* With no completed level anywhere every result below is the legacy value. */
static inline int sc64_pr_stage_weapon_level(int owner,int raw_weapon)
{
    if (!SC64_PR_ANY_LEVEL) return owner==SIDE ? weapons_level : 0;
    return sc64_pr_stage_weapon_level_slow(owner,raw_weapon);
}
static inline int sc64_pr_stage_unit_armor_level(const Unit *q)
{
    if (!SC64_PR_ANY_LEVEL) return q->owner==SIDE ? armor_level : 0;
    return sc64_pr_stage_unit_armor_level_slow(q);
}
static inline int sc64_pr_stage_shield_armor(int target)
{
    if (!SC64_PR_ANY_LEVEL) return 0;
    return sc64_pr_stage_shield_armor_slow(target);
}
static int sc64_pr_stage_dragoon_bonus_slow(const Unit *q);
static inline int sc64_pr_stage_dragoon_bonus(const Unit *q)
{
    if (q->type!=U_DRAGOON || !SC64_PR_ANY_LEVEL) return 0;
    return sc64_pr_stage_dragoon_bonus_slow(q);
}
static inline int sc64_pr_stage_shield_reduce(int target,int damage)
{
    int armor=sc64_pr_stage_shield_armor(target);
    if (!armor) return damage;
    damage-=armor;
    return damage<1 ? 1 : damage;
}
/* Pinned OpenBW grants the standard raw-66 Dragoon +64 source pixels
 * (32 at this engine's half scale) on either weapon target. Hero raw IDs
 * retain their existing behavior until their independent rule is integrated. */
static int sc64_pr_stage_dragoon_bonus_slow(const Unit *q)
{
    int raw=q->raw_type>=0 ? q->raw_type : sc64_race_raw_unit(q->type);
    if (q->type!=U_DRAGOON || raw!=66 || q->owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)q->owner)) return 0;
    return sc64_pr_stage_typed(q->owner,SC64_PR_SINGULARITY) ? 32 : 0;
}
static int sc64_pr_stage_has_raw(unsigned owner, int raw)
{
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Building *q=&b[i]; int mapped;
        if (!q->alive || q->under_construction || q->owner!=(int)owner) continue;
        mapped=sc64_race_raw_building(q->type);
        if (mapped==raw && (q->raw_type<0 || q->raw_type==mapped)) return 1;
    }
    return 0;
}
static int sc64_pr_stage_begin_slot(int slot,unsigned family)
{
    Building *q, candidate; Sc64ProtossResearchQuote quote;
    unsigned owner, level, max; int ore,vespene, legacy_w,legacy_a,pending_w,pending_a;
    if (sc64_pr_stage_fault || slot<0 || slot>=MAX_BUILDINGS || family>=SC64_PR_FAMILY_COUNT) return 0;
    q=&b[slot]; owner=(unsigned)q->owner;
    if (owner>=SC64_PR_CUSTOM_OWNERS || !sc64_pr_stage_owner_active(owner) ||
        !q->alive || q->under_construction || !sc64_pr_job_family_producer(family,q->type) ||
        (q->raw_type>=0 && q->raw_type!=sc64_race_raw_building(q->type)) ||
        !race_site_powered((int)owner,q->x,q->y)) return 0;
    legacy_w=owner==SIDE ? weapons_level : 0; legacy_a=owner==SIDE ? armor_level : 0;
    pending_w=owner==SIDE && weapons_research; pending_a=owner==SIDE && armor_research;
    if (legacy_w<0 || legacy_w>2 || legacy_a<0 || legacy_a>2 ||
        !sc64_pr_levels_valid(&sc64_experimental_research_levels,SC64_PR_CUSTOM_OWNERS)) return 0;
    level=sc64_pr_effective_level(family,sc64_experimental_research_levels.level[owner][family],
                                  (unsigned)legacy_w,(unsigned)legacy_a);
    max=family==SC64_PR_SINGULARITY ? 1u : 3u;
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
    if (!race_mode) {
        Sc64PrCampaignRule rule;
        if (!sc64_pr_stage_campaign_rule(family,&rule) ||
            !sc64_pr_campaign_quote(&rule,family,level,&quote)) return 0;
        max=rule.maximum;
    }
#endif
    if (level>=max || !sc64_pr_quote_v45(family,level,&quote)) return 0;
    if (level>=1 && family==SC64_PR_SHIELDS && !sc64_pr_stage_has_raw(owner,164)) return 0;
    if (level>=1 && (family==SC64_PR_GROUND_ARMOR || family==SC64_PR_GROUND_WEAPONS) &&
        !sc64_pr_stage_has_raw(owner,165)) return 0;
    if (level>=1 && (family==SC64_PR_AIR_ARMOR || family==SC64_PR_AIR_WEAPONS) &&
        !sc64_pr_stage_has_raw(owner,169)) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Sc64PrPaidReceipt existing;
        if (!sc64_pr_stage_job_marked(&b[i])) continue;
        if (!sc64_pr_stage_receipt(i,&existing) ||
            !sc64_pr_stage_owner_active(existing.payer)) return 0;
        if (existing.payer==owner && existing.family==family) return 0;
    }
    ore=owner==SIDE ? minerals : race_minerals[owner];
    vespene=owner==SIDE ? gas : race_gas[owner];
    if (ore<0 || vespene<0 || (owner!=SIDE && (ore>65535 || vespene>65535))) return 0;
    candidate=*q;
    if (!candidate.level && !candidate.queue_progress && !candidate.queue_count) {
        candidate.queue_head=0;
        for (unsigned j=0;j<5;++j) candidate.queue[j]=0;
    }
    if (!sc64_pr_job_begin_custom(&candidate,owner,family,&sc64_experimental_research_levels,
        &ore,&vespene,max,1,1,1,0,(unsigned)legacy_w,(unsigned)legacy_a,pending_w,pending_a)) return 0;
    *q=candidate; if (owner==SIDE) { minerals=ore; gas=vespene; }
    else { race_minerals[owner]=ore; race_gas[owner]=vespene; }
    SC64_PR_JOBS_ADJUST(1);
    return 1;
}
static int sc64_pr_stage_menu_selected(void)
{
    int slot=selected_building;
    if (slot<0 || slot>=MAX_BUILDINGS || !sc64_pr_stage_owner_active(SIDE) ||
        !b[slot].alive || b[slot].under_construction || b[slot].owner!=SIDE ||
        (b[slot].type!=B_FORGE && b[slot].type!=B_CYBERNETICS_CORE)) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].selected) return 0;
    return 1;
}
static int sc64_pr_stage_menu_family(int row)
{
    static const uint8_t forge[]={SC64_PR_GROUND_WEAPONS,SC64_PR_GROUND_ARMOR,SC64_PR_SHIELDS};
    static const uint8_t core[]={SC64_PR_AIR_WEAPONS,SC64_PR_AIR_ARMOR,SC64_PR_SINGULARITY};
    if (!sc64_pr_stage_menu_selected() || row<0 || row>=3) return -1;
    return b[selected_building].type==B_FORGE ? forge[row] : core[row];
}
static int sc64_pr_stage_menu_quote(int row,Sc64ProtossResearchQuote *quote)
{
    int family=sc64_pr_stage_menu_family(row); unsigned level;
    if (family<0 || weapons_level<0 || weapons_level>2 || armor_level<0 || armor_level>2) return 0;
    level=sc64_pr_effective_level((unsigned)family,sc64_experimental_research_levels.level[SIDE][family],
                                  (unsigned)weapons_level,(unsigned)armor_level);
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
    if (!race_mode) {
        Sc64PrCampaignRule rule;
        return sc64_pr_stage_campaign_rule((unsigned)family,&rule) &&
               sc64_pr_campaign_quote(&rule,(unsigned)family,level,quote);
    }
#endif
    return sc64_pr_quote_v45((unsigned)family,level,quote);
}
static int sc64_pr_stage_cancel_slot(int slot, int destroyed)
{
    Building candidate; Sc64PrPaidReceipt paid; unsigned owner; int ore,vespene;
    if (slot<0 || slot>=MAX_BUILDINGS) return 0;
    if (!sc64_pr_stage_receipt(slot,&paid)) return 0;
    owner=paid.payer;
    if (!paid.live || owner>=SC64_PR_CUSTOM_OWNERS || !sc64_pr_stage_owner_active(owner)) return 0;
    ore=owner==SIDE ? minerals : race_minerals[owner];
    vespene=owner==SIDE ? gas : race_gas[owner]; candidate=b[slot];
    if (!sc64_pr_job_cancel_custom(&candidate,owner,&paid,destroyed,&ore,&vespene,
                                  owner==SIDE ? INT_MAX : 65535)) return 0;
    b[slot]=candidate; if (owner==SIDE) { minerals=ore; gas=vespene; }
    else { race_minerals[owner]=ore; race_gas[owner]=vespene; }
    SC64_PR_JOBS_ADJUST(-1);
    return 1;
}
static int sc64_pr_stage_destroy_slot(int slot)
{
    Building candidate; Sc64PrPaidReceipt paid; unsigned owner; int ore,vespene;
    if (slot<0 || slot>=MAX_BUILDINGS) return 0;
    if (!sc64_pr_stage_receipt(slot,&paid)) return 0;
    owner=paid.payer;
    if (!paid.live || owner>=SC64_PR_CUSTOM_OWNERS || !sc64_pr_stage_owner_active(owner)) return 0;
    ore=owner==SIDE ? minerals : race_minerals[owner];
    vespene=owner==SIDE ? gas : race_gas[owner]; candidate=b[slot];
    if (!sc64_pr_job_destroy_custom(&candidate,owner,&paid,&ore,&vespene,
                                   owner==SIDE ? INT_MAX : 65535)) return 0;
    b[slot]=candidate; if (owner==SIDE) { minerals=ore; gas=vespene; }
    else { race_minerals[owner]=ore; race_gas[owner]=vespene; }
    SC64_PR_JOBS_ADJUST(-1);
    return 1;
}
static int sc64_pr_stage_transfer_allowed(int slot,int new_owner)
{
    if (slot<0 || slot>=MAX_BUILDINGS) return 0;
    if (!sc64_pr_stage_job_marked(&b[slot])) return 1;
    return b[slot].owner==new_owner;
}
/* Validate all rows and duplicate keys before advancing any job. */
static void sc64_pr_stage_tick(void)
{
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    if (!sc64_pr_stage_jobs) return; /* idle: nothing to validate or advance */
#endif
    if (sc64_pr_stage_fault) return;
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Sc64PrPaidReceipt p;
        if (!sc64_pr_stage_job_marked(&b[i])) continue;
        if (!sc64_pr_stage_receipt(i,&p) || !sc64_pr_stage_owner_active(p.payer)) return;
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
        if (!race_mode) {
            Sc64PrCampaignRule rule;
            if (!sc64_pr_stage_campaign_rule(p.family,&rule) || !rule.cost_supported ||
                sc64_experimental_research_levels.level[SIDE][p.family]<rule.initial ||
                p.target>rule.maximum) return;
        }
#endif
        for (int j=0;j<i;++j) if (sc64_pr_stage_job_marked(&b[j])) {
            Sc64PrPaidReceipt earlier;
            if (!sc64_pr_stage_receipt(j,&earlier) ||
                (earlier.payer==p.payer && earlier.family==p.family)) return;
        }
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (sc64_pr_stage_job_marked(&b[i])) {
        Sc64PrPaidReceipt p; unsigned o;
        if (!sc64_pr_stage_receipt(i,&p)) return;
        o=p.payer;
        int lw=o==SIDE ? weapons_level : 0, la=o==SIDE ? armor_level : 0;
        int pw=o==SIDE && weapons_research, pa=o==SIDE && armor_research;
        if (!race_site_powered((int)o,b[i].x,b[i].y)) continue;
        if (lw<0 || lw>2 || la<0 || la>2) continue;
        if (sc64_pr_job_tick_custom(&b[i],o,&sc64_experimental_research_levels,
                                    (unsigned)lw,(unsigned)la,pw,pa,1)==1) {
            SC64_PR_JOBS_ADJUST(-1);
#ifdef SC64_RESEARCH_SRAM_PRODUCT
            sc64_pr_stage_levels_any=1;
#endif
        }
    }
}
static int sc64_pr_stage_blocks_legacy(int type)
{
    for (int i=0;i<MAX_BUILDINGS;++i) if (sc64_pr_stage_job_marked(&b[i])) {
        Sc64PrPaidReceipt p; unsigned f;
        if (!sc64_pr_stage_receipt(i,&p)) return 1;
        if (p.payer!=SIDE) continue;
        f=p.family;
        if ((!type && (f==SC64_PR_GROUND_WEAPONS || f==SC64_PR_AIR_WEAPONS)) ||
            (type && (f==SC64_PR_GROUND_ARMOR || f==SC64_PR_AIR_ARMOR))) return 1;
    }
    return 0;
}

/* Forge/Core menu command, row 3 cancels. Returns the toast to show: 0 for an
 * accepted command, otherwise the reason it was refused. A failed start never
 * mutates the wallet or the producer (begin_slot validates before storing). */
static int sc64_pr_stage_menu_command(int choice)
{
    int slot=selected_building, family;
    Sc64ProtossResearchQuote quote;
    if (choice==3) return sc64_pr_stage_cancel_slot(slot,0) ? 0 : 10;
    family=sc64_pr_stage_menu_family(choice);
    if (family<0) return 8;
    if (sc64_pr_stage_begin_slot(slot,(unsigned)family)) return 0;
    if (b[slot].level || b[slot].queue_progress || !sc64_pr_stage_menu_quote(choice,&quote)) return 10;
    for (int i=0;i<MAX_BUILDINGS;++i) { /* the same family is already running elsewhere */
        Sc64PrPaidReceipt other;
        if (sc64_pr_stage_job_marked(&b[i]) && sc64_pr_stage_receipt(i,&other) &&
            other.payer==SIDE && other.family==(unsigned)family) return 10;
    }
    if (minerals<quote.mineral) { toast_refusal(1); return 1; }
    if (gas<quote.gas) { toast_refusal(12); return 12; }
    return 8;
}
/* Selected-building HUD. A Forge or Core shows three slots: weapons, armor and
 * the third family (Plasma Shields, Singularity Charge), each with its
 * effective level; a running job fills the bar under its own slot. */
static int sc64_pr_stage_hud_building(const Building *q)
{
    return (q->type==B_FORGE || q->type==B_CYBERNETICS_CORE) && !q->under_construction &&
           q->owner==SIDE && sc64_pr_stage_owner_active(SIDE);
}
static int sc64_pr_stage_hud_slot(unsigned family)
{
    return family==SC64_PR_GROUND_WEAPONS || family==SC64_PR_AIR_WEAPONS ? 0 :
           family==SC64_PR_GROUND_ARMOR || family==SC64_PR_AIR_ARMOR ? 1 : 2;
}
static int sc64_pr_stage_hud_level(int slot,int i)
{
    static const uint8_t forge[]={SC64_PR_GROUND_WEAPONS,SC64_PR_GROUND_ARMOR,SC64_PR_SHIELDS};
    static const uint8_t core[]={SC64_PR_AIR_WEAPONS,SC64_PR_AIR_ARMOR,SC64_PR_SINGULARITY};
    unsigned family=(b[slot].type==B_FORGE ? forge : core)[i];
    if (weapons_level<0 || weapons_level>2 || armor_level<0 || armor_level>2) return 0;
    return (int)sc64_pr_effective_level(family,sc64_pr_stage_typed(SIDE,family),
                                        (unsigned)weapons_level,(unsigned)armor_level);
}
static int sc64_pr_stage_hud_job(int slot,int *slot_out,int *done,int *total)
{
    Sc64PrPaidReceipt paid; Sc64ProtossResearchQuote quote;
    if (!sc64_pr_stage_job_marked(&b[slot]) || !sc64_pr_job_reconstruct_v45(&b[slot],&paid) ||
        !sc64_pr_quote_v45(paid.family,(unsigned)paid.target-1u,&quote)) return 0;
    *slot_out=sc64_pr_stage_hud_slot(paid.family); *total=quote.ticks;
    *done=quote.ticks-b[slot].queue_progress;
    return 1;
}
/* Selected-unit HUD numbers: the levels that unit actually fights with. */
static int sc64_pr_stage_hud_weapons(const Unit *q)
{
    int base=q->owner==SIDE ? weapons_level : 0, flying=sc64_race_unit_flying(q->type);
    unsigned family=flying ? SC64_PR_AIR_WEAPONS : SC64_PR_GROUND_WEAPONS;
    if (base<0 || base>2 || q->owner>=SC64_PR_CUSTOM_OWNERS ||
        !sc64_pr_stage_owner_active((unsigned)q->owner)) return base;
    return (int)sc64_pr_effective_level(family,sc64_pr_stage_typed(q->owner,family),(unsigned)base,0);
}

#endif
