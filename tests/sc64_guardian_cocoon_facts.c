/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/sc64_guardian_cocoon.h"
int main(void)
{
    Sc64GuardianCocoonRow r = { 12, 59, 100, 200, 44, 750, 0, 0, 1 };
    assert(sc64_guardian_cocoon_valid(&r, 12));
    r.target = 43; assert(!sc64_guardian_cocoon_valid(&r, 12)); r.target = 44;
    r.raw_type = 44; assert(!sc64_guardian_cocoon_valid(&r, 12)); r.raw_type = 59;
    r.work_timer = 0; assert(!sc64_guardian_cocoon_valid(&r, 12)); r.work_timer = 751;
    assert(!sc64_guardian_cocoon_valid(&r, 12)); r.work_timer = 1;
    r.maxhp = 150; assert(!sc64_guardian_cocoon_valid(&r, 12)); r.maxhp = 200;
    r.carry = 1; assert(!sc64_guardian_cocoon_valid(&r, 12));
    assert(sc64_guardian_scaled_hp(60, 120, 200) == 100);
    assert(sc64_guardian_scaled_hp(100, 200, 150) == 75);
    assert(sc64_guardian_scaled_hp(1, 200, 120) == 1);
    assert(!sc64_guardian_scaled_hp(201, 200, 120));
    return 0;
}
