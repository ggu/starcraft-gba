#ifndef SC64_P6_COMBAT_ADAPTER_H
#define SC64_P6_COMBAT_ADAPTER_H

/* P6 live combat policy. Include after the engine Unit/Building arrays and
 * before sc64_p6_runtime_adapter.h; the runtime adapter supplies the small
 * forward-declared mission/death/doodad callbacks below. */
#if defined(SC64_P6_COMBAT_ENABLED)
#if !defined(SC64_P6_ACTORS) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "P6 combat requires the P6 and Z/P actor tables"
#endif

#include "sc64_p6_combat.h"
#include "sc64_p6_world_adapter.h"
#include "sc64_z5_detection.h"

static int race_p6_active(void);
static int race_p6_owners_hostile(int, int);
static int race_p6_doodad_enabled_slot(int);
static int race_p6_doodad_slot_bound(int);
static int race_p6_combat_unit_burrowed(int);
static void race_p6_note_unit_death(const Unit *);
static void race_p6_note_building_death(const Building *);

enum {
    SC64_P6_COMBAT_PROPERTY_INVALID = 0,
    SC64_P6_COMBAT_PROPERTY_APPLIED = 1,
    SC64_P6_TRAP_DAMAGE_SOURCE = -1,
    SC64_P6_WALL_TRAP_COOLDOWN_GBA_TICKS = 28
};

static int
race_p6_combat_raw_unit(const Unit *q)
{
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
}

static int
race_p6_combat_raw_building(const Building *q)
{
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
}

static int
race_p6_combat_common_bounds(int raw, int *left, int *up, int *right, int *down)
{
    /* Source UnitType collision extents for common campaign actors used by
     * P6. Special campaign actors are taken from their pinned fact tables. */
    switch (raw) {
    case 0: case 15: case 20: case 34: return *left=8,*up=9,*right=8,*down=10,1;
    case 1: case 16: case 51: return *left=7,*up=10,*right=7,*down=11,1;
    case 2: case 3: case 19: case 30: case 83: case 89: case 90: case 95:
        return *left=16,*up=16,*right=15,*down=15,1;
    case 7: case 41: case 64: return *left=11,*up=11,*right=11,*down=11,1;
    case 9: return *left=32,*up=33,*right=32,*down=16,1;
    case 11: return *left=24,*up=16,*right=24,*down=20,1;
    case 12: case 29: return *left=37,*up=29,*right=37,*down=29,1;
    case 13: return *left=7,*up=7,*right=7,*down=7,1;
    case 32: return *left=11,*up=7,*right=11,*down=14,1;
    case 37: return *left=8,*up=4,*right=7,*down=11,1;
    case 38: case 53: return *left=10,*up=10,*right=10,*down=12,1;
    case 42: return *left=25,*up=25,*right=24,*down=24,1;
    case 43: return *left=22,*up=22,*right=21,*down=21,1;
    case 45: return *left=24,*up=24,*right=23,*down=23,1;
    case 47: return *left=12,*up=12,*right=11,*down=11,1;
    case 65: case 77: return *left=11,*up=5,*right=11,*down=13,1;
    case 66: case 78: return *left=15,*up=15,*right=16,*down=16,1;
    case 67: case 79: return *left=12,*up=10,*right=11,*down=13,1;
    case 69: return *left=20,*up=16,*right=19,*down=15,1;
    case 70: return *left=18,*up=16,*right=17,*down=15,1;
    case 74: case 75: return *left=12,*up=6,*right=11,*down=19,1;
    case 84: return *left=16,*up=16,*right=15,*down=15,1;
    case 101: return *left=13,*up=13,*right=13,*down=17,1;
    default: return 0;
    }
}

