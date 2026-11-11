#include <assert.h>
#include <stdio.h>
#define SC64_MID_ZERG_PERMISSIONS 1
#include "../starcraft/sc64_mid_zerg_permissions.h"
int main(void) {
    assert(!sc64_mid_zerg_permission_available(15,3,37));
    assert(!sc64_mid_zerg_permission_available(16,12,37));
    assert(!sc64_mid_zerg_permission_available(17,3,228));
    for(unsigned m=16;m<=17;++m)for(unsigned p=0;p<12;++p)for(unsigned u=0;u<228;++u)
        putchar('0'+sc64_mid_zerg_permission_available(m,p,u));
    return ferror(stdout)!=0;
}
