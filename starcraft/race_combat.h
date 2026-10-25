#ifndef SC64_RACE_COMBAT_H
#define SC64_RACE_COMBAT_H
/* Original ground/air weapon eligibility; render coordinates are half of
   the reference's pixels. Scenario invincibility is CHK UNIT flag 0x10. */
#ifdef SC64_Z6_RUNTIME
/* One shared ROM eligibility routine avoids repeating the growing source
 * actor/cloak policy in every hot query and preserves the IWRAM reserve. */
static int __attribute__((noinline))
#else
static int
#endif
race_can_attack(int attacker,int target) {
    if(target<0||target>=MAX_UNITS+MAX_BUILDINGS)return 0;
    if(target<MAX_UNITS&&SC64_TRANSPORT_LOADED(&u[target]))return 0;
#ifdef SC64_Z5_CLOAK_ENABLED
    if (!race_z5_cloaked_target_attackable(target)) return 0;
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (!race_z6_cloaked_target_attackable(target)) return 0;
#endif
#ifdef SC64_T4_RUNTIME
    if(target<MAX_UNITS) {
        if((sc64_race_t4_actor_flags(u[target].type)&SC64_T4_ACTOR_HIDDEN)||
           sc64_race_t4_unit_intrinsically_invincible(u[target].type))return 0;
    } else if(sc64_race_t4_building_intrinsically_invincible(b[target-MAX_UNITS].type))return 0;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if(target<MAX_UNITS) {
        unsigned actor = sc64_game_unit_actor_flags(u[target].type);
        if(actor & SC64_GAME_ACTOR_HIDDEN) return 0;
    } else {
        unsigned actor = sc64_game_building_actor_flags(b[target-MAX_UNITS].type);
        if(actor & SC64_GAME_ACTOR_INVINCIBLE) return 0;
    }
#endif
    if(target<MAX_UNITS) {
        if(u[target].flags&16)return 0;
        if(!race_mode&&race_selected_mission==1&&u[target].type==U_CHRYSALIS&&race_campaign.carrier_unit<MAX_UNITS)return 0;
        if(sc64_race_unit_flying(u[target].type)) {
            return sc64_race_unit_can_attack_air(attacker);
        }
    } else if((b[target-MAX_UNITS].flags&16)||b[target-MAX_UNITS].type==B_ZERG_BEACON)return 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if(target>=MAX_UNITS&&race_late_airborne(target-MAX_UNITS))
        return attacker<0||sc64_race_unit_can_attack_air(attacker);
#endif
    return attacker<0||sc64_race_unit_damage(attacker)>0;
}
#if defined(SC64_P3_DEFENSE_TARGETS) && !defined(SC64_P3_RUNTIME)
#error "SC64_P3_DEFENSE_TARGETS requires SC64_P3_RUNTIME"
#endif
#if defined(SC64_P3_DEFENSE_TARGETS) && !defined(SC64_UNIFIED)
#error "SC64_P3_DEFENSE_TARGETS requires SC64_UNIFIED"
#endif
#if defined(SC64_ZP_DEFENSE_TARGETS) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
#error "SC64_ZP_DEFENSE_TARGETS requires SC64_ZP_CAMPAIGN_STORAGE"
#endif
#ifdef SC64_ZP_DEFENSE_TARGETS
_Static_assert(MAX_UNITS < 255,
               "Z/P defense lists reserve 0xff as the end marker");
#endif

#if defined(SC64_P3_DEFENSE_TARGETS) || defined(SC64_TERRAN_LATE_RUNTIME) || \
    defined(SC64_ZP_DEFENSE_TARGETS)
/* Keep the original full scan as both the fallback for every other mission
 * and a differential oracle for candidate-list paths. */