static int
race_p6_combat_unit_box(const Unit *q, Sc64Z5DetectBox *box)
{
    int raw= race_p6_combat_raw_unit(q), l=8,u0=8,r=8,d=8,ok=0;
    const Sc64P6AddedActorFact *p6=sc64_p6_added_actor_fact(raw);
    const Sc64ZpActorFact *zp=sc64_zp_actor_fact(q->type);
    const Sc64Z5ActorFact *z5=sc64_z5_actor_fact(raw);
    if (!box || q->x < 0 || q->y < 0) return 0;
    if (p6) { l=p6->left;u0=p6->up;r=p6->right;d=p6->down;ok=1; }
    else if (zp) ok=sc64_zp_actor_bounds_source(q->type,&l,&u0,&r,&d);
    else if (z5) { l=z5->left;u0=z5->up;r=z5->right;d=z5->down;ok=1; }
#ifdef SC64_T4_RUNTIME
    else ok=sc64_race_t4_bounds_source(q->type,&l,&u0,&r,&d);
#endif
    if (!ok) ok=race_p6_combat_common_bounds(raw,&l,&u0,&r,&d);
    if (!ok) return 0;
    box->x=q->x*2;box->y=q->y*2;box->left=l;box->up=u0;box->right=r;box->down=d;
    return sc64_z5_detect_box_valid(box);
}

static int
race_p6_combat_building_box(const Building *q, Sc64Z5DetectBox *box)
{
    int raw=race_p6_combat_raw_building(q),l=8,u0=8,r=8,d=8,ok=0;
    const Sc64P6AddedActorFact *p6=sc64_p6_added_actor_fact(raw);
    const Sc64Z5BuildingFact *z5=sc64_z5_building_fact(raw);
    const Sc64ZpBuildingFact *zp=sc64_zp_building_fact(q->type);
    if (!box || q->x < 0 || q->y < 0) return 0;
    if (p6) { l=p6->left;u0=p6->up;r=p6->right;d=p6->down;ok=1; }
    else if (z5) { l=z5->left;u0=z5->up;r=z5->right;d=z5->down;ok=1; }
    else if (zp) ok=sc64_zp_building_bounds_source(q->type,&l,&u0,&r,&d);
#ifdef SC64_T4_RUNTIME
    else ok=sc64_race_t4_building_bounds_source(q->type,&l,&u0,&r,&d);
#endif
    if (!ok) {
        /* Authenticated common structure bounds used by the neighboring Z5
         * world adapter; needed because splash/range uses source extents. */
        switch(raw) {
        case 106:l=58;u0=41;r=58;d=41;ok=1;break;
        case 109:l=38;u0=22;r=38;d=26;ok=1;break;
        case 110:l=56;u0=32;r=56;d=31;ok=1;break;
        case 111:l=48;u0=40;r=56;d=32;ok=1;break;
        case 112:l=40;u0=32;r=44;d=24;ok=1;break;
        case 113:l=56;u0=40;r=56;d=40;ok=1;break;
        case 114:l=48;u0=40;r=48;d=38;ok=1;break;
        case 122:l=48;u0=32;r=48;d=28;ok=1;break;
        case 124:l=16;u0=32;r=16;d=16;ok=1;break;
        case 125:l=32;u0=24;r=32;d=16;ok=1;break;
        case 130:l=58;u0=41;r=58;d=41;ok=1;break;
        case 131:case 132:case 133:l=49;u0=32;r=49;d=32;ok=1;break;
        case 135:l=40;u0=32;r=40;d=24;ok=1;break;
        case 137:case 141:l=28;u0=32;r=28;d=24;ok=1;break;
        case 139:l=44;u0=32;r=32;d=20;ok=1;break;
        case 142:l=36;u0=28;r=40;d=18;ok=1;break;
        case 143:case 144:case 146:l=24;u0=24;r=23;d=23;ok=1;break;
        case 149:l=64;u0=32;r=63;d=31;ok=1;break;
        case 150:l=32;u0=32;r=31;d=31;ok=1;break;
        case 151:l=40;u0=32;r=32;d=31;ok=1;break;
        case 154:l=56;u0=39;r=56;d=39;ok=1;break;
        case 155:l=36;u0=16;r=40;d=20;ok=1;break;
        case 156:l=16;u0=12;r=16;d=20;ok=1;break;
        case 157:l=48;u0=32;r=48;d=24;ok=1;break;
        case 160:l=48;u0=32;r=48;d=40;ok=1;break;
        case 162:l=20;u0=16;r=20;d=16;ok=1;break;
        case 164:l=40;u0=24;r=40;d=24;ok=1;break;
        case 166:l=36;u0=24;r=36;d=20;ok=1;break;
        case 167:l=48;u0=40;r=48;d=32;ok=1;break;
        default: break;
        }
    }
    if (!ok) return 0;
    box->x=q->x*2;box->y=q->y*2;box->left=l;box->up=u0;box->right=r;box->down=d;
    return sc64_z5_detect_box_valid(box);
}

