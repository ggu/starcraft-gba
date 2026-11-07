/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic CPU initialization; no game or ROM is loaded. */
#include <ares/ares.hpp>
#include <n64/n64.hpp>
#include "../../experiments/ares-store-probe/store_recorder.h"
#include "../../experiments/ares-store-probe/store_hook.h"
#include <cassert>
static Sc64StoreRecorder recorder;
extern "C" void sc64_store_instruction_begin(std::uint64_t pc,std::uint32_t op,std::uint64_t base,std::uint64_t ra,std::uint64_t a0,std::uint64_t a1,std::uint64_t a2,std::uint64_t a3){recorder.instruction_begin(pc,op,base,ra,a0,a1,a2,a3);}
extern "C" void sc64_store_instruction_end(){recorder.instruction_end();}
extern "C" void sc64_store_committed(std::uint64_t va,std::uint32_t pa,unsigned width,std::uint64_t data){recorder.committed(va,pa,width,data);}
int main(){
 using namespace ares::Nintendo64;
 constexpr std::uint64_t site=0xffffffff8013b410ull, target=0xffffffff8000c390ull;
 constexpr std::uint32_t jal=0x0c0030e4;
 cpu.ipu.r[4].u64=0x12345678;recorder.armed=true;recorder.entry_enabled=true;
 cpu.pipeline.setPc(site);
 cpu.pipeline.begin();cpu.decoderEXECUTE(jal);cpu.instructionEpilogue<false>();cpu.pipeline.end();
 assert(cpu.ipu.r[31].u64==site+8 && cpu.ipu.pc==site+4);
 cpu.pipeline.begin();cpu.decoderEXECUTE(0);cpu.instructionEpilogue<false>();cpu.pipeline.end();
 assert(cpu.ipu.pc==target);
 sc64_store_instruction_begin(cpu.ipu.pc,0x27bdffe0,cpu.ipu.r[29].u64,cpu.ipu.r[31].u64,cpu.ipu.r[4].u64,cpu.ipu.r[5].u64,cpu.ipu.r[6].u64,cpu.ipu.r[7].u64);
 assert(recorder.entry_count==1 && recorder.entries[0].pc==target && recorder.entries[0].ra==site+8 && recorder.entries[0].a0==0x12345678);
 sc64_store_instruction_end();
}