#ifdef SC64_TERRAN_LATE_RUNTIME
static __attribute__((noinline)) void race_tick_defenses_fullscan(void) {
#elif defined(SC64_ZP_DEFENSE_TARGETS)
/* Keep this fallback out of IWRAM in the larger Z/P storage profiles. */
static __attribute__((noinline)) void race_tick_defenses_fullscan(void) {
#else
static void GBA_HOT race_tick_defenses_fullscan(void) {
#endif
#else
static void GBA_HOT race_tick_defenses(void) {
#endif
    for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i)) {
        Building *q=&b[i];int ground=0,air=0,range=0,damage=0,cooldown=0;
        switch(q->type) {
        case B_PHOTON_CANNON:
            if(!race_site_powered(q->owner,q->x,q->y))continue;
            ground=air=1;range=112;damage=20;cooldown=28;break;
        case B_SUNKEN_COLONY:ground=1;range=112;damage=40;cooldown=40;break;
        case B_SPORE_COLONY:air=1;range=112;damage=15;cooldown=19;break;
        case B_MISSILE_TURRET:air=1;range=112;damage=20;cooldown=19;break;
        default:continue;
        }
        if(q->queue_progress>0){q->queue_progress--;continue;}
        int best=-1,bestd=range*range;
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive) {
            /* Reject candidates outside the circular weapon radius with the
             * cheaper axis bounds before ownership, flags, and unit-class
             * queries. Equality is out of range because the exact test below
             * is strict (distance < range squared). */
            if(FAST_ABS(q->x-u[j].x)>=range||FAST_ABS(q->y-u[j].y)>=range)continue;
            if(!owners_hostile(q->owner,u[j].owner)||(u[j].flags&16))continue;
            int flying=sc64_race_unit_flying(u[j].type);
            if((flying&&!air)||(!flying&&!ground)||
               !SC64_CAN_ATTACK_FROM_SOURCE(-1,j,q->owner,q->raw_owner))continue;
            int d=dist2(q->x,q->y,u[j].x,u[j].y);
            if(d<bestd){bestd=d;best=j;}
        }
        if(best>=0){hit_stock_building_target(q,best,damage);
            add_tracer(q->x,q->y,u[best].x,u[best].y,sc64_weapon_vfx_building_profile(q->type));
            q->queue_progress=cooldown;}
    }
}

#ifdef SC64_ZP_DEFENSE_TARGETS
/* The mission tick runs before defenses and may change owners or alliances.
 * Build fresh owner/class lists here, after those effects, instead of reusing
 * the lists prepared before the mission tick by update_orders. */
static int GBA_HOT __attribute__((noinline))
race_defense_target_from_zp_lists(const Building *q, int ground, int air,
                                  int range, const u8 head[5][2],
                                  const u8 next[MAX_UNITS], u8 hostile_mask)
{
    int best = -1, bestd = range * range;
    for (int owner = 0; owner < 5; ++owner) {
        if (!(hostile_mask & (1u << owner))) continue;
        for (int flying = 0; flying < 2; ++flying) {
            if ((flying && !air) || (!flying && !ground)) continue;
            for (int j = head[owner][flying]; j != 0xff; j = next[j]) {
                if (!u[j].alive || u[j].owner != owner) continue;
                if (FAST_ABS(q->x - u[j].x) >= range ||
                    FAST_ABS(q->y - u[j].y) >= range || (u[j].flags & 16) ||
                    !SC64_CAN_ATTACK_FROM_SOURCE(-1, j, q->owner,
                                                 q->raw_owner)) continue;
                int d = dist2(q->x, q->y, u[j].x, u[j].y);
                /* The original ascending scan retains the lowest slot on ties. */
                if (d < bestd || (best >= 0 && d == bestd && j < best)) {
                    bestd = d;
                    best = j;
                }
            }
        }
    }
    return best;
}

