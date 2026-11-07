#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise the exact patched prologue/decoder/end snippet with fake CPU state."""
import argparse,pathlib
p=argparse.ArgumentParser();p.add_argument('--cpu',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();s=a.cpu.read_text();start=s.index('  instructionPrologue(ipu.pc, *data);');end=s.index('  instructionEpilogue<0>();',start);body=s[start:end]
assert 'sc64_store_instruction_begin' in body and 'sc64_store_instruction_end' in body
prefix='''#include "store_recorder.h"
#include <cassert>
#include <cstdint>
#define SC64_OBSERVER_CPU_STORE 1
struct Reg{std::uint64_t u64=0;};struct IPU{std::uint64_t pc;Reg r[32];} ipu;
static Sc64StoreRecorder rec;static int stage=0;
void instructionPrologue(std::uint64_t,std::uint32_t){assert(stage==0);stage=1;}
extern "C" void sc64_store_instruction_begin(std::uint64_t p,std::uint32_t op,std::uint64_t base,std::uint64_t ra,std::uint64_t a0,std::uint64_t a1,std::uint64_t a2,std::uint64_t a3){assert(stage==1);stage=2;rec.instruction_begin(p,op,base,ra,a0,a1,a2,a3);}
void decoderEXECUTE(std::uint32_t){assert(stage==2 && rec.active);stage=3;rec.committed(0x802ffd10,0x2ffd10,1,7);}
extern "C" void sc64_store_instruction_end(){assert(stage==3);stage=4;rec.instruction_end();}
void exercise(std::uint32_t* data){
'''
suffix='''}
int main(){rec.armed=true;rec.entry_enabled=true;rec.step=4202;ipu.pc=0xffffffff8000c390ull;ipu.r[31].u64=0xffffffff8013b418ull;ipu.r[4].u64=11;std::uint32_t op=0x27bdffe0;exercise(&op);assert(stage==4 && !rec.active && rec.count==1 && rec.entry_count==1);assert(rec.entries[0].ra==0xffffffff8013b418ull && rec.entries[0].a0==11 && rec.entries[0].opcode==op && rec.events[0].pc==ipu.pc);}
'''
a.output.write_text(prefix+body+suffix)
