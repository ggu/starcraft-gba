#ifndef SC64_RACE_COMBAT_H
#define SC64_RACE_COMBAT_H
/* Original ground/air weapon eligibility; render coordinates are half of
   the reference's pixels. Scenario invincibility is CHK UNIT flag 0x10. */
static int race_can_attack(int attacker,int target) {
    if(target<0||target>=MAX_UNITS+MAX_BUILDINGS)return 0;
    if(target<MAX_UNITS) {
        if(u[target].flags&16)return 0;
        if(!race_mode&&race_selected_mission==1&&u[target].type==U_CHRYSALIS&&race_campaign.carrier_unit<MAX_UNITS)return 0;
        if(sc64_race_unit_flying(u[target].type)) {
            switch(attacker) {
            case -1: case U_MARINE: case U_HYDRALISK: case U_HUNTER_KILLER:
            case U_DRAGOON: case U_FENIX_DRAGOON: case U_GOLIATH:
            case U_WRAITH: case U_MUTALISK: case U_SCOUT: return 1;
            default:return 0;
            }
        }
    } else if((b[target-MAX_UNITS].flags&16)||b[target-MAX_UNITS].type==B_ZERG_BEACON)return 0;
    return attacker<0||sc64_race_unit_damage(attacker)>0;
}
static void GBA_HOT race_tick_defenses(void) {
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
#endif