static __attribute__((noinline)) void race_tick_defenses_zp_targets(void)
{
    u8 head[5][2], next[MAX_UNITS], hostile_mask[5];

    /* Invalid ownership is tolerated by the old scan. Decide to fall back
     * before any cooldown or health mutation, so the result stays exact. */
    for (int j = 0; j < MAX_UNITS; ++j) {
        int owner = u[j].owner;
        if (u[j].alive && (owner < 0 || owner >= 5)) {
            race_tick_defenses_fullscan();
            return;
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int owner = b[i].owner;
        if (b[i].alive && (owner < 0 || owner >= 5)) {
            race_tick_defenses_fullscan();
            return;
        }
    }

    memset(head, 0xff, sizeof(head));
    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
        int owner = u[j].owner;
        int flying = sc64_race_unit_flying(u[j].type);
        next[j] = head[owner][flying];
        head[owner][flying] = (u8)j;
    }
    for (int owner = 0; owner < 5; ++owner) {
        u8 mask = 0;
        for (int other = 0; other < 5; ++other)
            if (owners_hostile(owner, other)) mask |= (u8)(1u << other);
        hostile_mask[owner] = mask;
    }

    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i)) {
        Building *q = &b[i];
        int ground = 0, air = 0, range = 0, damage = 0, cooldown = 0;
        switch (q->type) {
        case B_PHOTON_CANNON:
            if (!race_site_powered(q->owner, q->x, q->y)) continue;
            ground = air = 1; range = 112; damage = 20; cooldown = 28; break;
        case B_SUNKEN_COLONY: ground = 1; range = 112; damage = 40; cooldown = 40; break;
        case B_SPORE_COLONY: air = 1; range = 112; damage = 15; cooldown = 19; break;
        case B_MISSILE_TURRET: air = 1; range = 112; damage = 20; cooldown = 19; break;
        default: continue;
        }
        if (q->queue_progress > 0) { q->queue_progress--; continue; }
        int best = race_defense_target_from_zp_lists(
            q, ground, air, range, head, next, hostile_mask[q->owner]);
        if (best >= 0) {
            hit_stock_building_target(q,best,damage);
            add_tracer(q->x, q->y, u[best].x, u[best].y, sc64_weapon_vfx_building_profile(q->type));
            q->queue_progress = cooldown;
        }
    }
}
#endif

#ifdef SC64_P3_DEFENSE_TARGETS
/* Higher Ground has two hostile forces (owners 1 and 4) allied against SIDE.
 * Build their candidates once, in unit-slot order, rather than rescanning all
 * MAX_UNITS slots for each of the map's defensive buildings. */
static __attribute__((noinline)) void race_tick_defenses_p3_targets(void) {
    u8 side_targets[MAX_UNITS], force_targets[MAX_UNITS];
    int side_count = 0, force_count = 0;

    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
        if (u[j].owner == SIDE) side_targets[side_count++] = (u8)j;
        else if (u[j].owner == ENEMY || u[j].owner == 4)
            force_targets[force_count++] = (u8)j;
    }

    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i)) {
        Building *q = &b[i];
        int ground = 0, air = 0, range = 0, damage = 0, cooldown = 0;
        switch (q->type) {
        case B_PHOTON_CANNON:
            if (!race_site_powered(q->owner, q->x, q->y)) continue;
            ground = air = 1; range = 112; damage = 20; cooldown = 28; break;
        case B_SUNKEN_COLONY: ground = 1; range = 112; damage = 40; cooldown = 40; break;
        case B_SPORE_COLONY: air = 1; range = 112; damage = 15; cooldown = 19; break;
        case B_MISSILE_TURRET: air = 1; range = 112; damage = 20; cooldown = 19; break;
        default: continue;
        }
        if (q->queue_progress > 0) { q->queue_progress--; continue; }

        const u8 *targets = 0;
        int target_count = 0;
        if (q->owner == SIDE) { targets = force_targets; target_count = force_count; }
        else if (q->owner == ENEMY || q->owner == 4) {
            targets = side_targets; target_count = side_count;
        }
        int best = -1, bestd = range * range;
        for (int k = 0; k < target_count; ++k) {
            int j = targets[k];
            if (!u[j].alive) continue; /* Earlier defenses may have killed it. */
            if (FAST_ABS(q->x - u[j].x) >= range || FAST_ABS(q->y - u[j].y) >= range) continue;
            if (!owners_hostile(q->owner, u[j].owner) || (u[j].flags & 16)) continue;
            int flying = sc64_race_unit_flying(u[j].type);
            if ((flying && !air) || (!flying && !ground) ||
                !SC64_CAN_ATTACK_FROM_SOURCE(-1, j, q->owner,
                                             q->raw_owner)) continue;
            int d = dist2(q->x, q->y, u[j].x, u[j].y);
            if (d < bestd) { bestd = d; best = j; }
        }
        if (best >= 0) {
            hit_stock_building_target(q,best,damage);
            add_tracer(q->x, q->y, u[best].x, u[best].y, sc64_weapon_vfx_building_profile(q->type));
            q->queue_progress = cooldown;
        }
    }
}

