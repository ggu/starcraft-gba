#ifdef SC64_Z6_PRUNE_STARTED
#undef SC64_Z6_PRUNE_STARTED
#endif
#include "../starcraft/sc64_z6_adapter.h"

int sc64_z6_reference_tick(void *state,const void *ops,int user_paused)
{
    return sc64_z6_adapter_tick((Sc64Z6RuntimeState *)state,
                                (const Sc64Z6WorldOps *)ops,user_paused);
}
