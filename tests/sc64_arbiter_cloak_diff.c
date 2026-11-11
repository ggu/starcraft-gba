/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Randomized differential test of the Arbiter cloak and detection
 * bookkeeping. The shipping code (lazy detector snapshot, deferred
 * visibility scan, cheaper validity checks) runs next to a frozen copy of
 * the pre-change code (tests/sc64_arbiter_cloak_oracle.h) on the same
 * world, and after every update everything the subsystem produces must be
 * identical: every Unit row (the packed cache word and every other byte),
 * every Building row, the persistent service state, the saved clock and the
 * due/initialized flags. Detected masks of random targets and the world
 * validity checks are compared as well.
 *
 * Worlds are random: clustered and scattered positions, Arbiters, cloaked
 * and uncloaked actors, Observers, Overlords, powered, unpowered and
 * unfinished Cannons, Spore Colonies, Pylons, hidden Interceptors, loaded
 * and hallucinated actors, owners and source slots that are consistent or
 * malformed, a random SIDE visibility grid, and units that move, die, spawn
 * and change hands between updates, for far more updates than one timer
 * cycle (31) and one stagger cycle (300).
 *
 * usage: test [worlds [updates [seed]]] */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_arbiter_cloak_oracle.h"

#ifdef SC64_TRANSPORT_RUNTIME
#define TEST_TRANSPORTED ORDER_TRANSPORTED
#else
#define TEST_TRANSPORTED 0
#endif

static unsigned rng_state=0x2545f491u;
static unsigned rnd(void)
{
    rng_state^=rng_state<<13; rng_state^=rng_state>>17; rng_state^=rng_state<<5;
    return rng_state;
}
static unsigned rn(unsigned n) { return rnd()%n; }
static int chance(unsigned one_in) { return rn(one_in)==0; }

/* Pre-change process_due, driving the frozen source tick. */
static void oracle_process_due(void)
{
    if (!sc64_p7_arbiter_cache_initialized) {
        sc64_p7_arbiter_cache_due=0;
        oracle_arbiter_cache_source_tick();
        sc64_p7_arbiter_cache_initialized=1;
        return;
    }
    if (!sc64_p7_arbiter_cache_due) return;
    sc64_p7_arbiter_cache_due=0;
    oracle_arbiter_cache_source_tick();
}

typedef struct { int type, raw; } ActorKind;
static const ActorKind unit_kinds[]={
    {U_ZEALOT,65},{U_ZEALOT,65},{U_DRAGOON,66},{U_P7_ARBITER,71},{U_P7_OBSERVER,84},
    {U_P7_OBSERVER,84},{U_ZEALOT,74},{U_ZEALOT,75},{U_OVERLORD,42},{U_MARINE,0},
    {U_PROBE,64},{U_SCOUT,70},{U_P7_CARRIER,72},
#ifdef SC64_P7_PROJECTILES_ENABLED
    {U_P7_INTERCEPTOR,73},
#endif
{U_REAVER,83},
    {U_ZEALOT,-1},{U_P7_ARBITER,-1},{U_ZEALOT,86},{U_ZEALOT,14},{U_ZEALOT,101},
    {U_ZEALOT,228},{U_ZEALOT,300},{U_FENIX,78},{U_TASSADAR,79},{U_MUTALISK,43},
};
static const ActorKind building_kinds[]={
    {B_PHOTON_CANNON,162},{B_PHOTON_CANNON,162},{B_PHOTON_CANNON,162},{B_PYLON,156},
    {B_PYLON,156},{B_NEXUS,154},{B_GATEWAY,160},{B_FORGE,166},{B_SPORE_COLONY,144},
    {B_ASSIMILATOR,157},{B_CYBERNETICS_CORE,164},{B_PYLON,-1},{B_PHOTON_CANNON,-1},
    {B_STARGATE,167},{B_COMMAND,0},
};
#define N_UNIT_KINDS ((int)(sizeof(unit_kinds)/sizeof(unit_kinds[0])))
#define N_BUILDING_KINDS ((int)(sizeof(building_kinds)/sizeof(building_kinds[0])))

static int valid_slots[8], valid_slot_count;
static int cluster_x[8], cluster_y[8], cluster_count;

