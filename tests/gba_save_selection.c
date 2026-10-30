/* SPDX-License-Identifier: GPL-3.0-or-later
 * In-process SRAM journal oracle. No production test hooks or private inputs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
static const u32 tags[3]={0x10203040u,0x20304050u,0x30405060u};
static u8 scratch[64], before[32768];
static int validate(const void *data,u32 len,u32 tag,void *ctx){
 const u8 *p=data;(void)ctx;return len==4 && p[0]==(u8)(tag>>24) && p[1]==0xa5;
}
static void record(int slot,u32 tag,u32 seq,u8 accept){
 volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;u8 value[4]={(u8)(tag>>24),accept,7,9};
 sram_put32(p,0);sram_put32(p+4,sizeof(value));sram_put32(p+8,hash_bytes(value,sizeof(value)));sram_put32(p+12,seq);
 for(unsigned i=0;i<sizeof(value);i++)p[16+i]=value[i];sram_put32(p,tag);
}
static void reset(void){memset(host_save,0,sizeof(host_save));memset(scratch,0,sizeof(scratch));}
static GbaSaveSelection select_record(void){GbaSaveSelection s={0};assert(save_select_validated(scratch,sizeof(scratch),tags,3,validate,0,&s));return s;}
static void selection_unchanged_failure(const u32 *ts,unsigned n,u32 cap){GbaSaveSelection s={11,22,33,44,55},expected=s;assert(!save_select_validated(scratch,cap,ts,n,validate,0,&s));assert(!memcmp(&s,&expected,sizeof(s)));}
static void assert_unchanged_write(const GbaSaveSelection *s){const u8 payload[]={0x30,0xa5,4,5};memcpy(before,host_save,sizeof(before));assert(!save_write_selected(payload,sizeof(payload),tags[2],s));assert(!memcmp(before,host_save,sizeof(before)));}
int main(void){gba_init();
 /* Every old/new tag permutation, each slot order, and preserved predecessor. */
 for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++)for(int order=0;order<2;order++){
  reset();record(order,tags[a],10,0xa5);record(1-order,tags[b],11,0xa5);
  GbaSaveSelection s=select_record();assert(s.slot==1-order&&s.tag==tags[b]&&s.sequence==11&&scratch[0]==(u8)(tags[b]>>24));
  u8 predecessor[SAVE_SLOT_SIZE];memcpy(predecessor,host_save+(unsigned)s.slot*SAVE_SLOT_SIZE,sizeof(predecessor));
  const u8 payload[]={0x30,0xa5,4,5};assert(save_write_selected(payload,sizeof(payload),tags[2],&s));
  assert(!memcmp(predecessor,host_save+(unsigned)s.slot*SAVE_SLOT_SIZE,sizeof(predecessor)));
  GbaSaveSelection now=select_record();assert(now.slot==order&&now.tag==tags[2]&&now.sequence==12&&!memcmp(scratch,payload,4));
 }
 /* All tag pairs and slot positions: semantically invalid newer data must
  * not shadow a valid older generation. */
 for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++)for(int order=0;order<2;order++){
  reset();record(order,tags[a],10,0xa5);record(1-order,tags[b],11,0);
  GbaSaveSelection s=select_record();assert(s.slot==order && s.tag==tags[a] && s.sequence==10);
 }
 /* Wrap ordering, exact tie (slot 0), and the undefined half-range. */
 reset();record(0,tags[0],0xffffffffu,0xa5);record(1,tags[1],1,0xa5);assert(select_record().slot==1);
 reset();record(0,tags[1],7,0xa5);record(1,tags[2],7,0xa5);assert(select_record().slot==0);
 reset();record(0,tags[0],1,0xa5);record(1,tags[1],0x80000001u,0xa5);selection_unchanged_failure(tags,3,sizeof(scratch));
 /* Semantically bad but checksummed current falls back to older readable. */
 reset();record(0,tags[0],1,0xa5);record(1,tags[2],9,0);GbaSaveSelection old=select_record();assert(old.slot==0);
 u8 saved[SAVE_SLOT_SIZE];memcpy(saved,host_save,sizeof(saved));const u8 payload[]={0x30,0xa5,1,2};assert(save_write_selected(payload,4,tags[2],&old));assert(!memcmp(saved,host_save,sizeof(saved)));
 /* Incomplete/torn records and checksum errors never displace old records. */
 reset();record(0,tags[0],5,0xa5);record(1,tags[2],8,0xa5);host_save[SAVE_SLOT_SIZE+16]^=1;assert(select_record().slot==0);
 record(1,tags[2],8,0xa5);sram_put32(host_save+SAVE_SLOT_SIZE,0);assert(select_record().slot==0);
 /* A rechecksummed replacement with identical tag/length/sequence is stale. */
 reset();record(0,tags[0],5,0xa5);GbaSaveSelection stale=select_record();record(0,tags[0],5,0xa5);host_save[18]^=1;sram_put32(host_save+8,hash_bytes(host_save+16,4));assert_unchanged_write(&stale);
 stale=select_record();GbaSaveSelection bad=stale;bad.slot=2;assert_unchanged_write(&bad);bad=stale;bad.sequence++;assert_unchanged_write(&bad);bad=stale;bad.length++;assert_unchanged_write(&bad);bad=stale;bad.tag=tags[1];assert_unchanged_write(&bad);bad=stale;bad.checksum++;assert_unchanged_write(&bad);
 /* Fresh storage writes safely; a checksummed unknown record is protected. */
 reset();assert(save_write_first(payload,4,tags[2]));assert(select_record().sequence==1);
 memcpy(before,host_save,sizeof(before));assert(!save_write_first(payload,4,tags[2]));assert(!memcmp(before,host_save,sizeof(before)));
 reset();record(1,0x55667788u,8,0xa5);memcpy(before,host_save,sizeof(before));assert(!save_write_first(payload,4,tags[2]));assert(!memcmp(before,host_save,sizeof(before)));
 reset();record(0,tags[0],2,0xa5);host_save[16]^=1;assert(save_write_first(payload,4,tags[2]));assert(select_record().sequence==1);
 /* Reject duplicate/zero tags, excessive scratch and absent candidates. */
 {u32 dup[3]={tags[0],tags[0],tags[2]},zero[3]={tags[0],0,tags[2]};selection_unchanged_failure(dup,3,sizeof(scratch));selection_unchanged_failure(zero,3,sizeof(scratch));selection_unchanged_failure(tags,0,sizeof(scratch));selection_unchanged_failure(tags,3,SAVE_PAYLOAD_MAX+1);}
 reset();selection_unchanged_failure(tags,3,sizeof(scratch));
 puts("validated SRAM selection: 3-generation ordering, semantic fallback, wrap and atomic predecessor preservation passed");return 0;
}
