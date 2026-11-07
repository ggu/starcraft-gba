/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../../experiments/ares-store-probe/store_recorder.h"
#include <cassert>
int main(){
 auto p=new Sc64StoreRecorder;p->armed=true;p->entry_enabled=true;p->step=4202;
 p->instruction_begin(0xffffffff8000c394ull,1,0,2,3,4,5,6);assert(p->entry_count==0);
 p->instruction_begin(0xffffffff8000c390ull,0x27bdffe0,0,0xffffffff8013b418ull,1,2,3,4);
 assert(p->entry_count==1 && p->entries[0].ra==0xffffffff8013b418ull && p->entries[0].a3==4 && p->entries[0].step==4202);
 p->instruction_begin(0xffffffff80122904ull,0x240200e4,0,0xffffffff80008b18ull);
 assert(p->entry_count==2 && p->entries[1].ra==0xffffffff80008b18ull);
 for(unsigned i=2;i<p->entry_limit;++i)p->instruction_begin(0xffffffff8000c390ull,0,0);
 p->instruction_begin(0xffffffff8000c390ull,0,0);assert(p->entry_count==p->entry_limit && p->entry_overflow);
 delete p;
}