static int
race_p6_combat_unit_detector(int raw)
{ return raw == 9 || raw == 42 || raw == 84; }

static int
race_p6_combat_detector_sight_source_px(int raw)
{
    const Sc64P6AddedActorFact *p6=sc64_p6_added_actor_fact(raw);
    const Sc64Z5BuildingFact *z5=sc64_z5_building_fact(raw);
    if (p6 && (p6->flags & SC64_P6_SOURCE_DETECTOR)) return p6->sight_tiles*32;
    if (z5 && (z5->flags & SC64_Z5_SOURCE_DETECTOR)) return z5->sight_px;
    switch(raw) { case 9:return 320; case 42:return 288; case 84:return 288; default:return 0; }
}

static int
race_p6_combat_box_within(const Sc64Z5DetectBox *a,
                          const Sc64Z5DetectBox *b, unsigned range)
{ return sc64_z5_detect_box_valid(a) && sc64_z5_detect_box_valid(b) &&
         sc64_z5_box_distance(a,b)<=range; }

static int
race_p6_combat_owner_detects(int owner, const Sc64Z5DetectBox *target)
{
    Sc64Z5DetectBox detector;
    if (owner < SIDE || owner > 4 || !sc64_z5_detect_box_valid(target)) return 0;
    for (int i=0;i<MAX_UNITS;++i) {
        int raw,sight;
        if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
        raw=race_p6_combat_raw_unit(&u[i]);
        if (!race_p6_combat_unit_detector(raw) ||
            !(sight=race_p6_combat_detector_sight_source_px(raw)) ||
            race_p6_owners_hostile(owner,u[i].owner) ||
            !race_p6_combat_unit_box(&u[i],&detector)) continue;
        if (race_p6_combat_box_within(target,&detector,(unsigned)sight)) return 1;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
        int raw,sight;
        const Sc64Z5BuildingFact *z5;
        if (!b[i].alive || !race_p6_doodad_slot_bound(i) ||
            !race_p6_doodad_enabled_slot(i)) continue;
        raw=race_p6_combat_raw_building(&b[i]);
        z5=sc64_z5_building_fact(raw);
        if (z5) sight=(z5->flags&SC64_Z5_SOURCE_DETECTOR)?z5->sight_px:0;
        else sight=race_p6_combat_detector_sight_source_px(raw);
        if (!sight || (raw!=203 && raw!=209 && raw!=211 && raw!=213 &&
                       raw!=SC64_P6_RAW_WALL_MISSILE_TRAP) ||
            race_p6_owners_hostile(owner,b[i].owner) ||
            !race_p6_combat_building_box(&b[i],&detector)) continue;
        if (race_p6_combat_box_within(target,&detector,(unsigned)sight)) return 1;
    }
    return 0;
}

/* The caller invokes this from race_can_attack before testing the standard
 * scenario flags. Other targets retain their old behavior. */