static void race_tick_defenses(void) {
    if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID)
        race_tick_defenses_p3_targets();
#ifdef SC64_ZP_DEFENSE_TARGETS
    else if (!race_mode)
        race_tick_defenses_zp_targets();
#endif
    else
        race_tick_defenses_fullscan();
}
#elif defined(SC64_TERRAN_LATE_RUNTIME)
/* Per-tick defense candidate lists reduce the 96-building x 192-unit scan to
 * the owners each defense can actually attack. The links use unit slots, so
 * their byte representation remains valid while MAX_UNITS is 192. */
static int GBA_HOT __attribute__((noinline))
race_defense_target_from_owner_lists(const Building *q, int ground, int air,
                                     int range, const u8 head[5][2],
                                     const u8 next[MAX_UNITS], u8 hostile_mask)
{
    int best = -1, bestd = range * range;
    for (int owner = 0; owner < 5; ++owner) {
        if (!(hostile_mask & (1u << owner))) continue;
        for (int flying = 0; flying < 2; ++flying) {
            if ((flying && !air) || (!flying && !ground)) continue;
            for (int j = head[owner][flying]; j != 0xff; j = next[j]) {
                if (!u[j].alive || u[j].owner != owner) continue;
                if (FAST_ABS(q->x - u[j].x) >= range ||
                    FAST_ABS(q->y - u[j].y) >= range || (u[j].flags & 16) ||
                    !SC64_CAN_ATTACK_FROM_SOURCE(-1, j, q->owner,
                                                 q->raw_owner)) continue;
                int d = dist2(q->x, q->y, u[j].x, u[j].y);
                /* The original ascending full scan keeps the lowest slot on ties. */
                if (d < bestd || (best >= 0 && d == bestd && j < best)) {
                    bestd = d;
                    best = j;
                }
            }
        }
    }
    return best;
}

static __attribute__((noinline)) void race_tick_defenses_late_targets(void)
{
    u8 head[5][2], next[MAX_UNITS], hostile_mask[5];
    memset(head, 0xff, sizeof(head));
    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
        int owner = u[j].owner;
        if (owner < 0 || owner >= 5) continue;
        int flying = sc64_race_unit_flying(u[j].type);
        next[j] = head[owner][flying];
        head[owner][flying] = (u8)j;
    }
    for (int owner = 0; owner < 5; ++owner) {
        u8 mask = 0;
        for (int other = 0; other < 5; ++other)
            if (owners_hostile(owner, other)) mask |= (u8)(1u << other);
        hostile_mask[owner] = mask;
    }

    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i)) {
        Building *q = &b[i];
        int ground = 0, air = 0, range = 0, damage = 0, cooldown = 0;
        switch (q->type) {
        case B_PHOTON_CANNON:
            if (!race_site_powered(q->owner, q->x, q->y)) continue;
            ground = air = 1; range = 112; damage = 20; cooldown = 28; break;
        case B_SUNKEN_COLONY: ground = 1; range = 112; damage = 40; cooldown = 40; break;
        case B_SPORE_COLONY: air = 1; range = 112; damage = 15; cooldown = 19; break;
        case B_MISSILE_TURRET: air = 1; range = 112; damage = 20; cooldown = 19; break;
        default: continue;
        }
        if (q->queue_progress > 0) { q->queue_progress--; continue; }
        int best = q->owner < 5 ?
            race_defense_target_from_owner_lists(q, ground, air, range, head, next,
                                                 hostile_mask[q->owner]) : -1;
        if (best >= 0) {
            hit_stock_building_target(q,best,damage);
            add_tracer(q->x, q->y, u[best].x, u[best].y, sc64_weapon_vfx_building_profile(q->type));
            q->queue_progress = cooldown;
        }
    }
}

static void race_tick_defenses(void)
{
    if (race_late_active()) race_tick_defenses_late_targets();
    else race_tick_defenses_fullscan();
}
#elif defined(SC64_ZP_DEFENSE_TARGETS)
static void race_tick_defenses(void)
{
    if (!race_mode) race_tick_defenses_zp_targets();
    else race_tick_defenses_fullscan();
}
#endif
#endif
