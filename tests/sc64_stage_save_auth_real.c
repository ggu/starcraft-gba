/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authenticates the concrete four-product tag pairs, without journal writes. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_stage_save_auth.h"
#if !defined(SC64_SHARED_TRANSPORT) || !defined(SC64_WORKER_CARGO_KIND)
#error "This binding requires the canonical cargo/transport pair"
#endif
int main(void)
{
    enum { EGG=1,GUARDIAN=2 };
    const u32 gi=SAVE_VERSION ^ 0x10000000u;
    const u32 go=SC64_TRANSPORT_SAVE_VERSION ^ 0x10000000u;
    const Sc64StageTagPair tags[]={
      {SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0},
      {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0},
      {go,gi,GUARDIAN},
      {SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION,EGG},
      {go ^ 0x40000000u,gi ^ 0x40000000u,EGG|GUARDIAN}
    };
    int candidate;Sc64StageSaveAuth auth;
    for(unsigned i=0;i<5;i++) {
        assert(sc64_stage_save_authenticate(&auth,&candidate,tags[i].outer,tags[i].inner,tags,5));
        assert(sc64_stage_save_auth_matches(&auth,&candidate,tags[i].inner,tags[i].features));
        assert(!sc64_stage_save_authenticate(&auth,&candidate,tags[i].outer,tags[(i+1)%5].inner,tags,5));
        assert(!sc64_stage_save_auth_matches(&auth,&candidate,tags[i].inner,0));
    }
    for(unsigned i=0;i<5;i++)printf("%08lx/%08lx%s",(unsigned long)tags[i].outer,
                                    (unsigned long)tags[i].inner,i==4?"\n":" ");
    puts("concrete stage tag pair checks passed");return 0;
}
