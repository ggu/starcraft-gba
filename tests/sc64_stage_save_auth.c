/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_stage_save_auth.h"
int main(void)
{
    enum { EGG=1, GUARDIAN=2 };
    const Sc64StageTagPair pairs[]={ {1,11,0},{2,12,EGG},{3,13,GUARDIAN},{4,14,EGG|GUARDIAN} };
    Sc64StageSaveAuth auth;int candidate=0,other=0;
    assert(sc64_stage_save_authenticate(&auth,&candidate,4,14,pairs,4));
    assert(sc64_stage_save_auth_matches(&auth,&candidate,14,EGG|GUARDIAN));
    assert(!sc64_stage_save_auth_matches(&auth,&other,14,EGG));
    assert(!sc64_stage_save_auth_matches(&auth,&candidate,13,EGG));
    assert(!sc64_stage_save_authenticate(&auth,&candidate,2,13,pairs,4));
    assert(!sc64_stage_save_auth_matches(&auth,&candidate,14,EGG)); /* reset after failure */
    assert(!sc64_stage_save_authenticate(&auth,&candidate,99,14,pairs,4));
    assert(sc64_stage_save_authenticate(&auth,&candidate,1,11,pairs,4));
    assert(!sc64_stage_save_auth_matches(&auth,&candidate,11,EGG));
    const Sc64StageTagPair ambiguous[]={ {2,12,EGG},{2,13,GUARDIAN} };
    assert(!sc64_stage_save_authenticate(&auth,&candidate,2,12,ambiguous,2));
    assert(!sc64_stage_save_auth_matches(&auth,&candidate,11,0));
    puts("detached stage save tag authentication passed");return 0;
}
