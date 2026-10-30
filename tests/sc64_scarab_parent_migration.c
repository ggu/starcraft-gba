/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_scarab_fields.h"
int main(void)
{
    int c=111,t=222; Sc64ScarabParentFields p;
    assert(sc64_scarab_parent_convert(SC64_SCARAB_PARENT_LEGACY,83,0,0,&c,&t));
    assert(sc64_scarab_parent_fields_unpack(c,t,&p));
    assert(p.queued==0 && p.active_slot==SC64_SCARAB_FIELD_NO_SLOT && p.cooldown==0 && p.progress==0);
    assert(sc64_scarab_parent_convert(SC64_SCARAB_PARENT_PACKED,83,c,t,&c,&t));
    int saved_c=c,saved_t=t;
    assert(!sc64_scarab_parent_convert(SC64_SCARAB_PARENT_LEGACY,83,c,t,&c,&t));
    assert(c==saved_c && t==saved_t);
    assert(!sc64_scarab_parent_convert(SC64_SCARAB_PARENT_PACKED,83,0,0,&c,&t));
    assert(!sc64_scarab_parent_convert(SC64_SCARAB_PARENT_LEGACY,85,0,0,&c,&t));
    assert(!sc64_scarab_parent_convert(SC64_SCARAB_PARENT_LEGACY,83,0,1,&c,&t));
    assert(!sc64_scarab_parent_convert(0,83,0,0,&c,&t));
    assert(!sc64_scarab_parent_convert(SC64_SCARAB_PARENT_LEGACY,83,0,0,&c,&c));
    assert(c==saved_c && t==saved_t);
    puts("explicit-format Reaver parent field conversion passed");
}
