/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-store-probe/store_recorder.h"
#include <cassert>
#include <vector>
int main(){
 auto r=new Sc64StoreRecorder;constexpr auto b=Sc64StoreRecorder::begin;
 r->committed(0,b,4,1);assert(r->count==0);
 r->armed=true;r->instruction_begin(0x80001234,0xac830004,0xffffffff80001000ull);
 r->committed(0,b-4,4,1);r->committed(0,0xa0000000,4,1);r->committed(0,b,3,1);assert(r->count==0);
 for(unsigned w:{1u,2u,4u,8u})r->committed(0x80000000ull+b,b,w,0x1234);
 assert(r->count==4 && r->events[0].pc==0x80001234 && r->events[0].opcode==0xac830004 && r->events[0].base_index==4 && r->events[0].base==0xffffffff80001000ull);
 r->instruction_end();r->committed(0,b,4,0);assert(r->count==4);
 r->instruction_begin(9,0,0);for(unsigned i=4;i<r->limit;++i)r->committed(0,b,1,i);r->committed(0,b,1,0);assert(r->count==r->limit && r->overflow);
 std::vector<std::uint8_t> mem(Sc64StoreRecorder::ram_size);assert(!sc64_store_baseline_valid(mem.data(),mem.size()));assert(!sc64_store_baseline_valid(nullptr,mem.size()));
 auto put=[&](std::size_t x,std::uint32_t v){mem[x]=v>>24;mem[x+1]=v>>16;mem[x+2]=v>>8;mem[x+3]=v;};
 put(0xe02ec,0x802ffaa0);put(b,0x802ffbd8);put(b+4,0x802ffe48);put(b+8,5120);mem[b+76]=1;mem[b+101]=64;
 assert(sc64_store_baseline_valid(mem.data(),mem.size()));mem[b+76]=0;assert(!sc64_store_baseline_valid(mem.data(),mem.size()));delete r;
}