static int __attribute__((noinline))
race_p6_combat_target_detected(int target, int viewer_owner)
{
    int raw, owner;
    Sc64Z5DetectBox target_box;
    if (!race_p6_active() || target < 0 ||
        target >= MAX_UNITS + MAX_BUILDINGS) return 0;
    if (target < MAX_UNITS) {
        raw = race_p6_combat_raw_unit(&u[target]);
        if (!sc64_p6_requires_detector(raw)) return 1;
        owner=u[target].owner;
        if (!race_p6_combat_unit_box(&u[target],&target_box)) return 0;
    } else {
        Building *q = &b[target - MAX_UNITS];
        raw = race_p6_combat_raw_building(q);
        if (!sc64_p6_requires_detector(raw)) return 1;
        owner=q->owner;
        if (!race_p6_combat_building_box(q,&target_box)) return 0;
    }
    if (viewer_owner==owner) return 1;
    return race_p6_combat_owner_detects(viewer_owner,&target_box);
}

static int
race_p6_combat_unit_targetable(int target, int viewer_owner)
{
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return 0;
    if (target < MAX_UNITS && (!u[target].alive ||
        race_p6_combat_unit_burrowed(target))) return 0;
    if (target >= MAX_UNITS && !b[target - MAX_UNITS].alive) return 0;
    return race_p6_combat_target_detected(target, viewer_owner);
}

/* P6 uses the verified T6 adaptation of order 117: source-created Zerg actors
 * start in the persisted burrow flag. GBA movement/combat/render hooks must
 * consult race_p6_combat_unit_burrowed() and the release tick below. */
static int
race_p6_apply_source_properties(int unit_slot,
                                const Sc64P6WorldUnitProperties *properties)
{
    if (!race_p6_active() || !properties || unit_slot < 0 ||
        unit_slot >= MAX_UNITS || !u[unit_slot].alive ||
        properties->valid_special != 0x1a || properties->valid_fields != 2 ||
        properties->hp_percent != 100 || properties->special_value != 2 ||
        properties->requested_order != 117) return SC64_P6_COMBAT_PROPERTY_INVALID;
    int raw = race_p6_combat_raw_unit(&u[unit_slot]);
    if ((raw != 37 && raw != 38 && raw != SC64_P6_RAW_INFESTED_TERRAN) ||
        u[unit_slot].maxhp <= 0) return SC64_P6_COMBAT_PROPERTY_INVALID;
    u[unit_slot].hp = u[unit_slot].maxhp;
    u[unit_slot].flags |= 2u;
    u[unit_slot].order = ORDER_IDLE;
    u[unit_slot].target = -1;
    return SC64_P6_COMBAT_PROPERTY_APPLIED;
}

/* Candidate-save rule for state owned by this combat adapter. Raw 74/75
 * cloak is intrinsic; order 117 is persisted as a buried idle actor; raw210
 * cooldown is bounded in shield_remainder. */
static int
race_p6_combat_saved_actor_valid(int raw, unsigned flags, int order,
                                 int target, unsigned shield_remainder)
{
    if ((raw==37 || raw==38 || raw==SC64_P6_RAW_INFESTED_TERRAN) &&
        (flags&2u)) return order==ORDER_IDLE && target<0;
    if (raw==SC64_P6_RAW_WALL_MISSILE_TRAP)
        return shield_remainder<=SC64_P6_WALL_TRAP_COOLDOWN_GBA_TICKS;
    return 1;
}

static int
race_p6_combat_implemented_capabilities(int raw)
{
    if (!race_p6_active()) return 0;
    if (raw == 37 || raw == 38)
        return SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY;
    if (raw == SC64_P6_RAW_INFESTED_TERRAN)
        return SC64_P6_POLICY_NEEDS_ORDER_117_PROPERTY |
               SC64_P6_POLICY_NEEDS_SUICIDE_EXPLOSION;
    if (raw == SC64_P6_RAW_HERO_DARK_TEMPLAR || raw == SC64_P6_RAW_ZERATUL)
        return SC64_P6_POLICY_NEEDS_PERMANENT_CLOAK_AND_DETECTORS;
    if (raw == SC64_P6_RAW_WALL_MISSILE_TRAP)
        return SC64_P6_POLICY_NEEDS_WALL_TRAP_ART_AND_RUNTIME;
    return 0;
}

