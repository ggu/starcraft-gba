/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free property fixture for the P7 typed rows.
 * Synthetic canonical facts test bit packing and bounds only. Actual P7
 * constructor/runtime facts remain the responsibility of engine fixtures. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define SC64_RACES 1
enum { SC_RU_COUNT = 64, SC_RB_COUNT = 100, MAX_UNITS = 240, MAX_BUILDINGS = 160 };
typedef struct { unsigned char race, flags, producer, supply; unsigned short minerals, gas, build_frames, hp, shields; unsigned short prereq; } Sc64RaceUnitRule;
typedef struct { unsigned char race, flags, worker, supply_provided; unsigned short minerals, gas, build_frames, hp; unsigned short prereq; } Sc64RaceBuildingRule;
static Sc64RaceUnitRule sc64_race_unit_rules[SC_RU_COUNT];
static Sc64RaceBuildingRule sc64_race_building_rules[SC_RB_COUNT];
static const Sc64RaceUnitRule *sc64_race_unit_rule(int type) { return type >= 0 && type < SC_RU_COUNT ? &sc64_race_unit_rules[type] : 0; }
static const Sc64RaceBuildingRule *sc64_race_building_rule(int type) { return type >= 0 && type < SC_RB_COUNT ? &sc64_race_building_rules[type] : 0; }
static int sc64_zp_actor_default_hp(int type) { (void)type; return 0; }
static int sc64_zp_actor_default_shields(int type) { (void)type; return 0; }
static int sc64_zp_building_default_hp(int type) { (void)type; return 0; }
/* Synthetic canonical-provider API; real stock capacities are checked by
 * the imported game fixture, separately from these codec property cases. */
static int sc64_race_building_max_hp(int type) {
  const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
  int hp=sc64_zp_building_default_hp(type);
  return hp?hp:rule?rule->hp:0;
}
static int sc64_race_unit_armor(int type) { return type % 11; }
typedef struct {
  int hp, maxhp, carry, shield;
  int x, y, tx, ty, build, target, path_x, path_y, raw_type, raw_owner;
  int cd, work_timer, weapon, path_timer, path_retry, flags, shield_remainder;
  int owner, type, alive, selected, order, armor;
} Unit;
typedef struct {
  int x,y,hp,maxhp,owner,type,alive,queue[5],queue_head,queue_count;
  int queue_progress,build_progress,under_construction,level,shield;
  int raw_type,raw_owner,flags,larva,larva_timer,shield_remainder;
} Building;
#include "../starcraft/sc64_p7_typed_unit_save.h"
#include "../starcraft/sc64_p7_typed_building_save.h"
#include "../starcraft/sc64_zp_unit_save.h"
#include "../starcraft/sc64_compact_building_save.h"

