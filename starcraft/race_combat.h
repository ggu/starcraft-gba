#ifndef SC64_RACE_COMBAT_H
#define SC64_RACE_COMBAT_H
/* Original ground/air weapon eligibility; render coordinates are half of
   the reference's pixels. Scenario invincibility is CHK UNIT flag 0x10. */
static int race_can_attack(int attacker,int target) {
    if(target<0||target>=MAX_UNITS+MAX_BUILDINGS)return 0;
#ifdef SC64_T4_RUNTIME
    if(target<MAX_UNITS) {
        if((sc64_race_t4_actor_flags(u[target].type)&SC64_T4_ACTOR_HIDDEN)||
           sc64_race_t4_unit_intrinsically_invincible(u[target].type))return 0;
    } else if(sc64_race_t4_building_intrinsically_invincible(b[target-MAX_UNITS].type))return 0;
#endif
    if(target<MAX_UNITS) {
        if(u[target].flags&16)return 0;
        if(!race_mode&&race_selected_mission==1&&u[target].type==U_CHRYSALIS&&race_campaign.carrier_unit<MAX_UNITS)return 0;
        if(sc64_race_unit_flying(u[target].type)) {
            return sc64_race_unit_can_attack_air(attacker);
        }
    } else if((b[target-MAX_UNITS].flags&16)||b[target-MAX_UNITS].type==B_ZERG_BEACON)return 0;
    return attacker<0||sc64_race_unit_damage(attacker)>0;
}
#if defined(SC64_P3_DEFENSE_TARGETS) && !defined(SC64_P3_RUNTIME)
#error "SC64_P3_DEFENSE_TARGETS requires SC64_P3_RUNTIME"
#endif
#if defined(SC64_P3_DEFENSE_TARGETS) && !defined(SC64_UNIFIED)
#error "SC64_P3_DEFENSE_TARGETS requires SC64_UNIFIED"
#endif

#ifdef SC64_P3_DEFENSE_TARGETS
/* Keep the original full scan as both the fallback for every other mission
 * and a differential oracle for the P3-only candidate-list path. */
static void GBA_HOT race_tick_defenses_fullscan(void) {
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
            if((flying&&!air)||(!flying&&!ground)||!race_can_attack(-1,j))continue;
            int d=dist2(q->x,q->y,u[j].x,u[j].y);
            if(d<bestd){bestd=d;best=j;}
        }
        if(best>=0){hit_target(best,damage,U_MARINE,q->owner);
            add_tracer(q->x,q->y,u[best].x,u[best].y,owner_color(q->owner));
            q->queue_progress=cooldown;}
    }
}

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
            if ((flying && !air) || (!flying && !ground) || !race_can_attack(-1, j)) continue;
            int d = dist2(q->x, q->y, u[j].x, u[j].y);
            if (d < bestd) { bestd = d; best = j; }
        }
        if (best >= 0) {
            hit_target(best, damage, U_MARINE, q->owner);
            add_tracer(q->x, q->y, u[best].x, u[best].y, owner_color(q->owner));
            q->queue_progress = cooldown;
        }
    }
}

static void race_tick_defenses(void) {
    if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID)
        race_tick_defenses_p3_targets();
    else
        race_tick_defenses_fullscan();
}
#endif
#endif
