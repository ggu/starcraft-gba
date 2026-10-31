/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine aura-union oracle for the optional Arbiter duplicate skip. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static unsigned aura_rng=0x7293a5c1u;
static unsigned aura_random(void)
{
    aura_rng^=aura_rng<<13;
    aura_rng^=aura_rng>>17;
    aura_rng^=aura_rng<<5;
    return aura_rng;
}

static int make_actor(int live_owner,unsigned raw,int type,int owner,int x,int y)
{
    int n=spawn_unit(live_owner,type,x,y);
    assert(n>=0);
    u[n].raw_type=(int)raw;
    u[n].raw_owner=owner;
    return n;
}

static void aura_union_oracle(void)
{
    int raw_slots[8],slot_count=0,remap_a=-1,remap_b=-1;
    int checked_candidates=0;
    for(int raw=0;raw<8;++raw) {
        int live=race_p7_source_owner((unsigned)raw);
        if(live>=0 && live<5) raw_slots[slot_count++]=raw;
    }
    assert(slot_count>=2);
    for(int a=0;a<slot_count && remap_a<0;++a)
        for(int b=a+1;b<slot_count;++b)
            if(race_p7_source_owner((unsigned)raw_slots[a])==
               race_p7_source_owner((unsigned)raw_slots[b])) {
                remap_a=raw_slots[a]; remap_b=raw_slots[b]; break;
            }
    assert(remap_a>=0 && remap_b>=0);

    for(int trial=0;trial<80;++trial) {
        int expected_aura[MAX_UNITS]={0};
        int aura_cover_count[MAX_UNITS]={0};
        int arbiter_slots[5],arbiter_count=0;
        memset(u,0,sizeof(u));
        memset(b,0,sizeof(b));
        memset(&race_p7,0,sizeof(race_p7));
        memset(visible,0,sizeof(visible));
        race_mode=0;
        race_selected_mission=SC64_P7_GLOBAL_ID;
        race_p7.reserved[1]=150;
        sc64_p7_arbiter_cache_initialized=0;
        sc64_p7_arbiter_cache_due=0;

        for(int a=0;a<4;++a) {
            int raw=remap_a;
            int live=race_p7_source_owner((unsigned)raw);
            int x=(int)(aura_random()%(unsigned)WORLD_W);
            int y=(int)(aura_random()%(unsigned)WORLD_H);
            if(a==0 || a==1) { x=0; y=63; } /* overlapping same-source Arbiters */
            else if(a==2) { x=WORLD_W-1; y=WORLD_H-1; }
            arbiter_slots[arbiter_count++]=make_actor(live,71,U_P7_ARBITER,
                                                       raw,x,y);
        }
        for(int n=0;n<72;++n) {
            int raw=raw_slots[aura_random()%(unsigned)slot_count];
            int live=race_p7_source_owner((unsigned)raw);
            int x=(int)(aura_random()%(unsigned)WORLD_W);
            int y=(int)(aura_random()%(unsigned)WORLD_H);
            if((n%3)==0) {
                int a=arbiter_slots[n%arbiter_count];
                x=u[a].x+(int)(aura_random()%257u)-128;
                y=u[a].y+(int)(aura_random()%257u)-128;
                if(x<0)x=0; if(x>=WORLD_W)x=WORLD_W-1;
                if(y<0)y=0; if(y>=WORLD_H)y=WORLD_H-1;
                if((n%9)==0) {
                    x=(x/64)*64; y=(y/64)*64;
                    if(x>=WORLD_W)x=WORLD_W-1;
                    if(y>=WORLD_H)y=WORLD_H-1;
                }
            }
            if(n<5) {
                raw=n==0?remap_b:remap_a;
                live=race_p7_source_owner((unsigned)raw);
                if(n==0 || n==1) { x=0; y=63; }
                else if(n==2) { x=63; y=64; }
                else if(n==3) { x=64; y=63; }
                else { x=WORLD_W-1; y=WORLD_H-1; }
            }
            (void)make_actor(live,65,U_ZEALOT,raw,x,y);
        }

        if(trial==79) u[4].owner=5; /* force the legacy no-index fallback */
        sc64_target_index_begin();
        assert(sc64_target_index_active==(trial!=79));
        /* Move indexed records, then leave stale list entries after death or
           transport. Expected coverage is computed from final live state. */
        for(int n=0;n<10;++n) {
            int slot=(int)(aura_random()%(unsigned)MAX_UNITS);
            if(!u[slot].alive || u[slot].raw_type==71 || (slot>=4 && slot<=8)) continue;
            u[slot].x=(int)(aura_random()%(unsigned)WORLD_W);
            u[slot].y=(int)(aura_random()%(unsigned)WORLD_H);
            sc64_target_index_move(&u[slot]);
        }
        for(int n=0;n<8;++n) {
            int slot=(int)(aura_random()%(unsigned)MAX_UNITS);
            if(!u[slot].alive || u[slot].raw_type==71 || (slot>=4 && slot<=8)) continue;
            if(n&1) u[slot].order=ORDER_TRANSPORTED;
            else u[slot].alive=0;
        }

        /* Independent all-pairs aura union, without using the indexed query
           traversal under test. */
        for(int a=0;a<arbiter_count;++a) {
            int arb=arbiter_slots[a];
            if(!u[arb].alive || u[arb].raw_owner<0 || u[arb].raw_owner>=8 ||
               (u[arb].flags&8u) || SC64_TRANSPORT_LOADED(&u[arb]) ||
               race_p7_source_owner((unsigned)u[arb].raw_owner)!=u[arb].owner)
                continue;
            for(int t=0;t<MAX_UNITS;++t)
                if(t!=arb && sc64_p7_observer_arbiter_aura_candidate(t) &&
                   sc64_p7_arbiter_same_source(arb,t)) {
                    int distance=sc64_p7_observer_rect_distance(71u,u[arb].x,
                        u[arb].y,0,(unsigned)u[t].raw_type,u[t].x,u[t].y,0);
                    if(sc64_p7_arbiter_finder_square(arb,t) &&
                       distance>=0 && distance<=160) {
                        expected_aura[t]=1;
                        ++aura_cover_count[t];
                        ++checked_candidates;
                    }
                }
        }
        assert(!expected_aura[4]); /* live-force alias, distinct raw source */
        assert(expected_aura[5]);  /* matching raw source at the same point */
        assert(aura_cover_count[5]>1); /* overlapping Arbiters cover the union */
        assert(expected_aura[6] && expected_aura[7]); /* 63/64 cell boundary */
        assert(expected_aura[8]);  /* final row/column */

        sc64_p7_arbiter_cache_source_tick();
        for(int i=0;i<MAX_UNITS;++i) {
            unsigned expected=0;
            if(u[i].alive && expected_aura[i] &&
               !sc64_p7_observer_is_cloaked((unsigned)u[i].raw_type))
                expected=sc64_p7_arbiter_pack(0,30,1,1);
            else if(u[i].alive && sc64_p7_observer_is_cloaked((unsigned)u[i].raw_type))
                expected=sc64_p7_arbiter_pack(0,30,0,1);
            else if(!u[i].alive)
                expected=sc64_p7_arbiter_cache_empty_unit(&u[i]) ? u[i].weapon :
                         sc64_p7_arbiter_pack(0,0,0,0);
            else expected=sc64_p7_arbiter_pack(0,0,0,0);
            if(u[i].weapon!=(u16)expected) {
                fprintf(stderr,"aura oracle trial=%d slot=%d actual=%04x expected=%04x raw=%d owner=%d aura=%d covers=%d\n",
                        trial,i,(unsigned)u[i].weapon,(unsigned)expected,
                        u[i].raw_type,u[i].owner,expected_aura[i],aura_cover_count[i]);
                assert(u[i].weapon==(u16)expected);
            }
        }
    }
    assert(checked_candidates>0);
    puts("P7 Arbiter aura union oracle passed (80 randomized worlds)");
}