static void pick_clusters(void)
{
    cluster_count=2+(int)rn(6);
    for (int i=0;i<cluster_count;++i) {
        cluster_x[i]=(int)rn((unsigned)WORLD_W);
        cluster_y[i]=(int)rn((unsigned)WORLD_H);
    }
}

static int tclamp(int v,int lo,int hi) { return v<lo?lo:v>hi?hi:v; }

static void random_position(int *x,int *y)
{
    unsigned mode=rn(20);
    if (mode<13) {
        int c=(int)rn((unsigned)cluster_count);
        *x=cluster_x[c]+(int)rn(260)-130; *y=cluster_y[c]+(int)rn(260)-130;
    } else if (mode<17) {
        *x=(int)rn((unsigned)WORLD_W); *y=(int)rn((unsigned)WORLD_H);
    } else if (mode<19) {
        *x=rn(2)?0:WORLD_W-1; *y=(int)rn((unsigned)WORLD_H);
    } else {
        *x=(int)rn((unsigned)WORLD_W); *y=rn(2)?0:WORLD_H-1;
    }
    *x=tclamp(*x,0,WORLD_W-1); *y=tclamp(*y,0,WORLD_H-1);
}

/* Pick a (source slot, live owner) pair: normally the authenticated pair,
 * sometimes a malformed one. */
static void random_identity(int *live,int *raw)
{
    unsigned mode=rn(40);
    int slot=valid_slots[rn((unsigned)valid_slot_count)];
    *raw=slot;
    *live=race_protoss_source_owner((unsigned)slot);
    if (mode==0) *raw=-1;
    else if (mode==1) *raw=8+(int)rn(3);
    else if (mode==2) *live=(int)rn(5);
    else if (mode==3) *live=5;
    else if (mode==4) *raw=(int)rn(8);
    else if (mode==5) *live=SIDE;
    if (*live<0 || *live>5) *live=(int)rn(5);
}

static void make_unit(int slot)
{
    const ActorKind *k=&unit_kinds[rn((unsigned)N_UNIT_KINDS)];
    int live,raw,x,y;
    memset(&u[slot],0,sizeof(u[slot]));
    random_identity(&live,&raw);
    random_position(&x,&y);
    u[slot].alive=1; u[slot].type=k->type; u[slot].raw_type=k->raw;
    u[slot].owner=live; u[slot].raw_owner=raw; u[slot].x=x; u[slot].y=y;
    u[slot].hp=u[slot].maxhp=100;
    if (chance(25)) u[slot].order=TEST_TRANSPORTED;
    if (chance(20)) u[slot].flags|=8;
    if (k->raw==73) u[slot].flags|=(unsigned)rn(8)<<8;
    {
        unsigned mode=rn(60);
        if (mode==0) u[slot].weapon=0;
        else if (mode==1) u[slot].weapon=(u16)rnd();
        else if (mode==2) u[slot].weapon=(u16)(sc64_p7_arbiter_pack(rnd()&255u,rn(31),chance(2),chance(2)));
        else u[slot].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
    }
}

static void make_building(int slot)
{
    const ActorKind *k=&building_kinds[rn((unsigned)N_BUILDING_KINDS)];
    int live,raw,x,y;
    memset(&b[slot],0,sizeof(b[slot]));
    random_identity(&live,&raw);
    random_position(&x,&y);
    b[slot].alive=1; b[slot].type=k->type; b[slot].raw_type=k->raw;
    b[slot].owner=live; b[slot].raw_owner=raw; b[slot].x=x; b[slot].y=y;
    b[slot].hp=b[slot].maxhp=500;
    b[slot].under_construction=chance(8);
}

/* A cluster of Arbiters, Observers and Cannons around shared points makes
 * auras, detection and power all fire. */
