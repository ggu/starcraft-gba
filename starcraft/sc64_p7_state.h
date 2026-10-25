#ifndef SC64_P7_STATE_H
#define SC64_P7_STATE_H

/* Compact P7-only persistent supplement. The two enemy force wallets stay
 * keyed by their original source slots because the current engine maps both
 * forces to the same live combat owner. No source actor ownership is changed. */
#include "sc64_p7_adapter.h"

enum { SC64_P7_SOURCE_SLOT_COUNT = 8 };

typedef struct {
    Sc64P7RuntimeState runtime;
    u32 source_minerals[SC64_P7_SOURCE_SLOT_COUNT];
    u32 source_gas[SC64_P7_SOURCE_SLOT_COUNT];
    u16 moved_location3[4]; /* Source-pixel bounds; set only by MoveLocation. */
    u8 moved_location3_valid;
    u8 created_mask;         /* Trigger 5 CreateUnit action indices 8..10. */
    u8 reserved[3];
} Sc64P7PersistentState;

_Static_assert(sizeof(Sc64P7PersistentState) <= 148,
               "P7 persistent runtime state bound");

static inline int sc64_p7_persistent_state_valid(const Sc64P7PersistentState *s)
{
    if (!s || !sc64_p7_runtime_state_valid(&s->runtime) ||
        s->moved_location3_valid > 1 || (s->created_mask & 0xf8u) ||
        s->reserved[0] || s->reserved[1] || s->reserved[2]) return 0;
    if (!s->moved_location3_valid &&
        (s->moved_location3[0] || s->moved_location3[1] ||
         s->moved_location3[2] || s->moved_location3[3])) return 0;
    if (s->moved_location3_valid &&
        (s->moved_location3[0] >= s->moved_location3[2] ||
         s->moved_location3[1] >= s->moved_location3[3] ||
         s->moved_location3[2] > SC64_P7_MAP_COLS * 32u ||
         s->moved_location3[3] > SC64_P7_MAP_ROWS * 32u)) return 0;
    /* Authenticated SetResources actions initialize two Force wallets to
       10,000; bounded AI may subsequently spend those source-local funds. */
    for (unsigned i=0;i<SC64_P7_SOURCE_SLOT_COUNT;++i)
        if (s->source_minerals[i] > 65535u || s->source_gas[i] > 65535u)
            return 0;
    for (unsigned i=0;i<SC64_P7_SOURCE_SLOT_COUNT;++i)
        if (i != 0 && i != 4 && (s->source_minerals[i] || s->source_gas[i]))
            return 0;
    /* Both authored Force grants set minerals AND gas to 10,000. Wallets
     * are distinct even when combat ownership shares the ENEMY bucket. */
    if ((!sc64_p7_trigger_started(&s->runtime.model,6) &&
         (s->source_minerals[0] || s->source_gas[0])) ||
        (!sc64_p7_trigger_started(&s->runtime.model,7) &&
         (s->source_minerals[4] || s->source_gas[4])) ||
        s->source_minerals[0] > 10000u || s->source_minerals[4] > 10000u ||
        s->source_gas[0] > 10000u || s->source_gas[4] > 10000u) return 0;
    {
        u8 expected=0;
        if (sc64_p7_trigger_started(&s->runtime.model,5)) {
            u8 cursor=s->runtime.model.action_cursor[5];
            for (unsigned i=0;i<3;++i)
                if (!cursor || cursor >= (u8)(9u+i)) expected|=(u8)(1u<<i);
        }
        if (s->created_mask != expected) return 0;
    }
    return 1;
}

#ifdef SC64_P7_PREVALIDATED_TICK
/* Hot-tick counterpart to the full save/load validator above. The source
 * program is authenticated at init/rebind, but every mutable save field is
 * still checked on each tick. Save candidates must use the full validator. */
static inline int sc64_p7_persistent_state_valid_prevalidated(
    const Sc64P7PersistentState *s)
{
    if (!s || !sc64_p7_runtime_state_valid_prevalidated(&s->runtime) ||
        s->moved_location3_valid>1 || (s->created_mask&0xf8u) ||
        s->reserved[0] || s->reserved[1] || s->reserved[2]) return 0;
    if (!s->moved_location3_valid &&
        (s->moved_location3[0] || s->moved_location3[1] ||
         s->moved_location3[2] || s->moved_location3[3])) return 0;
    if (s->moved_location3_valid &&
        (s->moved_location3[0]>=s->moved_location3[2] ||
         s->moved_location3[1]>=s->moved_location3[3] ||
         s->moved_location3[2]>SC64_P7_MAP_COLS*32u ||
         s->moved_location3[3]>SC64_P7_MAP_ROWS*32u)) return 0;
    for (unsigned i=0;i<SC64_P7_SOURCE_SLOT_COUNT;++i)
        if (s->source_minerals[i]>65535u || s->source_gas[i]>65535u ||
            (i!=0 && i!=4 && (s->source_minerals[i] || s->source_gas[i])))
            return 0;
    if ((!sc64_p7_trigger_started(&s->runtime.model,6) &&
         (s->source_minerals[0] || s->source_gas[0])) ||
        (!sc64_p7_trigger_started(&s->runtime.model,7) &&
         (s->source_minerals[4] || s->source_gas[4])) ||
        s->source_minerals[0]>10000u || s->source_minerals[4]>10000u ||
        s->source_gas[0]>10000u || s->source_gas[4]>10000u) return 0;
    {
        u8 expected=0;
        if (sc64_p7_trigger_started(&s->runtime.model,5)) {
            u8 cursor=s->runtime.model.action_cursor[5];
            for (unsigned i=0;i<3;++i)
                if (!cursor || cursor >= (u8)(9u+i)) expected|=(u8)(1u<<i);
        }
        if (s->created_mask!=expected) return 0;
    }
    return 1;
}
#endif

#endif
