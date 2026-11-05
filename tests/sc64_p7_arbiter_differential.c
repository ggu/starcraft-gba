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
    assert(race_p7_source_owner(0)==race_p7_source_owner(4));
    assert(remap_a==0 && remap_b==4);

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
        /* Differential lifecycle oracle: compare the complete actor records,
         * buildings, persistent service state and due/initialized bits on
         * every frame, including due and non-due clocks and index fallback. */
        for(int step=0;step<24;++step) {
            int slot=10+(int)(aura_random()%60u);
            if(step%6==0) u[slot].alive=0;
            else if(step%6==1) { u[slot].alive=1; u[slot].raw_type=SC64_P7_OBSERVER_RAW; u[slot].raw_owner=7; u[slot].owner=SIDE; }
            else if(step%6==2) u[slot].order=ORDER_TRANSPORTED;
            else if(step%6==3) { u[slot].order=ORDER_IDLE; u[slot].raw_owner=-1; }
            else if(step%6==4) { u[slot].weapon=(u16)aura_random(); u[slot].raw_type=SC64_OVERLORD_RAW; }
            else { u[slot].x=(int)(aura_random()%WORLD_W); u[slot].y=(int)(aura_random()%WORLD_H); }
            if(step==2) { race_p7.reserved[1]=1; race_p7.reserved[2]=0; }
            if(step==12) { race_p7.reserved[1]=0; race_p7.reserved[2]=0; sc64_p7_arbiter_cache_initialized=0; }
            sc64_target_index_end();
            if(step%7==0) u[slot].owner=5; /* force the index fallback */
            sc64_target_index_begin();
            sc64_p7_arbiter_cache_frame_tick(); sc64_p7_arbiter_cache_process_due();
            assert(fwrite(u,1,sizeof(u),stdout)==sizeof(u));
            assert(fwrite(b,1,sizeof(b),stdout)==sizeof(b));
            assert(fwrite(&race_p7,1,sizeof(race_p7),stdout)==sizeof(race_p7));
            assert(fwrite(&sc64_p7_arbiter_cache_initialized,1,sizeof(sc64_p7_arbiter_cache_initialized),stdout)==sizeof(sc64_p7_arbiter_cache_initialized));
            assert(fwrite(&sc64_p7_arbiter_cache_due,1,sizeof(sc64_p7_arbiter_cache_due),stdout)==sizeof(sc64_p7_arbiter_cache_due));
        }
    }
    assert(checked_candidates>0);
    puts("P7 Arbiter aura union oracle passed (80 randomized worlds)");
}

/* Compare the same world through the real index and its scalar fallback,
 * then explicitly reject malformed index inputs. Both historical and
 * candidate binaries emit the same complete actor/cache stream. */
static void owner_bucket_fallback_oracle(void)
{
    int raw=-1, live=-1;
    for (int k=0;k<8;++k) {
        int owner=race_p7_source_owner((unsigned)k);
        if(owner>=0 && owner<5) { raw=k; live=owner; break; }
    }
    assert(raw>=0);
    for(int trial=0;trial<6;++trial) {
        Unit before[MAX_UNITS], indexed[MAX_UNITS];
        Building before_buildings[MAX_BUILDINGS];
        Sc64P7PersistentState before_state, indexed_state;
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
        memset(&race_p7,0,sizeof(race_p7)); memset(visible,0,sizeof(visible));
        race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
        race_p7.reserved[1]=150;
        sc64_p7_arbiter_cache_initialized=0; sc64_p7_arbiter_cache_due=0;
        int arb=make_actor(live,71,U_P7_ARBITER,raw,64,64);
        int target=make_actor(live,65,U_ZEALOT,raw,65,64);
        for(int owner=0;owner<5;++owner)
            (void)make_actor(owner,65,U_ZEALOT,raw,66+owner,64);
        if(trial==1) u[target].owner=-1;
        if(trial==2) u[target].owner=5;
        if(trial==3 || trial==4) { b[0].alive=1; b[0].owner=trial==3?-1:5; b[0].x=64; b[0].y=64; }
        if(trial==5) u[target].x=-1;
        sc64_target_index_begin();
        assert(sc64_target_index_active==(trial==0));
        memcpy(before,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b)); before_state=race_p7;
        sc64_p7_arbiter_cache_source_tick();
        memcpy(indexed,u,sizeof(u)); indexed_state=race_p7;
        memcpy(u,before,sizeof(u)); memcpy(b,before_buildings,sizeof(b)); race_p7=before_state;
        sc64_target_index_active=0;
        sc64_p7_arbiter_cache_source_tick();
        assert(!memcmp(indexed,u,sizeof(u))); assert(!memcmp(&indexed_state,&race_p7,sizeof(race_p7)));
        assert(!memcmp(before_buildings,b,sizeof(b)));
        if(trial==0) {
            /* Rebuild the exact production owner lists and index from the
             * original snapshot; it must match both direct and fallback. */
            memcpy(u,before,sizeof(u)); memcpy(b,before_buildings,sizeof(b)); race_p7=before_state;
            sc64_target_index_end();
            for(int k=0;k<5;++k) target_unit_head[k]=target_building_head[k]=-1;
            for(int k=MAX_UNITS-1;k>=0;--k) if(u[k].alive && !SC64_TRANSPORT_LOADED(&u[k]) && !SC64_SCARAB_HIDDEN(&u[k])) {
                int owner=u[k].owner; assert(owner>=0 && owner<5);
                target_unit_next[k]=target_unit_head[owner]; target_unit_head[owner]=k;
            }
            for(int k=MAX_BUILDINGS-1;k>=0;--k) if(b[k].alive) {
                int owner=b[k].owner; assert(owner>=0 && owner<5);
                target_building_next[k]=target_building_head[owner]; target_building_head[owner]=k;
            }
            sc64_target_index_begin_owner_lists(); assert(sc64_target_index_active);
            sc64_p7_arbiter_cache_source_tick();
            assert(!memcmp(indexed,u,sizeof(u))); assert(!memcmp(&indexed_state,&race_p7,sizeof(race_p7)));
            /* Destroy/reuse the target with a changed owner, rebuild, and
             * verify source authentication rejects the repurposed slot. */
            memcpy(u,before,sizeof(u)); race_p7=before_state;
            u[target].alive=0; sc64_target_index_end();
            u[target].alive=1; u[target].owner=(live+1)%5; u[target].weapon=0;
            sc64_target_index_begin(); assert(sc64_target_index_active);
            sc64_p7_arbiter_cache_source_tick();
            assert(!(u[target].weapon & SC64_P7_ARBITER_AURA));
        }
        assert(fwrite(u,1,sizeof(u),stdout)==sizeof(u));
        assert(fwrite(b,1,sizeof(b),stdout)==sizeof(b));
        assert(fwrite(&race_p7,1,sizeof(race_p7),stdout)==sizeof(race_p7));
        (void)arb;
    }
}

int main(void)
{
#ifdef SC64_P10_RUNTIME
    assert(SC64_RACE_SAVE_GENERATION==47u);
#elif defined(SC64_P7_TYPED_COMPACT_ENABLED)
    assert(SC64_RACE_SAVE_GENERATION==40u);
#else
    assert(SC64_RACE_SAVE_GENERATION==39u);
#endif
    aura_union_oracle();
    owner_bucket_fallback_oracle();
    return 0;
}
