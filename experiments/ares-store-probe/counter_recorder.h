/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_COUNTER_RECORDER_H
#define SC64_COUNTER_RECORDER_H
#include "store_recorder.h"
#include "stack_recorder.h"
struct Sc64CounterRecorder {
 static constexpr unsigned limit=96;static constexpr std::uint32_t begin=0xa5acc,end=0xa5ad1;
 Sc64StoreEvent events[limit]{};unsigned count=0;bool enabled=false,overflow=false;
 void committed(const Sc64StoreRecorder& source,std::uint64_t va,std::uint32_t pa,unsigned width,std::uint64_t data) {
  if(!enabled || !source.armed || !source.active || (width!=1 && width!=2 && width!=4 && width!=8) || pa>=source.ram_size || width>source.ram_size-pa || pa>=end || pa+width<=begin)return;
  if(count==limit){overflow=true;return;}
  const auto masked=width==8 ? data : data & ((std::uint64_t{1}<<(width*8))-1);
  events[count++]={source.pc,va,masked,source.base,source.opcode,pa,source.step,static_cast<std::uint16_t>(width),static_cast<std::uint16_t>((source.opcode>>21)&31)};
 }
};
static_assert(4096u*48u+1024u*56u+8u*288u+Sc64CounterRecorder::limit*48u+64u<=256u*1024u,"combined event budget");
#endif
