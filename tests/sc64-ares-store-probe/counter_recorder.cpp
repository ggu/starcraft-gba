/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-store-probe/counter_recorder.h"
#include <cassert>
int main(){
 auto s=new Sc64StoreRecorder;Sc64CounterRecorder c;constexpr auto b=Sc64CounterRecorder::begin;
 s->armed=true;s->instruction_begin(0xffffffff80023954ull,0xac825acc,0xffffffff800a0000ull);s->step=4202;
 c.committed(*s,0xffffffff800a5accull,b,4,0x123456789abcdef0ull);assert(!c.count);
 c.enabled=true;c.committed(*s,0,b-4,4,1);c.committed(*s,0,b,3,1);c.committed(*s,0,8u*1024u*1024u,1,1);assert(!c.count);
 c.committed(*s,0xffffffff800a5accull,b,4,0x123456789abcdef0ull);assert(c.count==1&&c.events[0].data==0x9abcdef0&&c.events[0].pc==0xffffffff80023954ull);
 c.committed(*s,0xffffffff800a5ad0ull,b+4,1,0x123);assert(c.count==2&&c.events[1].data==0x23);
 s->instruction_end();c.committed(*s,0,b,1,1);assert(c.count==2);
 s->instruction_begin(9,0,0);for(unsigned i=2;i<c.limit;++i)c.committed(*s,0,b,1,i);c.committed(*s,0,b,1,0);assert(c.count==c.limit&&c.overflow);delete s;
}