static void make_core_actors(void)
{
    int arbiters=1+(int)rn(4);
    for (int a=0;a<arbiters;++a) {
        int slot=(int)rn((unsigned)MAX_UNITS);
        int live,raw,c=(int)rn((unsigned)cluster_count);
        make_unit(slot);
        random_identity(&live,&raw);
        raw=valid_slots[rn((unsigned)valid_slot_count)];
        live=race_protoss_source_owner((unsigned)raw);
        u[slot].alive=1; u[slot].type=U_P7_ARBITER; u[slot].raw_type=71;
        u[slot].raw_owner=raw; u[slot].owner=live; u[slot].flags&=(u16)~8u;
        u[slot].order=0;
        u[slot].x=tclamp(cluster_x[c]+(int)rn(120)-60,0,WORLD_W-1);
        u[slot].y=tclamp(cluster_y[c]+(int)rn(120)-60,0,WORLD_H-1);
        u[slot].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
    }
    int obs=(int)rn(4);
    for (int a=0;a<obs;++a) {
        int slot=(int)rn((unsigned)MAX_UNITS), c=(int)rn((unsigned)cluster_count);
        make_unit(slot);
        u[slot].type=U_P7_OBSERVER; u[slot].raw_type=84; u[slot].order=0;
        u[slot].x=tclamp(cluster_x[c]+(int)rn(300)-150,0,WORLD_W-1);
        u[slot].y=tclamp(cluster_y[c]+(int)rn(300)-150,0,WORLD_H-1);
        u[slot].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
    }
    int cannons=(int)rn(6);
    for (int a=0;a<cannons;++a) {
        int slot=(int)rn((unsigned)MAX_BUILDINGS), c=(int)rn((unsigned)cluster_count);
        int live,raw;
        make_building(slot);
        random_identity(&live,&raw);
        b[slot].type=chance(5)?B_SPORE_COLONY:B_PHOTON_CANNON;
        b[slot].raw_type=b[slot].type==B_SPORE_COLONY?144:162;
        b[slot].owner=live; b[slot].raw_owner=raw;
        b[slot].x=tclamp(cluster_x[c]+(int)rn(300)-150,0,WORLD_W-1);
        b[slot].y=tclamp(cluster_y[c]+(int)rn(300)-150,0,WORLD_H-1);
        int py=(int)rn((unsigned)MAX_BUILDINGS);
        make_building(py);
        b[py].type=B_PYLON; b[py].raw_type=156; b[py].owner=live; b[py].raw_owner=raw;
        b[py].under_construction=0;
        b[py].x=tclamp(b[slot].x+(int)rn(120)-60,0,WORLD_W-1);
        b[py].y=tclamp(b[slot].y+(int)rn(120)-60,0,WORLD_H-1);
    }
}

static void init_world(int binding)
{
    int units, buildings;
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    memset(&race_p7,0,sizeof(race_p7));
    memset(visible,0,sizeof(visible));
    race_protoss_binding_override=-1;
    race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
    switch (binding) {
    case 1:
#ifdef SC64_P10_GLOBAL_ID
        race_selected_mission=SC64_P10_GLOBAL_ID;
#endif
        break;
    case 2: race_mode=1; break;
    default: break;
    }
    memset(race_protoss_generic_clock,0,sizeof(race_protoss_generic_clock));
    RACE_PROTOSS_SERVICE_CLOCK[0]=RACE_PROTOSS_SERVICE_CLOCK[1]=RACE_PROTOSS_SERVICE_CLOCK[2]=0;
    sc64_p7_arbiter_cache_initialized=0; sc64_p7_arbiter_cache_due=0;
    valid_slot_count=0;
    for (int slot=0;slot<8;++slot)
        if (race_protoss_source_owner((unsigned)slot)>=0) valid_slots[valid_slot_count++]=slot;
    assert(valid_slot_count>=2);
    pick_clusters();
    units=20+(int)rn((unsigned)(MAX_UNITS-20));
    buildings=4+(int)rn((unsigned)(MAX_BUILDINGS-4));
    for (int i=0;i<units;++i) make_unit(i);
    for (int i=0;i<buildings;++i) make_building(i);
    make_core_actors();
    /* SIDE visibility: random patches. */
    for (int patch=0;patch<2+(int)rn(8);++patch) {
        int cx=(int)rn((unsigned)MAP_COLS), cy=(int)rn((unsigned)MAP_ROWS), r=2+(int)rn(12);
        for (int y=cy-r;y<=cy+r;++y) for (int x=cx-r;x<=cx+r;++x)
            if (x>=0 && y>=0 && x<MAP_COLS && y<MAP_ROWS) SC64_GRID_SET(visible,x+y*MAP_COLS,1);
    }
}