/* Generic games derive the source slot from the live owner, regardless of
 * the imported raw-owner field. Exercise every owner-index lane, including
 * an unavailable-index fallback, against a separate all-pairs union. */
static void generic_aura_owner_lanes(void)
{
    for(int fallback=0;fallback<2;++fallback) {
        int expected[MAX_UNITS]={0};
        int arbs[5],targets[5];
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
        memset(visible,0,sizeof(visible));
        memset(race_protoss_generic_clock,0,sizeof(race_protoss_generic_clock));
        race_protoss_generic_clock[1]=150;
        race_mode=1;
        sc64_p7_arbiter_cache_initialized=0;
        sc64_p7_arbiter_cache_due=0;
        for(int owner=0;owner<5;++owner) {
            arbs[owner]=make_actor(owner,71,U_P7_ARBITER,0,63,64);
            targets[owner]=make_actor(owner,65,U_ZEALOT,4,64,63);
            (void)make_actor(owner,65,U_ZEALOT,-1,640,640);
        }
        if(fallback) (void)make_actor(5,65,U_ZEALOT,7,128,128);
        sc64_target_index_begin();
        assert(sc64_target_index_active==!fallback);
        for(int owner=0;owner<5;++owner) {
            int arb=arbs[owner];
            for(int target=0;target<MAX_UNITS;++target) {
                if(target==arb || !sc64_p7_observer_arbiter_aura_candidate(target) ||
                   !sc64_p7_arbiter_same_source(arb,target)) continue;
                int distance=sc64_p7_observer_rect_distance(71u,u[arb].x,u[arb].y,0,
                    (unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,u[target].y,0);
                if(sc64_p7_arbiter_finder_square(arb,target) && distance>=0 && distance<=160)
                    expected[target]=1;
            }
            assert(expected[targets[owner]]);
        }
        sc64_p7_arbiter_cache_source_tick();
        for(int i=0;i<MAX_UNITS;++i) {
            unsigned want=u[i].alive ? sc64_p7_arbiter_pack(0,expected[i]?30:0,
                                                          expected[i],expected[i]) : 0;
            assert(u[i].weapon==(u16)want);
        }
    }
    puts("Generic Arbiter aura matches all-pairs union in all five owner lanes and fallback");
}

int main(void)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(SC64_RACE_SAVE_GENERATION==40u);
#else
    assert(SC64_RACE_SAVE_GENERATION==39u);
#endif
    aura_union_oracle();
    generic_aura_owner_lanes();
    return 0;
}