static int
race_p6_combat_target_size(int raw, int type)
{
    const Sc64P6AddedActorFact *p6 = sc64_p6_added_actor_fact(raw);
    const Sc64ZpActorFact *zp = sc64_race_unit_from_raw(raw) == type ?
                              sc64_zp_actor_fact(type) : 0;
    if (p6) return p6->size;
    if (zp) return zp->size;
    if (sc64_race_building_from_raw(raw) == type) {
        const Sc64Z5BuildingFact *prop = sc64_z5_building_fact(raw);
        if (prop) return prop->size;
        const Sc64ZpBuildingFact *building = sc64_zp_building_fact(type);
        if (building && building->raw == raw) return building->size;
    }
    /* P6 does not compile the Terran T4 runtime. Resolve its ordinary UNIT
     * roster explicitly in this module's normalized size encoding. */
    switch (raw) {
    case 0: case 1: case 32: case 37: case 65: case 79: case 84:
        return SC64_P6_SIZE_SMALL;
    case 38: return SC64_P6_SIZE_MEDIUM;
    case 9: case 42: case 66: return SC64_P6_SIZE_LARGE;
    case 101: case 195: case 196: return SC64_P6_SIZE_INDEPENDENT;
    default: break;
    }
#ifdef SC64_TERRAN_LATE_ACTORS
    {
        const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
        if (late) return late->size;
    }
#endif
    /* Convert the older helper's BWAPI ordinals to this module's normalized
     * Small=0, Medium=1, Large=2, Independent=3 representation. */
    switch(raw) {
    case 0:case 1:case 7:case 13:case 15:case 16:case 20:case 32:case 34:
    case 37:case 41:case 43:case 47:case 64:case 65:case 77:case 79:
    case 89:case 90:case 95: return SC64_P6_SIZE_SMALL;
    case 2:case 19:case 38:case 45:case 53: return SC64_P6_SIZE_MEDIUM;
    case 3:case 5:case 8:case 11:case 30:case 42:case 66:case 70:case 78:
    case 83:case 106:case 109:case 110:case 111:case 112:case 113:case 114:
    case 124:case 125:case 130:case 131:case 132:case 133:case 135:case 137:
    case 139:case 141:case 142:case 143:case 144:case 146:case 149:case 150:
    case 151:case 154:case 155:case 156:case 157:case 160:case 162:case 164:
    case 166:case 167:case 203:case 209:case 210:case 211:case 213:
        return SC64_P6_SIZE_LARGE;
    case 195:case 196:case 218: return SC64_P6_SIZE_INDEPENDENT;
    default: break;
    }
    return SC64_P6_SIZE_LARGE;
}

static int __attribute__((noinline))
race_p6_combat_unit_burrowed(int slot)
{
    int raw;
    if (!race_p6_active() || slot < 0 || slot >= MAX_UNITS || !u[slot].alive)
        return 0;
    raw = race_p6_combat_raw_unit(&u[slot]);
    return (raw == 37 || raw == 38 || raw == SC64_P6_RAW_INFESTED_TERRAN) &&
           (u[slot].flags & 2u) != 0;
}

/* Bounded 30-Hz adaptation of the verified order-117 burrow request: scripted
 * Zerg actors begin concealed/idle, then clear the persisted bit and join the
 * ordinary local attack path when a hostile comes within 96 GBA pixels. Native
 * animation and exact command timing are not represented. */