static void mutate_world(void)
{
    for (int i=0;i<MAX_UNITS;++i) {
        if (!u[i].alive) {
            if (chance(150)) make_unit(i);
            continue;
        }
        if (chance(4)) {
            u[i].x=tclamp(u[i].x+(int)rn(9)-4,0,WORLD_W-1);
            u[i].y=tclamp(u[i].y+(int)rn(9)-4,0,WORLD_H-1);
        }
        if (chance(300)) {
            u[i].alive=0;
            if (chance(3)) memset(&u[i],0,sizeof(u[i]));
        }
        else if (chance(500)) { int live,raw; random_identity(&live,&raw); u[i].owner=live; u[i].raw_owner=raw; }
        else if (chance(500)) u[i].order=u[i].order==TEST_TRANSPORTED?0:TEST_TRANSPORTED;
        else if (chance(500)) u[i].flags^=8;
        else if (chance(700)) u[i].flags^=(u16)(1u<<(8+rn(3)));
        else if (chance(1500)) u[i].weapon=(u16)rnd();
        else if (chance(1500)) u[i].weapon=0;
        else if (chance(2000)) { int x,y; random_position(&x,&y); u[i].x=x; u[i].y=y; }
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
        if (!b[i].alive) { if (chance(300)) make_building(i); continue; }
        if (chance(400)) b[i].under_construction^=1;
        else if (chance(500)) b[i].alive=0;
        else if (chance(500)) { int live,raw; random_identity(&live,&raw); b[i].owner=live; b[i].raw_owner=raw; }
    }
    if (chance(3)) for (int n=0;n<24;++n) {
        int cell=(int)rn((unsigned)(MAP_COLS*MAP_ROWS));
        SC64_GRID_SET(visible,cell,SC64_GRID_GET(visible,cell)==0);
    }
    sc64_target_index_end();
    if (rn(8)) sc64_target_index_begin();
}

typedef struct {
    Unit u[MAX_UNITS];
    Building b[MAX_BUILDINGS];
    Sc64P7PersistentState p7;
    u8 generic_clock[3], clock[3];
    int initialized, due;
} Snapshot;
static Snapshot snap0, snap_new, snap_old;

static void save(Snapshot *s)
{
    memcpy(s->u,u,sizeof(u)); memcpy(s->b,b,sizeof(b)); s->p7=race_p7;
    memcpy(s->generic_clock,race_protoss_generic_clock,3);
    memcpy(s->clock,RACE_PROTOSS_SERVICE_CLOCK,3);
    s->initialized=sc64_p7_arbiter_cache_initialized; s->due=sc64_p7_arbiter_cache_due;
}
static void load(const Snapshot *s)
{
    memcpy(u,s->u,sizeof(u)); memcpy(b,s->b,sizeof(b)); race_p7=s->p7;
    memcpy(race_protoss_generic_clock,s->generic_clock,3);
    memcpy(RACE_PROTOSS_SERVICE_CLOCK,s->clock,3);
    sc64_p7_arbiter_cache_initialized=s->initialized; sc64_p7_arbiter_cache_due=s->due;
}

static unsigned long long unit_ticks, source_ticks, refreshed, nonzero_masks, aura_units,
    tracked_units, mask_checks, mask_positive, valid_checks, invalid_worlds, scalar_checks,
    cannon_powered_seen, hidden_seen;
static int current_world, current_update;

static void fail_row(const char *what,int index,const void *a,const void *c,size_t size)
{
    const unsigned char *pa=a,*pc=c;
    fprintf(stderr,"MISMATCH world=%d update=%d %s[%d]:",current_world,current_update,what,index);
    for (size_t i=0;i<size;++i) if (pa[i]!=pc[i]) fprintf(stderr," byte%zu new=%02x oracle=%02x",i,pa[i],pc[i]);
    fprintf(stderr,"\n");
    abort();
}

