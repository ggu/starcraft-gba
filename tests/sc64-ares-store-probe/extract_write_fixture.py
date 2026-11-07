#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a stub harness around the exact patched CPU::write template body."""
import argparse,pathlib
p=argparse.ArgumentParser();p.add_argument('--memory',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
s=a.memory.read_text();start=s.index('template<u32 Size>\nauto CPU::write(PhysAccess access, u64 data) -> bool {');end=s.index('\ntemplate<u32 Size>\nauto CPU::writeDebug',start);body=s[start:end]
assert 'sc64_store_committed(access.vaddr,paddr,Size,data)' in body
prefix='''#include "store_recorder.h"
#include <cassert>
using u32=std::uint32_t;using u64=std::uint64_t;
static Sc64StoreRecorder recorder;
extern "C" void sc64_store_committed(u64 va,u32 pa,unsigned w,u64 d){recorder.committed(va,pa,w,d);}
namespace GDB {struct Server {void reportMemWrite(u64,u32){}} server;}
struct CPU {struct PhysAccess {bool found,cache;u32 paddr;u64 vaddr;explicit operator bool()const{return found;}};
 struct Context {bool littleEndian()const{return false;}} context;
 struct Cache {unsigned calls=0;template<u32 S>void write(u64,u32,u64){++calls;}} dcache;
 unsigned bus=0;template<u32 S>void busWrite(u32,u64){++bus;}
 template<u32 S> bool write(PhysAccess,u64);
};
template<u32 S>u32 reverseEndianPaddr(u32 a){return a;}
#define SC64_OBSERVER_CPU_STORE 1
'''
suffix='''
int main(){auto b=Sc64StoreRecorder::begin; CPU c;recorder.armed=true;recorder.instruction_begin(0x80000104,0xac830004,0x80001000);
 assert(!c.write<4>({false,false,b,0x802ffd10},0x1234));assert(recorder.count==0 && c.bus==0);
 assert(c.write<1>({true,true,b,0x802ffd10},0x1234));assert(c.dcache.calls==1 && recorder.count==1 && recorder.events[0].data==0x34);
 assert(c.write<2>({true,false,b+1,0x802ffd11},0x123456));assert(c.bus==1 && recorder.count==2 && recorder.events[1].data==0x3456);
 assert(c.write<4>({true,false,0x10000000,0xa0000000},7));assert(recorder.count==2);
 recorder.instruction_end();assert(c.write<4>({true,false,b,0x802ffd10},9));assert(recorder.count==2);
}
'''
a.output.write_text(prefix+body+suffix)
