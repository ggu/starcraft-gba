/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STORE_RECORDER_H
#define SC64_STORE_RECORDER_H
#include <cstdint>
#include <cstddef>
struct Sc64StoreEvent {
 std::uint64_t pc, vaddr, data, base;
 std::uint32_t opcode, paddr, step;
 std::uint16_t width, base_index;
};
static_assert(sizeof(Sc64StoreEvent)==48,"store-event ABI");
struct Sc64StoreRecorder {
 static constexpr unsigned limit=4096;
 static constexpr std::uint32_t begin=0x2ffd10, end=begin+312, ram_size=8u*1024u*1024u;
 Sc64StoreEvent events[limit]{};
 std::uint64_t pc=0,base=0;std::uint32_t opcode=0,step=0;unsigned count=0;bool armed=false,active=false,overflow=false;
 void instruction_begin(std::uint64_t p,std::uint32_t op,std::uint64_t b) {if(!armed)return;pc=p;opcode=op;base=b;active=true;}
 void instruction_end(){active=false;}
 void committed(std::uint64_t va,std::uint32_t pa,unsigned width,std::uint64_t data) {
  if(!armed || !active || (width!=1 && width!=2 && width!=4 && width!=8) || pa>=ram_size || width>ram_size-pa || pa>=end || pa+width<=begin)return;
  if(count==limit){overflow=true;return;}
  events[count++]={pc,va,data,base,opcode,pa,step,static_cast<std::uint16_t>(width),static_cast<std::uint16_t>((opcode>>21)&31)};
 }
};
static_assert(sizeof(Sc64StoreEvent)*Sc64StoreRecorder::limit+64<=256u*1024u,"store output budget");
inline std::uint32_t sc64_store_be32(const std::uint8_t *p) {return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];}
inline bool sc64_store_baseline_valid(const std::uint8_t *p,std::size_t n) {
 if(!p || n!=Sc64StoreRecorder::ram_size)return false;
 constexpr auto b=Sc64StoreRecorder::begin;
 return sc64_store_be32(p+0xe02ec)==0x802ffaa0 && sc64_store_be32(p+b+8)==5120 && p[b+76]==1 && p[b+100]==0 && p[b+101]==64 && sc64_store_be32(p+b)==0x802ffbd8 && sc64_store_be32(p+b+4)==0x802ffe48;
}
#endif
