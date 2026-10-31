/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/sc64_interceptor_pool_codec.h"
int main(void)
{
    assert(sc64_interceptor_save_hit_flags_supported(0));
#ifdef SC64_SHARED_DARK_SWARM_PROBE
    assert(sc64_interceptor_save_hit_flags_supported(1));
#else
    assert(!sc64_interceptor_save_hit_flags_supported(1));
#endif
    /* General hallucinated Interceptors remain unqualified in these builds. */
    assert(!sc64_interceptor_save_hit_flags_supported(2));
    assert(!sc64_interceptor_save_hit_flags_supported(3));
    assert(!sc64_interceptor_save_hit_flags_supported(4));
    return 0;
}