static int __attribute__((noinline))
race_p6_combat_burrow_release_tick(void)
{
    int released = 0;
    if (!race_p6_active()) return 0;
    for (int i=0;i<MAX_UNITS;++i) {
        Unit *q=&u[i];
        if (!race_p6_combat_unit_burrowed(i)) continue;
        for (int j=0;j<MAX_UNITS;++j) {
            if (i==j || !u[j].alive ||
                !race_p6_owners_hostile(q->owner,u[j].owner)) continue;
            if (dist2(q->x,q->y,u[j].x,u[j].y) <= 96*96) {
                q->flags &= (u16)~2u;
                q->order = ORDER_ATTACK_MOVE;
                q->target = -1;
                ++released;
                break;
            }
        }
    }
    return released;
}

static int
race_p6_combat_target_is_invincible(int target)
{
    if (target < MAX_UNITS) {
        return (u[target].flags & 16) || invincible_units[target] ||
            sc64_game_unit_has_flag(u[target].type, SC64_GAME_ACTOR_HIDDEN) ||
            sc64_game_unit_has_flag(u[target].type, SC64_GAME_ACTOR_INVINCIBLE);
    }
    target -= MAX_UNITS;
    return (b[target].flags & 16) || invincible_buildings[target] ||
           sc64_game_building_has_flag(b[target].type, SC64_GAME_ACTOR_INVINCIBLE);
}

/* Apply an Explosive packet with source size modifiers, then route its
 * adjusted integer packet through hit_target so the existing death, supply,
 * transport, and campaign-accounting paths remain authoritative. Q8 source
 * damage is rounded down into this engine's integer HP; a nonzero sub-HP hit
 * is represented by its existing one-HP minimum. */
static int
race_p6_combat_apply_explosive(int target, unsigned damage, unsigned divisor,
                               int source_type, int source_owner)
{
    Sc64P6DamageTarget model;
    Sc64P6DamageResult result;
    int raw, type, hp, shield, armor, has_shields;
    int hp_damage, shield_damage, adjusted;
    if (!race_p6_active() || target < 0 ||
        target >= MAX_UNITS + MAX_BUILDINGS || race_p6_combat_target_is_invincible(target))
        return 0;
    if (target < MAX_UNITS) {
        Unit *q = &u[target];
        if (!q->alive || sc64_race_unit_flying(q->type) ||
            SC64_TRANSPORT_LOADED(q)) return 0;
        raw = race_p6_combat_raw_unit(q); type = q->type;
        hp = q->hp; shield = q->shield;
        armor = q->armor + (q->owner == SIDE ? armor_level : 0);
        has_shields = shield > 0;
    } else {
        Building *q = &b[target - MAX_UNITS];
        if (!q->alive) return 0;
        raw = race_p6_combat_raw_building(q); type = q->type;
        hp = q->hp; shield = q->shield;
        armor = sc64_race_building_armor(q->type);
        has_shields = shield > 0;
    }
    if (hp <= 0 || shield < 0 || armor < 0) return 0;
    model.hp_q8 = (u32)hp * 256u;
    model.shields_q8 = (u32)shield * 256u;
    model.armor_q8 = (u32)armor * 256u;
    model.shield_armor_q8 = 0;
    model.size = (u8)race_p6_combat_target_size(raw, type);
    model.has_shields = (u8)has_shields;
    model.invincible = 0;
    model.ground_target = 1;
    if (!sc64_p6_explosive_damage_q8(&model, damage, divisor, &result)) return 0;
    shield_damage = (int)(result.shield_damage_q8 >> 8);
    hp_damage = (int)(result.hp_damage_q8 >> 8);
    if (result.hp_damage_q8 && !hp_damage) hp_damage = 1;
    if (hp_damage > hp) hp_damage = hp;
    if (!shield_damage && !hp_damage) return 0;
    if (hp_damage == 0) adjusted = shield_damage;
    else adjusted = shield_damage + hp_damage + armor;
    if (adjusted < 1) adjusted = 1;
    hit_target(target, adjusted, source_type, source_owner);
    return 1;
}

