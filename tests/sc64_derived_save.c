/* Asset-free packing fixture for the opt-in derived-save row codecs.
 * The synthetic constructor facts below test bit packing only; actual type
 * facts and constructor behavior are covered by engine fixtures separately. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
enum { SC_RU_COUNT = 64, SC_RB_COUNT = 64 };
typedef struct { int hp; } Sc64SyntheticRule;
static Sc64SyntheticRule sc64_race_unit_rules[SC_RU_COUNT];
static Sc64SyntheticRule sc64_race_building_rules[SC_RB_COUNT];
static int sc64_zp_actor_default_hp(int type) { (void)type; return 0; }
static int sc64_zp_building_default_hp(int type) { (void)type; return 0; }
static int sc64_race_unit_armor(int type) { return type % 11; }
typedef struct {
  int x,y,hp,owner,type,alive,selected,cd,order,tx,ty,carry,build;
  int work_timer,target,path_timer,path_retry,path_x,path_y,shield;
  int maxhp,armor,weapon,raw_type,raw_owner,flags,shield_remainder;
} Unit;
typedef struct {
  int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count;
  int queue_progress,build_progress,under_construction,level,shield,maxhp;
  int raw_type,raw_owner,flags,larva,larva_timer,shield_remainder;
} Building;
#include "../starcraft/sc64_derived_save.h"
static uint32_t rng=0x9e3779b9u;
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static int pick(int lo,int hi){return lo+(int)(rnd()%(uint32_t)(hi-lo+1));}
static void unit_make(Unit *q,int edge){
  memset(q,0,sizeof(*q)); q->type=pick(0,63); q->x=pick(0,2047); q->y=pick(0,2047);
  q->hp=edge?-1:pick(-131072,131071); q->owner=pick(0,4); q->alive=q->hp<0?0:pick(0,1);
  q->selected=pick(0,1); q->cd=pick(0,65535);q->order=pick(0,15);
  q->tx=pick(0,2047);q->ty=pick(0,2047);q->carry=edge?-65536:pick(-65536,65535);
  q->build=edge?511:pick(-512,511);q->work_timer=pick(0,65535);
  q->target=edge?511:pick(-1,511);q->path_timer=pick(0,24);q->path_retry=pick(0,12);
  q->path_x=pick(0,2047);q->path_y=pick(0,2047);q->shield=pick(0,131071);
  q->maxhp=sc64_derived_unit_hp(q->type);q->armor=sc64_race_unit_armor(q->type);
  q->weapon=pick(0,65535);q->raw_type=edge?254:pick(-1,254);
  q->raw_owner=edge?14:pick(-1,14);q->flags=edge?255:pick(0,255);
  q->shield_remainder=edge?1279:pick(0,1279);
}
static void building_make(Building *q,int edge){
  memset(q,0,sizeof(*q));q->type=pick(0,63);q->x=pick(0,2047);q->y=pick(0,2047);
  q->hp=edge?-1:pick(-131072,131071);q->owner=pick(0,4);q->alive=q->hp<0?0:pick(0,1);
  for(int i=0;i<5;i++)q->queue[i]=pick(0,63);
  q->queue_head=pick(0,4);q->queue_count=pick(0,5);q->queue_progress=pick(0,65535);
  q->build_progress=edge?-1:pick(-1,65534);q->under_construction=pick(0,1);
  q->level=edge?127:pick(0,127);q->shield=pick(0,131071);
  q->maxhp=sc64_derived_building_hp(q->type);q->raw_type=edge?254:pick(-1,254);
  q->raw_owner=edge?14:pick(-1,14);q->flags=edge?65535:pick(0,65535);
  q->larva=pick(0,3);q->larva_timer=edge?511:pick(0,511);
  q->shield_remainder=edge?1279:pick(0,1279);
}
static int unit_equal_except_weapon(const Unit*a,const Unit*b){
#define EQ(f) if(a->f!=b->f)return 0
  EQ(x);EQ(y);EQ(hp);EQ(owner);EQ(type);EQ(alive);EQ(selected);EQ(cd);EQ(order);EQ(tx);EQ(ty);EQ(carry);EQ(build);EQ(work_timer);EQ(target);EQ(path_timer);EQ(path_retry);EQ(path_x);EQ(path_y);EQ(shield);EQ(maxhp);EQ(armor);EQ(raw_type);EQ(raw_owner);EQ(flags);EQ(shield_remainder);
#undef EQ
  return b->weapon==0;
}
int main(void){
  for(int i=0;i<64;i++){sc64_race_unit_rules[i].hp=1+i*13;sc64_race_building_rules[i].hp=1+i*127;}
  /* Omitting derived maxhp must not make a dead occupied row serialize as
     an unused all-zero slot. This exact row collided before the marker. */
  {
    Building source={0},decoded;
    Sc64DerivedBuildingSave row;
    source.maxhp=sc64_derived_building_hp(0);
    source.hp=-131072;source.build_progress=-1;
    source.raw_type=source.raw_owner=-1;
    assert(sc64_derived_building_encode(&row,&source,2048,2048));
    assert(!sc64_derived_zero(row.bytes,sizeof(row.bytes)));
    assert(sc64_derived_building_decode(&decoded,&row));
    assert(!memcmp(&source,&decoded,sizeof(source)));
    Building empty={0};
    assert(sc64_derived_building_encode(&row,&empty,1,1));
    assert(sc64_derived_zero(row.bytes,sizeof(row.bytes)));
    assert(sc64_derived_building_decode(&decoded,&row));
    assert(!memcmp(&empty,&decoded,sizeof(empty)));
  }
  for(int n=0;n<100000;n++){
    Unit u,d;Sc64DerivedUnitSave ur;unit_make(&u,n%7==0);assert(sc64_derived_unit_encode(&ur,&u,2048,2048));
    memset(&d,0xA5,sizeof(d));assert(sc64_derived_unit_decode(&d,&ur,2048,2048));assert(unit_equal_except_weapon(&u,&d));
    Building b,e;Sc64DerivedBuildingSave br;building_make(&b,n%7==0);assert(sc64_derived_building_encode(&br,&b,2048,2048));
    memset(&e,0x5A,sizeof(e));assert(sc64_derived_building_decode(&e,&br));
#define BEQ(f) assert(b.f==e.f)
    BEQ(x);BEQ(y);BEQ(hp);BEQ(owner);BEQ(type);BEQ(alive);for(int j=0;j<5;j++)BEQ(queue[j]);
    BEQ(queue_head);BEQ(queue_count);BEQ(queue_progress);BEQ(build_progress);BEQ(under_construction);BEQ(level);BEQ(shield);BEQ(maxhp);BEQ(raw_type);BEQ(raw_owner);BEQ(flags);BEQ(larva);BEQ(larva_timer);BEQ(shield_remainder);
#undef BEQ
    if(n==0){
      Unit bad=u,out;Sc64DerivedUnitSave sentinel;memset(&sentinel,0xD3,sizeof(sentinel));out=bad;bad.target=512;Sc64DerivedUnitSave sentinel_before=sentinel;assert(!sc64_derived_unit_encode(&sentinel,&bad,2048,2048));assert(!memcmp(&sentinel,&sentinel_before,sizeof(sentinel)));
      Unit before=out;assert(!sc64_derived_unit_decode(&out,&ur,0,2048));assert(!memcmp(&out,&before,sizeof(out)));
      bad=u;bad.maxhp++;sentinel=(Sc64DerivedUnitSave){{0xA7}};Sc64DerivedUnitSave save=sentinel;assert(!sc64_derived_unit_encode(&sentinel,&bad,2048,2048));assert(!memcmp(&sentinel,&save,sizeof(save)));
      bad=u;bad.flags=256;sentinel=sentinel_before;assert(!sc64_derived_unit_encode(&sentinel,&bad,2048,2048));assert(!memcmp(&sentinel,&sentinel_before,sizeof(sentinel)));
      bad=u;bad.type=64;sentinel=sentinel_before;assert(!sc64_derived_unit_encode(&sentinel,&bad,2048,2048));assert(!memcmp(&sentinel,&sentinel_before,sizeof(sentinel)));
      Building bb=b,bo;Sc64DerivedBuildingSave bs={{0x6C}},bs0=bs;bb.flags=0x10000;assert(!sc64_derived_building_encode(&bs,&bb,2048,2048));assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b;bb.maxhp++;bs=bs0;assert(!sc64_derived_building_encode(&bs,&bb,2048,2048));assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bb=b;bb.type=64;bs=bs0;assert(!sc64_derived_building_encode(&bs,&bb,2048,2048));assert(!memcmp(&bs,&bs0,sizeof(bs)));
      bo=b;assert(sc64_derived_building_decode(&bo,&br));
      Unit z={0},zd;Sc64DerivedUnitSave zr;assert(sc64_derived_unit_encode(&zr,&z,1,1));assert(sc64_derived_unit_decode(&zd,&zr,1,1));assert(!memcmp(&zd,&z,sizeof(z)));
      assert(!sc64_derived_unit_decode(&zd,&zr,0,1));
      Unit keepu;memset(&keepu,0x47,sizeof(keepu));Unit keepu0=keepu;Sc64DerivedUnitSave unitpad=ur;unitpad.bytes[28]|=0x80;assert(!sc64_derived_unit_decode(&keepu,&unitpad,2048,2048));assert(!memcmp(&keepu,&keepu0,sizeof(keepu)));
      Sc64DerivedUnitSave unitmarker=ur;unitmarker.bytes[28]&=(u8)~2u;assert(!sc64_derived_unit_decode(&keepu,&unitmarker,2048,2048));assert(!memcmp(&keepu,&keepu0,sizeof(keepu)));
      Building keepb;memset(&keepb,0x47,sizeof(keepb));Building keepb0=keepb;Sc64DerivedBuildingSave bpad=br;bpad.bytes[24]|=0x80;assert(!sc64_derived_building_decode(&keepb,&bpad));assert(!memcmp(&keepb,&keepb0,sizeof(keepb)));
      Sc64DerivedBuildingSave bmarker=br;bmarker.bytes[24]&=(u8)~2u;assert(!sc64_derived_building_decode(&keepb,&bmarker));assert(!memcmp(&keepb,&keepb0,sizeof(keepb)));
      for(unsigned j=0;j<sizeof(ur.bytes)*8;j++){Sc64DerivedUnitSave x=ur;x.bytes[j/8]^=(u8)(1u<<(j%8));Unit untouched;memset(&untouched,0x71,sizeof(untouched));Unit old=untouched;if(!sc64_derived_unit_decode(&untouched,&x,2048,2048))assert(!memcmp(&untouched,&old,sizeof(old)));}
      for(unsigned j=193;j<sizeof(br.bytes)*8;j++){Sc64DerivedBuildingSave x=br;x.bytes[j/8]^=(u8)(1u<<(j%8));Building untouched;memset(&untouched,0x71,sizeof(untouched));Building old=untouched;if(!sc64_derived_building_decode(&untouched,&x))assert(!memcmp(&untouched,&old,sizeof(old)));}
    }
  }
  printf("asset-free derived codec fixture: 100000 randomized Unit/Building roundtrips; edge and transactional rejection checks passed\n");
  return 0;
}
