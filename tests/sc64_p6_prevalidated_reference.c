#include "../starcraft/sc64_p6_adapter.h"

int sc64_p6_full_reference_tick(void *state, const void *ops, int user_paused)
{
    return sc64_p6_adapter_tick((Sc64P6RuntimeState *)state,
                                (const Sc64P6WorldOps *)ops,user_paused);
}