static int
race_p6_combat_box_for_target(int target, Sc64Z5DetectBox *box)
{
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return 0;
    if (target < MAX_UNITS) return race_p6_combat_unit_box(&u[target],box);
    return race_p6_combat_building_box(&b[target-MAX_UNITS],box);
}

static unsigned
race_p6_combat_distance_to_target_box(int x, int y,
                                     const Sc64Z5DetectBox *target)
{
    Sc64Z5DetectBox point;
    point.x=x;point.y=y;point.left=point.up=point.right=point.down=0;
    if (!sc64_z5_detect_box_valid(&point) || !sc64_z5_detect_box_valid(target))
        return 0xffffffffu;
    return sc64_z5_box_distance(&point,target);
}

/* Called on the source suicide order event, not merely when selecting a
 * target. The caller still owns the authored order/iscript signal that
 * decides when this event occurs. */
static int
race_p6_combat_infested_explode(int slot)
{
    Unit *source;
    int cx, cy, source_owner, hit_count = 0;
    if (!race_p6_active() || slot < 0 || slot >= MAX_UNITS) return 0;
    source = &u[slot];
    if (!source->alive || race_p6_combat_raw_unit(source) !=
        SC64_P6_RAW_INFESTED_TERRAN) return 0;
    cx = source->x * 2; cy = source->y * 2; source_owner = source->owner;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Sc64Z5DetectBox target_box;
        unsigned dist;
        int burrowed=0;
        if (i == slot || !u[i].alive || sc64_race_unit_flying(u[i].type) ||
            SC64_TRANSPORT_LOADED(&u[i]) || !race_p6_combat_box_for_target(i,&target_box))
            continue;
        dist = race_p6_combat_distance_to_target_box(cx,cy,&target_box);
        if (dist > SC64_P6_EXPLOSION_OUTER_SOURCE_PX) continue;
        burrowed = race_p6_combat_unit_burrowed(i);
        if (!sc64_p6_infested_burrowed_targetable(dist,burrowed)) continue;
        if (race_p6_combat_apply_explosive(i,SC64_P6_EXPLOSION_DAMAGE,
                dist<=SC64_P6_EXPLOSION_INNER_SOURCE_PX?1u:
                dist<=SC64_P6_EXPLOSION_MIDDLE_SOURCE_PX?2u:4u,
                U_P6_INFESTED_TERRAN,source_owner)) ++hit_count;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int target=MAX_UNITS+i;
        Sc64Z5DetectBox target_box;
        unsigned dist;
        if (!b[i].alive || !race_p6_combat_box_for_target(target,&target_box)) continue;
        dist=race_p6_combat_distance_to_target_box(cx,cy,&target_box);
        if (dist>SC64_P6_EXPLOSION_OUTER_SOURCE_PX) continue;
        if(race_p6_combat_apply_explosive(target,SC64_P6_EXPLOSION_DAMAGE,
             dist<=SC64_P6_EXPLOSION_INNER_SOURCE_PX?1u:
             dist<=SC64_P6_EXPLOSION_MIDDLE_SOURCE_PX?2u:4u,
             U_P6_INFESTED_TERRAN,source_owner))++hit_count;
    }
    race_p6_note_unit_death(source);
    if (source->owner == SIDE) supply_used -= unit_supply(source->type);
    source->alive = 0; source->selected = 0; source->target = -1;
    source->order = ORDER_IDLE;
#ifdef SC64_RACES
    sc64_target_index_active = 0;
#endif
    return hit_count + 1;
}

/* Runtime entry for raw50's normal attack order. Returns handled for every
 * active Infested Terran, including malformed/stale targets, so generic
 * zero-damage logic cannot leave it inert. Contact is source-box distance;
 * approach reuses the engine's ordinary movement command. */