static void compare_snapshots(void)
{
    for (int i=0;i<MAX_UNITS;++i)
        if (memcmp(&snap_new.u[i],&snap_old.u[i],sizeof(Unit)))
            fail_row("unit",i,&snap_new.u[i],&snap_old.u[i],sizeof(Unit));
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (memcmp(&snap_new.b[i],&snap_old.b[i],sizeof(Building)))
            fail_row("building",i,&snap_new.b[i],&snap_old.b[i],sizeof(Building));
    if (memcmp(&snap_new.p7,&snap_old.p7,sizeof(snap_new.p7))) fail_row("p7",0,&snap_new.p7,&snap_old.p7,sizeof(snap_new.p7));
    if (memcmp(snap_new.generic_clock,snap_old.generic_clock,3)) fail_row("generic_clock",0,snap_new.generic_clock,snap_old.generic_clock,3);
    if (memcmp(snap_new.clock,snap_old.clock,3)) fail_row("clock",0,snap_new.clock,snap_old.clock,3);
    if (snap_new.initialized!=snap_old.initialized || snap_new.due!=snap_old.due) {
        fprintf(stderr,"MISMATCH world=%d update=%d flags\n",current_world,current_update);
        abort();
    }
}

/* Cannon power as the pre-change source tick computed it up front. */
static void oracle_powered(u8 powered[MAX_BUILDINGS])
{
    memset(powered,0,MAX_BUILDINGS);
    for (int j=0;j<MAX_BUILDINGS;++j) if (b[j].alive &&
        RACE_PROTOSS_RAW(&b[j])==SC64_P7_CANNON_RAW && building_complete(j)) {
        int source=sc64_p7_detector_current_slot(b[j].owner,RACE_PROTOSS_SLOT(&b[j]));
        if (source>=0 && source<8) {
            int live=race_protoss_source_owner((unsigned)source);
            if (live==b[j].owner)
                powered[j]=(u8)sc64_p7_observer_pylon_powers(j,source,live);
        }
    }
}

static void compare_masks(void)
{
    Sc64P7DetectorLists lists;
    OracleDetectorLists olists;
    u8 powered[MAX_BUILDINGS];
    sc64_p7_detector_lists_begin(&lists);
    oracle_detector_lists_build(&olists);
    oracle_powered(powered);
    for (int n=0;n<8;++n) {
        int t=(int)rn((unsigned)MAX_UNITS);
        if (n<4) {
            /* Bias toward live, tracked targets inside a cluster. */
            for (int tries=0;tries<20 && !(u[t].alive &&
                 (sc64_p7_observer_is_cloaked((unsigned)RACE_PROTOSS_RAW(&u[t])) ||
                  (u[t].weapon&SC64_P7_ARBITER_AURA)));++tries)
                t=(int)rn((unsigned)MAX_UNITS);
        }
        unsigned want=oracle_detected_mask_lists(t,powered,&olists);
        unsigned got=sc64_p7_arbiter_detected_mask_lists(t,0,&lists);
        if (want!=got) {
            fprintf(stderr,"MASK MISMATCH world=%d update=%d target=%d new=%02x oracle=%02x\n",
                    current_world,current_update,t,got,want);
            abort();
        }
        ++mask_checks; mask_positive+=want!=0;
        /* With the fixture's explicit power table the new route must agree too. */
        got=sc64_p7_arbiter_detected_mask_lists(t,powered,&lists);
        if (want!=got) {
            fprintf(stderr,"MASK MISMATCH (explicit power) world=%d update=%d target=%d new=%02x oracle=%02x\n",
                    current_world,current_update,t,got,want);
            abort();
        }
#ifdef GBA_HOST
        if (n<2) {
            got=sc64_p7_arbiter_detected_mask(t,powered);
            if (want!=got) {
                fprintf(stderr,"MASK MISMATCH (scalar) world=%d update=%d target=%d scalar=%02x oracle=%02x\n",
                        current_world,current_update,t,got,want);
                abort();
            }
            ++scalar_checks;
        }
#endif
    }
}

