/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Frozen baseline from commit 5ad0adc; independent of the candidate. */
static int sc64_p7_count_snapshot_reference(Sc64P7WorldContext *ctx)
{
    Sc64P7CountSnapshot next = {{0},0};
    static const u8 selector[8] = {13,17,17,17,19,18,18,19};
    static const u8 wanted[8] = {229,78,79,75,229,229,154,154};
    if (!ctx) return 0;
    ctx->count_snapshot.active=0;
    for (unsigned q=0;q<8;++q) {
        unsigned trigger = q==0 ? 0 : q<=3 ? q : q+2;
        const Sc64P7SourceCondition *c;
        if (!sc64_p7_condition_row(trigger,0,&c) ||
            c->opcode!=SC64_P7_CONDITION_COMMAND || c->player!=selector[q] ||
            c->unit!=wanted[q] || c->location || c->resource || c->mask ||
            c->flags!=0x10u) return 0;
    }
    for (int pass=0;pass<2;++pass) {
        int limit=pass ? MAX_BUILDINGS : MAX_UNITS;
        for (int i=0;i<limit;++i) {
            int alive=pass ? b[i].alive : u[i].alive;
            int raw_owner=pass ? b[i].raw_owner : u[i].raw_owner;
            int live_owner=pass ? b[i].owner : u[i].owner;
            if (!alive) continue;
            int raw=pass ? (b[i].raw_type>=0 ? b[i].raw_type : sc64_race_raw_building(b[i].type)) :
                           (u[i].raw_type>=0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type));
            int owner13=sc64_p7_world_owner_matches(ctx,13,raw_owner,live_owner);
            int owner17=sc64_p7_world_owner_matches(ctx,17,raw_owner,live_owner);
            int owner19=sc64_p7_world_owner_matches(ctx,19,raw_owner,live_owner);
            int owner18=sc64_p7_world_owner_matches(ctx,18,raw_owner,live_owner);
            if (owner13<0 || owner17<0 || owner19<0 || owner18<0) return 0;
            if (!(owner13 || owner17 || owner19 || owner18)) continue;
            if (raw<0 || raw>227) return 0;
            if (owner13) ++next.count[0];
            if (owner17) {
                for (unsigned q=1;q<=3;++q) {
                    int match=sc64_p7_world_type_match(wanted[q],(unsigned)raw,pass);
                    if (match<0) return 0;
                    if (match) ++next.count[q];
                }
            }
            if (owner19) ++next.count[4];
            if (owner18) {
                ++next.count[5];
                int match=sc64_p7_world_type_match(154,(unsigned)raw,pass);
                if (match<0) return 0;
                if (match) ++next.count[6];
            }
            if (owner19) {
                int match=sc64_p7_world_type_match(154,(unsigned)raw,pass);
                if (match<0) return 0;
                if (match) ++next.count[7];
            }
        }
    }
    next.active=1;
    ctx->count_snapshot=next;
    return 1;
}

static unsigned seed=0x9172a6u;
static unsigned rnd(void){ seed=seed*1664525u+1013904223u;return seed >> 8; }
static int source_for_force(unsigned force)
{
    for (unsigned i = 0; i < 8; ++i)
        if (race_p7_world.source_slot_active[i] &&
            race_p7_world.source_force_for_slot[i] == force) return (int)i;
    return -1;
}

