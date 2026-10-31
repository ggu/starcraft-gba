/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#define MAX_UNITS 240
#define MAX_BUILDINGS 160
#include "../starcraft/sc64_impact_events.h"
#include "../starcraft/sc64_scarab_impact_packet.h"
int main(void)
{
    const int coords[]={0,1,15,16,2047,2048,4094,4095};
    const int targets[]={-1,0,239,240,399};
    for(unsigned x=0;x<sizeof(coords)/sizeof(*coords);x++)
      for(unsigned y=0;y<sizeof(coords)/sizeof(*coords);y++)
       for(unsigned t=0;t<sizeof(targets)/sizeof(*targets);t++)
        for(int owner=0;owner<5;owner++)for(unsigned ticks=1;ticks<=6;ticks++) {
          Sc64ScarabImpact in={coords[x],coords[y],targets[t],owner,ticks},out;
          Sc64ImpactEvent row;assert(sc64_scarab_impact_pack(&row,&in));
          assert(row.source_type==SC64_SCARAB_IMPACT_MARKER);
          assert(!sc64_impact_event_valid(&row,400,5,SC64_IMPACT_TYPE_LIMIT));
          assert(sc64_scarab_impact_unpack(&row,&out));
          assert(out.x==in.x && out.y==in.y && out.target==in.target && out.owner==in.owner && out.ticks==in.ticks);
          sc64_scarab_impact_forget_target(&row);assert(sc64_scarab_impact_unpack(&row,&out));
          assert(out.target==-1 && out.x==in.x && out.y==in.y && out.owner==in.owner && out.ticks==in.ticks);
        }
    Sc64ImpactEvent row;Sc64ScarabImpact p={4096,0,-1,0,1},out;
    assert(!sc64_scarab_impact_pack(&row,&p));p.x=0;p.target=400;assert(!sc64_scarab_impact_pack(&row,&p));
    p.target=-1;p.owner=5;assert(!sc64_scarab_impact_pack(&row,&p));p.owner=0;p.ticks=7;assert(!sc64_scarab_impact_pack(&row,&p));
    p.ticks=1;assert(sc64_scarab_impact_pack(&row,&p));
    { uint8_t bytes[sizeof(Sc64ImpactSaveHeader)+sizeof(Sc64ImpactEvent)];Sc64ImpactEvent candidate;
      unsigned len=sc64_impact_encode(&row,1,bytes);
      assert(sc64_impact_decode(bytes,len,1,400,5,SC64_IMPACT_TYPE_LIMIT,&candidate)<0);
    }
    row.steps|=128;assert(!sc64_scarab_impact_unpack(&row,&out));
    puts("Scarab shared impact packet bounds, identity and invalidation passed");return 0;
}
