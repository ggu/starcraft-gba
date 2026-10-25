#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_P6_COMBAT_ENABLED
#error "Compile with the P6 combat profile flags"
#endif

typedef struct {
    s16 x, y, raw_type;
    u16 flags;
    s16 target;
    u8 owner, alive, order;
} Sc64P6BurrowActor;

int sc64_p6_burrow_reference(const Sc64P6BurrowActor *, unsigned,
                             int (*)(int, int), unsigned *, unsigned);

static u32 rng_state=0x7b84d9a1u;
static unsigned next_random(void)
{
    rng_state^=rng_state<<13; rng_state^=rng_state>>17; rng_state^=rng_state<<5;
    return rng_state;
}

static void reset_world(void)
{
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(&race_p6,0,sizeof(race_p6));
    race_mode=0; race_selected_mission=SC64_P6_GLOBAL_ID;
    sc64_p6_runtime_init(&race_p6.runtime);
}

static void project_actors(Sc64P6BurrowActor *out)
{
    for (unsigned i=0;i<MAX_UNITS;++i) {
        out[i]=(Sc64P6BurrowActor){u[i].x,u[i].y,u[i].raw_type,
                                   u[i].flags,u[i].target,u[i].owner,
                                   u[i].alive,u[i].order};
    }
}

static void assert_current_world_matches_fullscan_reference(void)
{
    Unit expected[MAX_UNITS];
    Building saved_buildings[MAX_BUILDINGS];
    u8 saved_invincible_units[sizeof(invincible_units)];
    u8 saved_invincible_buildings[sizeof(invincible_buildings)];
    Sc64P6PersistentState saved_state;
    Sc64P6BurrowActor actors[MAX_UNITS];
    unsigned released[MAX_UNITS];
    unsigned expected_count, actual_count;

    project_actors(actors);
    expected_count=sc64_p6_burrow_reference(actors,MAX_UNITS,
                 race_p6_owners_hostile,released,MAX_UNITS);
    assert(expected_count<=MAX_UNITS);
    memcpy(expected,u,sizeof(u));
    for (unsigned i=0;i<expected_count;++i) {
        Unit *q=&expected[released[i]];
        q->flags&=(u16)~2u;
        q->order=ORDER_ATTACK_MOVE;
        q->target=-1;
    }
    memcpy(saved_buildings,b,sizeof(b));
    memcpy(saved_invincible_units,invincible_units,sizeof(invincible_units));
    memcpy(saved_invincible_buildings,invincible_buildings,sizeof(invincible_buildings));
    saved_state=race_p6;

    actual_count=(unsigned)race_p6_combat_burrow_release_tick();
    assert(actual_count==expected_count);
    assert(memcmp(u,expected,sizeof(u))==0);
    assert(memcmp(b,saved_buildings,sizeof(b))==0);
    assert(memcmp(invincible_units,saved_invincible_units,sizeof(invincible_units))==0);
    assert(memcmp(invincible_buildings,saved_invincible_buildings,
                  sizeof(invincible_buildings))==0);
    assert(memcmp(&race_p6,&saved_state,sizeof(race_p6))==0);
}

static void assert_matches_fullscan_reference(unsigned iteration)
{
    static const s16 burrow_raws[]={37,38,SC64_P6_RAW_INFESTED_TERRAN};
    static const s16 other_raws[]={0,1,2,42,65,79,84};
    reset_world();
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit *q=&u[i];
        q->alive=(u8)((next_random()%9u)!=0);
        q->owner=(u8)(next_random()%5u);
        q->raw_owner=q->owner;
        if (i<120u) {
            q->raw_type=burrow_raws[next_random()%3u];
            q->type=sc64_race_unit_from_raw(q->raw_type);
            if ((next_random()%5u)!=0) q->flags|=2u;
        } else {
            q->raw_type=other_raws[next_random()%7u];
            q->type=sc64_race_unit_from_raw(q->raw_type);
        }
        /* Keep the full-scan dist2 safely inside int range while exercising
           signed compact positions and dense near/far actor distributions. */
        q->x=(s16)((int)(next_random()%2400u)-300);
        q->y=(s16)((int)(next_random()%2400u)-300);
        q->hp=q->maxhp=1000;
        q->order=(next_random()%7u)==0?ORDER_ATTACK_MOVE:ORDER_IDLE;
        q->target=(s16)(next_random()%MAX_UNITS);
        if ((next_random()%23u)==0) q->flags|=16u;
        if ((next_random()%29u)==0) invincible_units[i]=1;
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        b[i].alive=(u8)(next_random()%2u);
        b[i].owner=(u8)(next_random()%6u);
        b[i].raw_owner=b[i].owner;
        b[i].raw_type=(s16)(next_random()%220u);
        b[i].hp=b[i].maxhp=1000;
        invincible_buildings[i]=(u8)(next_random()%2u);
    }
    assert_current_world_matches_fullscan_reference();
    (void)iteration;
}

