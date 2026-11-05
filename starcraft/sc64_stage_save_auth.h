/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STAGE_SAVE_AUTH_H
#define SC64_STAGE_SAVE_AUTH_H
#include <stdint.h>
/* This context is local to one prepared candidate. The caller must first
 * validate the journal checksum and sequence, then pass that slot's outer
 * tag and its detached payload's unmodified inner version. No field value
 * inside the payload alone can authorize a staged interpretation. */
typedef struct {
    uint32_t outer,inner;
    unsigned features;
} Sc64StageTagPair;
typedef struct {
    const void *candidate;
    uint32_t outer,inner;
    unsigned features;
} Sc64StageSaveAuth;
static int sc64_stage_save_authenticate(Sc64StageSaveAuth *auth,const void *candidate,
                                        uint32_t outer,uint32_t inner,
                                        const Sc64StageTagPair *allowed,unsigned count)
{
    unsigned found=0,features=0;
    if (!auth) return 0;
    *auth=(Sc64StageSaveAuth){0};
    if (!candidate || !outer || !inner || !allowed || !count) return 0;
    for (unsigned i=0;i<count;i++) {
        if (!allowed[i].outer || !allowed[i].inner) return 0;
        for (unsigned j=0;j<i;j++)
            if (allowed[i].outer==allowed[j].outer) return 0;
        if (allowed[i].outer==outer && allowed[i].inner==inner) {
            found=1;features=allowed[i].features;
        }
    }
    if (!found) return 0;
    *auth=(Sc64StageSaveAuth){candidate,outer,inner,features};
    return 1;
}
static int sc64_stage_save_auth_matches(const Sc64StageSaveAuth *auth,const void *candidate,
                                         uint32_t unchanged_inner,unsigned required)
{
    return auth && candidate && auth->candidate==candidate && auth->outer &&
           auth->inner==unchanged_inner && (auth->features & required)==required;
}
#endif
