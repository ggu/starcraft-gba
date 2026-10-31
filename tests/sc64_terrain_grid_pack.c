/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare packed base terrain to the existing bit-at-a-time construction. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
#include "sc64_terrain_grid_pack.h"
static u8 src[16416], before[2072], expected[2072], actual[2072];
static unsigned state=0x61fc3421u;
static unsigned rnd(void){state=state*1664525u+1013904223u;return state;}
int main(void){
 unsigned tests=0;
 for (unsigned n=0;n<=16384;n+=n<256?1:137) {
  for(unsigned i=0;i<sizeof(src);i++)src[i]=(u8)(rnd()>>24);
  for(unsigned i=0;i<sizeof(before);i++)before[i]=(u8)(rnd()>>24);
  memcpy(expected,before,sizeof(before));memcpy(actual,before,sizeof(before));
  for(unsigned i=0;i<n;i++){
   u8 bit=(u8)(1u<<(i&7u));
   if(!(src[i]&1u))expected[1+(i>>3)]|=bit;
   else expected[1+(i>>3)]&=(u8)~bit;
  }
  sc64_terrain_grid_pack(actual+1,src,n);
  assert(!memcmp(expected,actual,sizeof(actual)));tests++;
 }
 puts("terrain pack randomized odd sizes, tails and untouched guards: PASS");
 printf("%u cases\n",tests);return 0;
}
