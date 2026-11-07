/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-store-probe/stack_recorder.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(){
 Sc64StackRecorder r;r.enabled=true;constexpr auto pc=0xffffffff8000c390ull,sp=0xffffffff80002000ull,actor=0xffffffff802ffd10ull;
 unsigned calls=0;auto hit=[&](std::uint32_t x){++calls;return x%2==0?1:0;};auto cache=[](std::uint32_t x){return std::uint8_t(x);};auto ram=[](std::uint32_t x){return std::uint8_t(x+1);};
 r.capture(pc,0x27bdffe0,sp,7,actor,4204,8u*1024u*1024u,hit,cache,ram);assert(r.count==1&&!r.failed&&calls==256);
 for(unsigned i=0;i<256;++i)assert(r.records[0].bytes[i]==std::uint8_t(i+(i%2)));
 auto old=calls;r.capture(pc,0x27bdffe0,sp+1,7,actor,4204,8u*1024u*1024u,hit,cache,ram);assert(r.failed&&r.count==1&&calls==old);
 Sc64StackRecorder x;x.enabled=true;x.capture(pc,0x27bdffe0,0xffffffff807fff80ull,7,actor,4204,8u*1024u*1024u,hit,cache,ram);assert(x.failed&&x.count==0&&calls==old);
 Sc64StackRecorder conflict;conflict.enabled=true;auto bad=[](std::uint32_t a){return a==0x2008?-1:0;};conflict.capture(pc,0x27bdffe0,sp,7,actor,4204,8u*1024u*1024u,bad,cache,ram);assert(conflict.failed&&!conflict.count);
 char dir[]="/tmp/sc64-stack-XXXXXX";assert(mkdtemp(dir));char path[256];assert(std::snprintf(path,sizeof(path),"%s/stack",dir)>0);assert(!conflict.publish(path)&&access(path,F_OK)!=0);
 Sc64StackRecorder valid;valid.enabled=true;valid.capture(pc,0x27bdffe0,sp,7,actor,4204,8u*1024u*1024u,hit,cache,ram);assert(valid.publish(path));assert(!valid.publish(path));
 int fd=open(path,O_RDONLY);Sc64StackEvent copy{};assert(fd>=0&&read(fd,&copy,sizeof copy)==sizeof copy&&close(fd)==0);assert(std::memcmp(&copy,&valid.records[0],sizeof copy)==0);assert(unlink(path)==0&&rmdir(dir)==0);
 for(unsigned i=1;i<8;++i)valid.capture(pc,0x27bdffe0,sp,7,actor,4204,8u*1024u*1024u,hit,cache,ram);valid.capture(pc,0x27bdffe0,sp,7,actor,4204,8u*1024u*1024u,hit,cache,ram);assert(valid.count==8&&valid.skipped==1&&!valid.failed);
}
