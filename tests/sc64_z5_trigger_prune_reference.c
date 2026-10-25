#ifdef SC64_Z5_PRUNE_STARTED
#undef SC64_Z5_PRUNE_STARTED
#endif
#include "../starcraft/sc64_z5_adapter.h"

/* Same adapter API compiled without pruning to serve as an executable oracle. */
int sc64_z5_reference_tick(void *state, const void *ops, int user_paused)
{
    return sc64_z5_adapter_tick((Sc64Z5RuntimeState *)state,
                                (const Sc64Z5WorldOps *)ops, user_paused);
}