static void check_all_rows(void)
{
    int s0=source_for_force(0), s1=source_for_force(1);
    assert(s0 >= 0 && s1 >= 0);
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    const int raws[] = {75, 78, 79};
    for (int i=0; i<3; ++i) {
        int type=sc64_race_unit_from_raw(raws[i]); assert(type>=0);
        u[i].alive=1; u[i].raw_type=raws[i]; u[i].type=type;
        u[i].raw_owner=s0; u[i].owner=race_p7_world.live_owner_for_source[s0];
    }
    u[3]=u[0]; u[3].owner=SIDE; /* selector 13 */
    int type=sc64_race_building_from_raw(154); assert(type>=0);
    for (int i=0;i<2;++i) {
        int source=i?s1:s0;
        b[i].alive=1; b[i].raw_type=154; b[i].type=type;
        b[i].raw_owner=source; b[i].owner=race_p7_world.live_owner_for_source[source];
    }
    Sc64P7WorldContext old=race_p7_world, cur=race_p7_world;
    assert(sc64_p7_count_snapshot_reference(&old));
    assert(sc64_p7_count_snapshot_begin(&cur));
    assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
    for (unsigned i=0;i<8;++i) assert(cur.count_snapshot.count[i]>0);

    /* A matching raw id of the wrong kind must not count as the other kind. */
    static const int unit_ids[] = {75, 78, 79};
    for (unsigned i=0;i<3;++i) {
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
        assert(sc64_race_building_from_raw(unit_ids[i]) < 0);
        b[0].alive=1;b[0].raw_type=unit_ids[i];b[0].raw_owner=s0;
        b[0].owner=race_p7_world.live_owner_for_source[s0];
        old=race_p7_world;cur=race_p7_world;
        old.count_snapshot.count[0]=cur.count_snapshot.count[0]=0xabcdef01u;
        old.count_snapshot.active=cur.count_snapshot.active=1;
        assert(sc64_p7_count_snapshot_reference(&old));
        assert(sc64_p7_count_snapshot_begin(&cur));
        assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
        assert(cur.count_snapshot.active && cur.count_snapshot.count[1+i]==0);
    }
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    assert(sc64_race_unit_from_raw(154) < 0);
    u[0].alive=1;u[0].raw_type=154;u[0].raw_owner=s0;
    u[0].owner=race_p7_world.live_owner_for_source[s0];
    old=race_p7_world;cur=race_p7_world;
    old.count_snapshot.count[0]=cur.count_snapshot.count[0]=0xabcdef01u;
    old.count_snapshot.active=cur.count_snapshot.active=1;
    assert(sc64_p7_count_snapshot_reference(&old));
    assert(sc64_p7_count_snapshot_begin(&cur));
    assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
    assert(cur.count_snapshot.active && cur.count_snapshot.count[6]==0 && cur.count_snapshot.count[7]==0);

    /* An inactive source rejects; a mismatched live owner still follows the
       original selector rules rather than inventing a new validity check. */
    u[0].raw_type=75;u[0].type=sc64_race_unit_from_raw(75);
    old=race_p7_world;cur=race_p7_world;
    old.source_slot_active[s0]=cur.source_slot_active[s0]=0;
    old.count_snapshot.active=cur.count_snapshot.active=1;
    assert(!sc64_p7_count_snapshot_reference(&old));
    assert(!sc64_p7_count_snapshot_begin(&cur));
    assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
    assert(!cur.count_snapshot.active);
    old=race_p7_world;cur=race_p7_world;u[0].owner=SIDE;
    assert(sc64_p7_count_snapshot_reference(&old));
    assert(sc64_p7_count_snapshot_begin(&cur));
    assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
}
int main(void){
 assert(!sc64_p7_count_snapshot_reference(0));
 assert(!sc64_p7_count_snapshot_begin(0));
 assert(race_p7_bind_world(&sc64_race_maps[SC64_P7_GLOBAL_ID]));
 check_all_rows();
 /* Populated valid worlds exercise combinations of owners, units and buildings. */
 for (unsigned run=0; run<500; ++run) {
  memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
  for (int i=0;i<60;++i) {
   static const int ids[]={0,75,78,79,84};
   int raw=ids[rnd()%5], type=sc64_race_unit_from_raw(raw);
   if(type<0) continue;
   int source=(rnd()&1)?source_for_force(0):source_for_force(1);
   u[i].alive=1;u[i].raw_type=raw;u[i].type=type;u[i].raw_owner=source;
   u[i].owner=race_p7_world.live_owner_for_source[source];
  }
  int bt=sc64_race_building_from_raw(154); assert(bt>=0);
  for(int i=0;i<20;++i) {
   int source=(rnd()&1)?source_for_force(0):source_for_force(1);
   b[i].alive=1;b[i].raw_type=154;b[i].type=bt;b[i].raw_owner=source;
   b[i].owner=race_p7_world.live_owner_for_source[source];
  }
  Sc64P7WorldContext a=race_p7_world,c=race_p7_world;
  assert(sc64_p7_count_snapshot_reference(&a));
  assert(sc64_p7_count_snapshot_begin(&c));
  assert(!memcmp(&a.count_snapshot,&c.count_snapshot,sizeof(a.count_snapshot)));
 }
 unsigned successes=0, failures=0, nonzero=0;
 const int raws[]={0,75,78,79,84,154,227,228,255,-1};
 for(unsigned run=0;run<3000;run++){
  memset(u,0,sizeof(u));memset(b,0,sizeof(b));
  for(int i=0;i<MAX_UNITS;i++)if(run%4 && rnd()%7==0){
   u[i].alive=1;u[i].type=rnd()%10;u[i].owner=rnd()%5;u[i].raw_owner=rnd()%8;
   u[i].raw_type=raws[rnd()%10];
  }
  for(int i=0;i<MAX_BUILDINGS;i++)if(run%4 && rnd()%9==0){
   b[i].alive=1;b[i].type=rnd()%10;b[i].owner=rnd()%5;b[i].raw_owner=rnd()%8;
   b[i].raw_type=raws[rnd()%10];
  }
  if(run%4==0) {
   static const int desired[]={75,78,79,154};
   for(int j=0;j<4;j++) {
    int raw=desired[j], type=sc64_race_unit_from_raw(raw);
    if(type<0)continue;
    for(int source=0;source<8;source++) {
     int live=race_p7_world.live_owner_for_source[source];
     if(live>=0 && sc64_p7_world_owner_matches(&race_p7_world,17,source,live)==1) {
      u[j].alive=1;u[j].raw_type=raw;u[j].type=type;u[j].raw_owner=source;u[j].owner=live;break;
     }
    }
   }
  }
  if(run%7==0){u[0].alive=1;u[0].raw_owner=-1;}
  if(run%11==0){b[0].alive=1;b[0].raw_type=255;}
  Sc64P7WorldContext old=race_p7_world, cur=race_p7_world;
  for(unsigned k=0;k<8;k++)old.count_snapshot.count[k]=cur.count_snapshot.count[k]=0x12340000u+k;
  old.count_snapshot.active=cur.count_snapshot.active=1;
  int x=sc64_p7_count_snapshot_reference(&old),y=sc64_p7_count_snapshot_begin(&cur);
  assert(x==y);assert(!memcmp(&old.count_snapshot,&cur.count_snapshot,sizeof(old.count_snapshot)));
  successes+=x!=0;failures+=x==0;
  if(x)for(unsigned k=0;k<8;k++) if(cur.count_snapshot.count[k]){nonzero++;break;}
 }
 assert(successes && failures && nonzero);printf("count snapshot differential: %u success, %u rejected, %u nonzero\n",successes,failures,nonzero);return 0;
}
