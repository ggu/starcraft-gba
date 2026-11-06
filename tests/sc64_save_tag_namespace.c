/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exhaust the current base/generation families against every combination of
 * historical selector bits, including roster, cargo, BC and actor stages. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_save_tag_namespace.h"
static void check(uint32_t base) {
 assert(SC64_SAVE_TAG_BASE_VALID(base));
 for(unsigned bits=0;bits<256;bits++) {
  uint32_t old=(base&0x00ffffffu)|(bits<<24);
  assert(SC64_SAVE_TAG_BASE_VALID(old));
  uint32_t research=SC64_SAVE_TAG_RESEARCH(old);
  uint32_t jobs=SC64_SAVE_TAG_RESEARCH_JOBS(old);
  uint32_t campaign=SC64_SAVE_TAG_CAMPAIGN_JOBS(old);
  assert((campaign&SC64_SAVE_TAG_NAMESPACE_MASK)==SC64_SAVE_TAG_CAMPAIGN_JOBS_NAMESPACE);
  assert((campaign&0xff00ffffu)==(old&0xff00ffffu));
  assert((jobs&SC64_SAVE_TAG_NAMESPACE_MASK)==SC64_SAVE_TAG_RESEARCH_JOBS_NAMESPACE);
  assert((jobs&0xff00ffffu)==(old&0xff00ffffu));
  assert(jobs!=research);
  assert((research&SC64_SAVE_TAG_NAMESPACE_MASK)==SC64_SAVE_TAG_RESEARCH_NAMESPACE);
  assert((research&0xff00ffffu)==(old&0xff00ffffu));
  for(unsigned other=0;other<256;other++) {
   uint32_t existing=(base&0x00ffffffu)|(other<<24);
   assert(research!=existing);
   assert(jobs!=existing);
   assert(jobs!=SC64_SAVE_TAG_RESEARCH(existing));
   assert(campaign!=existing);
   assert(campaign!=SC64_SAVE_TAG_RESEARCH(existing));
   assert(campaign!=SC64_SAVE_TAG_RESEARCH_JOBS(existing));
  }
 }
}
int main(void) {
 static const uint32_t fixed[]={0x53435536u,0x53435535u,0x53435534u,0x53435532u,0x53435531u,0x53435231u,0x53435431u,0x53434232u,0x53434b31u,0x53433631u,0x53434733u};
 static const uint32_t race[]={0x53435430u,0x53435a30u,0x53435030u};
 for(unsigned i=0;i<sizeof(fixed)/sizeof(*fixed);i++)check(fixed[i]);
 for(unsigned i=0;i<sizeof(race)/sizeof(*race);i++)for(unsigned gen=0;gen<=49;gen++)check(race[i]+gen);
 /* Previous failed proposal collides exactly with the roster variant. */
 assert((0x56435a60u^0x80000000u)==0xd6435a60u);
 assert(SC64_SAVE_TAG_RESEARCH(0x56435a60u)==0x56445a60u);
 puts("save tag namespace inventory: PASS");return 0;
}
