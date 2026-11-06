/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic versioned-record and atomic predecessor fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/sc64_protoss_research_padding_probe.h"
static const u32 versions[]={0x10203040u,0x30405060u};
enum { ROWS=160, WIDTH=28, OFFSET=1024, PAYLOAD=16368 };
static u8 scratch[PAYLOAD], normalized[ROWS*WIDTH], original[PAYLOAD], encoded[PAYLOAD], before[32768];
static int live=77;
static int prepare(const void *p,u32 len,u32 tag,void *ctx) {
 const u8 *q=p; Sc64ProtossResearchLevels levels; (void)ctx;
 if(len!=PAYLOAD)return 0;
 if(tag==versions[0]) {
  for(unsigned i=0;i<ROWS;i++)if(q[OFFSET+i*WIDTH+WIDTH-1]&0xfe)return 0;
  return 1;
 }
 if(!sc64_pr_pad_transform(normalized,sizeof(normalized),&levels,q+OFFSET,ROWS*WIDTH,0,8,
                            SC64_PR_PAD_COMPACT,ROWS,tag,versions[1],versions,1,0))return 0;
 /* A real adapter must run its existing row and context validators here. */
 return 1;
}
static void record(unsigned slot,u32 tag,u32 sequence,const u8 *data) {
 volatile u8 *p=host_save+slot*SAVE_SLOT_SIZE;
 sram_put32(p,0); sram_put32(p+4,PAYLOAD); sram_put32(p+8,hash_bytes(data,PAYLOAD)); sram_put32(p+12,sequence);
 for(unsigned i=0;i<PAYLOAD;i++)p[16+i]=data[i]; sram_put32(p,tag);
}
int main(void) {
 Sc64ProtossResearchLevels levels={0}; GbaSaveSelection sel={0},next={0};
 for(unsigned i=0;i<PAYLOAD;i++)original[i]=(u8)(i*31u+17u);
 memset(original+OFFSET,0,ROWS*WIDTH); memcpy(encoded,original,PAYLOAD);
 levels.level[0][0]=3; levels.level[7][4]=2;
 assert(sc64_pr_pad_transform(encoded+OFFSET,ROWS*WIDTH,0,original+OFFSET,ROWS*WIDTH,&levels,8,
                              SC64_PR_PAD_COMPACT,ROWS,versions[1],versions[1],versions,1,1));
 assert(!memcmp(original,encoded,OFFSET));
 assert(!memcmp(original+OFFSET+ROWS*WIDTH,encoded+OFFSET+ROWS*WIDTH,PAYLOAD-OFFSET-ROWS*WIDTH));
 memset(host_save,0,sizeof(host_save)); record(0,versions[0],10,original);
 encoded[OFFSET+13*WIDTH+WIDTH-1]|=0x80; /* unused bit 97 */
 record(1,versions[1],11,encoded); memcpy(before,host_save,sizeof(before));
 assert(save_select_prepared(scratch,sizeof(scratch),versions,2,prepare,0,&sel));
 assert(sel.slot==0 && sel.tag==versions[0] && live==77 && !memcmp(before,host_save,sizeof(before)));
 encoded[OFFSET+13*WIDTH+WIDTH-1]&=0x7f;
 assert(save_write_selected(encoded,PAYLOAD,versions[1],&sel,versions,2));
 assert(!memcmp(before,host_save,SAVE_SLOT_SIZE));
 memcpy(before,host_save,sizeof(before));
 assert(save_select_prepared(scratch,sizeof(scratch),versions,2,prepare,0,&next));
 assert(next.slot==1 && next.tag==versions[1] && next.sequence==11 && live==77 && !memcmp(before,host_save,sizeof(before)));
 puts("research journal fallback and predecessor: PASS"); return 0;
}
