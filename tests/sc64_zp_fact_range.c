#include <limits.h>
#include <stdio.h>
#include "race_rules.h"
#include "sc64_zp_actor_rules.h"

/* Stable text output lets the Python driver compare the ordinary resolver
 * with the range-cull build byte for byte, including all public wrappers. */
static void actor(int type)
{
    const Sc64ZpActorFact *f = sc64_zp_actor_fact(type);
    int l=-91,u=-92,r=-93,d=-94;
    int bounds=sc64_zp_actor_bounds_source(type,&l,&u,&r,&d);
    printf("U:%d:",type);
    if (!f) printf("NULL");
    else printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u",
        f->raw,f->hp,f->shields,f->armor,f->minerals,f->gas,f->build_frames,
        f->sight_px,f->left,f->up,f->right,f->down,f->flying,f->size,
        f->ground_damage,f->ground_bonus,f->ground_cooldown,f->ground_range,
        f->ground_hits,f->air_damage,f->air_bonus,f->air_cooldown,
        f->air_range,f->air_hits,f->flags);
    printf("|%u,%d,%d,%d,%d,%d,%d,%d,%d\n",sc64_zp_actor_flags(type),
        sc64_zp_actor_sight_source_pixels(type),sc64_zp_actor_default_hp(type),
        sc64_zp_actor_default_shields(type),bounds,l,u,r,d);
}

static void building(int type)
{
    const Sc64ZpBuildingFact *f = sc64_zp_building_fact(type);
    int l=-91,u=-92,r=-93,d=-94;
    int bounds=sc64_zp_building_bounds_source(type,&l,&u,&r,&d);
    printf("B:%d:",type);
    if (!f) printf("NULL");
    else printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u",
        f->raw,f->hp,f->shields,f->armor,f->minerals,f->gas,f->build_frames,
        f->sight_px,f->width_tiles,f->height_tiles,f->left,f->up,f->right,
        f->down,f->size,f->flags);
    printf("|%u,%d,%d,%d,%d,%d,%d,%d\n",sc64_zp_building_flags(type),
        sc64_zp_building_sight_source_pixels(type),
        sc64_zp_building_default_hp(type),bounds,l,u,r,d);
}

int main(void)
{
    int t;
    printf("COUNTS:%d,%d\n",SC_RU_COUNT,SC_RB_COUNT);
    for (t=0;t<SC_RU_COUNT;++t) actor(t);
    for (t=0;t<SC_RB_COUNT;++t) building(t);
    for (t=-32;t<0;++t) { actor(t); building(t); }
    for (t=SC_RU_COUNT;t<SC_RU_COUNT+32;++t) actor(t);
    for (t=SC_RB_COUNT;t<SC_RB_COUNT+32;++t) building(t);
    actor(INT_MIN); actor(INT_MAX); building(INT_MIN); building(INT_MAX);
    return 0;
}