static int __attribute__((noinline))
race_p6_combat_infested_attack(Unit *q, int target)
{
    int slot;
    Sc64Z5DetectBox attacker_box,target_box;
    if (!race_p6_active() || !q || !q->alive ||
        race_p6_combat_raw_unit(q)!=SC64_P6_RAW_INFESTED_TERRAN) return 0;
    slot=(int)(q-u);
    if (slot<0 || slot>=MAX_UNITS || target<0 ||
        target>=MAX_UNITS+MAX_BUILDINGS || !target_alive(target)) return 1;
    if (race_p6_combat_unit_burrowed(slot)) return 1;
    int target_owner=target<MAX_UNITS?u[target].owner:b[target-MAX_UNITS].owner;
    if (!race_p6_owners_hostile(q->owner,target_owner) ||
        (target<MAX_UNITS && sc64_race_unit_flying(u[target].type)) ||
        race_p6_combat_target_is_invincible(target) ||
        !race_p6_combat_target_detected(target,q->owner) ||
        !race_p6_combat_unit_box(q,&attacker_box) ||
        !race_p6_combat_box_for_target(target,&target_box)) return 1;
    if (sc64_z5_box_distance(&attacker_box,&target_box)<=3u)
        (void)race_p6_combat_infested_explode(slot);
    else
        move_toward(q,target_x(target),target_y(target));
    return 1;
}

/* One bounded per-tick autonomous attack for each enabled raw210 prop. Trap
 * cooldown is persisted in the unused shield_remainder field (the trap has
 * no shields). */
static int __attribute__((noinline))
race_p6_combat_tick_traps(void)
{
    int fired = 0;
    if (!race_p6_active()) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *trap = &b[i];
        int best = -1;
        unsigned best_dist = SC64_P6_WALL_TRAP_RANGE_SOURCE_PX + 1u;
        if (!trap->alive || race_p6_combat_raw_building(trap) !=
            SC64_P6_RAW_WALL_MISSILE_TRAP || !race_p6_doodad_slot_bound(i) ||
            !race_p6_doodad_enabled_slot(i)) continue;
        if (trap->shield_remainder && --trap->shield_remainder) continue;
        int cx=trap->x*2, cy=trap->y*2;
        Sc64Z5DetectBox trap_box;
        if (!race_p6_combat_building_box(trap,&trap_box)) continue;
        for (int j=0;j<MAX_UNITS+MAX_BUILDINGS;++j) {
            int owner,flying=0,raw;
            unsigned dist;
            Sc64Z5DetectBox target_box;
            if (j==MAX_UNITS+i) continue;
            if (j<MAX_UNITS) {
                Unit *q=&u[j];
                if (!q->alive || SC64_TRANSPORT_LOADED(q) ||
                    race_p6_combat_unit_burrowed(j)) continue;
                owner=q->owner; raw=race_p6_combat_raw_unit(q);
                flying=sc64_race_unit_flying(q->type);
            } else {
                Building *q=&b[j-MAX_UNITS];
                if (!q->alive) continue;
                owner=q->owner; raw=race_p6_combat_raw_building(q);
            }
            if (race_p6_owners_hostile(trap->owner,owner)==0 || flying ||
                race_p6_combat_target_is_invincible(j) ||
                !race_p6_combat_target_detected(j,trap->owner) ||
                !race_p6_combat_box_for_target(j,&target_box)) continue;
            dist=sc64_z5_box_distance(&trap_box,&target_box);
            if (dist>SC64_P6_WALL_TRAP_RANGE_SOURCE_PX) continue;
            (void)raw; (void)cx; (void)cy;
            if (dist<best_dist || (dist==best_dist && best>=0 && j<best)) {
                best=j; best_dist=dist;
            }
        }
        if (best<0) continue;
        if (race_p6_combat_apply_explosive(best,SC64_P6_WALL_TRAP_DAMAGE,1u,
                SC64_P6_TRAP_DAMAGE_SOURCE,trap->owner)) {
            trap->shield_remainder=SC64_P6_WALL_TRAP_COOLDOWN_GBA_TICKS;
            ++fired;
        }
    }
    return fired;
}

#endif /* SC64_P6_COMBAT_ENABLED */
#endif
