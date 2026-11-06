/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#include "../starcraft/sc64_q8_typed_unit_wire.h"
int main(void) {
 int max=0,max_type=-1, count=0;
 for(int t=0;t<SC_RU_COUNT;++t) {
  int hp,armor,shield;
  if(!sc64_p7_typed_unit_canonical(t,&hp,&armor,&shield))continue;
  ++count; if(hp>max){max=hp;max_type=t;}
  printf("type=%d hp=%d shields=%d\n",t,hp,shield);
  assert(hp<=1023);
  Unit q={0},r; Sc64P7TypedUnitSave old,newrow,decoded;
  q.type=t;q.alive=1;q.hp=hp;q.maxhp=hp;q.armor=armor;q.build=-1;q.target=-1;q.raw_type=-1;q.raw_owner=-1;
  assert(sc64_p7_typed_unit_encode(&old,&q,2048,2048));
  for(unsigned f=0;f<256;++f) {
   unsigned f1=f,f2=0; assert(sc64_q8_typed_unit_transform(&newrow,&old,&f1,1,2048,2048));
   assert(sc64_q8_typed_unit_transform(&decoded,&newrow,&f2,0,2048,2048));
   assert(f==f2 && !memcmp(&old,&decoded,sizeof(old)));
   assert(sc64_p7_typed_unit_decode(&r,&decoded,2048,2048) && r.hp==hp);
  }
  q.alive=0;q.hp=-131072;assert(sc64_p7_typed_unit_encode(&old,&q,2048,2048));
  unsigned zero=0;assert(sc64_q8_typed_unit_transform(&newrow,&old,&zero,1,2048,2048));
  assert(sc64_q8_typed_unit_transform(&decoded,&newrow,&zero,0,2048,2048));assert(!memcmp(&old,&decoded,sizeof(old)));
 }
 printf("count=%d max=%d type=%d\n",count,max,max_type);return 0;
}
