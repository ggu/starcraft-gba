/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free, synthetic-tag proof; no runtime save adapter is enabled. */
#define main existing_codec_fixture_main
#include "sc64_p7_typed_codec.c"
#undef main
#include "../starcraft/sc64_protoss_research_padding_probe.h"
static uint8_t source[4480],wire[4480],normalized[4480],before[4480];
static const uint32_t oldtags[]={0x10203040u,0x20304050u};
#define NEWTAG 0x30405060u
static void one(Sc64PrPadFormat fmt,size_t rows,int alive,unsigned owners) {
 size_t w=fmt==SC64_PR_PAD_COMPACT?28:25, n=rows*w; unsigned used=fmt==SC64_PR_PAD_COMPACT?1:3;
 Sc64ProtossResearchLevels lv={0},out={0}; Building b,decoded;
 memset(source,0,n); init_facts();
 for(size_t i=0;i<rows;i++) {
  if(alive) { building_make(&b,1,1); b.alive=1; }
  else memset(&b,0,sizeof(b));
  if(fmt==SC64_PR_PAD_COMPACT) assert(sc64_building_save_encode((Sc64PackedBuildingSave *)(void *)(source+i*w),&b,2048,2048));
  else assert(sc64_p7_typed_building_encode((Sc64P7TypedBuildingSave *)(void *)(source+i*w),&b,2048,2048));
 }
 for(unsigned o=0;o<owners;o++)for(unsigned f=0;f<6;f++)lv.level[o][f]=(f==5)?1:(o+f)%4;
 memcpy(before,source,n);
 assert(sc64_pr_pad_transform(wire,n,0,source,n,&lv,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,1));
 assert(!memcmp(source,before,n));
 if(fmt==SC64_PR_PAD_COMPACT) assert(!sc64_building_save_decode(&decoded,(const Sc64PackedBuildingSave *)(const void *)wire));
 else assert(!sc64_p7_typed_building_decode(&decoded,(const Sc64P7TypedBuildingSave *)(const void *)wire,2048,2048));
 assert(sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,0));
 assert(!memcmp(&lv,&out,sizeof(lv)) && !memcmp(source,normalized,n));
 for(size_t i=0;i<rows;i++) {
  if(fmt==SC64_PR_PAD_COMPACT) assert(sc64_building_save_decode(&decoded,(const Sc64PackedBuildingSave *)(const void *)(normalized+i*w)));
  else assert(sc64_p7_typed_building_decode(&decoded,(const Sc64P7TypedBuildingSave *)(const void *)(normalized+i*w),2048,2048));
 }
 memset(normalized,0xa5,n); memset(&out,0xa5,sizeof(out)); memcpy(before,normalized,n);
 assert(!sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,oldtags[0],NEWTAG,oldtags,2,0));
 assert(!memcmp(before,normalized,n));
 assert(!sc64_pr_pad_transform(wire,n,&out,wire,n,0,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,0));
 assert(!sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,oldtags[0],oldtags[0],oldtags,2,0));
 assert(!memcmp(before,normalized,n));
 /* First unused padding bit, after the 96 research bits. */
 size_t r=96/(8-used); unsigned bit=used+96%(8-used);
 wire[r*w+w-1]|=(uint8_t)(1u<<bit);
 assert(!sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,0));
 assert(!memcmp(before,normalized,n)); wire[r*w+w-1]&=(uint8_t)~(1u<<bit);
 /* Custom contexts cannot accept owner 5's first bit. */
 if(owners==5) { size_t k=60; r=k/(8-used); bit=used+k%(8-used); wire[r*w+w-1]|=(uint8_t)(1u<<bit);
  assert(!sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,0)); wire[r*w+w-1]&=(uint8_t)~(1u<<bit); }
 /* Singularity is one bit semantically; set its upper bit. */
 { size_t k=11; r=k/(8-used); bit=used+k%(8-used); wire[r*w+w-1]|=(uint8_t)(1u<<bit);
  assert(!sc64_pr_pad_transform(normalized,n,&out,wire,n,0,owners,fmt,rows,NEWTAG,NEWTAG,oldtags,2,0)); }
}
int main(void) {
 for(int alive=0;alive<=1;alive++)for(unsigned owners=5;owners<=8;owners+=3) {
  one(SC64_PR_PAD_COMPACT,96,alive,owners); one(SC64_PR_PAD_COMPACT,160,alive,owners); one(SC64_PR_PAD_TYPED,160,alive,owners);
 }
 puts("research padding codec: PASS"); return 0;
}