static uint32_t rng = 0x49d3a21bu;
static uint32_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static int pick(int lo, int hi) { return lo + (int)(rnd() % (uint32_t)(hi - lo + 1)); }
static void init_facts(void) {
  for (int i=0;i<SC_RB_COUNT;i++) sc64_race_building_rules[i].hp=(unsigned short)(100+i*991);
  for (int i=0;i<64;i++) {
    sc64_race_unit_rules[i].hp=(unsigned short)(100+i*997);
    sc64_race_unit_rules[i].shields=(unsigned short)(i*5);
  }
}
static int unit_hp(int t) { return sc64_race_unit_rules[t].hp; }
static int unit_shield_max(int t) { return sc64_race_unit_rules[t].shields; }
static int building_hp(int t) { return sc64_race_building_rules[t].hp; }
static void unit_make(Unit *q,int type,int edge) {
  memset(q,0,sizeof(*q)); q->type=type; q->maxhp=unit_hp(type); q->armor=sc64_race_unit_armor(type);
  q->x=edge?2047:pick(0,2047); q->y=edge?0:pick(0,2047); q->hp=edge?q->maxhp:pick(1,q->maxhp);
  q->owner=pick(0,4); q->alive=edge?1:pick(0,1); q->selected=pick(0,1); q->cd=pick(0,255);
  q->order=pick(0,15); q->tx=pick(0,2047); q->ty=pick(0,2047); q->carry=edge?-65536:pick(-65536,65535);
  q->build=edge?-2:pick(-2,239); q->work_timer=edge?2047:pick(0,2047); q->weapon=edge?65535:pick(0,65535);
  q->target=edge?MAX_UNITS+MAX_BUILDINGS-1:pick(-1,MAX_UNITS+MAX_BUILDINGS-1);
  q->path_timer=edge?24:pick(0,24); q->path_retry=edge?12:pick(0,12); q->path_x=pick(0,2047); q->path_y=pick(0,2047);
  q->shield=edge?unit_shield_max(type):pick(0,unit_shield_max(type)); q->raw_type=edge?254:pick(-1,254); q->raw_owner=edge?14:pick(-1,14);
  q->flags=edge?0x3fff:pick(0,0x3fff); q->shield_remainder=edge?1279:pick(0,1279);
}
static void building_make(Building *q,int type,int edge) {
  memset(q,0,sizeof(*q)); q->type=type; q->maxhp=building_hp(type); q->x=edge?2047:pick(0,2047); q->y=edge?0:pick(0,2047);
  q->hp=edge?q->maxhp:pick(1,q->maxhp); q->owner=pick(0,4); q->alive=edge?1:pick(0,1);
  for(int i=0;i<5;i++) q->queue[i]=edge?(i+59):pick(0,63);
  q->queue_head=edge?4:pick(0,4); q->queue_count=edge?5:pick(0,5); q->queue_progress=edge?65535:pick(0,65535);
  q->build_progress=edge?65534:pick(-1,65534); q->under_construction=pick(0,1); q->level=edge?127:pick(0,127);
  q->shield=edge?131071:pick(0,131071); q->raw_type=edge?254:pick(-1,254); q->raw_owner=edge?14:pick(-1,14);
  q->flags=edge?65535:pick(0,65535); q->larva=edge?3:pick(0,3); q->larva_timer=edge?511:pick(0,511); q->shield_remainder=edge?1279:pick(0,1279);
}
#define UEQ(a,b,f) assert((a).f==(b).f)
static void unit_equal(const Unit *a,const Unit *b) {
  UEQ(*a,*b,hp);UEQ(*a,*b,maxhp);UEQ(*a,*b,carry);UEQ(*a,*b,shield);UEQ(*a,*b,x);UEQ(*a,*b,y);UEQ(*a,*b,tx);UEQ(*a,*b,ty);UEQ(*a,*b,build);UEQ(*a,*b,target);UEQ(*a,*b,path_x);UEQ(*a,*b,path_y);UEQ(*a,*b,raw_type);UEQ(*a,*b,raw_owner);UEQ(*a,*b,cd);UEQ(*a,*b,work_timer);UEQ(*a,*b,weapon);UEQ(*a,*b,path_timer);UEQ(*a,*b,path_retry);UEQ(*a,*b,flags);UEQ(*a,*b,shield_remainder);UEQ(*a,*b,owner);UEQ(*a,*b,type);UEQ(*a,*b,alive);UEQ(*a,*b,selected);UEQ(*a,*b,order);UEQ(*a,*b,armor);
}
static void building_equal(const Building *a,const Building *b) {
#define BEQ(f) assert(a->f==b->f)
  BEQ(x);BEQ(y);BEQ(hp);BEQ(maxhp);BEQ(owner);BEQ(type);BEQ(alive);for(int i=0;i<5;i++)assert(a->queue[i]==b->queue[i]);BEQ(queue_head);BEQ(queue_count);BEQ(queue_progress);BEQ(build_progress);BEQ(under_construction);BEQ(level);BEQ(shield);BEQ(raw_type);BEQ(raw_owner);BEQ(flags);BEQ(larva);BEQ(larva_timer);BEQ(shield_remainder);
#undef BEQ
}
static void unit_setbits(Sc64P7TypedUnitSave *r,unsigned bit,unsigned width,unsigned value) { for(unsigned i=0;i<width;i++) { u8 m=(u8)(1u<<((bit+i)&7u)); if((value>>i)&1u) r->bytes[(bit+i)>>3]|=m; else r->bytes[(bit+i)>>3]&=(u8)~m; } }
static void building_setbits(Sc64P7TypedBuildingSave *r,unsigned bit,unsigned width,unsigned value) { for(unsigned i=0;i<width;i++) { u8 m=(u8)(1u<<((bit+i)&7u)); if((value>>i)&1u) r->bytes[(bit+i)>>3]|=m; else r->bytes[(bit+i)>>3]&=(u8)~m; } }
static void reject_unit_decode_unchanged(const Sc64P7TypedUnitSave *row,unsigned ww,unsigned wh) {
  Unit out,before; memset(&out,0x69,sizeof(out)); before=out; assert(!sc64_p7_typed_unit_decode(&out,row,ww,wh)); assert(!memcmp(&out,&before,sizeof(out)));
}
static void reject_building_decode_unchanged(const Sc64P7TypedBuildingSave *row,unsigned ww,unsigned wh) {
  Building out,before; memset(&out,0x96,sizeof(out)); before=out; assert(!sc64_p7_typed_building_decode(&out,row,ww,wh)); assert(!memcmp(&out,&before,sizeof(out)));
}
int main(void) {
  init_facts();
  for(int t=0;t<64;t++) {
    Unit u,d; Sc64P7TypedUnitSave tr; unit_make(&u,t,1);
    assert(sc64_p7_typed_unit_encode(&tr,&u,2048,2048)); assert(sc64_p7_typed_unit_decode(&d,&tr,2048,2048)); unit_equal(&u,&d);
    Sc64ZpPackedUnitSave lr; Unit ld; assert(sc64_zp_unit_save_encode(&lr,&u,2048,2048)); assert(sc64_zp_unit_save_decode(&ld,&lr,2048,2048)); unit_equal(&u,&ld);
    Building b,bd; Sc64P7TypedBuildingSave br; building_make(&b,t,1);
    assert(sc64_p7_typed_building_encode(&br,&b,2048,2048)); assert(sc64_p7_typed_building_decode(&bd,&br,2048,2048)); building_equal(&b,&bd);
    Sc64PackedBuildingSave oldbr; Building oldbd; assert(sc64_building_save_encode(&oldbr,&b,2048,2048)); assert(sc64_building_save_decode(&oldbd,&oldbr)); building_equal(&b,&oldbd);
  }
  {
    Unit z={0},zd; Sc64P7TypedUnitSave zr; assert(sc64_p7_typed_unit_encode(&zr,&z,1,1)); assert(sc64_p7_typed_unit_zero(&zr)); assert(sc64_p7_typed_unit_decode(&zd,&zr,1,1)); assert(!memcmp(&z,&zd,sizeof(z)));
    Building z2={0},z2d; Sc64P7TypedBuildingSave zbr; assert(sc64_p7_typed_building_encode(&zbr,&z2,1,1)); assert(sc64_p7_typed_building_zero(&zbr)); assert(sc64_p7_typed_building_decode(&z2d,&zbr,1,1)); assert(!memcmp(&z2,&z2d,sizeof(z2)));
    Unit dead; Building deadb; unit_make(&dead,63,1); dead.alive=0; building_make(&deadb,63,1); deadb.alive=0;
    assert(sc64_p7_typed_unit_encode(&zr,&dead,2048,2048)&&!sc64_p7_typed_unit_zero(&zr)); assert(sc64_p7_typed_unit_decode(&zd,&zr,2048,2048)); unit_equal(&dead,&zd);
    assert(sc64_p7_typed_building_encode(&zbr,&deadb,2048,2048)&&!sc64_p7_typed_building_zero(&zbr)); assert(sc64_p7_typed_building_decode(&z2d,&zbr,2048,2048)); building_equal(&deadb,&z2d);
  }
  {
    Unit u={0},d; Sc64P7TypedUnitSave row; u.type=0;u.maxhp=unit_hp(0);u.armor=sc64_race_unit_armor(0);u.hp=0;u.alive=0;u.owner=0;u.x=0;u.y=0;u.tx=0;u.ty=0;u.carry=-65536;u.build=-2;u.target=-1;u.path_x=0;u.path_y=0;u.raw_type=-1;u.raw_owner=-1;
    assert(sc64_p7_typed_unit_encode(&row,&u,2048,2048)); assert(sc64_p7_typed_unit_decode(&d,&row,2048,2048)); unit_equal(&u,&d);
    Building b={0},bd; Sc64P7TypedBuildingSave br; b.type=0;b.maxhp=building_hp(0);b.x=0;b.y=0;b.hp=0;b.raw_type=-1;b.raw_owner=-1;b.build_progress=-1;
    assert(sc64_p7_typed_building_encode(&br,&b,2048,2048)); assert(sc64_p7_typed_building_decode(&bd,&br,2048,2048)); building_equal(&b,&bd);
  }
  {
    const int hp_edges[]={-131072,-1,0,131071};
    for(unsigned i=0;i<sizeof(hp_edges)/sizeof(hp_edges[0]);++i) {
      Unit u,d; Building b,bd; Sc64P7TypedUnitSave ur; Sc64P7TypedBuildingSave br;
      unit_make(&u,0,1); building_make(&b,0,1);
      u.alive=0; b.alive=0; u.hp=b.hp=hp_edges[i];
      assert(sc64_p7_typed_unit_encode(&ur,&u,2048,2048));
      assert(sc64_p7_typed_unit_decode(&d,&ur,2048,2048)); unit_equal(&u,&d);
      assert(sc64_p7_typed_building_encode(&br,&b,2048,2048));
      assert(sc64_p7_typed_building_decode(&bd,&br,2048,2048)); building_equal(&b,&bd);
    }
  }
  for(int n=0;n<12000;n++) {
    Unit u,d; Building b,bd; Sc64P7TypedUnitSave ur; Sc64P7TypedBuildingSave br;
    int type=n%64; unit_make(&u,type,n%11==0); building_make(&b,type,n%13==0);
    if(n%9==0) { u.alive=0; b.alive=0; u.hp=-pick(1,131072); b.hp=-pick(1,131072); }
    assert(sc64_p7_typed_unit_encode(&ur,&u,2048,2048)); memset(&d,0xa5,sizeof(d)); assert(sc64_p7_typed_unit_decode(&d,&ur,2048,2048)); unit_equal(&u,&d);
    assert(sc64_p7_typed_building_encode(&br,&b,2048,2048)); memset(&bd,0x5a,sizeof(bd)); assert(sc64_p7_typed_building_decode(&bd,&br,2048,2048)); building_equal(&b,&bd);
    Sc64ZpPackedUnitSave legacy_unit; Unit legacy_unit_out;
    assert(sc64_zp_unit_save_encode(&legacy_unit,&u,2048,2048));
    assert(sc64_zp_unit_save_decode(&legacy_unit_out,&legacy_unit,2048,2048));
    unit_equal(&u,&legacy_unit_out);
    Sc64PackedBuildingSave legacy_building; Building legacy_building_out;
    assert(sc64_building_save_encode(&legacy_building,&b,2048,2048));
    assert(sc64_building_save_decode(&legacy_building_out,&legacy_building));
    building_equal(&b,&legacy_building_out);
    if(n==0) {
      Sc64P7TypedUnitSave us; memset(&us,0xc3,sizeof(us)); Sc64P7TypedUnitSave us0=us; Unit bad=u; bad.type=64; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.flags=0x4000; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.maxhp++; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.weapon=-1; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.x=2048; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.target=MAX_UNITS+MAX_BUILDINGS; assert(!sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); assert(!memcmp(&us,&us0,sizeof(us)));
      bad=u; bad.type=63; bad.maxhp=unit_hp(63); bad.armor=sc64_race_unit_armor(63); bad.weapon=0xffff; assert(sc64_p7_typed_unit_encode(&us,&bad,2048,2048)); Unit edgeout; assert(sc64_p7_typed_unit_decode(&edgeout,&us,2048,2048)); unit_equal(&bad,&edgeout);
      Sc64P7TypedBuildingSave bs; memset(&bs,0x3c,sizeof(bs)); Sc64P7TypedBuildingSave bs0=bs; Building bb=b; bb.type=100; assert(!sc64_p7_typed_building_encode(&bs,&bb,2048,2048)); assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b; bb.flags=65536; assert(!sc64_p7_typed_building_encode(&bs,&bb,2048,2048)); assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b; bb.queue[4]=64; assert(!sc64_p7_typed_building_encode(&bs,&bb,2048,2048)); assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b; bb.queue_count=6; assert(!sc64_p7_typed_building_encode(&bs,&bb,2048,2048)); assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b; bb.flags=0xffff; for(int j=0;j<5;j++) bb.queue[j]=63; assert(sc64_p7_typed_building_encode(&bs,&bb,2048,2048)); Building bout; assert(sc64_p7_typed_building_decode(&bout,&bs,2048,2048)); building_equal(&bb,&bout);
      /* The extension occupies padding after the legacy end marker, so a
       * pre-extension type-63 row and a new type-99 row both round-trip. */
      building_make(&bb,63,1); assert(sc64_p7_typed_building_encode(&bs,&bb,2048,2048));
      assert(!(bs.bytes[24]&0x04u)); assert(sc64_p7_typed_building_decode(&bout,&bs,2048,2048)); building_equal(&bb,&bout);
      building_make(&bb,99,1); assert(sc64_p7_typed_building_encode(&bs,&bb,2048,2048));
      assert(bs.bytes[24]&0x04u); assert(sc64_p7_typed_building_decode(&bout,&bs,2048,2048)); building_equal(&bb,&bout);
      for(unsigned i=0;i<6;i++) building_setbits(&bs,43+i,1,(36u>>i)&1u);
      Building bsentinel; memset(&bout,0x74,sizeof(bout)); bsentinel=bout;
      assert(!sc64_p7_typed_building_decode(&bout,&bs,2048,2048));
      assert(!memcmp(&bout,&bsentinel,sizeof(bout)));
      Sc64P7TypedUnitSave badrow=ur; badrow.bytes[27]&=(u8)~0x80u; reject_unit_decode_unchanged(&badrow,2048,2048);
      badrow=ur; unit_setbits(&badrow,40,3,7); reject_unit_decode_unchanged(&badrow,2048,2048);
      badrow=ur; unit_setbits(&badrow,146,5,31); reject_unit_decode_unchanged(&badrow,2048,2048); reject_unit_decode_unchanged(&ur,0,2048);
      Sc64P7TypedBuildingSave badbr=br; badbr.bytes[24]&=(u8)~0x02u; reject_building_decode_unchanged(&badbr,2048,2048); reject_building_decode_unchanged(&br,0,2048);
      badbr=br; badbr.bytes[24]|=0x80u; reject_building_decode_unchanged(&badbr,2048,2048);
      badbr=br; building_setbits(&badbr,40,3,7); reject_building_decode_unchanged(&badbr,2048,2048);
      badbr=br; building_setbits(&badbr,80,3,7); reject_building_decode_unchanged(&badbr,2048,2048);
      badbr=br; building_setbits(&badbr,83,3,7); reject_building_decode_unchanged(&badbr,2048,2048);
      Unit uout,ubefore; memset(&uout,0x47,sizeof(uout)); ubefore=uout; assert(!sc64_p7_typed_unit_decode(&uout,&ur,2049,2048)); assert(!memcmp(&uout,&ubefore,sizeof(uout)));
      Building bout2,bbefore; memset(&bout2,0x74,sizeof(bout2)); bbefore=bout2; assert(!sc64_p7_typed_building_decode(&bout2,&br,2049,2048)); assert(!memcmp(&bout2,&bbefore,sizeof(bout2)));
    }
  }
  printf("asset-free typed codec fixture: 12000 randomized pairs plus 64/64 types, boundaries, markers, padding, rejection atomicity, legacy-field comparisons passed\n");
  return 0;
}