static void exact_radius_cases(void)
{
    reset_world();
    u[0].alive=1; u[0].owner=ENEMY; u[0].raw_type=37; u[0].flags=2;
    u[0].type=sc64_race_unit_from_raw(37); u[0].x=0; u[0].y=0;
    u[1].alive=1; u[1].owner=SIDE; u[1].raw_type=0;
    u[1].type=U_MARINE; u[1].x=96; u[1].y=0;
    u[2].alive=1; u[2].owner=SIDE; u[2].raw_type=0;
    u[2].type=U_MARINE; u[2].x=97; u[2].y=0;
    assert(race_p6_combat_burrow_center_may_reach(0,0,96,0));
#ifdef SC64_P6_BURROW_CULL
    assert(!race_p6_combat_burrow_center_may_reach(0,0,97,0));
    assert(race_p6_combat_burrow_center_may_reach(0,0,-96,0));
    assert(!race_p6_combat_burrow_center_may_reach(0,0,-97,0));
    assert(race_p6_combat_burrow_center_may_reach(0,0,0,96));
    assert(!race_p6_combat_burrow_center_may_reach(0,0,0,-97));
    assert(race_p6_combat_burrow_center_may_reach(0,0,68,68));
    assert(!race_p6_combat_burrow_center_may_reach(-32768,0,32767,0));
    assert(!race_p6_combat_burrow_center_may_reach(32767,0,-32768,0));
#endif
    assert(race_p6_combat_burrow_release_tick()==1);
    assert(!(u[0].flags&2u) && u[0].order==ORDER_ATTACK_MOVE && u[0].target==-1);
    u[0].flags|=2u; u[0].order=ORDER_IDLE; u[0].target=17;
    u[1].alive=0; /* The only live hostile is now 97 pixels away. */
    assert(race_p6_combat_burrow_release_tick()==0);
    assert((u[0].flags&2u) && u[0].order==ORDER_IDLE && u[0].target==17);
    u[2].x=0; u[2].y=96;
    assert(race_p6_combat_burrow_release_tick()==1);
    assert(!(u[0].flags&2u) && u[0].order==ORDER_ATTACK_MOVE && u[0].target==-1);
}

#ifdef SC64_P6_BURROW_GROUPED
static void grouped_owner_groups_and_invalid_fallback(void)
{
    reset_world();
    /* Exercise every supported burrower owner, separated spatially so each
       release is explained by exactly one intended hostile actor. */
    static const s16 qx[]={0,500,1000,1500,1800};
    static const s16 qy[]={0,500,1000,1500,1800};
    static const u8 qo[]={SIDE,ENEMY,RESCUABLE,4,NEUTRAL};
    static const s16 qr[]={37,38,SC64_P6_RAW_INFESTED_TERRAN,37,38};
    static const u8 to[]={ENEMY,RESCUABLE,4,SIDE,SIDE};
    static const s16 tx[]={96,596,1000,1596,1800};
    static const s16 ty[]={0,500,1096,1500,1800};

    for (unsigned n=0;n<5;++n) {
        unsigned qi=n*2u, ti=qi+1u;
        u[qi].alive=1;u[qi].owner=qo[n];u[qi].raw_type=qr[n];
        u[qi].type=sc64_race_unit_from_raw(qr[n]);u[qi].flags=2u;
        u[qi].x=qx[n];u[qi].y=qy[n];u[qi].order=ORDER_IDLE;u[qi].target=77;
        u[ti].alive=1;u[ti].owner=to[n];u[ti].raw_type=0;
        u[ti].type=U_MARINE;u[ti].x=tx[n];u[ti].y=ty[n];
    }
    assert_current_world_matches_fullscan_reference();
    assert(!(u[0].flags&2u) && !(u[2].flags&2u) &&
           !(u[4].flags&2u) && !(u[6].flags&2u));
    assert((u[8].flags&2u) && u[8].order==ORDER_IDLE && u[8].target==77);

    /* A live unsupported owner forces the exact legacy scan for this tick.
       Include both a normal supported group and an invalid-owner burrower so
       the fallback preserves the old hostility predicate for both. */
    reset_world();
    u[0].alive=1;u[0].owner=ENEMY;u[0].raw_type=37;
    u[0].type=sc64_race_unit_from_raw(37);u[0].flags=2u;
    u[0].x=0;u[0].y=0;u[0].order=ORDER_IDLE;u[0].target=77;
    u[1].alive=1;u[1].owner=SIDE;u[1].raw_type=0;u[1].type=U_MARINE;
    u[1].x=96;u[1].y=0;
    u[2].alive=1;u[2].owner=5;u[2].raw_type=37;
    u[2].type=sc64_race_unit_from_raw(37);u[2].flags=2u;
    u[2].x=500;u[2].y=500;u[2].order=ORDER_IDLE;u[2].target=78;
    u[3].alive=1;u[3].owner=ENEMY;u[3].raw_type=0;u[3].type=U_MARINE;
    u[3].x=500;u[3].y=500;
    assert_current_world_matches_fullscan_reference();
    assert(!(u[0].flags&2u) && u[0].order==ORDER_ATTACK_MOVE && u[0].target==-1);
    assert((u[2].flags&2u) && u[2].order==ORDER_IDLE && u[2].target==78);
}
#endif

static void geometry_cull_is_conservative(void)
{
#ifdef SC64_P6_BURROW_CULL
    unsigned state=rng_state;
    for (unsigned i=0;i<100000u;++i) {
        int qx=(s16)(u16)next_random(), qy=(s16)(u16)next_random();
        int tx=(s16)(u16)next_random(), ty=(s16)(u16)next_random();
        int dx=qx-tx, dy=qy-ty;
        long long exact=(long long)dx*dx+(long long)dy*dy;
        if (!race_p6_combat_burrow_center_may_reach(qx,qy,tx,ty))
            assert(exact>96LL*96LL);
    }
    rng_state=state;
#endif
}

int main(void)
{
    exact_radius_cases();
#ifdef SC64_P6_BURROW_GROUPED
    grouped_owner_groups_and_invalid_fallback();
#endif
    geometry_cull_is_conservative();
    for (unsigned i=0;i<150u;++i) assert_matches_fullscan_reference(i);
#ifdef SC64_P6_BURROW_CULL
    puts("P6 burrow-release cull matches full-scan reference across 150 worlds and 100000 geometry proofs");
#else
    puts("P6 burrow-release full scan matches independent reference across 150 worlds");
#endif
    return 0;
}
