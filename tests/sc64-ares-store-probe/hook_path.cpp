/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source-shaped fake: verifies recorder contract, not a compiled ares core. */
#include "../../experiments/ares-store-probe/store_recorder.h"
#include <cassert>
struct FakeCPU {
 Sc64StoreRecorder &rec; unsigned writes=0; unsigned value=0;
 void execute(std::uint64_t actual_pc,std::uint32_t opcode,std::uint64_t base,bool translated,bool conditional,unsigned width,unsigned pa,unsigned data) {
  rec.instruction_begin(actual_pc,opcode,base);
  if(translated && conditional) {value=data;++writes;rec.committed(base,pa,width,data);}
  rec.instruction_end();
 }
};
int main(){
 auto r=new Sc64StoreRecorder;r->armed=true;FakeCPU c{*r};constexpr auto b=Sc64StoreRecorder::begin;
 c.execute(0x80000104,0xac830004,0xffffffff80001000ull,false,true,4,b,1);assert(!r->count && !c.writes);
 c.execute(0x80000108,0xe0830004,0xffffffff80001000ull,true,false,4,b,2);assert(!r->count && !c.writes);
 c.execute(0x8000010c,0xac830004,0xffffffff80001000ull,true,true,4,b,3);assert(r->count==1 && c.value==3 && r->events[0].pc==0x8000010c && r->events[0].base==0xffffffff80001000ull);
 // Source-shaped unaligned store: components preserve ordering and common instruction PC.
 r->instruction_begin(0x80000204,0xb0830001,0x80002000);r->committed(0x80002001,b,1,4);r->committed(0x80002002,b+1,2,5);r->instruction_end();
 assert(r->count==3 && r->events[1].pc==r->events[2].pc && r->events[1].width==1 && r->events[2].width==2);
 delete r;
}
