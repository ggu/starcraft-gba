/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUNKER_COMBAT_H
#define SC64_BUNKER_COMBAT_H
/* Included immediately after tick_combat. Coordinates use the existing GBA
 * center/footprint approximation; this is not N64 collision geometry. */
static int __attribute__((noinline))
race_bunker_target_in_range(const Unit *q,int target,int raw)
{
    if (target<0 || target>=MAX_UNITS+MAX_BUILDINGS || !target_alive(target)) return 0;
    int owner=target<MAX_UNITS?u[target].owner:b[target-MAX_UNITS].owner;
    if (!owners_hostile(q->owner,owner)) return 0;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    if (target<MAX_UNITS ? invincible_units[target] : invincible_buildings[target-MAX_UNITS]) return 0;
#endif
    if (!sc64_auto_target_candidate_allowed(q->type,target,q->owner,q->raw_owner)) return 0;
    int flying=target<MAX_UNITS && sc64_race_unit_flying(u[target].type);
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (target>=MAX_UNITS && race_bc_facility_airborne(target-MAX_UNITS)) flying=1;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (target>=MAX_UNITS && race_late_airborne(target-MAX_UNITS)) flying=1;
#endif
    Sc64StockAttack attack={0};
    if (!sc64_stock_attack_select(raw,flying,&attack) ||
        sc64_stock_attack_damage_per_impact(&attack,q->owner==SIDE?weapons_level:0)<=0) return 0;
    int range=(attack.weapon->max_range+1)/2+32;
    int minimum=(attack.weapon->min_range+1)/2;
    if (target>=MAX_UNITS) {range+=18;if (minimum) minimum+=18;}
    int dx=target_x(target)-q->x,dy=target_y(target)-q->y;
    if (FAST_ABS(dx)>range || FAST_ABS(dy)>range) return 0;
    int d=dx*dx+dy*dy;
    return d<=range*range && d>=minimum*minimum;
}
static void __attribute__((noinline)) race_bunker_fire_unit(int unit,int slot)
{
    if (unit<0 || unit>=MAX_UNITS || !race_bunker_carrier_valid(slot)) return;
    Unit *q=&u[unit];
    if (!q->alive || q->order!=ORDER_TRANSPORTED || q->target!=MAX_UNITS+slot || q->cd>0) return;
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    if (!sc64_bunker_firing_raw(raw)) {q->build=-1;return;}
    int cached=q->build;
    if (cached>=0 && cached<240 && race_bunker_target_in_range(q,cached,raw)) {
        tick_combat(q,cached);return;
    }
    q->build=-1;
    /* Derive a safe enclosing bound from both source weapons. Skip expensive
     * visibility/target predicates for distant entities. */
    int bound=0;Sc64StockAttack attack={0};
    for (int air=0;air<2;++air) if (sc64_stock_attack_select(raw,air,&attack)) {
        int r=(attack.weapon->max_range+1)/2+32+18;
        if (r>bound) bound=r;
    }
    if (!bound) return;
    int best=-1,best_distance=0;
    for (int target=0;target<MAX_UNITS+MAX_BUILDINGS;++target) {
        int alive=target<MAX_UNITS?u[target].alive:b[target-MAX_UNITS].alive;
        if (!alive) continue;
        int x=target_x(target),y=target_y(target);
        int dx=x-q->x,dy=y-q->y;
        if (FAST_ABS(dx)>bound || FAST_ABS(dy)>bound) continue;
        int distance=dx*dx+dy*dy;
        if ((best>=0 && distance>=best_distance) || !race_bunker_target_in_range(q,target,raw)) continue;
        best=target;best_distance=distance;
    }
    if (best<0) return;
    /* P7's compact codec accepts only -2..239; larger building target IDs
     * remain valid to fire at, but are deliberately reacquired each shot. */
    if (best<240) q->build=best;
    tick_combat(q,best);
}
#endif