static void compare_validity(void)
{
    int want=oracle_cache_world_valid(), got=sc64_p7_arbiter_cache_world_valid();
    if (want!=got) {
        fprintf(stderr,"VALID MISMATCH world=%d update=%d new=%d oracle=%d\n",current_world,current_update,got,want);
        abort();
    }
    ++valid_checks; invalid_worlds+=!want;
    if (current_update%4==0) {
        unsigned masks[3]={sc64_p7_valid_source_mask(),rnd()&255u,0};
        for (int i=0;i<MAX_UNITS;++i) for (int m=0;m<3;++m) {
            int a=oracle_cache_unit_valid_with_mask(&u[i],masks[m]);
            int c=sc64_p7_arbiter_cache_unit_valid_with_mask(&u[i],masks[m]);
            if (a!=c) {
                fprintf(stderr,"UNIT VALID MISMATCH world=%d update=%d unit=%d mask=%02x new=%d oracle=%d\n",
                        current_world,current_update,i,masks[m],c,a);
                abort();
            }
            ++valid_checks;
        }
    }
    /* Corrupt one row in place (a stray byte in an empty row, or a random
     * cache word) and compare the verdicts again. */
    if (chance(6)) {
        int i=(int)rn((unsigned)MAX_UNITS);
        Unit keep=u[i];
        if (chance(2)) ((unsigned char *)&u[i])[rn((unsigned)sizeof(Unit))]^=(unsigned char)(1u<<rn(8));
        else u[i].weapon=(u16)rnd();
        want=oracle_cache_world_valid(); got=sc64_p7_arbiter_cache_world_valid();
        if (want!=got) {
            fprintf(stderr,"VALID MISMATCH (corrupt) world=%d update=%d unit=%d new=%d oracle=%d\n",current_world,current_update,i,got,want);
            abort();
        }
        ++valid_checks; invalid_worlds+=!want;
        u[i]=keep;
    }
}

static void one_update(void)
{
    mutate_world();
    for (int i=0;i<MAX_UNITS;++i) unit_ticks+=u[i].alive!=0;
    save(&snap0);
    sc64_p7_arbiter_cache_frame_tick();
    sc64_p7_arbiter_cache_process_due();
    save(&snap_new);
#ifdef GBA_HOST
    if (current_update%4==1) {
        /* The per-actor scalar route of the shipping code. */
        load(&snap0);
        sc64_p7_detector_lists_test_scalar=1;
        sc64_p7_arbiter_cache_frame_tick();
        sc64_p7_arbiter_cache_process_due();
        sc64_p7_detector_lists_test_scalar=0;
        save(&snap_old);
        ++scalar_checks;
        compare_snapshots();
    }
#endif
    load(&snap0);
    sc64_p7_arbiter_cache_frame_tick();
    oracle_process_due();
    save(&snap_old);
    compare_snapshots();
    load(&snap_new);
    if (memcmp(snap0.u,snap_new.u,sizeof(snap0.u))) ++source_ticks;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        unsigned w=u[i].weapon;
        if (SC64_P7_ARBITER_CACHE_VALID(w)) {
            if (SC64_P7_ARBITER_DETECTED_MASK(w)) ++nonzero_masks;
            aura_units+=(w&SC64_P7_ARBITER_AURA)!=0;
            tracked_units+=(w&SC64_P7_ARBITER_TRACKED)!=0;
            refreshed+=SC64_P7_ARBITER_COUNTDOWN(w)==30;
        }
    }
    for (int i=0;i<MAX_BUILDINGS;++i) cannon_powered_seen+=b[i].alive && b[i].raw_type==162 && !b[i].under_construction;
    for (int i=0;i<MAX_UNITS;++i) hidden_seen+=u[i].alive && SC64_CARRIER_HANGAR_HIDDEN(&u[i]);
    compare_masks();
    compare_validity();
}

int main(int argc,char **argv)
{
    int worlds=argc>1?atoi(argv[1]):240, updates=argc>2?atoi(argv[2]):420;
    if (argc>3) rng_state=(unsigned)strtoul(argv[3],0,0)|1u;
    for (current_world=0;current_world<worlds;++current_world) {
        init_world(current_world%3);
        for (current_update=0;current_update<updates;++current_update) one_update();
    }
    printf("arbiter cloak differential: %d worlds x %d updates, %llu unit-updates, "
           "%llu changing source ticks, %llu refreshes, %llu tracked, %llu aura, "
           "%llu nonzero detected masks, %llu mask checks (%llu positive), %llu validity checks "
           "(%llu invalid), %llu scalar-route runs, hidden=%llu: identical\n",
           worlds,updates,unit_ticks,source_ticks,refreshed,tracked_units,aura_units,
           nonzero_masks,mask_checks,mask_positive,valid_checks,invalid_worlds,scalar_checks,hidden_seen);
    assert(unit_ticks>=200000u);
    assert(nonzero_masks>0 && aura_units>0 && mask_positive>0 && invalid_worlds>0);
    return 0;
}
