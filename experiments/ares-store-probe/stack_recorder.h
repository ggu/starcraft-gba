/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STACK_RECORDER_H
#define SC64_STACK_RECORDER_H
#include "../ares-headless/observer_read.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
struct Sc64StackEvent {std::uint64_t pc,ra,sp;std::uint32_t step,reserved;std::uint8_t bytes[256];};
static_assert(sizeof(Sc64StackEvent)==288,"stack-event ABI");
struct Sc64StackRecorder {
 static constexpr unsigned limit=8;Sc64StackEvent records[limit]{};unsigned count=0,skipped=0;bool failed=false,enabled=false;
 template<class Hit,class Cache,class Ram>
 void capture(std::uint64_t pc,std::uint32_t opcode,std::uint64_t sp,std::uint64_t ra,std::uint64_t a0,std::uint32_t step,std::size_t size,Hit hit,Cache cache,Ram ram) {
  if(!enabled || pc!=0xffffffff8000c390ull || a0!=0xffffffff802ffd10ull)return;
  if(count==limit){++skipped;return;}
  auto hi=sp>>32;auto lo=std::uint32_t(sp);auto seg=lo&0xe0000000u;
  if(opcode!=0x27bdffe0u || hi!=0xffffffffu || (seg!=0x80000000u && seg!=0xa0000000u) || (lo&7u) || size!=8u*1024u*1024u) {failed=true;return;}
  std::uint32_t physical=lo&0x1fffffffu;Sc64StackEvent scratch{};
  if(!sc64_read_ram(physical,sizeof scratch.bytes,size,scratch.bytes,hit,cache,ram)){failed=true;return;}
  scratch.pc=pc;scratch.ra=ra;scratch.sp=sp;scratch.step=step;records[count++]=scratch;
 }
 bool publish(const char* path) const {
  if(failed || !count || !path)return false;
  int fd=::open(path,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)return false;
  std::size_t n=count*sizeof(Sc64StackEvent),done=0;auto data=reinterpret_cast<const std::uint8_t*>(records);
  while(done<n){auto w=::write(fd,data+done,n-done);if(w<=0){::close(fd);return false;}done+=static_cast<std::size_t>(w);}
  return ::close(fd)==0;
 }
};
static_assert(4096u*48u+1024u*56u+Sc64StackRecorder::limit*sizeof(Sc64StackEvent)+64u<=256u*1024u,"event budget");
#endif
